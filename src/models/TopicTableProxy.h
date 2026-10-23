#pragma once

#include <optional>
#include <QSortFilterProxyModel>
#include <QString>

namespace JobPrep::Models {

/// Sort and filter proxy for the Study flat list view (SPEC §3.2 S6, amendment C4).
/// Filters by track and search text across title and tags.
/// Sorts by column with empty target dates always placed last in both directions.
class TopicTableProxy : public QSortFilterProxyModel {
    Q_OBJECT

public:
    explicit TopicTableProxy(QObject* parent = nullptr);
    ~TopicTableProxy() override = default;

    void setTrackFilter(std::optional<int> trackId);
    std::optional<int> trackFilter() const { return m_trackId; }

    void setSearchFilter(const QString& text);
    QString searchFilter() const { return m_searchText; }

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;
    bool lessThan(const QModelIndex& left, const QModelIndex& right) const override;

private:
    std::optional<int> m_trackId;
    QString m_searchText;
};

}  // namespace JobPrep::Models
