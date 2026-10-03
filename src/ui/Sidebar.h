#pragma once

#include <QWidget>

class QButtonGroup;
class QPushButton;
class QVBoxLayout;
class QLabel;

namespace JobPrep::Ui::Theme {
class ThemeManager;
}

namespace JobPrep::Ui {

/// Collapsible left navigation sidebar with active accent pill indicator.
class Sidebar : public QWidget {
    Q_OBJECT

public:
    static constexpr int kWidthCollapsed = 72;
    static constexpr int kWidthExpanded = 208;

    explicit Sidebar(Theme::ThemeManager& themeMgr, QWidget* parent = nullptr);
    ~Sidebar() override = default;

    bool isCollapsed() const { return m_collapsed; }
    void setCollapsed(bool collapsed);
    int currentPage() const { return m_currentPage; }
    void setCurrentPage(int index);

signals:
    void pageSelected(int index);
    void collapseToggled(bool collapsed);

private:
    void setupUi();
    void updateButtons();
    void updateToggleIcon();

    Theme::ThemeManager& m_themeMgr;
    bool m_collapsed{false};
    int m_currentPage{0};

    QVBoxLayout* m_mainLayout{nullptr};
    QWidget* m_logoContainer{nullptr};
    QLabel* m_logoIcon{nullptr};
    QLabel* m_logoText{nullptr};
    QPushButton* m_toggleBtn{nullptr};

    QButtonGroup* m_buttonGroup{nullptr};
    struct NavItem {
        QPushButton* button{nullptr};
        QString iconName;
        QString text;
        int pageIndex{0};
    };
    QList<NavItem> m_navItems;
};

}  // namespace JobPrep::Ui
