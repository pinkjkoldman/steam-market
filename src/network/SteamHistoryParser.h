#pragma once

#include <QByteArray>
#include <QVector>

#include "core/errors.h"
#include "core/models/PricePoint.h"

struct SteamHistoryParseResult {
    QVector<PricePoint> points;
    AppError error;
    int invalidRows = 0;
    int duplicateRows = 0;
    bool explicitEmpty = false;
};

// Steam pricehistory 响应的纯解析器：校验业务成功、时间/价格/成交量，
// 输出按 UTC 升序且按时间去重的历史点。无网络和数据库依赖，便于 fixture 测试。
class SteamHistoryParser {
public:
    static SteamHistoryParseResult parse(const QByteArray &body);
};

