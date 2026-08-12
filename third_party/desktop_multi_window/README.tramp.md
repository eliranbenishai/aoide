# Tramp fork notes (`desktop_multi_window` 0.3.0)

Path override via `pubspec.yaml` `dependency_overrides`.

Linux `MultiWindowManager::Create` changes vs upstream:

- No GTK header bar (Tramp secondaries are frameless via `window_manager`)
- Default size **619×261** (75% of 825×348) instead of **1280×720**

Upstream template size left a huge black `FlView` when Dart `setSize` raced or
flaked (common under Distrobox + software GL).
