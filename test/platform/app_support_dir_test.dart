import 'package:flutter_test/flutter_test.dart';
import 'package:tramp/platform/app_support_dir.dart';

/// Pretends the listed directories exist and nothing else does.
bool Function(String) only(List<String> existing) =>
    (path) => existing.contains(path);

void main() {
  const home = '/home/listener';
  const share = '$home/.local/share';
  const pinned = '$share/$trampApplicationId';
  const legacy = '$share/tramp';

  group('Linux support directory', () {
    test('is the application id under XDG_DATA_HOME', () {
      expect(
        resolveLinuxSupportPath(
          environment: const {'HOME': home, 'XDG_DATA_HOME': share},
          exists: only(const [pinned]),
        ),
        pinned,
      );
    });

    test('falls back to the XDG default when XDG_DATA_HOME is unset', () {
      expect(
        resolveLinuxSupportPath(
          environment: const {'HOME': home},
          exists: only(const [pinned]),
        ),
        pinned,
      );
    });

    test('ignores a relative XDG_DATA_HOME, as the spec requires', () {
      expect(
        resolveLinuxSupportPath(
          environment: const {'HOME': home, 'XDG_DATA_HOME': 'relative/share'},
          exists: only(const [pinned]),
        ),
        pinned,
      );
    });

    test('honours an XDG_DATA_HOME pointing somewhere else entirely', () {
      expect(
        resolveLinuxSupportPath(
          environment: const {'HOME': home, 'XDG_DATA_HOME': '/data/xdg'},
          exists: only(const ['/data/xdg/$trampApplicationId']),
        ),
        '/data/xdg/$trampApplicationId',
      );
    });

    test('adopts the legacy executable-name directory when it holds the data',
        () {
      // path_provider used basename(/proc/self/exe) before it read the
      // application id. Pinning the id must not strand data left there.
      expect(
        resolveLinuxSupportPath(
          environment: const {'HOME': home},
          exists: only(const [legacy]),
        ),
        legacy,
      );
    });

    test('prefers the application id when both directories exist', () {
      expect(
        resolveLinuxSupportPath(
          environment: const {'HOME': home},
          exists: only(const [pinned, legacy]),
        ),
        pinned,
      );
    });

    test('picks the application id for a first run, with neither present', () {
      expect(
        resolveLinuxSupportPath(
          environment: const {'HOME': home},
          exists: only(const []),
        ),
        pinned,
      );
    });

    test('answers the same on every call, however the lookup is asked', () {
      // The whole point of pinning: no runtime lookup, so no run-to-run drift.
      final answers = {
        for (var i = 0; i < 5; i++)
          resolveLinuxSupportPath(
            environment: const {'HOME': home},
            exists: only(const [pinned]),
          ),
      };
      expect(answers, hasLength(1));
    });
  });

  test('the pinned id matches linux/CMakeLists.txt APPLICATION_ID', () {
    // If someone changes one, this fails rather than silently moving every
    // listener's settings, skins, and playlists to a new directory.
    expect(trampApplicationId, 'com.tramp.tramp');
  });
}
