use super::JumpCommand;
use crate::Config;

pub fn resolve_jump_key(key: &str, config: &Config) -> Option<JumpCommand> {
    // Builtins
    if let Some(cmd) = JumpCommand::from_spec(key) {
        return Some(cmd);
    }

    // User aliases
    if let Some(spec) = config.jump.aliases.get(key) {
        return JumpCommand::from_spec(spec);
    }

    // Not found
    None
}
