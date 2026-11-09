#include "models/ApplicationFilterProxy.h"
#include "models/ApplicationTableModel.h"

using namespace Qt::StringLiterals;

namespace JobPrep::Models {

ApplicationFilterProxy::ApplicationFilterProxy(QObject* parent)
    : QSortFilterProxyModel(parent), m_chip(All) {
    setDynamicSortFilter(true);
    setSortRole(ApplicationTableModel::SortRole);
    sort(ApplicationTableModel::UpdatedColumn, Qt::DescendingOrder);
}

void ApplicationFilterProxy::setFilterChip(FilterChip chip) {
    if (m_chip == chip) return;
#if QT_VERSION >= QT_VERSION_CHECK(6, 9, 0)
    beginFilterChange();
    m_chip = chip;
    endFilterChange(QSortFilterProxyModel::Direction::Rows);
#else
    m_chip = chip;
    QSortFilterProxyModel::invalidateFilter();
#endif
}

void ApplicationFilterProxy::setSearchText(const QString& text) {
    if (m_searchText == text) return;
#if QT_VERSION >= QT_VERSION_CHECK(6, 9, 0)
    beginFilterChange();
    m_searchText = text;
    endFilterChange(QSortFilterProxyModel::Direction::Rows);
#else
    m_searchText = text;
    QSortFilterProxyModel::invalidateFilter();
#endif
}

bool ApplicationFilterProxy::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const {
    const QModelIndex idx = sourceModel()->index(sourceRow, ApplicationTableModel::StatusColumn, sourceParent);
    const auto status = sourceModel()->data(idx, ApplicationTableModel::StatusRole).value<JobPrep::Domain::ApplicationStatus>();

    // Chip filter
    if (m_chip != All) {
        using AS = JobPrep::Domain::ApplicationStatus;
        bool ok = false;
        if (m_chip == Active) {
            if (status == AS::Applied || status == AS::HrScreen || status == AS::Technical ||
                status == AS::Final || status == AS::Offer) ok = true;
        } else if (m_chip == Interviewing) {
            if (status == AS::HrScreen || status == AS::Technical || status == AS::Final) ok = true;
        } else if (m_chip == Offer) {
            if (status == AS::Offer) ok = true;
        } else if (m_chip == Closed) {
            if (status == AS::Rejected || status == AS::Withdrawn || status == AS::Ghosted ||
                status == AS::Accepted) ok = true;
        }
        if (!ok) return false;
    }

    if (!m_searchText.isEmpty()) {
        const QModelIndex idx0 = sourceModel()->index(sourceRow, ApplicationTableModel::CompanyColumn, sourceParent);
        const QString hay = sourceModel()->data(idx0, ApplicationTableModel::SearchRole).toString();
        if (!hay.contains(m_searchText, Qt::CaseInsensitive)) {
            return false;
        }
    }
    return true;
}

bool ApplicationFilterProxy::lessThan(const QModelIndex& left, const QModelIndex& right) const {
    return QSortFilterProxyModel::lessThan(left, right);
}

}  // namespace JobPrep::Models
