#include "abstractmediasequence.hpp"
#include "filter.hpp"
#include "sort.hpp" // IWYU pragma: keep this provides prettifiers.tpp

#include "virtual/mediafilter.hpp"
#include "virtual/locallibrary.hpp" // IWYU pragma: keep for qobject_cast<LocalLibrary*>

#include "l_mediatransforms.hpp"

#include <QIdentityProxyModel>
#include <QList>
#include <QMutex>
#include <QPromise>
#include <QSemaphore>
#include <QThreadPool>
#include <QtConcurrent/QtConcurrentRun>

#include <memory>
#include <vector>

#include <rapidfuzz/fuzz.hpp>
#include <unicode/translit.h>
#include <unicode/unistr.h>
#include <unicode/utypes.h>

// bridge icu and icu_<version> safely
using namespace U_ICU_NAMESPACE;

namespace {

std::unique_ptr<Transliterator>
make_transliterator()
{
    UErrorCode status = U_ZERO_ERROR;
    std::unique_ptr<Transliterator> translit(Transliterator::createInstance(
        "Any-Lower; Any-Latin; NFKD; [:Nonspacing Mark:] Remove", UTRANS_FORWARD, status));

    if (U_FAILURE(status) || !translit) {
        qCWarning(l_mediatransforms) << "ICU Transliterator failed to initialize:" << u_errorName(status);
        return nullptr;
    }
    return translit;
}

std::string
nfkd_and_translit(const std::string& input, Transliterator *transliterator) {
    if (!transliterator) {
        qCDebug(l_mediatransforms) << "The transliterator pointer arrived nullptr. Returning original query.";
        return input;
    }
    UnicodeString uText = UnicodeString::fromUTF8(input);
    transliterator->transliterate(uText);

    std::string result;
    uText.toUTF8String(result);
    return result;
}

// ICU transliterators are not thread-safe: a Lease is the only way to touch one,
// and it is exclusive until destroyed.
class TransliteratorPool
{
public:
    class Lease
    {
    public:
        Lease(TransliteratorPool &pool, std::unique_ptr<Transliterator> translit)
            : m_pool(pool), m_translit(std::move(translit)) {}
        ~Lease() { m_pool.release(std::move(m_translit)); }

        Lease(const Lease &) = delete;
        Lease &operator=(const Lease &) = delete;

        Transliterator *get() const { return m_translit.get(); }

    private:
        TransliteratorPool &m_pool;
        std::unique_ptr<Transliterator> m_translit;
    };

    explicit TransliteratorPool(int count) : m_available(count)
    {
        for (int i = 0; i < count; ++i)
            m_free.push_back(make_transliterator());
    }

    // Blocks while every instance is leased
    Lease acquire()
    {
        m_available.acquire();
        QMutexLocker lock(&m_mutex);
        auto translit = std::move(m_free.back());
        m_free.pop_back();
        return Lease(*this, std::move(translit));
    }

private:
    void release(std::unique_ptr<Transliterator> translit)
    {
        {
            QMutexLocker lock(&m_mutex);
            m_free.push_back(std::move(translit));
        }
        m_available.release();
    }

    QSemaphore m_available;
    QMutex m_mutex;
    std::vector<std::unique_ptr<Transliterator>> m_free;
};

} // namespace

class MediaFilterLIPrivate
{
public:
    static constexpr int translit_concurrency = 2;

    MediaFilterLIPrivate() : translits(translit_concurrency)
    {
        translit_pool.setMaxThreadCount(translit_concurrency);
    }

    // Tasks borrow `translits`, and stage 1 schedules stage 2: wait in pipeline order
    ~MediaFilterLIPrivate()
    {
        translit_pool.waitForDone();
        score_pool.waitForDone();
    }

    TransliteratorPool translits;
    QThreadPool translit_pool; // the queue: FIFO, one transliterator per worker
    QThreadPool score_pool;    // scoring: not bounded by the transliterators
};


