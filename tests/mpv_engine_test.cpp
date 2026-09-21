#include "mpv_engine.h"
#include "equalizer.h"
#include "settings.h"
#include "wav_reader.h"

#include <QDataStream>
#include <QFile>
#include <QJsonDocument>
#include <QTemporaryDir>
#include <QTimer>
#include <QtTest>
#include <clocale>
#include <cmath>
#include <mpv/client.h>

namespace aoide {
class MpvEngineTest : public QObject {
  Q_OBJECT

  static QByteArray tone(int seconds = 1) {
    QByteArray bytes;
    QDataStream out(&bytes, QIODevice::WriteOnly);
    out.setByteOrder(QDataStream::LittleEndian);
    constexpr int rate = 48000;
    const int count = rate * seconds;
    out.writeRawData("RIFF", 4);
    out << quint32(36 + count * 2);
    out.writeRawData("WAVEfmt ", 8);
    out << quint32(16) << quint16(1) << quint16(1) << quint32(rate)
        << quint32(rate * 2) << quint16(2) << quint16(16);
    out.writeRawData("data", 4);
    out << quint32(count * 2);
    for (int i = 0; i < count; ++i)
      out << qint16(1000 * std::sin(2 * 3.141592653589793 * 60 * i / rate));
    return bytes;
  }

  static void property(MpvEngine& engine, const char* name, const QByteArray& value) {
    const int rc = mpv_set_property_string(engine.mpv_, name, value.constData());
    QVERIFY2(rc >= 0, mpv_error_string(rc));
  }

  struct Render {
    PcmBuffer pcm;
    bool bypassed = false;
  };

  void render(const QString& af, double preamp, Render& result, double volume = 1) {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString source = dir.filePath("tone.wav");
    const QString output = dir.filePath("out.wav");
    QFile input(source);
    QVERIFY(input.open(QIODevice::WriteOnly));
    input.write(tone());
    input.close();
    MpvEngine engine;
    QVERIFY(engine.available());
    property(engine, "ao", "pcm");
    property(engine, "ao-pcm-file", QFile::encodeName(output));
    property(engine, "ao-pcm-waveheader", "yes");
    property(engine, "untimed", "yes");
    bool completed = false;
    QString error;
    engine.onCompleted = [&] { completed = true; };
    engine.onError = [&](QString message) { error = message; };
    engine.setVolume(volume);
    engine.setEqualizerAf(af, preamp);
    Track track;
    track.path = source;
    engine.open(track);
    engine.play();
    QTRY_VERIFY_WITH_TIMEOUT(completed || !error.isEmpty(), 3000);
    QVERIFY2(error.isEmpty(), qPrintable(error));
    QVERIFY(completed);
    result.bypassed = !af.isEmpty() && engine.pendingAf_.isEmpty();
    if (result.bypassed) QCOMPARE(engine.pendingPreampDb_, 0);
    engine.dispose();
    QFile pcm(output);
    QVERIFY(pcm.open(QIODevice::ReadOnly));
    result.pcm = WavReader().read(pcm.readAll());
    QVERIFY(result.pcm.samples.size() >= 47000);
  }

  static double rms(const PcmBuffer& pcm) {
    if (pcm.samples.size() <= 5000) return 0;
    double energy = 0;
    // Skip filter startup transients.
    for (qsizetype i = 5000; i < pcm.samples.size(); ++i)
      energy += pcm.samples[i] * pcm.samples[i];
    return std::sqrt(energy / (pcm.samples.size() - 5000));
  }

 private slots:
  void initTestCase() {
    // Match application startup: Qt adopts the host locale, while libmpv
    // requires the C numeric locale even for locales that use a decimal dot.
    QVERIFY(std::setlocale(LC_NUMERIC, "C") != nullptr);
  }

  void flatEnabledProducesAudio() {
    EqualizerSettings eq;
    eq.enabled = true;
    Render enabled, disabled;
    render(buildEqualizerAf(eq), 0, enabled);
    render({}, 0, disabled);
    QVERIFY(!enabled.bypassed);
    QVERIFY(rms(enabled.pcm) > 0.01);
    QVERIFY(std::abs(rms(enabled.pcm) / rms(disabled.pcm) - 1) < 0.01);
  }

