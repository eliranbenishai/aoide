#include "m3u.h"

#include <QTest>

class M3uEncodingTest : public QObject {
  Q_OBJECT

 private slots:
  void windows1252PunctuationRemainsReadable();
  void hebrewPathsProvideEvidenceForWindows1255();
  void hebrewMetadataWithoutPathEvidenceRequiresReview();
  void validUnicodeStaysUnchanged();
  void windowsPathsResolveAfterHebrewRecovery();
  void mojibakeOnlyChangesWhenRepairedPathsExist();
  void hebrewMojibakeRecoversWindowsPunctuation();
  void conflictingPathEvidenceStillRequiresReview();
  void utf8BomRequiresPathEvidenceBeforeOfferingRecovery();
  void hebrewMojibakeFromLatin1IsReversible();
  void mixedUnicodeMetadataOffersReviewWithoutChangingCorrectText();
  void mixedUnicodePathsUseEvidenceForOnlyTheReversibleLine();
};

void M3uEncodingTest::windows1252PunctuationRemainsReadable() {
  const QByteArray bytes = QByteArray::fromHex(
      "234558544d33550a23455854494e463a312c436166e9209620934c697665940a"
      "436166e92096204c6976652e6d70330a");
  const QString text = aoide::decodeM3uBytes(bytes);
  QCOMPARE(text, QStringLiteral("#EXTM3U\n#EXTINF:1,Café – “Live”\nCafé – Live.mp3\n"));
}

void M3uEncodingTest::hebrewPathsProvideEvidenceForWindows1255() {
  const QByteArray bytes = QByteArray::fromHex(
      "234558544d33550a23455854494e463a3234302cf2e1f8e920ece9e3f8202d20"
      "e6ebe9fae920ece0e4e5e10af2e1f8e920ece9e3f82e6d70330a");
  const QString existing = QStringLiteral("/music/עברי לידר.mp3");
  const aoide::M3uCodec codec([&](const QString& path) { return path == existing; });
  const auto decoded = codec.decode(bytes, QStringLiteral("/music/album.m3u"));
  QCOMPARE(decoded.encoding, QStringLiteral("Windows-1255"));
  QVERIFY(decoded.recovered);
  QVERIFY(!decoded.needsReview);
  QCOMPARE(decoded.matchedPaths, 1);
  const auto tracks = codec.parse(decoded.text, QStringLiteral("/music/album.m3u"));
  QCOMPARE(tracks.size(), 1);
  QCOMPARE(tracks.front().artist, QStringLiteral("עברי לידר"));
  QCOMPARE(tracks.front().title, QStringLiteral("זכיתי לאהוב"));
  QCOMPARE(tracks.front().path, existing);
}

void M3uEncodingTest::hebrewMetadataWithoutPathEvidenceRequiresReview() {
  const QByteArray bytes = QByteArray::fromHex(
      "234558544d33550a23455854494e463a3234302cf2e1f8e920ece9e3f8202d20"
      "e6ebe9fae920ece0e4e5e10a747261636b2e6d70330a");
  const aoide::M3uCodec codec([](const QString& path) {
    return path == QStringLiteral("/music/track.mp3");
  });
  const auto decoded = codec.decode(bytes, QStringLiteral("/music/album.m3u"));
  QCOMPARE(decoded.encoding, QStringLiteral("Windows-1252"));
  QVERIFY(decoded.recovered);
  QVERIFY(decoded.needsReview);
  QCOMPARE(decoded.alternatives.size(), 2);
  QCOMPARE(decoded.alternatives.front().text, decoded.text);
  QCOMPARE(decoded.alternatives.back().encoding, QStringLiteral("Windows-1255"));
  const auto reviewed = codec.parse(decoded.alternatives.back().text,
                                    QStringLiteral("/music/album.m3u"));
  QCOMPARE(reviewed.front().artist, QStringLiteral("עברי לידר"));
  QCOMPARE(reviewed.front().path, QStringLiteral("/music/track.mp3"));
}

void M3uEncodingTest::validUnicodeStaysUnchanged() {
  const aoide::M3uCodec codec([](const QString&) { return false; });
  const QString text = QStringLiteral(
      "#EXTM3U\n#EXTINF:240,עברי לידר - זכיתי לאהוב\nעברי לידר.mp3\n"
      "#EXTINF:180,坂本龍一 - 東京\nBjörk – Jóga.flac\n");
  for (const QByteArray& bytes : {text.toUtf8(), QByteArray::fromHex("efbbbf") + text.toUtf8()}) {
    const auto decoded = codec.decode(bytes, QStringLiteral("/music/album.m3u8"));
    QCOMPARE(decoded.text, text);
    QCOMPARE(decoded.encoding, QStringLiteral("UTF-8"));
    QVERIFY(!decoded.recovered);
    QVERIFY(!decoded.needsReview);
    QCOMPARE(decoded.alternatives.size(), 1);
  }
  const QString utf16Text = QStringLiteral("#EXTM3U\nעברי.mp3\n");
  for (const QByteArray& bytes : {
           QByteArray::fromHex("fffe23004500580054004d00330055000a00e205d105e805d9052e006d00700033000a00"),
           QByteArray::fromHex("feff0023004500580054004d00330055000a05e205d105e805d9002e006d00700033000a")}) {
    const auto decoded = codec.decode(bytes);
    QCOMPARE(decoded.text, utf16Text);
    QCOMPARE(decoded.encoding, QStringLiteral("UTF-16"));
    QVERIFY(!decoded.needsReview);
  }
  const auto binary = codec.decode(QByteArray::fromHex("23004100"));
  QVERIFY(!aoide::isPlaylistText(binary.text));
  QVERIFY(codec.parse(binary.text, QStringLiteral("/music/binary.m3u")).isEmpty());
}

