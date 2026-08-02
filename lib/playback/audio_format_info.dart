/// Stream properties shown on the display well.
///
/// Every field is nullable because they are only known once a track is open and
/// decoding has started; the labels render placeholders until then.
class AudioFormatInfo {
  const AudioFormatInfo({this.bitrateKbps, this.sampleRateHz, this.channels});

  static const unknown = AudioFormatInfo();

  final int? bitrateKbps;
  final int? sampleRateHz;
  final int? channels;

  String get bitrateLabel =>
      bitrateKbps == null ? '— kbps' : '$bitrateKbps kbps';

  String get sampleRateLabel => sampleRateHz == null
      ? '— kHz'
      : '${(sampleRateHz! / 1000).round()} kHz';

  String get channelLabel => switch (channels) {
        null => '—',
        1 => 'mono',
        2 => 'stereo',
        final n => '$n ch',
      };

  @override
  bool operator ==(Object other) =>
      other is AudioFormatInfo &&
      other.bitrateKbps == bitrateKbps &&
      other.sampleRateHz == sampleRateHz &&
      other.channels == channels;

  @override
  int get hashCode => Object.hash(bitrateKbps, sampleRateHz, channels);
}
