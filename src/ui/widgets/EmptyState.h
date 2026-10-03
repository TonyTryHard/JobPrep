#pragma once

#include <QWidget>

class QLabel;
class QPushButton;
class QVBoxLayout;

namespace JobPrep::Ui::Widgets {

/// Reusable empty state placeholder widget showing icon, title, description, and optional action.
class EmptyState : public QWidget {
    Q_OBJECT

public:
    explicit EmptyState(const QString& iconName, const QString& title,
                        const QString& description = QString(),
                        const QString& actionText = QString(),
                        QWidget* parent = nullptr);
    ~EmptyState() override = default;

    void setIcon(const QString& iconName);
    void setTitle(const QString& title);
    void setDescription(const QString& description);
    void setActionText(const QString& actionText);

signals:
    void actionClicked();

protected:
    void changeEvent(QEvent* event) override;
    void showEvent(QShowEvent* event) override;

private:
    void setupUi();
    void updateIcon();

    QString m_iconName;
    QLabel* m_iconLabel{nullptr};
    QLabel* m_titleLabel{nullptr};
    QLabel* m_descLabel{nullptr};
    QPushButton* m_actionBtn{nullptr};
    QVBoxLayout* m_layout{nullptr};
};

}  // namespace JobPrep::Ui::Widgets
