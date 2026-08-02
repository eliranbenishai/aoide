// Spike: drive libmpv directly over FFI to answer two questions.
//
//   1. Does `ao=pcm` decode to a WAV file? (foundation of the precomputed
//      spectrogram route)
//   2. Do chained FFmpeg `equalizer` filters apply via the `af` property?
//
// Usage:
//   dart run eq_spike.dart <libmpv-2.dll> <input.wav> <output.wav> [afString]
//
// Exits non-zero on mpv error. Prints the `af` value mpv reports back.

import 'dart:ffi';
import 'dart:io';

import 'package:ffi/ffi.dart';

typedef _CreateNative = Pointer<Void> Function();

typedef _IntHandleNative = Int32 Function(Pointer<Void>);
typedef _IntHandleDart = int Function(Pointer<Void>);

typedef _SetOptNative = Int32 Function(
    Pointer<Void>, Pointer<Utf8>, Pointer<Utf8>);
typedef _SetOptDart = int Function(
    Pointer<Void>, Pointer<Utf8>, Pointer<Utf8>);

typedef _CmdNative = Int32 Function(Pointer<Void>, Pointer<Pointer<Utf8>>);
typedef _CmdDart = int Function(Pointer<Void>, Pointer<Pointer<Utf8>>);

typedef _GetPropStrNative = Pointer<Utf8> Function(
    Pointer<Void>, Pointer<Utf8>);

typedef _WaitEventNative = Pointer<Int32> Function(Pointer<Void>, Double);
typedef _WaitEventDart = Pointer<Int32> Function(Pointer<Void>, double);

typedef _ErrStrNative = Pointer<Utf8> Function(Int32);
typedef _ErrStrDart = Pointer<Utf8> Function(int);

const _eventShutdown = 1;
const _eventLogMessage = 2;
const _eventEndFile = 7;

void main(List<String> args) {
  if (args.length < 3) {
    stderr.writeln('need <dll> <input> <output> [af]');
    exit(64);
  }
  final dllPath = args[0];
  final input = args[1];
  final output = args[2];
  final af = args.length > 3 ? args[3] : '';

  final lib = DynamicLibrary.open(dllPath);

  final create = lib.lookupFunction<_CreateNative, _CreateNative>('mpv_create');
  final initialize =
      lib.lookupFunction<_IntHandleNative, _IntHandleDart>('mpv_initialize');
  final setOption =
      lib.lookupFunction<_SetOptNative, _SetOptDart>('mpv_set_option_string');
  final command = lib.lookupFunction<_CmdNative, _CmdDart>('mpv_command');
  final getPropString = lib.lookupFunction<_GetPropStrNative, _GetPropStrNative>(
      'mpv_get_property_string');
  final waitEvent =
      lib.lookupFunction<_WaitEventNative, _WaitEventDart>('mpv_wait_event');
  final errorString =
      lib.lookupFunction<_ErrStrNative, _ErrStrDart>('mpv_error_string');
  final terminate = lib
      .lookupFunction<_IntHandleNative, _IntHandleDart>('mpv_terminate_destroy');

  String err(int code) => errorString(code).toDartString();

  final handle = create();
  if (handle == nullptr) {
    stderr.writeln('mpv_create failed');
    exit(1);
  }

  void opt(String name, String value) {
    final n = name.toNativeUtf8();
    final v = value.toNativeUtf8();
    final rc = setOption(handle, n, v);
    calloc.free(n);
    calloc.free(v);
    if (rc < 0) {
      stderr.writeln('SET FAIL  $name=$value  -> ${err(rc)} ($rc)');
      exit(2);
    }
    stdout.writeln('set ok    $name=$value');
  }

  opt('config', 'no');
  // Logs on: mpv accepts an `af` string up front and only reports filter
  // construction failures at runtime, so silence here hides the real answer.
  opt('terminal', 'yes');
  opt('msg-level', 'all=v');
  opt('vid', 'no');
  opt('ao', 'pcm');
  opt('ao-pcm-file', output);
  opt('ao-pcm-waveheader', 'yes');
  // Decode as fast as the CPU allows rather than in real time.
  opt('untimed', 'yes');
  opt('audio-samplerate', '44100');
  // 5th arg overrides the sample format: the lavfi bridge needs `aresample` to
  // convert, and that filter is absent, so the format we hand it decides
  // whether the graph can configure at all.
  opt('audio-format', args.length > 4 ? args[4] : 's16');
  opt('audio-channels', 'stereo');
  opt('keep-open', 'no');
  if (af.isNotEmpty) opt('af', af);

  final rcInit = initialize(handle);
  if (rcInit < 0) {
    stderr.writeln('mpv_initialize -> ${err(rcInit)} ($rcInit)');
    exit(3);
  }

  // loadfile <input>
  final argv = calloc<Pointer<Utf8>>(3);
  argv[0] = 'loadfile'.toNativeUtf8();
  argv[1] = input.toNativeUtf8();
  argv[2] = nullptr;
  final rcCmd = command(handle, argv);
  if (rcCmd < 0) {
    stderr.writeln('loadfile -> ${err(rcCmd)} ($rcCmd)');
    exit(4);
  }

  var reportedAf = '(not read)';
  final deadline = DateTime.now().add(const Duration(seconds: 30));
  var done = false;
  while (!done && DateTime.now().isBefore(deadline)) {
    final ev = waitEvent(handle, 0.5);
    if (ev == nullptr) continue;
    final id = ev.value;
    if (id == _eventEndFile) {
      done = true;
    } else if (id == _eventShutdown) {
      done = true;
    } else if (id == _eventLogMessage) {
      // ignored; terminal=no keeps these quiet
    }
    if (reportedAf == '(not read)') {
      final n = 'af'.toNativeUtf8();
      final p = getPropString(handle, n);
      calloc.free(n);
      if (p != nullptr) reportedAf = p.toDartString();
    }
  }

  stdout.writeln('af reported by mpv: $reportedAf');
  stdout.writeln(done ? 'playback reached end-of-file' : 'TIMEOUT');
  terminate(handle);
  exit(done ? 0 : 5);
}
