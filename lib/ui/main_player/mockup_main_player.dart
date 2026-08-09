import 'dart:async';
import 'dart:math' as math;

import 'package:flutter/widgets.dart' hide RepeatMode;
import 'package:path/path.dart' as p;

import '../../domain/repeat_mode.dart';
import '../../domain/tramp_settings.dart';
import '../../playback/audio_levels.dart';
import '../../playback/playback_controller.dart';
import '../../theme/tramp_metrics.dart';
import '../chrome/mockup/mockup_button.dart';
import '../chrome/mockup/mockup_icons.dart';
import '../chrome/mockup/mockup_led.dart';
import '../chrome/mockup/mockup_screen.dart';
import '../chrome/mockup/mockup_shell.dart';
import '../chrome/mockup/mockup_slider.dart';
import '../chrome/transport_icons.dart';
import '../format.dart';
import '../session/session_messages.dart';
import '../../look/look_materials.dart';
import '../../theme/look_scope.dart';

/// Mockup-faithful main player body (825×306) + optional full window chrome.
///
/// Absolute layout matches `player-mockup-2.html` (clutterbar, display, vol /
/// seek / transport rows). Clutterbar product letters: **O / A / I** only.
class MockupMainPlayer extends StatefulWidget {
  const MockupMainPlayer({
    super.key,
    required this.playback,
    required this.trackCount,
    this.forceMono = false,
    this.alwaysOnTop = false,
    this.equalizerVisible = true,
    this.playlistVisible = true,
    this.onSessionCommand,
    this.onOpenFiles,
    this.onOpenOptions,
    this.onShowTrackInfo,
    this.spectrumBars,
    this.spectrumPeaks,
    this.showElapsed = true,
  });

  static const bodySize = Size(825, 306);
  static const windowSize = TrampMetrics.mainPlayer;

  final PlaybackController playback;
  final int trackCount;
  final bool forceMono;
  final bool alwaysOnTop;
  final bool equalizerVisible;
  final bool playlistVisible;

  /// Host/window wiring: Mono → [MonoCommand], EQ/PL → [ToggleWindowCommand].
  final ValueChanged<SessionCommand>? onSessionCommand;

  final VoidCallback? onOpenFiles;
  final VoidCallback? onOpenOptions;
  final VoidCallback? onShowTrackInfo;

  /// When set, spectrum paints these fixed heights (goldens / demo).
  final List<double>? spectrumBars;
  final List<double>? spectrumPeaks;

  final bool showElapsed;

  @override
  State<MockupMainPlayer> createState() => _MockupMainPlayerState();
}

class _MockupMainPlayerState extends State<MockupMainPlayer> {
  late bool _showElapsed;

  @override
  void initState() {
    super.initState();
    _showElapsed = widget.showElapsed;
  }

  @override
  void didUpdateWidget(covariant MockupMainPlayer oldWidget) {
    super.didUpdateWidget(oldWidget);
    if (oldWidget.showElapsed != widget.showElapsed) {
      _showElapsed = widget.showElapsed;
    }
  }

  PlaybackController get playback => widget.playback;

  void _emit(SessionCommand command) => widget.onSessionCommand?.call(command);

  Future<void> _play() async {
    if (widget.trackCount <= 0 && playback.currentTrack == null) return;
    if (playback.playing) return;
    await playback.playPause();
  }

  Future<void> _pause() async {
    if (playback.playing) await playback.playPause();
  }

  @override
  Widget build(BuildContext context) {
    return SizedBox(
      width: MockupMainPlayer.bodySize.width,
      height: MockupMainPlayer.bodySize.height,
      child: ListenableBuilder(
        listenable: playback,
        builder: (context, _) => Stack(
          clipBehavior: Clip.none,
          children: [
            const Positioned(
              left: 9,
              bottom: 8,
              child: MockupRivet(),
            ),
            const Positioned(
              right: 9,
              bottom: 8,
              child: MockupRivet(),
            ),
            _Clutterbar(
              alwaysOnTop: widget.alwaysOnTop,
              onOptions: widget.onOpenOptions,
              onAlwaysOnTop: () => _emit(
                AlwaysOnTopCommand(!widget.alwaysOnTop),
              ),
              onInfo: widget.onShowTrackInfo,
            ),
            Positioned.fromRect(
              rect: const Rect.fromLTWH(96, 14, 705, 132),
              child: _DisplayWell(
                playback: playback,
                trackCount: widget.trackCount,
                showElapsed: _showElapsed,
                onToggleElapsed: () =>
                    setState(() => _showElapsed = !_showElapsed),
                spectrumBars: widget.spectrumBars,
                spectrumPeaks: widget.spectrumPeaks,
              ),
            ),
            Positioned(
              left: 22,
              top: 156,
              right: 22,
              height: 40,
              child: _VolumeRow(
                playback: playback,
                forceMono: widget.forceMono,
                equalizerVisible: widget.equalizerVisible,
                playlistVisible: widget.playlistVisible,
                onMono: () => _emit(MonoCommand(!widget.forceMono)),
                onEq: () => _emit(
                  ToggleWindowCommand(
                    window: WindowId.equalizer,
                    visible: !widget.equalizerVisible,
                  ),
                ),
                onPl: () => _emit(
                  ToggleWindowCommand(
                    window: WindowId.playlist,
                    visible: !widget.playlistVisible,
                  ),
                ),
              ),
            ),
            Positioned(
              left: 22,
              top: 206,
              right: 22,
              height: 32,
              child: _SeekRow(playback: playback),
            ),
            Positioned(
              left: 22,
              top: 246,
              right: 22,
              height: 50,
              child: _TransportRow(
                playback: playback,
                canTransport:
                    playback.currentTrack != null || widget.trackCount > 0,
                onPlay: () => unawaited(_play()),
                onPause: () => unawaited(_pause()),
                onOpenFiles: widget.onOpenFiles,
              ),
            ),
          ],
        ),
      ),
    );
  }
}

