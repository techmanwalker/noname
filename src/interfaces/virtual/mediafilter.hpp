#pragma once

#include <QFuture>
#include <QList>
#include <QtQmlIntegration>

#include <stop_token>

#include "mediatypes.hpp"

struct search_response {
    Q_GADGET
    QML_VALUE_TYPE(searchResponse)          // value type names must start lowercase
    Q_PROPERTY(QList<Types::Track> items MEMBER items)
    Q_PROPERTY(double highScore MEMBER high_score)

public:                                      // Q_GADGET leaves you in private
    QList<Types::Track> items;
    double high_score {}; // softer threshold
};

// The future is never cancelled, so it always completes with exactly one result
// (empty if stopped). Cancel through `stop` only.
struct search_job {
    QFuture<search_response> future;
    std::stop_source stop;
};

class MediaFilter
{

public:
    virtual ~MediaFilter () = default;

    // Must be called from the thread that owns sourceModel
    virtual search_job search (const QString &query, QObject *sourceModel) = 0;
};
