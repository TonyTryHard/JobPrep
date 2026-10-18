#pragma once

#include <QWidget>
#include <QList>
#include <QHash>
#include <QPushButton>
#include <limits>

class QButtonGroup;
class QHBoxLayout;

namespace JobPrep::Ui::Widgets {

class ChipBar : public QWidget {
    Q_OBJECT

public:
    /// Returned by currentId() when no chip is checked.
    static constexpr int kNoSelection = std::numeric_limits<int>::min();

    explicit ChipBar(QWidget* parent = nullptr);
    void addChip(const QString& text, int id);
    void clearChips();
    void setCurrent(int id);
    int currentId() const;
    bool hasChip(int id) const;
    int count() const;
    QPushButton* button(int id) const;

signals:
    void chipSelected(int id);

private:
    QButtonGroup* m_group{nullptr};
    QHBoxLayout* m_layout{nullptr};
    /// Chips by caller id. QButtonGroup cannot hold id -1 (its "no id" sentinel),
    /// so ids live here and the group only provides exclusivity.
    QHash<int, QPushButton*> m_buttons;
};

}  // namespace JobPrep::Ui::Widgets
