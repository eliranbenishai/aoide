import 'package:flutter/material.dart';

import 'theme/tramp_theme.dart';
import 'ui/tramp_shell.dart';

class TrampApp extends StatelessWidget {
  const TrampApp({super.key, this.launchArgs = const []});

  final List<String> launchArgs;

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: 'Tramp',
      debugShowCheckedModeBanner: false,
      theme: buildTrampTheme(),
      home: const TrampShell(
        transport: SizedBox.shrink(),
        playlist: SizedBox.shrink(),
      ),
    );
  }
}
