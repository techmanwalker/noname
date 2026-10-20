#include "audiortpump.hpp"
#include "l_audioengine.hpp" // l_soundio

#include <soundio/soundio.h>

rt_event_pump::rt_event_pump (rt_event_mailbox &mailbox, QObject *parent)
    : QObject(parent),
      m_mailbox(mailbox),
      m_notifier(new QSocketNotifier(mailbox.fd(), QSocketNotifier::Read, this))
{
    connect(m_notifier, &QSocketNotifier::activated, this, &rt_event_pump::drain);
}

uint32_t
rt_event_pump::take (rt_event e)
{
    const size_t   i   = static_cast<size_t>(e);
    const uint32_t now = m_mailbox.total(e);
    const uint32_t d   = now - m_seen[i]; // unsigned: wrap-safe
    m_seen[i] = now;
    return d;
}

void
rt_event_pump::drain ()
{
    // Clear first: a post() racing with us then re-arms the notifier instead of being lost.
    m_mailbox.clear_wakeup();

    for (uint32_t n = take(rt_event::boundary_hit);  n > 0; --n) emit boundary_hit();
    for (uint32_t n = take(rt_event::tape_ran_out); n > 0; --n) emit tape_ran_out();

    if (const uint32_t n = take(rt_event::underflow)) {
        qCWarning(l_soundio) << "SoundIo outstream underflow (x" << n << ")";
    }
    if (const uint32_t n = take(rt_event::stream_error)) {
        qCWarning(l_soundio) << "SoundIo outstream error:"
                             << soundio_strerror(m_mailbox.last_error(rt_event::stream_error)) << "(x" << n << ")";
    }

    // Same wording execute_soundio used, but printed here instead of at the RT call site
    const auto report_call = [this](rt_event e, const char *name) {
        if (const uint32_t n = take(e)) {
            const int err = m_mailbox.last_error(e);
            qCWarning(l_soundio) << name << "failed with exit code" << err << ":"
                                 << soundio_strerror(err) << "(x" << n << ")";
        }
    };
    report_call(rt_event::begin_write_failed, "soundio_outstream_begin_write");
    report_call(rt_event::end_write_failed,   "soundio_outstream_end_write");
}