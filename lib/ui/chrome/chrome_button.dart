import 'package:flutter/widgets.dart';

import '../../theme/tramp_colors.dart';
import '../../theme/tramp_text.dart';
import 'metal_panel.dart';
import 'transport_icons.dart';

/// A raised graphite button.
///
/// Three shapes cover every control in the chrome: an icon button (transport,
/// shuffle, window controls), a label button (OPEN, ON, AUTO, EQ, PL), and a
/// dropdown (ZOOM, PRESETS).
class ChromeButton extends StatefulWidget {
  const ChromeButton._({
    super.key,
    required this.onPressed,
    required this.semanticLabel,
    required this.active,
    required this.size,
    this.icon,
    this.text,
    this.leading,
    this.chevron = false,
  });

  factory ChromeButton.icon({
    Key? key,
    required Widget icon,
    required VoidCallback? onPressed,
    required String semanticLabel,
    Size size = const Size(26, 26),
    bool active = false,
  }) {
    return ChromeButton._(
      key: key,
      onPressed: onPressed,
      semanticLabel: semanticLabel,
      active: active,
      size: size,
      icon: icon,
    );
  }

  factory ChromeButton.label({
    Key? key,
    required String text,
    required VoidCallback? onPressed,
    Widget? leading,
    Size? size,
    bool active = false,
    String? semanticLabel,
  }) {
    return ChromeButton._(
      key: key,
      onPressed: onPressed,
      semanticLabel: semanticLabel ?? text,
      active: active,
      size: size,
      text: text,
      leading: leading,
    );
  }

  factory ChromeButton.dropdown({
    Key? key,
    required String text,
    required VoidCallback? onPressed,
    Size? size,
    String? semanticLabel,
  }) {
    return ChromeButton._(
      key: key,
      onPressed: onPressed,
      semanticLabel: semanticLabel ?? text,
      active: false,
      size: size,
      text: text,
      chevron: true,
    );
  }

  static const chevronKey = Key('chrome-button-chevron');

  final VoidCallback? onPressed;
  final String semanticLabel;
  final bool active;
  final Size? size;
  final Widget? icon;
  final String? text;
  final Widget? leading;
  final bool chevron;

  bool get isEnabled => onPressed != null;

  @override
  State<ChromeButton> createState() => _ChromeButtonState();
}

class _ChromeButtonState extends State<ChromeButton> {
  bool _down = false;

  void _setDown(bool value) {
    if (!widget.isEnabled || _down == value) return;
    setState(() => _down = value);
  }

  Color get _contentColour {
    if (!widget.isEnabled) return TrampColors.labelDim;
    return widget.active ? TrampColors.phosphor : TrampColors.label;
  }

  @override
  Widget build(BuildContext context) {
    final colour = _contentColour;

    final Widget content;
    if (widget.icon != null) {
      content = Center(child: widget.icon);
    } else {
      final row = Row(
        mainAxisAlignment: MainAxisAlignment.center,
        mainAxisSize: MainAxisSize.min,
        children: [
          if (widget.leading != null) ...[
            widget.leading!,
            const SizedBox(width: 4),
          ],
          Text(widget.text!, style: TrampText.chromeLabel.copyWith(color: colour)),
          if (widget.chevron) ...[
            const SizedBox(width: 5),
            SizedBox(
              key: ChromeButton.chevronKey,
              width: 7,
              height: 5,
              child: CustomPaint(painter: ChevronPainter(colour: colour)),
            ),
          ],
        ],
      );
      content = widget.size != null
          ? FittedBox(fit: BoxFit.scaleDown, child: row)
          : row;
    }

    Widget button = MetalPanel(
      surface: _down ? TrampSurface.pressedButton : TrampSurface.raisedButton,
      padding: widget.icon != null
          ? null
          : const EdgeInsets.symmetric(horizontal: 7),
      child: content,
    );

    if (widget.size != null) {
      button = SizedBox(
        width: widget.size!.width,
        height: widget.size!.height,
        child: button,
      );
    }

    return Semantics(
      button: true,
      enabled: widget.isEnabled,
      label: widget.semanticLabel,
      child: MouseRegion(
        cursor: widget.isEnabled
            ? SystemMouseCursors.click
            : SystemMouseCursors.basic,
        child: GestureDetector(
          behavior: HitTestBehavior.opaque,
          onTapDown: (_) => _setDown(true),
          onTapUp: (_) => _setDown(false),
          onTapCancel: () => _setDown(false),
          onTap: widget.onPressed,
          child: button,
        ),
      ),
    );
  }
}
