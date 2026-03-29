#include "TrayManager.h"
#include "MainWindow.h"
#include <QMenu>
#include <QApplication>
#include <QSystemTrayIcon>

TrayManager::TrayManager(QObject *parent)
    : QObject(parent)
    , m_tray(nullptr)
    , m_menu(nullptr)
{}

void TrayManager::setup(MainWindow *window) {
    if (!QSystemTrayIcon::isSystemTrayAvailable()) return;

    m_tray = new QSystemTrayIcon(QIcon(":/icons/app_icon.png"), this);
    m_tray->setToolTip("ScreenMaster Pro");

    m_menu = new QMenu();
    m_menu->setStyleSheet(R"(
        QMenu { background: #1a1a2e; color: #e0e0e0; border: 1px solid #404060;
                border-radius: 6px; padding: 4px; }
        QMenu::item { padding: 8px 20px; border-radius: 4px; }
        QMenu::item:selected { background: #00aeff; color: white; }
        QMenu::separator { height: 1px; background: #404060; margin: 4px 8px; }
    )");

    QAction *showAction     = m_menu->addAction("📸  ScreenMaster Pro");
    showAction->setFont(QFont("", -1, QFont::Bold));
    m_menu->addSeparator();
    QAction *fullScAction   = m_menu->addAction("🖥️  Full Screen Capture");
    QAction *regionAction   = m_menu->addAction("✂️  Region Capture");
    QAction *windowAction   = m_menu->addAction("🪟  Window Capture");
    m_menu->addSeparator();
    QAction *recordAction   = m_menu->addAction("🎬  Start Recording");
    m_menu->addSeparator();
    QAction *openAction     = m_menu->addAction("🪟  Open Window");
    QAction *settingsAction = m_menu->addAction("⚙️  Settings");
    m_menu->addSeparator();
    QAction *quitAction     = m_menu->addAction("✖  Quit");

    connect(showAction,  &QAction::triggered, window, [window]() {
        window->show(); window->raise(); window->activateWindow(); });
    connect(fullScAction, &QAction::triggered, window, &MainWindow::onFullScreenCapture);
    connect(regionAction, &QAction::triggered, window, &MainWindow::onRegionCapture);
    connect(windowAction, &QAction::triggered, window, &MainWindow::onWindowCapture);
    connect(openAction,  &QAction::triggered, window, [window]() {
        window->show(); window->raise(); window->activateWindow(); });
    connect(settingsAction, &QAction::triggered, window, [window]() {
        window->onNavigate(3); window->show(); window->raise(); });
    connect(quitAction, &QAction::triggered, qApp, &QApplication::quit);

    connect(m_tray, &QSystemTrayIcon::activated,
            this, &TrayManager::activated);

    m_tray->setContextMenu(m_menu);
    m_tray->show();
}

void TrayManager::showMessage(const QString &title, const QString &msg) {
    if (m_tray && m_tray->isVisible()) {
        m_tray->showMessage(title, msg, QSystemTrayIcon::Information, 3000);
    }
}
