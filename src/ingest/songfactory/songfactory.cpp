#include "attributesstruct.hpp"
#include "songfactoryimpl.hpp"

#include "songfactory.hpp"

#include <QUrl>

namespace song_factory {

QFuture<Types::Track>
extract(const QUrl &source, attributes a)
{
    return song_factory_impl::extract(source, a);
}

QList<QFuture<Types::Track>>
progressive_extract (const QList<QUrl> &sources, attributes a)
{
    return song_factory_impl::progressive_extract(sources, a);
}

QFuture<QList<Types::Track>>
batch_extract (const QList<QUrl> &sources, attributes a)
{
    return song_factory_impl::batch_extract(sources, a);
}

void
teardown ()
{
    return song_factory_impl::teardown();
}

}
