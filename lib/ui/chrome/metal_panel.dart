import 'package:flutter/material.dart';

import '../../theme/tramp_colors.dart';

enum MetalPanelStyle { raised, insetLcd }

class MetalPanel extends StatelessWidget {
  const MetalPanel({
    super.key,
    required this.child,
    this.style = MetalPanelStyle.raised,
  });

  final Widget child;
  final MetalPanelStyle style;

  @override
  Widget build(BuildContext context) {
    return DecoratedBox(
      decoration: _decoration,
      child: child,
    );
  }

  BoxDecoration get _decoration {
    switch (style) {
      case MetalPanelStyle.raised:
        return BoxDecoration(
          gradient: const LinearGradient(
            begin: Alignment.topCenter,
            end: Alignment.bottomCenter,
            colors: [
              TrampColors.metalHi,
              TrampColors.metalMid,
              TrampColors.metalShadow,
            ],
            stops: [0.0, 0.45, 1.0],
          ),
          border: Border(
            top: BorderSide(
              color: TrampColors.metalHi,
              width: TrampColors.borderWidth,
            ),
            left: BorderSide(
              color: TrampColors.metalHi,
              width: TrampColors.borderWidth,
            ),
            right: BorderSide(
              color: TrampColors.metalDeep,
              width: TrampColors.borderWidth,
            ),
            bottom: BorderSide(
              color: TrampColors.metalDeep,
              width: TrampColors.borderWidth,
            ),
          ),
        );
      case MetalPanelStyle.insetLcd:
        return BoxDecoration(
          color: TrampColors.lcdBackground,
          border: Border(
            top: BorderSide(
              color: TrampColors.metalDeep,
              width: TrampColors.borderWidth * 2,
            ),
            left: BorderSide(
              color: TrampColors.metalDeep,
              width: TrampColors.borderWidth * 2,
            ),
            right: BorderSide(
              color: TrampColors.skinBorder,
              width: TrampColors.borderWidth,
            ),
            bottom: BorderSide(
              color: TrampColors.skinBorder,
              width: TrampColors.borderWidth,
            ),
          ),
        );
    }
  }
}
