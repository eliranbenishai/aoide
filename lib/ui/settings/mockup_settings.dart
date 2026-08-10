import 'package:flutter/material.dart';

import '../../domain/tramp_settings.dart';
import '../../theme/look_scope.dart';
import '../../theme/tramp_text.dart';
import '../session/session_messages.dart';
import 'skins_panel.dart';

/// Settings body: side tabs (General / Skins) + single Reset Settings action.
class MockupSettings extends StatefulWidget {
  const MockupSettings({
    super.key,
    required this.snapshot,
    this.onSessionCommand,
  });

  final SettingsSnapshotEvent snapshot;
  final ValueChanged<SessionCommand>? onSessionCommand;

  @override
  State<MockupSettings> createState() => _MockupSettingsState();
}

class _MockupSettingsState extends State<MockupSettings> {
  int _tabIndex = 0;

  void _emit(SessionCommand command) =>
      widget.onSessionCommand?.call(command);

  Future<void> _confirmReset() async {
    final look = LookScope.of(context);
    final palette = look.palette;
    final ok = await showDialog<bool>(
      context: context,
      builder: (context) => AlertDialog(
        backgroundColor: palette.shellMid,
        title: Text(
          'Reset settings?',
          style: TrampText.chromeLabel(look).copyWith(color: palette.inkDefault),
        ),
        content: Text(
          'This restores all Tramp settings to defaults, including window '
          'positions and the active skin.',
          style: TrampText.lcd(look).copyWith(color: palette.inkDim),
        ),
        actions: [
          TextButton(
            onPressed: () => Navigator.of(context).pop(false),
            child: Text(
              'Cancel',
              style: TrampText.chromeLabel(look).copyWith(color: palette.inkDim),
            ),
          ),
          TextButton(
            key: const Key('settings-reset-confirm'),
            onPressed: () => Navigator.of(context).pop(true),
            child: Text(
              'Reset',
              style: TrampText.chromeLabel(look)
                  .copyWith(color: palette.accentDefault),
            ),
          ),
        ],
      ),
    );
    if (ok == true) {
      _emit(const ResetSettingsCommand());
    }
  }

  @override
  Widget build(BuildContext context) {
    final look = LookScope.of(context);
    final palette = look.palette;
    final snap = widget.snapshot;

    return Column(
      crossAxisAlignment: CrossAxisAlignment.stretch,
      children: [
        Expanded(
          child: Row(
            crossAxisAlignment: CrossAxisAlignment.stretch,
            children: [
              SizedBox(
                width: 108,
                child: ColoredBox(
                  color: palette.shellDeep,
                  child: Column(
                    crossAxisAlignment: CrossAxisAlignment.stretch,
                    children: [
                      _SideTab(
                        label: 'General',
                        selected: _tabIndex == 0,
                        onTap: () => setState(() => _tabIndex = 0),
                      ),
                      _SideTab(
                        label: 'Skins',
                        selected: _tabIndex == 1,
                        onTap: () => setState(() => _tabIndex = 1),
                      ),
                    ],
                  ),
                ),
              ),
              Expanded(
                child: _tabIndex == 0
                    ? _GeneralTab(
                        snapshot: snap,
                        onChanged: (cmd) => _emit(cmd),
                      )
                    : Padding(
                        padding: const EdgeInsets.fromLTRB(8, 8, 8, 8),
                        child: SkinsPanel(
                          skins: snap.skins,
                          activeSkinId: snap.activeSkinId,
                          lastError: snap.lastSkinError,
                          onActivate: (id) => _emit(ActivateSkinCommand(id)),
                          onInstallZipPath: (path) => _emit(
                            InstallSkinPathCommand(
                              path: path,
                              isDirectory: false,
                            ),
                          ),
                          onInstallDirectoryPath: (path) => _emit(
                            InstallSkinPathCommand(
                              path: path,
                              isDirectory: true,
                            ),
                          ),
                          onSetSkinsDirectory: (path) =>
                              _emit(SetSkinsDirectoryCommand(path)),
                          onResetSkinsDirectory: () =>
                              _emit(const SetSkinsDirectoryCommand(null)),
                        ),
                      ),
              ),
            ],
          ),
        ),
        Padding(
          padding: const EdgeInsets.fromLTRB(12, 0, 12, 10),
          child: Align(
            alignment: Alignment.centerLeft,
            child: TextButton(
              key: const Key('settings-reset'),
              onPressed: _confirmReset,
              child: Text(
                'Reset Settings',
                style: TrampText.chromeLabel(look)
                    .copyWith(color: palette.accentDefault),
              ),
            ),
          ),
        ),
      ],
    );
  }
}

