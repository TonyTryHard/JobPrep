#pragma once

#include "ui/PageBase.h"

class QCheckBox;
class QComboBox;
class QFrame;
class QListWidget;
class QPushButton;
class QSpinBox;

namespace JobPrep::Services {
class SettingsService;
}

namespace JobPrep::Ui {
class DataToolsController;
}

namespace JobPrep::Ui::Theme {
class ThemeManager;
}

namespace JobPrep::Ui::Pages {

class GoalSpinBox;

/// Application settings page with theme, study goal, preferences, and data tools.
class SettingsPage : public PageBase {
    Q_OBJECT

public:
    explicit SettingsPage(Services::SettingsService& settings,
                          Theme::ThemeManager& themeMgr,
                          bool trayAvailable = false,
                          QWidget* parent = nullptr,
                          DataToolsController* dataTools = nullptr);
    ~SettingsPage() override = default;

    /// Updates system tray availability and updates close-to-tray checkbox accordingly.
    void setTrayAvailable(bool available);
    bool isTrayAvailable() const { return m_trayAvailable; }

    /// Injects or updates the data tools controller for export, backup and restore actions.
    void setDataToolsController(DataToolsController* controller);
    DataToolsController* dataToolsController() const { return m_dataTools; }

    // Test accessors
    QComboBox* themeCombo() const { return m_themeCombo; }
    QComboBox* accentCombo() const { return m_accentCombo; }
    QSpinBox* studyGoalSpinBox() const;
    QComboBox* weekStartCombo() const { return m_weekStartCombo; }
    QCheckBox* closeToTrayCheck() const { return m_closeToTrayCheck; }
    QListWidget* leadList() const { return m_leadList; }
    QComboBox* leadPresetCombo() const { return m_leadPresetCombo; }
    QSpinBox* customLeadSpinBox() const { return m_customLeadSpinBox; }
    QPushButton* addLeadButton() const { return m_addLeadBtn; }
    QPushButton* removeLeadButton() const { return m_removeLeadBtn; }
    QPushButton* resetLeadButton() const { return m_resetLeadBtn; }

    // Data tools accessors (C8)
    QFrame* dataCard() const { return m_dataCard; }
    QPushButton* exportCsvButton() const { return m_exportCsvBtn; }
    QPushButton* exportIcsButton() const { return m_exportIcsBtn; }
    QPushButton* backupJsonButton() const { return m_backupJsonBtn; }
    QPushButton* restoreJsonButton() const { return m_restoreJsonBtn; }
    QPushButton* loadSampleButton() const { return m_loadSampleBtn; }
    QPushButton* openFolderButton() const { return m_openFolderBtn; }

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    void setupUi();
    void loadSettings();
    void refreshLeadList();
    void updateCloseToTrayState();

    Services::SettingsService& m_settings;
    Theme::ThemeManager& m_themeMgr;
    bool m_trayAvailable{false};
    bool m_syncingTheme{false};
    DataToolsController* m_dataTools{nullptr};

    QComboBox* m_themeCombo{nullptr};
    QComboBox* m_accentCombo{nullptr};
    GoalSpinBox* m_studyGoalSpinBox{nullptr};
    QComboBox* m_weekStartCombo{nullptr};
    QCheckBox* m_closeToTrayCheck{nullptr};

    QListWidget* m_leadList{nullptr};
    QComboBox* m_leadPresetCombo{nullptr};
    QSpinBox* m_customLeadSpinBox{nullptr};
    QPushButton* m_addLeadBtn{nullptr};
    QPushButton* m_removeLeadBtn{nullptr};
    QPushButton* m_resetLeadBtn{nullptr};

    // Data section controls (C8)
    QFrame* m_dataCard{nullptr};
    QPushButton* m_exportCsvBtn{nullptr};
    QPushButton* m_exportIcsBtn{nullptr};
    QPushButton* m_backupJsonBtn{nullptr};
    QPushButton* m_restoreJsonBtn{nullptr};
    QPushButton* m_loadSampleBtn{nullptr};
    QPushButton* m_openFolderBtn{nullptr};
};

}  // namespace JobPrep::Ui::Pages
