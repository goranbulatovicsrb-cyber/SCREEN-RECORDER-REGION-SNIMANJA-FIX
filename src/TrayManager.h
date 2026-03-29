#pragma once
#include <QObject>
#include <QSystemTrayIcon>
class QMenu;
class MainWindow;

class TrayManager : public QObject {
    Q_OBJECT
public:
    explicit TrayManager(QObject *parent = nullptr);
    void setup(MainWindow *window);
    void showMessage(const QString &title, const QString &msg);

signals:
    void activated(QSystemTrayIcon::ActivationReason reason);
    void fullScreenRequested();
    void regionRequested();
    void showWindowRequested();

private:
    QSystemTrayIcon *m_tray;
    QMenu           *m_menu;
};
