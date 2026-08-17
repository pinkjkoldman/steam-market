#pragma once

#include <QObject>
#include <QString>

// 通知服务：向 UI（托盘）转发，UI 通过 setSink 注入实现，避免 core 依赖 widgets。
class NotificationService : public QObject {
    Q_OBJECT

public:
    using Sink = std::function<void(const QString &title, const QString &body)>;

    explicit NotificationService(QObject *parent = nullptr);

    void setSink(Sink sink) { m_sink = std::move(sink); }
    void notify(const QString &title, const QString &body);

private:
    Sink m_sink;
};