class _SideTab extends StatelessWidget {
  const _SideTab({
    required this.label,
    required this.selected,
    required this.onTap,
  });

  final String label;
  final bool selected;
  final VoidCallback onTap;

  @override
  Widget build(BuildContext context) {
    final look = LookScope.of(context);
    final palette = look.palette;
    return Material(
      color: selected
          ? palette.shellHighlight.withValues(alpha: 0.4)
          : Colors.transparent,
      child: InkWell(
        onTap: onTap,
        child: Container(
          alignment: Alignment.centerLeft,
          padding: const EdgeInsets.symmetric(horizontal: 12, vertical: 14),
          decoration: BoxDecoration(
            border: Border(
              left: BorderSide(
                color: selected ? palette.phosphorDefault : Colors.transparent,
                width: 3,
              ),
            ),
          ),
          child: Text(
            label,
            style: TrampText.chromeLabel(look).copyWith(
              fontSize: 12,
              color: selected ? palette.phosphorDefault : palette.inkDim,
            ),
          ),
        ),
      ),
    );
  }
}

class _GeneralTab extends StatelessWidget {
  const _GeneralTab({
    required this.snapshot,
    required this.onChanged,
  });

  final SettingsSnapshotEvent snapshot;
  final ValueChanged<UpdateGeneralSettingsCommand> onChanged;

  @override
  Widget build(BuildContext context) {
    final look = LookScope.of(context);
    final palette = look.palette;

    return ListView(
      padding: const EdgeInsets.fromLTRB(16, 12, 16, 12),
      children: [
        _ToggleRow(
          label: 'Resume last session',
          value: snapshot.resumeLastSession,
          onChanged: (v) => onChanged(
            UpdateGeneralSettingsCommand(resumeLastSession: v),
          ),
        ),
        _ToggleRow(
          label: 'Confirm before quit',
          value: snapshot.confirmBeforeQuit,
          onChanged: (v) => onChanged(
            UpdateGeneralSettingsCommand(confirmBeforeQuit: v),
          ),
        ),
        _ToggleRow(
          label: 'Scroll title',
          value: snapshot.scrollTitle,
          onChanged: (v) =>
              onChanged(UpdateGeneralSettingsCommand(scrollTitle: v)),
        ),
        _ToggleRow(
          label: 'Minimize hides secondaries',
          value: snapshot.minimizeHidesSecondaries,
          onChanged: (v) => onChanged(
            UpdateGeneralSettingsCommand(minimizeHidesSecondaries: v),
          ),
        ),
        const SizedBox(height: 8),
        Text(
          'Dock snap strength',
          style: TrampText.chromeLabel(look).copyWith(
            fontSize: 12,
            color: palette.inkDefault,
          ),
        ),
        const SizedBox(height: 6),
        SegmentedButton<DockSnapStrength>(
          segments: const [
            ButtonSegment(value: DockSnapStrength.off, label: Text('Off')),
            ButtonSegment(
              value: DockSnapStrength.normal,
              label: Text('Normal'),
            ),
            ButtonSegment(
              value: DockSnapStrength.strong,
              label: Text('Strong'),
            ),
          ],
          selected: {snapshot.dockSnapStrength},
          onSelectionChanged: (set) {
            final value = set.first;
            onChanged(UpdateGeneralSettingsCommand(dockSnapStrength: value));
          },
        ),
      ],
    );
  }
}

class _ToggleRow extends StatelessWidget {
  const _ToggleRow({
    required this.label,
    required this.value,
    required this.onChanged,
  });

  final String label;
  final bool value;
  final ValueChanged<bool> onChanged;

  @override
  Widget build(BuildContext context) {
    final look = LookScope.of(context);
    final palette = look.palette;
    return SwitchListTile(
      contentPadding: EdgeInsets.zero,
      dense: true,
      title: Text(
        label,
        style: TrampText.chromeLabel(look).copyWith(
          fontSize: 12,
          color: palette.inkDefault,
        ),
      ),
      value: value,
      activeThumbColor: palette.phosphorDefault,
      onChanged: onChanged,
    );
  }
}
