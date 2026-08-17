#pragma once

#include <QMainWindow>
#include <QStringList>

class QListWidget;
class QStackedWidget;
class QLabel;
class MarketPage;
class DetailPage;
class WatchlistPage;
class PortfolioPage;
class SettingsPage;
class RankingPage;
class RulesPage;
class InventoryAssistantPage;
class WelcomePage;
class MarketOverviewPage;
class MarketBrowserPage;
class AccountCard;

// 主窗口：左侧导航 + 页面堆栈 + 状态栏。
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(WelcomePage *welcomePage, MarketOverviewPage *overviewPage,
               MarketBrowserPage *browserPage, AccountCard *accountCard,
               MarketPage *marketPage, DetailPage *detailPage, WatchlistPage *watchlistPage,
               PortfolioPage *portfolioPage, RankingPage *rankingPage, RulesPage *rulesPage,
               InventoryAssistantPage *inventoryPage, SettingsPage *settingsPage, QWidget *parent = nullptr);

    void setStatus(const QString &network, const QString &sync);
    void setWelcomeVisible(bool visible);
    bool isWelcomeVisible() const;
    void setCloseToTray(bool enabled) { m_closeToTray = enabled; }
    bool closeToTray() const { return m_closeToTray; }
    void navigateTo(const QString &pageName);

public slots:
    void showDetail(const QString &marketHashName, int appid);

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    QListWidget *m_nav = nullptr;
    void activatePage(int index);
    void updatePageHeader(int index);

    QStackedWidget *m_experience = nullptr;
    QStackedWidget *m_stack = nullptr;
    DetailPage *m_detail = nullptr;
    MarketBrowserPage *m_browser = nullptr;
    QLabel *m_statusLabel = nullptr;
    QLabel *m_pageTitle = nullptr;
    QLabel *m_pageSubtitle = nullptr;
    QLabel *m_connectionLabel = nullptr;
    int m_detailIndex = 0;
    QStringList m_pageNames;
    QStringList m_pageTitles;
    bool m_closeToTray = true;
    int m_detailSourceIndex = 0;
};
