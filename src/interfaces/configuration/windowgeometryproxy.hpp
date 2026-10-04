#pragma once

#include "windowgeometry-in.hpp"

#include <QObject>
#include <QtQmlIntegration>

class WindowGeometryProxy : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(WindowGeometry)
    QML_SINGLETON

public:
    Q_PROPERTY(int width  READ width  WRITE poll_width  NOTIFY sizeChanged)
    Q_PROPERTY(int height READ height WRITE poll_height NOTIFY sizeChanged)

    explicit WindowGeometryProxy (QObject *parent = nullptr)        
        : QObject(parent),
          m_geometry(s_injectedGeometry) // Copies shared_ptr, incrementing ref count
    {
        // Opposite direction from the LyricsManifest/LocalLibrary proxies:
        // going interface* -> QObject*, which qobject_cast cannot do (it only
        // casts FROM a QObject). This is a genuine cross-cast between sibling
        // bases of the same PlayerPresenterLI object, hence dynamic_cast here.
        if (auto *concrete = dynamic_cast<QObject*>(m_geometry.get())) {
            connect(concrete, SIGNAL(sizeChanged()), this, SIGNAL(sizeChanged()));
        }
    }

    static void inject(const std::shared_ptr<WindowGeometry> &geometry) {
        s_injectedGeometry = geometry; // Ref count incremented, caller's instance unaffected
    }

    int width ()  { 
        if (!m_geometry) return 0;

        return m_geometry->width();;
    }

    int height () {
        if (!m_geometry) return 0;

        return m_geometry->height();
    }

    void poll_width (int width) {
        if (!m_geometry) return;

        m_geometry->poll_width(width);
    }

    void poll_height (int height) {
        if (!m_geometry) return;

        m_geometry->poll_height(height);
    }

    Q_INVOKABLE void poll_and_save_to_disk (int width, int height) { if (m_geometry) m_geometry->poll_and_save_to_disk(width, height); }

signals:
    void sizeChanged();

private:
    std::shared_ptr<WindowGeometry> m_geometry;
    inline static std::shared_ptr<WindowGeometry> s_injectedGeometry = nullptr;
};