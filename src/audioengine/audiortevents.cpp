#include "audiortevents.hpp"

rt_event_mailbox::rt_event_mailbox () 
    : m_fd(::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC)) 
{
}

rt_event_mailbox::~rt_event_mailbox () 
{
    if (m_fd >= 0) ::close(m_fd);
}

void
rt_event_mailbox::post (rt_event e) noexcept
{
    m_counts[idx(e)].fetch_add(1, std::memory_order_release);
    const uint64_t one = 1;
    const ssize_t r = ::write(m_fd, &one, sizeof one);
    (void) r; // EAGAIN only on counter overflow: the counters above still hold the truth
}

void
rt_event_mailbox::post_error (rt_event e, int soundio_err) noexcept
{
    m_last_error[idx(e)].store(soundio_err, std::memory_order_relaxed);
    post(e); // the release add publishes the error code too
}

int      rt_event_mailbox::fd () const noexcept { return m_fd; }
uint32_t rt_event_mailbox::total (rt_event e) const noexcept { return m_counts[idx(e)].load(std::memory_order_acquire); }
int      rt_event_mailbox::last_error (rt_event e) const noexcept { return m_last_error[idx(e)].load(std::memory_order_relaxed); }

void
rt_event_mailbox::clear_wakeup () noexcept
{
    uint64_t v;
    const ssize_t r = ::read(m_fd, &v, sizeof v);
    (void) r;
}