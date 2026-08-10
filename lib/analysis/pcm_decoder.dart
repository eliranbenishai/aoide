import 'dart:ffi';
import 'dart:io';
import 'dart:typed_data';

import 'package:ffi/ffi.dart';
import 'package:path/path.dart' as p;

import '../platform/libmpv_bundle.dart';
import 'wav_reader.dart';

/// Decoded mono PCM buffer ready for STFT.
class PcmBuffer {
  const PcmBuffer({
    required this.samples,
    required this.sampleRateHz,
  });

  final Float64List samples;
  final int sampleRateHz;
}

typedef PcmLoader = Future<PcmBuffer> Function(String path);

/// Decode an audio file to PCM via a throwaway mpv `ao=pcm` instance.
class MpvPcmDecoder {
  MpvPcmDecoder({String? libraryPath});

  String? _libraryPath;

  Future<PcmBuffer> decode(String path) async {
    final libPath = _libraryPath ??=
        LibmpvBundle.resolveLibraryPath() ??
            (Platform.isWindows
                ? p.join(
                    Directory.current.path,
                    'third_party',
                    'libmpv',
                    'windows',
                    'x86_64',
                    'libmpv-2.dll',
                  )
                : null);
    if (libPath == null || !File(libPath).existsSync()) {
      throw StateError('libmpv not found for PCM decode');
    }

    final work = Directory.systemTemp.createTempSync('tramp_pcm_');
    final outPath = p.join(work.path, 'out.wav');
    try {
      _renderToWav(
        libraryPath: libPath,
        inputPath: path,
        outputPath: outPath,
      );
      final bytes = await File(outPath).readAsBytes();
      final wav = const WavReader().read(Uint8List.fromList(bytes));
      return PcmBuffer(samples: wav.samples, sampleRateHz: wav.sampleRateHz);
    } finally {
      try {
        work.deleteSync(recursive: true);
      } catch (_) {}
    }
  }
}

typedef _CreateC = Pointer<Void> Function();
typedef _CreateDart = Pointer<Void> Function();
typedef _IntHandleC = Int32 Function(Pointer<Void>);
typedef _IntHandleDart = int Function(Pointer<Void>);
typedef _SetOptC = Int32 Function(Pointer<Void>, Pointer<Utf8>, Pointer<Utf8>);
typedef _SetOptDart = int Function(Pointer<Void>, Pointer<Utf8>, Pointer<Utf8>);
typedef _CmdC = Int32 Function(Pointer<Void>, Pointer<Utf8>);
typedef _CmdDart = int Function(Pointer<Void>, Pointer<Utf8>);
typedef _WaitC = Pointer<_MpvEvent> Function(Pointer<Void>, Double);
typedef _WaitDart = Pointer<_MpvEvent> Function(Pointer<Void>, double);
typedef _DestroyC = Void Function(Pointer<Void>);
typedef _DestroyDart = void Function(Pointer<Void>);

final class _MpvEvent extends Struct {
  @Int32()
  external int eventId;
  @Int32()
  external int error;
  @Uint64()
  external int replyUserdata;
  external Pointer<Void> data;
}

const _eventNone = 0;
const _eventShutdown = 1;
const _eventEndFile = 7;

void _renderToWav({
  required String libraryPath,
  required String inputPath,
  required String outputPath,
}) {
  final lib = DynamicLibrary.open(libraryPath);
  final create = lib.lookupFunction<_CreateC, _CreateDart>('mpv_create');
  final initialize =
      lib.lookupFunction<_IntHandleC, _IntHandleDart>('mpv_initialize');
  final setOption =
      lib.lookupFunction<_SetOptC, _SetOptDart>('mpv_set_option_string');
  final command = lib.lookupFunction<_CmdC, _CmdDart>('mpv_command_string');
  final waitEvent = lib.lookupFunction<_WaitC, _WaitDart>('mpv_wait_event');
  final destroy =
      lib.lookupFunction<_DestroyC, _DestroyDart>('mpv_terminate_destroy');

  final handle = create();
  if (handle == nullptr) {
    throw StateError('mpv_create failed');
  }

  try {
    using((arena) {
      int opt(String k, String v) => setOption(
            handle,
            k.toNativeUtf8(allocator: arena),
            v.toNativeUtf8(allocator: arena),
          );

      void check(int rc, String what) {
        if (rc < 0) throw StateError('$what failed: $rc');
      }

      check(opt('config', 'no'), 'config');
      check(opt('vo', 'null'), 'vo');
      check(opt('vid', 'no'), 'vid');
      check(opt('audio-display', 'no'), 'audio-display');
      check(opt('untimed', 'yes'), 'untimed');
      check(opt('ao', 'pcm'), 'ao');
      check(opt('ao-pcm-waveheader', 'yes'), 'ao-pcm-waveheader');
      check(opt('ao-pcm-file', outputPath), 'ao-pcm-file');
      // Shrink analysis buffer when downmix/resample engage.
      check(opt('audio-channels', 'mono'), 'audio-channels');
      check(initialize(handle), 'mpv_initialize');

      final load =
          'loadfile "${inputPath.replaceAll(r'\', '/')}" replace';
      check(command(handle, load.toNativeUtf8(allocator: arena)), 'loadfile');

      final deadline = DateTime.now().add(const Duration(seconds: 120));
      while (DateTime.now().isBefore(deadline)) {
        final ev = waitEvent(handle, 0.25);
        if (ev == nullptr) continue;
        final id = ev.ref.eventId;
        if (id == _eventEndFile || id == _eventShutdown) return;
        if (id == _eventNone) continue;
      }
      throw StateError('timeout waiting for PCM end-file');
    });
  } finally {
    destroy(handle);
  }
}
