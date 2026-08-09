import 'dart:async';
import 'dart:convert';

import 'package:desktop_multi_window/desktop_multi_window.dart';
import 'package:flutter/services.dart';

import 'session_messages.dart';

/// Typed IPC over [WindowMethodChannel] between the session host and clients.
///
/// Host registers a unidirectional handler on [channelName]. Clients invoke
/// [sendCommand]. Host pushes events and frame applies through each client's
/// [WindowController.invokeMethod] (per-window handler).
class SessionBus {
  SessionBus({WindowMethodChannel? channel})
      : _channel = channel ??
            const WindowMethodChannel(
              channelName,
              mode: ChannelMode.unidirectional,
            );

  static const channelName = 'tramp/session';
  static const commandMethod = 'session_command';
  static const eventMethod = 'session_event';
  static const applyFrameMethod = 'apply_frame';
  static const raiseMethod = 'raise_window';

  final WindowMethodChannel _channel;

  Future<void> bindHost(
    FutureOr<void> Function(SessionCommand command) onCommand,
  ) {
    return _channel.setMethodCallHandler((call) async {
      if (call.method != commandMethod) {
        throw MissingPluginException('Not implemented: ${call.method}');
      }
      final envelope = SessionCommand.decodeEnvelope(call.arguments);
      await onCommand(SessionCommand.fromJson(envelope));
      return null;
    });
  }

  Future<void> unbind() => _channel.setMethodCallHandler(null);

  Future<void> sendCommand(SessionCommand command) {
    return _channel.invokeMethod(
      commandMethod,
      jsonEncode(command.toEnvelope()),
    );
  }

  /// Deliver [event] to a secondary window that registered a method handler.
  static Future<void> pushEvent(
    WindowController controller,
    SessionEvent event,
  ) {
    return controller.invokeMethod(
      eventMethod,
      jsonEncode(event.toEnvelope()),
    );
  }

  /// Ask a secondary window to resize/reposition via its local window_manager.
  ///
  /// When [positionOnly] is true, the client updates OS position alone — used
  /// during dock drag so size/AOT/show/focus do not fight the gesture.
  static Future<void> pushFrame(
    WindowController controller, {
    required double left,
    required double top,
    required double width,
    required double height,
    required bool visible,
    required bool alwaysOnTop,
    bool positionOnly = false,
  }) {
    return controller.invokeMethod(applyFrameMethod, {
      'left': left,
      'top': top,
      'width': width,
      'height': height,
      'visible': visible,
      'alwaysOnTop': alwaysOnTop,
      'positionOnly': positionOnly,
    });
  }

  /// Bring a secondary HWND above peers without changing its frame.
  ///
  /// Used when the main player regains focus so EQ/PL rise with the group.
  static Future<void> pushRaise(WindowController controller) {
    return controller.invokeMethod(raiseMethod, const <String, dynamic>{});
  }
}
