#include "ui/dialogs/SessionDialog.h"

#include <QComboBox>
#include <QDateEdit>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QShortcut>
#include <QSpinBox>
#include <QTextEdit>
#include <QVBoxLayout>
#include "ui/widgets/SingleTabDateTime.h"
#include "data/SessionRepository.h"
#include "data/TopicRepository.h"

using namespace Qt::StringLiterals;

namespace JobPrep::Ui::Dialogs {

SessionDialog::SessionDialog(Data::SessionRepository& sessions,
                             Data::TopicRepository& topics,
                             QWidget* parent,
                             std::function<QDate()> todayProvider)
    : QDialog(parent),
      m_sessions(sessions),
      m_topics(topics),
      m_todayProvider(todayProvider ? std::move(todayProvider)
                                    : std::function<QDate()>([] { return QDate::currentDate(); })) {
    setWindowTitle(tr("Log Study Session"));
    setModal(true);
    resize(420, 340);
    setupUi();
    populateTopics();
    validateForm();
}

void SessionDialog::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(14);

    auto* formLayout = new QFormLayout();
    formLayout->setSpacing(12);
    formLayout->setLabelAlignment(Qt::AlignLeft);

    const QDate today = m_todayProvider ? m_todayProvider() : QDate::currentDate();
    m_dateEdit = new Widgets::SingleTabDateEdit(today, this);
    m_dateEdit->setObjectName(u"SessionDateEdit"_s);
    m_dateEdit->setCalendarPopup(true);
    m_dateEdit->setDisplayFormat(u"yyyy-MM-dd"_s);
    formLayout->addRow(tr("Date:"), m_dateEdit);

    m_minutesSpin = new QSpinBox(this);
    m_minutesSpin->setObjectName(u"SessionMinutesSpin"_s);
    m_minutesSpin->setRange(0, 1440);
    m_minutesSpin->setValue(30);
    m_minutesSpin->setSuffix(tr(" min"));
    formLayout->addRow(tr("Duration:"), m_minutesSpin);

    m_topicCombo = new QComboBox(this);
    m_topicCombo->setObjectName(u"SessionTopicCombo"_s);
    formLayout->addRow(tr("Topic:"), m_topicCombo);

    m_noteEdit = new QTextEdit(this);
    m_noteEdit->setObjectName(u"SessionNoteEdit"_s);
    m_noteEdit->setPlaceholderText(tr("Notes (optional)"));
    m_noteEdit->setTabChangesFocus(true);
    m_noteEdit->setMaximumHeight(80);
    formLayout->addRow(tr("Notes:"), m_noteEdit);

    mainLayout->addLayout(formLayout);

    m_messageLabel = new QLabel(this);
    m_messageLabel->setObjectName(u"SessionMessageLabel"_s);
    m_messageLabel->hide();
    mainLayout->addWidget(m_messageLabel);

    auto* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch(1);

    m_cancelBtn = new QPushButton(tr("Cancel"), this);
    m_cancelBtn->setObjectName(u"SessionCancelBtn"_s);
    m_cancelBtn->setToolTip(tr("Discard changes (Esc)"));
    buttonLayout->addWidget(m_cancelBtn);

    m_saveBtn = new QPushButton(tr("Log session"), this);
    m_saveBtn->setObjectName(u"SessionSaveBtn"_s);
    m_saveBtn->setProperty("primary", true);
    m_saveBtn->setDefault(true);
    m_saveBtn->setToolTip(tr("Log session (Ctrl+S)"));
    buttonLayout->addWidget(m_saveBtn);

    mainLayout->addLayout(buttonLayout);

    auto* saveShortcut = new QShortcut(QKeySequence::Save, this);
    connect(saveShortcut, &QShortcut::activated, this, &SessionDialog::onSave);

    QWidget::setTabOrder(m_dateEdit, m_minutesSpin);
    QWidget::setTabOrder(m_minutesSpin, m_topicCombo);
    QWidget::setTabOrder(m_topicCombo, m_noteEdit);
    QWidget::setTabOrder(m_noteEdit, m_saveBtn);
    QWidget::setTabOrder(m_saveBtn, m_cancelBtn);

    connect(m_dateEdit, &QDateEdit::dateChanged, this, &SessionDialog::onFormChanged);
    connect(m_minutesSpin, &QSpinBox::valueChanged, this, &SessionDialog::onFormChanged);
    connect(m_cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    connect(m_saveBtn, &QPushButton::clicked, this, &SessionDialog::onSave);
}

void SessionDialog::populateTopics() {
    m_topicCombo->clear();
    m_topicCombo->addItem(tr("(No topic)"), QVariant());

    auto allTopics = m_topics.all();
    std::sort(allTopics.begin(), allTopics.end(), [](const Domain::Topic& a, const Domain::Topic& b) {
        return a.title.compare(b.title, Qt::CaseInsensitive) < 0;
    });

    for (const auto& topic : allTopics) {
        m_topicCombo->addItem(topic.title, topic.id);
    }
}

void SessionDialog::setTopicId(std::optional<int> topicId) {
    if (!topicId.has_value() || *topicId <= 0) {
        m_topicCombo->setCurrentIndex(0);
        return;
    }
    const int idx = m_topicCombo->findData(*topicId);
    if (idx >= 0) {
        m_topicCombo->setCurrentIndex(idx);
    } else {
        m_topicCombo->setCurrentIndex(0);
    }
}

void SessionDialog::setDate(QDate date) {
    if (date.isValid()) {
        m_dateEdit->setDate(date);
    }
}

void SessionDialog::setMinutes(int minutes) {
    m_minutesSpin->setValue(minutes);
}

void SessionDialog::setNote(const QString& note) {
    m_noteEdit->setPlainText(note);
}

Domain::StudySession SessionDialog::session() const {
    Domain::StudySession s;
    s.sessionDate = m_dateEdit->date();
    s.minutes = m_minutesSpin->value();
    const QVariant val = m_topicCombo->currentData();
    if (val.isValid() && val.toInt() > 0) {
        s.topicId = val.toInt();
    } else {
        s.topicId = std::nullopt;
    }
    s.note = m_noteEdit->toPlainText().trimmed();
    return s;
}

QString SessionDialog::validationMessage() const {
    if (m_minutesSpin->value() < 1) {
        return tr("Duration must be at least 1 minute.");
    }
    return QString();
}

bool SessionDialog::validateForm() {
    const QString msg = validationMessage();
    if (!msg.isEmpty()) {
        m_messageLabel->setText(msg);
        m_messageLabel->show();
        m_saveBtn->setEnabled(false);
        return false;
    }
    m_messageLabel->hide();
    m_saveBtn->setEnabled(true);
    return true;
}

void SessionDialog::onFormChanged() {
    validateForm();
}

void SessionDialog::onSave() {
    if (!validateForm()) return;

    m_session = session();
    if (!m_sessions.insert(m_session)) {
        m_messageLabel->setText(tr("Could not log session: %1").arg(m_sessions.lastError()));
        m_messageLabel->show();
        return;
    }
    accept();
}

}  // namespace JobPrep::Ui::Dialogs