// ---------------------------------------------------------------------------
// Clutterbar
// ---------------------------------------------------------------------------

class _Clutterbar extends StatelessWidget {
  const _Clutterbar({
    required this.alwaysOnTop,
    this.onOptions,
    this.onAlwaysOnTop,
    this.onInfo,
  });

  final bool alwaysOnTop;
  final VoidCallback? onOptions;
  final VoidCallback? onAlwaysOnTop;
  final VoidCallback? onInfo;

  @override
  Widget build(BuildContext context) {
    return Positioned(
      left: 22,
      top: 18,
      width: 26,
      height: 129,
      child: DecoratedBox(
        decoration: BoxDecoration(
          borderRadius: BorderRadius.circular(3),
          gradient: LinearGradient(
            begin: Alignment.topCenter,
            end: Alignment.bottomCenter,
            colors: [
              Color(0x8C000000),
              Color(0x0AE2ECFF),
            ],
          ),
          boxShadow: const [
            BoxShadow(
              color: Color(0xCC000000),
              blurRadius: 4,
              offset: Offset(0, 2),
              blurStyle: BlurStyle.inner,
            ),
            BoxShadow(color: Color(0x14E2ECFF), offset: Offset(0, 1)),
          ],
        ),
        child: Padding(
          padding: const EdgeInsets.symmetric(vertical: 4),
          // Keep mockup's 5-slot spacing (O A I D V) so O/A/I land on the
          // same vertical positions as the HTML; D/V slots stay empty.
          child: Column(
            mainAxisAlignment: MainAxisAlignment.spaceBetween,
            children: [
              _ClutterGlyph(
                key: const Key('clutter-o'),
                letter: 'O',
                lit: true,
                semanticLabel: 'Options',
                onTap: onOptions,
              ),
              _ClutterGlyph(
                key: const Key('clutter-a'),
                letter: 'A',
                lit: alwaysOnTop,
                semanticLabel: 'Always on top',
                onTap: onAlwaysOnTop,
              ),
              _ClutterGlyph(
                key: const Key('clutter-i'),
                letter: 'I',
                lit: false,
                semanticLabel: 'Track info',
                onTap: onInfo,
              ),
              const SizedBox(width: 26, height: 20),
              const SizedBox(width: 26, height: 20),
            ],
          ),
        ),
      ),
    );
  }
}

class _ClutterGlyph extends StatelessWidget {
  const _ClutterGlyph({
    super.key,
    required this.letter,
    required this.lit,
    required this.semanticLabel,
    this.onTap,
  });

  final String letter;
  final bool lit;
  final String semanticLabel;
  final VoidCallback? onTap;

  @override
  Widget build(BuildContext context) {
    return Semantics(
      button: true,
      label: semanticLabel,
      child: GestureDetector(
        behavior: HitTestBehavior.opaque,
        onTap: onTap,
        child: SizedBox(
          width: 26,
          height: 20,
          child: Center(
            child: Text(
              letter,
              style: TextStyle(
                fontFamily: LookScope.of(context).chromeFamily,
                fontWeight: FontWeight.w700,
                fontSize: 12,
                height: 1,
                decoration: TextDecoration.none,
                color: lit
                    ? LookScope.of(context).palette.phosphorDefault
                    : LookScope.of(context).palette.phosphorDefault.withValues(alpha: 0.4),
                shadows: [
                  Shadow(
                    color: Color.fromRGBO(61, 231, 255, lit ? 0.7 : 0.25),
                    blurRadius: lit ? 9 : 6,
                  ),
                ],
              ),
            ),
          ),
        ),
      ),
    );
  }
}

