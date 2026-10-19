#pragma once

#include <QStyledItemDelegate>

namespace JobPrep::Ui::Delegates {

class StatusBadgeDelegate : public QStyledItemDelegate {
    Q_OBJECT

public:
    enum class Mode { Application, Topic };

    explicit StatusBadgeDelegate(Mode mode = Mode::Application, QObject* parent = nullptr);
    explicit StatusBadgeDelegate(QObject* parent) : StatusBadgeDelegate(Mode::Application, parent) {}

    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;

private:
    Mode m_mode{Mode::Application};
};

}  // namespace JobPrep::Ui::Delegates
