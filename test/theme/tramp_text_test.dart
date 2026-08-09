import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:tramp/look/builtin_look.dart';
import 'package:tramp/theme/tramp_text.dart';

void main() {
  final look = BuiltinLook.resolved;

  final chrome = {
    'chromeLabel': TrampText.chromeLabel(look),
    'eqScale': TrampText.eqScale(look),
  };

  final lcd = {
    'lcd': TrampText.lcd(look),
    'lcdDim': TrampText.lcdDim(look),
    'lcdLarge': TrampText.lcdLarge(look),
  };

  // Weights registered in pubspec.yaml for each family.
  final chromeWeights = {FontWeight.w700};
  final monoWeights = {FontWeight.w500};

  test('chrome styles use TrampCondensed', () {
    for (final entry in chrome.entries) {
      expect(
        entry.value.fontFamily,
        'TrampCondensed',
        reason: entry.key,
      );
    }
  });

  test('LCD styles use TrampMono', () {
    for (final entry in lcd.entries) {
      expect(entry.value.fontFamily, 'TrampMono', reason: entry.key);
    }
  });

  test('every chrome weight is one pubspec bundles', () {
    for (final entry in chrome.entries) {
      expect(
        chromeWeights,
        contains(entry.value.fontWeight),
        reason: entry.key,
      );
    }
  });

  test('every LCD weight is one pubspec bundles', () {
    for (final entry in lcd.entries) {
      expect(monoWeights, contains(entry.value.fontWeight), reason: entry.key);
    }
  });

  test('lcdLarge is larger than lcd', () {
    expect(
      TrampText.lcdLarge(look).fontSize!,
      greaterThan(TrampText.lcd(look).fontSize!),
    );
  });
}
