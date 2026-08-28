#include "network/WebView2SessionHost.h"

#include <windows.h>
#include <objbase.h>

#include <atomic>

#include <QCoreApplication>
#include <QDialog>
#include <QEvent>
#include <QFileInfo>
#include <QLabel>
#include <QNetworkCookie>
#include <QStandardPaths>
#include <QUrl>
#include <QVBoxLayout>
#include <QWidget>

#include "WebView2.h"

namespace {
template <typename Interface>
class ComHandlerBase : public Interface {
public:
    virtual ~ComHandlerBase() = default;
    ULONG STDMETHODCALLTYPE AddRef() override { return ++m_references; }
    ULONG STDMETHODCALLTYPE Release() override {
        const ULONG remaining = --m_references;
        if (remaining == 0) delete this;
        return remaining;
    }

protected:
    std::atomic_ulong m_references{1};
};

bool matches(REFIID actual, REFIID expected) {
    return IsEqualIID(actual, expected) != FALSE;
}

bool hostMatches(const QString &host, const QString &suffix) {
    return host == suffix || host.endsWith(QLatin1Char('.') + suffix);
}

QString takeWideString(LPWSTR value) {
    if (!value) return {};
    const QString result = QString::fromWCharArray(value);
    CoTaskMemFree(value);
    return result;
}
}  // namespace

class EnvironmentHandler final
    : public ComHandlerBase<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler> {
public:
    explicit EnvironmentHandler(WebView2SessionHost *host) : m_host(host) {}
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID id, void **object) override {
        if (!object) return E_POINTER;
        if (matches(id, IID_IUnknown)
            || matches(id, IID_ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler)) {
            *object = this;
            AddRef();
            return S_OK;
        }
        *object = nullptr;
        return E_NOINTERFACE;
    }
    HRESULT STDMETHODCALLTYPE Invoke(HRESULT result,
                                     ICoreWebView2Environment *environment) override {
        m_host->onEnvironmentCreated(result, environment);
        return S_OK;
    }

private:
    WebView2SessionHost *m_host = nullptr;
};

class ControllerHandler final
    : public ComHandlerBase<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler> {
public:
    explicit ControllerHandler(WebView2SessionHost *host) : m_host(host) {}
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID id, void **object) override {
        if (!object) return E_POINTER;
        if (matches(id, IID_IUnknown)
            || matches(id, IID_ICoreWebView2CreateCoreWebView2ControllerCompletedHandler)) {
            *object = this;
            AddRef();
            return S_OK;
        }
        *object = nullptr;
        return E_NOINTERFACE;
    }
    HRESULT STDMETHODCALLTYPE Invoke(HRESULT result,
                                     ICoreWebView2Controller *controller) override {
        m_host->onControllerCreated(result, controller);
        return S_OK;
    }

private:
    WebView2SessionHost *m_host = nullptr;
};

class NavigationStartingHandler final
    : public ComHandlerBase<ICoreWebView2NavigationStartingEventHandler> {
public:
    explicit NavigationStartingHandler(WebView2SessionHost *host) : m_host(host) {}
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID id, void **object) override {
        if (!object) return E_POINTER;
        if (matches(id, IID_IUnknown)
            || matches(id, IID_ICoreWebView2NavigationStartingEventHandler)) {
            *object = this;
            AddRef();
            return S_OK;
        }
        *object = nullptr;
        return E_NOINTERFACE;
    }
    HRESULT STDMETHODCALLTYPE Invoke(ICoreWebView2 *,
                                     ICoreWebView2NavigationStartingEventArgs *args) override {
        LPWSTR uri = nullptr;
        if (SUCCEEDED(args->get_Uri(&uri))) {
            const QUrl url(takeWideString(uri));
            if (!m_host->isAllowedNavigation(url)) {
                args->put_Cancel(TRUE);
                emit m_host->hostError(QStringLiteral("已阻止非 Steam 官方页面：%1").arg(url.host()));
            }
        }
        return S_OK;
    }

private:
    WebView2SessionHost *m_host = nullptr;
};

