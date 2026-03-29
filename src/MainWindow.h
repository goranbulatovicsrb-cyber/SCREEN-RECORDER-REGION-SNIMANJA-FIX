#pragma once
#include <QMainWindow>
#include <QSystemTrayIcon>
#include <QLabel>
#include <QStackedWidget>
#include <QPushButton>
#include <QSlider>
#include <QComboBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QListWidget>
#include <QTimer>
#include <QShortcut>
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>
#include "Translator.h"
#include "SettingsManager.h"

class CaptureManager;
class ScreenRecorder;
class AnnotationWidget;
class HistoryPanel;
class TrayManager;
class SettingsDialog;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

public slots:
    void onFullScreenCapture();
    void onRegionCapture();
    void onWindowCapture();
    void onScrollCapture();
    void onManualScrollCapture();
    void onDelayedCapture();
    void onStartRecording();
    void onStopRecording();
    void onPauseRecording();
    void onCaptureComplete(const QPixmap &pixmap, const QString &filepath);
    void onRecordingTick(int seconds);
    void onNavigate(int index);
    void onOpenHistory();
    void onOpenSettings();
    void onCopyToClipboard();
    void onSaveAs();
    void onAnnotateCapture();
    void onOcrCapture();
    void onUploadToCloud();
    void updateRecordingUI(bool recording, bool paused);
    void onTrayActivated(QSystemTrayIcon::ActivationReason reason);
    void showNotification(const QString &title, const QString &msg);
    void onDelayChanged(int seconds);
    void onFormatChanged(const QString &format);
    void onColorPickerTool();

private:
    void setupUI();
    void setupSidebar();
    void setupCenterPanel();
    void setupScreenshotTab();
    void setupRecordingTab();
    void setupHistoryTab();
    void setupSettingsTab();
    void buildScreenshotPage(QWidget *page);
    void buildRecordingPage(QWidget *page);
    void buildHistoryPage(QWidget *page);
    void buildSettingsPage(QWidget *page);
    void setupStatusBar();
    void setupShortcuts();
    void setupTray();
    void applyStyles();
    void applyTheme(const QString &theme);
    void retranslateUi();
    void applyAllSettings();
    void animateSidebarButton(QPushButton *btn);
    void updatePreview(const QPixmap &pixmap);
    void saveSettings();
    void loadSettings();
    QPushButton* createSidebarButton(const QString &icon, const QString &text, int index);
    QPushButton* createActionButton(const QString &icon, const QString &label, const QString &shortcut = "");

    // Core components
    CaptureManager    *m_captureManager;
    ScreenRecorder    *m_recorder;
    TrayManager       *m_trayManager;
    SettingsDialog    *m_settingsDialog;
    HistoryPanel      *m_historyPanel;

    // UI Elements
    QWidget           *m_sidebar;
    QStackedWidget    *m_pageStack;
    QLabel            *m_previewLabel;
    QLabel            *m_statusLabel;
    QLabel            *m_recordingTimeLabel;
    QLabel            *m_recordingDot;
    QTimer            *m_dotTimer;
    QWidget           *m_recordingBadge;

    // Screenshot controls
    QComboBox         *m_formatCombo;
    QSpinBox          *m_qualitySpinBox;
    QSpinBox          *m_delaySpin;
    QCheckBox         *m_autoCopyCheck;
    QCheckBox         *m_autoSaveCheck;
    QCheckBox         *m_showCursorCheck;
    QCheckBox         *m_soundCheck;
    QComboBox         *m_monitorCombo;

    // Recording controls
    QComboBox         *m_recFpsCombo;
    QComboBox         *m_recQualityCombo;
    QCheckBox         *m_recAudioCheck;
    QCheckBox         *m_recCursorCheck;
    QCheckBox         *m_recAnnotateCheck;
    QPushButton       *m_startRecBtn;
    QPushButton       *m_stopRecBtn;
    QPushButton       *m_pauseRecBtn;

    // Sidebar buttons
    QList<QPushButton*> m_navButtons;

    // Translatable labels (updated on language change)
    QLabel            *m_titleLabel;
    QLabel            *m_captureTitleLabel;
    QLabel            *m_previewTitleLabel;
    QLabel            *m_recTitleLabel;
    QLabel            *m_infoBarLabel;
    QList<QPushButton*> m_captureBtns;
    QList<QString>      m_captureBtnKeys;

    // State
    QPixmap           m_lastCapture;
    QString           m_lastSavePath;
    bool              m_isRecording;
    bool              m_isPaused;
    int               m_currentDelay;
};
