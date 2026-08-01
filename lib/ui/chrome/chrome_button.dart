import 'package:flutter/material.dart';

import '../../theme/tramp_colors.dart';

class ChromeButton extends StatefulWidget {
  const ChromeButton({
    super.key,
    required this.child,
    this.onPressed,
    this.primary = false,
  });

  final Widget child;
  final VoidCallback? onPressed;
  final bool primary;

  @override
  State<ChromeButton> createState() => _ChromeButtonState();
}

class _ChromeButtonState extends State<ChromeButton> {
  bool _pressed = false;

  bool get _enabled => widget.onPressed != null;

  @override
  Widget build(BuildContext context) {
    final raised = _enabled && !_pressed;
    final face = widget.primary ? TrampColors.metalFace : TrampColors.metalMid;

    return Semantics(
      button: true,
      enabled: _enabled,
      child: GestureDetector(
        onTap: widget.onPressed,
        onTapDown: _enabled ? (_) => setState(() => _pressed = true) : null,
        onTapUp: _enabled ? (_) => setState(() => _pressed = false) : null,
        onTapCancel: _enabled ? () => setState(() => _pressed = false) : null,
        child: ConstrainedBox(
          constraints: const BoxConstraints(minWidth: 28, minHeight: 28),
          child: DecoratedBox(
            decoration: BoxDecoration(
              gradient: LinearGradient(
                begin: Alignment.topCenter,
                end: Alignment.bottomCenter,
                colors: raised
                    ? [
                        TrampColors.metalHi,
                        face,
                        TrampColors.metalShadow,
                      ]
                    : [
                        TrampColors.metalShadow,
                        face,
                        TrampColors.metalHi,
                      ],
              ),
              border: Border(
                top: BorderSide(
                  color: raised ? TrampColors.metalHi : TrampColors.metalDeep,
                  width: TrampColors.borderWidth,
                ),
                left: BorderSide(
                  color: raised ? TrampColors.metalHi : TrampColors.metalDeep,
                  width: TrampColors.borderWidth,
                ),
                right: BorderSide(
                  color: raised ? TrampColors.metalDeep : TrampColors.metalHi,
                  width: TrampColors.borderWidth,
                ),
                bottom: BorderSide(
                  color: raised ? TrampColors.metalDeep : TrampColors.metalHi,
                  width: TrampColors.borderWidth,
                ),
              ),
            ),
            child: Center(
              child: Opacity(
                opacity: _enabled ? 1.0 : 0.4,
                child: IconTheme(
                  data: IconThemeData(
                    color: _enabled
                        ? TrampColors.metalDeep
                        : TrampColors.metalShadow,
                    size: 16,
                  ),
                  child: DefaultTextStyle(
                    style: TextStyle(
                      color: _enabled
                          ? TrampColors.metalDeep
                          : TrampColors.metalShadow,
                      fontSize: 11,
                      fontWeight: FontWeight.w600,
                    ),
                    child: widget.child,
                  ),
                ),
              ),
            ),
          ),
        ),
      ),
    );
  }
}
