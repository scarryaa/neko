#ifndef FFI_TYPES_FWD_H
#define FFI_TYPES_FWD_H

#include <cstdint>

namespace neko {
class Editor;
class Selection;
class CursorPosition;
class ChangeSetFfi;
class ConfigSnapshotFfi;
class ConfigManager;
class FileTree;
class AppState;
class ShortcutsManager;
class ThemeManager;
class ScrollOffsetFfi;
class TabsSnapshot;
class TabSnapshot;

class TabCommandFfi;
class TabContextFfi;
class TabCommandStateFfi;

class FileOpenResult;

class CommandResultFfi;
enum class CommandKindFfi : uint8_t;

enum class AddCursorDirectionKind : uint8_t;
} // namespace neko

#endif
