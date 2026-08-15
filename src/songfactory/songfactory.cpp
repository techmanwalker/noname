#include "attributesstruct.hpp"
#include "songfactoryimpl.hpp"

#include "coverprovider.hpp"

#include "songfactory.hpp"

#include <QUrl>

namespace song_factory {

QFuture<Types::Song> 
extract(const QUrl &source, std::shared_ptr<covers::live::cover_provider> provider, attributes a)
{
    return song_factory_impl::extract(source, provider, a);
}

QList<QFuture<Types::Song>>
progressive_extract (const QList<QUrl> &sources, std::shared_ptr<covers::live::cover_provider> provider, attributes a)
{
    return song_factory_impl::progressive_extract(sources, provider, a);
}

QFuture<QList<Types::Song>>
batch_extract (const QList<QUrl> &sources, std::shared_ptr<covers::live::cover_provider> provider, attributes a)
{
    return song_factory_impl::batch_extract(sources, provider, a);
}

void shutdown ()
{
    return song_factory_impl::shutdown();
}

}