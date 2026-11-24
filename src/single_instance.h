#pragma once

#include <QLocalServer>
#ifdef Q_OS_LINUX
#include <QFile>
#else
#include <QLockFile>
#endif
#include <QObject>
#include <QQueue>
#include <QStringList>
#include <functional>
#include <memory>

namespace aoide {

class SingleInstance : public QObject {
 public:
  enum class Result { primary, forwarded, error };
  using Handler = std::function<void(const QStringList&)>;

  SingleInstance();
  ~SingleInstance() override;
  Result start(const QString& profileDirectory, const QStringList& paths);
  void setHandler(Handler handler);
  QString errorString() const { return error_; }

 private:
  void acceptRequests();
  void drainRequests();

  // Close the endpoint before releasing ownership of the profile.
#ifdef Q_OS_LINUX
  std::unique_ptr<QFile> lock_;
#else
  std::unique_ptr<QLockFile> lock_;
#endif
  QLocalServer server_;
  QString error_;
  Handler handler_;
  QQueue<QStringList> pending_;
  bool dispatching_ = false;
  int directoryFd_ = -1;
};

}  // namespace aoide
