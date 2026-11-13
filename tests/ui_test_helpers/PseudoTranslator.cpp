#include "PseudoTranslator.h"

#include <QRegularExpression>
#include <QString>

using namespace Qt::StringLiterals;

namespace JobPrep::UiTest {

PseudoTranslator::PseudoTranslator(QObject* parent) : QTranslator(parent) {}

bool PseudoTranslator::isEmpty() const {
    return false;
}

QString PseudoTranslator::translate(const char* context, const char* sourceText,
                                    const char* disambiguation, int n) const {
    Q_UNUSED(context);
    Q_UNUSED(disambiguation);
    if (!sourceText || sourceText[0] == '\0') {
        return QString();
    }
    return pseudoLocalize(QString::fromUtf8(sourceText), n);
}

QString PseudoTranslator::pseudoLocalize(const QString& src, int n) {
    Q_UNUSED(n);
    if (src.isEmpty()) return src;

    auto toCyrillic = [](QChar c) -> QChar {
        switch (c.toLatin1()) {
            case 'a': return u'а';
            case 'b': return u'б';
            case 'c': return u'с';
            case 'd': return u'д';
            case 'e': return u'е';
            case 'f': return u'ф';
            case 'g': return u'г';
            case 'h': return u'х';
            case 'i': return u'і';
            case 'j': return u'ј';
            case 'k': return u'к';
            case 'l': return u'л';
            case 'm': return u'м';
            case 'n': return u'н';
            case 'o': return u'о';
            case 'p': return u'р';
            case 'r': return u'р';
            case 's': return u'с';
            case 't': return u'т';
            case 'u': return u'у';
            case 'v': return u'в';
            case 'w': return u'ш';
            case 'x': return u'х';
            case 'y': return u'у';
            case 'z': return u'з';
            case 'A': return u'А';
            case 'B': return u'В';
            case 'C': return u'С';
            case 'E': return u'Е';
            case 'H': return u'Н';
            case 'K': return u'К';
            case 'M': return u'М';
            case 'O': return u'О';
            case 'P': return u'Р';
            case 'T': return u'Т';
            case 'X': return u'Х';
            case 'Y': return u'У';
            default: return c;
        }
    };

    static const QRegularExpression tokenRe(uR"((<[^>]+>|%[0-9n]|&[A-Za-z]|.))"_s);
    auto it = tokenRe.globalMatch(src);
    QString transformed;
    transformed.reserve(src.size() * 2);

    int normalCharCount = 0;
    while (it.hasNext()) {
        const auto match = it.next();
        const QString token = match.captured(0);
        if (token.startsWith(u'<') && token.endsWith(u'>')) {
            transformed += token;
        } else if (token.size() == 2 && token.startsWith(u'%')) {
            transformed += token;
        } else if (token.size() == 2 && token.startsWith(u'&')) {
            transformed += token[0];
            transformed += toCyrillic(token[1]);
            normalCharCount++;
        } else {
            const QChar c = token[0];
            transformed += toCyrillic(c);
            if (c.isLetter()) normalCharCount++;
        }
    }

    if (normalCharCount == 0) {
        return transformed;
    }

    const int expansionChars = qMax(1, qRound(normalCharCount * 0.40));
    static const QString padChars = u"абвгдежзийклмнопрстуфхцчшщ"_s;
    QString padding;
    padding.reserve(expansionChars + 1);
    padding += u'~';
    for (int i = 0; i < expansionChars; ++i) {
        padding += padChars[i % padChars.size()];
    }

    return transformed + padding;
}

}  // namespace JobPrep::UiTest
