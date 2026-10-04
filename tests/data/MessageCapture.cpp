#include "MessageCapture.h"

#include <QLoggingCategory>
#include <QtGlobal>

namespace {

QStringList s_messages;
QtMessageHandler s_previousHandler = nullptr;

void captureHandler(QtMsgType type, const QMessageLogContext& context, const QString& message) {
    Q_UNUSED(context);
    if (type == QtWarningMsg || type == QtCriticalMsg) s_messages.append(message);
}

}  // namespace

namespace JobPrep::Tests {

MessageCapture::MessageCapture() {
    Q_ASSERT_X(s_previousHandler == nullptr, "MessageCapture", "instances must not overlap");
    s_messages.clear();
    s_previousHandler = qInstallMessageHandler(captureHandler);
}

MessageCapture::~MessageCapture() {
    qInstallMessageHandler(s_previousHandler);
    s_previousHandler = nullptr;
    s_messages.clear();
}

QStringList MessageCapture::messages() const {
    return s_messages;
}

bool MessageCapture::contains(QStringView needle) const {
    for (const QString& message : s_messages) {
        if (message.contains(needle)) return true;
    }
    return false;
}

}  // namespace JobPrep::Tests
