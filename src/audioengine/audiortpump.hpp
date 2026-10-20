#pragma once

#include <QObject>
#include <QSocketNotifier>
#include <array>

#include "audiortevents.hpp"

// Qt-thread half of rt_event_mailbox: wakes on the eventfd, emits signals, prints logs.
class rt_event_pump : public QObject
{
    Q_OBJECT

public:
    explicit rt_event_pump (rt_event_mailbox &mailbox, QObject *parent = nullptr);

signals:
    void tape_ran_out ();   // A's samples ended, no B
    void boundary_hit ();   // A's samples ended, B follows

private slots:
    void drain ();

private:
    uint32_t take (rt_event e); // events since the previous call

    rt_event_mailbox &m_mailbox;
    QSocketNotifier  *m_notifier;
    std::array<uint32_t, rt_event_count> m_seen {};
};