class NavigationCompletedHandler final
    : public ComHandlerBase<ICoreWebView2NavigationCompletedEventHandler> {
public:
    explicit NavigationCompletedHandler(WebView2SessionHost *host) : m_host(host) {}
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID id, void **object) override {
        if (!object) return E_POINTER;
        if (matches(id, IID_IUnknown)
            || matches(id, IID_ICoreWebView2NavigationCompletedEventHandler)) {
            *object = this;
            AddRef();
            return S_OK;
        }
        *object = nullptr;
        return E_NOINTERFACE;
    }
    HRESULT STDMETHODCALLTYPE Invoke(ICoreWebView2 *,
                                     ICoreWebView2NavigationCompletedEventArgs *args) override {
        BOOL success = FALSE;
        COREWEBVIEW2_WEB_ERROR_STATUS errorStatus = COREWEBVIEW2_WEB_ERROR_STATUS_UNKNOWN;
        if (args) {
            args->get_IsSuccess(&success);
            args->get_WebErrorStatus(&errorStatus);
        }
        m_host->onNavigationCompleted(success != FALSE, static_cast<long>(errorStatus));
        return S_OK;
    }

private:
    WebView2SessionHost *m_host = nullptr;
};

class ClearBrowsingDataHandler final
    : public ComHandlerBase<ICoreWebView2ClearBrowsingDataCompletedHandler> {
public:
    explicit ClearBrowsingDataHandler(WebView2SessionHost *host) : m_host(host) {}
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID id, void **object) override {
        if (!object) return E_POINTER;
        if (matches(id, IID_IUnknown)
            || matches(id, IID_ICoreWebView2ClearBrowsingDataCompletedHandler)) {
            *object = this;
            AddRef();
            return S_OK;
        }
        *object = nullptr;
        return E_NOINTERFACE;
    }
    HRESULT STDMETHODCALLTYPE Invoke(HRESULT result) override {
        m_host->finishFreshSessionPreparation(
            SUCCEEDED(result) ? FreshSessionResult::Ready : FreshSessionResult::ClearFailed);
        return S_OK;
    }
private:
    WebView2SessionHost *m_host = nullptr;
};
class CookiesHandler final : public ComHandlerBase<ICoreWebView2GetCookiesCompletedHandler> {
public:
    explicit CookiesHandler(WebView2SessionHost *host) : m_host(host) {}
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID id, void **object) override {
        if (!object) return E_POINTER;
        if (matches(id, IID_IUnknown) || matches(id, IID_ICoreWebView2GetCookiesCompletedHandler)) {
            *object = this;
            AddRef();
            return S_OK;
        }
        *object = nullptr;
        return E_NOINTERFACE;
    }
    HRESULT STDMETHODCALLTYPE Invoke(HRESULT result, ICoreWebView2CookieList *cookies) override {
        if (FAILED(result) || !cookies) return S_OK;
        UINT32 count = 0;
        cookies->get_Count(&count);
        QList<QNetworkCookie> networkCookies;
        QString steamId;
        for (UINT32 index = 0; index < count; ++index) {
            ICoreWebView2Cookie *cookie = nullptr;
            if (FAILED(cookies->GetValueAtIndex(index, &cookie)) || !cookie) continue;
            LPWSTR name = nullptr;
            LPWSTR value = nullptr;
            LPWSTR domain = nullptr;
            LPWSTR path = nullptr;
            cookie->get_Name(&name);
            cookie->get_Value(&value);
            cookie->get_Domain(&domain);
            cookie->get_Path(&path);
            const QString cookieName = takeWideString(name);
            const QString cookieValue = takeWideString(value);
            QNetworkCookie networkCookie(cookieName.toUtf8(), cookieValue.toUtf8());
            networkCookie.setDomain(takeWideString(domain));
            networkCookie.setPath(takeWideString(path));
            BOOL secure = FALSE;
            BOOL httpOnly = FALSE;
            cookie->get_IsSecure(&secure);
            cookie->get_IsHttpOnly(&httpOnly);
            networkCookie.setSecure(secure != FALSE);
            networkCookie.setHttpOnly(httpOnly != FALSE);
            networkCookies.append(networkCookie);
            if (cookieName == QLatin1String("steamLoginSecure")) {
                steamId = QUrl::fromPercentEncoding(cookieValue.toUtf8()).section(
                    QLatin1String("||"), 0, 0);
            }
            cookie->Release();
        }
        if (!networkCookies.isEmpty()) {
            emit m_host->sessionCookiesReady(steamId, networkCookies);
        }
        return S_OK;
    }

private:
    WebView2SessionHost *m_host = nullptr;
};

