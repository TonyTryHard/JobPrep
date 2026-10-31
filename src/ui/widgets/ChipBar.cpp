#include "ui/widgets/ChipBar.h"
#include <QButtonGroup>
#include <QHBoxLayout>
#include <QPushButton>

using namespace Qt::StringLiterals;

namespace JobPrep::Ui::Widgets {

ChipBar::ChipBar(QWidget* parent) : QWidget(parent) {
    setFocusPolicy(Qt::TabFocus);
    m_layout = new QHBoxLayout(this);
    m_layout->setContentsMargins(0, 0, 0, 0);
    m_layout->setSpacing(8);
    m_group = new QButtonGroup(this);
    m_group->setExclusive(true);
}

void ChipBar::addChip(const QString& text, int id) {
    auto* btn = new QPushButton(text, this);
    btn->setObjectName(u"ChipBarChip"_s);
    btn->setProperty("chip", true);
    btn->setCheckable(true);
    btn->setAutoExclusive(true);
    m_group->addButton(btn);  // exclusivity only; ids live in m_buttons
    m_buttons.insert(id, btn);
    m_layout->addWidget(btn);
    connect(btn, &QPushButton::clicked, this, [this, id] { emit chipSelected(id); });
}

void ChipBar::clearChips() {
    setFocusProxy(nullptr);
    const auto buttons = m_group->buttons();
    for (auto* btn : buttons) {
        m_group->removeButton(btn);
        m_layout->removeWidget(btn);
        m_buttons.remove(m_buttons.key(qobject_cast<QPushButton*>(btn)));
        btn->deleteLater();
    }
}

void ChipBar::setCurrent(int id) {
    if (auto* btn = m_buttons.value(id)) {
        btn->setChecked(true);
        setFocusProxy(btn);
    }
}

int ChipBar::currentId() const {
    for (auto it = m_buttons.cbegin(); it != m_buttons.cend(); ++it) {
        if (it.value()->isChecked()) return it.key();
    }
    return kNoSelection;
}

bool ChipBar::hasChip(int id) const {
    return m_buttons.contains(id);
}

int ChipBar::count() const {
    return static_cast<int>(m_group->buttons().size());
}

QPushButton* ChipBar::button(int id) const {
    return m_buttons.value(id, nullptr);
}

}  // namespace JobPrep::Ui::Widgets