namespace MediaTransforms
{

// The Search proxy

MediaFilterLI::MediaFilterLI()
    : m_internal ( std::make_shared<MediaFilterLIPrivate>() )
{
}

search_job
MediaFilterLI::search(const QString &query, QObject *sourceModel)
{
    // Everything touching sourceModel stays on the calling thread;
    // only the track snapshot goes to a worker.
    QList<Types::Track> all_tracks;

    // Interface-backed QML singletons (LocalLibrary, and any future ones) are now
    // exposed through a QIdentityProxyModel, not the real object — unwrap first.
    if (auto *proxy = qobject_cast<QIdentityProxyModel*>(sourceModel)) {
        sourceModel = proxy->sourceModel();
    }

    // Check if the passed object is our LocalLibrary singleton
    if (auto *localLib = qobject_cast<LocalLibrary*>(sourceModel)) {

        all_tracks = localLib->flattened(); // faster and dedicated
    } else if (auto *sourceSequence = qobject_cast<AbstractMediaSequence*>(sourceModel)) {
        // Filter its tracks and traverse 1 level through its directories
        // the player does not support more than one level at the moment

        auto [tracks, directories] = sourceSequence->items_of_types<Types::Track, Types::Directory>();

        all_tracks = std::move(tracks);

        for (const Types::Directory &dir : directories) {
            all_tracks += dir.tracks;
        }
    } else {
        qCWarning(l_mediatransforms) << "Invalid source model provided for search.";
        return {QtFuture::makeReadyValueFuture(search_response{}), {}};
    }

    // Execute RapidFuzz search logic on 'all_tracks' and return the list (...)
    return search (query, all_tracks);
}

namespace {

struct Transliterated {
    std::string query;
    std::vector<std::string> titles;
};

// Stage 1 (queued): the only code that touches a transliterator
void
transliterate (TransliteratorPool &translits, QPromise<Transliterated> &promise,
               std::stop_token stop, const QString &query, const QList<Types::Track> &tracks)
{
    Transliterated out; // stays empty if stopped

    if (!stop.stop_requested()) {
        auto lease = translits.acquire();
        Transliterator *translit = lease.get();

        out.query = nfkd_and_translit(query.toStdString(), translit);
        out.titles.reserve(static_cast<size_t>(tracks.size()));

        for (const Types::Track &track : tracks) {
            if (stop.stop_requested()) { out = {}; break; }
            out.titles.push_back(nfkd_and_translit(track.title.toStdString(), translit));
        }
    } // lease released here, before any scoring starts

    // Always exactly one result: stage 2 relies on it
    promise.addResult(std::move(out));
}

search_response
rank_tracks (const Transliterated &clean, const QList<Types::Track> &song_list, std::stop_token stop)
{
    // Stop is monotonic: empty titles imply stop_requested(), so this also guards the indexing below
    if (stop.stop_requested()) return {};

    using rankable_item = std::pair<qsizetype, double>;
    std::vector<rankable_item> rankable;

    rapidfuzz::fuzz::CachedPartialRatio<char> scorer(clean.query.c_str());
    const double score_thresh = 50.0;

    double high_score {-1};

    // Execute search directly against the flattened song list
    for (qsizetype i = 0; i < song_list.size(); ++i) {
        if (stop.stop_requested()) return {};

        double score = scorer.similarity(clean.titles[static_cast<size_t>(i)].c_str(), score_thresh);

        if (score >= score_thresh) {
            if (score > high_score) high_score = score;
            rankable.emplace_back(i, score);
        }
    }

    // Rank by scores in descending order using C++20 projections
    std::ranges::sort(rankable, std::greater<>{}, &rankable_item::second);

    // Map the ranked results back to the original song objects
    QList<Types::Track> matching_tracks;
    matching_tracks.reserve(static_cast<qsizetype>(rankable.size()));

    for (const rankable_item &ranked : rankable) {
        matching_tracks.append(song_list.at(ranked.first));
    }

    return {matching_tracks, high_score};
}

} // namespace

search_job
MediaFilterLI::search (const QString &query, const QList<Types::Track> &song_list)
{
    // Tasks own their (implicitly shared) copies; `state` outlives them via ~MediaFilterLIPrivate
    auto *state = m_internal.get();
    std::stop_source source;

    // Nothing to transliterate: skip the queue
    if (query.isEmpty()) {
        return {QtConcurrent::run(&state->score_pool, [song_list] {
                    return search_response{sort_by (&Types::Track::title, song_list), -1};
                }),
                std::move(source)};
    }

    auto future = QtConcurrent::run(&state->translit_pool,
                      [state, query, song_list, stop = source.get_token()](QPromise<Transliterated> &promise) {
                          transliterate(state->translits, promise, stop, query, song_list);
                      })
        .then(&state->score_pool, [song_list, stop = source.get_token()](const Transliterated &clean) {
            return rank_tracks(clean, song_list, stop);
        });

    return {std::move(future), std::move(source)};
}

}
