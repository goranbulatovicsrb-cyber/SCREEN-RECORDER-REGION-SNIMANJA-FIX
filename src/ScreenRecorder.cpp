#include "ScreenRecorder.h"
#include "RecordingCompleteDialog.h"

#include <QGuiApplication>
#include <QScreen>
#include <QDir>
#include <QDateTime>
#include <QStandardPaths>
#include <QCoreApplication>
#include <QDebug>
#include <QUrl>
#include <QFile>
#include <QFileInfo>

#include <QScreenCapture>
#include <QMediaCaptureSession>
#include <QMediaRecorder>
#include <QMediaFormat>
#include <QAudioInput>
#include <QMediaDevices>

ScreenRecorder::ScreenRecorder(QObject *parent)
    : QObject(parent)
{
    m_outputPath = QStandardPaths::writableLocation(QStandardPaths::MoviesLocation)
                   + "/ScreenMasterPro";
    QDir().mkpath(m_outputPath);

    m_tickTimer = new QTimer(this);
    m_tickTimer->setInterval(1000);
    connect(m_tickTimer, &QTimer::timeout, this, &ScreenRecorder::onTick);

    setupSession();
}

ScreenRecorder::~ScreenRecorder() {
    if (m_recording) stopRecording();
}

QString ScreenRecorder::ffmpegPath() {
    // Look for ffmpeg.exe next to the application executable
    QString appDir = QCoreApplication::applicationDirPath();
    QString path   = appDir + "/ffmpeg.exe";
    if (QFile::exists(path)) return path;
    // Also check current directory
    if (QFile::exists("ffmpeg.exe")) return "ffmpeg.exe";
    return QString();
}

void ScreenRecorder::setupSession() {
    m_screenCapture = new QScreenCapture(this);
    m_screenCapture->setScreen(QGuiApplication::primaryScreen());
    m_session = new QMediaCaptureSession(this);
    m_session->setScreenCapture(m_screenCapture);
    m_recorder = new QMediaRecorder(this);
    m_session->setRecorder(m_recorder);
    connect(m_recorder, &QMediaRecorder::errorOccurred,
            this, &ScreenRecorder::onRecorderError);
    connect(m_recorder, &QMediaRecorder::recorderStateChanged,
            this, [this](QMediaRecorder::RecorderState state) {
        if (state == QMediaRecorder::StoppedState && m_recording && !m_usingFfmpeg) {
            m_recording = false; m_paused = false;
            m_tickTimer->stop();
            emit recordingStopped(m_outputFile);
            if (QFile::exists(m_outputFile)) {
                auto *dlg = new RecordingCompleteDialog(m_outputFile);
                dlg->setAttribute(Qt::WA_DeleteOnClose);
                dlg->show();
            }
        }
    });
}

void ScreenRecorder::startRecording() {
    if (m_recording) return;
    m_outputFile = buildOutputPath();
    m_elapsed = 0; m_recording = true; m_paused = false;

    // Use FFmpeg for region recording if available, Qt for full screen
    QString ff = ffmpegPath();
    if (m_useRegion && !ff.isEmpty()) {
        m_usingFfmpeg = true;
        startFfmpegRecording();
    } else {
        if (m_useRegion && ff.isEmpty()) {
            emit recordingError("FFmpeg not found — recording full screen instead.\n"
                                "Place ffmpeg.exe next to ScreenMasterPro.exe for region recording.");
        }
        m_usingFfmpeg = false;
        startQtRecording();
    }
    m_tickTimer->start();
    emit recordingStarted();
}

// ── Qt full-screen recording ──────────────────────────────────────────────────

void ScreenRecorder::startQtRecording() {
    QMediaFormat fmt;
    fmt.setFileFormat(QMediaFormat::MPEG4);
    auto codecs = QMediaFormat(QMediaFormat::MPEG4).supportedVideoCodecs(QMediaFormat::Encode);
    if (codecs.contains(QMediaFormat::VideoCodec::H264))
        fmt.setVideoCodec(QMediaFormat::VideoCodec::H264);
    else if (!codecs.isEmpty())
        fmt.setVideoCodec(codecs.first());
    if (m_recordAudio) fmt.setAudioCodec(QMediaFormat::AudioCodec::AAC);
    m_recorder->setMediaFormat(fmt);

    if (m_quality == "Low")        m_recorder->setQuality(QMediaRecorder::LowQuality);
    else if (m_quality == "Medium") m_recorder->setQuality(QMediaRecorder::NormalQuality);
    else if (m_quality == "Ultra")  m_recorder->setQuality(QMediaRecorder::VeryHighQuality);
    else                            m_recorder->setQuality(QMediaRecorder::HighQuality);

    m_recorder->setVideoFrameRate(m_fps);
    m_recorder->setVideoBitRate(bitrateForQuality().toLongLong());

    if (m_recordAudio) {
        m_audioInput = new QAudioInput(this);
        m_session->setAudioInput(m_audioInput);
    } else {
        m_session->setAudioInput(nullptr);
    }

    m_recorder->setOutputLocation(QUrl::fromLocalFile(m_outputFile));
    m_screenCapture->start();
    m_recorder->record();
}

// ── FFmpeg region recording ───────────────────────────────────────────────────