WebView2SessionHost::WebView2SessionHost(QObject *parent) : IWebSessionHost(parent) {
    m_dialog = new QDialog();
    m_dialog->setAttribute(Qt::WA_DeleteOnClose, false);
    m_dialog->setWindowTitle(QStringLiteral("Steam 官方登录"));
    m_dialog->resize(1000, 720);
    auto *layout = new QVBoxLayout(m_dialog);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    m_status = new QLabel(m_dialog);
    m_status->setAlignment(Qt::AlignCenter);
    m_status->setWordWrap(true);
    m_status->setMinimumHeight(48);
    m_status->setStyleSheet(QStringLiteral(
        "QLabel { background: #121722; color: #dce6f2; padding: 12px 18px; "
        "font-size: 13px; border-bottom: 1px solid #293244; }"));
    m_status->hide();
    layout->addWidget(m_status);
    m_surface = new QWidget(m_dialog);
    m_surface->setAttribute(Qt::WA_NativeWindow);
    m_surface->installEventFilter(this);
    layout->addWidget(m_surface);
    connect(m_dialog, &QDialog::finished, this, [this](int) {
        if (m_freshSessionPreparing) {
            finishFreshSessionPreparation(FreshSessionResult::ClearFailed);
        }
        emit surfaceClosedByUser();
    });
}

WebView2SessionHost::~WebView2SessionHost() {
    if (m_webView) m_webView->Release();
    if (m_controller) m_controller->Release();
    if (m_environment) m_environment->Release();
    delete m_dialog;
    if (m_comInitialized) CoUninitialize();
}

void WebView2SessionHost::showLogin() {
    // A prepared clean profile is consumed by this login attempt. The next
    // attempt must clear cookies again, even if the user closes this window.
    m_freshSessionPrepared = false;
    m_pendingUrl = QUrl(QStringLiteral("https://steamcommunity.com/login/home/"));
    showSurfaceMessage(QStringLiteral("正在打开 Steam 官方登录页面…"));
    m_dialog->show();
    m_dialog->raise();
    m_dialog->activateWindow();
    if (ensureInitialized()) navigatePending();
}

void WebView2SessionHost::dismissSurface() {
    if (m_dialog) m_dialog->hide();
}

void WebView2SessionHost::openOfficialUrl(const QUrl &url) {
    if (!isAllowedNavigation(url)) {
        emit hostError(QStringLiteral("只允许打开 Steam 官方页面"));
        return;
    }
    m_pendingUrl = url;
    showSurfaceMessage(QStringLiteral("正在打开 Steam 官方页面…"));
    m_dialog->show();
    if (ensureInitialized()) navigatePending();
}

void WebView2SessionHost::clearSession() {
    m_freshSessionPrepared = false;
    if (!m_webView) return;
    ICoreWebView2_2 *webView2 = nullptr;
    if (FAILED(m_webView->QueryInterface(IID_ICoreWebView2_2,
                                         reinterpret_cast<void **>(&webView2)))
        || !webView2) return;
    ICoreWebView2CookieManager *manager = nullptr;
    if (SUCCEEDED(webView2->get_CookieManager(&manager)) && manager) {
        manager->DeleteAllCookies();
        manager->Release();
    }
    webView2->Release();
}

void WebView2SessionHost::prepareFreshLoginSession(
    std::function<void(FreshSessionResult)> completion) {
    if (!completion) return;
    if (m_freshSessionPrepared) {
        QMetaObject::invokeMethod(this, [completion = std::move(completion)]() mutable {
            completion(FreshSessionResult::Ready);
        }, Qt::QueuedConnection);
        return;
    }
    if (m_freshSessionPreparing) {
        QMetaObject::invokeMethod(this, [completion = std::move(completion)]() mutable {
            completion(FreshSessionResult::ClearFailed);
        }, Qt::QueuedConnection);
        return;
    }
    m_freshSessionPreparing = true;
    m_freshSessionCompletion = std::move(completion);
    m_pendingUrl = QUrl(QStringLiteral("https://steamcommunity.com/login/home/"));
    showSurfaceMessage(QStringLiteral("正在准备安全的 Steam 官方登录环境…"));
    m_dialog->show();
    m_dialog->raise();
    m_dialog->activateWindow();
    if (ensureInitialized()) continueFreshSessionPreparation();
}

