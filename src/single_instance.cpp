#include "single_instance.h"

#include <QCryptographicHash>
#include <QDeadlineTimer>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QLocalSocket>
#include <QScopedValueRollback>
#include <QStandardPaths>
#include <QThread>
#include <QTimer>
#include <utility>

#ifdef Q_OS_UNIX
#include <sys/un.h>
#include <unistd.h>
#endif

#ifdef Q_OS_LINUX
#include <cerrno>
#include <fcntl.h>
#include <sys/file.h>
#endif

namespace aoide {
namespace {

constexpr int kStartupTimeoutMs = 30000;
constexpr int kMaxRequestBytes = 16 * 1024 * 1024;

QString endpointName(QString profile, [[maybe_unused]] int& directoryFd) {
#ifdef Q_OS_WIN
  profile = profile.toCaseFolded();
#endif
  const QByteArray key = QCryptographicHash::hash(profile.toUtf8(), QCryptographicHash::Sha256);
  const QString name = QStringLiteral("aoide-") + QString::fromLatin1(key.toHex().left(32));
#ifdef Q_OS_UNIX
  // macOS ignores socket access modes. A private parent directory also
  // protects the path while a dead owner's endpoint is being replaced.
#ifdef Q_OS_MACOS
  // Darwin's short sockaddr_un also has to fit with a long account/temp path.
  const QString dir = QDir(QStringLiteral("/tmp")).filePath(QStringLiteral("aoide-%1").arg(geteuid()));
#else
  const QString temporary = QDir(QDir::tempPath()).filePath(QStringLiteral("aoide-%1").arg(geteuid()));
  const QString runtime = QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation);
  // The Flatpak already shares xdg-run/aoide between its sandbox instances.
  const QString dir = runtime.isEmpty() ? temporary
                                        : QDir(runtime).filePath(QStringLiteral("aoide/instance"));
#endif
  const auto ownerOnly = QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner;
  if (!QDir().mkpath(dir, ownerOnly)) return {};
  const QFileInfo info(dir);
  const auto others = QFileDevice::ReadGroup | QFileDevice::WriteGroup | QFileDevice::ExeGroup |
                      QFileDevice::ReadOther | QFileDevice::WriteOther | QFileDevice::ExeOther;
  if (info.isSymLink() || !info.isDir() || info.ownerId() != geteuid() ||
      (info.permissions() & others)) return {};
  const QString endpoint = QDir(dir).filePath(name);
  if (QFile::encodeName(endpoint).size() < qsizetype(sizeof(sockaddr_un::sun_path))) return endpoint;
#ifdef Q_OS_LINUX
  // bind/connect take a short fixed-size sockaddr_un. Even when the shared
  // runtime directory has a long name, both processes can address it through
  // their own directory descriptor. Keep the primary's descriptor until close.
  directoryFd = ::open(QFile::encodeName(dir).constData(), O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
  if (directoryFd >= 0) return QStringLiteral("/proc/self/fd/%1/%2").arg(directoryFd).arg(name);
#endif
  return {};
#else
  return name;
#endif
}

QByteArray encodeRequest(const QStringList& paths) {
  QJsonArray names;
  for (const QString& path : paths) names.append(QFileInfo(path).absoluteFilePath());
  return QJsonDocument(names).toJson(QJsonDocument::Compact) + '\n';
}

bool sendRequest(QLocalSocket& socket, const QByteArray& request) {
  QDeadlineTimer deadline(kStartupTimeoutMs);
  if (socket.write(request) != request.size()) return false;
  while (socket.bytesToWrite() > 0) {
    if (!socket.waitForBytesWritten(int(deadline.remainingTime()))) return false;
  }
  while (!socket.canReadLine()) {
    if (!socket.waitForReadyRead(int(deadline.remainingTime())) && !socket.canReadLine()) return false;
  }
  return socket.readLine() == "OK\n";
}

}  // namespace

SingleInstance::SingleInstance() {
#ifdef Q_OS_WIN
  server_.setSocketOptions(QLocalServer::UserAccessOption);
#endif
  connect(&server_, &QLocalServer::newConnection, this, &SingleInstance::acceptRequests);
}

SingleInstance::~SingleInstance() {
#ifdef Q_OS_LINUX
  server_.close();
  if (directoryFd_ >= 0) ::close(directoryFd_);
#endif
}

SingleInstance::Result SingleInstance::start(const QString& profileDirectory,
                                             const QStringList& paths) {
  if (!QDir().mkpath(profileDirectory)) {
    error_ = QStringLiteral("Could not create the Aoide data directory.");
    return Result::error;
  }
  const QString profile = QFileInfo(profileDirectory).canonicalFilePath();
  const QString endpoint = endpointName(profile, directoryFd_);
  if (profile.isEmpty() || endpoint.isEmpty()) {
    error_ = QStringLiteral("Could not create a private local connection for Aoide.");
    return Result::error;
  }
  const QString lockPath = QDir(profile).filePath(QStringLiteral("instance.lock"));
#ifdef Q_OS_LINUX
  // Flatpak instances can see the same profile from different PID namespaces.
  // The kernel owns this lock and releases it on exit. Keep its inode in place.
  lock_ = std::make_unique<QFile>(lockPath);
  if (!lock_->open(QIODevice::ReadWrite)) {
    error_ = lock_->errorString();
    return Result::error;
  }
#else
  lock_ = std::make_unique<QLockFile>(lockPath);
  lock_->setStaleLockTime(0);  // a healthy player can stay open for days
#endif
  QDeadlineTimer deadline(kStartupTimeoutMs);
  do {
#ifdef Q_OS_LINUX
    const bool owner = ::flock(lock_->handle(), LOCK_EX | LOCK_NB) == 0;
    const bool busy = !owner && (errno == EWOULDBLOCK || errno == EINTR);
#else
    const bool owner = lock_->tryLock();
    const bool busy = lock_->error() == QLockFile::LockFailedError;
#endif
    if (owner) {
      // Only the current lock owner may clear the socket left by a crashed player.
      QLocalServer::removeServer(endpoint);
      if (server_.listen(endpoint)) return Result::primary;
      error_ = server_.errorString();
      lock_.reset();
      return Result::error;
    }
    if (!busy) {
      error_ = QStringLiteral("Could not lock the Aoide data directory.");
      return Result::error;
    }
    QLocalSocket socket;
    socket.connectToServer(endpoint);
    if (socket.waitForConnected(100)) {
      const QByteArray request = encodeRequest(paths);
      if (request.size() > kMaxRequestBytes) {
        error_ = QStringLiteral("The list of files is too large to send to Aoide.");
        return Result::error;
      }
      if (sendRequest(socket, request)) return Result::forwarded;
      error_ = QStringLiteral("The running Aoide player did not acknowledge this request.");
      return Result::error;
    }
    // Another process may have won the lock and not yet bound its endpoint.
    QThread::msleep(25);
  } while (!deadline.hasExpired());
  error_ = QStringLiteral("Could not connect to the running Aoide player.");
  return Result::error;
}

void SingleInstance::acceptRequests() {
  while (QLocalSocket* socket = server_.nextPendingConnection()) {
    socket->setReadBufferSize(kMaxRequestBytes + 1);
    connect(socket, &QLocalSocket::disconnected, socket, &QObject::deleteLater);
    auto receive = [this, socket] {
      if (socket->state() != QLocalSocket::ConnectedState) return;
      if (socket->bytesAvailable() > kMaxRequestBytes) {
        socket->abort();
        return;
      }
      if (!socket->canReadLine()) return;
      const QJsonDocument doc = QJsonDocument::fromJson(socket->readLine());
      if (!doc.isArray()) {
        socket->abort();
        return;
      }
      QStringList paths;
      for (const QJsonValue& name : doc.array()) {
        if (!name.isString() || !QDir::isAbsolutePath(name.toString())) {
          socket->abort();
          return;
        }
        paths.push_back(name.toString());
      }
      pending_.enqueue(std::move(paths));
      socket->write("OK\n");
      socket->disconnectFromServer();
      // Opening a file can start a modal dialog. Acknowledge receipt before
      // dispatch so the launching process never waits for a listener's choice.
      QTimer::singleShot(0, this, &SingleInstance::drainRequests);
    };
    connect(socket, &QLocalSocket::readyRead, this, receive);
    QTimer::singleShot(kStartupTimeoutMs, socket, &QLocalSocket::abort);
    receive();  // the peer may already have written before newConnection
  }
}

void SingleInstance::setHandler(Handler handler) {
  handler_ = std::move(handler);
  drainRequests();
}

void SingleInstance::drainRequests() {
  if (!handler_ || dispatching_) return;
  QScopedValueRollback<bool> guard(dispatching_, true);
  while (handler_ && !pending_.isEmpty()) handler_(pending_.dequeue());
}

}  // namespace aoide
