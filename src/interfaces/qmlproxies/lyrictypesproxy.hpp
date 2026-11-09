#pragma once

#include <QObject>
#include <QString>
#include <QtQmlIntegration/qqmlintegration.h>

struct Lyric {
    Q_GADGET
    Q_PROPERTY(quint64 timestamp MEMBER timestamp);
    Q_PROPERTY(QString text MEMBER text);

public:
    quint64 timestamp;
    QString text;
};

struct LyricForeign {
    Q_GADGET
    QML_FOREIGN(Lyric)
    QML_VALUE_TYPE(lyric)
};
