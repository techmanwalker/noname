#pragma once

#include "virtual/mediafilter.hpp"

// forward declaration
class MediaFilterLIPrivate;

namespace MediaTransforms
{

class MediaFilterLI : public MediaFilter
{

public:
    explicit MediaFilterLI();

    QFuture<search_response> search (const QString &query, QObject *sourceModel) override;
    QFuture<search_response> search (const QString &query, const QList<Types::Track> &song_list);

private:

    std::shared_ptr<MediaFilterLIPrivate> m_internal;
};

}
