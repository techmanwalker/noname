#pragma once

#include <QObject>
#include <QQmlEngine>
#include <QtQmlIntegration>

class WindowGeometry : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(int width READ width NOTIFY widthChanged)
    Q_PROPERTY(int height READ height NOTIFY heightChanged)

public:
    static WindowGeometry &instance();
    static WindowGeometry *create(QQmlEngine *, QJSEngine *);

    int width() const;
    int height() const;

    Q_INVOKABLE void save(int width, int height);

signals:
    void widthChanged();
    void heightChanged();

private:
    explicit WindowGeometry(QObject *parent = nullptr);
    Q_DISABLE_COPY(WindowGeometry)

    int m_width = 1200;
    int m_height = 800;

    static constexpr int MIN_WIDTH = 300;
    static constexpr int MIN_HEIGHT = 200;
};