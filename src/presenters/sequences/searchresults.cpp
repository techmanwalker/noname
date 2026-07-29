#include "searchresults.hpp"
#include "locallibrary.hpp"
#include <qhashfunctions.h>
#include <qlist.h>

// Meyers singleton implementation
SearchResults &
SearchResults::instance()
{
    static SearchResults s_instance;
    return s_instance;
}

SearchResults::SearchResults(QObject *parent)
    : PlaylistSequence(parent)
{
}

// factory for the qml engine
SearchResults *
SearchResults::create(QQmlEngine *qmlEngine, QJSEngine *jsEngine)
{
    Q_UNUSED(qmlEngine);
    Q_UNUSED(jsEngine);
    
    SearchResults *inst = &instance();

    // avoid QML GC to try to free object memory
    QJSEngine::setObjectOwnership(inst, QJSEngine::CppOwnership);

    return inst;
}


// The Search proxy

void
SearchResults::performSearch(const QString &query, QObject *sourceModel)
{
    QList<Types::Song> all_songs;

    // Check if the passed object is our LocalLibrary singleton
    if (auto *localLib = qobject_cast<LocalLibrary*>(sourceModel)) {
        
        all_songs = localLib->flattened(); // faster and dedicated
    } 
    else if (auto *sourceSequence = qobject_cast<AbstractMediaSequence*>(sourceModel)) {
        
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
SearchResults::performSearch (const QString &query, QList<Types::Song> &song_list)
{
    if (query.isEmpty()) {
        // Return all available songs instead

        respawn_list(song_list);
        return;
    }

    // 2. Prepare the RapidFuzz algorithm
    using rankable_item = std::pair<size_t, double>;
    std::vector<rankable_item> rankable;

    // Normalize query safely using the sequence's static helper
    const std::string clean_keywords = AbstractMediaSequence::normalize_string_for_search(query);
    rapidfuzz::fuzz::CachedPartialRatio<char> scorer(clean_keywords.c_str());
    const double score_thresh = 50.0;

    // 3. Execute search directly against the flattened song list
    for (size_t i = 0; i < song_list.size(); ++i) {
        const std::string clean_title = AbstractMediaSequence::normalize_string_for_search(song_list.at(i).title);
        double score = scorer.similarity(clean_title.c_str(), score_thresh);

        if (score >= score_thresh) {
            rankable.emplace_back(i, score);
        }
    }

    // 4. Rank by scores in descending order using C++20 projections
    std::ranges::sort(rankable, std::greater<>{}, &rankable_item::second);

    // 5. Map the ranked results back to the original song objects
    QList<Types::Song> matching_songs;
    matching_songs.reserve(rankable.size());

    for (const rankable_item &ranked : rankable) {
        matching_songs.append(song_list.at(ranked.first));
    }

    // 6. Clear and repopulate the singleton in one step
    respawn_list(matching_songs);
}