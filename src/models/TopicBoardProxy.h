#pragma once

#include <optional>
#include <QSortFilterProxyModel>
#include "domain/Enums.h"

namespace JobPrep::Data {
class TopicRepository;
}

namespace JobPrep::Models {

/// Per-column proxy model filtering by TopicStatus and optional track filter,
/// handling board Drag & Drop with MIME "application/x-jobprep-topic-id" (SPEC §8).
class TopicBoardProxy : public QSortFilterProxyModel {
    Q_OBJECT

public:
    static constexpr char kTopicMimeType[] = "application/x-jobprep-topic-id";

    explicit TopicBoardProxy(Domain::TopicStatus status,
                             Data::TopicRepository& topics,
                             QObject* parent = nullptr);
    ~TopicBoardProxy() override = default;

    Domain::TopicStatus columnStatus() const { return m_status; }

    std::optional<int> trackFilter() const { return m_trackId; }
    void setTrackFilter(std::optional<int> trackId);

    Qt::ItemFlags flags(const QModelIndex& index) const override;
    QStringList mimeTypes() const override;
    QMimeData* mimeData(const QModelIndexList& indexes) const override;
    Qt::DropActions supportedDropActions() const override;
    bool canDropMimeData(const QMimeData* data,
                         Qt::DropAction action,
                         int row,
                         int column,
                         const QModelIndex& parent) const override;
    bool dropMimeData(const QMimeData* data,
                      Qt::DropAction action,
                      int row,
                      int column,
                      const QModelIndex& parent) override;

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;

private:
    Domain::TopicStatus m_status;
    Data::TopicRepository& m_topics;
    std::optional<int> m_trackId;
};

}  // namespace JobPrep::Models