void ScreenRecorder::startFfmpegRecording() {
    QString ff = ffmpegPath();
    if (ff.isEmpty()) { emit recordingError("ffmpeg.exe not found"); return; }

    // FFmpeg gdigrab captures a specific screen region directly on Windows
    QRect r = m_recordRegion;
    QString size    = QString("%1x%2").arg(r.width()).arg(r.height());
    QString offsetX = QString::number(r.x());
    QString offsetY = QString::number(r.y());
    QString fps     = QString::number(m_fps);
    QString preset  = (m_quality == "Low")   ? "veryfast" :
                      (m_quality == "Medium") ? "fast"     :
                      (m_quality == "Ultra")  ? "slow"     : "medium";
    QString crf     = (m_quality == "Low")   ? "28" :
                      (m_quality == "Medium") ? "23" :
                      (m_quality == "Ultra")  ? "18" : "20";
    // Draw mouse cursor
    QString drawMouse = m_recordCursor ? "1" : "0";

    QStringList args = {
        "-y",                               // overwrite output
        "-f",         "gdigrab",            // Windows GDI screen capture
        "-framerate", fps,                  // FPS
        "-offset_x",  offsetX,             // Region X
        "-offset_y",  offsetY,             // Region Y
        "-video_size", size,                // Region W×H
        "-draw_mouse", drawMouse,           // cursor yes/no
        "-i",         "desktop",            // input = desktop
        "-c:v",       "libx264",            // H264 codec
        "-preset",    preset,              // encoding speed
        "-crf",       crf,                 // quality (lower = better)
        "-pix_fmt",   "yuv420p",           // pixel format for compatibility
        m_outputFile                        // output file
    };

    if (m_recordAudio) {
        // Also capture system audio
        args = {
            "-y",
            "-f", "gdigrab",
            "-framerate", fps,
            "-offset_x", offsetX,
            "-offset_y", offsetY,
            "-video_size", size,
            "-draw_mouse", drawMouse,
            "-i", "desktop",
            "-f", "dshow",
            "-i", "audio=virtual-audio-capturer",
            "-c:v", "libx264",
            "-preset", preset,
            "-crf", crf,
            "-c:a", "aac",
            "-pix_fmt", "yuv420p",
            m_outputFile
        };
    }

    m_ffmpeg = new QProcess(this);
    connect(m_ffmpeg, QOverload<int,QProcess::ExitStatus>::of(&QProcess::finished),
            this, &ScreenRecorder::onFfmpegFinished);

    qDebug() << "Starting FFmpeg:" << ff << args;
    m_ffmpeg->start(ff, args);

    if (!m_ffmpeg->waitForStarted(3000)) {
        m_recording = false;
        emit recordingError("Failed to start FFmpeg: " + m_ffmpeg->errorString());
    }
}

void ScreenRecorder::onFfmpegFinished(int exitCode) {
    qDebug() << "FFmpeg finished with code" << exitCode;
    if (m_recording) {
        m_recording = false; m_paused = false;
        m_tickTimer->stop();
        m_usingFfmpeg = false;
        emit recordingStopped(m_outputFile);
        if (QFile::exists(m_outputFile)) {
            auto *dlg = new RecordingCompleteDialog(m_outputFile);
            dlg->setAttribute(Qt::WA_DeleteOnClose);
            dlg->show();
        }
    }
}

// ── Stop / Pause / Resume ─────────────────────────────────────────────────────

void ScreenRecorder::stopRecording() {
    if (!m_recording) return;

    if (m_usingFfmpeg && m_ffmpeg) {
        // Send 'q' to FFmpeg stdin to gracefully stop
        m_ffmpeg->write("q");
        m_ffmpeg->closeWriteChannel();
        if (!m_ffmpeg->waitForFinished(5000)) {
            m_ffmpeg->terminate();
            m_ffmpeg->waitForFinished(2000);
        }
    } else {
        m_recorder->stop();
        m_screenCapture->stop();
        if (m_audioInput) { delete m_audioInput; m_audioInput = nullptr; }
    }

    m_tickTimer->stop();
    m_recording = false; m_paused = false; m_elapsed = 0;
    if (!m_usingFfmpeg) emit recordingStopped(m_outputFile);
}

void ScreenRecorder::pauseRecording() {
    if (!m_recording || m_paused) return;
    if (m_usingFfmpeg) {
        // FFmpeg doesn't support pause easily — skip
        emit recordingError("Pause not supported for region recording");
        return;
    }
    m_recorder->pause();
    m_tickTimer->stop();
    m_paused = true;
    emit recordingPaused();
}

void ScreenRecorder::resumeRecording() {
    if (!m_recording || !m_paused) return;
    m_recorder->record();
    m_tickTimer->start();
    m_paused = false;
    emit recordingResumed();
}

void ScreenRecorder::onTick() {
    if (!m_paused) { m_elapsed++; emit recordingTick(m_elapsed); }
}

void ScreenRecorder::onRecorderError() {
    emit recordingError(m_recorder->errorString());
    if (m_recording) stopRecording();
}

void ScreenRecorder::setFullScreen() {
    clearRegion();
    m_screenCapture->setScreen(QGuiApplication::primaryScreen());
}

void ScreenRecorder::selectRegion() { /* handled in MainWindow */ }
void ScreenRecorder::selectWindow() {
    clearRegion();
    m_screenCapture->setScreen(QGuiApplication::primaryScreen());
}

void ScreenRecorder::setOutputPath(const QString &path) {
    m_outputPath = path; QDir().mkpath(path);
}

QString ScreenRecorder::buildOutputPath() {
    QString ts = QDateTime::currentDateTime().toString("yyyy-MM-dd_hh-mm-ss");
    return m_outputPath + "/Recording_" + ts + ".mp4";
}

QString ScreenRecorder::bitrateForQuality() const {
    if (m_quality == "Low")    return "2000000";
    if (m_quality == "Medium") return "4000000";
    if (m_quality == "Ultra")  return "20000000";
    return "8000000"; // High
}
