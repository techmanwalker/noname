#pragma once

#include "locallibrary-in.hpp"
#include <QIdentityProxyModel>
#include <QtQmlIntegration/qqmlintegration.h>
#include <memory>

class LocalLibraryProxy : public QIdentityProxyModel
{
    Q_OBJECT
    QML_NAMED_ELEMENT(LocalLibrary)
    QML_SINGLETON

public:
    explicit LocalLibraryProxy(QObject *parent = nullptr)
        : QIdentityProxyModel(parent),
          m_library(s_injectedLibrary), // Copies shared_ptr, incrementing ref count
          m_iface(qobject_cast<LocalLibrary*>(m_library.get()))
    {
        if (m_library) {
            setSourceModel(m_library.get());

            /*  refreshFinished isn't (and can't be) part of the LocalLibrary interface,
                since LocalLibrary isn't a QObject. Bind by name against the injected
                object's runtime meta-object instead of a compile-time &Concrete::signal
                pointer, so this proxy stays free of any #include on LocalLibraryLDB. */
            connect(m_library.get(), SIGNAL(refreshFinished()),
                    this, SIGNAL(refreshFinished()));
        }
    }

    // Non-destructive injection via const reference
    static void inject(const std::shared_ptr<QAbstractListModel> &library) {
        s_injectedLibrary = library; // Ref count incremented, caller's instance unaffected
    }

    // Respect and expose the public interface using the abstraction
    Q_INVOKABLE QFuture<void> snapshot_known_directories() {
        if (!m_iface) return QtFuture::makeReadyVoidFuture(); 

        return m_iface->snapshot_known_directories();
    }

    Q_INVOKABLE QList<Types::Song> flattened() const {
        if (!m_iface) return {}; 
        
        return m_iface->flattened();
    }

    Q_INVOKABLE QStringList flattened_sources() const {
        if (!m_iface) return {}; 

        return m_iface->flattened_sources();
    }

signals:
    void refreshFinished ();

private:
    std::shared_ptr<QAbstractListModel> m_library;
    inline static std::shared_ptr<QAbstractListModel> s_injectedLibrary = nullptr;

    LocalLibrary *m_iface = nullptr; // same lifetime as m_library
};