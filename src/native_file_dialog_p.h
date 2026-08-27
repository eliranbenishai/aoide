#pragma once

#include <QEventLoop>
#include <QObject>
#include <QVariantMap>

namespace aoide {

class PortalWaiter : public QObject {
  Q_OBJECT
 public:
  QEventLoop loop;
  uint code = 2;
  QVariantMap results;

 public slots:
  void onResponse(uint response, const QVariantMap& body) {
    code = response;
    results = body;
    loop.quit();
  }
};

}  // namespace aoide
