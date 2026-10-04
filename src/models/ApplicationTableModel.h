#pragma once

#include <QAbstractTableModel>
#include <QDateTime>
#include <QList>
#include <optional>
#include "domain/Enums.h"
#include "domain/Structs.h"

namespace JobPrep::Data {
class ApplicationRepository;
class InterviewRepository;
}  // namespace JobPrep::Data

namespace JobPrep::Models {

class ApplicationTableModel : public QAbstractTableModel {
    Q_OBJECT

public:
    enum Column {
        CompanyColumn = 0,
        PositionColumn,
        StatusColumn,
        AppliedColumn,
        NextStepColumn,
        SalaryColumn,
        SourceColumn,
        UpdatedColumn,
        ColumnCount
    };

    enum Role {
        StatusRole = Qt::UserRole + 1,
        SortRole,
        SearchRole
    };

    ApplicationTableModel(JobPrep::Data::ApplicationRepository& apps,
                          JobPrep::Data::InterviewRepository& interviews,
                          QObject* parent = nullptr);
    ~ApplicationTableModel() override = default;

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

public slots:
    void reload();

private:
    struct RowCache {
        JobPrep::Domain::JobApplication app;
        std::optional<QDateTime> nextInterviewAt;
        QString nextStepText;
    };

    int effectiveSalary(const JobPrep::Domain::JobApplication& app) const;
    void buildCache();

    JobPrep::Data::ApplicationRepository& m_apps;
    JobPrep::Data::InterviewRepository& m_interviews;
    QList<RowCache> m_cache;
};

}  // namespace JobPrep::Models
