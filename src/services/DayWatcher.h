#pragma once

#include <functional>
#include <QDate>
#include <QObject>

class QTimer;

namespace JobPrep::Services {

/// Watches for calendar day rollover and emits `dayChanged(QDate)` (SPEC §3.1, amendment B4).
/// Uses an injectable clock and an internal periodic timer (60s), plus `checkNow()` for testing.
class DayWatcher : public QObject {
    Q_OBJECT

public:
    using Clock = std::function<QDate()>;

    explicit DayWatcher(Clock clock = [] { return QDate::currentDate(); },
                        QObject* parent = nullptr);
    ~DayWatcher() override = default;

    void setClock(Clock clock);
    QDate currentDate() const;

    /// Checks the current date against the last observed date and emits `dayChanged` if different.
    void checkNow();

signals:
    void dayChanged(QDate newDate);

private:
    Clock m_clock;
    QDate m_lastDate;
    QTimer* m_timer{nullptr};
};

}  // namespace JobPrep::Services
