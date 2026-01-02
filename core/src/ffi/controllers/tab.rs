use crate::{
    AppState, Buffer, Tab, TabError, TabId,
    ffi::{
        CloseManyTabsResult, CloseTabOperationTypeFfi, CreateDocumentTabAndViewResultFfi,
        MoveActiveTabResult, PinTabResult, ScrollOffsetFfi, TabSnapshot, TabSnapshotMaybe,
        TabsSnapshot,
    },
};
use std::{cell::RefCell, rc::Rc};

struct TabMetadata {
    modified: bool,
    title: String,
    path: String,
}

pub struct TabController {
    pub(crate) app_state: Rc<RefCell<AppState>>,
}

impl TabController {
    fn make_tab_snapshot(&self, tab: &Tab) -> TabSnapshot {
        let app_state = self.app_state.borrow();
        let tab_metadata = self.get_tab_metadata(&app_state, tab);

        TabSnapshot {
            id: tab.get_id().into(),
            document_id: tab.get_document_id().into(),
            pinned: tab.get_is_pinned(),
            modified: tab_metadata.modified,
            title: tab_metadata.title,
            path: tab_metadata.path.clone(),
            path_present: !tab_metadata.path.is_empty(),
            scroll_offsets: ScrollOffsetFfi {
                x: tab.get_scroll_offsets().0,
                y: tab.get_scroll_offsets().1,
            },
        }
    }

    fn get_tab_metadata(&self, app_state: &AppState, tab: &Tab) -> TabMetadata {
        let document = app_state
            .get_view_manager()
            .get_view(tab.get_view_id())
            .and_then(|v| {
                app_state
                    .get_document_manager()
                    .get_document(v.document_id())
            });

        if let Some(document) = document {
            let document_path = document
                .path
                .as_ref()
                .map(|path| path.to_string_lossy().to_string())
                .unwrap_or_default();

            TabMetadata {
                modified: document.modified,
                title: document.title.clone(),
                path: document_path,
            }
        } else {
            TabMetadata {
                modified: false,
                title: "Untitled".to_string(),
                path: String::new(),
            }
        }
    }

    pub fn get_close_tab_ids(
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

        self.app_state
            .borrow()
            .get_close_tab_ids(operation_type.into(), anchor, close_pinned)
            .expect("Unable to get close tab ids")
            .into_iter()
            .map(|id| id.into())
            .collect()
    }

    pub fn get_tab_snapshot(&self, id: u64) -> TabSnapshotMaybe {
        if let Ok(tab) = self.app_state.borrow().get_tab(id.into()) {
            TabSnapshotMaybe {
                found: true,
                snapshot: self.make_tab_snapshot(tab),
            }
        } else {
            TabSnapshotMaybe {
                found: false,
                snapshot: TabSnapshot::default(),
            }
        }
    }

    pub(crate) fn close_tabs(
        &self,
        operation_type: CloseTabOperationTypeFfi,
        anchor_tab_id: u64,
        close_pinned: bool,
    ) -> CloseManyTabsResult {
        match self.app_state.borrow_mut().close_tabs(
            operation_type.into(),
            anchor_tab_id.into(),
            close_pinned,
        ) {
            Ok(closed_ids) => self
                .build_close_many_result(true, closed_ids.iter().map(|id| (*id).into()).collect()),
            Err(_) => self.build_close_many_result(false, vec![]),
        }
    }

    pub fn get_tabs_snapshot(&self) -> TabsSnapshot {
        let tabs: Vec<TabSnapshot> = self
            .app_state
            .borrow()
            .get_tabs()
            .iter()
            .map(|tab| self.make_tab_snapshot(tab))
            .collect();

        let active_id = self.app_state.borrow().get_active_tab_id();
        let active_present = self
            .app_state
            .borrow()
            .get_tabs()
            .iter()
            .any(|t| t.get_id() == active_id);

        TabsSnapshot {
            active_present,
            active_id: if active_present { active_id.into() } else { 0 },
            tabs,
        }
    }

    pub fn close_tab(
        &self,
        operation_type: CloseTabOperationTypeFfi,
        anchor_tab_id: u64,
        close_pinned: bool,
    ) -> CloseManyTabsResult {
        match self.app_state.borrow_mut().close_tabs(
            operation_type.into(),
            anchor_tab_id.into(),
            close_pinned,
        ) {
            Ok(closed_ids) => self
                .build_close_many_result(true, closed_ids.iter().map(|id| (*id).into()).collect()),
            Err(_) => self.build_close_many_result(false, vec![]),
        }
    }

