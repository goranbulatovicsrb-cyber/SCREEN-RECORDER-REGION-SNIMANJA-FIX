#include "CaptureManager.h"
#include "RegionSelector.h"
#include <QGuiApplication>
#include <QScreen>
#include <QStandardPaths>
#include <QDateTime>
#include <QDir>
#include <QTimer>
#include <QPainter>
#include <QImage>
#include <climits>
#ifdef Q_OS_WIN
#include <windows.h>
#endif

CaptureManager::CaptureManager(QObject *parent)
    : QObject(parent), m_scrolling(false), m_noScrollCount(0)
    , m_filenameTemplate("Screenshot_%Y%m%d_%H%M%S")
{
    m_savePath = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation)
                 + "/ScreenMasterPro";
    QDir().mkpath(m_savePath);
    m_scrollTimer = new QTimer(this);
    m_scrollTimer->setInterval(1000); // 1 screenshot per second
    connect(m_scrollTimer, &QTimer::timeout, this, &CaptureManager::onScrollFrame);
}

// ── Basic captures ────────────────────────────────────────────────────────────

void CaptureManager::captureFullScreen(int monitorIndex) {
    auto screens = QGuiApplication::screens();
    QScreen *s = (monitorIndex >= 0 && monitorIndex < screens.size())
                  ? screens[monitorIndex] : QGuiApplication::primaryScreen();
    if (!s) return;
    QPixmap px = s->grabWindow(0);
    if (px.isNull()) return;
    QString path = buildSavePath("png");
    px.save(path, "PNG");
    addToHistory(px, path, "PNG");
    emit captureComplete(px, path);
}

void CaptureManager::captureRegion(bool autoCapture, const QPixmap &preGrabbed) {
    RegionSelector *sel = new RegionSelector(autoCapture, preGrabbed, nullptr);

    // regionCropped: pixmap already clean (cropped from pre-grabbed background)
    connect(sel, &RegionSelector::regionCropped, this, [this](const QPixmap &px) {
        if (px.isNull()) return;
        QString path = buildSavePath("png");
        px.save(path, "PNG");
        addToHistory(px, path, "PNG");
        emit captureComplete(px, path);
    });

    // regionSelected: fallback for compatibility (re-grabs screen)
    connect(sel, &RegionSelector::regionSelected, this, &CaptureManager::onRegionSelected);

    connect(sel, &RegionSelector::cancelled, this, [this](){
        emit captureError("Cancelled");
    });
}

void CaptureManager::onRegionSelected(const QRect &r) {
    if (!r.isValid()) return;
    QScreen *s = QGuiApplication::primaryScreen();
    if (!s) return;
    QPixmap px = s->grabWindow(0, r.x(), r.y(), r.width(), r.height());
    if (px.isNull()) return;
    QString path = buildSavePath("png");
    px.save(path, "PNG");
    addToHistory(px, path, "PNG");
    emit captureComplete(px, path);
}

void CaptureManager::captureWindow() {
#ifdef Q_OS_WIN
    HWND hwnd = GetForegroundWindow();
    if (!hwnd) { captureFullScreen(0); return; }
    RECT r; GetWindowRect(hwnd, &r);
    QRect wr(r.left, r.top, r.right - r.left, r.bottom - r.top);
    QScreen *s = QGuiApplication::primaryScreen();
    if (!s) return;
    QPixmap px = s->grabWindow(0, wr.x(), wr.y(), wr.width(), wr.height());
    if (px.isNull()) { captureFullScreen(0); return; }
    QString path = buildSavePath("png");
    px.save(path, "PNG");
    addToHistory(px, path, "PNG");
    emit captureComplete(px, path);
#else
    captureFullScreen(0);
#endif
}

// ── SCROLL STITCHING ──────────────────────────────────────────────────────────
//
// With 1-second interval, user scrolls a large amount between captures.
// So: bottom of frame[i] appears near the TOP of frame[i+1].
//
// Algorithm:
//   1. Take a reference strip from BOTTOM 15% of prev frame
//      (X range: 30-70% to avoid fixed sidebars)
//   2. Search for this strip only in TOP 45% of curr frame
//      (because large scroll → prev bottom landed near curr top)
//   3. If found at testY → new content in curr starts at testY + stripH
//   4. If not found → frames don't overlap much → stack with small 5% safety overlap
//
// This approach is robust because:
//   - 1s interval = big scroll = clear separation between frames
//   - Searching only top 45% avoids false matches in mid/bottom of curr
//   - 30-70% X range avoids fixed left sidebar and right ads

