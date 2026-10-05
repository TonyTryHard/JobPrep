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

    /// Moves focus into the page's own search box (QKeySequence::Find). Pages without a
    /// search box keep the default no-op; the header decides whether to switch pages.
    virtual void focusSearch() {}

    /// Opens the page's primary "add" flow (QKeySequence::New).
    virtual void triggerNew() {}

private:
    QString m_title;
};

}  // namespace JobPrep::Ui
