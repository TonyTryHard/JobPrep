#pragma once

#include <QWidget>

namespace JobPrep::Ui {

/// Base class for all page views in JobPrep.
class PageBase : public QWidget {
    Q_OBJECT

public:
    explicit PageBase(const QString& title, QWidget* parent = nullptr);
    ~PageBase() override = default;

    QString pageTitle() const { return m_title; }

    /// Called whenever the page becomes the active page in the main stack.
    virtual void onActivated() {}

private:
    QString m_title;
};

}  // namespace JobPrep::Ui
