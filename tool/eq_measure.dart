// Measurement gate for audible EQ on full libmpv.
//
// Generates a sine fixture, renders it through mpv with flat vs boosted EQ
// (same af string as production `buildEqualizerAf`), and asserts ≥6 dB of
// band-centre energy increase.
//
//   dart run tool/eq_measure.dart
//
// Requires staged full DLL: tool/fetch_full_libmpv.ps1

import 'dart:ffi';
import 'dart:io';
import 'dart:math' as math;
import 'dart:typed_data';

import 'package:ffi/ffi.dart';
import 'package:path/path.dart' as p;
import 'package:tramp/domain/equalizer_settings.dart';
import 'package:tramp/eq/equalizer_af.dart';

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

// mpv/client.h
const _eventNone = 0;
const _eventShutdown = 1;
const _eventEndFile = 7;

const _sampleRate = 48000;
const _durationSec = 0.5;
const _bandIndex = 4; // 1000 Hz
const _boostDb = 12.0;
const _minDeltaDb = 6.0;

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

  final freq = EqualizerSettings.bandFrequencies[_bandIndex];
  final work = Directory.systemTemp.createTempSync('tramp_eq_measure_');
  try {
    final tonePath = p.join(work.path, 'tone_${freq}hz.wav');
    final flatPath = p.join(work.path, 'out_flat.wav');
    final boostedPath = p.join(work.path, 'out_boosted.wav');
    _writeSineWav(tonePath, frequencyHz: freq.toDouble());

    final gains = List<double>.filled(EqualizerSettings.bandFrequencies.length, 0);
    gains[_bandIndex] = _boostDb;
    final flatAf = buildEqualizerAf(
      EqualizerSettings.flat.copyWith(enabled: true),
    );
    final boostedAf = buildEqualizerAf(
      EqualizerSettings(
        enabled: true,
        auto: false,
        preamp: 0,
        gains: gains,
      ),
    );
    stdout.writeln('af flat: "${flatAf.isEmpty ? "<clear>" : flatAf}"');
    stdout.writeln('af boosted: "$boostedAf"');

    final lib = DynamicLibrary.open(dllPath);
    _renderWithAf(
      lib: lib,
      inputPath: tonePath,
      outputPath: flatPath,
      af: flatAf,
    );
    _renderWithAf(
      lib: lib,
      inputPath: tonePath,
      outputPath: boostedPath,
      af: boostedAf,
    );

    final flatRms = _pcmRms(flatPath);
    final boostedRms = _pcmRms(boostedPath);
    if (flatRms <= 0 || boostedRms <= 0) {
      stderr.writeln(
        'FAIL: empty PCM (flatRms=$flatRms boostedRms=$boostedRms)',
      );
      exit(4);
    }
    final deltaDb = 20 * math.log(boostedRms / flatRms) / math.ln10;
    stdout.writeln(
      'band=${freq}Hz boost=${_boostDb}dB '
      'flatRms=${flatRms.toStringAsFixed(6)} '
      'boostedRms=${boostedRms.toStringAsFixed(6)} '
      'deltaDb=${deltaDb.toStringAsFixed(2)} '
      '(need ≥$_minDeltaDb)',
    );
    if (deltaDb < _minDeltaDb) {
      stderr.writeln(
        'FAIL: measured delta ${deltaDb.toStringAsFixed(2)} dB '
        '< $_minDeltaDb dB — EQ not audible on this libmpv',
      );
      exit(5);
    }
    stdout.writeln('EQ_MEASURE OK: band response ≥ $_minDeltaDb dB');
  } finally {
    try {
      work.deleteSync(recursive: true);
    } catch (_) {}
  }
}

