#include "models/ApplicationTableModel.h"
#include <QDate>
#include <QMap>
#include "data/ApplicationRepository.h"
#include "data/DbFormat.h"
#include "data/InterviewRepository.h"
#include "domain/EnumStrings.h"

using namespace Qt::StringLiterals;

namespace JobPrep::Models {

ApplicationTableModel::ApplicationTableModel(JobPrep::Data::ApplicationRepository& apps,
                                             JobPrep::Data::InterviewRepository& interviews,
                                             QObject* parent)
    : QAbstractTableModel(parent), m_apps(apps), m_interviews(interviews) {
    connect(&m_apps, &JobPrep::Data::ApplicationRepository::changed, this, &ApplicationTableModel::reload);
    connect(&m_interviews, &JobPrep::Data::InterviewRepository::changed, this, &ApplicationTableModel::reload);
    reload();
}

int ApplicationTableModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) return 0;
    return m_cache.size();
}

int ApplicationTableModel::columnCount(const QModelIndex& parent) const {
    if (parent.isValid()) return 0;
    return ColumnCount;
}

int ApplicationTableModel::effectiveSalary(const JobPrep::Domain::JobApplication& app) const {
    if (app.salaryMax.has_value()) return app.salaryMax.value();
    if (app.salaryMin.has_value()) return app.salaryMin.value();
    return -1;
}

void ApplicationTableModel::buildCache() {
    const auto apps = m_apps.all();
    // Group all interviews by application
    const auto allInterviews = m_interviews.all();
    QMap<int, QList<JobPrep::Domain::Interview> > byApp;
    for (const auto& iv : allInterviews) {
        byApp[iv.applicationId].append(iv);
    }
    const QDateTime now = QDateTime::currentDateTime();

    m_cache.clear();
    m_cache.reserve(apps.size());
    for (const auto& app : apps) {
        RowCache rc;
        rc.app = app;
        // Find earliest upcoming pending interview
        QList<JobPrep::Domain::Interview> ivs = byApp.value(app.id);
        // Sort by startAt
        std::sort(ivs.begin(), ivs.end(), [](const auto& a, const auto& b) {
            if (a.startAt == b.startAt) return a.id < b.id;
            return a.startAt < b.startAt;
        });
        for (const auto& iv : ivs) {
            if (iv.outcome == JobPrep::Domain::InterviewOutcome::Pending && iv.startAt >= now) {
                rc.nextInterviewAt = iv.startAt;
                break;
            }
        }
        // Build next step text
        if (rc.nextInterviewAt.has_value()) {
            const QDateTime sat = rc.nextInterviewAt.value();
            rc.nextStepText = sat.toString(u"yyyy-MM-dd HH:mm"_s);
        } else {
            if (!app.nextAction.isEmpty() && app.nextActionDate.has_value()) {
                rc.nextStepText = app.nextAction + u" ("_s + JobPrep::Data::DbFormat::toText(app.nextActionDate.value()) + u")"_s;
            } else if (app.nextActionDate.has_value()) {
                rc.nextStepText = JobPrep::Data::DbFormat::toText(app.nextActionDate.value());
            } else if (!app.nextAction.isEmpty()) {
                rc.nextStepText = app.nextAction;
            }
        }
        m_cache.append(rc);
    }
}

