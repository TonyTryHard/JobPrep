#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

#include "models/TopicBoardProxy.h"

#include <algorithm>
#include <QMimeData>
#include <QStringList>
#include "data/TopicRepository.h"
#include "models/TopicListModel.h"

using namespace Qt::StringLiterals;

namespace JobPrep::Models {

TopicBoardProxy::TopicBoardProxy(Domain::TopicStatus status,
                                 Data::TopicRepository& topics,
                                 QObject* parent)
    : QSortFilterProxyModel(parent), m_status(status), m_topics(topics) {}

void TopicBoardProxy::setTrackFilter(std::optional<int> trackId) {
    if (m_trackId == trackId) return;
    m_trackId = trackId;
    invalidateFilter();
}

bool TopicBoardProxy::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const {
    if (!sourceModel()) return false;
    const auto idx = sourceModel()->index(sourceRow, 0, sourceParent);
    const auto status = idx.data(TopicListModel::StatusRole).value<Domain::TopicStatus>();
    if (status != m_status) return false;

    if (m_trackId.has_value()) {
        const int trackId = idx.data(TopicListModel::TrackIdRole).toInt();
        if (trackId != *m_trackId) return false;
    }

    return true;
}

Qt::ItemFlags TopicBoardProxy::flags(const QModelIndex& index) const {
    const Qt::ItemFlags defaultFlags = QSortFilterProxyModel::flags(index);
    if (index.isValid()) {
        return defaultFlags | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;
    }
    return defaultFlags | Qt::ItemIsDropEnabled;
}

QStringList TopicBoardProxy::mimeTypes() const {
    return { QString::fromLatin1(kTopicMimeType) };
}

QMimeData* TopicBoardProxy::mimeData(const QModelIndexList& indexes) const {
    if (indexes.isEmpty()) return nullptr;

    int topicId = 0;
    for (const auto& idx : indexes) {
        if (idx.isValid()) {
            topicId = idx.data(TopicListModel::IdRole).toInt();
            if (topicId > 0) break;
        }
    }

    if (topicId <= 0) return nullptr;

    auto* mime = new QMimeData();
    mime->setData(QString::fromLatin1(kTopicMimeType), QByteArray::number(topicId));
    return mime;
}

Qt::DropActions TopicBoardProxy::supportedDropActions() const {
    return Qt::MoveAction;
}

bool TopicBoardProxy::canDropMimeData(const QMimeData* data,
                                     Qt::DropAction action,
                                     int row,
                                     int column,
                                     const QModelIndex& parent) const {
    Q_UNUSED(action);
    Q_UNUSED(row);
    Q_UNUSED(column);
    Q_UNUSED(parent);

    if (!data || !data->hasFormat(QString::fromLatin1(kTopicMimeType))) {
        return false;
    }

    bool ok = false;
    const int topicId = data->data(QString::fromLatin1(kTopicMimeType)).toInt(&ok);
    if (!ok || topicId <= 0) return false;

    return m_topics.byId(topicId).has_value();
}

bool TopicBoardProxy::dropMimeData(const QMimeData* data,
                                   Qt::DropAction action,
                                   int row,
                                   int column,
                                   const QModelIndex& parent) {
    Q_UNUSED(action);
    Q_UNUSED(column);

    if (!canDropMimeData(data, action, row, column, parent)) {
        return false;
    }

    bool ok = false;
    const int topicId = data->data(QString::fromLatin1(kTopicMimeType)).toInt(&ok);
    if (!ok || topicId <= 0) return false;

    QList<int> visibleIds;
    visibleIds.reserve(rowCount());
    for (int r = 0; r < rowCount(); ++r) {
        visibleIds.append(this->data(index(r, 0), TopicListModel::IdRole).toInt());
    }

    int destRow = -1;
    if (parent.isValid()) {
        destRow = parent.row();
    } else if (row >= 0) {
        destRow = row;
    } else {
        destRow = visibleIds.size();
    }
    destRow = std::clamp(destRow, 0, static_cast<int>(visibleIds.size()));

    if (visibleIds.contains(topicId)) {
        const int srcRow = visibleIds.indexOf(topicId);
        if (destRow > srcRow) {
            destRow--;
        }
        if (srcRow == destRow) {
            return true;
        }
        visibleIds.removeAt(srcRow);
        visibleIds.insert(destRow, topicId);
    } else {
        visibleIds.insert(destRow, topicId);
    }

    return m_topics.moveTopic(topicId, m_status, visibleIds);
}

}  // namespace JobPrep::Models
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif

