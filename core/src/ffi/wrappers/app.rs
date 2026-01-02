use crate::{
    AppState, Buffer, ConfigManager, Tab, TabError, TabId,
    ffi::{
        CloseManyTabsResult, CloseTabOperationTypeFfi, CreateDocumentTabAndViewResultFfi,
        DocumentErrorFfi, MoveActiveTabResult, PinTabResult, ScrollOffsetFfi, TabSnapshot,
        TabSnapshotMaybe, TabsSnapshot,
    },
};
use std::path::Path;

pub fn new_app_state(
    root_path: &str,
    config_manager: &ConfigManager,
) -> std::io::Result<Box<AppState>> {
    let path_opt = if root_path.is_empty() {
        None
    } else {
        Some(root_path)
    };

    let state = AppState::new(config_manager, path_opt)
        .map_err(|e| std::io::Error::other(format!("Failed to initialize app state: {e}")))?;

    Ok(Box::new(state))
}

impl AppState {
    pub fn get_close_tab_ids_wrapper(
        &self,
        operation_type: CloseTabOperationTypeFfi,
        anchor_tab_id: u64,
        close_pinned: bool,
    ) -> Vec<u64> {
        let anchor = if anchor_tab_id == 0 {
            None
        } else {
            Some(TabId::new(anchor_tab_id).expect("Anchor tab id must not be 0"))
        };

        self.get_close_tab_ids(operation_type.into(), anchor, close_pinned)
            .expect("Unable to get close tab ids")
            .into_iter()
            .map(|id| id.into())
            .collect()
    }

    fn make_tab_snapshot(tab: &Tab) -> TabSnapshot {
        TabSnapshot {
            id: tab.get_id().into(),
            document_id: tab.get_document_id().into(),
            pinned: tab.get_is_pinned(),
            scroll_offsets: ScrollOffsetFfi {
                x: tab.get_scroll_offsets().0,
                y: tab.get_scroll_offsets().1,
            },
        }
    }

    pub(crate) fn get_tab_snapshot(&self, id: u64) -> TabSnapshotMaybe {
        if let Ok(tab) = self.get_tab(id.into()) {
            TabSnapshotMaybe {
                found: true,
                snapshot: Self::make_tab_snapshot(tab),
            }
        } else {
            TabSnapshotMaybe {
                found: false,
                snapshot: TabSnapshot::default(),
            }
        }
    }

    pub(crate) fn get_tabs_snapshot(&self) -> TabsSnapshot {
        let tabs: Vec<TabSnapshot> = self
            .get_tabs()
            .iter()
            .map(Self::make_tab_snapshot)
            .collect();

        let active_id = self.get_active_tab_id();
        let active_present = self.get_tabs().iter().any(|t| t.get_id() == active_id);

        TabsSnapshot {
            active_present,
            active_id: if active_present { active_id.into() } else { 0 },
            tabs,
        }
    }

    pub fn ensure_tab_for_path_wrapper(
        &mut self,
        path: &str,
        add_to_history: bool,
    ) -> Result<u64, DocumentErrorFfi> {
        self.ensure_tab_for_path(Path::new(path), add_to_history)
            .map(|tab_id| tab_id.into())
            .map_err(DocumentErrorFfi::from)
    }

    pub fn create_document_tab_and_view_wrapper(
        &mut self,
        title: String,
        add_tab_to_history: bool,
        activate_view: bool,
    ) -> CreateDocumentTabAndViewResultFfi {
        let (document_id, tab_id, view_id) =
            self.create_document_tab_and_view(Some(title), add_tab_to_history, activate_view);

        CreateDocumentTabAndViewResultFfi {
            document_id: document_id.into(),
            view_id: view_id.into(),
            tab_id: tab_id.into(),
        }
    }

    fn build_close_many_result(&self, success: bool, closed_ids: Vec<u64>) -> CloseManyTabsResult {
        let any_tabs_left = !self.get_tabs().is_empty();
        let active_id = if any_tabs_left {
            self.get_active_tab_id().into()
        } else {
            0
        };

        CloseManyTabsResult {
            success,
            closed_ids: closed_ids.into_iter().collect(),
            has_active: any_tabs_left,
            active_id,
        }
    }

    pub(crate) fn close_tabs_wrapper(
        self: &mut AppState,
        operation_type: CloseTabOperationTypeFfi,
        anchor_tab_id: u64,
        close_pinned: bool,
    ) -> CloseManyTabsResult {
        match self.close_tabs(operation_type.into(), anchor_tab_id.into(), close_pinned) {
            Ok(closed_ids) => self
                .build_close_many_result(true, closed_ids.iter().map(|id| (*id).into()).collect()),
            Err(_) => self.build_close_many_result(false, vec![]),
        }
    }

