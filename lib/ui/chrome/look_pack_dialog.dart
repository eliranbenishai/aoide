import 'dart:io';

import 'package:file_picker/file_picker.dart';
import 'package:flutter/material.dart';

import '../../look/look_controller.dart';
import '../../look/look_installer.dart';
import '../../look/look_manifest.dart';
import '../../theme/look_scope.dart';
import '../../theme/tramp_text.dart';

/// Dark look-pack manager: list, activate, install zip/folder, looks directory.
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
          'Replace look?',
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

  Future<String?> _defaultPickZip() async {
    final result = await FilePicker.platform.pickFiles(
      type: FileType.custom,
      allowedExtensions: const ['zip'],
      dialogTitle: 'Install look pack',
    );
    if (result == null || result.files.isEmpty) return null;
    return result.files.first.path;
  }

  Future<String?> _defaultPickDirectory() =>
      FilePicker.platform.getDirectoryPath(dialogTitle: 'Install look folder');

  Future<String?> _defaultPickLooksDirectory() =>
      FilePicker.platform.getDirectoryPath(dialogTitle: 'Looks folder');

  Future<void> _installZip(BuildContext context) async {
    final path = await (pickZipPath ?? _defaultPickZip)();
    if (path == null || !context.mounted) return;
    await controller.installZip(
      File(path),
      onConflict: (conflict) => showLookConflictDialog(context, conflict),
    );
  }

  Future<void> _installDirectory(BuildContext context) async {
    final path = await (pickDirectoryPath ?? _defaultPickDirectory)();
    if (path == null || !context.mounted) return;
    await controller.installDirectory(
      Directory(path),
      onConflict: (conflict) => showLookConflictDialog(context, conflict),
    );
  }

  Future<void> _setLooksFolder(BuildContext context) async {
    final path = await (pickLooksDirectoryPath ?? _defaultPickLooksDirectory)();
    if (path == null) return;
    await controller.setLooksDirectory(path);
  }

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
        'Look packs',
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
            final looks = controller.installed;
            final activeId = controller.activeLookId;
            return Column(
              crossAxisAlignment: CrossAxisAlignment.stretch,
              children: [
                Expanded(
                  child: ListView.builder(
                    itemCount: looks.length,
                    itemBuilder: (context, index) {
                      final manifest = looks[index];
                      return _LookRow(
                        manifest: manifest,
                        active: manifest.id == activeId,
                        onTap: () => controller.activate(manifest.id),
                      );
                    },
                  ),
                ),
                const SizedBox(height: 8),
                Wrap(
                  spacing: 8,
                  runSpacing: 4,
                  children: [
                    _DialogAction(
                      key: const Key('look-install-zip'),
                      label: 'Install look…',
                      onPressed: () => _installZip(context),
                    ),
                    _DialogAction(
                      key: const Key('look-install-folder'),
                      label: 'Install folder…',
                      onPressed: () => _installDirectory(context),
                    ),
                    _DialogAction(
                      key: const Key('look-folder'),
                      label: 'Looks folder…',
                      onPressed: () => _setLooksFolder(context),
                    ),
                    _DialogAction(
                      key: const Key('look-reset-folder'),
                      label: 'Reset folder',
                      onPressed: () => controller.setLooksDirectory(null),
                    ),
                  ],
                ),
              ],
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

class _LookRow extends StatelessWidget {
  const _LookRow({
    required this.manifest,
    required this.active,
    required this.onTap,
  });

  final LookManifest manifest;
  final bool active;
  final VoidCallback onTap;

  @override
  Widget build(BuildContext context) {
    final look = LookScope.of(context);
    final palette = look.palette;
    final author = manifest.author;
    final subtitle = author == null || author.isEmpty ? null : author;

    return Material(
      color: active
          ? palette.shellHighlight.withValues(alpha: 0.45)
          : Colors.transparent,
      child: InkWell(
        onTap: onTap,
        child: Padding(
          padding: const EdgeInsets.symmetric(horizontal: 10, vertical: 8),
          child: Column(
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              Text(
                manifest.name,
                style: TrampText.chromeLabel(look).copyWith(
                  fontSize: 12,
                  color: active ? palette.phosphorDefault : palette.inkDefault,
                ),
              ),
              if (subtitle != null) ...[
                const SizedBox(height: 2),
                Text(
                  subtitle,
                  style: TrampText.lcd(look).copyWith(
                    fontSize: 10,
                    color: palette.inkDim,
                  ),
                ),
              ],
            ],
          ),
        ),
      ),
    );
  }
}

class _DialogAction extends StatelessWidget {
  const _DialogAction({
    super.key,
    required this.label,
    required this.onPressed,
  });

  final String label;
  final VoidCallback onPressed;

  @override
  Widget build(BuildContext context) {
    final look = LookScope.of(context);
    final palette = look.palette;
    return TextButton(
      onPressed: onPressed,
      style: TextButton.styleFrom(
        padding: const EdgeInsets.symmetric(horizontal: 8, vertical: 4),
        minimumSize: Size.zero,
        tapTargetSize: MaterialTapTargetSize.shrinkWrap,
      ),
      child: Text(
        label,
        style: TrampText.chromeLabel(look).copyWith(
          fontSize: 11,
          color: palette.phosphorDefault,
        ),
      ),
    );
  }
}