void M3uEncodingTest::windowsPathsResolveAfterHebrewRecovery() {
  const QByteArray bytes = QByteArray("#EXTM3U\nZ:\\old\\") +
      QByteArray::fromHex("f2e1f8e920ece9e3f8") + "\\track.mp3\n";
  const QString existing = QStringLiteral("/music/עברי לידר/track.mp3");
  const aoide::M3uCodec codec([&](const QString& path) { return path == existing; });
  const auto decoded = codec.decode(bytes, QStringLiteral("/music/album.m3u"));
  QCOMPARE(decoded.encoding, QStringLiteral("Windows-1255"));
  QVERIFY(!decoded.needsReview);
  const auto tracks = codec.parse(decoded.text, QStringLiteral("/music/album.m3u"));
  QCOMPARE(tracks.front().path, existing);
}

void M3uEncodingTest::mojibakeOnlyChangesWhenRepairedPathsExist() {
  // Literal UTF-8 for a filename that was previously read as Windows-1252.
  const QByteArray bytes = QByteArray::fromHex(
      "234558544d33550a426ac383c2b6726b2e6d70330a");
  const aoide::M3uCodec noFiles([](const QString&) { return false; });
  const auto uncertain = noFiles.decode(bytes, QStringLiteral("/music/album.m3u"));
  QCOMPARE(uncertain.text, QStringLiteral("#EXTM3U\nBjÃ¶rk.mp3\n"));
  QVERIFY(uncertain.needsReview);
  QVERIFY(!uncertain.recovered);
  QCOMPARE(uncertain.alternatives.size(), 2);
  QCOMPARE(uncertain.alternatives.back().text, QStringLiteral("#EXTM3U\nBjörk.mp3\n"));

  const aoide::M3uCodec repairedExists([](const QString& path) {
    return path == QStringLiteral("/music/Björk.mp3");
  });
  const auto recovered = repairedExists.decode(bytes, QStringLiteral("/music/album.m3u"));
  QCOMPARE(recovered.text, QStringLiteral("#EXTM3U\nBjörk.mp3\n"));
  QVERIFY(recovered.recovered);
  QVERIFY(!recovered.needsReview);

  const aoide::M3uCodec originalExists([](const QString& path) {
    return path == QStringLiteral("/music/BjÃ¶rk.mp3");
  });
  const auto preserved = originalExists.decode(bytes, QStringLiteral("/music/album.m3u"));
  QCOMPARE(preserved.text, QStringLiteral("#EXTM3U\nBjÃ¶rk.mp3\n"));
  QVERIFY(!preserved.recovered);
  QVERIFY(!preserved.needsReview);
}

void M3uEncodingTest::hebrewMojibakeRecoversWindowsPunctuation() {
  const QByteArray bytes = QByteArray::fromHex(
      "234558544d33550ac397c2a2c397e28098c397c2a8c397e284a220c397c593"
      "c397e284a2c397e2809cc397c2a82e6d70330a");
  const aoide::M3uCodec codec([](const QString& path) {
    return path == QStringLiteral("/music/עברי לידר.mp3");
  });
  const auto decoded = codec.decode(bytes, QStringLiteral("/music/album.m3u"));
  QCOMPARE(decoded.text, QStringLiteral("#EXTM3U\nעברי לידר.mp3\n"));
  QVERIFY(decoded.recovered);
  QVERIFY(!decoded.needsReview);
}

void M3uEncodingTest::conflictingPathEvidenceStillRequiresReview() {
  const QByteArray bytes = QByteArray::fromHex(
      "234558544d33550ae0612e6d70330ae0622e6d70330ae0632e6d70330a");
  const aoide::M3uCodec codec([](const QString& path) {
    return path == QStringLiteral("/music/àa.mp3") ||
           path == QStringLiteral("/music/אb.mp3") ||
           path == QStringLiteral("/music/אc.mp3");
  });
  const auto decoded = codec.decode(bytes, QStringLiteral("/music/mixed.m3u"));
  QCOMPARE(decoded.encoding, QStringLiteral("Windows-1255"));
  QVERIFY(decoded.needsReview);
}

