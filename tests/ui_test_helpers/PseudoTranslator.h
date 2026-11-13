#pragma once

#include <QTranslator>

namespace JobPrep::UiTest {

/// Test-only translator that expands strings by ~40% with Cyrillic-looking text
/// while preserving placeholders (%1..%9, %n), HTML tags, and accelerators (&).
class PseudoTranslator : public QTranslator {
    Q_OBJECT

public:
    explicit PseudoTranslator(QObject* parent = nullptr);
    ~PseudoTranslator() override = default;

    bool isEmpty() const override;

    QString translate(const char* context, const char* sourceText,
                      const char* disambiguation = nullptr, int n = -1) const override;

    static QString pseudoLocalize(const QString& src, int n = -1);
};

}  // namespace JobPrep::UiTest
