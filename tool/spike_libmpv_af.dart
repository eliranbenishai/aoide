// Spike: load staged full libmpv via dart:ffi and prove af get/set works.
// Run from worktree root after tool/fetch_full_libmpv.ps1:
//   dart run tool/spike_libmpv_af.dart

import 'dart:ffi';
import 'dart:io';

import 'package:ffi/ffi.dart';
import 'package:path/path.dart' as p;

typedef _CreateC = Pointer<Void> Function();
typedef _CreateDart = Pointer<Void> Function();
typedef _IntHandleC = Int32 Function(Pointer<Void>);
typedef _IntHandleDart = int Function(Pointer<Void>);
typedef _SetOptC = Int32 Function(Pointer<Void>, Pointer<Utf8>, Pointer<Utf8>);
typedef _SetOptDart = int Function(Pointer<Void>, Pointer<Utf8>, Pointer<Utf8>);
typedef _GetPropC = Pointer<Utf8> Function(Pointer<Void>, Pointer<Utf8>);
typedef _GetPropDart = Pointer<Utf8> Function(Pointer<Void>, Pointer<Utf8>);
typedef _FreeC = Void Function(Pointer<Void>);
typedef _FreeDart = void Function(Pointer<Void>);
typedef _DestroyC = Void Function(Pointer<Void>);
typedef _DestroyDart = void Function(Pointer<Void>);

void main() {
  final root = Directory.current.path;
  final dllPath =
      p.join(root, 'third_party', 'libmpv', 'windows', 'x86_64', 'libmpv-2.dll');
  if (!File(dllPath).existsSync()) {
    stderr.writeln('Missing $dllPath — run tool/fetch_full_libmpv.ps1');
    exit(2);
  }

  final bytes = File(dllPath).readAsBytesSync();
  final ascii = String.fromCharCodes(bytes);
  final hasDisable = ascii.contains('--disable-filters');
  final hasAr = ascii.contains('aresample');
  final hasEq = ascii.contains('equalizer');
  final sizeMb = (bytes.length / (1024 * 1024)).toStringAsFixed(1);
  stdout.writeln('DLL: $dllPath ($sizeMb MiB)');
  stdout.writeln(
    'markers: disable-filters=$hasDisable aresample=$hasAr equalizer=$hasEq',
  );
  if (hasDisable || !hasAr || !hasEq) {
    stderr.writeln('FAIL: DLL does not look like a full filter-enabled build');
    exit(3);
  }

  final lib = DynamicLibrary.open(dllPath);
  final create = lib.lookupFunction<_CreateC, _CreateDart>('mpv_create');
  final initialize =
      lib.lookupFunction<_IntHandleC, _IntHandleDart>('mpv_initialize');
  final setOption = lib
      .lookupFunction<_SetOptC, _SetOptDart>('mpv_set_option_string');
  final setProperty = lib
      .lookupFunction<_SetOptC, _SetOptDart>('mpv_set_property_string');
  final getProperty = lib
      .lookupFunction<_GetPropC, _GetPropDart>('mpv_get_property_string');
  final free = lib.lookupFunction<_FreeC, _FreeDart>('mpv_free');
  final destroy =
      lib.lookupFunction<_DestroyC, _DestroyDart>('mpv_terminate_destroy');

  final handle = create();
  if (handle == nullptr) {
    stderr.writeln('mpv_create failed');
    exit(4);
  }

  using((arena) {
    void opt(String k, String v) {
      final rc = setOption(handle, k.toNativeUtf8(allocator: arena),
          v.toNativeUtf8(allocator: arena));
      if (rc < 0) {
        throw StateError('set_option $k=$v failed: $rc');
      }
    }

    opt('config', 'no');
    opt('vo', 'null');
    opt('ao', 'null');
    opt('vid', 'no');
    final initRc = initialize(handle);
    if (initRc < 0) {
      throw StateError('mpv_initialize failed: $initRc');
    }

    const af =
        'lavfi=[equalizer=f=1000:t=h:w=200:g=5]';
    final setRc = setProperty(
      handle,
      'af'.toNativeUtf8(allocator: arena),
      af.toNativeUtf8(allocator: arena),
    );
    if (setRc < 0) {
      throw StateError('set_property af failed: $setRc');
    }
    final gotPtr = getProperty(handle, 'af'.toNativeUtf8(allocator: arena));
    final got = gotPtr == nullptr ? '' : gotPtr.toDartString();
    if (gotPtr != nullptr) free(gotPtr.cast());
    stdout.writeln('af set rc=$setRc get="$got"');
    if (!got.contains('equalizer')) {
      throw StateError('af property did not echo equalizer filter');
    }
  });

  destroy(handle);
  stdout.writeln('SPIKE OK: full libmpv loads; af get/set works');
}