    pub(crate) fn save_document_wrapper(&mut self, id: u64) -> bool {
        self.save_document(id.into()).is_ok()
    }

    pub(crate) fn save_document_as_wrapper(&mut self, id: u64, path: &str) -> bool {
        self.save_document_as(id.into(), Path::new(path)).is_ok()
    }

    pub(crate) fn set_active_tab_wrapper(&mut self, id: u64) -> Result<(), TabError> {
        self.set_active_tab(id.into())
    }

    pub(crate) fn move_tab_wrapper(&mut self, from: usize, to: usize) -> bool {
        self.move_tab(from, to).is_ok()
    }

    pub fn move_active_tab_by_wrapper(
        self: &mut AppState,
        buffer: &mut Buffer,
        delta: i64,
        use_history: bool,
    ) -> MoveActiveTabResult {
        let result = self.move_active_tab_by(buffer, delta, use_history);

        // Try to find the tab with this id
        if let Some(found_id) = result.found_id {
            let snapshot = if let Ok(tab) = self.get_tab(found_id) {
                let mut snapshot = Self::make_tab_snapshot(tab);

                // Replace the scroll offsets
                if let Some(scroll_offsets) = result.scroll_offsets {
                    snapshot.scroll_offsets =
                        (scroll_offsets.x as i32, scroll_offsets.y as i32).into();
                }

                snapshot
            } else {
                eprintln!(
                    "move_active_tab_by_wrapper: no Tab with id {} (reopened: {})",
                    found_id, result.reopened_tab
                );
                TabSnapshot::default()
            };

            MoveActiveTabResult {
                id: found_id.into(),
                reopened: result.reopened_tab,
                snapshot,
            }
        } else {
            eprintln!("move_active_tab_by_wrapper: no Tab id found");
            MoveActiveTabResult {
                id: 0,
                reopened: false,
                snapshot: TabSnapshot::default(),
            }
        }
    }

    pub(crate) fn pin_tab_wrapper(&mut self, id: u64) -> PinTabResult {
        let from_index = match self.get_tabs().iter().position(|t| t.get_id() == id.into()) {
            Some(idx) => idx as u32,
            None => {
                return PinTabResult {
                    success: false,
                    from_index: 0,
                    to_index: 0,
                    snapshot: TabSnapshot::default(),
                };
            }
        };

        if self.pin_tab(id.into()).is_err() {
            return PinTabResult {
                success: false,
                from_index: 0,
                to_index: 0,
                snapshot: TabSnapshot::default(),
            };
        }

        let to_index = match self.get_tabs().iter().position(|t| t.get_id() == id.into()) {
            Some(idx) => idx as u32,
            None => {
                return PinTabResult {
                    success: false,
                    from_index: 0,
                    to_index: 0,
                    snapshot: TabSnapshot::default(),
                };
            }
        };

        let tab_ref = &self.get_tabs()[to_index as usize];
        let snapshot = Self::make_tab_snapshot(tab_ref);

        PinTabResult {
            success: true,
            from_index,
            to_index,
            snapshot,
        }
    }

    pub(crate) fn unpin_tab_wrapper(&mut self, id: u64) -> PinTabResult {
        let from_index = match self.get_tabs().iter().position(|t| t.get_id() == id.into()) {
            Some(idx) => idx as u32,
            None => {
                return PinTabResult {
                    success: false,
                    from_index: 0,
                    to_index: 0,
                    snapshot: TabSnapshot::default(),
                };
            }
        };

        if self.unpin_tab(id.into()).is_err() {
            return PinTabResult {
                success: false,
                from_index: 0,
                to_index: 0,
                snapshot: TabSnapshot::default(),
            };
        }

        let to_index = match self.get_tabs().iter().position(|t| t.get_id() == id.into()) {
            Some(idx) => idx as u32,
            None => {
                return PinTabResult {
                    success: false,
                    from_index: 0,
                    to_index: 0,
                    snapshot: TabSnapshot::default(),
                };
            }
        };

        let tab_ref = &self.get_tabs()[to_index as usize];
        let snapshot = Self::make_tab_snapshot(tab_ref);

        PinTabResult {
            success: true,
            from_index,
            to_index,
            snapshot,
        }
    }

    pub(crate) fn set_tab_scroll_offsets_wrapper(
        &mut self,
        id: u64,
        new_offsets: ScrollOffsetFfi,
    ) -> bool {
        self.set_tab_scroll_offsets(id.into(), (new_offsets.x, new_offsets.y))
            .is_ok()
    }
}
