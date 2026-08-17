#include "data/repositories/PortfolioRepository.h"

#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>
#include <QtDebug>

PortfolioRepository::PortfolioRepository(QSqlDatabase &db) : m_db(db) {}

QVector<PortfolioItem> PortfolioRepository::all() const {
    QVector<PortfolioItem> out;
    QSqlQuery q(m_db);
    if (!q.exec(QStringLiteral(
            "SELECT id, market_hash_name, appid, quantity, purchase_price, purchase_currency, "
            " purchase_date, note FROM portfolio_items ORDER BY id"))) {
        qWarning() << "读取持仓失败:" << q.lastError().text();
        return out;
    }
    while (q.next()) {
        PortfolioItem it;
        it.id = q.value(0).toInt();
        it.marketHashName = q.value(1).toString();
        it.appid = q.value(2).toInt();
        it.quantity = q.value(3).toInt();
        it.purchasePrice = q.value(4).isNull() ? -1.0 : q.value(4).toDouble();
        it.purchaseCurrency = q.value(5).toString();
        const QString d = q.value(6).toString();
        if (!d.isEmpty()) it.purchaseDate = QDate::fromString(d, Qt::ISODate);
        it.note = q.value(7).toString();
        out.append(it);
    }
    return out;
}

bool PortfolioRepository::add(const PortfolioItem &item) {
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral(
        "INSERT INTO portfolio_items (market_hash_name, appid, quantity, purchase_price, "
        " purchase_currency, purchase_date, note, created_at, updated_at) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, datetime('now'), datetime('now'))"));
    q.addBindValue(item.marketHashName);
    q.addBindValue(item.appid);
    q.addBindValue(item.quantity);
    q.addBindValue(item.purchasePrice > 0 ? QVariant(item.purchasePrice) : QVariant());
    q.addBindValue(item.purchaseCurrency);
    q.addBindValue(item.purchaseDate.isValid() ? QVariant(item.purchaseDate.toString(Qt::ISODate))
                                               : QVariant());
    q.addBindValue(item.note.isEmpty() ? QVariant() : QVariant(item.note));
    if (!q.exec()) {
        qWarning() << "添加持仓失败:" << q.lastError().text();
        return false;
    }
    return true;
}

bool PortfolioRepository::update(const PortfolioItem &item) {
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral(
        "UPDATE portfolio_items SET market_hash_name=?, appid=?, quantity=?, purchase_price=?, "
        " purchase_currency=?, purchase_date=?, note=?, updated_at=datetime('now') WHERE id=?"));
    q.addBindValue(item.marketHashName);
    q.addBindValue(item.appid);
    q.addBindValue(item.quantity);
    q.addBindValue(item.purchasePrice > 0 ? QVariant(item.purchasePrice) : QVariant());
    q.addBindValue(item.purchaseCurrency);
    q.addBindValue(item.purchaseDate.isValid() ? QVariant(item.purchaseDate.toString(Qt::ISODate))
                                               : QVariant());
    q.addBindValue(item.note.isEmpty() ? QVariant() : QVariant(item.note));
    q.addBindValue(item.id);
    if (!q.exec()) {
        qWarning() << "更新持仓失败:" << q.lastError().text();
        return false;
    }
    return q.numRowsAffected() > 0;
}

bool PortfolioRepository::remove(int id) {
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("DELETE FROM portfolio_items WHERE id = ?"));
    q.addBindValue(id);
    return q.exec() && q.numRowsAffected() > 0;
}

QStringList PortfolioRepository::marketHashNames() const {
    QStringList out;
    QSqlQuery q(m_db);
    if (q.exec(QStringLiteral("SELECT DISTINCT market_hash_name FROM portfolio_items"))) {
        while (q.next()) out << q.value(0).toString();
    }
    return out;
}
