// windowgeometry.cpp
#include "configuration.hpp"
#include "windowgeometry.hpp"

#include <QQmlEngine>

using configuration::conf_file_type;

WindowGeometry::WindowGeometry(QObject *parent) : QObject(parent) {

    auto &conf = configuration::manager::instance();
    const auto lines = conf.read_lines(conf_file_type::window_geometry);

    if (lines.size() == 2) {
        bool wOk = false, hOk = false;
        const int w = lines[0].toInt(&wOk);
        const int h = lines[1].toInt(&hOk);

        // Never trust a stored size blindly — a corrupt/hand-edited
        // value here should fall back to defaults, not brick the window
        if (wOk && hOk && w >= MIN_WIDTH && h >= MIN_HEIGHT) {
            m_width = w;
            m_height = h;
        }
    }
}

WindowGeometry &WindowGeometry::instance() {
    static WindowGeometry inst;
    return inst;
}

WindowGeometry *WindowGeometry::create(QQmlEngine *, QJSEngine *) {
    auto *inst = &instance();
    QJSEngine::setObjectOwnership(inst, QJSEngine::CppOwnership);
    return inst;
}

void WindowGeometry::save(int width, int height) {
    if (width < MIN_WIDTH || height < MIN_HEIGHT) return;

    configuration::manager::instance().write_lines(
        conf_file_type::window_geometry,
        { QString::number(width), QString::number(height) }
    );
}

int WindowGeometry::width()  const { return m_width;  }
int WindowGeometry::height() const { return m_height; }