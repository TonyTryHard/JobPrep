#include "ui/widgets/ChipBar.h"
#include <QButtonGroup>
#include <QHBoxLayout>
#include <QPushButton>

using namespace Qt::StringLiterals;

namespace JobPrep::Ui::Widgets {

ChipBar::ChipBar(QWidget* parent) : QWidget(parent) {
    m_layout = new QHBoxLayout(this);
    m_layout->setContentsMargins(0, 0, 0, 0);
    m_layout->setSpacing(8);
    m_group = new QButtonGroup(this);
    m_group->setExclusive(true);
    connect(m_group, &QButtonGroup::idClicked, this, &ChipBar::onClicked);
}

void ChipBar::addChip(const QString& text, int id) {
    auto* btn = new QPushButton(text, this);
    btn->setCheckable(true);
    btn->setAutoExclusive(true);
    m_group->addButton(btn, id);
    m_layout->addWidget(btn);
}

void ChipBar::setCurrent(int id) {
    auto* btn = m_group->button(id);
    if (btn) btn->setChecked(true);
}

void ChipBar::onClicked(int id) {
    emit chipSelected(id);
}

}  // namespace JobPrep::Ui::Widgets
