#pragma once
#include <QWidget>
#include <QPoint>
#include <QRect>
#include <QPixmap>
#include <QLabel>

class RegionSelector : public QWidget {
    Q_OBJECT
public:
    explicit RegionSelector(bool autoCapture = false,
                            const QPixmap &bg = QPixmap(),
                            QWidget *parent = nullptr);

signals:
    void regionSelected(const QRect &region);  // kept for compatibility
    void regionCropped(const QPixmap &cropped); // new: already cropped, no re-grab
    void cancelled();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    void drawMagnifier(QPainter &p, const QPoint &pos);
    void drawCrosshair(QPainter &p, const QPoint &pos);
    void drawDimensions(QPainter &p, const QRect &rect);
    void drawToolbar(QPainter &p, const QRect &rect);
    QColor colorAtPoint(const QPoint &pos);

    QPixmap    m_backgroundPixmap;
    QPoint     m_startPoint;
    QPoint     m_currentPoint;
    QRect      m_selectionRect;
    bool       m_selecting;
    bool       m_hasSelection;
    bool       m_autoCapture;
    QLabel    *m_infoLabel;
};
