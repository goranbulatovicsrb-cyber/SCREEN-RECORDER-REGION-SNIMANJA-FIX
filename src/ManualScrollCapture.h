#pragma once
#include <QWidget>
#include <QPixmap>
#include <QList>
#include <QPainter>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QGuiApplication>
#include <QScreen>
#include <QTimer>
#include <QApplication>

// ── Overlay that stays on screen between captures ─────────────────────────────
// Shows a small toolbar at top: "Capture Section" button + Done + Cancel
// User scrolls freely, clicks "Capture Section" when ready, repeat

class ManualScrollCapture : public QWidget {
    Q_OBJECT
public:
    explicit ManualScrollCapture(QWidget *parent = nullptr);

signals:
    void captureComplete(const QPixmap &stitched);
    void cancelled();

private slots:
    void onCaptureSection();
    void onDone();
    void onCancel();
    void doGrab();

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    void updateCounter();
    QPixmap stitchAll();

    QPushButton    *m_captureBtn;
    QPushButton    *m_doneBtn;
    QPushButton    *m_cancelBtn;
    QLabel         *m_countLabel;
    QLabel         *m_hintLabel;
    QList<QPixmap>  m_sections;
    bool            m_grabPending = false;
};
