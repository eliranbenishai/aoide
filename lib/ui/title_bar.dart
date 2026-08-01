import 'package:flutter/material.dart';
import 'package:google_fonts/google_fonts.dart';
import 'package:window_manager/window_manager.dart';

import '../theme/tramp_colors.dart';

class TitleBar extends StatelessWidget {
  const TitleBar({super.key});

  static const height = 44.0;

  @override
  Widget build(BuildContext context) {
    return ColoredBox(
      color: TrampColors.ink,
      child: SizedBox(
        height: height,
        child: Padding(
          padding: const EdgeInsets.symmetric(horizontal: 12),
          child: Row(
            children: [
              Expanded(
                child: DragToMoveArea(
                  child: Align(
                    alignment: Alignment.centerLeft,
                    child: _BrandWordmark(),
                  ),
                ),
              ),
              _WindowControls(),
            ],
          ),
        ),
      ),
    );
  }
}

class _BrandWordmark extends StatelessWidget {
  @override
  Widget build(BuildContext context) {
    final style = GoogleFonts.syne(
      fontSize: 22,
      fontWeight: FontWeight.w800,
      letterSpacing: -0.44,
      color: TrampColors.surface,
      height: 1,
    );
    return Text.rich(
      TextSpan(
        children: [
          TextSpan(text: 'TRAMP', style: style),
          TextSpan(
            text: '.',
            style: style.copyWith(color: TrampColors.brandAccent),
          ),
        ],
      ),
    );
  }
}

class _WindowControls extends StatelessWidget {
  @override
  Widget build(BuildContext context) {
    return Row(
      mainAxisSize: MainAxisSize.min,
      children: [
        _WindowControlButton(
          label: 'Minimize',
          color: TrampColors.minimize,
          onPressed: windowManager.minimize,
        ),
        const SizedBox(width: 6),
        _WindowControlButton(
          label: 'Close',
          color: TrampColors.accent,
          onPressed: windowManager.close,
        ),
      ],
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
