import 'package:flutter/material.dart';

import '../../theme/tramp_colors.dart';

class TrampButton extends StatefulWidget {
  const TrampButton({
    super.key,
    required this.onPressed,
    required this.child,
    this.primary = false,
  });

  final VoidCallback? onPressed;
  final Widget child;
  final bool primary;

  @override
  State<TrampButton> createState() => _TrampButtonState();
}

class _TrampButtonState extends State<TrampButton> {
  bool _hovered = false;

  @override
  Widget build(BuildContext context) {
    final enabled = widget.onPressed != null;
    final primary = widget.primary;

    Color background;
    Color foreground;
    Color border;

    if (primary) {
      if (_hovered && enabled) {
        background = TrampColors.accent;
        border = TrampColors.accent;
      } else {
        background = TrampColors.ink;
        border = TrampColors.ink;
      }
      foreground = TrampColors.surface;
    } else {
      background = Colors.transparent;
      if (_hovered && enabled) {
        foreground = TrampColors.accent;
        border = TrampColors.accent;
      } else {
        foreground = TrampColors.ink;
        border = TrampColors.ink;
      }
    }

    return Semantics(
      button: true,
      enabled: enabled,
      label: _semanticLabel(widget.child),
      child: MouseRegion(
        onEnter: enabled ? (_) => setState(() => _hovered = true) : null,
        onExit: enabled ? (_) => setState(() => _hovered = false) : null,
        child: GestureDetector(
          onTap: widget.onPressed,
          child: DecoratedBox(
            decoration: BoxDecoration(
              color: background,
              border: Border.all(
                color: border,
                width: TrampColors.borderWidth,
              ),
            ),
            child: DefaultTextStyle(
              style: TextStyle(color: foreground),
              child: Padding(
                padding: const EdgeInsets.symmetric(horizontal: 10, vertical: 6),
                child: widget.child,
              ),
            ),
          ),
        ),
      ),
    );
  }

  String? _semanticLabel(Widget child) {
    if (child is Text) return child.data;
    return null;
  }
}
