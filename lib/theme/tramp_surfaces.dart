import 'package:flutter/painting.dart';

import 'tramp_colors.dart';

/// The complete set of chrome materials.
///
/// Every panel, button, groove and display in the app draws its decoration from
/// here. Widgets must not compose their own gradients or bevels — drift between
/// the equalizer, the transport buttons and the playlist is exactly what this
/// single definition prevents.
abstract final class TrampSurfaces {
  static const _radius = BorderRadius.all(Radius.circular(3));
  static const _buttonRadius = BorderRadius.all(Radius.circular(2));

  static BoxDecoration raisedPanel({double bevel = 1}) {
    return BoxDecoration(
      borderRadius: _radius,
      gradient: const LinearGradient(
        begin: Alignment.topCenter,
        end: Alignment.bottomCenter,
        colors: [TrampColors.panelTop, TrampColors.panelBottom],
      ),
      border: Border(
        top: BorderSide(color: TrampColors.bevelHi, width: bevel),
        left: BorderSide(color: TrampColors.bevelHi, width: bevel),
        right: BorderSide(color: TrampColors.bevelLo, width: bevel),
        bottom: BorderSide(color: TrampColors.bevelLo, width: bevel),
      ),
    );
  }

  static BoxDecoration raisedButton({double bevel = 1}) {
    return BoxDecoration(
      borderRadius: _buttonRadius,
      gradient: const LinearGradient(
        begin: Alignment.topCenter,
        end: Alignment.bottomCenter,
        colors: [TrampColors.buttonTop, TrampColors.buttonBottom],
      ),
      border: Border(
        top: BorderSide(color: TrampColors.bevelHi, width: bevel),
        left: BorderSide(color: TrampColors.bevelHi, width: bevel),
        right: BorderSide(color: TrampColors.bevelLo, width: bevel),
        bottom: BorderSide(color: TrampColors.bevelLo, width: bevel),
      ),
    );
  }

  static BoxDecoration pressedButton({double bevel = 1}) {
    return BoxDecoration(
      borderRadius: _buttonRadius,
      gradient: const LinearGradient(
        begin: Alignment.topCenter,
        end: Alignment.bottomCenter,
        colors: [TrampColors.buttonBottom, TrampColors.buttonTop],
      ),
      border: Border(
        top: BorderSide(color: TrampColors.bevelLo, width: bevel),
        left: BorderSide(color: TrampColors.bevelLo, width: bevel),
        right: BorderSide(color: TrampColors.bevelHi, width: bevel),
        bottom: BorderSide(color: TrampColors.bevelHi, width: bevel),
      ),
    );
  }

  static BoxDecoration insetWell({double bevel = 1}) {
    return BoxDecoration(
      color: TrampColors.wellDeep,
      border: Border(
        top: BorderSide(color: TrampColors.bevelLo, width: bevel),
        left: BorderSide(color: TrampColors.bevelLo, width: bevel),
        right: BorderSide(color: TrampColors.bevelHi, width: bevel),
        bottom: BorderSide(color: TrampColors.bevelHi, width: bevel),
      ),
    );
  }

  static BoxDecoration lcdGlass({double bevel = 1}) {
    return BoxDecoration(
      color: TrampColors.lcdGlass,
      border: Border(
        top: BorderSide(color: TrampColors.bevelLo, width: bevel),
        left: BorderSide(color: TrampColors.bevelLo, width: bevel),
        right: BorderSide(color: TrampColors.bevelHi, width: bevel),
        bottom: BorderSide(color: TrampColors.bevelHi, width: bevel),
      ),
    );
  }
}
