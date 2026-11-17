#pragma once

#include <QFuture>
#include <QHash>
#include <QLoggingCategory>
#include <QReadWriteLock>
#include <QString>
#include <QUrl>

#include "virtual/manager.hpp"

namespace configuration {

Q_DECLARE_LOGGING_CATEGORY(l_configuration)

QString file (setting s);

class managerLI : public QObject, public manager
{
    Q_OBJECT

public:
    explicit managerLI(QObject *parent);
    ~managerLI() override = default;

    std::optional<QStringList> fetch_setting (setting s) override;

    // its QFuture means when the new content has finished writing to disk
    QFuture<bool> write_setting (setting s, const QStringList &lines) override;

private:
    mutable QReadWriteLock m_lock; // internal lock for threads within the same instance
};

}
