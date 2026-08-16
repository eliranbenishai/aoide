// THROW AWAY — one-engine / two-window Linux drag spike.
//
// Question: with Impeller off, does a second Flutter *view* on the same
// engine keep title-bar drag buttery, or is the sludge "any extra window"?
//
// Not product code. Run: tool/run_windowing_spike.sh

// ignore_for_file: invalid_use_of_internal_member
// ignore_for_file: implementation_imports

import 'dart:ffi' show Native, Pointer, Void, Bool, Int, Uint32;
import 'dart:io';
import 'dart:ui' show FlutterView;

import 'package:flutter/material.dart';
import 'package:flutter/src/foundation/_features.dart' show isWindowingEnabled;
import 'package:flutter/src/widgets/_window.dart';
import 'package:flutter/src/widgets/_window_linux.dart';

void main() {
  // 3.47 stable blocks --dart-define=FLUTTER_ENABLED_FEATURE_FLAGS and
  // does not offer `flutter config --enable-windowing`. The flag is a
  // mutable top-level; flip it before the binding chooses a WindowingOwner.
  isWindowingEnabled = true;
  WidgetsFlutterBinding.ensureInitialized();

  if (!Platform.isLinux) {
    stderr.writeln('windowing spike is Linux-only');
    exit(2);
  }
  if (!isWindowingEnabled) {
    stderr.writeln('windowing flag still off after force-enable');
    exit(2);
  }

  final mainController = RegularWindowController(
    size: const Size(420, 200),
    title: 'SPIKE MAIN',
    delegate: _ExitOnClose(),
  );
  final otherController = RegularWindowController(
    size: const Size(420, 200),
    title: 'SPIKE OTHER',
  );
  _frameless(mainController);
  _frameless(otherController);
  _gtkWindowSetSkipTaskbarHint(_handle(otherController), true);
  _gtkWindowMove(_handle(otherController), 520, 160);

  runWidget(
    ViewCollection(
      views: [
        RegularWindow(
          controller: mainController,
          child: _SpikePane(
            label: 'MAIN',
            color: const Color(0xFF2A2A2A),
            controller: mainController,
          ),
        ),
        RegularWindow(
          controller: otherController,
          child: _SpikePane(
            label: 'OTHER (sit still)',
            color: const Color(0xFF3A2A1A),
            controller: otherController,
          ),
        ),
      ],
    ),
  );
}

class _ExitOnClose with RegularWindowControllerDelegate {
  @override
  void onWindowDestroyed() {
    super.onWindowDestroyed();
    exit(0);
  }
}

class _SpikePane extends StatelessWidget {
  const _SpikePane({
    required this.label,
    required this.color,
    required this.controller,
  });

  final String label;
  final Color color;
  final RegularWindowController controller;

  @override
  Widget build(BuildContext context) {
    final dispatcher = WidgetsBinding.instance.platformDispatcher;
    final views = dispatcher.views.toList(growable: false);
    final viewIds = views.map((FlutterView v) => v.viewId).join(', ');

    return Directionality(
      textDirection: TextDirection.ltr,
      child: ColoredBox(
        color: color,
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.stretch,
          children: [
            Listener(
              behavior: HitTestBehavior.opaque,
              onPointerDown: (event) {
                _gtkWindowBeginMoveDrag(
                  _handle(controller),
                  1,
                  event.position.dx.round(),
                  event.position.dy.round(),
                  _gtkGetCurrentEventTime(),
                );
              },
              child: const ColoredBox(
                color: Color(0xFF555555),
                child: SizedBox(
                  height: 36,
                  child: Center(
                    child: Text(
                      'DRAG HERE',
                      style: TextStyle(
                        color: Color(0xFFEEEEEE),
                        fontSize: 13,
                        fontWeight: FontWeight.w700,
                      ),
                    ),
                  ),
                ),
              ),
            ),
            Expanded(
              child: Padding(
                padding: const EdgeInsets.all(16),
                child: Text(
                  '$label\n'
                  'engineId=${dispatcher.engineId}\n'
                  'views=${views.length}  ids=[$viewIds]\n'
                  'this view=${controller.rootView.viewId}\n\n'
                  'Drag the gray bar. The other window should sit still.\n'
                  'If this feels like solo-main (buttery), extra *engines* '
                  'were the sludge — not extra OS windows.',
                  style: const TextStyle(
                    color: Color(0xFFEEEEEE),
                    fontSize: 13,
                    height: 1.35,
                  ),
                ),
              ),
            ),
          ],
        ),
      ),
    );
  }
}

Pointer<Void> _handle(RegularWindowController controller) {
  return (controller as WindowControllerLinux).windowHandle;
}

void _frameless(RegularWindowController controller) {
  _gtkWindowSetDecorated(_handle(controller), false);
}

@Native<Void Function(Pointer<Void>, Bool)>(
  symbol: 'gtk_window_set_decorated',
)
external void _gtkWindowSetDecorated(Pointer<Void> window, bool decorated);

@Native<Void Function(Pointer<Void>, Bool)>(
  symbol: 'gtk_window_set_skip_taskbar_hint',
)
external void _gtkWindowSetSkipTaskbarHint(Pointer<Void> window, bool skip);

@Native<Void Function(Pointer<Void>, Int, Int)>(symbol: 'gtk_window_move')
external void _gtkWindowMove(Pointer<Void> window, int x, int y);

@Native<Void Function(Pointer<Void>, Int, Int, Int, Uint32)>(
  symbol: 'gtk_window_begin_move_drag',
)
external void _gtkWindowBeginMoveDrag(
  Pointer<Void> window,
  int button,
  int rootX,
  int rootY,
  int timestamp,
);

@Native<Uint32 Function()>(symbol: 'gtk_get_current_event_time')
external int _gtkGetCurrentEventTime();
