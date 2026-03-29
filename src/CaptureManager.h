#pragma once
#include <QObject>
#include <QPixmap>
#include <QRect>
#include <QString>
#include <QTimer>
#include <QList>

struct CaptureRecord {
    QPixmap pixmap;
    QString filepath;
    QString timestamp;
    QString format;
    QSize   size;
};

class CaptureManager : public QObject {
    Q_OBJECT
public:
    explicit CaptureManager(QObject *parent = nullptr);
    void captureFullScreen(int monitorIndex = 0);
    void captureRegion(bool autoCapture = false, const QPixmap &preGrabbed = QPixmap());
    void captureWindow();
    void captureScrolling();
    void startColorPicker();
    void saveCapture(const QPixmap &px, const QString &format, int quality = 95);
    void setSavePath(const QString &path);
    void setFilenameTemplate(const QString &tmpl);
    QString generateFilename(const QString &format);
    QList<CaptureRecord> history() const { return m_history; }
    void clearHistory()             { m_history.clear(); }
    void removeFromHistory(int i)   { if(i>=0&&i<m_history.size()) m_history.removeAt(i); }

    // Public helpers for ManualScrollCapture
    QString buildSavePathPublic(const QString &fmt) { return buildSavePath(fmt); }
    void addToHistoryPublic(const QPixmap &px, const QString &path, const QString &fmt) {
        addToHistory(px, path, fmt);
    }
signals:
    void captureComplete(const QPixmap &pixmap, const QString &filepath);
    void colorPicked(const QColor &color, const QPoint &pos);
    void captureError(const QString &error);
    void scrollCountdownStarted(int seconds);
private slots:
    void onRegionSelected(const QRect &region);
    void onScrollFrame();
private:
    void finishScrollCapture();
    void addToHistory(const QPixmap &px, const QString &path, const QString &fmt);
    QString buildSavePath(const QString &format);
    QString              m_savePath;
    QString              m_filenameTemplate;
    QList<CaptureRecord> m_history;
    QTimer              *m_scrollTimer;
    QList<QPixmap>       m_scrollFrames;
    bool                 m_scrolling;
    QRect                m_scrollCaptureArea;
    int                  m_noScrollCount;
};
