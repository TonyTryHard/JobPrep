#include "models/TopicTableProxy.h"

#include <QDate>
#include "models/TopicListModel.h"

namespace JobPrep::Models {

TopicTableProxy::TopicTableProxy(QObject* parent) : QSortFilterProxyModel(parent) {}

void TopicTableProxy::setTrackFilter(std::optional<int> trackId) {
    if (m_trackId == trackId) return;
    m_trackId = trackId;
    invalidate();
}

void TopicTableProxy::setSearchFilter(const QString& text) {
    const QString trimmed = text.trimmed();
    if (m_searchText == trimmed) return;
    m_searchText = trimmed;
    invalidate();
}

bool TopicTableProxy::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const {
    if (!sourceModel()) return false;

    if (m_trackId.has_value()) {
        const int trackId = sourceModel()
                                ->index(sourceRow, 0, sourceParent)
                                .data(TopicListModel::TrackIdRole)
                                .toInt();
        if (trackId != *m_trackId) return false;
    }

    if (!m_searchText.isEmpty()) {
        const QString title = sourceModel()
                                  ->index(sourceRow, TopicListModel::TitleColumn, sourceParent)
                                  .data(Qt::DisplayRole)
                                  .toString();
        const QString tags = sourceModel()
                                 ->index(sourceRow, 0, sourceParent)
                                 .data(TopicListModel::TagsRole)
                                 .toString();
        if (!title.contains(m_searchText, Qt::CaseInsensitive) &&
            !tags.contains(m_searchText, Qt::CaseInsensitive)) {
            return false;
        }
    }

    return true;
}

bool TopicTableProxy::lessThan(const QModelIndex& left, const QModelIndex& right) const {
    if (left.column() == TopicListModel::TargetDateColumn) {
        const QVariant leftVal = left.data(TopicListModel::TargetDateRole);
        const QVariant rightVal = right.data(TopicListModel::TargetDateRole);
        const bool leftEmpty = !leftVal.isValid() || leftVal.isNull() || !leftVal.toDate().isValid();
        const bool rightEmpty = !rightVal.isValid() || rightVal.isNull() || !rightVal.toDate().isValid();

        if (leftEmpty && rightEmpty) return false;
        if (leftEmpty) {
            return sortOrder() == Qt::DescendingOrder;
        }
        if (rightEmpty) {
            return sortOrder() == Qt::AscendingOrder;
        }
        return leftVal.toDate() < rightVal.toDate();
    }

    if (left.column() == TopicListModel::PriorityColumn) {
        const int leftRank = left.data(TopicListModel::SortRole).toInt();
        const int rightRank = right.data(TopicListModel::SortRole).toInt();
        return leftRank < rightRank;
    }

    return QSortFilterProxyModel::lessThan(left, right);
}

}  // namespace JobPrep::Models
