import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:tramp/theme/tramp_text.dart';

void main() {
  const chrome = {
    'chromeLabel': TrampText.chromeLabel,
    'eqScale': TrampText.eqScale,
  };

  const lcd = {
    'lcd': TrampText.lcd,
    'lcdDim': TrampText.lcdDim,
    'lcdLarge': TrampText.lcdLarge,
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
      TrampText.lcdLarge.fontSize!,
      greaterThan(TrampText.lcd.fontSize!),
    );
  });
}
