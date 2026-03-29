#include "ManualScrollCapture.h"
#include <QShortcut>
#include <QGuiApplication>
#include <QScreen>

ManualScrollCapture::ManualScrollCapture(QWidget *parent)
    : QWidget(parent)
{
    // Frameless toolbar at top of screen
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_ShowWithoutActivating);
    setFixedHeight(64);

    // Position at top center of screen
    QScreen *screen = QGuiApplication::primaryScreen();
    if (screen) {
        int sw = screen->availableGeometry().width();
        setFixedWidth(520);
        move(sw/2 - 260, 8);
    }

    QHBoxLayout *lay = new QHBoxLayout(this);
    lay->setContentsMargins(12, 8, 12, 8);
    lay->setSpacing(10);

    // Camera icon label
    QLabel *icon = new QLabel("📸");
    icon->setStyleSheet("font-size:20px; background:transparent;");
    lay->addWidget(icon);

    m_countLabel = new QLabel("0 sections");
    m_countLabel->setStyleSheet(
        "color:#ffffff; font-size:13px; font-weight:bold; background:transparent;");
    m_countLabel->setMinimumWidth(90);
    lay->addWidget(m_countLabel);

    m_hintLabel = new QLabel("Scroll, then click Capture");
    m_hintLabel->setStyleSheet(
        "color:#aaddff; font-size:11px; background:transparent;");
    lay->addWidget(m_hintLabel, 1);

    m_captureBtn = new QPushButton("📷  Capture Section");
    m_captureBtn->setFixedHeight(40);
    m_captureBtn->setCursor(Qt::PointingHandCursor);
    m_captureBtn->setStyleSheet(R"(
        QPushButton { background:#00aeff; color:white; font-weight:bold;
                      border:none; border-radius:8px; padding:0 16px; font-size:13px; }
        QPushButton:hover { background:#00ccff; }
        QPushButton:pressed { background:#0088cc; }
    )");

    m_doneBtn = new QPushButton("✔ Done");
    m_doneBtn->setFixedHeight(40);
    m_doneBtn->setFixedWidth(80);
    m_doneBtn->setCursor(Qt::PointingHandCursor);
    m_doneBtn->setEnabled(false);
    m_doneBtn->setStyleSheet(R"(
        QPushButton { background:#22aa44; color:white; font-weight:bold;
                      border:none; border-radius:8px; font-size:13px; }
        QPushButton:hover { background:#33cc55; }
        QPushButton:disabled { background:#224433; color:#446644; }
    )");

    m_cancelBtn = new QPushButton("✕");
    m_cancelBtn->setFixedSize(40, 40);
    m_cancelBtn->setCursor(Qt::PointingHandCursor);
    m_cancelBtn->setStyleSheet(R"(
        QPushButton { background:rgba(180,40,40,0.8); color:white; font-weight:bold;
                      border:none; border-radius:8px; font-size:14px; }
        QPushButton:hover { background:rgba(220,50,50,0.95); }
    )");

    lay->addWidget(m_captureBtn);
    lay->addWidget(m_doneBtn);
    lay->addWidget(m_cancelBtn);

    connect(m_captureBtn, &QPushButton::clicked, this, &ManualScrollCapture::onCaptureSection);
    connect(m_doneBtn,    &QPushButton::clicked, this, &ManualScrollCapture::onDone);
    connect(m_cancelBtn,  &QPushButton::clicked, this, &ManualScrollCapture::onCancel);

    // Keyboard shortcuts
    QShortcut *capSc  = new QShortcut(QKeySequence("Ctrl+Shift+Space"), this);
    QShortcut *doneSc = new QShortcut(QKeySequence("Ctrl+Shift+Return"), this);
    QShortcut *escSc  = new QShortcut(Qt::Key_Escape, this);
    connect(capSc,  &QShortcut::activated, this, &ManualScrollCapture::onCaptureSection);
    connect(doneSc, &QShortcut::activated, this, &ManualScrollCapture::onDone);
    connect(escSc,  &QShortcut::activated, this, &ManualScrollCapture::onCancel);
}

void ManualScrollCapture::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    // Dark pill background
    p.setBrush(QColor(15, 15, 30, 235));
    p.setPen(QPen(QColor(0, 174, 255, 160), 1.5));
    p.drawRoundedRect(rect().adjusted(1,1,-1,-1), 14, 14);
}

void ManualScrollCapture::onCaptureSection() {
    if (m_grabPending) return;
    m_grabPending = true;
    m_captureBtn->setEnabled(false);
    m_hintLabel->setText("Capturing...");

    // Hide toolbar briefly so it doesn't appear in screenshot
    hide();
    QApplication::processEvents();

    // Wait for toolbar to disappear from screen
    QTimer::singleShot(400, this, &ManualScrollCapture::doGrab);
}

void ManualScrollCapture::doGrab() {
    QScreen *screen = QGuiApplication::primaryScreen();
    if (screen) {
        QRect area = screen->availableGeometry();
        QPixmap px = screen->grabWindow(0,
            area.x(), area.y(), area.width(), area.height());
        if (!px.isNull()) m_sections.append(px);
    }

    m_grabPending = false;
    show();
    m_captureBtn->setEnabled(true);
    m_doneBtn->setEnabled(m_sections.size() >= 1);
    updateCounter();
}

void ManualScrollCapture::updateCounter() {
    int n = m_sections.size();
    m_countLabel->setText(QString("%1 section%2").arg(n).arg(n==1?"":"s"));
    if (n == 0)
        m_hintLabel->setText("Scroll, then click Capture");
    else if (n == 1)
        m_hintLabel->setText("Scroll more, capture again or Done");
    else
        m_hintLabel->setText(QString("%1 captured — scroll & capture or Done").arg(n));
}

void ManualScrollCapture::onDone() {
    if (m_sections.isEmpty()) { onCancel(); return; }
    hide();
    QPixmap result = stitchAll();
    emit captureComplete(result);
    close();
}

void ManualScrollCapture::onCancel() {
    emit cancelled();
    close();
}

// ── Stitch sections ───────────────────────────────────────────────────────────
// For each consecutive pair, find overlap using thumbnail comparison
// then append only the NEW part of the next section.

static int findOverlapRows(const QPixmap &top, const QPixmap &bottom)
{
    // Scale to narrow thumbnail for speed
    const int TW = 120;
    QImage a = top.toImage().scaledToWidth(TW).convertToFormat(QImage::Format_RGB32);
    QImage b = bottom.toImage().scaledToWidth(TW).convertToFormat(QImage::Format_RGB32);
    int hA = a.height(), hB = b.height(), w = a.width();

    // X range: 25%-75% to avoid fixed sidebars
    int xS = w/4, xE = w*3/4;

    // The user scrolled between captures → bottom of 'top' overlaps with top of 'bottom'
    // Strip from bottom of 'top' at refY, search in top half of 'bottom'
    const int STRIP = 20;
    int refY = hA * 80 / 100;
    if (refY + STRIP >= hA) refY = hA - STRIP - 2;

    int bestY = -1, bestScore = INT_MAX;
    int searchEnd = hB * 60 / 100; // search only top 60%

    for (int testY = 0; testY < searchEnd && testY + STRIP < hB; ++testY) {
        int sc = 0, n = 0;
        for (int dy = 0; dy < STRIP; dy += 3) {
            const QRgb *rA = reinterpret_cast<const QRgb*>(a.constScanLine(refY+dy));
            const QRgb *rB = reinterpret_cast<const QRgb*>(b.constScanLine(testY+dy));
            for (int x = xS; x < xE; x += 3) {
                sc += qAbs(qRed(rA[x])-qRed(rB[x]))
                    + qAbs(qGreen(rA[x])-qGreen(rB[x]))
                    + qAbs(qBlue(rA[x])-qBlue(rB[x]));
                n++;
            }
        }
        if (n > 0) {
            int avg = sc/n;
            if (avg < bestScore) { bestScore = avg; bestY = testY; }
        }
    }

    if (bestScore > 18 || bestY < 0) return -1;

    // New content in 'bottom' starts after the matched strip
    int thumbJoin = bestY + STRIP;
    double scaleY = (double)top.height() / hA;
    return qBound(10, (int)(thumbJoin * scaleY), bottom.height() - 10);
}

QPixmap ManualScrollCapture::stitchAll() {
    if (m_sections.size() == 1) return m_sections[0];

    int w = m_sections[0].width();
    int totalH = m_sections[0].height();

    // Calculate total height
    QList<int> joinPoints;
    for (int i = 1; i < m_sections.size(); ++i) {
        int jp = findOverlapRows(m_sections[i-1], m_sections[i]);
        if (jp < 0) jp = m_sections[i].height() * 3 / 100; // 3% safety if no match
        joinPoints.append(jp);
        totalH += m_sections[i].height() - jp;
    }

    QPixmap result(w, qMin(totalH, 30000));
    result.fill(Qt::white);
    QPainter p(&result);

    // Draw first section fully
    p.drawPixmap(0, 0, m_sections[0]);
    int y = m_sections[0].height();

    for (int i = 1; i < m_sections.size(); ++i) {
        int jp = joinPoints[i-1];
        int newH = m_sections[i].height() - jp;
        if (newH < 3) continue;
        if (y + newH > result.height()) break;
        p.drawPixmap(0, y, m_sections[i], 0, jp, w, newH);
        y += newH;
    }
    p.end();

    return result.copy(0, 0, w, qMin(y, result.height()));
}
