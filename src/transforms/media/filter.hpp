#pragma once

#include "mediafilter-in.hpp"

// forward declaration
class MediaFilterLIPrivate;

class MediaFilterLI : public MediaFilter
{

public:
    explicit MediaFilterLI();

    QList<Types::Song> performSearch (const QString &query, QObject *sourceModel) override;
    QList<Types::Song> performSearch (const QString &query, QList<Types::Song> &song_list);

private:

    std::shared_ptr<MediaFilterLIPrivate> m_internal;
};