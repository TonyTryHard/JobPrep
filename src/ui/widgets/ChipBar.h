#pragma once

#include <QWidget>
#include <QList>
#include <QPushButton>

class QButtonGroup;
class QHBoxLayout;

namespace JobPrep::Ui::Widgets {

class ChipBar : public QWidget {
    Q_OBJECT

public:
    explicit ChipBar(QWidget* parent = nullptr);
    void addChip(const QString& text, int id);
    void setCurrent(int id);

signals:
    void chipSelected(int id);

private slots:
    void onClicked(int id);

private:
    QButtonGroup* m_group{nullptr};
    QHBoxLayout* m_layout{nullptr};
};

}  // namespace JobPrep::Ui::Widgets