QVariant ApplicationTableModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= m_cache.size()) return QVariant();
    const auto& rc = m_cache.at(index.row());
    const auto& app = rc.app;
    if (role == Qt::DisplayRole) {
        switch (index.column()) {
            case CompanyColumn: return app.company;
            case PositionColumn: return app.position;
            case StatusColumn: {
                switch (app.status) {
                    case Domain::ApplicationStatus::Wishlist: return tr("Wishlist");
                    case Domain::ApplicationStatus::Applied: return tr("Applied");
                    case Domain::ApplicationStatus::HrScreen: return tr("HR Screen");
                    case Domain::ApplicationStatus::Technical: return tr("Technical");
                    case Domain::ApplicationStatus::Final: return tr("Final");
                    case Domain::ApplicationStatus::Offer: return tr("Offer");
                    case Domain::ApplicationStatus::Accepted: return tr("Accepted");
                    case Domain::ApplicationStatus::Rejected: return tr("Rejected");
                    case Domain::ApplicationStatus::Withdrawn: return tr("Withdrawn");
                    case Domain::ApplicationStatus::Ghosted: return tr("Ghosted");
                }
                return QString();
            }
            case AppliedColumn: return app.appliedDate.has_value() ? JobPrep::Data::DbFormat::toText(app.appliedDate.value()) : QString();
            case NextStepColumn: return rc.nextStepText;
            case SalaryColumn: {
                if (app.salaryMin.has_value() && app.salaryMax.has_value()) {
                    return QString::number(app.salaryMin.value()) + u"–"_s + QString::number(app.salaryMax.value()) + u" "_s + app.currency;
                }
                if (app.salaryMax.has_value()) return QString::number(app.salaryMax.value()) + u" "_s + app.currency;
                if (app.salaryMin.has_value()) return QString::number(app.salaryMin.value()) + u" "_s + app.currency;
                return QString();
            }
            case SourceColumn: return app.source;
            case UpdatedColumn: return JobPrep::Data::DbFormat::toText(app.updatedAt.date());
            default: return QVariant();
        }
    } else if (role == StatusRole) {
        return QVariant::fromValue(app.status);
    } else if (role == SearchRole) {
        return (app.company + u" "_s + app.position + u" "_s + app.notes).toLower();
    } else if (role == SortRole) {
        switch (index.column()) {
            case CompanyColumn: return app.company.toLower();
            case PositionColumn: return app.position.toLower();
            case StatusColumn: {
                switch (app.status) {
                    case Domain::ApplicationStatus::Wishlist: return 0;
                    case Domain::ApplicationStatus::Applied: return 1;
                    case Domain::ApplicationStatus::HrScreen: return 2;
                    case Domain::ApplicationStatus::Technical: return 3;
                    case Domain::ApplicationStatus::Final: return 4;
                    case Domain::ApplicationStatus::Offer: return 5;
                    case Domain::ApplicationStatus::Accepted: return 6;
                    case Domain::ApplicationStatus::Rejected: return 10;
                    case Domain::ApplicationStatus::Withdrawn: return 11;
                    case Domain::ApplicationStatus::Ghosted: return 12;
                }
                return 99;
            }
            case AppliedColumn: return app.appliedDate.has_value() ? app.appliedDate.value() : QDate(1900,1,1);
            case NextStepColumn: {
                if (rc.nextInterviewAt.has_value()) return rc.nextInterviewAt.value();
                if (app.nextActionDate.has_value()) return QDateTime(app.nextActionDate.value(), QTime(0,0));
                return QDateTime(QDate(9999,12,31), QTime(23,59));
            }
            case SalaryColumn: return effectiveSalary(app);
            case SourceColumn: return app.source.toLower();
            case UpdatedColumn: return app.updatedAt;
            default: return QVariant();
        }
    }
    return QVariant();
}

QVariant ApplicationTableModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole) return QAbstractTableModel::headerData(section, orientation, role);
    switch (section) {
        case CompanyColumn: return tr("Company");
        case PositionColumn: return tr("Position");
        case StatusColumn: return tr("Status");
        case AppliedColumn: return tr("Applied");
        case NextStepColumn: return tr("Next step");
        case SalaryColumn: return tr("Salary");
        case SourceColumn: return tr("Source");
        case UpdatedColumn: return tr("Updated");
        default: return QVariant();
    }
}

void ApplicationTableModel::reload() {
    beginResetModel();
    buildCache();
    endResetModel();
}

}  // namespace JobPrep::Models
