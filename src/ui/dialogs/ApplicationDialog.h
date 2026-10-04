#pragma once

#include <QDialog>
#include "domain/Structs.h"

namespace JobPrep::Data {
class ApplicationRepository;
}

namespace JobPrep::Ui::Dialogs {

class ApplicationDialog : public QDialog {
    Q_OBJECT

public:
    enum Mode { Create, Edit };

    ApplicationDialog(JobPrep::Data::ApplicationRepository& apps, Mode mode, QWidget* parent = nullptr);
    void setApplication(const JobPrep::Domain::JobApplication& app);
    JobPrep::Domain::JobApplication application() const;

private slots:
    void onSave();
    void onCancel();

private:
    JobPrep::Data::ApplicationRepository& m_apps;
    Mode m_mode;
    JobPrep::Domain::JobApplication m_app;
};

}  // namespace JobPrep::Ui::Dialogs
