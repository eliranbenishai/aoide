import 'package:flutter_test/flutter_test.dart';
import 'package:tramp/playback/audio_format_info.dart';

void main() {
  test('unknown renders em-dash placeholders', () {
    expect(AudioFormatInfo.unknown.bitrateLabel, '— kbps');
    expect(AudioFormatInfo.unknown.sampleRateLabel, '— kHz');
    expect(AudioFormatInfo.unknown.channelLabel, '—');
  });

  test('known values render as the mockup shows them', () {
    const info = AudioFormatInfo(
      bitrateKbps: 128,
      sampleRateHz: 44100,
      channels: 2,
    );
    expect(info.bitrateLabel, '128 kbps');
    expect(info.sampleRateLabel, '44 kHz');
    expect(info.channelLabel, 'stereo');
  });

  test('mono and surround are named', () {
    expect(const AudioFormatInfo(channels: 1).channelLabel, 'mono');
    expect(const AudioFormatInfo(channels: 6).channelLabel, '6 ch');
  });

  test('sample rate rounds to the nearest kHz', () {
    expect(const AudioFormatInfo(sampleRateHz: 48000).sampleRateLabel, '48 kHz');
    expect(const AudioFormatInfo(sampleRateHz: 22050).sampleRateLabel, '22 kHz');
  });
}