void M3uEncodingTest::utf8BomRequiresPathEvidenceBeforeOfferingRecovery() {
  const QByteArray bytes = QByteArray::fromHex(
      "efbbbf234558544d33550a426ac383c2b6726b2e6d70330a");
  const aoide::M3uCodec noFiles([](const QString&) { return false; });
  const auto preserved = noFiles.decode(bytes, QStringLiteral("/music/album.m3u8"));
  QCOMPARE(preserved.text, QStringLiteral("#EXTM3U\nBjÃ¶rk.mp3\n"));
  QCOMPARE(preserved.alternatives.size(), 1);
  QVERIFY(!preserved.needsReview);
  QVERIFY(!preserved.recovered);

  const aoide::M3uCodec repairedExists([](const QString& path) {
    return path == QStringLiteral("/music/Björk.mp3");
  });
  const auto recovered = repairedExists.decode(bytes, QStringLiteral("/music/album.m3u8"));
  QCOMPARE(recovered.text, QStringLiteral("#EXTM3U\nBjörk.mp3\n"));
  QVERIFY(recovered.recovered);
}

void M3uEncodingTest::hebrewMojibakeFromLatin1IsReversible() {
  const QByteArray bytes = QByteArray::fromHex(
      "234558544d33550ac397c2a2c397c291c397c2a8c397c29920c397c29c"
      "c397c299c397c293c397c2a82e6d70330a");
  const aoide::M3uCodec codec([](const QString& path) {
    return path == QStringLiteral("/music/עברי לידר.mp3");
  });
  const auto decoded = codec.decode(bytes, QStringLiteral("/music/album.m3u"));
  QCOMPARE(decoded.text, QStringLiteral("#EXTM3U\nעברי לידר.mp3\n"));
  QCOMPARE(decoded.encoding, QStringLiteral("UTF-8 (recovered from Latin-1)"));
  QVERIFY(decoded.recovered);
  QVERIFY(!decoded.needsReview);
}

void M3uEncodingTest::mixedUnicodeMetadataOffersReviewWithoutChangingCorrectText() {
  const QByteArray garbledArtist = QByteArray::fromHex(
      "c397c2a2c397e28098c397c2a8c397e284a220c397c593c397e284a2c397e2809cc397c2a8");
  const QByteArray bytes = QByteArray("#EXTM3U\r\n#EXTINF:240,") + garbledArtist +
      QStringLiteral(" - זכיתי לאהוב\r\nעברי לידר.mp3\r\n"
                     "#EXTINF:180,坂本龍一 - 東京\r\n東京.flac\r\n").toUtf8();
  const aoide::M3uCodec codec([](const QString& path) {
    return path == QStringLiteral("/music/עברי לידר.mp3") ||
           path == QStringLiteral("/music/東京.flac");
  });
  const auto decoded = codec.decode(bytes, QStringLiteral("/music/mixed.m3u"));
  QCOMPARE(decoded.text, QString::fromUtf8(bytes));
  QVERIFY(decoded.needsReview);
  QVERIFY(!decoded.recovered);
  QCOMPARE(decoded.alternatives.size(), 2);
  const QString expected = QStringLiteral(
      "#EXTM3U\r\n#EXTINF:240,עברי לידר - זכיתי לאהוב\r\nעברי לידר.mp3\r\n"
      "#EXTINF:180,坂本龍一 - 東京\r\n東京.flac\r\n");
  QCOMPARE(decoded.alternatives.back().text, expected);
  const auto reviewed = codec.parse(decoded.alternatives.back().text,
                                    QStringLiteral("/music/mixed.m3u"));
  QCOMPARE(reviewed.size(), 2);
  QCOMPARE(reviewed[0].artist, QStringLiteral("עברי לידר"));
  QCOMPARE(reviewed[0].title, QStringLiteral("זכיתי לאהוב"));
  QCOMPARE(reviewed[0].path, QStringLiteral("/music/עברי לידר.mp3"));
  QCOMPARE(reviewed[1].artist, QStringLiteral("坂本龍一"));
  QCOMPARE(reviewed[1].title, QStringLiteral("東京"));
}

void M3uEncodingTest::mixedUnicodePathsUseEvidenceForOnlyTheReversibleLine() {
  const QByteArray bytes = QStringLiteral(
      "#EXTM3U\n#EXTINF:240,עברי לידר - זכיתי לאהוב\nעברי לידר.mp3\n").toUtf8() +
      QByteArray::fromHex("426ac383c2b6726b2e6d70330a");
  const aoide::M3uCodec codec([](const QString& path) {
    return path == QStringLiteral("/music/עברי לידר.mp3") ||
           path == QStringLiteral("/music/Björk.mp3");
  });
  const auto decoded = codec.decode(bytes, QStringLiteral("/music/mixed.m3u"));
  QCOMPARE(decoded.text, QStringLiteral(
      "#EXTM3U\n#EXTINF:240,עברי לידר - זכיתי לאהוב\nעברי לידר.mp3\nBjörk.mp3\n"));
  QVERIFY(decoded.recovered);
  QVERIFY(!decoded.needsReview);
  QCOMPARE(decoded.matchedPaths, 2);
}

QTEST_APPLESS_MAIN(M3uEncodingTest)
#include "m3u_encoding_test.moc"