// ---------------------------------------------------------------------------
// Display well
// ---------------------------------------------------------------------------

class _DisplayWell extends StatelessWidget {
  const _DisplayWell({
    required this.playback,
    required this.trackCount,
    required this.showElapsed,
    required this.onToggleElapsed,
    this.spectrumBars,
    this.spectrumPeaks,
  });

  final PlaybackController playback;
  final int trackCount;
  final bool showElapsed;
  final VoidCallback onToggleElapsed;
  final List<double>? spectrumBars;
  final List<double>? spectrumPeaks;

  @override
  Widget build(BuildContext context) {
    final track = playback.currentTrack;
    final index = playback.playingIndex;
    final format = playback.formatInfo;

    final time = showElapsed
        ? playback.position
        : (playback.duration > playback.position
            ? playback.duration - playback.position
            : Duration.zero);

    final title = track == null
        ? 'No track'
        : [
            if (index != null) '${index + 1}.',
            if (track.artist != null && track.artist!.trim().isNotEmpty)
              '${track.artist!.trim()} —',
            track.displayTitle,
          ].join(' ');

    final subParts = <String>[
      if (track?.album != null && track!.album!.trim().isNotEmpty)
        track.album!.trim(),
      if (index != null && trackCount > 0) 'track ${index + 1} of $trackCount',
    ];

    final channel = switch (format.channels) {
      1 => 'MONO',
      2 => 'STEREO',
      null => '—',
      final n => '$n CH',
    };

    final fmtChip = track == null
        ? '—'
        : p.extension(track.path).replaceFirst('.', '').toUpperCase();

    final bitrate = format.bitrateKbps == null
        ? '— kbps'
        : '${format.bitrateKbps} kbps';
    final rate = format.sampleRateHz == null
        ? '— kHz'
        : '${(format.sampleRateHz! / 1000).toStringAsFixed(format.sampleRateHz! % 1000 == 0 ? 0 : 1)} kHz';

    return MockupScreen(
      child: Padding(
        padding: const EdgeInsets.fromLTRB(16, 12, 16, 12),
        child: Column(
          children: [
            Expanded(
              child: Row(
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  SizedBox(
                    width: 268,
                    child: Column(
                      crossAxisAlignment: CrossAxisAlignment.start,
                      children: [
                        GestureDetector(
                          onTap: onToggleElapsed,
                          child: Row(
                            crossAxisAlignment: CrossAxisAlignment.baseline,
                            textBaseline: TextBaseline.alphabetic,
                            children: [
                              Text(
                                formatDuration(time),
                                style: TextStyle(
                                  fontFamily: LookScope.of(context).lcdFamily,
                                  fontWeight: FontWeight.w500,
                                  fontSize: 46,
                                  height: 0.9,
                                  letterSpacing: 46 * 0.02,
                                  decoration: TextDecoration.none,
                                  color: LookScope.of(context).palette.phosphorDefault,
                                  shadows: [
                                    // `.glow` — 0 0 1px / 0 0 12px phosphor
                                    Shadow(
                                      color: Color(0xD93DE7FF),
                                      blurRadius: 1,
                                    ),
                                    Shadow(
                                      color: Color(0x733DE7FF),
                                      blurRadius: 8,
                                    ),
                                  ],
                                ),
                              ),
                              const SizedBox(width: 10),
                              Text(
                                showElapsed ? 'ELAPSED' : 'REMAIN',
                                style: TextStyle(
                                  fontFamily: LookScope.of(context).chromeFamily,
                                  fontWeight: FontWeight.w700,
                                  fontSize: 12,
                                  height: 1,
                                  letterSpacing: 12 * 0.22,
                                  decoration: TextDecoration.none,
                                  color: LookScope.of(context).palette.phosphorDefault.withValues(alpha: 0.5),
                                  shadows: const [
                                    Shadow(
                                      color: Color(0x403DE7FF),
                                      blurRadius: 8,
                                    ),
                                  ],
                                ),
                              ),
                            ],
                          ),
                        ),
                        const Spacer(),
                        SizedBox(
                          height: 42,
                          child: _MockupSpectrum(
                            levels: playback.levelsStream,
                            bars: spectrumBars,
                            peaks: spectrumPeaks,
                          ),
                        ),
                      ],
                    ),
                  ),
                  const SizedBox(width: 20),
                  Expanded(
                    child: Column(
                      crossAxisAlignment: CrossAxisAlignment.start,
                      children: [
                        _MarqueeTitle(title),
                        if (subParts.isNotEmpty) ...[
                          const SizedBox(height: 5),
                          Text(
                            subParts.join(' · ').toUpperCase(),
                            maxLines: 1,
                            overflow: TextOverflow.ellipsis,
                            style: TextStyle(
                              fontFamily: LookScope.of(context).chromeFamily,
                              fontWeight: FontWeight.w700,
                              fontSize: 14,
                              height: 1.15,
                              letterSpacing: 14 * 0.14,
                              decoration: TextDecoration.none,
                              color: LookScope.of(context).palette.phosphorDefault.withValues(alpha: 0.5),
                              shadows: const [
                                Shadow(
                                  color: Color(0x403DE7FF),
                                  blurRadius: 8,
                                ),
                              ],
                            ),
                          ),
                        ],
                        const Spacer(),
                        Row(
                          children: [
                            Expanded(
                              child: FittedBox(
                                fit: BoxFit.scaleDown,
                                alignment: Alignment.centerLeft,
                                child: Row(
                                  mainAxisSize: MainAxisSize.min,
                                  children: [
                                    _MetaDim(bitrate),
                                    const SizedBox(width: 22),
                                    _MetaDim(rate),
                                    const SizedBox(width: 22),
                                    Text(
                                      channel,
                                      style: TextStyle(
                                        fontFamily: LookScope.of(context).chromeFamily,
                                        fontWeight: FontWeight.w700,
                                        fontSize: 12,
                                        height: 1,
                                        letterSpacing: 12 * 0.2,
                                        decoration: TextDecoration.none,
                                        color: LookScope.of(context).palette.phosphorDefault,
                                        shadows: [
                                          Shadow(
                                            color: Color(0xD93DE7FF),
                                            blurRadius: 1,
                                          ),
                                          Shadow(
                                            color: Color(0x733DE7FF),
                                            blurRadius: 12,
                                          ),
                                        ],
                                      ),
                                    ),
                                  ],
                                ),
                              ),
                            ),
                            const SizedBox(width: 8),
                            _RepeatStatusChip(
                              mode: playback.repeatMode,
                              onPressed: playback.cycleRepeatMode,
                            ),
                            const SizedBox(width: 10),
                            _FormatChip(fmtChip),
                          ],
                        ),
                      ],
                    ),
                  ),
                ],
              ),
            ),
          ],
        ),
      ),
    );
  }
}

