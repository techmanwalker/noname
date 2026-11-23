#include "songfactoryimpl.hpp"

#include "songfactory.hpp"

#include <QUrl>

namespace song_factory {

QFuture<Types::Track>
extract(const QUrl &source)
{
    return song_factory_impl::extract(source);
}

QList<QFuture<Types::Track>>
progressive_extract (const QList<QUrl> &sources)
{
    return song_factory_impl::progressive_extract(sources);
}

QFuture<QList<Types::Track>>
batch_extract (const QList<QUrl> &sources)
{
    return song_factory_impl::batch_extract(sources);
}

void
teardown ()
{
    return song_factory_impl::teardown();
}

}
