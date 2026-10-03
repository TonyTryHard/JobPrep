#include "ui/PageBase.h"

namespace JobPrep::Ui {

PageBase::PageBase(const QString& title, QWidget* parent)
    : QWidget(parent), m_title(title) {}

}  // namespace JobPrep::Ui
