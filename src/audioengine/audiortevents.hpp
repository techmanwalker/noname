#pragma once

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>

#include <sys/eventfd.h>
#include <unistd.h>

enum class rt_event : unsigned { playlist_finished, track_boundary, underflow, stream_error,
                                 begin_write_failed, end_write_failed, count_ };
inline constexpr size_t rt_event_count = static_cast<size_t>(rt_event::count_);

/**
    RT thread -> Qt thread. post() is wait-free and allocation-free: one fetch_add and
    one non-blocking write(2) on an eventfd. Counters (not flags) so no event is ever lost.
*/
class rt_event_mailbox
{
public:
    rt_event_mailbox () : m_fd(::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC)) {}
    ~rt_event_mailbox () { if (m_fd >= 0) ::close(m_fd); }

    rt_event_mailbox (const rt_event_mailbox &) = delete;
    rt_event_mailbox &operator= (const rt_event_mailbox &) = delete;

    // ── producer side: any thread, safe in the RT callback ──
    void post (rt_event e) noexcept
    {
        m_counts[idx(e)].fetch_add(1, std::memory_order_release);
        const uint64_t one = 1;
        const ssize_t r = ::write(m_fd, &one, sizeof one);
        (void) r; // EAGAIN only on counter overflow: the counters above still hold the truth
    }

    // Error code is kept per kind, so two different failing calls don't overwrite each other.
    void post_error (rt_event e, int soundio_err) noexcept
    {
        m_last_error[idx(e)].store(soundio_err, std::memory_order_relaxed);
        post(e); // the release add publishes the error code too
    }

    // ── consumer side (Qt thread) ──
    int      fd () const noexcept { return m_fd; }
    uint32_t total (rt_event e) const noexcept { return m_counts[idx(e)].load(std::memory_order_acquire); }
    int      last_error (rt_event e) const noexcept { return m_last_error[idx(e)].load(std::memory_order_relaxed); }

    void clear_wakeup () noexcept
    {
        uint64_t v;
        const ssize_t r = ::read(m_fd, &v, sizeof v);
        (void) r;
    }

private:
    static constexpr size_t idx (rt_event e) { return static_cast<size_t>(e); }

    int m_fd;
    std::array<std::atomic<uint32_t>, rt_event_count> m_counts {};
    std::array<std::atomic<int>, rt_event_count> m_last_error {};

    static_assert(std::atomic<uint32_t>::is_always_lock_free);
};