class _MarqueeTitle extends StatefulWidget {
  const _MarqueeTitle(this.text);

  final String text;

  @override
  State<_MarqueeTitle> createState() => _MarqueeTitleState();
}

class _MarqueeTitleState extends State<_MarqueeTitle>
    with TickerProviderStateMixin {
  TextStyle _style(BuildContext context) => TextStyle(
        fontFamily: LookScope.of(context).chromeFamily,
        fontWeight: FontWeight.w700,
        fontSize: 24,
        height: 1.15,
        letterSpacing: 24 * 0.03,
        decoration: TextDecoration.none,
        color: LookScope.of(context).palette.phosphorHot,
        shadows: const [
          Shadow(color: Color(0xE63DE7FF), blurRadius: 1.5),
          Shadow(color: Color(0x803DE7FF), blurRadius: 10),
        ],
      );

  /// Pixels per second for the LTR marquee crawl.
  static const _speedPxPerSec = 36.0;

  /// Hold at the end of the crawl before snapping back to the start.
  static const _endPause = Duration(seconds: 1);

  /// Matches the ShaderMask stop where the trailing fade begins.
  static const _fadeStart = 0.84;

  AnimationController? _controller;
  Timer? _endPauseTimer;
  double _textWidth = 0;
  double _viewportWidth = 0;

  bool get _overflows =>
      _viewportWidth > 0 && _textWidth > _viewportWidth + 0.5;

  /// Extra travel so the last glyphs clear the fade before the end pause.
  double get _endPad => _viewportWidth * (1.0 - _fadeStart);

  double get _travel =>
      (_textWidth - _viewportWidth + _endPad).clamp(0.0, double.infinity);

  @override
  void initState() {
    super.initState();
    _measureText();
  }

  @override
  void didUpdateWidget(covariant _MarqueeTitle oldWidget) {
    super.didUpdateWidget(oldWidget);
    if (oldWidget.text != widget.text) {
      _measureText();
      _restartIfNeeded();
    }
  }

  @override
  void dispose() {
    _endPauseTimer?.cancel();
    _controller?.dispose();
    super.dispose();
  }

  void _measureText() {
    final painter = TextPainter(
      text: TextSpan(text: widget.text, style: _style(context)),
      textDirection: TextDirection.ltr,
      maxLines: 1,
    )..layout();
    _textWidth = painter.width;
  }

  void _restartIfNeeded() {
    _endPauseTimer?.cancel();
    _endPauseTimer = null;
    _controller?.stop();
    _controller?.dispose();
    _controller = null;
    if (!_overflows) {
      if (mounted) setState(() {});
      return;
    }
    final seconds = (_travel / _speedPxPerSec).clamp(1.5, 20.0);
    _controller = AnimationController(
      vsync: this,
      duration: Duration(milliseconds: (seconds * 1000).round()),
    )..addStatusListener((status) {
        if (status == AnimationStatus.completed) {
          // Hold at the end, then snap back and crawl again (L→R reveal).
          _endPauseTimer?.cancel();
          _endPauseTimer = Timer(_endPause, () {
            if (!mounted) return;
            _controller?.forward(from: 0);
          });
        }
      });
    _controller!.forward();
    if (mounted) setState(() {});
  }

  Widget _titleText() {
    return Text(
      widget.text,
      maxLines: 1,
      softWrap: false,
      overflow: TextOverflow.visible,
      style: _style(context),
    );
  }

  @override
  Widget build(BuildContext context) {
    return LayoutBuilder(
      builder: (context, constraints) {
        final maxW = constraints.maxWidth;
        if ((maxW - _viewportWidth).abs() > 0.5) {
          _viewportWidth = maxW;
          WidgetsBinding.instance.addPostFrameCallback((_) {
            if (mounted) _restartIfNeeded();
          });
        }

        if (!_overflows) {
          return SizedBox(
            width: maxW,
            child: Text(
              widget.text,
              maxLines: 1,
              softWrap: false,
              overflow: TextOverflow.ellipsis,
              style: _style(context),
            ),
          );
        }

        final travel = _travel;
        final animation = _controller;
        Widget content = _titleText();
        if (animation != null) {
          content = AnimatedBuilder(
            animation: animation,
            builder: (context, child) {
              return Transform.translate(
                offset: Offset(-travel * animation.value, 0),
                child: child,
              );
            },
            child: content,
          );
        }

        // SizedBox pins layout width; ClipRect only clips paint otherwise the
        // full title width expands the parent Row (yellow/black overflow bars).
        return SizedBox(
          width: maxW,
          child: ClipRect(
            child: ShaderMask(
              blendMode: BlendMode.dstIn,
              shaderCallback: (bounds) => LinearGradient(
                colors: [
                  Color(0xFF000000),
                  Color(0xFF000000),
                  Color(0x00000000),
                ],
                stops: [0, _fadeStart, 1],
              ).createShader(bounds),
              child: content,
            ),
          ),
        );
      },
    );
  }
}