void _renderWithAf({
  required DynamicLibrary lib,
  required String inputPath,
  required String outputPath,
  required String af,
}) {
  final create = lib.lookupFunction<_CreateC, _CreateDart>('mpv_create');
  final initialize =
      lib.lookupFunction<_IntHandleC, _IntHandleDart>('mpv_initialize');
  final setOption =
      lib.lookupFunction<_SetOptC, _SetOptDart>('mpv_set_option_string');
  final setProperty =
      lib.lookupFunction<_SetOptC, _SetOptDart>('mpv_set_property_string');
  final command =
      lib.lookupFunction<_CmdC, _CmdDart>('mpv_command_string');
  final waitEvent =
      lib.lookupFunction<_WaitC, _WaitDart>('mpv_wait_event');
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
      int prop(String k, String v) => setProperty(
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
      check(opt('ao', 'pcm'), 'ao');
      check(opt('ao-pcm-waveheader', 'yes'), 'ao-pcm-waveheader');
      check(opt('ao-pcm-file', outputPath), 'ao-pcm-file');
      check(initialize(handle), 'mpv_initialize');

      // Clear or apply the same af string production will set.
      check(prop('af', af), 'af');

      final load = 'loadfile ${inputPath.replaceAll(r'\', '/')} replace';
      check(command(handle, load.toNativeUtf8(allocator: arena)), 'loadfile');

      final deadline =
          DateTime.now().add(const Duration(seconds: 15));
      while (DateTime.now().isBefore(deadline)) {
        final ev = waitEvent(handle, 0.25);
        if (ev == nullptr) continue;
        final id = ev.ref.eventId;
        if (id == _eventEndFile || id == _eventShutdown) return;
        if (id == _eventNone) continue;
      }
      throw StateError('timeout waiting for end-file ($outputPath)');
    });
  } finally {
    destroy(handle);
  }
}

void _writeSineWav(String path, {required double frequencyHz}) {
  final sampleCount = (_sampleRate * _durationSec).round();
  final dataSize = sampleCount * 2; // mono s16le
  final bytes = BytesBuilder();
  void u16(int v) {
    bytes.addByte(v & 0xff);
    bytes.addByte((v >> 8) & 0xff);
  }

  void u32(int v) {
    bytes.addByte(v & 0xff);
    bytes.addByte((v >> 8) & 0xff);
    bytes.addByte((v >> 16) & 0xff);
    bytes.addByte((v >> 24) & 0xff);
  }

  bytes.add('RIFF'.codeUnits);
  u32(36 + dataSize);
  bytes.add('WAVE'.codeUnits);
  bytes.add('fmt '.codeUnits);
  u32(16);
  u16(1); // PCM
  u16(1); // mono
  u32(_sampleRate);
  u32(_sampleRate * 2);
  u16(2);
  u16(16);
  bytes.add('data'.codeUnits);
  u32(dataSize);

  final pcm = ByteData(dataSize);
  for (var i = 0; i < sampleCount; i++) {
    final t = i / _sampleRate;
    // Stay well below full-scale so +12 dB boost does not clip.
    final sample = 0.15 * math.sin(2 * math.pi * frequencyHz * t);
    pcm.setInt16(i * 2, (sample * 32767).round(), Endian.little);
  }
  bytes.add(pcm.buffer.asUint8List());
  File(path).writeAsBytesSync(bytes.toBytes());
}

double _pcmRms(String path) {
  final all = File(path).readAsBytesSync();
  if (all.length < 44) return 0;
  // Skip RIFF header; mpv waveheader writes a standard 44-byte header for PCM.
  var offset = 12;
  var dataOffset = -1;
  var dataSize = 0;
  while (offset + 8 <= all.length) {
    final id = String.fromCharCodes(all.sublist(offset, offset + 4));
    final size = ByteData.sublistView(all, offset + 4, offset + 8)
        .getUint32(0, Endian.little);
    if (id == 'data') {
      dataOffset = offset + 8;
      dataSize = size;
      break;
    }
    offset += 8 + size;
  }
  if (dataOffset < 0 || dataOffset + dataSize > all.length) return 0;

  final samples = dataSize ~/ 2;
  if (samples == 0) return 0;
  var sumSq = 0.0;
  final view = ByteData.sublistView(all, dataOffset, dataOffset + dataSize);
  for (var i = 0; i < samples; i++) {
    final s = view.getInt16(i * 2, Endian.little) / 32768.0;
    sumSq += s * s;
  }
  return math.sqrt(sumSq / samples);
}
