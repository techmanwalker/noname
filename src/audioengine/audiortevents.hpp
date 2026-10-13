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
    rt_event_mailbox ();
    ~rt_event_mailbox ();

    rt_event_mailbox (const rt_event_mailbox &) = delete;
    rt_event_mailbox &operator= (const rt_event_mailbox &) = delete;

    // producer side: any thread, safe in the RT callback
    void post (rt_event e) noexcept;

    // Error code is kept per kind, so two different failing calls don't overwrite each other.
    void post_error (rt_event e, int soundio_err) noexcept;

    // consumer side (Qt thread)
    int      fd () const noexcept;
    uint32_t total (rt_event e) const noexcept;
    int      last_error (rt_event e) const noexcept;

    void clear_wakeup () noexcept;

private:
    static constexpr size_t idx (rt_event e) { return static_cast<size_t>(e); }

    int m_fd;
    std::array<std::atomic<uint32_t>, rt_event_count> m_counts {};
    std::array<std::atomic<int>, rt_event_count> m_last_error {};

    static_assert(std::atomic<uint32_t>::is_always_lock_free);
};