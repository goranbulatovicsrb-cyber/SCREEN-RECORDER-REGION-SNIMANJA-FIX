#pragma once
#include <QObject>
#include <QTimer>
#include <QRect>
#include <QString>
#include <QPixmap>
#include <QProcess>
#include <QUrl>

class QScreenCapture;
class QMediaCaptureSession;
class QMediaRecorder;
class QAudioInput;

class ScreenRecorder : public QObject {
    Q_OBJECT
public:
    explicit ScreenRecorder(QObject *parent = nullptr);
    ~ScreenRecorder();

    void setFps(int fps)                { m_fps = fps; }
    void setQuality(const QString &q)   { m_quality = q; }
    void setRecordCursor(bool v)        { m_recordCursor = v; }
    void setRecordAudio(bool v)         { m_recordAudio = v; }
    void setOutputPath(const QString &p);
    void setRegion(const QRect &r)      { m_recordRegion = r; m_useRegion = r.isValid() && !r.isEmpty(); }
    void clearRegion()                  { m_recordRegion = QRect(); m_useRegion = false; }

    bool    isRecording()   const { return m_recording; }
    bool    isPaused()      const { return m_paused; }
    bool    hasRegion()     const { return m_useRegion; }
    QRect   currentRegion() const { return m_recordRegion; }
    QString lastOutputFile()const { return m_outputFile; }

    // Find ffmpeg.exe next to the app
    static QString ffmpegPath();

public slots:
    void startRecording();
    void stopRecording();
    void pauseRecording();
    void resumeRecording();
    void setFullScreen();
    void selectRegion();
    void selectWindow();

signals:
    void recordingStarted();
    void recordingStopped(const QString &filePath);
    void recordingPaused();
    void recordingResumed();
    void recordingTick(int seconds);
    void recordingError(const QString &msg);

private slots:
    void onTick();
    void onRecorderError();
    void onFfmpegFinished(int exitCode);

private:
    void startQtRecording();
    void startFfmpegRecording();
    void setupSession();
    QString buildOutputPath();
    QString bitrateForQuality() const;

    // Qt Multimedia (full screen)
    QScreenCapture       *m_screenCapture   = nullptr;
    QMediaCaptureSession *m_session         = nullptr;
    QMediaRecorder       *m_recorder        = nullptr;
    QAudioInput          *m_audioInput      = nullptr;

    // FFmpeg (region recording)
    QProcess             *m_ffmpeg          = nullptr;

    QTimer   *m_tickTimer;
    int       m_fps          = 30;
    QString   m_quality      = "High";
    bool      m_recordCursor = true;
    bool      m_recordAudio  = false;
    bool      m_recording    = false;
    bool      m_paused       = false;
    bool      m_useRegion    = false;
    bool      m_usingFfmpeg  = false;
    int       m_elapsed      = 0;
    QString   m_outputPath;
    QString   m_outputFile;
    QRect     m_recordRegion;
};
