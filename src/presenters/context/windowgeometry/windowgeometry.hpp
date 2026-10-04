#pragma once

#include <QObject>
#include <QSize>

#include <memory>
#include <qtypes.h>

#include "manager-in.hpp"
#include "windowgeometry-in.hpp"

class WindowGeometryLI : public QObject, public WindowGeometry
{
    Q_OBJECT

public:
    explicit WindowGeometryLI(QObject *parent, std::shared_ptr<configuration::manager> confmanager);

    int width  () const override;
    int height () const override;

    void poll_width  (int width)  override;
    void poll_height (int height) override;

    Q_INVOKABLE void poll_and_save_to_disk (int width, int height) override;

signals:
    void sizeChanged();

private:

    QSize m_size {800, 600};

    static constexpr int MIN_WIDTH = 300;
    static constexpr int MIN_HEIGHT = 200;

    std::shared_ptr<configuration::manager> cm;
};