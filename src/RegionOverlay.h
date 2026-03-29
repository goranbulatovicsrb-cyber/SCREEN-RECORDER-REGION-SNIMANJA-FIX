#pragma once
#include <QWidget>
#include <QRect>
#include <QPainter>
#include <QTimer>
#include <QLabel>

// Shows a red dashed border around the recording region
// User can see exactly what's being recorded
class RegionOverlay : public QWidget {
public:
    explicit RegionOverlay(const QRect &region, QWidget *parent = nullptr)
        : QWidget(parent), m_dash(true)
    {
        setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint |
                       Qt::Tool | Qt::WindowTransparentForInput);
        setAttribute(Qt::WA_TranslucentBackground);
        setAttribute(Qt::WA_ShowWithoutActivating);
        setAttribute(Qt::WA_DeleteOnClose);
        setObjectName("RegionOverlay");

        // Position around the selected region
        setGeometry(region.adjusted(-3,-3,3,3));

        // Animate dashes
        m_dashTimer = new QTimer(this);
        m_dashTimer->setInterval(500);
        connect(m_dashTimer, &QTimer::timeout, this, [this](){
            m_dash = !m_dash; update();
        });
        m_dashTimer->start();
    }

    ~RegionOverlay() { m_dashTimer->stop(); }

protected:
    void paintEvent(QPaintEvent *) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        QPen pen(QColor(255, 60, 60), 3,
                 m_dash ? Qt::DashLine : Qt::SolidLine);
        p.setPen(pen);
        p.setBrush(Qt::NoBrush);
        p.drawRect(rect().adjusted(1,1,-1,-1));

        // "REC" label at top-left
        p.fillRect(0, 0, 52, 20, QColor(200, 30, 30, 200));
        p.setPen(Qt::white);
        QFont f; f.setPixelSize(11); f.setBold(true); p.setFont(f);
        p.drawText(QRect(0,0,52,20), Qt::AlignCenter, "● REC");
    }

private:
    QTimer *m_dashTimer;
    bool    m_dash;
};
