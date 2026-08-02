import 'package:flutter/widgets.dart';

import '../../theme/tramp_colors.dart';
import '../../theme/tramp_text.dart';

enum LcdSize { normal, large }

/// Phosphor text on the display well.
class LcdText extends StatelessWidget {
  const LcdText(
    this.text, {
    super.key,
    this.lit = true,
    this.size = LcdSize.normal,
    this.textAlign,
  });

  final String text;
  final bool lit;
  final LcdSize size;
  final TextAlign? textAlign;

  @override
  Widget build(BuildContext context) {
    final base = size == LcdSize.large ? TrampText.lcdLarge : TrampText.lcd;
    return Text(
      text,
      textAlign: textAlign,
      maxLines: 1,
      overflow: TextOverflow.clip,
      style: base.copyWith(
        color: lit ? TrampColors.phosphor : TrampColors.phosphorDim,
      ),
    );
  }
}

/// The small `EQ` / `PL` markers inside the display well.
///
/// These report which lower region is showing; they are readouts first and
/// shortcuts second, so an absent [onTap] is a valid state.
class LcdIndicator extends StatelessWidget {
  const LcdIndicator(
    this.label, {
    super.key,
    required this.lit,
    required this.onTap,
  });

  final String label;
  final bool lit;
  final VoidCallback? onTap;

  @override
  Widget build(BuildContext context) {
    final text = LcdText(label, lit: lit);

    final boxed = Container(
      padding: const EdgeInsets.symmetric(horizontal: 4, vertical: 2),
      decoration: BoxDecoration(
        border: Border.all(
          color: lit ? TrampColors.phosphor : TrampColors.phosphorDim,
        ),
      ),
      child: text,
    );

    if (onTap == null) {
      return Semantics(label: label, child: boxed);
    }

    return Semantics(
      button: true,
      label: label,
      child: MouseRegion(
        cursor: SystemMouseCursors.click,
        child: GestureDetector(
          behavior: HitTestBehavior.opaque,
          onTap: onTap,
          child: boxed,
        ),
      ),
    );
  }
}
