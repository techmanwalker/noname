#include "manager-in.hpp"
#include "windowgeometry.hpp"

#include <memory>
#include <qtypes.h>

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
            m_size = {w, h};
        }
    }
}

int WindowGeometryLI::width()  const { return m_size.width();  }
int WindowGeometryLI::height() const { return m_size.height(); }

void WindowGeometryLI::poll_width  (int width)  { m_size.setWidth(width);   }
void WindowGeometryLI::poll_height (int height) { m_size.setHeight(height); }

void
WindowGeometryLI::poll_and_save_to_disk (int width, int height)
{
    if (width < MIN_WIDTH || height < MIN_HEIGHT) return;

    m_size = {width, height};

    cm->write_lines(
        conf_file_type::window_geometry,
        { QString::number(width), QString::number(height) }
    );
}