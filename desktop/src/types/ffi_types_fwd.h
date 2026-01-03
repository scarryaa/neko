#ifndef FFI_TYPES_FWD_H
#define FFI_TYPES_FWD_H

#include <cstdint>

namespace neko {
class Editor;
class EditorHandle;
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
class TabSnapshotMaybe;
class PinTabResult;
class NewTabResult;
class CloseManyTabsResult;
class CreateDocumentTabAndViewResultFfi;

class TabController;
class AppController;
class EditorController;

class FileOpenResult;
class MoveActiveTabResult;

class CommandFfi;
class CommandResultFfi;
enum class CommandKindFfi : uint8_t;

class JumpCommandFfi;
enum class JumpCommandKindFfi : uint8_t;
enum class LineTargetFfi : uint8_t;
enum class DocumentTargetFfi : uint8_t;

class TabCommandFfi;
class TabContextFfi;
class TabCommandStateFfi;

class FileExplorerCommandFfi;
class FileExplorerContextFfi;
class FileExplorerCommandStateFfi;

enum class AddCursorDirectionKind : uint8_t;
enum class CloseTabOperationTypeFfi : uint8_t;
class AddCursorDirectionFfi;
} // namespace neko

#endif