class _MetaDim extends StatelessWidget {
  const _MetaDim(this.text);

  final String text;

  @override
  Widget build(BuildContext context) {
    return Text(
      text,
      style: TextStyle(
        fontFamily: LookScope.of(context).lcdFamily,
        fontWeight: FontWeight.w500,
        fontSize: 13,
        height: 1,
        letterSpacing: 13 * 0.04,
        decoration: TextDecoration.none,
        color: LookScope.of(context).palette.phosphorDefault.withValues(alpha: 0.5),
        shadows: const [
          Shadow(color: Color(0x403DE7FF), blurRadius: 8),
        ],
      ),
    );
  }
}

class _RepeatStatusChip extends StatelessWidget {
  const _RepeatStatusChip({
    required this.mode,
    required this.onPressed,
  });

  final RepeatMode mode;
  final VoidCallback onPressed;

  String get _label => switch (mode) {
        RepeatMode.off => 'OFF',
        RepeatMode.all => 'PLAYLIST',
        RepeatMode.one => 'TRACK',
      };

  @override
  Widget build(BuildContext context) {
    final lit = mode != RepeatMode.off;
    final colour = lit
        ? LookScope.of(context).palette.phosphorDefault
        : LookScope.of(context).palette.phosphorDefault.withValues(alpha: 0.5);
    return GestureDetector(
      key: const Key('player-display-repeat'),
      behavior: HitTestBehavior.opaque,
      onTap: onPressed,
      child: Semantics(
        button: true,
        label: 'Repeat $_label',
        child: Row(
          mainAxisSize: MainAxisSize.min,
          children: [
            TransportIcons.reload(colour: colour),
            const SizedBox(width: 4),
            Text(
              _label,
              style: TextStyle(
                fontFamily: LookScope.of(context).chromeFamily,
                fontWeight: FontWeight.w700,
                fontSize: 12,
                height: 1,
                letterSpacing: 12 * 0.14,
                decoration: TextDecoration.none,
                color: colour,
                shadows: [
                  Shadow(
                    color: Color.fromRGBO(61, 231, 255, lit ? 0.85 : 0.25),
                    blurRadius: lit ? 8 : 6,
                  ),
                ],
              ),
            ),
          ],
        ),
      ),
    );
  }
}

