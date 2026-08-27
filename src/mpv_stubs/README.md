# libmpv SONAME stubs

The bundled `third_party/libmpv/linux/x86_64/libmpv.so` is a full mpv build.
Its `DT_NEEDED` list includes optional decoder/scripting libraries that Atomic
hosts (Fedora Bazzite / rpm-ostree) often do not ship:

- `libmujs.so.0.1`
- `liblua-5.1.so`
- `libuchardet.so.0`
- `libvapoursynth-script.so.0`
- `libXpresent.so.1`

These C files export dummy symbols with the right SONAMEs so the process can
load. Prefer a system `libmpv` (`pkg-config mpv`) when it is available — then
the stubs are not used.

Link the stub `.so` files into `aoide` itself (not only `-rpath`). ELF
`DT_RUNPATH` is not inherited: `libmpv.so` still looks for `libmujs.so.0.1` on
`LD_LIBRARY_PATH` / the cache / default paths. A Cursor AppImage terminal sets
`LD_LIBRARY_PATH` to its own libs, so an unlinked stub shows up as
`cursor.appimage: error while loading shared libraries: libmujs.so.0.1`.
