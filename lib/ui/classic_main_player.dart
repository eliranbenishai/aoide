import 'package:flutter/material.dart';
import 'package:window_manager/window_manager.dart';

import '../playback/playback_controller.dart';
import '../theme/tramp_colors.dart';
import 'chrome/chrome_button.dart';
import 'chrome/chrome_slider.dart';
import 'chrome/metal_panel.dart';
import 'chrome/transport_icons.dart';

/// Fixed-aspect classic main player chrome, scaled by the shell via [FittedBox].
class ClassicMainPlayer extends StatelessWidget {
  const ClassicMainPlayer({
    super.key,
    required this.playback,
    required this.hasTracks,
    this.onFocusPlaylist,
  });

  static const logicalSize = Size(550, 232);

  final PlaybackController playback;
  final bool hasTracks;
  final VoidCallback? onFocusPlaylist;

  @override
  Widget build(BuildContext context) {
    return SizedBox(
      width: logicalSize.width,
      height: logicalSize.height,
      child: MetalPanel(
        style: MetalPanelStyle.raised,
        child: Padding(
          padding: const EdgeInsets.all(6),
          child: ListenableBuilder(
            listenable: playback,
            builder: (context, _) {
              return Column(
                children: [
                  const _TitleBarStub(),
                  const SizedBox(height: 6),
                  const Expanded(child: _LcdRowStub()),
                  const SizedBox(height: 6),
                  SizedBox(
                    height: 22,
                    child: ChromeSlider(value: 0, onChanged: (_) {}),
                  ),
                  const SizedBox(height: 6),
                  _TransportRowStub(
                    hasTracks: hasTracks,
                    onFocusPlaylist: onFocusPlaylist,
                  ),
                ],
              );
            },
          ),
        ),
      ),
    );
  }
}

class _TitleBarStub extends StatelessWidget {
  const _TitleBarStub();

  @override
  Widget build(BuildContext context) {
    return SizedBox(
      height: 28,
      child: Row(
        children: [
          Expanded(
            child: DragToMoveArea(
              child: const Align(
                alignment: Alignment.centerLeft,
                child: Text(
                  'TRAMP',
                  style: TextStyle(
                    color: TrampColors.metalDeep,
                    fontSize: 16,
                    fontWeight: FontWeight.w800,
                    letterSpacing: 1.2,
                    height: 1,
                  ),
                ),
              ),
            ),
          ),
          _WindowControlButton(
            label: 'Minimize',
            color: TrampColors.minimize,
            onPressed: windowManager.minimize,
          ),
          const SizedBox(width: 6),
          _WindowControlButton(
            label: 'Close',
            color: TrampColors.windowClose,
            onPressed: windowManager.close,
          ),
        ],
      ),
    );
  }
}

class _WindowControlButton extends StatelessWidget {
  const _WindowControlButton({
    required this.label,
    required this.color,
    required this.onPressed,
  });

  final String label;
  final Color color;
  final Future<void> Function() onPressed;

  @override
  Widget build(BuildContext context) {
    return Semantics(
      button: true,
      label: label,
      child: MouseRegion(
        cursor: SystemMouseCursors.click,
        child: GestureDetector(
          onTap: () => onPressed(),
          child: SizedBox(
            width: 12,
            height: 12,
            child: ColoredBox(color: color),
          ),
        ),
      ),
    );
  }
}

class _LcdRowStub extends StatelessWidget {
  const _LcdRowStub();

  @override
  Widget build(BuildContext context) {
    return const Row(
      children: [
        Expanded(
          flex: 2,
          child: MetalPanel(
            style: MetalPanelStyle.insetLcd,
            child: SizedBox.expand(),
          ),
        ),
        SizedBox(width: 6),
        Expanded(
          flex: 3,
          child: MetalPanel(
            style: MetalPanelStyle.insetLcd,
            child: SizedBox.expand(),
          ),
        ),
      ],
    );
  }
}

class _TransportRowStub extends StatelessWidget {
  const _TransportRowStub({
    required this.hasTracks,
    this.onFocusPlaylist,
  });

  final bool hasTracks;
  final VoidCallback? onFocusPlaylist;

  @override
  Widget build(BuildContext context) {
    // Stubs only — full wiring is Task 4.
    final onPressed = hasTracks ? () {} : null;

    return SizedBox(
      height: 36,
      child: Row(
        children: [
          ChromeButton(onPressed: onPressed, child: TransportIcons.prev()),
          const SizedBox(width: 4),
          ChromeButton(
            onPressed: onPressed,
            primary: true,
            child: TransportIcons.play(),
          ),
          const SizedBox(width: 4),
          ChromeButton(onPressed: onPressed, child: TransportIcons.pause()),
          const SizedBox(width: 4),
          ChromeButton(onPressed: onPressed, child: TransportIcons.stop()),
          const SizedBox(width: 4),
          ChromeButton(onPressed: onPressed, child: TransportIcons.next()),
          const SizedBox(width: 12),
          const Expanded(
            child: SizedBox(
              height: 22,
              child: ChromeSlider(value: 0.7),
            ),
          ),
          if (onFocusPlaylist != null) ...[
            const SizedBox(width: 8),
            ChromeButton(
              onPressed: onFocusPlaylist,
              child: const Text('PL'),
            ),
          ],
        ],
      ),
    );
  }
}
