#include "searchresults.hpp"
#include "abstractmediasequence.hpp"
#include "prettifiers.hpp" // IWYU pragma: keep this provides prettifiers.tpp

#include "locallibrary.hpp" // IWYU pragma: keep for qobject_cast<LocalLibrary*>

#include <QIdentityProxyModel>
#include <QList>

#include <memory>

#include <qlist.h>
#include <qobject.h>
#include <rapidfuzz/fuzz.hpp>
#include <unicode/translit.h>
#include <unicode/unistr.h>
#include <unicode/utypes.h>

// bridge icu and icu_<version> safely
using namespace U_ICU_NAMESPACE; 

class SearchResultsLIPrivate
{
public:
    std::shared_ptr<Transliterator> translit;

    SearchResultsLIPrivate (decltype(translit) transliterator)
        : translit(transliterator) {}
};

namespace {

std::shared_ptr<Transliterator>
make_transliterator()
{
    UErrorCode status = U_ZERO_ERROR;
    std::shared_ptr<Transliterator> translit(Transliterator::createInstance(
        "Any-Lower; Any-Latin; NFKD; [:Nonspacing Mark:] Remove", UTRANS_FORWARD, status));

    if (U_FAILURE(status) || !translit) {
        qCWarning(l_mediasequences) << "ICU Transliterator failed to initialize:" << u_errorName(status);
        return nullptr;
    }
    return translit;
}

std::string
nfkd_and_translit(const std::string& input, std::shared_ptr<Transliterator> transliterator) {
    if (!transliterator) {
        qCDebug(l_mediasequences) << "The transliterator pointer arrived nullptr. Returning original query.";
        return input;
    }
    UnicodeString uText = UnicodeString::fromUTF8(input);
    transliterator->transliterate(uText);
    
    std::string result;
    uText.toUTF8String(result);
    return result;
}

} // namespace

SearchResultsLI::SearchResultsLI()
    : m_internal ( std::make_shared<SearchResultsLIPrivate>(make_transliterator()) )
{
}


// The Search proxy

QList<Types::Song>
SearchResultsLI::performSearch(const QString &query, QObject *sourceModel)
{
    QList<Types::Song> all_songs;

    // Interface-backed QML singletons (LocalLibrary, and any future ones) are now
    // exposed through a QIdentityProxyModel, not the real object — unwrap first.
    if (auto *proxy = qobject_cast<QIdentityProxyModel*>(sourceModel)) {
        sourceModel = proxy->sourceModel();
    }

    // Check if the passed object is our LocalLibrary singleton
    if (auto *localLib = qobject_cast<LocalLibrary*>(sourceModel)) {
        
        all_songs = localLib->flattened(); // faster and dedicated
    } else if (auto *sourceSequence = qobject_cast<AbstractMediaSequence*>(sourceModel)) {
        
        // Fallback for generic sequences if needed
        for (const Types::Any &item : sourceSequence->items()) {
            if (const auto *song = std::get_if<Types::Song>(&item)) {
                all_songs.append(*song);
            } else if (const auto *dir = std::get_if<Types::Directory>(&item)) {
                all_songs.append(dir->songs);
            }
        }
    } else {
        qCWarning(l_mediasequences) << "Invalid source model provided for search.";
        return {};
    }

    // Execute RapidFuzz search logic on 'all_songs' and return the list (...)
    return performSearch(query, all_songs); 
}

QList<Types::Song>
SearchResultsLI::performSearch(const QString &query, QList<Types::Song> &song_list)
{
    if (query.isEmpty()) {
        return Prettifiers::sortBy(&Types::Song::title, song_list);
    }

    using rankable_item = std::pair<size_t, double>;
    std::vector<rankable_item> rankable;

    // Clean query once using the local transliterator instance
    const std::string clean_keywords = nfkd_and_translit(query.toStdString(), m_internal->translit);
    rapidfuzz::fuzz::CachedPartialRatio<char> scorer(clean_keywords.c_str());
    const double score_thresh = 50.0;

    // Execute search directly against the flattened song list
    for (qsizetype i = 0; i < song_list.size(); ++i) {
        // Pass transPtr to process text instantly without initialization overhead
        const std::string clean_title = nfkd_and_translit(song_list.at(i).title.toStdString(), m_internal->translit);
        double score = scorer.similarity(clean_title.c_str(), score_thresh);

        if (score >= score_thresh) {
            rankable.emplace_back(i, score);
        }
    }

    // Rank by scores in descending order using C++20 projections
    std::ranges::sort(rankable, std::greater<>{}, &rankable_item::second);

    // Map the ranked results back to the original song objects
    QList<Types::Song> matching_songs;
    matching_songs.reserve(rankable.size());

    for (const rankable_item &ranked : rankable) {
        matching_songs.append(song_list.at(ranked.first));
    }

    return matching_songs;
}