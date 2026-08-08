# macOS full libmpv override

`media_kit_libs_macos_audio` downloads **audio-default** (slim) xcframeworks in
its pod `Makefile`. Tramp needs **audio-full**.

1. Run `./tool/fetch_full_libmpv.sh` (stages under `third_party/libmpv/macos/universal/`).
2. After `pod install` / first Flutter macOS build creates the plugin Frameworks,
   replace the pod’s `Frameworks/*.xcframework` with the staged full build
   (or point a vendored copy into the Runner).
3. Confirm `LibmpvBundle.verify()` does not see `--disable-filters` in the
   embedded binary before claiming audible EQ.

Pinned URL: `third_party/libmpv/pins.json` → `macos.universal`.
