/// Temporary mutual-exclusion region for the legacy single-window shell.
///
/// Maps onto [TrampSettings] equalizer/playlist [WindowFrameState.visible]
/// until the multi-window host replaces this swap model.
enum LowerRegion { equalizer, playlist }
