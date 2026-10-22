#pragma once

#include <QList>
#include <QtQmlIntegration>

#include "mediatypes.hpp"

struct search_response {
    Q_GADGET
    QML_VALUE_TYPE(searchResponse)          // value type names must start lowercase
    Q_PROPERTY(QList<Types::Song> items MEMBER items)
    Q_PROPERTY(double highScore MEMBER high_score)

public:                                      // Q_GADGET leaves you in private
    QList<Types::Song> items;
    double high_score {}; // softer threshold
};

class MediaFilter
{

public: 
    virtual ~MediaFilter () = default;

    virtual search_response search (const QString &query, QObject *sourceModel) = 0;
};