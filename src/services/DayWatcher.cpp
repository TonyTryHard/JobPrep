#include "services/DayWatcher.h"

#include <QTimer>

namespace JobPrep::Services {

DayWatcher::DayWatcher(Clock clock, QObject* parent)
    : QObject(parent),
      m_clock(clock ? std::move(clock) : Clock([] { return QDate::currentDate(); })),
      m_lastDate(m_clock()),
      m_timer(new QTimer(this)) {
    m_timer->setInterval(60000);  // 60 s
    connect(m_timer, &QTimer::timeout, this, &DayWatcher::checkNow);
    m_timer->start();
}

void DayWatcher::setClock(Clock clock) {
    m_clock = clock ? std::move(clock) : Clock([] { return QDate::currentDate(); });
}

QDate DayWatcher::currentDate() const {
    return m_clock();
}

void DayWatcher::checkNow() {
    const QDate today = m_clock();
    if (today != m_lastDate) {
        m_lastDate = today;
        emit dayChanged(today);
    }
}

}  // namespace JobPrep::Services
