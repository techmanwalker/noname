#pragma once

#include <QObject>
#include <QtQmlIntegration>
#include <memory>

#include "manager-in.hpp"
#include "windowgeometry-in.hpp"

class WindowGeometryLI : public QObject, public WindowGeometry
{
    Q_OBJECT

public:
    explicit WindowGeometryLI(QObject *parent, std::shared_ptr<configuration::manager> confmanager);

    int width() const override;
    int height() const override;

    Q_INVOKABLE void save(int width, int height) override;

signals:
    void widthChanged();
    void heightChanged();

private:

    int m_width = 1200;
    int m_height = 800;

    static constexpr int MIN_WIDTH = 300;
    static constexpr int MIN_HEIGHT = 200;

    std::shared_ptr<configuration::manager> cm;
};