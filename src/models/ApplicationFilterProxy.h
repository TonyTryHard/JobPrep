#pragma once

#include <QSortFilterProxyModel>
#include "domain/Enums.h"

namespace JobPrep::Models {

class ApplicationFilterProxy : public QSortFilterProxyModel {
    Q_OBJECT

public:
    enum FilterChip {
        All = 0,
        Active,
        Interviewing,
        Offer,
        Closed
    };

    explicit ApplicationFilterProxy(QObject* parent = nullptr);
    void setFilterChip(FilterChip chip);
    void setSearchText(const QString& text);

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;
    bool lessThan(const QModelIndex& left, const QModelIndex& right) const override;

private:
    FilterChip m_chip{All};
    QString m_searchText;
};

}  // namespace JobPrep::Models
