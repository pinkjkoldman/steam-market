#include "network/SteamHistoryParser.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocale>
#include <QMap>
#include <QRegularExpression>
#include <QTimeZone>
#include <QtGlobal>

#include <cmath>
#include <limits>

namespace {
QDateTime parseHistoryDate(const QString &value) {
    const QLocale english(QLocale::English, QLocale::UnitedStates);
    QString text = value.trimmed();

    int offsetSeconds = 0;
    static const QRegularExpression offsetPattern(QStringLiteral("\\s+([+-])(\\d{1,2})$"));
    const QRegularExpressionMatch offset = offsetPattern.match(text);
    if (offset.hasMatch()) {
        const int hours = offset.captured(2).toInt();
        offsetSeconds = (offset.captured(1) == QStringLiteral("-") ? -1 : 1) * hours * 3600;
        text.truncate(offset.capturedStart());
    }
    if (text.endsWith(QLatin1Char(':'))) text.append(QStringLiteral("00"));

    QDateTime parsed = english.toDateTime(text, QStringLiteral("MMM dd yyyy HH:mm"));
    if (!parsed.isValid()) {
        parsed = english.toDateTime(text, QStringLiteral("MMM d yyyy HH:mm"));
    }
    if (!parsed.isValid()) parsed = QDateTime::fromString(text, Qt::ISODate);
    if (!parsed.isValid()) return {};

    // QLocale creates a local-time QDateTime. Reconstruct it with Steam's explicit
    // offset so the clock fields are interpreted in that zone instead of converted
    // from the machine's local zone first.
    const QDateTime zoned(parsed.date(), parsed.time(),
                          QTimeZone::fromSecondsAheadOfUtc(offsetSeconds));
    return zoned.toUTC();
}

bool parsePrice(const QJsonValue &value, double *out) {
    bool ok = false;
    double price = 0.0;
    if (value.isDouble()) {
        price = value.toDouble();
        ok = true;
    } else if (value.isString()) {
        price = value.toString().trimmed().toDouble(&ok);
    }
    if (!ok || !std::isfinite(price) || price <= 0.0) return false;
    *out = price;
    return true;
}

bool parseVolume(const QJsonValue &value, int *out) {
    bool ok = false;
    qlonglong volume = 0;
    if (value.isDouble()) {
        const double raw = value.toDouble();
        if (!std::isfinite(raw) || raw < 0.0 || std::floor(raw) != raw) return false;
        volume = static_cast<qlonglong>(raw);
        ok = true;
    } else if (value.isString()) {
        QString text = value.toString().trimmed();
        text.remove(QLatin1Char(','));
        volume = text.toLongLong(&ok);
    }
    if (!ok || volume < 0 || volume > std::numeric_limits<int>::max()) return false;
    *out = static_cast<int>(volume);
    return true;
}
}  // namespace

SteamHistoryParseResult SteamHistoryParser::parse(const QByteArray &body) {
    SteamHistoryParseResult result;
    if (body.size() > 8 * 1024 * 1024) {
        result.error = AppError::make(ErrorCode::kSourceInvalid,
                                      QStringLiteral("Steam 历史响应过大，已停止解析"));
        return result;
    }

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(body, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        result.error = AppError::make(ErrorCode::kSourceInvalid,
                                      QStringLiteral("Steam 历史响应不是有效 JSON"));
        return result;
    }

    const QJsonObject root = document.object();
    if (!root.value(QStringLiteral("success")).isBool()
        || !root.value(QStringLiteral("success")).toBool()) {
        result.error = AppError::make(ErrorCode::kAuthenticationRequired,
                                      QStringLiteral("Steam 官方历史需要有效登录会话"));
        return result;
    }
    if (!root.contains(QStringLiteral("prices"))
        || !root.value(QStringLiteral("prices")).isArray()) {
        result.error = AppError::make(ErrorCode::kSourceInvalid,
                                      QStringLiteral("Steam 历史数据格式已变化"));
        return result;
    }

    const QJsonArray rows = root.value(QStringLiteral("prices")).toArray();
    result.explicitEmpty = rows.isEmpty();
    QMap<qint64, PricePoint> ordered;
    for (const QJsonValue &entry : rows) {
        if (!entry.isArray()) {
            ++result.invalidRows;
            continue;
        }
        const QJsonArray row = entry.toArray();
        if (row.size() < 2 || !row.at(0).isString()) {
            ++result.invalidRows;
            continue;
        }

        PricePoint point;
        point.recordedAt = parseHistoryDate(row.at(0).toString());
        if (!point.recordedAt.isValid() || !parsePrice(row.at(1), &point.price)) {
            ++result.invalidRows;
            continue;
        }
        if (row.size() > 2 && !row.at(2).isNull()) {
            if (!parseVolume(row.at(2), &point.volume)) {
                ++result.invalidRows;
                continue;
            }
            point.hasVolume = true;
        }

        const qint64 key = point.recordedAt.toMSecsSinceEpoch();
        if (ordered.contains(key)) ++result.duplicateRows;
        ordered.insert(key, point);
    }

    if (!rows.isEmpty() && result.invalidRows * 20 > rows.size()) {
        result.error = AppError::make(ErrorCode::kSourceInvalid,
                                      QStringLiteral("Steam 历史数据有效行不足 95%"));
        return result;
    }
    result.points.reserve(ordered.size());
    for (auto it = ordered.cbegin(); it != ordered.cend(); ++it) result.points.append(it.value());
    result.error = AppError::ok();
    return result;
}
