#pragma once

#include <QAbstractListModel>
#include <QList>
#include "domain/Structs.h"

namespace JobPrep::Models {

/// Flat list of `AgendaItem`s for the calendar agenda panel. Exposes only neutral data
/// roles; colors and icons live in the UI layer (`AgendaKindStyle`), never here (SPEC
/// §5, Amendment C5).
class AgendaListModel : public QAbstractListModel {
    Q_OBJECT

public:
    enum AgendaRole {
        KindRole = Qt::UserRole + 1,
        IdRole,
        SubtitleRole,
        TimeRole,
        DateRole,
        PastRole,
    };

    explicit AgendaListModel(QObject* parent = nullptr);

    /// Replaces the item list; the panel calls this whenever the day or repository changes.
    void setItems(const QList<Domain::AgendaItem>& items);
    const QList<Domain::AgendaItem>& items() const { return m_items; }

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

private:
    QList<Domain::AgendaItem> m_items;
};

}  // namespace JobPrep::Models