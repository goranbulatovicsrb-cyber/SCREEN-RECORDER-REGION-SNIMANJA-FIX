#pragma once
#include <QWidget>
#include <QTimer>
#include <QPainter>
#include <QGuiApplication>
#include <QScreen>
#include <QPropertyAnimation>
#include <QAbstractAnimation>

class FlashOverlay : public QWidget {
    Q_OBJECT
    Q_PROPERTY(qreal opacity READ opacity WRITE setOpacity)
public:
    static void flash() {
        FlashOverlay *f = new FlashOverlay();
        f->show();
    }

    qreal opacity() const { return m_opacity; }
    void setOpacity(qreal v) {
        m_opacity = v;
        setWindowOpacity(v);
        update();
    }

protected:
    void paintEvent(QPaintEvent *) override {
        QPainter p(this);
        p.fillRect(rect(), Qt::white);
    }

private:
    explicit FlashOverlay(QWidget *parent = nullptr) : QWidget(parent), m_opacity(0.85) {
        setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint |
                       Qt::Tool | Qt::WindowTransparentForInput);
        setAttribute(Qt::WA_DeleteOnClose);

        QRect total;
        for (auto *s : QGuiApplication::screens())
            total = total.united(s->geometry());
        setGeometry(total);
        setWindowOpacity(0.85);

        auto *anim = new QPropertyAnimation(this, "opacity", this);
        anim->setDuration(220);
        anim->setStartValue(0.85);
        anim->setEndValue(0.0);
        anim->setEasingCurve(QEasingCurve::OutQuad);
        connect(anim, &QPropertyAnimation::finished, this, &QWidget::close);
        anim->start(QAbstractAnimation::DeleteWhenStopped);
    }

    qreal m_opacity;
};
