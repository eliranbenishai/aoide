# One engine, several windows

Product windows stay five OS windows. They must share **one Flutter engine / one Dart isolate**. Extra engines are what made EQ+PL drag slow after Impeller was already off.

Pinned SDK stays Flutter **3.47.0**. The spike force-enables `isWindowingEnabled` in Dart (stable rejects the dart-define and has no `flutter config --enable-windowing`). Do not move CI to master for this.

Linux spike first (`tool/run_windowing_spike.sh`). Session rewrite only if that drag feels like solo-main.
