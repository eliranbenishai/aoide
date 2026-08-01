import 'package:flutter/material.dart';

class TrampApp extends StatelessWidget {
  const TrampApp({super.key, this.launchArgs = const []});

  final List<String> launchArgs;

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: 'Tramp',
      debugShowCheckedModeBanner: false,
      home: Scaffold(
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
