#ifndef COMMAND_PALETTE_TYPES_H
#define COMMAND_PALETTE_TYPES_H

#include <QString>
#include <cstdint>

enum class CommandPaletteMode : std::uint8_t { Command, Jump };

struct ShortcutHintRow {
  QString code;
  QString description;
};

#endif
