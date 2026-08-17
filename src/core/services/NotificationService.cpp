#include "core/services/NotificationService.h"

NotificationService::NotificationService(QObject *parent) : QObject(parent) {}

void NotificationService::notify(const QString &title, const QString &body) {
    if (m_sink) {
        m_sink(title, body);
    }
}
