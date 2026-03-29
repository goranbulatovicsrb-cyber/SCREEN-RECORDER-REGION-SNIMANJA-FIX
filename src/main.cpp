#include <QApplication>
#include <QFile>
#include <QFontDatabase>
#include <QDir>
#include <QSettings>
#include <QSplashScreen>
#include <QTimer>
#include <QPainter>
#include "MainWindow.h"

int main(int argc, char *argv[])
{
    QApplication::setHighDpiScaleFactorRoundingPolicy(
        Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);

    QApplication app(argc, argv);
    app.setApplicationName("ScreenMaster Pro");
    app.setApplicationDisplayName("ScreenMaster Pro");
    app.setApplicationVersion("2.0.0");
    app.setOrganizationName("ScreenMasterPro");
    app.setOrganizationDomain("screenmasterpro.app");

    // Load stylesheet
    QFile styleFile(":/styles/dark.qss");
    if (styleFile.open(QFile::ReadOnly | QFile::Text)) {
        app.setStyleSheet(QString::fromUtf8(styleFile.readAll()));
        styleFile.close();
    }

    // Splash screen
    QPixmap splashPx(520, 300);
    splashPx.fill(QColor(15, 15, 30));
    {
        QPainter sp(&splashPx);
        sp.setRenderHint(QPainter::Antialiasing);

        // Gradient background
        QLinearGradient grad(0,0,520,300);
        grad.setColorAt(0, QColor(15,15,30));
        grad.setColorAt(1, QColor(25,25,60));
        sp.fillRect(splashPx.rect(), grad);

        // Accent line at top
        sp.fillRect(0,0,520,3, QColor(0,174,255));

        // Icon area
        sp.setBrush(QColor(0,174,255,30));
        sp.setPen(QPen(QColor(0,174,255,80), 2));
        sp.drawEllipse(230,40,60,60);
        sp.setPen(Qt::NoPen);
        sp.setBrush(QColor(0,174,255));

        // Camera lens shape
        sp.drawEllipse(248,58,24,24);
        sp.setBrush(QColor(15,15,30));
        sp.drawEllipse(254,64,12,12);
        sp.setBrush(QColor(0,174,255));
        sp.drawEllipse(257,67,6,6);

        // Title - draw as one clean line
        QFont titleFont("Arial", 30, QFont::Bold);
        sp.setFont(titleFont);

        // "ScreenMaster" in white
        sp.setPen(Qt::white);
        sp.drawText(QRect(0, 115, 340, 50), Qt::AlignRight | Qt::AlignVCenter, "ScreenMaster");

        // "Pro" in blue - right next to it
        sp.setPen(QColor(0, 174, 255));
        sp.drawText(QRect(345, 115, 175, 50), Qt::AlignLeft | Qt::AlignVCenter, " Pro");

        QFont subFont("Arial", 11);
        sp.setFont(subFont);
        sp.setPen(QColor(150,150,180));
        sp.drawText(QRect(0,162,520,25), Qt::AlignCenter,
                    "Professional Screenshot & Screen Recording");

        // Version badge
        sp.setBrush(QColor(0,174,255));
        sp.setPen(Qt::NoPen);
        sp.drawRoundedRect(QRect(218,200,84,24), 12,12);
        QFont vFont("Arial", 10, QFont::Bold);
        sp.setFont(vFont);
        sp.setPen(Qt::white);
        sp.drawText(QRect(218,200,84,24), Qt::AlignCenter, "Version 2.0");

        // Loading text
        sp.setPen(QColor(100,100,140));
        QFont loadFont("Arial", 9);
        sp.setFont(loadFont);
        sp.drawText(QRect(0,270,520,22), Qt::AlignCenter, "Loading...");
    }

    QSplashScreen splash(splashPx);
    splash.setWindowFlags(Qt::SplashScreen | Qt::WindowStaysOnTopHint);
    splash.show();
    app.processEvents();

    // Show splash for 1.8 seconds
    MainWindow *window = nullptr;
    QTimer::singleShot(1800, [&]() {
        splash.close();
        window->show();
        window->raise();
        window->activateWindow();
    });

    window = new MainWindow();

    int ret = app.exec();
    delete window;
    return ret;
}
