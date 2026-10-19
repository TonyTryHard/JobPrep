#include "models/TopicListModel.h"

#include <algorithm>
#include <QHash>
#include "data/DbFormat.h"
#include "data/TopicRepository.h"
#include "data/TrackRepository.h"
#include "domain/EnumStrings.h"
#include "domain/TopicProgress.h"

using namespace Qt::StringLiterals;

namespace JobPrep::Models {

namespace {

int priorityRank(Domain::Priority priority) {
    switch (priority) {
        case Domain::Priority::Low:
            return 0;
        case Domain::Priority::Normal:
            return 1;
        case Domain::Priority::High:
            return 2;
    }
    return 1;
}

}  // namespace

TopicListModel::TopicListModel(Data::TopicRepository& topics,
                               Data::TrackRepository& tracks,
                               QObject* parent,
                               TodayClock clock)
    : QAbstractTableModel(parent),
      m_topics(topics),
      m_tracks(tracks),
      m_clock(clock ? std::move(clock) : TodayClock([] { return QDate::currentDate(); })) {
    connect(&m_topics, &Data::TopicRepository::changed, this, &TopicListModel::reload);
    connect(&m_tracks, &Data::TrackRepository::changed, this, &TopicListModel::reload);
    reload();
}

void TopicListModel::setTodayClock(TodayClock clock) {
    m_clock = clock ? std::move(clock) : TodayClock([] { return QDate::currentDate(); });
    reload();
}

int TopicListModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) return 0;
    return static_cast<int>(m_cache.size());
}

int TopicListModel::columnCount(const QModelIndex& parent) const {
    if (parent.isValid()) return 0;
    return ColumnCount;
}

void TopicListModel::buildCache(QList<RowCache>& newCache) const {
    const auto topics = m_topics.all();
    const auto tracks = m_tracks.all();
    const auto subtaskProgressMap = m_topics.subtaskProgress();
    const QDate today = m_clock ? m_clock() : QDate::currentDate();

    QHash<int, Domain::Track> tracksById;
    for (const auto& tr : tracks) {
        tracksById.insert(tr.id, tr);
    }

    newCache.clear();
    newCache.reserve(topics.size());
    for (const auto& topic : topics) {
        RowCache rc;
        rc.topic = topic;
        const auto trackIt = tracksById.constFind(topic.trackId);
        if (trackIt != tracksById.constEnd()) {
            rc.trackName = trackIt->name;
            rc.trackColor = trackIt->color;
        }
        const auto prog = subtaskProgressMap.value(topic.id);
        rc.doneSubtasks = prog.done;
        rc.totalSubtasks = prog.total;
        rc.progress = Domain::topicProgress(topic.status, prog.done, prog.total);
        rc.overdue = topic.targetDate.has_value() &&
                     (*topic.targetDate < today) &&
                     (topic.status != Domain::TopicStatus::Done);
        newCache.append(rc);
    }
}

void TopicListModel::reload() {
    QList<RowCache> newCache;
    buildCache(newCache);

    bool sameIds = (newCache.size() == m_cache.size());
    if (sameIds) {
        for (int i = 0; i < newCache.size(); ++i) {
            if (newCache[i].topic.id != m_cache[i].topic.id) {
                sameIds = false;
                break;
            }
        }
    }

    if (sameIds) {
        m_cache = std::move(newCache);
        if (!m_cache.isEmpty()) {
            emit dataChanged(index(0, 0), index(static_cast<int>(m_cache.size()) - 1, ColumnCount - 1));
        }
    } else {
        beginResetModel();
        m_cache = std::move(newCache);
        endResetModel();
    }
}

QVariant TopicListModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= m_cache.size()) {
        return {};
    }

    const auto& rc = m_cache.at(index.row());
    const auto& topic = rc.topic;

    if (role == Qt::DisplayRole) {
        switch (index.column()) {
            case TitleColumn:
                return topic.title;
            case TrackColumn:
                return rc.trackName;
            case StatusColumn:
                return {};  // Status badge delegate paints this from StatusRole
            case PriorityColumn:
                return Domain::EnumStrings::toString(topic.priority);
            case TargetDateColumn:
                return topic.targetDate.has_value()
                           ? Data::DbFormat::toText(*topic.targetDate)
                           : QString();
            case ProgressColumn:
                return QString::number(rc.progress) + u"%"_s;
            case TagsColumn:
                return topic.tags;
            default:
                return {};
        }
    } else if (role == IdRole) {
        return topic.id;
    } else if (role == StatusRole) {
        return QVariant::fromValue(topic.status);
    } else if (role == PriorityRole) {
        return QVariant::fromValue(topic.priority);
    } else if (role == ProgressRole) {
        return rc.progress;
    } else if (role == TrackIdRole) {
        return topic.trackId;
    } else if (role == TrackColorRole) {
        return rc.trackColor;
    } else if (role == TrackNameRole) {
        return rc.trackName;
    } else if (role == TargetDateRole) {
        return topic.targetDate.has_value() ? QVariant(*topic.targetDate) : QVariant();
    } else if (role == DoneSubtasksRole) {
        return rc.doneSubtasks;
    } else if (role == TotalSubtasksRole) {
        return rc.totalSubtasks;
    } else if (role == OverdueRole) {
        return rc.overdue;
    } else if (role == NotesRole) {
        return topic.notes;
    } else if (role == ResourcesRole) {
        return topic.resources;
    } else if (role == TagsRole) {
        return topic.tags;

    } else if (role == SortRole) {
        switch (index.column()) {
            case TitleColumn:
                return topic.title.toLower();
            case TrackColumn:
                return rc.trackName.toLower();
            case StatusColumn:
                return static_cast<int>(topic.status);
            case PriorityColumn:
                return priorityRank(topic.priority);
            case TargetDateColumn:
                return topic.targetDate.has_value() ? *topic.targetDate : QDate(9999, 12, 31);
            case ProgressColumn:
                return rc.progress;
            case TagsColumn:
                return topic.tags.toLower();
            default:
                return {};
        }
    }

    return {};
}

QVariant TopicListModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole) {
        return QAbstractTableModel::headerData(section, orientation, role);
    }
    switch (section) {
        case TitleColumn:
            return tr("Title");
        case TrackColumn:
            return tr("Track");
        case StatusColumn:
            return tr("Status");
        case PriorityColumn:
            return tr("Priority");
        case TargetDateColumn:
            return tr("Target Date");
        case ProgressColumn:
            return tr("Progress");
        case TagsColumn:
            return tr("Tags");
        default:
            return {};
    }
}

std::optional<Domain::Topic> TopicListModel::topicAt(int row) const {
    if (row < 0 || row >= m_cache.size()) return std::nullopt;
    return m_cache.at(row).topic;
}

std::optional<Domain::Topic> TopicListModel::topicById(int id) const {
    for (const auto& rc : m_cache) {
        if (rc.topic.id == id) return rc.topic;
    }
    return std::nullopt;
}

int TopicListModel::rowForTopicId(int id) const {
    for (int i = 0; i < m_cache.size(); ++i) {
        if (m_cache.at(i).topic.id == id) return i;
    }
    return -1;
}

}  // namespace JobPrep::Models