class _FormatChip extends StatelessWidget {
  const _FormatChip(this.label);

  final String label;

  @override
  Widget build(BuildContext context) {
    return Container(
      padding: const EdgeInsets.symmetric(horizontal: 9, vertical: 3),
      decoration: BoxDecoration(
        borderRadius: BorderRadius.circular(2),
        gradient: LinearGradient(
          begin: Alignment.topCenter,
          end: Alignment.bottomCenter,
          colors: [
            Color(0xFFFFB3D4),
            LookScope.of(context).palette.accentDefault,
            Color(0xFFB8226A),
          ],
          stops: [0, 0.55, 1],
        ),
        boxShadow: const [
          BoxShadow(color: Color(0x66FF3D9A), blurRadius: 12),
          BoxShadow(
            color: Color(0x80FFFFFF),
            offset: Offset(0, 1),
            blurStyle: BlurStyle.inner,
          ),
        ],
      ),
      child: Text(
        label,
        style: TextStyle(
          fontFamily: LookScope.of(context).chromeFamily,
          fontWeight: FontWeight.w700,
          fontSize: 12,
          height: 1,
          letterSpacing: 12 * 0.18,
          decoration: TextDecoration.none,
          color: Color(0xFF2B0616),
        ),
      ),
    );
  }
}

/// 20-bar mockup spectrum (cyan→accent + peak caps).
class _MockupSpectrum extends StatefulWidget {
  const _MockupSpectrum({
    required this.levels,
    this.bars,
    this.peaks,
  });

  final Stream<AudioLevels> levels;
  final List<double>? bars;
  final List<double>? peaks;

  @override
  State<_MockupSpectrum> createState() => _MockupSpectrumState();
}

class _MockupSpectrumState extends State<_MockupSpectrum> {
  static const _decay = 0.86;
  static const _peakDecay = 0.97;

  late List<double> _bars;
  late List<double> _peaks;
  StreamSubscription<AudioLevels>? _subscription;

  @override
  void initState() {
    super.initState();
    _bars = List<double>.from(
      widget.bars ?? List<double>.filled(AudioLevels.bandCount, 0),
    );
    _peaks = List<double>.from(
      widget.peaks ??
          widget.bars ??
          List<double>.filled(AudioLevels.bandCount, 0),
    );
    if (widget.bars == null) _listen();
  }

  @override
  void didUpdateWidget(covariant _MockupSpectrum oldWidget) {
    super.didUpdateWidget(oldWidget);
    if (widget.bars != null) {
      _subscription?.cancel();
      _subscription = null;
      _bars = List<double>.from(widget.bars!);
      _peaks = List<double>.from(widget.peaks ?? widget.bars!);
      return;
    }
    if (oldWidget.levels != widget.levels || oldWidget.bars != null) {
      _subscription?.cancel();
      _listen();
    }
  }

  void _listen() {
    _subscription = widget.levels.listen((frame) {
      if (!mounted) return;
      setState(() {
        for (var i = 0; i < AudioLevels.bandCount; i++) {
          final incoming = frame.bands[i].clamp(0.0, 1.0);
          _bars[i] = incoming > _bars[i] ? incoming : _bars[i] * _decay;
          _peaks[i] = math.max(_bars[i], _peaks[i] * _peakDecay);
        }
      });
    });
  }

  @override
  void dispose() {
    _subscription?.cancel();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    return CustomPaint(
      painter: _MockupSpectrumPainter(
        materials: LookScope.of(context).materials,
        bars: _bars,
        peaks: _peaks,
      ),
      child: const SizedBox.expand(),
    );
  }
}

class _MockupSpectrumPainter extends CustomPainter {
  const _MockupSpectrumPainter({
    required this.materials,
    required this.bars,
    required this.peaks,
  });

  final LookMaterials materials;
  final List<double> bars;
  final List<double> peaks;

  static const _barWidth = 9.0;
  static const _gap = 3.0;

