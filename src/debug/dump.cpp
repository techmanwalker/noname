#include "dump.hpp"

#include <QIterable>
#include <QMetaProperty>
#include <QMetaSequence>
#include <QVariant>

namespace
{

// Types Qt registers as sequences of characters/bytes but that are logically
// one scalar value, so they must print whole instead of element by element.
bool
is_text_type (const QMetaType &type)
{
    return type == QMetaType::fromType<QString>()
        || type == QMetaType::fromType<QByteArray>();
}

// Prints one named value. Sequences (QList<T>) print as a summary line unless
// `expand` is set, in which case each element is printed too. Gadgets always
// print their properties, so their scalar fields are visible without expanding.
void
dump_variant (
    const QLoggingCategory &cat,
    const QString &name,
    const QVariant &value,
    bool expand,
    int depth
)
{
    const QString pad (depth * 2, u' ');
    const QMetaType type = value.metaType();

    // Strings first: they also satisfy the sequence check below
    if (is_text_type(type)) {
        qCDebug (cat).noquote().nospace()
            << pad << name << " : \"" << value.toString() << '"';
        return;
    }

    // QList<T> and friends: iterate through the metatype, T can be anything
    if (value.canConvert<QMetaSequence::Iterable>()) {
        const QMetaSequence::Iterable seq = value.value<QMetaSequence::Iterable>();

        qCDebug (cat).noquote().nospace()
            << pad << name << " : [List of " << seq.size() << " rows]";

        if (!expand) return;

        qsizetype i = 0;
        for (const QVariant &element : seq) {
            dump_variant(cat, QStringLiteral("[%1]").arg(i++), element, expand, depth + 1);
        }
        return;
    }

    // Q_GADGET: read every property MOC generated for it
    if (type.flags().testFlag(QMetaType::IsGadget)) {
        const QMetaObject *meta = type.metaObject();

        qCDebug (cat).noquote().nospace() << pad << name << " : " << type.name() << " {";

        for (int p = 0; p < meta->propertyCount(); ++p) {
            const QMetaProperty prop = meta->property(p);

            dump_variant(
                cat,
                QString::fromLatin1(prop.name()),
                prop.readOnGadget(value.constData()),
                expand,
                depth + 1
            );
        }

        qCDebug (cat).noquote().nospace() << pad << "}";
        return;
    }

    // Anything else (QString, QUrl, numbers...) prints as is
    qCDebug (cat).noquote().nospace() << pad << name << " : " << value;
}

}

namespace debug
{

void
dump_list_model (
    const QLoggingCategory &cat,
    const QAbstractListModel &list,
    const QString &display_list_name,
    bool dump_nested_lists
)
{
    qCDebug (cat) << "--- Dumping " << display_list_name << "via QAbstractItemModel interface ---";

    // Get the role map (exactly what QML does while booting)
    const QHash<int, QByteArray> roles = list.roleNames();
    const int rowCount = list.rowCount();

    qCDebug (cat) << "Detected rows:" << rowCount;

    // Iterate over every row using model indices
    for (int row = 0; row < rowCount; ++row) {
        QModelIndex idx = list.index(row, 0);
        qCDebug (cat).nospace() << "\n[Row " << row << "]";

        // Iterate over every role available for this row
        for (auto it = roles.cbegin(); it != roles.cend(); ++it) {
            dump_variant(
                cat,
                QString::fromUtf8(it.value()),
                list.data(idx, it.key()),
                dump_nested_lists,
                1
            );
        }
    }

    qCDebug (cat) << "-------------------------------------------------------------";
}

}