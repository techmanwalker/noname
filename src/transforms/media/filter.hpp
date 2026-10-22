#pragma once

#include "mediafilter-in.hpp"

// forward declaration
class MediaFilterLIPrivate;

namespace MediaTransforms
{

class MediaFilterLI : public MediaFilter
{

public:
    explicit MediaFilterLI();

    search_response search (const QString &query, QObject *sourceModel) override;
    search_response search (const QString &query, QList<Types::Song> &song_list);

private:

    std::shared_ptr<MediaFilterLIPrivate> m_internal;
};

}