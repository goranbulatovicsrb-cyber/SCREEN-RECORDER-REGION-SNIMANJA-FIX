#include "RegionSelector.h"

#include <QPainter>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QGuiApplication>
#include <QScreen>
#include <QApplication>
#include <QCursor>
#include <QPainterPath>
#include <QFont>
#include <QFontMetrics>

RegionSelector::RegionSelector(bool autoCapture, const QPixmap &bg, QWidget *parent)
    : QWidget(parent)
    , m_selecting(false)
    , m_hasSelection(false)
    , m_autoCapture(autoCapture)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_DeleteOnClose);
    setMouseTracking(true);
    setCursor(Qt::CrossCursor);

    QList<QScreen*> screens = QGuiApplication::screens();
    QRect totalGeom;
    for (auto *s : screens) totalGeom = totalGeom.united(s->geometry());
    setGeometry(totalGeom);

    // Use pre-grabbed pixmap if provided, otherwise grab now
    if (!bg.isNull()) {
        m_backgroundPixmap = bg;
    } else {
        QScreen *primary = QGuiApplication::primaryScreen();
        m_backgroundPixmap = primary->grabWindow(0,
            totalGeom.x(), totalGeom.y(), totalGeom.width(), totalGeom.height());
    }

    showFullScreen();
    raise();
    activateWindow();
    setFocus();
}

void RegionSelector::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    // Draw background
    p.drawPixmap(rect(), m_backgroundPixmap);

    // Dark overlay
    p.fillRect(rect(), QColor(0, 0, 0, 110));

    if (m_hasSelection || m_selecting) {
        QRect sel = QRect(m_startPoint, m_currentPoint).normalized();

        // Clear selection area (show original)
        p.drawPixmap(sel, m_backgroundPixmap, sel);

        // Selection border
        QPen selPen(QColor(0, 174, 255), 2, Qt::SolidLine);
        p.setPen(selPen);
        p.drawRect(sel);

        // Corner handles
        int hs = 8;
        p.setBrush(QColor(0, 174, 255));
        p.setPen(Qt::NoPen);
        QList<QPoint> corners = {
            sel.topLeft(), sel.topRight(), sel.bottomLeft(), sel.bottomRight(),
            QPoint(sel.center().x(), sel.top()),
            QPoint(sel.center().x(), sel.bottom()),
            QPoint(sel.left(), sel.center().y()),
            QPoint(sel.right(), sel.center().y())
        };
        for (const auto &c : corners) {
            p.drawRect(QRect(c.x()-hs/2, c.y()-hs/2, hs, hs));
        }

        // Draw dimensions text
        drawDimensions(p, sel);

        // Draw toolbar if selection is big enough
        if (sel.width() > 120 && sel.height() > 60 && !m_selecting) {
            drawToolbar(p, sel);
        }
    }

    // Draw crosshair at cursor
    if (!m_hasSelection) {
        drawCrosshair(p, m_currentPoint);
    }

    // Draw magnifier in corner
    drawMagnifier(p, m_currentPoint);

    // Draw instruction text if no selection started
    if (!m_selecting && !m_hasSelection) {
        p.setPen(Qt::white);
        QFont font = p.font();
        font.setPixelSize(16); font.setBold(false);
        p.setFont(font);
        QString hint = m_autoCapture
            ? "  Click and drag to select — releases automatically  |  ESC to cancel"
            : "  Click and drag to select  |  Enter to capture  |  ESC to cancel  |  Enter = full screen";
        QRect textRect(0, height() - 50, width(), 40);
        p.fillRect(textRect, QColor(0,0,0,160));
        p.drawText(textRect, Qt::AlignCenter, hint);
    }
}

void RegionSelector::drawCrosshair(QPainter &p, const QPoint &pos) {
    QPen pen(QColor(0, 200, 255, 200), 1, Qt::DashLine);
    p.setPen(pen);
    p.drawLine(0, pos.y(), width(), pos.y());
    p.drawLine(pos.x(), 0, pos.x(), height());

    // Color indicator at cursor
    QColor c = colorAtPoint(pos);
    p.setBrush(c);
    p.setPen(QPen(Qt::white, 1));
    p.drawEllipse(pos, 5, 5);
}