  @override
  void paint(Canvas canvas, Size size) {
    if (bars.isEmpty) return;
    for (var i = 0; i < bars.length; i++) {
      final left = i * (_barWidth + _gap);
      if (left + _barWidth > size.width) break;
      final h = (size.height * bars[i].clamp(0.0, 1.0)).clamp(0.0, size.height);
      final peakH =
          (size.height * peaks[i].clamp(0.0, 1.0)).clamp(0.0, size.height);
      final barRect = Rect.fromLTWH(
        left,
        size.height - h,
        _barWidth,
        h,
      );
      if (h > 0) {
        canvas.drawRect(
          barRect,
          Paint()
            ..shader = LinearGradient(
              begin: Alignment.topCenter,
              end: Alignment.bottomCenter,
              colors: materials.spectrumStops,
              stops: materials.spectrumStops.length == 4
                  ? const [0, 0.26, 0.62, 1]
                  : null,
            ).createShader(barRect),
        );
        canvas.drawRect(
          barRect,
          Paint()
            ..color = const Color(0x4D3DE7FF)
            ..maskFilter = const MaskFilter.blur(BlurStyle.normal, 2.5),
        );
      }
      if (peakH > 2) {
        final capTop = size.height - peakH;
        final cap = Rect.fromLTWH(left, capTop, _barWidth, 2);
        canvas.drawRect(cap, Paint()..color = const Color(0xFFEAFFFF));
        canvas.drawRect(
          cap,
          Paint()
            ..color = const Color(0xE63DE7FF)
            ..maskFilter = const MaskFilter.blur(BlurStyle.normal, 4),
        );
      }
    }
  }

  @override
  bool shouldRepaint(covariant _MockupSpectrumPainter oldDelegate) =>
      !identical(bars, oldDelegate.bars) ||
      !identical(peaks, oldDelegate.peaks) ||
      materials != oldDelegate.materials;
}

// ---------------------------------------------------------------------------
// Volume / seek / transport
// ---------------------------------------------------------------------------

class _VolumeRow extends StatelessWidget {
  const _VolumeRow({
    required this.playback,
    required this.forceMono,
    required this.equalizerVisible,
    required this.playlistVisible,
    required this.onMono,
    required this.onEq,
    required this.onPl,
  });

  final PlaybackController playback;
  final bool forceMono;
  final bool equalizerVisible;
  final bool playlistVisible;
  final VoidCallback onMono;
  final VoidCallback onEq;
  final VoidCallback onPl;

  @override
  Widget build(BuildContext context) {
    final volume = playback.muted ? 0.0 : playback.volume.clamp(0.0, 1.0);
    return Row(
      children: [
        MockupButton(
          key: const Key('transport-mute'),
          width: 40,
          height: 40,
          semanticLabel: playback.muted ? 'Unmute' : 'Mute',
          on: playback.muted,
          onPressed: playback.toggleMute,
          child: MockupIcons.mute(),
        ),
        const SizedBox(width: 14),
        Expanded(
          child: Row(
            children: [
              Text(
                'VOL',
                style: TextStyle(
                  fontFamily: LookScope.of(context).chromeFamily,
                  fontWeight: FontWeight.w700,
                  fontSize: 11,
                  height: 1,
                  letterSpacing: 11 * 0.2,
                  decoration: TextDecoration.none,
                  color: LookScope.of(context).palette.inkFaint,
                  shadows: const [
                    Shadow(offset: Offset(0, 1), color: Color(0xB3000000)),
                  ],
                ),
              ),
              const SizedBox(width: 10),
              Expanded(
                child: MockupSlider(
                  key: const Key('transport-volume'),
                  value: volume,
                  onChanged: playback.setVolume,
                ),
              ),
            ],
          ),
        ),
        const SizedBox(width: 14),
        MockupButton(
          key: const Key('player-mono'),
          label: 'Mono',
          on: forceMono,
          width: 86,
          height: 38,
          onPressed: onMono,
        ),
        const SizedBox(width: 14),
        MockupButton(
          key: const Key('player-eq'),
          label: 'EQ',
          on: equalizerVisible,
          width: 74,
          height: 38,
          onPressed: onEq,
        ),
        const SizedBox(width: 8),
        MockupButton(
          key: const Key('player-pl'),
          label: 'PL',
          on: playlistVisible,
          width: 74,
          height: 38,
          onPressed: onPl,
        ),
      ],
    );
  }
}

class _SeekRow extends StatelessWidget {
  const _SeekRow({required this.playback});

  final PlaybackController playback;

  @override
  Widget build(BuildContext context) {
    final durationMs = playback.duration.inMilliseconds;
    final seek = durationMs > 0
        ? (playback.position.inMilliseconds / durationMs).clamp(0.0, 1.0)
        : 0.0;

    return Row(
      children: [
        _Stamp(formatDuration(playback.position)),
        const SizedBox(width: 14),
        Expanded(
          child: MockupSlider(
            key: const Key('transport-seek'),
            value: seek,
            seekStyle: true,
            trackHeight: 16,
            onChanged: durationMs > 0
                ? (v) => unawaited(
                      playback.seek(
                        Duration(milliseconds: (v * durationMs).round()),
                      ),
                    )
                : null,
          ),
        ),
        const SizedBox(width: 14),
        _Stamp(formatDuration(playback.duration)),
      ],
    );
  }
}

class _Stamp extends StatelessWidget {
  const _Stamp(this.text);

  final String text;

