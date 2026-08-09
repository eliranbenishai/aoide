import 'package:flutter/widgets.dart';

import '../look/resolved_look.dart';

/// Provides the active [ResolvedLook] to mockup chrome under a window root.
class LookScope extends InheritedWidget {
  const LookScope({
    super.key,
    required this.look,
    required super.child,
  });

  final ResolvedLook look;

  static ResolvedLook of(BuildContext context) {
    final scope = context.dependOnInheritedWidgetOfExactType<LookScope>();
    assert(scope != null, 'LookScope.of() called with no LookScope ancestor');
    return scope!.look;
  }

  static ResolvedLook? maybeOf(BuildContext context) {
    return context.dependOnInheritedWidgetOfExactType<LookScope>()?.look;
  }

  @override
  bool updateShouldNotify(LookScope oldWidget) => look != oldWidget.look;
}
