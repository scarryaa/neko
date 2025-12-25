#ifndef FFI_TYPES_FWD_H
#define FFI_TYPES_FWD_H

#include <cstdint>

namespace neko {
class Editor;
class Selection;
class CursorPosition;
class ChangeSetFfi;
class ConfigManager;
class FileTree;
class AppState;
class ShortcutsManager;
class ThemeManager;
class ScrollOffsetFfi;
class TabsSnapshot;

class TabContextFfi;
class TabCommandStateFfi;

enum class AddCursorDirectionKind : uint8_t;
} // namespace neko

#endif
