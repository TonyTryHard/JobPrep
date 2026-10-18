#include "ui/widgets/SelectionRestorer.h"

#include <QAbstractItemModel>
#include <QAbstractItemView>
#include <QItemSelectionModel>
#include <QScrollBar>

namespace JobPrep::Ui::Widgets {

SelectionRestorer::SelectionRestorer(QAbstractItemView* view, int idRole, QObject* parent)
    : QObject(parent ? parent : view), m_view(view), m_idRole(idRole) {
    if (m_view && m_view->model()) {
        connectModel(m_view->model());
    }
}

void SelectionRestorer::setView(QAbstractItemView* view) {
    if (m_view == view) return;
    m_view = view;
    if (m_view && m_view->model()) {
        connectModel(m_view->model());
    }
}

void SelectionRestorer::connectModel(QAbstractItemModel* model) {
    if (!model) return;
    connect(model, &QAbstractItemModel::modelAboutToBeReset,
            this, &SelectionRestorer::onModelAboutToBeReset,
            Qt::UniqueConnection);
    connect(model, &QAbstractItemModel::modelReset,
            this, &SelectionRestorer::onModelReset,
            Qt::UniqueConnection);
}

void SelectionRestorer::capture() {
    if (!m_view) return;
    const QModelIndex current = m_view->currentIndex();
    if (current.isValid()) {
        m_savedId = current.data(m_idRole).toInt();
    } else {
        m_savedId = std::nullopt;
    }
    if (m_view->verticalScrollBar()) {
        m_savedVScroll = m_view->verticalScrollBar()->value();
    }
    if (m_view->horizontalScrollBar()) {
        m_savedHScroll = m_view->horizontalScrollBar()->value();
    }
}

bool SelectionRestorer::restore(std::optional<int> overrideId) {
    if (!m_view || !m_view->model()) return false;

    // If an external model was attached directly after construction
    connectModel(m_view->model());

    const auto targetId = overrideId.has_value() ? overrideId : m_savedId;
    bool found = false;

    if (targetId.has_value()) {
        const int rows = m_view->model()->rowCount();
        for (int r = 0; r < rows; ++r) {
            const QModelIndex idx = m_view->model()->index(r, m_column);
            if (idx.data(m_idRole).toInt() == *targetId) {
                m_view->setCurrentIndex(idx);
                if (auto* sm = m_view->selectionModel()) {
                    sm->select(idx, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
                }
                found = true;
                break;
            }
        }
    }

    if (!found) {
        m_view->clearSelection();
    }

    if (m_view->verticalScrollBar()) {
        m_view->verticalScrollBar()->setValue(m_savedVScroll);
    }
    if (m_view->horizontalScrollBar()) {
        m_view->horizontalScrollBar()->setValue(m_savedHScroll);
    }

    return found;
}

void SelectionRestorer::preserveAround(const std::function<void()>& action) {
    capture();
    if (action) {
        action();
    }
    restore();
}

void SelectionRestorer::onModelAboutToBeReset() {
    capture();
}

void SelectionRestorer::onModelReset() {
    restore();
}

}  // namespace JobPrep::Ui::Widgets