    fn build_close_many_result(&self, success: bool, closed_ids: Vec<u64>) -> CloseManyTabsResult {
        let any_tabs_left = !self.app_state.borrow().get_tabs().is_empty();
        let active_id = if any_tabs_left {
            self.app_state.borrow().get_active_tab_id().into()
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

    pub(crate) fn move_tab(&mut self, from: usize, to: usize) -> bool {
        self.app_state.borrow_mut().move_tab(from, to).is_ok()
    }

    pub fn move_active_tab_by(
        &mut self,
        buffer: &mut Buffer,
        delta: i64,
        use_history: bool,
    ) -> MoveActiveTabResult {
        let result = self
            .app_state
            .borrow_mut()
            .move_active_tab_by(buffer, delta, use_history);

        // Try to find the tab with this id
        if let Some(found_id) = result.found_id {
            let snapshot = if let Ok(tab) = self.app_state.borrow().get_tab(found_id) {
                let mut snapshot = self.make_tab_snapshot(tab);

                // Replace the scroll offsets
                if let Some(scroll_offsets) = result.scroll_offsets {
                    snapshot.scroll_offsets =
                        (scroll_offsets.x as i32, scroll_offsets.y as i32).into();
                }

                snapshot
            } else {
                eprintln!(
                    "move_active_tab_by: no Tab with id {} (reopened: {})",
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
            eprintln!("move_active_tab_by: no Tab id found");
            MoveActiveTabResult {
                id: 0,
                reopened: false,
                snapshot: TabSnapshot::default(),
            }
        }
    }

    pub(crate) fn pin_tab(&mut self, id: u64) -> PinTabResult {
        let from_index = match self
            .app_state
            .borrow()
            .get_tabs()
            .iter()
            .position(|t| t.get_id() == id.into())
        {
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

        if self.app_state.borrow_mut().pin_tab(id.into()).is_err() {
            return PinTabResult {
                success: false,
                from_index: 0,
                to_index: 0,
                snapshot: TabSnapshot::default(),
            };
        }

        let to_index = match self
            .app_state
            .borrow()
            .get_tabs()
            .iter()
            .position(|t| t.get_id() == id.into())
        {
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

        let app_state = self.app_state.borrow();
        let tab_ref = &app_state.get_tabs()[to_index as usize];
        let snapshot = self.make_tab_snapshot(tab_ref);

        PinTabResult {
            success: true,
            from_index,
            to_index,
            snapshot,
        }
    }

    pub(crate) fn unpin_tab(&mut self, id: u64) -> PinTabResult {
        let from_index = match self
            .app_state
            .borrow()
            .get_tabs()
            .iter()
            .position(|t| t.get_id() == id.into())
        {
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

        if self.app_state.borrow_mut().unpin_tab(id.into()).is_err() {
            return PinTabResult {
                success: false,
                from_index: 0,
                to_index: 0,
                snapshot: TabSnapshot::default(),
            };
        }

        let to_index = match self
            .app_state
            .borrow()
            .get_tabs()
            .iter()
            .position(|t| t.get_id() == id.into())
        {
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

        let app_state = self.app_state.borrow();
        let tab_ref = &app_state.get_tabs()[to_index as usize];
        let snapshot = self.make_tab_snapshot(tab_ref);

        PinTabResult {
            success: true,
            from_index,
            to_index,
            snapshot,
        }
    }

    pub fn set_active_tab(&mut self, id: u64) -> Result<(), TabError> {
        self.app_state.borrow_mut().set_active_tab(id.into())
    }

    pub fn create_document_tab_and_view(
        &mut self,
        title: String,
        add_tab_to_history: bool,
        activate_view: bool,
    ) -> CreateDocumentTabAndViewResultFfi {
        let (document_id, tab_id, view_id) = self
            .app_state
            .borrow_mut()
            .create_document_tab_and_view(Some(title), add_tab_to_history, activate_view);

        CreateDocumentTabAndViewResultFfi {
            document_id: document_id.into(),
            view_id: view_id.into(),
            tab_id: tab_id.into(),
        }
    }

    pub(crate) fn set_tab_scroll_offsets(&mut self, id: u64, new_offsets: ScrollOffsetFfi) -> bool {
        self.app_state
            .borrow_mut()
            .set_tab_scroll_offsets(id.into(), (new_offsets.x, new_offsets.y))
            .is_ok()
    }
}
