import 'package:file_picker/file_picker.dart';
import 'package:flutter/material.dart';

import '../../theme/look_scope.dart';
import '../../theme/tramp_text.dart';
import '../session/session_messages.dart';

/// Reusable skins list + install actions (Settings Skins tab / dialogs).
class SkinsPanel extends StatelessWidget {
  const SkinsPanel({
    super.key,
    required this.skins,
    required this.activeSkinId,
    this.lastError,
    this.onActivate,
    this.onInstallZipPath,
    this.onInstallDirectoryPath,
    this.onSetSkinsDirectory,
    this.onResetSkinsDirectory,
    this.pickZipPath,
    this.pickDirectoryPath,
    this.pickSkinsDirectoryPath,
  });

  final List<SkinCatalogEntry> skins;
  final String activeSkinId;
  final String? lastError;
  final ValueChanged<String>? onActivate;
  final ValueChanged<String>? onInstallZipPath;
  final ValueChanged<String>? onInstallDirectoryPath;
  final ValueChanged<String>? onSetSkinsDirectory;
  final VoidCallback? onResetSkinsDirectory;
  final Future<String?> Function()? pickZipPath;
  final Future<String?> Function()? pickDirectoryPath;
  final Future<String?> Function()? pickSkinsDirectoryPath;

  Future<String?> _defaultPickZip() async {
    final result = await FilePicker.platform.pickFiles(
      type: FileType.custom,
      allowedExtensions: const ['zip'],
      dialogTitle: 'Install skin',
    );
    if (result == null || result.files.isEmpty) return null;
    return result.files.first.path;
  }

  Future<String?> _defaultPickDirectory() =>
      FilePicker.platform.getDirectoryPath(dialogTitle: 'Install skin folder');

  Future<String?> _defaultPickSkinsDirectory() =>
      FilePicker.platform.getDirectoryPath(dialogTitle: 'Skins folder');

  Future<void> _installZip() async {
    final path = await (pickZipPath ?? _defaultPickZip)();
    if (path == null) return;
    onInstallZipPath?.call(path);
  }

  Future<void> _installDirectory() async {
    final path = await (pickDirectoryPath ?? _defaultPickDirectory)();
    if (path == null) return;
    onInstallDirectoryPath?.call(path);
  }

  Future<void> _setSkinsFolder() async {
    final path = await (pickSkinsDirectoryPath ?? _defaultPickSkinsDirectory)();
    if (path == null) return;
    onSetSkinsDirectory?.call(path);
  }

  @override
  Widget build(BuildContext context) {
    final look = LookScope.of(context);
    final palette = look.palette;

    return Column(
      crossAxisAlignment: CrossAxisAlignment.stretch,
      children: [
        Expanded(
          child: ListView.builder(
            itemCount: skins.length,
            itemBuilder: (context, index) {
              final skin = skins[index];
              return _SkinRow(
                skin: skin,
                active: skin.id == activeSkinId,
                onTap: () => onActivate?.call(skin.id),
              );
            },
          ),
        ),
        if (lastError != null) ...[
          const SizedBox(height: 6),
          Text(
            key: const Key('skins-panel-error'),
            lastError!,
            style: TrampText.lcd(look).copyWith(
              fontSize: 10,
              color: palette.accentDefault,
              height: 1.3,
            ),
          ),
        ],
        const SizedBox(height: 8),
        Wrap(
          spacing: 8,
          runSpacing: 4,
          children: [
            _PanelAction(
              key: const Key('skin-install-zip'),
              label: 'Install skin…',
              onPressed: _installZip,
            ),
            _PanelAction(
              key: const Key('skin-install-folder'),
              label: 'Install folder…',
              onPressed: _installDirectory,
            ),
            _PanelAction(
              key: const Key('skins-folder'),
              label: 'Skins folder…',
              onPressed: _setSkinsFolder,
            ),
            _PanelAction(
              key: const Key('skins-reset-folder'),
              label: 'Reset folder',
              onPressed: () => onResetSkinsDirectory?.call(),
            ),
          ],
        ),
      ],
    );
  }
}

class _SkinRow extends StatelessWidget {
  const _SkinRow({
    required this.skin,
    required this.active,
    required this.onTap,
  });

  final SkinCatalogEntry skin;
  final bool active;
  final VoidCallback onTap;

  @override
  Widget build(BuildContext context) {
    final look = LookScope.of(context);
    final palette = look.palette;
    final author = skin.author;
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
                skin.name,
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

class _PanelAction extends StatelessWidget {
  const _PanelAction({
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