void WebView2SessionHost::continueFreshSessionPreparation() {
    if (!m_freshSessionPreparing || !m_webView) return;
    ICoreWebView2_13 *webView13 = nullptr;
    if (FAILED(m_webView->QueryInterface(IID_ICoreWebView2_13,
                                         reinterpret_cast<void **>(&webView13)))
        || !webView13) {
        finishFreshSessionPreparation(FreshSessionResult::Unavailable);
        return;
    }
    ICoreWebView2Profile *profile = nullptr;
    const HRESULT profileResult = webView13->get_Profile(&profile);
    webView13->Release();
    if (FAILED(profileResult) || !profile) {
        finishFreshSessionPreparation(FreshSessionResult::Unavailable);
        return;
    }
    ICoreWebView2Profile2 *profile2 = nullptr;
    const HRESULT interfaceResult = profile->QueryInterface(
        IID_ICoreWebView2Profile2, reinterpret_cast<void **>(&profile2));
    profile->Release();
    if (FAILED(interfaceResult) || !profile2) {
        finishFreshSessionPreparation(FreshSessionResult::Unavailable);
        return;
    }
    auto *handler = new ClearBrowsingDataHandler(this);
    const HRESULT clearResult = profile2->ClearBrowsingData(
        COREWEBVIEW2_BROWSING_DATA_KINDS_COOKIES, handler);
    handler->Release();
    profile2->Release();
    if (FAILED(clearResult)) finishFreshSessionPreparation(FreshSessionResult::ClearFailed);
}

void WebView2SessionHost::finishFreshSessionPreparation(FreshSessionResult result) {
    if (!m_freshSessionPreparing) return;
    m_freshSessionPreparing = false;
    m_freshSessionPrepared = result == FreshSessionResult::Ready;
    auto completion = std::move(m_freshSessionCompletion);
    m_freshSessionCompletion = {};
    QMetaObject::invokeMethod(this, [completion = std::move(completion), result]() mutable {
        if (completion) completion(result);
    }, Qt::QueuedConnection);
}
bool WebView2SessionHost::eventFilter(QObject *watched, QEvent *event) {
    if (watched == m_surface && event->type() == QEvent::Resize) updateBounds();
    return IWebSessionHost::eventFilter(watched, event);
}

bool WebView2SessionHost::ensureInitialized() {
    if (m_webView) return true;
    if (m_initializing) return false;
    const QString loaderPath = QCoreApplication::applicationDirPath()
                               + QStringLiteral("/WebView2Loader.dll");
    if (!QFileInfo::exists(loaderPath)) {
        emit hostError(QStringLiteral("缺少 WebView2Loader.dll，无法打开官方登录"));
        finishFreshSessionPreparation(FreshSessionResult::Unavailable);
        return false;
    }
    m_loader.setFileName(loaderPath);
    if (!m_loader.load()) {
        emit hostError(QStringLiteral("无法加载 WebView2Loader.dll：%1").arg(m_loader.errorString()));
        finishFreshSessionPreparation(FreshSessionResult::Unavailable);
        return false;
    }
    using CreateEnvironment = HRESULT(STDAPICALLTYPE *)(
        PCWSTR, PCWSTR, ICoreWebView2EnvironmentOptions *,
        ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler *);
    auto createEnvironment = reinterpret_cast<CreateEnvironment>(
        m_loader.resolve("CreateCoreWebView2EnvironmentWithOptions"));
    if (!createEnvironment) {
        emit hostError(QStringLiteral("WebView2 Loader 接口不可用"));
        finishFreshSessionPreparation(FreshSessionResult::Unavailable);
        return false;
    }
    const HRESULT comResult = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (FAILED(comResult) && comResult != RPC_E_CHANGED_MODE) {
        emit hostError(QStringLiteral("无法初始化 WebView2 COM 环境"));
        finishFreshSessionPreparation(FreshSessionResult::Unavailable);
        return false;
    }
    m_comInitialized = SUCCEEDED(comResult);
    const QString userData = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation)
                             + QStringLiteral("/WebView2");
    m_initializing = true;
    auto *environmentHandler = new EnvironmentHandler(this);
    const HRESULT result = createEnvironment(nullptr,
                                             reinterpret_cast<PCWSTR>(userData.utf16()),
                                             nullptr, environmentHandler);
    environmentHandler->Release();
    if (FAILED(result)) {
        m_initializing = false;
        emit hostError(QStringLiteral("创建 WebView2 环境失败：0x%1")
                           .arg(static_cast<quint32>(result), 8, 16, QLatin1Char('0')));
        finishFreshSessionPreparation(FreshSessionResult::Unavailable);
    }
    return false;
}

void WebView2SessionHost::onEnvironmentCreated(long result,
                                               ICoreWebView2Environment *environment) {
    if (FAILED(result) || !environment) {
        m_initializing = false;
        emit hostError(QStringLiteral("WebView2 Runtime 不可用"));
        finishFreshSessionPreparation(FreshSessionResult::Unavailable);
        return;
    }
    m_environment = environment;
    m_environment->AddRef();
    const HWND window = reinterpret_cast<HWND>(m_surface->winId());
    auto *controllerHandler = new ControllerHandler(this);
    const HRESULT controllerResult =
        m_environment->CreateCoreWebView2Controller(window, controllerHandler);
    controllerHandler->Release();
    if (FAILED(controllerResult)) onControllerCreated(controllerResult, nullptr);
}

