# One engine, several windows

Product windows stay five OS windows. They must share **one Flutter engine / one Dart isolate**. Extra engines are what made EQ+PL drag slow after Impeller was already off.

Pinned SDK stays Flutter **3.47.0**. Product host force-enables `isWindowingEnabled` in Dart before the binding. Do not move CI to master.

Linux spike: extra view is not the sludge; `RegularWindow` rebuild-on-move is choppy. Session rewrite: extra views on the existing engine; ignore configure during native drag.
