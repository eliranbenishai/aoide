import 'package:flutter/material.dart';

import 'theme/tramp_colors.dart';
import 'theme/tramp_theme.dart';

class TrampApp extends StatelessWidget {
  const TrampApp({super.key, this.launchArgs = const []});

  final List<String> launchArgs;

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: 'Tramp',
      debugShowCheckedModeBanner: false,
      theme: buildTrampTheme(),
      home: Scaffold(
        backgroundColor: TrampColors.surface,
        body: Center(
          child: Text(
            'Tramp scaffold'
            '${launchArgs.isEmpty ? '' : ' args=${launchArgs.length}'}',
          ),
        ),
      ),
    );
  }
}
