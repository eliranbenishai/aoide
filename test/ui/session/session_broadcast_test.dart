import 'package:flutter_test/flutter_test.dart';
import 'package:tramp/ui/session/session_broadcast.dart';
import 'package:tramp/ui/session/session_messages.dart';

void main() {
  test('a window that has not finished its handshake is not broadcast to', () {
    expect(
      secondaryBroadcastRoles(
        created: {WindowRole.equalizer, WindowRole.playlist},
        ready: const {},
      ),
      isEmpty,
    );
  });

  test('a window that has announced itself is broadcast to', () {
    expect(
      secondaryBroadcastRoles(
        created: {WindowRole.equalizer, WindowRole.playlist},
        ready: {WindowRole.equalizer},
      ),
      [WindowRole.equalizer],
    );
  });

  test('a role with no window is never a target', () {
    expect(
      secondaryBroadcastRoles(
        created: const {},
        ready: {WindowRole.playlist},
      ),
      isEmpty,
    );
  });

  test('the main window is never a broadcast target', () {
    expect(
      secondaryBroadcastRoles(
        created: {WindowRole.main},
        ready: {WindowRole.main},
      ),
      isEmpty,
    );
  });
}
