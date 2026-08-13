# Tramp fork notes (`desktop_multi_window` 0.3.0)

Path override via `pubspec.yaml` `dependency_overrides`.

Linux `MultiWindowManager::Create` changes vs upstream:

- No GTK header bar (Tramp secondaries are frameless via `window_manager`)
- Per-role 75% seed instead of **1280×720** (EQ **619×261**, playlist **805×522**,
  settings **390×315**, about **360×270**)
- Transparent `FlView` background so MockupShell rounded corners punch through

Windows/macOS Create uses the same per-role seeds (not 800×600).

Upstream template size left a huge black `FlView` when Dart `setSize` raced or
flaked (common under Distrobox + software GL). Using the EQ seed for about
left a black rectangle around the smaller chrome.
