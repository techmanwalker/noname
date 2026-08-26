#include "manager-in.hpp"
#include "windowgeometry.hpp"

#include <QQmlEngine>
#include <memory>

using configuration::conf_file_type;

WindowGeometryLI::WindowGeometryLI(QObject *parent, std::shared_ptr<configuration::manager> confmanager) 
    : QObject(parent),
      cm(confmanager)
{

    const auto lines = cm->read_lines(conf_file_type::window_geometry);

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

void WindowGeometryLI::save(int width, int height) {
    if (width < MIN_WIDTH || height < MIN_HEIGHT) return;

    cm->write_lines(
        conf_file_type::window_geometry,
        { QString::number(width), QString::number(height) }
    );
}

int WindowGeometryLI::width()  const { return m_width;  }
int WindowGeometryLI::height() const { return m_height; }