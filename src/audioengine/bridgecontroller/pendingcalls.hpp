#pragma once

#include <deque>

#include <QFuture>
#include <QPromise>

// A call that did not take effect (superseded, refused, failed): its future ends cancelled.
inline void
abandon (QPromise<void> &call)
{
    call.future().cancel();
    call.finish();
}

/**
    The calls of one function group that the decoder thread still owes a settlement for. It runs
    its jobs FIFO, so they settle in issue order. Last call wins: a call that settles while a
    newer one is in flight was superseded and ends cancelled. Engine thread only.
*/
class pending_calls
{
public:
    void add (QPromise<void> call) { m_calls.push_back(std::move(call)); }
    bool empty () const            { return m_calls.empty(); }
    bool last_one () const         { return m_calls.size() == 1; }   // settling it leaves the group quiet

    void settle_oldest (bool ok)
    {
        Q_ASSERT(!m_calls.empty());
        if (m_calls.empty()) return;

        QPromise<void> call = std::move(m_calls.front());
        m_calls.pop_front();

        if (ok && m_calls.empty()) call.finish();
        else                       abandon(call);
    }

private:
    std::deque<QPromise<void>> m_calls;
};