  @override
  Widget build(BuildContext context) {
    return Text(
      text,
      style: TextStyle(
        fontFamily: LookScope.of(context).lcdFamily,
        fontWeight: FontWeight.w500,
        fontSize: 14,
        height: 1,
        decoration: TextDecoration.none,
        color: LookScope.of(context).palette.inkDim,
      ),
    );
  }
}

class _TransportRow extends StatelessWidget {
  const _TransportRow({
    required this.playback,
    required this.canTransport,
    required this.onPlay,
    required this.onPause,
    this.onOpenFiles,
  });

  final PlaybackController playback;
  final bool canTransport;
  final VoidCallback onPlay;
  final VoidCallback onPause;
  final VoidCallback? onOpenFiles;

  @override
  Widget build(BuildContext context) {
    final trackOpen = playback.currentTrack != null;
    return Row(
      children: [
        _TransportBtn(
          buttonKey: const Key('transport-prev'),
          semanticLabel: 'Previous',
          onPressed:
              canTransport ? () => unawaited(playback.previous()) : null,
          child: MockupIcons.previous(),
        ),
        const SizedBox(width: 6),
        _TransportBtn(
          buttonKey: const Key('transport-play'),
          semanticLabel: 'Play',
          width: 78,
          on: playback.playing,
          onPressed: canTransport ? onPlay : null,
          child: MockupIcons.play(),
        ),
        const SizedBox(width: 6),
        _TransportBtn(
          buttonKey: const Key('transport-pause'),
          semanticLabel: 'Pause',
          on: playback.paused,
          onPressed: canTransport ? onPause : null,
          child: MockupIcons.pause(),
        ),
        const SizedBox(width: 6),
        _TransportBtn(
          buttonKey: const Key('transport-stop'),
          semanticLabel: 'Stop',
          onPressed:
              trackOpen ? () => unawaited(playback.stop()) : null,
          child: MockupIcons.stop(),
        ),
        const SizedBox(width: 6),
        _TransportBtn(
          buttonKey: const Key('transport-next'),
          semanticLabel: 'Next',
          onPressed: canTransport ? () => unawaited(playback.next()) : null,
          child: MockupIcons.next(),
        ),
        const SizedBox(width: 16),
        _TransportBtn(
          buttonKey: const Key('player-open'),
          semanticLabel: 'Open files',
          onPressed: onOpenFiles,
          child: MockupIcons.eject(),
        ),
        const SizedBox(width: 12),
        const Expanded(child: MockupRail(minWidth: 0)),
        const SizedBox(width: 12),
        _ToggleBtn(
          buttonKey: const Key('player-shuffle'),
          label: 'Shuffle',
          lit: playback.shuffle,
          onPressed: playback.toggleShuffle,
        ),
        const SizedBox(width: 6),
        _ToggleBtn(
          buttonKey: const Key('player-repeat'),
          label: 'Repeat',
          lit: playback.repeatMode != RepeatMode.off,
          onPressed: playback.cycleRepeatMode,
        ),
      ],
    );
  }
}

class _TransportBtn extends StatelessWidget {
  const _TransportBtn({
    required this.buttonKey,
    required this.child,
    required this.semanticLabel,
    this.onPressed,
    this.on = false,
    this.width = 66,
  });

  final Key buttonKey;
  final Widget child;
  final String semanticLabel;
  final VoidCallback? onPressed;
  final bool on;
  final double width;

  @override
  Widget build(BuildContext context) {
    return MockupButton(
      key: buttonKey,
      width: width,
      height: 50,
      semanticLabel: semanticLabel,
      on: on,
      onPressed: onPressed,
      child: child,
    );
  }
}

class _ToggleBtn extends StatelessWidget {
  const _ToggleBtn({
    required this.buttonKey,
    required this.label,
    required this.lit,
    required this.onPressed,
  });

  final Key buttonKey;
  final String label;
  final bool lit;
  final VoidCallback onPressed;

  @override
  Widget build(BuildContext context) {
    return MockupButton(
      key: buttonKey,
      height: 50,
      padding: const EdgeInsets.symmetric(horizontal: 15),
      semanticLabel: label,
      onPressed: onPressed,
      child: Row(
        mainAxisSize: MainAxisSize.min,
        children: [
          MockupLed(lit: lit),
          const SizedBox(width: 9),
          Text(
            label.toUpperCase(),
            style: TextStyle(
              fontFamily: LookScope.of(context).chromeFamily,
              fontWeight: FontWeight.w700,
              fontSize: 13,
              height: 1,
              letterSpacing: 13 * 0.16,
              decoration: TextDecoration.none,
              color: Color(0xB8C4D2E8),
            ),
          ),
        ],
      ),
    );
  }
}
