#pragma once

#include <QString>
#include <QStringList>

namespace JobPrep::Tests {

/// Collects warnings and criticals instead of printing them, for tests that
/// deliberately provoke failing statements. Instances must not overlap.
class MessageCapture {
public:
    MessageCapture();
    ~MessageCapture();

    MessageCapture(const MessageCapture&) = delete;
    MessageCapture& operator=(const MessageCapture&) = delete;
    MessageCapture(MessageCapture&&) = delete;
    MessageCapture& operator=(MessageCapture&&) = delete;

    QStringList messages() const;
    bool contains(QStringView needle) const;
};

}  // namespace JobPrep::Tests
