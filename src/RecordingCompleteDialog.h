#pragma once
#include <QDialog>
#include <QString>

class QLabel;
class QPushButton;
class QSlider;
class QMediaPlayer;
class QVideoWidget;

class RecordingCompleteDialog : public QDialog {
    Q_OBJECT
public:
    explicit RecordingCompleteDialog(const QString &videoPath, QWidget *parent = nullptr);

private slots:
    void onOpenFolder();
    void onPlayPause();
    void onPositionChanged(qint64 pos);
    void onDurationChanged(qint64 dur);
    void onSliderMoved(int val);

private:
    void setupUi(const QString &videoPath);

    QString       m_videoPath;
    QMediaPlayer *m_player     = nullptr;
    QVideoWidget *m_video      = nullptr;
    QPushButton  *m_playBtn    = nullptr;
    QSlider      *m_seekSlider = nullptr;
    QLabel       *m_timeLbl    = nullptr;
};
