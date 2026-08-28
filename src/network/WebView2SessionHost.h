#pragma once

#include <QLibrary>

#include "network/IWebSessionHost.h"

struct ICoreWebView2;
struct ICoreWebView2Controller;
struct ICoreWebView2Environment;
class QDialog;
class QLabel;
class QWidget;

class EnvironmentHandler;
class ControllerHandler;
class NavigationStartingHandler;
class NavigationCompletedHandler;
class CookiesHandler;

class WebView2SessionHost : public IWebSessionHost {
    Q_OBJECT

public:
    explicit WebView2SessionHost(QObject *parent = nullptr);
    ~WebView2SessionHost() override;

    void showLogin() override;
    void dismissSurface() override;
    void openOfficialUrl(const QUrl &url) override;
    void clearSession() override;
    void prepareFreshLoginSession(
        std::function<void(FreshSessionResult)> completion) override;

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    friend class EnvironmentHandler;
    friend class ControllerHandler;
    friend class NavigationStartingHandler;
    friend class NavigationCompletedHandler;
    friend class CookiesHandler;
    friend class ClearBrowsingDataHandler;

    bool ensureInitialized();
    void navigatePending();
    void updateBounds();
    void requestCookies();
    void continueFreshSessionPreparation();
    void finishFreshSessionPreparation(FreshSessionResult result);
    void onEnvironmentCreated(long result, ICoreWebView2Environment *environment);
    void onControllerCreated(long result, ICoreWebView2Controller *controller);
    void onNavigationCompleted(bool success, long webErrorStatus);
    void showSurfaceMessage(const QString &message);
    bool isAllowedNavigation(const QUrl &url) const;

    QDialog *m_dialog = nullptr;
    QLabel *m_status = nullptr;
    QWidget *m_surface = nullptr;
    QLibrary m_loader;
    ICoreWebView2Environment *m_environment = nullptr;
    ICoreWebView2Controller *m_controller = nullptr;
    ICoreWebView2 *m_webView = nullptr;
    QUrl m_pendingUrl;
    bool m_initializing = false;
    bool m_comInitialized = false;
    bool m_freshSessionPrepared = false;
    bool m_freshSessionPreparing = false;
    std::function<void(FreshSessionResult)> m_freshSessionCompletion;
};
