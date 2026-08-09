import 'package:flutter/widgets.dart';
import 'package:tramp/look/builtin_look.dart';
import 'package:tramp/look/resolved_look.dart';
import 'package:tramp/theme/look_scope.dart';

/// Wraps [child] in [LookScope] with the builtin look (or an override).
Widget wrapWithLook(Widget child, {ResolvedLook? look}) {
  return LookScope(
    look: look ?? BuiltinLook.resolved,
    child: child,
  );
}

/// Common widget-test host: LTR + builtin [LookScope].
Widget lookHost(Widget child) {
  return Directionality(
    textDirection: TextDirection.ltr,
    child: wrapWithLook(Center(child: child)),
  );
}
