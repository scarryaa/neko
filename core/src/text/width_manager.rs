#[derive(Debug, Default)]
pub struct WidthManager {
    line_widths: Vec<f64>,
    max_width: f64,
    max_width_line: usize,
}

impl WidthManager {
    pub fn new() -> Self {
        Self {
            line_widths: vec![-1.0],
            max_width: 0.0,
            max_width_line: 0,
        }
    }

    // Getters
    pub fn needs_width_measurement(&self, line_idx: usize) -> bool {
        if line_idx >= self.line_widths.len() {
            return false;
        }

        self.line_widths[line_idx] == -1.0
    }

    pub fn max_width(&self) -> f64 {
        self.max_width
    }

    // Setters
    pub fn invalidate_line_width(&mut self, line_idx: usize) {
        if line_idx >= self.line_widths.len() {
            return;
        }

        self.line_widths[line_idx] = -1.0;

        if line_idx == self.max_width_line {
            self.recalculate_max_width();
        }
    }

    fn recalculate_max_width(&mut self) {
        self.max_width = 0.0;
        self.max_width_line = 0;

        for (idx, &width) in self.line_widths.iter().enumerate() {
            if width > self.max_width {
                self.max_width = width;
                self.max_width_line = idx;
            }
        }
    }

    pub fn update_line_width(&mut self, line_idx: usize, width: f64) {
        if line_idx >= self.line_widths.len() {
            return;
        }

        let old = self.line_widths[line_idx];
        self.line_widths[line_idx] = width;

        if width > self.max_width {
            self.max_width = width;
            self.max_width_line = line_idx;
        } else if line_idx == self.max_width_line && width < old {
            self.recalculate_max_width();
        }
    }

    pub fn sync_line_widths(&mut self, line_count: usize) {
        match line_count.cmp(&self.line_widths.len()) {
            std::cmp::Ordering::Greater => {
                // Buffer grew, add invalidated entries
                self.line_widths.resize(line_count, -1.0);
            }
            std::cmp::Ordering::Less => {
                // Buffer shrunk, remove entries
                self.line_widths.truncate(line_count);

                // If we removed the max width line, recalculate
                if self.max_width_line >= line_count {
                    self.recalculate_max_width();
                }
            }
            std::cmp::Ordering::Equal => {}
        }
    }

    pub fn clear_and_rebuild_line_widths(&mut self, line_count: usize) {
        self.line_widths.clear();
        self.line_widths.resize(line_count, -1.0);
        self.max_width = 0.0;
        self.max_width_line = 0;
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    fn assert_f64_eq(a: f64, b: f64) {
        assert!((a - b).abs() < 1e-9, "{a} != {b}");
    }

    #[test]
    fn new_starts_with_one_invalid_entry_and_zero_max() {
        let w = WidthManager::new();

        assert!(w.needs_width_measurement(0));
        assert!(!w.needs_width_measurement(1));
        assert_f64_eq(w.max_width(), 0.0);
    }

    #[test]
    fn clear_and_rebuild_resets_state_and_sets_all_invalid() {
        let mut w = WidthManager::new();

        w.update_line_width(0, 10.0);
        w.clear_and_rebuild_line_widths(3);

        assert!(w.needs_width_measurement(0));
        assert!(w.needs_width_measurement(1));
        assert!(w.needs_width_measurement(2));
        assert_f64_eq(w.max_width(), 0.0);
    }

    #[test]
    fn sync_grows_by_adding_invalid_entries() {
        let mut w = WidthManager::new();

        w.update_line_width(0, 5.0);
        w.sync_line_widths(4);

        // Existing is preserved
        assert!(!w.needs_width_measurement(0));

        // New ones are invalid
        assert!(w.needs_width_measurement(1));
        assert!(w.needs_width_measurement(2));
        assert!(w.needs_width_measurement(3));
        assert_f64_eq(w.max_width(), 5.0);
    }

    #[test]
    fn sync_shrinks_by_truncating_and_recalculates_if_max_line_removed() {
        let mut w = WidthManager::new();

        w.sync_line_widths(3);
        w.update_line_width(0, 5.0);
        w.update_line_width(1, 12.0); // Max
        w.update_line_width(2, 7.0);

        // Shrink so that max line (1) is removed
        w.sync_line_widths(1);

        assert_f64_eq(w.max_width(), 5.0);
    }

    #[test]
    fn invalidate_marks_line_invalid_and_recalculates_if_it_was_max() {
        let mut w = WidthManager::new();

        w.sync_line_widths(3);
        w.update_line_width(0, 5.0);
        w.update_line_width(1, 12.0); // Max
        w.update_line_width(2, 7.0);

        w.invalidate_line_width(1);

        assert!(w.needs_width_measurement(1));

        // Max should now be 7.0 (ln 2) because invalidated (ln 1) is -1
        assert_f64_eq(w.max_width(), 7.0);
    }

    #[test]
    fn invalidate_does_not_panic_or_change_state_when_oob() {
        let mut w = WidthManager::new();

        w.update_line_width(0, 10.0);
        w.invalidate_line_width(999);

        assert_f64_eq(w.max_width(), 10.0);
        assert!(!w.needs_width_measurement(0));
    }

    #[test]
    fn needs_width_measurement_is_false_when_oob() {
        let w = WidthManager::new();
        assert!(!w.needs_width_measurement(999));
    }

    #[test]
    fn update_line_width_sets_value_and_updates_max_when_bigger() {
        let mut w = WidthManager::new();

        w.sync_line_widths(2);

        w.update_line_width(0, 10.0);
        assert_f64_eq(w.max_width(), 10.0);

        w.update_line_width(1, 20.0);
        assert_f64_eq(w.max_width(), 20.0);
    }

    #[test]
    fn update_line_width_ignores_oob() {
        let mut w = WidthManager::new();

        w.update_line_width(0, 10.0);
        w.update_line_width(123, 999.0);

        assert_f64_eq(w.max_width(), 10.0);
    }

    #[test]
    fn decreasing_width_of_current_max_line_should_recalculate_max() {
        let mut w = WidthManager::new();

        w.sync_line_widths(2);

        w.update_line_width(0, 10.0);
        w.update_line_width(1, 20.0);
        assert_f64_eq(w.max_width(), 20.0);

        // Make the max line smaller than line 0
        w.update_line_width(1, 5.0);

        // Max should become 10.0
        assert_f64_eq(w.max_width(), 10.0);
    }
}