  void curveChangesAudio() {
    EqualizerSettings eq;
    eq.enabled = true;
    eq.gains = {12, 8.43, 6.16, -1, 1.30, 1, 3, 1.62, 5, 5};
    Render curve, flat;
    render(buildEqualizerAf(eq), 0, curve);
    render({}, 0, flat);
    QVERIFY(!curve.bypassed);
    const double gain = 20 * std::log10(rms(curve.pcm) / rms(flat.pcm));
    qInfo() << "Measured curve gain at 60 Hz:" << gain << "dB";
    QVERIFY(gain > 11 && gain < 16);
  }

  void preampUsesSoftwareVolume_data() {
    QTest::addColumn<double>("preamp");
    QTest::addColumn<double>("volume");
    QTest::newRow("boost") << 12.0 << 1.0;
    QTest::newRow("cut") << -12.0 << 1.0;
    QTest::newRow("partial-volume") << 6.0 << 0.5;
  }

  void preampUsesSoftwareVolume() {
    QFETCH(double, preamp);
    QFETCH(double, volume);
    EqualizerSettings eq;
    eq.enabled = true;
    Render withPreamp, withoutPreamp;
    render(buildEqualizerAf(eq), preamp, withPreamp, volume);
    render({}, preamp, withoutPreamp, volume); // Disabled EQ ignores stored preamp.
    QVERIFY(!withPreamp.bypassed);
    const double actual = 20 * std::log10(rms(withPreamp.pcm) / rms(withoutPreamp.pcm));
    QVERIFY2(std::abs(actual - preamp) < 0.15, qPrintable(QString::number(actual)));
  }

  void invalidFilterStillProducesAudio_data() {
    QTest::addColumn<QString>("af");
    QTest::newRow("mpv-parser") << QStringLiteral("aoide_nonexistent_filter");
    QTest::newRow("lavfi-deferred") << QStringLiteral("lavfi=[aoide_nonexistent_filter]");
    QTest::newRow("invalid-equalizer-option") << QStringLiteral("lavfi=[equalizer=bogus=1]");
  }

  void invalidFilterStillProducesAudio() {
    QFETCH(QString, af);
    // Both the exact failed graph and the recovery must be observable.
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(
        "Aoide EQ: bypassing equalizer; continuing without EQ\\..*" +
        QRegularExpression::escape(af)));
    Render failed, baseline;
    render(af, 12, failed);
    render({}, 0, baseline);
    QVERIFY(failed.bypassed);
    QVERIFY(std::abs(rms(failed.pcm) / rms(baseline.pcm) - 1) < 0.01);
  }

