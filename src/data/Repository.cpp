#include "data/Repository.h"

using namespace Qt::StringLiterals;

namespace JobPrep::Data {

Repository::Repository(QObject* parent) : QObject(parent) {}

bool Repository::commitAndNotify(Database& database, Database::Transaction& transaction) {
    const bool owner = transaction.isOwner();
    if (!transaction.commit()) return false;
    // A joined write has not changed anything durable yet, so its signal waits for
    // the outer commit; otherwise every write would notify its listeners separately.
    if (owner) {
        emit changed();
    } else {
        database.deferChanged(this);
    }
    return true;
}

bool Repository::acceptExisting(Database& database, bool found, const QString& what, int id) const {
    if (found) return true;
    database.setExpectedError(u"No %1 with id %2."_s.arg(what, QString::number(id)));
    return false;
}

}  // namespace JobPrep::Data