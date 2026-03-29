#include "RecordingCompleteDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QSlider>
#include <QFileInfo>
#include <QDesktopServices>
#include <QUrl>
#include <QMediaPlayer>
#include <QVideoWidget>
#include <QAudioOutput>
#include <QStyle>
#include <QTime>

RecordingCompleteDialog::RecordingCompleteDialog(const QString &videoPath, QWidget *parent)
    : QDialog(parent), m_videoPath(videoPath)
{
    setupUi(videoPath);
    setStyleSheet(R"(
        QDialog { background:#1a1a2e; color:#e0e0f0; }
        QLabel  { color:#e0e0f0; }
        QPushButton { background:#2d2d44; border:1px solid #404060; border-radius:6px;
                      padding:8px 16px; color:#e0e0f0; }
        QPushButton:hover { background:#3d3d5c; border-color:#00aeff; }
        QPushButton#primary { background:#0099dd; color:white; font-weight:bold; border:none; }
        QPushButton#primary:hover { background:#00aaee; }
        QPushButton#danger  { background:rgba(180,40,40,0.3); color:#ee6666; }
        QSlider::groove:horizontal { background:#404060; height:4px; border-radius:2px; }
        QSlider::handle:horizontal { background:#00aeff; width:14px; height:14px;
                                      margin:-5px 0; border-radius:7px; }
        QSlider::sub-page:horizontal { background:#00aeff; border-radius:2px; }
    )");
}

void RecordingCompleteDialog::setupUi(const QString &videoPath) {
    setWindowTitle("Recording Complete");
    setMinimumSize(720, 540);
    resize(820, 600);

    QVBoxLayout *main = new QVBoxLayout(this);
    main->setSpacing(12);
    main->setContentsMargins(16,16,16,16);

    // Title
    QLabel *title = new QLabel("✅  Recording Saved");
    title->setStyleSheet("font-size:16px; font-weight:bold; color:#00aeff;");
    main->addWidget(title);

    // File info
    QFileInfo fi(videoPath);
    QLabel *info = new QLabel(QString("📁  %1  (%2 MB)")
        .arg(fi.fileName())
        .arg(fi.size() / 1048576.0, 0, 'f', 1));
    info->setStyleSheet("color:#8888aa; font-size:12px;");
    main->addWidget(info);

    // Video preview
    m_video = new QVideoWidget();
    m_video->setMinimumHeight(320);
    m_video->setStyleSheet("background:#000;");
    main->addWidget(m_video, 1);

    // Playback controls
    QWidget *controls = new QWidget();
    QHBoxLayout *cl = new QHBoxLayout(controls);
    cl->setContentsMargins(0,0,0,0);
    cl->setSpacing(8);

    m_playBtn = new QPushButton("▶  Play");
    m_playBtn->setFixedWidth(90);
    m_playBtn->setCursor(Qt::PointingHandCursor);

    m_seekSlider = new QSlider(Qt::Horizontal);
    m_seekSlider->setRange(0, 1000);

    m_timeLbl = new QLabel("0:00 / 0:00");
    m_timeLbl->setStyleSheet("color:#8888aa; font-family:Consolas; min-width:100px;");
    m_timeLbl->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    cl->addWidget(m_playBtn);
    cl->addWidget(m_seekSlider, 1);
    cl->addWidget(m_timeLbl);
    main->addWidget(controls);

    // Action buttons
    QHBoxLayout *btnRow = new QHBoxLayout();
    QPushButton *openFolderBtn = new QPushButton("📁  Open Folder");
    openFolderBtn->setObjectName("primary");
    openFolderBtn->setFixedHeight(40);

    QPushButton *closeBtn = new QPushButton("✖  Close");
    closeBtn->setObjectName("danger");
    closeBtn->setFixedHeight(40);

    btnRow->addWidget(openFolderBtn);
    btnRow->addStretch();
    btnRow->addWidget(closeBtn);
    main->addLayout(btnRow);

    // Setup player
    m_player = new QMediaPlayer(this);
    auto *audioOut = new QAudioOutput(this);
    m_player->setAudioOutput(audioOut);
    m_player->setVideoOutput(m_video);
    m_player->setSource(QUrl::fromLocalFile(videoPath));

    connect(m_playBtn, &QPushButton::clicked, this, &RecordingCompleteDialog::onPlayPause);
    connect(m_seekSlider, &QSlider::sliderMoved, this, &RecordingCompleteDialog::onSliderMoved);
    connect(m_player, &QMediaPlayer::positionChanged, this, &RecordingCompleteDialog::onPositionChanged);
    connect(m_player, &QMediaPlayer::durationChanged, this, &RecordingCompleteDialog::onDurationChanged);
    connect(m_player, &QMediaPlayer::playbackStateChanged, this,
            [this](QMediaPlayer::PlaybackState st) {
        m_playBtn->setText(st == QMediaPlayer::PlayingState ? "⏸  Pause" : "▶  Play");
    });

    connect(openFolderBtn, &QPushButton::clicked, this, &RecordingCompleteDialog::onOpenFolder);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
}

void RecordingCompleteDialog::onPlayPause() {
    if (m_player->playbackState() == QMediaPlayer::PlayingState)
        m_player->pause();
    else
        m_player->play();
}

void RecordingCompleteDialog::onPositionChanged(qint64 pos) {
    if (m_player->duration() > 0) {
        m_seekSlider->setValue((int)(pos * 1000 / m_player->duration()));
        QTime t = QTime::fromMSecsSinceStartOfDay((int)pos);
        QTime d = QTime::fromMSecsSinceStartOfDay((int)m_player->duration());
        m_timeLbl->setText(t.toString("m:ss") + " / " + d.toString("m:ss"));
    }
}

void RecordingCompleteDialog::onDurationChanged(qint64) { onPositionChanged(m_player->position()); }

void RecordingCompleteDialog::onSliderMoved(int val) {
    if (m_player->duration() > 0)
        m_player->setPosition(m_player->duration() * val / 1000);
}

void RecordingCompleteDialog::onOpenFolder() {
    QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(m_videoPath).absolutePath()));
}
