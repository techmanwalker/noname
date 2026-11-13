#pragma once

#include "virtual/windowgeometry.hpp"

#include <QObject>
#include <QtQmlIntegration>

#include "objectproxybase.hpp"

// QObject MUST stay the first base: the QML prototype is resolved from it
class WindowGeometryProxy : public QObject,
                            public ObjectProxyBase<WindowGeometryProxy, WindowGeometry>
{
    Q_OBJECT
    QML_NAMED_ELEMENT(WindowGeometry)
    QML_SINGLETON

    Q_PROPERTY(int width  READ width  WRITE poll_width  NOTIFY sizeChanged)
    Q_PROPERTY(int height READ height WRITE poll_height NOTIFY sizeChanged)

public:
    explicit WindowGeometryProxy(QObject *parent = nullptr) : QObject(parent) {
        forward(SIGNAL(size_changed()), SIGNAL(sizeChanged()));
    }

    int  width()  const { return call(&WindowGeometry::width); }
    int  height() const { return call(&WindowGeometry::height); }
    void poll_width(int width)   { call(&WindowGeometry::poll_width, width); }
    void poll_height(int height) { call(&WindowGeometry::poll_height, height); }

signals:
    void sizeChanged();

public slots:
    void poll_and_save_to_disk(int width, int height) {
        call(&WindowGeometry::poll_and_save_to_disk, width, height);
    }
};
