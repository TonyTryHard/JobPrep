#pragma once

#include <functional>
#include <optional>
#include <QObject>

class QAbstractItemModel;
class QAbstractItemView;

namespace JobPrep::Ui::Widgets {

/// Reusable helper that preserves and restores view selection and scroll position by item ID across model resets (SPEC §8, A5).
class SelectionRestorer : public QObject {
    Q_OBJECT

public:
    explicit SelectionRestorer(QAbstractItemView* view,
                               int idRole = Qt::UserRole + 1,
                               QObject* parent = nullptr);
    ~SelectionRestorer() override = default;

    void setView(QAbstractItemView* view);
    QAbstractItemView* view() const { return m_view; }

    void setIdRole(int role) { m_idRole = role; }
    int idRole() const { return m_idRole; }

    void setTargetColumn(int column) { m_column = column; }
    int targetColumn() const { return m_column; }

    void capture();
    bool restore(std::optional<int> overrideId = std::nullopt);
    void preserveAround(const std::function<void()>& action);

    void setSavedId(std::optional<int> id) { m_savedId = id; }
    std::optional<int> savedId() const { return m_savedId; }

    int savedVerticalScroll() const { return m_savedVScroll; }
    int savedHorizontalScroll() const { return m_savedHScroll; }

public slots:
    void onModelAboutToBeReset();
    void onModelReset();

private:
    void connectModel(QAbstractItemModel* model);

    QAbstractItemView* m_view{nullptr};
    int m_idRole{Qt::UserRole + 1};
    int m_column{0};
    std::optional<int> m_savedId;
    int m_savedVScroll{0};
    int m_savedHScroll{0};
};

}  // namespace JobPrep::Ui::Widgets