  void savedEnabledCurvePlaysAfterRestart() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    AoideSettings saved;
    saved.equalizerCurve.enabled = true;
    saved.equalizerCurve.preamp = -2.5;
    saved.equalizerCurve.gains = {12, 8.43, 6.16, -1, 1.30, 1, 3, 1.62, 5, 5};
    const QByteArray json = QJsonDocument(saved.toJson()).toJson();
    QFile file(dir.filePath("settings.json"));
    QVERIFY(file.open(QIODevice::WriteOnly));
    QCOMPARE(file.write(json), json.size());
    file.close();
    for (int launch = 0; launch < 2; ++launch) {
      QVERIFY(file.open(QIODevice::ReadOnly));
      const auto loaded = AoideSettings::fromJson(QJsonDocument::fromJson(file.readAll()).object());
      file.close();
      QCOMPARE(loaded.equalizerCurve.enabled, true);
      QCOMPARE(loaded.equalizerCurve.preamp, saved.equalizerCurve.preamp);
      QCOMPARE(loaded.equalizerCurve.gains, saved.equalizerCurve.gains);
      Render audio;
      render(buildEqualizerAf(loaded.equalizerCurve), loaded.equalizerCurve.preamp, audio);
      QVERIFY(!audio.bypassed);
      QVERIFY(rms(audio.pcm) > 0.03);
    }
    QVERIFY(file.open(QIODevice::ReadOnly));
    QCOMPARE(file.readAll(), json);
  }

  void liveTogglesAndInvalidFilterKeepClockRunning() {
    QTemporaryDir dir;
    QFile source(dir.filePath("tone.wav"));
    QVERIFY(source.open(QIODevice::WriteOnly));
    source.write(tone(10));
    source.close();
    MpvEngine engine;
    QVERIFY(engine.available());
    property(engine, "ao", "null");
    QString error;
    int completions = 0;
    engine.onError = [&](QString message) { error = message; };
    engine.onCompleted = [&] { ++completions; };
    Track track;
    track.path = source.fileName();
    engine.open(track);
    engine.play();
    QTRY_VERIFY_WITH_TIMEOUT(engine.queryPositionMs() > 100, 3000);
    EqualizerSettings eq;
    eq.enabled = true;
    eq.gains[0] = 12;
    engine.setVolume(0.5);
    for (int toggle = 0; toggle < 6; ++toggle) {
      const qint64 before = engine.queryPositionMs();
      const bool enabled = toggle % 2 == 0;
      engine.setEqualizerAf(enabled ? buildEqualizerAf(eq) : QString(), 6);
      QCOMPARE(engine.pendingAf_.isEmpty(), !enabled);
      QTRY_VERIFY_WITH_TIMEOUT(engine.queryPositionMs() > before + 50, 1000);
      QVERIFY2(error.isEmpty(), qPrintable(error));
      double volume = 0;
      QCOMPARE(mpv_get_property(engine.mpv_, "volume", MPV_FORMAT_DOUBLE, &volume), 0);
      QVERIFY(std::abs(volume - 50 * std::pow(10.0, enabled ? 0.1 : 0)) < 0.001);
    }
    const auto before = engine.queryPositionMs();
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(
        "Aoide EQ: bypassing equalizer; continuing without EQ\\..*"));
    engine.setEqualizerAf(QStringLiteral("lavfi=[aoide_nonexistent_filter]"), 12);
    QTRY_VERIFY_WITH_TIMEOUT(engine.pendingAf_.isEmpty(), 1000);
    QCOMPARE(engine.pendingPreampDb_, 0);
    QTRY_VERIFY_WITH_TIMEOUT(engine.queryPositionMs() > before + 50, 1000);
    QVERIFY2(error.isEmpty(), qPrintable(error));
    QCOMPARE(completions, 0);
    engine.setEqualizerAf(buildEqualizerAf(eq), 0);
    QVERIFY(!engine.pendingAf_.isEmpty()); // An explicit user edit can re-enable EQ.
    engine.stop();
  }

  void nonEqFailureDoesNotRetryForever() {
    QTemporaryDir dir;
    MpvEngine engine;
    QVERIFY(engine.available());
    property(engine, "ao", "null");
    EqualizerSettings eq;
    eq.enabled = true;
    engine.setEqualizerAf(buildEqualizerAf(eq));
    QString error;
    engine.onError = [&](QString message) { error = message; };
    Track track;
    track.path = dir.filePath("missing.wav");
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(
        "Aoide EQ: bypassing equalizer; continuing without EQ\\..*retrying track once.*"));
    engine.open(track);
    engine.play();
    QTRY_VERIFY_WITH_TIMEOUT(!error.isEmpty(), 3000);
    QVERIFY(engine.pendingAf_.isEmpty());
  }

  void recoveryPreservesPausedResumePosition() {
    QTemporaryDir dir;
    QFile source(dir.filePath("tone.wav"));
    QVERIFY(source.open(QIODevice::WriteOnly));
    source.write(tone(10));
    source.close();
    MpvEngine engine;
    QVERIFY(engine.available());
    property(engine, "ao", "null");
    QString error;
    engine.onError = [&](QString message) { error = message; };
    engine.setEqualizerAf(QStringLiteral("lavfi=[aoide_nonexistent_filter]"), 6);
    Track track;
    track.path = source.fileName();
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(
        "Aoide EQ: bypassing equalizer; continuing without EQ\\..*"));
    engine.open(track);
    engine.play();
    engine.seekMs(3000);
    engine.pause();
    QTimer clock;
    connect(&clock, &QTimer::timeout, &engine, [&] { engine.queryPositionMs(); });
    clock.start(1);
    QTRY_VERIFY_WITH_TIMEOUT(engine.pendingAf_.isEmpty() &&
                            !engine.restoringAfterEqFailure_, 3000);
    QTRY_VERIFY_WITH_TIMEOUT(engine.queryPositionMs() >= 2990, 3000);
    int paused = 0;
    QCOMPARE(mpv_get_property(engine.mpv_, "pause", MPV_FORMAT_FLAG, &paused), 0);
    QCOMPARE(paused, 1);
    const auto before = engine.queryPositionMs();
    QTest::qWait(100);
    QCOMPARE(engine.queryPositionMs(), before);
    QVERIFY2(error.isEmpty(), qPrintable(error));
    engine.play();
    QTRY_VERIFY_WITH_TIMEOUT(engine.queryPositionMs() > before + 50, 1000);
    engine.stop();
  }
};
}  // namespace aoide

QTEST_GUILESS_MAIN(aoide::MpvEngineTest)
#include "mpv_engine_test.moc"