void RegionSelector::drawMagnifier(QPainter &p, const QPoint &pos) {
    const int magSize = 120;
    const int magZoom = 4;
    const int captureSize = magSize / magZoom;

    QRect captureRect(pos.x() - captureSize/2, pos.y() - captureSize/2,
                      captureSize, captureSize);
    captureRect = captureRect.intersected(m_backgroundPixmap.rect());

    QPixmap zoomed = m_backgroundPixmap.copy(captureRect)
                         .scaled(magSize, magSize, Qt::IgnoreAspectRatio, Qt::FastTransformation);

    // Position magnifier away from edges
    int mx = pos.x() + 20, my = pos.y() + 20;
    if (mx + magSize + 80 > width())  mx = pos.x() - magSize - 80;
    if (my + magSize + 40 > height()) my = pos.y() - magSize - 40;

    // Background
    p.setPen(QPen(QColor(0, 174, 255), 2));
    p.setBrush(Qt::black);
    p.drawRoundedRect(mx - 4, my - 4, magSize + 8, magSize + 8 + 22, 6, 6);
    p.drawPixmap(mx, my, zoomed);

    // Crosshair in magnifier
    p.setPen(QPen(QColor(255, 80, 80), 1));
    p.drawLine(mx, my + magSize/2, mx + magSize, my + magSize/2);
    p.drawLine(mx + magSize/2, my, mx + magSize/2, my + magSize);

    // Color at cursor
    QColor col = colorAtPoint(pos);
    QString hexColor = col.name().toUpper();
    p.fillRect(mx, my + magSize + 2, magSize/2, 18, col);
    p.setPen(col.lightnessF() > 0.5 ? Qt::black : Qt::white);
    QFont font; font.setPixelSize(9);
    p.setFont(font);
    p.drawText(QRect(mx, my + magSize + 2, magSize/2, 18), Qt::AlignCenter, hexColor);

    // Position label
    p.setPen(Qt::white);
    p.drawText(QRect(mx + magSize/2, my + magSize + 2, magSize/2, 18),
               Qt::AlignCenter, QString("%1,%2").arg(pos.x()).arg(pos.y()));
}

void RegionSelector::drawDimensions(QPainter &p, const QRect &rect) {
    QString dimText = QString("  %1 × %2 px  ").arg(rect.width()).arg(rect.height());
    QFont font; font.setPixelSize(13); font.setBold(true);
    p.setFont(font);
    QFontMetrics fm(font);
    QRect textRect = fm.boundingRect(dimText);
    textRect.moveCenter(QPoint(rect.center().x(), rect.top() - 14));
    textRect.adjust(-6,-3,6,3);

    if (textRect.top() < 4) textRect.moveTop(rect.bottom() + 4);

    p.fillRect(textRect, QColor(0, 174, 255, 220));
    p.setPen(Qt::white);
    p.drawText(textRect, Qt::AlignCenter, dimText);
}

void RegionSelector::drawToolbar(QPainter &p, const QRect &rect) {
    QRect toolbar(rect.left(), rect.bottom() + 6, 200, 36);
    if (toolbar.right() > width()) toolbar.moveRight(rect.right());

    p.setBrush(QColor(30, 30, 40, 230));
    p.setPen(QPen(QColor(80, 80, 100), 1));
    p.drawRoundedRect(toolbar, 8, 8);

    // Simple text hint
    p.setPen(Qt::white);
    QFont font; font.setPixelSize(11);
    p.setFont(font);
    p.drawText(toolbar, Qt::AlignCenter, "Enter ✓ | Esc ✗ | A: Annotate");
}

QColor RegionSelector::colorAtPoint(const QPoint &pos) {
    if (m_backgroundPixmap.isNull()) return Qt::black;
    QImage img = m_backgroundPixmap.toImage();
    if (pos.x() < 0 || pos.y() < 0 ||
        pos.x() >= img.width() || pos.y() >= img.height()) return Qt::black;
    return img.pixelColor(pos);
}

void RegionSelector::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        m_startPoint = event->pos();
        m_currentPoint = event->pos();
        m_selecting = true;
        m_hasSelection = false;
        update();
    }
}

void RegionSelector::mouseMoveEvent(QMouseEvent *event) {
    m_currentPoint = event->pos();
    update();
}

void RegionSelector::mouseReleaseEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton && m_selecting) {
        m_currentPoint = event->pos();
        m_selectionRect = QRect(m_startPoint, m_currentPoint).normalized();
        m_selecting = false;

        if (m_selectionRect.width() > 5 && m_selectionRect.height() > 5) {
            if (m_autoCapture) {
                // Crop directly from clean background (no RegionSelector overlay)
                QPixmap cropped = m_backgroundPixmap.copy(m_selectionRect);
                emit regionCropped(cropped);
                close();
            } else {
                m_hasSelection = true;
                update();
            }
        } else {
            emit cancelled();
            close();
        }
    }
}

void RegionSelector::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Escape) {
        emit cancelled();
        close();
    } else if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        if (m_hasSelection && m_selectionRect.isValid()) {
            // Crop from clean pre-grabbed background — no overlay visible
            QPixmap cropped = m_backgroundPixmap.copy(m_selectionRect);
            emit regionCropped(cropped);
            close();
        } else {
            // Full screen — crop entire background
            QPixmap cropped = m_backgroundPixmap;
            emit regionCropped(cropped);
            close();
        }
    } else if (event->key() == Qt::Key_Space && m_hasSelection) {
        QPixmap cropped = m_backgroundPixmap.copy(m_selectionRect);
        emit regionCropped(cropped);
        close();
    }
    QWidget::keyPressEvent(event);
}
