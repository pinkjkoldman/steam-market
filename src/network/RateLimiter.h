#pragma once

#include <QElapsedTimer>

// 简单限速器：保证两次请求间隔不小于 minIntervalMs，超出 5 分钟视为 RATE_LIMITED。
class RateLimiter {
public:
    explicit RateLimiter(qint64 minIntervalMs) : m_minIntervalMs(minIntervalMs), m_first(true) {
        m_last.start();
    }

    qint64 waitMs() const {
        if (m_first) return 0;
        const qint64 elapsed = m_last.elapsed();
        const qint64 remain = m_minIntervalMs - elapsed;
        return remain > 0 ? remain : 0;
    }

    void tick() {
        m_first = false;
        m_last.restart();
    }

    void setMinIntervalMs(qint64 ms) {
        m_minIntervalMs = ms;
    }

private:
    qint64 m_minIntervalMs;
    QElapsedTimer m_last;
    bool m_first;
};