static QPixmap grabScreen(const QRect &area) {
    QScreen *screen = QGuiApplication::primaryScreen();
    if (!screen) return QPixmap();
    return screen->grabWindow(0, area.x(), area.y(), area.width(), area.height());
}

// Compare strip (prevY row in prev) vs (currY row in curr)
// using central X band only (avoid fixed sidebars)
static int stripScore(const QImage &prev, const QImage &curr,
                       int prevY, int currY, int stripH, int w)
{
    int xS = w * 3 / 10; // 30%
    int xE = w * 7 / 10; // 70%
    int score = 0, n = 0;
    for (int dy = 0; dy < stripH; dy += 3) {
        int pY = prevY + dy, cY = currY + dy;
        if (pY >= prev.height() || cY >= curr.height()) break;
        const QRgb *rP = reinterpret_cast<const QRgb*>(prev.constScanLine(pY));
        const QRgb *rC = reinterpret_cast<const QRgb*>(curr.constScanLine(cY));
        for (int x = xS; x < xE; x += 3) {
            score += qAbs(qRed(rP[x])   - qRed(rC[x]))
                   + qAbs(qGreen(rP[x]) - qGreen(rC[x]))
                   + qAbs(qBlue(rP[x])  - qBlue(rC[x]));
            n++;
        }
    }
    return n > 0 ? score / n : 255;
}

// Returns the Y position in curr where NEW content starts.
// Returns -1 if frames don't overlap (stack them with tiny overlap).
static int findJoinPoint(const QPixmap &prev, const QPixmap &curr)
{
    // Scale down to thumbnail for speed
    const int TW = 150;
    QImage a = prev.toImage().scaledToWidth(TW).convertToFormat(QImage::Format_RGB32);
    QImage b = curr.toImage().scaledToWidth(TW).convertToFormat(QImage::Format_RGB32);
    int hA = a.height(), hB = b.height(), w = a.width();

    // Reference strip: bottom 15% of prev (just above taskbar-safe area)
    const int STRIP = 20; // rows in thumbnail
    int refY = hA * 84 / 100; // 84% down = bottom area
    if (refY + STRIP >= hA) refY = hA - STRIP - 2;

    // ONLY search in top 45% of curr (large scroll assumption)
    int searchEnd = hB * 45 / 100;

    int bestY = -1, bestScore = INT_MAX;
    for (int testY = 0; testY < searchEnd && testY + STRIP < hB; ++testY) {
        int sc = stripScore(a, b, refY, testY, STRIP, w);
        if (sc < bestScore) { bestScore = sc; bestY = testY; }
    }

    // Require good match (score < 18 = very similar)
    if (bestScore > 18 || bestY < 0) return -1;

    // Join point: where strip was found + strip height = start of new content
    int joinThumb = bestY + STRIP;

    // Scale back to original pixels
    double scaleY = (double)prev.height() / hA;
    int joinOrig  = qBound(10, (int)(joinThumb * scaleY), curr.height() - 10);

    return joinOrig;
}

// Quick check: are two frames almost identical?
static bool framesSame(const QPixmap &a, const QPixmap &b)
{
    QImage ia = a.toImage().scaledToWidth(80).convertToFormat(QImage::Format_RGB32);
    QImage ib = b.toImage().scaledToWidth(80).convertToFormat(QImage::Format_RGB32);
    int h = qMin(ia.height(), ib.height()), w = ia.width();
    int sc = 0, n = 0;
    for (int y = h / 4; y < h * 3 / 4; y += 5) {
        const QRgb *rA = reinterpret_cast<const QRgb*>(ia.constScanLine(y));
        const QRgb *rB = reinterpret_cast<const QRgb*>(ib.constScanLine(y));
        for (int x = w * 3 / 10; x < w * 7 / 10; x += 4) {
            sc += qAbs(qRed(rA[x])   - qRed(rB[x]))
                + qAbs(qGreen(rA[x]) - qGreen(rB[x]))
                + qAbs(qBlue(rA[x])  - qBlue(rB[x]));
            n++;
        }
    }
    return n > 0 && (sc / n) < 8;
}

// ── Scroll capture ────────────────────────────────────────────────────────────

