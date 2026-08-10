import 'dart:io';

import 'package:flutter/material.dart';

import '../../look/look_controller.dart';
import '../../look/look_installer.dart';
import '../../theme/look_scope.dart';
import '../../theme/tramp_text.dart';
import '../session/session_messages.dart';
import '../settings/skins_panel.dart';

/// Dark skins manager dialog (tests / legacy). Prefer the Settings window.
Future<void> showLookPackDialog(
  BuildContext context, {
  required LookController controller,
  Future<String?> Function()? pickZipPath,
  Future<String?> Function()? pickDirectoryPath,
  Future<String?> Function()? pickLooksDirectoryPath,
}) {
  return showDialog<void>(
    context: context,
    builder: (context) => LookPackDialog(
      controller: controller,
      pickZipPath: pickZipPath,
      pickDirectoryPath: pickDirectoryPath,
      pickLooksDirectoryPath: pickLooksDirectoryPath,
    ),
  );
}

/// Replace / Cancel when an install would overwrite an existing pack id.
Future<LookConflictChoice> showLookConflictDialog(
  BuildContext context,
  LookConflict conflict,
) async {
  final look = LookScope.of(context);
  final palette = look.palette;
  final result = await showDialog<LookConflictChoice>(
    context: context,
    builder: (context) {
      return AlertDialog(
        backgroundColor: palette.shellMid,
        surfaceTintColor: Colors.transparent,
        title: Text(
          'Replace skin?',
          style: TrampText.chromeLabel(look).copyWith(
            fontSize: 14,
            color: palette.inkDefault,
          ),
        ),
        content: Text(
          'Installed: ${conflict.installedName}'
          '${conflict.installedAuthor != null ? ' — ${conflict.installedAuthor}' : ''}\n'
          'Incoming: ${conflict.incomingName}'
          '${conflict.incomingAuthor != null ? ' — ${conflict.incomingAuthor}' : ''}',
          style: TrampText.lcd(look).copyWith(color: palette.inkDim, height: 1.35),
        ),
        actions: [
          TextButton(
            key: const Key('look-conflict-cancel'),
            onPressed: () =>
                Navigator.of(context).pop(LookConflictChoice.cancel),
            child: Text(
              'Cancel',
              style: TrampText.chromeLabel(look).copyWith(color: palette.inkDim),
            ),
          ),
          TextButton(
            key: const Key('look-conflict-replace'),
            onPressed: () =>
                Navigator.of(context).pop(LookConflictChoice.replace),
            child: Text(
              'Replace',
              style: TrampText.chromeLabel(look)
                  .copyWith(color: palette.phosphorDefault),
            ),
          ),
        ],
      );
    },
  );
  return result ?? LookConflictChoice.cancel;
}

class LookPackDialog extends StatelessWidget {
  const LookPackDialog({
    super.key,
    required this.controller,
    this.pickZipPath,
    this.pickDirectoryPath,
    this.pickLooksDirectoryPath,
  });

  final LookController controller;
  final Future<String?> Function()? pickZipPath;
  final Future<String?> Function()? pickDirectoryPath;
  final Future<String?> Function()? pickLooksDirectoryPath;

  @override
  Widget build(BuildContext context) {
    final look = LookScope.of(context);
    final palette = look.palette;

    return AlertDialog(
      backgroundColor: palette.shellMid,
      surfaceTintColor: Colors.transparent,
      titlePadding: const EdgeInsets.fromLTRB(20, 16, 20, 0),
      contentPadding: const EdgeInsets.fromLTRB(12, 12, 12, 8),
      actionsPadding: const EdgeInsets.fromLTRB(12, 0, 12, 12),
      title: Text(
        'Skins',
        style: TrampText.chromeLabel(look).copyWith(
          fontSize: 14,
          letterSpacing: 1.2,
          color: palette.phosphorDefault,
        ),
      ),
      content: SizedBox(
        width: 360,
        height: 280,
        child: ListenableBuilder(
          listenable: controller,
          builder: (context, _) {
            final skins = [
              for (final m in controller.installed)
                SkinCatalogEntry(
                  id: m.id,
                  name: m.name,
                  author: m.author,
                ),
            ];
            return SkinsPanel(
              skins: skins,
              activeSkinId: controller.activeSkinId,
              lastError: controller.lastError,
              pickZipPath: pickZipPath,
              pickDirectoryPath: pickDirectoryPath,
              pickSkinsDirectoryPath: pickLooksDirectoryPath,
              onActivate: controller.activate,
              onInstallZipPath: (path) async {
                try {
                  await controller.installZip(
                    File(path),
                    onConflict: (conflict) =>
                        showLookConflictDialog(context, conflict),
                  );
                } catch (_) {}
              },
              onInstallDirectoryPath: (path) async {
                try {
                  await controller.installDirectory(
                    Directory(path),
                    onConflict: (conflict) =>
                        showLookConflictDialog(context, conflict),
                  );
                } catch (_) {}
              },
              onSetSkinsDirectory: (path) async {
                try {
                  await controller.setSkinsDirectory(path);
                } catch (_) {}
              },
              onResetSkinsDirectory: () =>
                  controller.setSkinsDirectory(null),
            );
          },
        ),
      ),
      actions: [
        TextButton(
          key: const Key('look-pack-close'),
          onPressed: () => Navigator.of(context).pop(),
          child: Text(
            'Close',
            style: TrampText.chromeLabel(look).copyWith(color: palette.inkDim),
          ),
        ),
      ],
    );
  }
}
