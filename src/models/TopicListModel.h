#pragma once

#include <functional>
#include <optional>
#include <QAbstractTableModel>
#include <QDate>
#include <QList>
#include "domain/Enums.h"
#include "domain/Structs.h"

namespace JobPrep::Data {
class TopicRepository;
class TrackRepository;
}  // namespace JobPrep::Data

namespace JobPrep::Models {

/// Flat table model representing all topics (SPEC §3.2, S6).
/// Exposes only neutral types (hex strings, ints, dates); custom visuals
/// are handled by delegates in ui/.
class TopicListModel : public QAbstractTableModel {
    Q_OBJECT

public:
    using TodayClock = std::function<QDate()>;

    enum Column {
        TitleColumn = 0,
        TrackColumn,
        StatusColumn,
        PriorityColumn,
        TargetDateColumn,
        ProgressColumn,
        TagsColumn,
        ColumnCount
    };

    enum Role {
        IdRole = Qt::UserRole + 1,
        StatusRole,
        PriorityRole,
        ProgressRole,
        TrackIdRole,
        TrackColorRole,
        TrackNameRole,
        TargetDateRole,
        DoneSubtasksRole,
        TotalSubtasksRole,
        OverdueRole,
        NotesRole,
        ResourcesRole,
        TagsRole,
        SortRole,
    };

    explicit TopicListModel(Data::TopicRepository& topics,
                            Data::TrackRepository& tracks,
                            QObject* parent = nullptr,
                            TodayClock clock = {});
    ~TopicListModel() override = default;

    void setTodayClock(TodayClock clock);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    std::optional<Domain::Topic> topicAt(int row) const;
    std::optional<Domain::Topic> topicById(int id) const;
    int rowForTopicId(int id) const;

public slots:
    void reload();

private:
    struct RowCache {
        Domain::Topic topic;
        QString trackName;
        QString trackColor;
        int doneSubtasks{0};
        int totalSubtasks{0};
        int progress{0};
        bool overdue{false};
    };

    void buildCache(QList<RowCache>& newCache) const;

    Data::TopicRepository& m_topics;
    Data::TrackRepository& m_tracks;
    TodayClock m_clock;
    QList<RowCache> m_cache;
};

}  // namespace JobPrep::Models