void CaptureManager::captureScrolling() {
    emit scrollCountdownStarted(4);

    // Wait: 4s countdown + 400ms GO! + 800ms OS render = 5200ms
    QTimer::singleShot(5200, this, [this]() {
        m_scrollFrames.clear();
        m_scrolling     = true;
        m_noScrollCount = 0;

        QScreen *screen = QGuiApplication::primaryScreen();
        if (!screen) return;
        m_scrollCaptureArea = screen->availableGeometry(); // no taskbar

        // Capture first frame
        QPixmap first = grabScreen(m_scrollCaptureArea);
        if (!first.isNull()) m_scrollFrames.append(first);

        m_scrollTimer->setInterval(1000); // 1 frame per second
        m_scrollTimer->start();

        // Hard stop: 60 seconds max
        QTimer::singleShot(60000, this, [this]() {
            if (m_scrolling) {
                m_scrollTimer->stop();
                m_scrolling = false;
                finishScrollCapture();
            }
        });
    });
}

void CaptureManager::onScrollFrame() {
    if (!m_scrolling) return;

    QPixmap frame = grabScreen(m_scrollCaptureArea);
    if (frame.isNull()) return;

    // Skip if identical to last (user stopped scrolling)
    if (!m_scrollFrames.isEmpty() && framesSame(m_scrollFrames.last(), frame)) {
        m_noScrollCount++;
        // Auto-stop after 3 identical frames (3 seconds of no movement)
        if (m_noScrollCount >= 3) {
            m_scrollTimer->stop();
            m_scrolling = false;
            finishScrollCapture();
        }
        return;
    }

    m_noScrollCount = 0;
    m_scrollFrames.append(frame);
}

void CaptureManager::finishScrollCapture() {
    if (m_scrollFrames.isEmpty()) return;

    if (m_scrollFrames.size() == 1) {
        // Only one frame captured
        QString path = buildSavePath("png");
        m_scrollFrames[0].save(path, "PNG");
        addToHistory(m_scrollFrames[0], path, "PNG");
        emit captureComplete(m_scrollFrames[0], path);
        m_scrollFrames.clear();
        return;
    }

    int w  = m_scrollFrames[0].width();
    int fh = m_scrollFrames[0].height();

    // Output canvas: 30000px max height
    QPixmap result(w, 30000);
    result.fill(Qt::white);
    QPainter p(&result);

    // Draw first frame completely
    p.drawPixmap(0, 0, m_scrollFrames[0]);
    int y = fh; // current write position

    for (int i = 1; i < m_scrollFrames.size(); ++i) {
        const QPixmap &prev = m_scrollFrames[i - 1];
        const QPixmap &curr = m_scrollFrames[i];
        int h = curr.height();

        // Find where new content starts in curr
        int joinY = findJoinPoint(prev, curr);

        if (joinY < 0) {
            // No overlap found → frames are far apart, stack with 3% safety overlap
            joinY = h * 3 / 100;
        }

        int newH = h - joinY;
        if (newH < 5) continue;
        if (y + newH > result.height()) break;

        p.drawPixmap(0, y, curr, 0, joinY, w, newH);
        y += newH;
    }
    p.end();

    QPixmap trimmed = result.copy(0, 0, w, qMin(y, result.height()));
    m_scrollFrames.clear();

    QString path = buildSavePath("png");
    trimmed.save(path, "PNG");
    addToHistory(trimmed, path, "PNG");
    emit captureComplete(trimmed, path);
}

// ── Utility ───────────────────────────────────────────────────────────────────

void CaptureManager::startColorPicker() { emit colorPicked(Qt::red, QPoint(0, 0)); }

void CaptureManager::saveCapture(const QPixmap &px, const QString &fmt, int quality) {
    px.save(buildSavePath(fmt.toLower()), fmt.toUpper().toLocal8Bit(), quality);
}

void CaptureManager::setSavePath(const QString &path) {
    m_savePath = path; QDir().mkpath(path);
}

void CaptureManager::setFilenameTemplate(const QString &t) { m_filenameTemplate = t; }

QString CaptureManager::generateFilename(const QString &fmt) {
    QDateTime now = QDateTime::currentDateTime();
    QString n = m_filenameTemplate;
    n.replace("%Y", now.toString("yyyy")).replace("%m", now.toString("MM"))
     .replace("%d", now.toString("dd")).replace("%H", now.toString("hh"))
     .replace("%M", now.toString("mm")).replace("%S", now.toString("ss"));
    return n + "." + fmt.toLower();
}

QString CaptureManager::buildSavePath(const QString &fmt) {
    return m_savePath + "/" + generateFilename(fmt);
}

void CaptureManager::addToHistory(const QPixmap &px, const QString &path, const QString &fmt) {
    CaptureRecord r;
    r.pixmap    = px;
    r.filepath  = path;
    r.timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
    r.size      = px.size();
    r.format    = fmt;
    m_history.prepend(r);
}