void WebView2SessionHost::onControllerCreated(long result,
                                              ICoreWebView2Controller *controller) {
    m_initializing = false;
    if (FAILED(result) || !controller) {
        emit hostError(QStringLiteral("创建 Steam 登录窗口失败"));
        finishFreshSessionPreparation(FreshSessionResult::Unavailable);
        return;
    }
    m_controller = controller;
    m_controller->AddRef();
    if (FAILED(m_controller->get_CoreWebView2(&m_webView)) || !m_webView) {
        emit hostError(QStringLiteral("WebView2 页面控制器不可用"));
        finishFreshSessionPreparation(FreshSessionResult::Unavailable);
        return;
    }
    ICoreWebView2Settings *settings = nullptr;
    if (SUCCEEDED(m_webView->get_Settings(&settings)) && settings) {
        settings->put_AreDevToolsEnabled(FALSE);
        settings->put_AreDefaultContextMenusEnabled(FALSE);
        settings->put_IsStatusBarEnabled(FALSE);
        settings->Release();
    }
    EventRegistrationToken token{};
    auto *startingHandler = new NavigationStartingHandler(this);
    m_webView->add_NavigationStarting(startingHandler, &token);
    startingHandler->Release();
    auto *completedHandler = new NavigationCompletedHandler(this);
    m_webView->add_NavigationCompleted(completedHandler, &token);
    completedHandler->Release();
    updateBounds();
    if (m_freshSessionPreparing) continueFreshSessionPreparation();
    else navigatePending();
}

void WebView2SessionHost::navigatePending() {
    if (!m_webView || !m_pendingUrl.isValid()) return;
    const QString url = m_pendingUrl.toString(QUrl::FullyEncoded);
    m_webView->Navigate(reinterpret_cast<LPCWSTR>(url.utf16()));
}

void WebView2SessionHost::updateBounds() {
    if (!m_controller || !m_surface) return;
    const QSize size = m_surface->size();
    const RECT bounds{0, 0, size.width(), size.height()};
    m_controller->put_Bounds(bounds);
}

void WebView2SessionHost::onNavigationCompleted(bool success, long webErrorStatus) {
    if (!success) {
        if (webErrorStatus != COREWEBVIEW2_WEB_ERROR_STATUS_OPERATION_CANCELED) {
            showSurfaceMessage(QStringLiteral(
                "Steam 页面加载失败（错误 %1）。请检查网络后关闭窗口并重试。")
                                   .arg(webErrorStatus));
        }
        return;
    }
    if (m_status) m_status->hide();
    if (!m_webView) return;
    LPWSTR source = nullptr;
    if (SUCCEEDED(m_webView->get_Source(&source))) {
        const QUrl url(takeWideString(source));
        if (hostMatches(url.host().toLower(), QStringLiteral("steamcommunity.com"))) {
            requestCookies();
        }
    }
}

void WebView2SessionHost::showSurfaceMessage(const QString &message) {
    if (!m_status) return;
    m_status->setText(message);
    m_status->show();
}

void WebView2SessionHost::requestCookies() {
    ICoreWebView2_2 *webView2 = nullptr;
    if (FAILED(m_webView->QueryInterface(IID_ICoreWebView2_2,
                                         reinterpret_cast<void **>(&webView2)))
        || !webView2) return;
    ICoreWebView2CookieManager *manager = nullptr;
    if (SUCCEEDED(webView2->get_CookieManager(&manager)) && manager) {
        auto *cookiesHandler = new CookiesHandler(this);
        manager->GetCookies(L"https://steamcommunity.com", cookiesHandler);
        cookiesHandler->Release();
        manager->Release();
    }
    webView2->Release();
}

bool WebView2SessionHost::isAllowedNavigation(const QUrl &url) const {
    if (url.scheme() != QLatin1String("https")) return false;
    const QString host = url.host().toLower();
    return hostMatches(host, QStringLiteral("steamcommunity.com"))
           || hostMatches(host, QStringLiteral("steampowered.com"))
           || hostMatches(host, QStringLiteral("steamstatic.com"))
           || hostMatches(host, QStringLiteral("akamaihd.net"));
}
