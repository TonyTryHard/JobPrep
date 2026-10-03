#pragma once

#include <QMainWindow>
#include <QList>

class QLabel;
class QLineEdit;
class QPushButton;
class QStackedWidget;

namespace JobPrep::App {
class AppContext;
}

namespace JobPrep::Ui {
class Sidebar;
class PageBase;

/// Main application window shell for JobPrep.
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(App::AppContext& ctx, QWidget* parent = nullptr);
    ~MainWindow() override;

    Sidebar* sidebar() const { return m_sidebar; }
    QStackedWidget* pageStack() const { return m_pageStack; }

protected:
    void closeEvent(QCloseEvent* event) override;

private:
    void setupUi();
    void setupShortcuts();
    void setupConnections();
    void updateHeader(int pageIndex);
    void updateIcons();

    App::AppContext& m_ctx;
    Sidebar* m_sidebar{nullptr};
    QLabel* m_pageTitleLabel{nullptr};
    QLineEdit* m_searchEdit{nullptr};
    QAction* m_searchAction{nullptr};
    QPushButton* m_newBtn{nullptr};
    QStackedWidget* m_pageStack{nullptr};
    QList<PageBase*> m_pages;

    QLabel* m_statusStreakLabel{nullptr};
    QLabel* m_statusProcessesLabel{nullptr};
    QLabel* m_statusNextInterviewLabel{nullptr};
};

}  // namespace JobPrep::Ui
