#include "searchresults.hpp"
#include "prettifiers.hpp" // IWYU pragma: keep this provides prettifiers.tpp

#include "locallibrary.hpp" // IWYU pragma: keep for qobject_cast<LocalLibrary*>

#include <QIdentityProxyModel>
#include <QList>

#include <QJSEngine>
#include <QQmlEngine>

#include <qobject.h>

#include <rapidfuzz/fuzz.hpp>
#include <unicode/translit.h>
#include <unicode/unistr.h>
#include <unicode/utypes.h>

// bridge icu and icu_<version> safely
using namespace U_ICU_NAMESPACE; 


SearchResultsLI::SearchResultsLI(QObject *parent)
    : PlaylistSequence(parent)
{
}

std::string
SearchResultsLI::nfkd_and_translit(const std::string& input, Transliterator* transliterator) {
    if (!transliterator) {
        return input;
    }
    UnicodeString uText = UnicodeString::fromUTF8(input);
    transliterator->transliterate(uText);
    
    std::string result;
    uText.toUTF8String(result);
    return result;
}


// The Search proxy

void
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
        return;
    }

    // Execute RapidFuzz search logic on 'all_songs' and call respawn_list(...)
    performSearch(query, all_songs); 
}

void
SearchResultsLI::performSearch(const QString &query, QList<Types::Song> &song_list)
{
    if (query.isEmpty()) {
        respawn_list(Prettifiers::sortBy(&Types::Song::title, song_list));
        return;
    }

    // Initialize ICU Transliterator ONCE per search execution
    UErrorCode status = U_ZERO_ERROR;
    std::unique_ptr<Transliterator> transliterator(
        Transliterator::createInstance("Any-Lower; Any-Latin; NFKD; [:Nonspacing Mark:] Remove", UTRANS_FORWARD, status)
    );

    // Fallback indicator if ICU initialization fails
    Transliterator* transPtr = U_SUCCESS(status) ? transliterator.get() : nullptr;
    if (!transPtr) {
        qCWarning(l_mediasequences) << "ICU Transliterator failed to initialize:" << u_errorName(status);
    }

    using rankable_item = std::pair<size_t, double>;
    std::vector<rankable_item> rankable;

    // Clean query once using the local transliterator instance
    const std::string clean_keywords = nfkd_and_translit(query.toStdString(), transPtr);
    rapidfuzz::fuzz::CachedPartialRatio<char> scorer(clean_keywords.c_str());
    const double score_thresh = 50.0;

    // Execute search directly against the flattened song list
    for (size_t i = 0; i < song_list.size(); ++i) {
        // Pass transPtr to process text instantly without initialization overhead
        const std::string clean_title = nfkd_and_translit(song_list.at(i).title.toStdString(), transPtr);
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

    respawn_list(matching_songs);
}