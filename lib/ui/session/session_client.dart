import 'dart:async';

import 'package:desktop_multi_window/desktop_multi_window.dart';
import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:window_manager/window_manager.dart';

import '../../domain/tramp_settings.dart';
import '../../theme/mockup_tokens.dart';
import 'session_bus.dart';
import 'session_messages.dart';

/// Secondary-engine shell (EQ / playlist) until Tasks 6–8 mount real chrome.
class SessionClientApp extends StatefulWidget {
  const SessionClientApp({
    super.key,
    required this.role,
    required this.windowController,
  });

  final WindowRole role;
  final WindowController windowController;

  @override
  State<SessionClientApp> createState() => _SessionClientAppState();
}

class _SessionClientAppState extends State<SessionClientApp>
    with WindowListener {
  final _bus = SessionBus();
  String? _lastEventType;

  @override
  void initState() {
    super.initState();
    windowManager.addListener(this);
    unawaited(_bootstrap());
  }

  Future<void> _bootstrap() async {
    await windowManager.setPreventClose(true);
    await widget.windowController.setWindowMethodHandler(_onWindowMethod);
    await _configureChrome();
    await _bus.sendCommand(ClientReadyCommand(widget.role));
  }

  Future<void> _configureChrome() async {
    final title = switch (widget.role) {
      WindowRole.equalizer => 'Tramp — Equalizer',
      WindowRole.playlist => 'Tramp — Playlist',
      WindowRole.main => 'Tramp',
    };
    await windowManager.setTitle(title);
    await windowManager.setAsFrameless();
    await windowManager.setResizable(widget.role == WindowRole.playlist);
  }

  Future<dynamic> _onWindowMethod(MethodCall call) async {
    switch (call.method) {
      case SessionBus.applyFrameMethod:
        final args = Map<String, dynamic>.from(call.arguments as Map);
        await _applyFrame(args);
        return null;
      case SessionBus.eventMethod:
        final envelope = SessionEvent.decodeEnvelope(call.arguments);
        final event = SessionEvent.fromJson(envelope);
        if (mounted) {
          setState(() => _lastEventType = event.type);
        }
        return null;
      case 'window_close':
        await _hideInsteadOfClose();
        return null;
      case 'session_shutdown':
        await windowManager.setPreventClose(false);
        await windowManager.destroy();
        return null;
      default:
        throw MissingPluginException('Not implemented: ${call.method}');
    }
  }

  Future<void> _applyFrame(Map<String, dynamic> args) async {
    final left = (args['left'] as num).toDouble();
    final top = (args['top'] as num).toDouble();
    final width = (args['width'] as num).toDouble();
    final height = (args['height'] as num).toDouble();
    final visible = args['visible'] == true;

    final alwaysOnTop = args['alwaysOnTop'] == true;

    await windowManager.setMinimumSize(Size(width, height));
    await windowManager.setSize(Size(width, height));
    await windowManager.setPosition(Offset(left, top));
    await windowManager.setAlwaysOnTop(alwaysOnTop);
    if (visible) {
      await windowManager.show();
      await widget.windowController.show();
    } else {
      await windowManager.hide();
      await widget.windowController.hide();
    }
  }

  @override
  void onWindowClose() {
    unawaited(_hideInsteadOfClose());
  }

  Future<void> _hideInsteadOfClose() async {
    final windowId = switch (widget.role) {
      WindowRole.equalizer => WindowId.equalizer,
      WindowRole.playlist => WindowId.playlist,
      WindowRole.main => WindowId.main,
    };
    try {
      await _bus.sendCommand(
        ToggleWindowCommand(window: windowId, visible: false),
      );
    } catch (_) {
      // Host may already be tearing down.
    }
    await windowManager.hide();
    await widget.windowController.hide();
  }

  @override
  void dispose() {
    windowManager.removeListener(this);
    unawaited(widget.windowController.setWindowMethodHandler(null));
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    final label = switch (widget.role) {
      WindowRole.equalizer => 'Equalizer',
      WindowRole.playlist => 'Playlist',
      WindowRole.main => 'Main',
    };
    return MaterialApp(
      debugShowCheckedModeBanner: false,
      home: ColoredBox(
        color: MockupTokens.shellMid,
        child: Center(
          child: Column(
            mainAxisSize: MainAxisSize.min,
            children: [
              Text(
                'Tramp — $label',
                style: const TextStyle(
                  color: MockupTokens.phos,
                  fontSize: 22,
                  fontWeight: FontWeight.w700,
                ),
              ),
              const SizedBox(height: 8),
              Text(
                _lastEventType == null
                    ? 'placeholder until chrome task'
                    : 'last event: $_lastEventType',
                style: const TextStyle(
                  color: MockupTokens.inkDim,
                  fontSize: 12,
                ),
              ),
            ],
          ),
        ),
      ),
    );
  }
}
