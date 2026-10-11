#include "models/AgendaListModel.h"

using namespace Qt::StringLiterals;

namespace JobPrep::Models {

AgendaListModel::AgendaListModel(QObject* parent) : QAbstractListModel(parent) {}

void AgendaListModel::setItems(const QList<Domain::AgendaItem>& items) {
    beginResetModel();
    m_items = items;
    endResetModel();
}

int AgendaListModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) return 0;
    return static_cast<int>(m_items.size());
}

QVariant AgendaListModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= m_items.size()) {
        return QVariant();
    }
    const Domain::AgendaItem& item = m_items.at(index.row());
    switch (role) {
        case Qt::DisplayRole:
            return item.title;
        case KindRole:
            return static_cast<int>(item.kind);
        case IdRole:
            return item.refId;
        case SubtitleRole:
            return item.subtitle;
        case TimeRole:
            return item.time.has_value() ? item.time->toString(u"HH:mm"_s) : QString();
        case DateRole:
            return item.date;
        case Qt::ToolTipRole:
            return item.kind == Domain::AgendaKind::StudyTarget
                       ? QVariant(u"Study targets open in a later milestone"_s)
                       : QVariant();
        case PastRole:
            return item.past;
        default:
            return QVariant();
    }
}

QHash<int, QByteArray> AgendaListModel::roleNames() const {
    QHash<int, QByteArray> names;
    names.insert(KindRole, "kind");
    names.insert(IdRole, "refId");
    names.insert(SubtitleRole, "subtitle");
    names.insert(TimeRole, "time");
    names.insert(DateRole, "date");
    names.insert(PastRole, "past");
    return names;
}

}  // namespace JobPrep::Models