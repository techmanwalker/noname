#pragma once

#include <cassert>
#include <memory>
#include <new>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/frame.h>
}

struct av_packet_deleter { void operator() (AVPacket *p) const { av_packet_free(&p); } };
struct av_frame_deleter  { void operator() (AVFrame  *f) const { av_frame_free(&f); } };

// One packet + one frame, allocated once and reused every decode tick.
//
// Invariant: both are EMPTY whenever no lease exists. The only way to reach them is
// through a lease, whose destructor unrefs both on every exit path. Data from one tick
// (or from a file/position that was since replaced) can therefore never be seen by the next.
class av_scratch
{
public:
    class lease
    {
    public:
        explicit lease (av_scratch &owner) : m_owner(owner)
        {
            assert(!m_owner.m_leased && "av_scratch leased twice (re-entrant decode_step?)");
            assert(m_owner.empty()   && "av_scratch handed out non-empty");
            m_owner.m_leased = true;
        }

        ~lease ()
        {
            av_packet_unref(m_owner.m_pkt.get());
            av_frame_unref(m_owner.m_frame.get());
            m_owner.m_leased = false;
        }

        lease (const lease &) = delete;
        lease &operator= (const lease &) = delete;

        AVPacket *pkt   () const { return m_owner.m_pkt.get(); }
        AVFrame  *frame () const { return m_owner.m_frame.get(); }

    private:
        av_scratch &m_owner;
    };

    av_scratch () : m_pkt(av_packet_alloc()), m_frame(av_frame_alloc())
    {
        if (!m_pkt || !m_frame) throw std::bad_alloc();   // no null state exists afterwards
    }

    [[nodiscard]] lease acquire () { return lease(*this); }

private:
    bool empty () const { return !m_pkt->data && m_pkt->size == 0 && !m_frame->data[0]; }

    std::unique_ptr<AVPacket, av_packet_deleter> m_pkt;
    std::unique_ptr<AVFrame,  av_frame_deleter>  m_frame;
    bool m_leased = false;   // only touched under m_seek_decode_mutex
};