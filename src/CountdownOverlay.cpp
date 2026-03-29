#include "CountdownOverlay.h"
#include <QPainter>
#include <QGuiApplication>
#include <QScreen>
#include <QFont>
#include <QKeyEvent>
#include <QPainterPath>

CountdownOverlay::CountdownOverlay(int seconds, QWidget *parent,
                                   const QString &hintText)
    : QWidget(parent)
    , m_remaining(seconds)
    , m_hintText(hintText)
    , m_autoClose(true)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_DeleteOnClose, false);

    QScreen *screen = QGuiApplication::primaryScreen();
    if (screen) setGeometry(screen->geometry());

    m_timer = new QTimer(this);
    m_timer->setInterval(1000);
    connect(m_timer, &QTimer::timeout, this, &CountdownOverlay::onTick);
    m_timer->start();

    setFocus();
}

void CountdownOverlay::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    // Semi-dark overlay
    p.fillRect(rect(), QColor(0,0,0,140));

    QPoint center = rect().center();

    // Outer ring
    p.setPen(QPen(QColor(0,174,255,60), 6));
    p.setBrush(Qt::NoBrush);
    p.drawEllipse(center, 90, 90);

    // Progress arc
    double progress = 1.0 - (double)m_remaining / 10.0;
    p.setPen(QPen(QColor(0,174,255), 6, Qt::SolidLine, Qt::RoundCap));
    p.drawArc(QRect(center.x()-90,center.y()-90,180,180),
              90*16, -int(progress*360*16));

    // Background circle
    p.setBrush(QColor(20,20,40,230));
    p.setPen(Qt::NoPen);
    p.drawEllipse(center, 76, 76);

    // Number (green at 0, blue otherwise)
    QFont numFont("Arial", 52, QFont::Bold);
    p.setFont(numFont);
    p.setPen(m_remaining == 0 ? QColor(0,255,120) : QColor(0,200,255));
    p.drawText(QRect(center.x()-76,center.y()-76,152,152),
               Qt::AlignCenter,
               m_remaining == 0 ? "GO!" : QString::number(m_remaining));

    // Hint text
    QFont hintFont("Arial", 14);
    p.setFont(hintFont);
    p.setPen(Qt::white);
    p.drawText(QRect(center.x()-280,center.y()+110,560,36),
               Qt::AlignCenter, m_hintText);

    // ESC hint
    QFont escFont("Arial", 10);
    p.setFont(escFont);
    p.setPen(QColor(120,120,160));
    p.drawText(QRect(center.x()-200,center.y()+150,400,24),
               Qt::AlignCenter, "Press ESC to cancel");
}

void CountdownOverlay::onTick() {
    m_remaining--;
    update();
    if (m_remaining <= 0) {
        m_timer->stop();
        // Show "GO!" for 400ms then close
        QTimer::singleShot(400, this, [this]() {
            emit finished();
            if (m_autoClose) close();
        });
    }
}

void CountdownOverlay::keyPressEvent(QKeyEvent *e) {
    if (e->key() == Qt::Key_Escape) {
        m_timer->stop();
        close();
    }
    QWidget::keyPressEvent(e);
}
