#include "MainWindow.h"
#include "CaptureManager.h"
#include "ScreenRecorder.h"
#include "AnnotationWidget.h"
#include "HistoryPanel.h"
#include "TrayManager.h"
#include "SettingsDialog.h"
#include "CountdownOverlay.h"
#include "FlashOverlay.h"
#include "Translator.h"
#include "SettingsManager.h"
#include "ManualScrollCapture.h"
#include "OcrEngine.h"
#include "OcrDialog.h"
#include "RegionSelector.h"
#include "RegionOverlay.h"

#include <QApplication>
#include <QClipboard>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QFrame>
#include <QScrollArea>
#include <QSettings>
#include <QCloseEvent>
#include <QKeyEvent>
#include <QScreen>
#include <QGuiApplication>
#include <QMessageBox>
#include <QGroupBox>
#include <QTabWidget>
#include <QToolButton>
#include <QSizePolicy>
#include <QSpacerItem>
#include <QGraphicsDropShadowEffect>
#include <QDateTime>
#include <QDir>
#include <QStandardPaths>
#include <QDesktopServices>
#include <QLineEdit>
#include <QRadioButton>
#include <QFileDialog>
#include <QPainter>
#include <QFileInfo>
#include <QResizeEvent>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_isRecording(false)
    , m_isPaused(false)
    , m_currentDelay(0)
    , m_titleLabel(nullptr)
    , m_captureTitleLabel(nullptr)
    , m_previewTitleLabel(nullptr)
    , m_recTitleLabel(nullptr)
    , m_infoBarLabel(nullptr)
    , m_historyPanel(nullptr)
{
    setWindowTitle("ScreenMaster Pro");
    setMinimumSize(1100, 720);
    resize(1280, 820);
    setWindowIcon(QIcon(":/icons/app_icon.png"));

    m_captureManager = new CaptureManager(this);
    m_recorder = new ScreenRecorder(this);
    m_trayManager = new TrayManager(this);
    m_settingsDialog = new SettingsDialog(this);

    setupUI();
    setupShortcuts();
    setupTray();
    loadSettings();
    applyStyles();

    // Connect language change signal
    connect(&Translator::instance(), &Translator::languageChanged,
            this, &MainWindow::retranslateUi);

    // Apply initial language
    Translator::instance().setLanguage(SM.language());

    connect(m_captureManager, &CaptureManager::captureComplete,
            this, &MainWindow::onCaptureComplete);
    connect(m_captureManager, &CaptureManager::scrollCountdownStarted,
            this, [this](int secs) {
        CountdownOverlay *overlay = new CountdownOverlay(
            secs, nullptr,
            "Scrolling starts now! Scroll your page slowly...");
        overlay->setAttribute(Qt::WA_DeleteOnClose);
        overlay->setAutoClose(true);
        overlay->show();
        // When capture completes, show main window
    });
    connect(m_recorder, &ScreenRecorder::recordingStarted,
            this, [this]() {
        updateRecordingUI(true, false);
        m_dotTimer->start(600);
        // Show region overlay if region is set
        if (m_recorder->hasRegion()) {
            RegionOverlay *overlay = new RegionOverlay(m_recorder->currentRegion(), nullptr);
            overlay->setAttribute(Qt::WA_DeleteOnClose);
            overlay->show();
            // Close overlay when recording stops
            connect(m_recorder, &ScreenRecorder::recordingStopped,
                    overlay, &QWidget::close);
        }
    });
    connect(m_recorder, &ScreenRecorder::recordingStopped,
            this, [this](const QString &path) {
        updateRecordingUI(false, false);
        m_dotTimer->stop();
        m_recordingDot->setVisible(false);
        showNotification("Recording Saved", QFileInfo(path).fileName());
        m_statusLabel->setText("Recording saved: " + QFileInfo(path).fileName());
    });
    connect(m_recorder, &ScreenRecorder::recordingTick,
            this, &MainWindow::onRecordingTick);
    connect(m_recorder, &ScreenRecorder::recordingError,
            this, [this](const QString &err) {
        QMessageBox::warning(this, "Recording Error", err);
        updateRecordingUI(false, false);
    });

    m_dotTimer = new QTimer(this);
    connect(m_dotTimer, &QTimer::timeout, this, [this]() {
        static bool vis = true;
        m_recordingDot->setVisible(vis);
        vis = !vis;
    });
}

MainWindow::~MainWindow() { saveSettings(); }

void MainWindow::setupUI() {
    QWidget *central = new QWidget(this);
    setCentralWidget(central);
    QHBoxLayout *mainLayout = new QHBoxLayout(central);
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0,0,0,0);

    setupSidebar();
    setupCenterPanel();

    mainLayout->addWidget(m_sidebar);
    mainLayout->addWidget(m_pageStack, 1);
}

void MainWindow::setupSidebar() {
    m_sidebar = new QWidget();
    m_sidebar->setObjectName("sidebar");
    m_sidebar->setFixedWidth(220);

    QVBoxLayout *sl = new QVBoxLayout(m_sidebar);
    sl->setSpacing(4);
    sl->setContentsMargins(12, 20, 12, 20);

    // Logo / Title
    QLabel *logoLabel = new QLabel();
    logoLabel->setObjectName("appLogo");
    logoLabel->setText("📸");
    logoLabel->setAlignment(Qt::AlignCenter);
    logoLabel->setFixedHeight(50);

    QLabel *titleLabel = new QLabel("ScreenMaster");
    titleLabel->setObjectName("appTitle");
    titleLabel->setAlignment(Qt::AlignCenter);

    QLabel *versionLabel = new QLabel("PRO  v2.0");
    versionLabel->setObjectName("appVersion");
    versionLabel->setAlignment(Qt::AlignCenter);

    sl->addWidget(logoLabel);
    sl->addWidget(titleLabel);
    sl->addWidget(versionLabel);

    QFrame *sep1 = new QFrame(); sep1->setFrameShape(QFrame::HLine);
    sep1->setObjectName("separator");
    sl->addWidget(sep1);
    sl->addSpacing(8);

    // Navigation buttons
    auto addBtn = [&](const QString &emoji, const QString &text, int idx) {
        QPushButton *btn = createSidebarButton(emoji, text, idx);
        m_navButtons.append(btn);
        sl->addWidget(btn);
        connect(btn, &QPushButton::clicked, this, [this, idx]() { onNavigate(idx); });
        return btn;
    };

    addBtn("🖥️",  " Screenshot",  0);
    addBtn("🎬",  " Recording",   1);
    addBtn("🕐",  " History",     2);
    addBtn("⚙️",  " Settings",    3);

    sl->addStretch();

    QFrame *sep2 = new QFrame(); sep2->setFrameShape(QFrame::HLine);
    sep2->setObjectName("separator");
    sl->addWidget(sep2);

    // Recording badge at bottom of sidebar
    m_recordingBadge = new QWidget();
    m_recordingBadge->setObjectName("recordingBadge");
    m_recordingBadge->setVisible(false);
    QHBoxLayout *rbLayout = new QHBoxLayout(m_recordingBadge);
    rbLayout->setContentsMargins(8,6,8,6);
    m_recordingDot = new QLabel("●");
    m_recordingDot->setObjectName("recordingDot");
    m_recordingTimeLabel = new QLabel("00:00:00");
    m_recordingTimeLabel->setObjectName("recordingTime");
    rbLayout->addWidget(m_recordingDot);
    rbLayout->addWidget(m_recordingTimeLabel);
    rbLayout->addStretch();
    sl->addWidget(m_recordingBadge);

    // Status label
    m_statusLabel = new QLabel("Ready");
    m_statusLabel->setObjectName("statusLabel");
    m_statusLabel->setAlignment(Qt::AlignCenter);
    sl->addWidget(m_statusLabel);
}

QPushButton* MainWindow::createSidebarButton(const QString &icon, const QString &text, int index) {
    QPushButton *btn = new QPushButton(icon + text);
    btn->setObjectName("navButton");
    btn->setCheckable(true);
    btn->setFixedHeight(44);
    btn->setCursor(Qt::PointingHandCursor);
    if (index == 0) btn->setChecked(true);
    return btn;
}

void MainWindow::setupCenterPanel() {
    m_pageStack = new QStackedWidget();
    m_pageStack->setObjectName("pageStack");

    QWidget *screenshotPage = new QWidget();
    QWidget *recordingPage  = new QWidget();
    QWidget *historyPage    = new QWidget();
    QWidget *settingsPage   = new QWidget();

    m_pageStack->addWidget(screenshotPage);
    m_pageStack->addWidget(recordingPage);
    m_pageStack->addWidget(historyPage);
    m_pageStack->addWidget(settingsPage);

    setupScreenshotTab();
    buildScreenshotPage(screenshotPage);
    buildRecordingPage(recordingPage);
    buildHistoryPage(historyPage);
    buildSettingsPage(settingsPage);
}

void MainWindow::setupScreenshotTab() { /* Handled in buildScreenshotPage */ }

void MainWindow::buildScreenshotPage(QWidget *page) {
    QHBoxLayout *mainLay = new QHBoxLayout(page);
    mainLay->setContentsMargins(24, 24, 24, 24);
    mainLay->setSpacing(20);

    // Left: Capture buttons
    QWidget *leftPanel = new QWidget();
    leftPanel->setObjectName("leftPanel");
    leftPanel->setFixedWidth(280);
    QVBoxLayout *leftLay = new QVBoxLayout(leftPanel);
    leftLay->setSpacing(12);
    leftLay->setContentsMargins(0,0,0,0);

    // Section title
    QLabel *captureTitle = new QLabel(TR("Capture Mode"));
    captureTitle->setObjectName("sectionTitle");
    m_captureTitleLabel = captureTitle;
    leftLay->addWidget(captureTitle);

    // Capture mode buttons grid
    QWidget *btnContainer = new QWidget();
    QGridLayout *grid2 = new QGridLayout(btnContainer);
    grid2->setSpacing(10);

    auto makeCapBtn = [&](const QString &emoji, const QString &title, const QString &sc) -> QPushButton* {
        QPushButton *btn = new QPushButton();
        btn->setObjectName("captureModeBtn");
        btn->setCursor(Qt::PointingHandCursor);
        btn->setFixedHeight(80);
        btn->setText(QString("%1\n%2\n%3").arg(emoji, title, sc));
        return btn;
    };

    QPushButton *btnFull    = makeCapBtn("🖥️", "Full Screen",    "PrtSc");
    QPushButton *btnRegion  = makeCapBtn("✂️",  "Region",         "Ctrl+⇧+S");
    QPushButton *btnWindow  = makeCapBtn("🪟",  "Window",         "Alt+PrtSc");
    QPushButton *btnScroll  = makeCapBtn("📜",  "Auto Scroll",    "Ctrl+⇧+W");
    QPushButton *btnDelay   = makeCapBtn("🕐",  "Timed",          "Ctrl+⇧+T");
    QPushButton *btnColor   = makeCapBtn("🎨",  "Color Picker",   "Ctrl+⇧+C");
    QPushButton *btnManual  = makeCapBtn("🖱️",  "Manual Scroll",  "Ctrl+⇧+M");
    QPushButton *btnAnnot   = makeCapBtn("✏️",  "Annotate Last",  "Ctrl+E");

    grid2->addWidget(btnFull,   0,0); grid2->addWidget(btnRegion, 0,1);
    grid2->addWidget(btnWindow, 1,0); grid2->addWidget(btnScroll, 1,1);
    grid2->addWidget(btnDelay,  2,0); grid2->addWidget(btnColor,  2,1);
    grid2->addWidget(btnManual, 3,0); grid2->addWidget(btnAnnot,  3,1);

    m_captureBtns = {btnFull, btnRegion, btnWindow, btnScroll, btnDelay, btnColor, btnManual, btnAnnot};

    connect(btnFull,   &QPushButton::clicked, this, &MainWindow::onFullScreenCapture);
    connect(btnRegion, &QPushButton::clicked, this, &MainWindow::onRegionCapture);
    connect(btnWindow, &QPushButton::clicked, this, &MainWindow::onWindowCapture);
    connect(btnScroll, &QPushButton::clicked, this, &MainWindow::onScrollCapture);
    connect(btnDelay,  &QPushButton::clicked, this, &MainWindow::onDelayedCapture);
    connect(btnColor,  &QPushButton::clicked, this, &MainWindow::onColorPickerTool);
    connect(btnManual, &QPushButton::clicked, this, &MainWindow::onManualScrollCapture);
    connect(btnAnnot,  &QPushButton::clicked, this, &MainWindow::onAnnotateCapture);

    leftLay->addWidget(btnContainer);
    leftLay->addSpacing(16);

    // Options group
    QGroupBox *optsBox = new QGroupBox("Options");
    optsBox->setObjectName("optionsBox");
    QVBoxLayout *optsLay = new QVBoxLayout(optsBox);
    optsLay->setSpacing(8);

    // Delay
    QHBoxLayout *delayRow = new QHBoxLayout();
    QLabel *delayLbl = new QLabel("Delay (sec):");
    delayLbl->setObjectName("optLabel");
    m_delaySpin = new QSpinBox();
    m_delaySpin->setRange(0, 30); m_delaySpin->setValue(0);
    m_delaySpin->setObjectName("optSpinBox");
    connect(m_delaySpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &MainWindow::onDelayChanged);
    delayRow->addWidget(delayLbl);
    delayRow->addWidget(m_delaySpin);
    optsLay->addLayout(delayRow);

    // Format
    QHBoxLayout *fmtRow = new QHBoxLayout();
    QLabel *fmtLbl = new QLabel("Format:");
    fmtLbl->setObjectName("optLabel");
    m_formatCombo = new QComboBox();
    m_formatCombo->setObjectName("optCombo");
    m_formatCombo->addItems({"PNG", "JPG", "BMP", "WebP", "TIFF"});
    m_formatCombo->setCurrentText(SM.format());
    connect(m_formatCombo, &QComboBox::currentTextChanged,
            [](const QString &v){ SM.set("format", v); });
    fmtRow->addWidget(fmtLbl);
    fmtRow->addWidget(m_formatCombo);
    optsLay->addLayout(fmtRow);

    // Quality
    QHBoxLayout *qualRow = new QHBoxLayout();
    QLabel *qualLbl = new QLabel("Quality:");
    qualLbl->setObjectName("optLabel");
    m_qualitySpinBox = new QSpinBox();
    m_qualitySpinBox->setRange(10, 100); m_qualitySpinBox->setValue(SM.quality());
    m_qualitySpinBox->setSuffix("%");
    m_qualitySpinBox->setObjectName("optSpinBox");
    connect(m_qualitySpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            [](int v){ SM.set("quality", v); });
    qualRow->addWidget(qualLbl);
    qualRow->addWidget(m_qualitySpinBox);
    optsLay->addLayout(qualRow);

    // Monitor selection
    QHBoxLayout *monRow = new QHBoxLayout();
    QLabel *monLbl = new QLabel("Monitor:");
    monLbl->setObjectName("optLabel");
    m_monitorCombo = new QComboBox();
    m_monitorCombo->setObjectName("optCombo");
    auto screens = QGuiApplication::screens();
    for (int i = 0; i < screens.size(); ++i)
        m_monitorCombo->addItem(QString("Display %1").arg(i+1));
    monRow->addWidget(monLbl);
    monRow->addWidget(m_monitorCombo);
    optsLay->addLayout(monRow);

    // Checkboxes
    m_autoCopyCheck  = new QCheckBox("Auto copy to clipboard");
    m_autoSaveCheck  = new QCheckBox("Auto save to folder");
    m_showCursorCheck = new QCheckBox("Include cursor");
    m_soundCheck     = new QCheckBox("Capture sound");
    for (auto *cb : {m_autoCopyCheck, m_autoSaveCheck, m_showCursorCheck, m_soundCheck}) {
        cb->setObjectName("optCheck"); optsLay->addWidget(cb);
    }
    m_autoCopyCheck->setChecked(SM.autoCopy());
    m_autoSaveCheck->setChecked(SM.autoSave());
    m_soundCheck->setChecked(SM.captureSound());

    connect(m_autoCopyCheck,  &QCheckBox::toggled, [](bool v){ SM.set("autoCopy", v); });
    connect(m_autoSaveCheck,  &QCheckBox::toggled, [](bool v){ SM.set("autoSave", v); });
    connect(m_soundCheck,     &QCheckBox::toggled, [](bool v){ SM.set("captureSound", v); });

    leftLay->addWidget(optsBox);
    leftLay->addStretch();

    // Right: Preview + actions
    QWidget *rightPanel = new QWidget();
    QVBoxLayout *rightLay = new QVBoxLayout(rightPanel);
    rightLay->setSpacing(16);
    rightLay->setContentsMargins(0,0,0,0);

    QLabel *prevTitle = new QLabel(TR("Preview"));
    prevTitle->setObjectName("sectionTitle");
    m_previewTitleLabel = prevTitle;
    rightLay->addWidget(prevTitle);

    // Preview frame
    QWidget *previewFrame = new QWidget();
    previewFrame->setObjectName("previewFrame");
    previewFrame->setMinimumHeight(380);
    QVBoxLayout *prevLay = new QVBoxLayout(previewFrame);
    prevLay->setContentsMargins(4,4,4,4);

    m_previewLabel = new QLabel("No capture yet\n\nClick a capture mode to start");
    m_previewLabel->setObjectName("previewLabel");
    m_previewLabel->setAlignment(Qt::AlignCenter);
    m_previewLabel->setScaledContents(false);
    m_previewLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    prevLay->addWidget(m_previewLabel);

    rightLay->addWidget(previewFrame, 1);

    // Action toolbar
    QWidget *actionBar = new QWidget();
    actionBar->setObjectName("actionBar");
    QHBoxLayout *abLay = new QHBoxLayout(actionBar);
    abLay->setContentsMargins(12,10,12,10);
    abLay->setSpacing(8);

    auto makeActBtn = [&](const QString &emoji, const QString &tip) -> QPushButton* {
        QPushButton *b = new QPushButton(emoji);
        b->setObjectName("actionBtn");
        b->setToolTip(tip);
        b->setFixedSize(44, 44);
        b->setCursor(Qt::PointingHandCursor);
        return b;
    };

    QPushButton *copyBtn     = makeActBtn("📋", "Copy to Clipboard");
    QPushButton *saveBtn     = makeActBtn("💾", "Save As...");
    QPushButton *annotateBtn = makeActBtn("✏️", "Annotate");
    QPushButton *ocrBtn      = makeActBtn("🔍", "OCR — Extract Text");
    QPushButton *shareBtn    = makeActBtn("☁️", "Upload");
    QPushButton *trashBtn    = makeActBtn("🗑️", "Discard");

    connect(copyBtn,     &QPushButton::clicked, this, &MainWindow::onCopyToClipboard);
    connect(saveBtn,     &QPushButton::clicked, this, &MainWindow::onSaveAs);
    connect(annotateBtn, &QPushButton::clicked, this, &MainWindow::onAnnotateCapture);
    connect(ocrBtn,      &QPushButton::clicked, this, &MainWindow::onOcrCapture);
    connect(shareBtn,    &QPushButton::clicked, this, &MainWindow::onUploadToCloud);
    connect(trashBtn,    &QPushButton::clicked, this, [this]() {
        m_lastCapture = QPixmap();
        m_previewLabel->setPixmap(QPixmap());
        m_previewLabel->setText("No capture yet\n\nClick a capture mode to start");
    });

    abLay->addWidget(copyBtn);
    abLay->addWidget(saveBtn);
    abLay->addWidget(annotateBtn);
    abLay->addWidget(ocrBtn);
    abLay->addWidget(shareBtn);
    abLay->addStretch();
    abLay->addWidget(trashBtn);

    rightLay->addWidget(actionBar);

    // Info bar
    QLabel *infoBar = new QLabel(TR("Tip: Use Ctrl+Shift+S for instant region capture anywhere"));
    infoBar->setObjectName("infoBar");
    infoBar->setAlignment(Qt::AlignCenter);
    m_infoBarLabel = infoBar;
    rightLay->addWidget(infoBar);

    mainLay->addWidget(leftPanel);
    mainLay->addWidget(rightPanel, 1);
}

void MainWindow::buildRecordingPage(QWidget *page) {
    QHBoxLayout *mainLay = new QHBoxLayout(page);
    mainLay->setContentsMargins(24, 24, 24, 24);
    mainLay->setSpacing(20);

    // Left controls
    QWidget *leftPanel = new QWidget();
    leftPanel->setObjectName("leftPanel");
    leftPanel->setFixedWidth(280);
    QVBoxLayout *leftLay = new QVBoxLayout(leftPanel);
    leftLay->setSpacing(12);
    leftLay->setContentsMargins(0,0,0,0);

    QLabel *recTitle = new QLabel(TR("Recording Controls"));
    recTitle->setObjectName("sectionTitle");
    m_recTitleLabel = recTitle;
    leftLay->addWidget(recTitle);

    // Main record button
    m_startRecBtn = new QPushButton("⏺  Start Recording");
    m_startRecBtn->setObjectName("startRecordBtn");
    m_startRecBtn->setFixedHeight(56);
    m_startRecBtn->setCursor(Qt::PointingHandCursor);
    connect(m_startRecBtn, &QPushButton::clicked, this, &MainWindow::onStartRecording);
    leftLay->addWidget(m_startRecBtn);

    QHBoxLayout *recCtrlRow = new QHBoxLayout();
    m_pauseRecBtn = new QPushButton("⏸  Pause");
    m_pauseRecBtn->setObjectName("pauseRecordBtn");
    m_pauseRecBtn->setEnabled(false);
    m_pauseRecBtn->setFixedHeight(44);
    m_pauseRecBtn->setCursor(Qt::PointingHandCursor);
    connect(m_pauseRecBtn, &QPushButton::clicked, this, &MainWindow::onPauseRecording);

    m_stopRecBtn = new QPushButton("⏹  Stop");
    m_stopRecBtn->setObjectName("stopRecordBtn");
    m_stopRecBtn->setEnabled(false);
    m_stopRecBtn->setFixedHeight(44);
    m_stopRecBtn->setCursor(Qt::PointingHandCursor);
    connect(m_stopRecBtn, &QPushButton::clicked, this, &MainWindow::onStopRecording);

    recCtrlRow->addWidget(m_pauseRecBtn);
    recCtrlRow->addWidget(m_stopRecBtn);
    leftLay->addLayout(recCtrlRow);

    // Recording options
    QGroupBox *recOptsBox = new QGroupBox("Recording Options");
    recOptsBox->setObjectName("optionsBox");
    QVBoxLayout *recOptsLay = new QVBoxLayout(recOptsBox);
    recOptsLay->setSpacing(10);

    // FPS
    QHBoxLayout *fpsRow = new QHBoxLayout();
    QLabel *fpsLbl = new QLabel("Frame Rate:");
    fpsLbl->setObjectName("optLabel");
    m_recFpsCombo = new QComboBox();
    m_recFpsCombo->setObjectName("optCombo");
    m_recFpsCombo->addItems({"15 FPS", "24 FPS", "30 FPS", "60 FPS"});
    m_recFpsCombo->setCurrentIndex(2);
    fpsRow->addWidget(fpsLbl);
    fpsRow->addWidget(m_recFpsCombo);
    recOptsLay->addLayout(fpsRow);

    // Quality
    QHBoxLayout *rqRow = new QHBoxLayout();
    QLabel *rqLbl = new QLabel("Quality:");
    rqLbl->setObjectName("optLabel");
    m_recQualityCombo = new QComboBox();
    m_recQualityCombo->setObjectName("optCombo");
    m_recQualityCombo->addItems({"Low (Fast)", "Medium", "High", "Ultra HD"});
    m_recQualityCombo->setCurrentIndex(2);
    rqRow->addWidget(rqLbl);
    rqRow->addWidget(m_recQualityCombo);
    recOptsLay->addLayout(rqRow);

    m_recAudioCheck   = new QCheckBox("Record system audio");
    m_recCursorCheck  = new QCheckBox("Show cursor");
    m_recAnnotateCheck = new QCheckBox("Enable live annotation");
    m_recCursorCheck->setChecked(true);
    for (auto *cb : {m_recAudioCheck, m_recCursorCheck, m_recAnnotateCheck}) {
        cb->setObjectName("optCheck"); recOptsLay->addWidget(cb);
    }

    leftLay->addWidget(recOptsBox);

    // Region for recording
    QGroupBox *recRegionBox = new QGroupBox("Capture Area");
    recRegionBox->setObjectName("optionsBox");
    QVBoxLayout *rrLay = new QVBoxLayout(recRegionBox);

    QPushButton *selRegionBtn = new QPushButton("🖱️  Select Region");
    selRegionBtn->setObjectName("secondaryBtn");
    selRegionBtn->setFixedHeight(40);
    selRegionBtn->setCursor(Qt::PointingHandCursor);

    QPushButton *fullScreenRecBtn = new QPushButton("🖥️  Full Screen");
    fullScreenRecBtn->setObjectName("secondaryBtn");
    fullScreenRecBtn->setFixedHeight(40);
    fullScreenRecBtn->setCursor(Qt::PointingHandCursor);

    QPushButton *windowRecBtn = new QPushButton("🪟  Specific Window");
    windowRecBtn->setObjectName("secondaryBtn");
    windowRecBtn->setFixedHeight(40);
    windowRecBtn->setCursor(Qt::PointingHandCursor);

    // Recording area label shows current selection
    QLabel *areaLabel = new QLabel("Area: Full Screen");
    areaLabel->setObjectName("infoBar");
    areaLabel->setAlignment(Qt::AlignCenter);

    connect(selRegionBtn, &QPushButton::clicked, this, [this, areaLabel]() {
        if (m_isRecording) {
            QMessageBox::information(this,"Recording","Stop recording first, then select a region.");
            return;
        }
        hide();
        QTimer::singleShot(400, this, [this, areaLabel]() {
            QScreen *screen = QGuiApplication::primaryScreen();
            QRect total;
            for (auto *s : QGuiApplication::screens()) total = total.united(s->geometry());
            QPixmap bg = screen->grabWindow(0, total.x(), total.y(), total.width(), total.height());
            // Use autoCapture=false so user presses Enter to confirm
            RegionSelector *sel = new RegionSelector(false, bg, nullptr);
            connect(sel, &RegionSelector::regionSelected, this, [this, areaLabel](const QRect &r) {
                m_recorder->setRegion(r);
                areaLabel->setText(QString("✅ Region set: %1 × %2 px").arg(r.width()).arg(r.height()));
                show(); raise();
            });
            connect(sel, &RegionSelector::regionCropped, this, [this](const QPixmap &){
                show(); raise();
            });
            connect(sel, &RegionSelector::cancelled, this, [this]() { show(); raise(); });
            sel->show();
        });
    });

    connect(fullScreenRecBtn, &QPushButton::clicked, this, [this, areaLabel]() {
        m_recorder->setFullScreen();
        areaLabel->setText("Area: Full Screen");
    });

    connect(windowRecBtn, &QPushButton::clicked, this, [this, areaLabel]() {
        m_recorder->selectWindow();
        areaLabel->setText("Area: Active Window");
    });

    rrLay->addWidget(selRegionBtn);
    rrLay->addWidget(fullScreenRecBtn);
    rrLay->addWidget(windowRecBtn);
    rrLay->addWidget(areaLabel);
    leftLay->addWidget(recRegionBox);
    leftLay->addStretch();

    // Right: Recording status/preview
    QWidget *rightPanel = new QWidget();
    QVBoxLayout *rightLay = new QVBoxLayout(rightPanel);
    rightLay->setSpacing(16);
    rightLay->setContentsMargins(0,0,0,0);

    QLabel *statusTitle = new QLabel("Recording Status");
    statusTitle->setObjectName("sectionTitle");
    rightLay->addWidget(statusTitle);

    // Status card
    QWidget *statusCard = new QWidget();
    statusCard->setObjectName("statusCard");
    QVBoxLayout *scLay = new QVBoxLayout(statusCard);
    scLay->setAlignment(Qt::AlignCenter);
    scLay->setSpacing(16);

    QLabel *camIcon = new QLabel("🎬");
    camIcon->setObjectName("bigIcon");
    camIcon->setAlignment(Qt::AlignCenter);
    scLay->addWidget(camIcon);

    QLabel *readyLbl = new QLabel("Ready to Record");
    readyLbl->setObjectName("statusCardTitle");
    readyLbl->setAlignment(Qt::AlignCenter);
    scLay->addWidget(readyLbl);

    QLabel *readyDesc = new QLabel("Choose your capture area and\npress Start Recording");
    readyDesc->setObjectName("statusCardDesc");
    readyDesc->setAlignment(Qt::AlignCenter);
    scLay->addWidget(readyDesc);

    rightLay->addWidget(statusCard, 1);

    // Recent recordings
    QLabel *recListTitle = new QLabel("Recent Recordings");
    recListTitle->setObjectName("sectionTitle");
    rightLay->addWidget(recListTitle);

    QListWidget *recList = new QListWidget();
    recList->setObjectName("historyList");
    recList->setFixedHeight(150);
    recList->addItem("No recordings yet");
    rightLay->addWidget(recList);

    mainLay->addWidget(leftPanel);
    mainLay->addWidget(rightPanel, 1);
}

void MainWindow::buildHistoryPage(QWidget *page) {
    QVBoxLayout *lay = new QVBoxLayout(page);
    lay->setContentsMargins(24, 24, 24, 24);
    lay->setSpacing(16);

    QHBoxLayout *topRow = new QHBoxLayout();

    QLabel *histTitle = new QLabel(TR("Capture History"));
    histTitle->setObjectName("sectionTitle");

    QPushButton *openFolderBtn = new QPushButton("📁  " + TR("Open Folder"));
    openFolderBtn->setObjectName("secondaryBtn");
    openFolderBtn->setFixedHeight(36);

    QPushButton *clearBtn = new QPushButton("🗑️  " + TR("Clear All"));
    clearBtn->setObjectName("dangerBtn");
    clearBtn->setFixedHeight(36);

    topRow->addWidget(histTitle);
    topRow->addStretch();
    topRow->addWidget(openFolderBtn);
    topRow->addWidget(clearBtn);
    lay->addLayout(topRow);

    m_historyPanel = new HistoryPanel(m_captureManager, this);
    lay->addWidget(m_historyPanel, 1);

    // Wire Clear All to panel
    connect(clearBtn, &QPushButton::clicked,
            m_historyPanel, &HistoryPanel::clearAll);

    // Wire Open Folder to panel
    connect(openFolderBtn, &QPushButton::clicked,
            m_historyPanel, &HistoryPanel::onOpenFolderClicked);
}

void MainWindow::buildSettingsPage(QWidget *page) {
    QVBoxLayout *lay = new QVBoxLayout(page);
    lay->setContentsMargins(24, 24, 24, 24);
    lay->setSpacing(16);

    QLabel *settTitle = new QLabel("Settings");
    settTitle->setObjectName("sectionTitle");
    lay->addWidget(settTitle);

    QScrollArea *scroll = new QScrollArea();
    scroll->setWidgetResizable(true);
    scroll->setObjectName("settingsScroll");
    QWidget *scrollContent = new QWidget();
    QVBoxLayout *scLay = new QVBoxLayout(scrollContent);
    scLay->setSpacing(16);
    scLay->setContentsMargins(4, 4, 4, 4);

    // ── 1. REGION CAPTURE BEHAVIOUR ──────────────────────────────
    QGroupBox *regionBox = new QGroupBox("✂️  Region Capture Behaviour");
    regionBox->setObjectName("settingsGroup");
    QVBoxLayout *regionLay = new QVBoxLayout(regionBox);
    regionLay->setSpacing(8);

    QRadioButton *enterRadio = new QRadioButton(
        "Select region → press Enter to capture  (default)");
    QRadioButton *autoRadio  = new QRadioButton(
        "Select region → capture automatically on mouse release");
    enterRadio->setObjectName("optCheck");
    autoRadio->setObjectName("optCheck");

    QSettings s;
    bool autoCapture = s.value("regionAutoCapture", false).toBool();
    autoRadio->setChecked(autoCapture);
    enterRadio->setChecked(!autoCapture);

    connect(autoRadio,  &QRadioButton::toggled, this, [](bool on) {
        QSettings ss; ss.setValue("regionAutoCapture", on); });
    connect(enterRadio, &QRadioButton::toggled, this, [](bool on) {
        QSettings ss; ss.setValue("regionAutoCapture", !on); });

    regionLay->addWidget(enterRadio);
    regionLay->addWidget(autoRadio);

    QCheckBox *showMagCheck = new QCheckBox("Show magnifier / color picker while selecting");
    QCheckBox *showDimCheck = new QCheckBox("Show dimensions tooltip while selecting");
    QCheckBox *rememberCheck = new QCheckBox("Remember last region selection");
    showMagCheck->setObjectName("optCheck");
    showDimCheck->setObjectName("optCheck");
    rememberCheck->setObjectName("optCheck");
    showMagCheck->setChecked(s.value("showMagnifier", true).toBool());
    showDimCheck->setChecked(s.value("showDimensions", true).toBool());
    rememberCheck->setChecked(s.value("rememberRegion", false).toBool());
    connect(showMagCheck,  &QCheckBox::toggled, [](bool v){ QSettings ss; ss.setValue("showMagnifier",v); });
    connect(showDimCheck,  &QCheckBox::toggled, [](bool v){ QSettings ss; ss.setValue("showDimensions",v); });
    connect(rememberCheck, &QCheckBox::toggled, [](bool v){ QSettings ss; ss.setValue("rememberRegion",v); });
    regionLay->addWidget(showMagCheck);
    regionLay->addWidget(showDimCheck);
    regionLay->addWidget(rememberCheck);
    scLay->addWidget(regionBox);

    // ── 2. CAPTURE BEHAVIOUR ─────────────────────────────────────
    QGroupBox *capBox = new QGroupBox("📸  After Capture");
    capBox->setObjectName("settingsGroup");
    QVBoxLayout *capLay = new QVBoxLayout(capBox);
    capLay->setSpacing(8);

    QCheckBox *autoCopyS   = new QCheckBox("Auto copy to clipboard");
    QCheckBox *autoSaveS   = new QCheckBox("Auto save to folder");
    QCheckBox *autoAnnotS  = new QCheckBox("Open annotation editor automatically");
    QCheckBox *autoPreviewS= new QCheckBox("Show preview window after capture");
    QCheckBox *miniAfterS  = new QCheckBox("Minimize app after capture");
    QCheckBox *flashS      = new QCheckBox("Show screen flash on capture");
    QCheckBox *soundS      = new QCheckBox("Play shutter sound");
    QCheckBox *notifyS     = new QCheckBox("Show desktop notification");

    struct CBDef { QCheckBox *cb; const char *key; bool def; };
    QList<CBDef> capChecks = {
        {autoCopyS,    "autoCopy",     true },
        {autoSaveS,    "autoSave",     false},
        {autoAnnotS,   "autoAnnotate", false},
        {autoPreviewS, "autoPreview",  false},
        {miniAfterS,   "miniAfter",    false},
        {flashS,       "flashEffect",  true },
        {soundS,       "captureSound", true },
        {notifyS,      "notify",       true },
    };
    for (auto &item : capChecks) {
        item.cb->setObjectName("optCheck");
        item.cb->setChecked(s.value(item.key, item.def).toBool());
        QString k = item.key;
        connect(item.cb, &QCheckBox::toggled, [k](bool v){ QSettings ss; ss.setValue(k, v); });
        capLay->addWidget(item.cb);
    }
    scLay->addWidget(capBox);

    // ── 3. SAVE & EXPORT ─────────────────────────────────────────
    QGroupBox *saveBox = new QGroupBox("💾  Save & Export");
    saveBox->setObjectName("settingsGroup");
    QVBoxLayout *saveLay = new QVBoxLayout(saveBox);
    saveLay->setSpacing(10);

    auto makeRow = [](const QString &label, QWidget *ctrl) {
        QHBoxLayout *row = new QHBoxLayout();
        QLabel *lbl = new QLabel(label);
        lbl->setObjectName("optLabel");
        lbl->setFixedWidth(140);
        row->addWidget(lbl);
        row->addWidget(ctrl, 1);
        return row;
    };

    QLineEdit *pathEdit = new QLineEdit(s.value("savePath",
        QStandardPaths::writableLocation(QStandardPaths::PicturesLocation)).toString());
    pathEdit->setObjectName("settingsLineEdit");
    QPushButton *browseBtn = new QPushButton("Browse");
    browseBtn->setObjectName("secondaryBtn");
    browseBtn->setFixedWidth(80);
    connect(browseBtn, &QPushButton::clicked, this, [pathEdit]() {
        QString dir = QFileDialog::getExistingDirectory(nullptr, "Select Save Folder");
        if (!dir.isEmpty()) { pathEdit->setText(dir);
            QSettings ss; ss.setValue("savePath", dir); }
    });
    QHBoxLayout *pathRow = new QHBoxLayout();
    QLabel *pathLbl = new QLabel("Save folder:"); pathLbl->setObjectName("optLabel"); pathLbl->setFixedWidth(140);
    pathRow->addWidget(pathLbl); pathRow->addWidget(pathEdit,1); pathRow->addWidget(browseBtn);
    saveLay->addLayout(pathRow);

    QLineEdit *nameEdit = new QLineEdit(s.value("filenameTemplate","Screenshot_%Y%m%d_%H%M%S").toString());
    nameEdit->setObjectName("settingsLineEdit");
    connect(nameEdit, &QLineEdit::textChanged, [](const QString &v){ QSettings ss; ss.setValue("filenameTemplate",v); });
    saveLay->addLayout(makeRow("Filename template:", nameEdit));

    QComboBox *fmtCombo = new QComboBox(); fmtCombo->setObjectName("optCombo");
    fmtCombo->addItems({"PNG", "JPG", "BMP", "WebP", "TIFF"});
    fmtCombo->setCurrentText(s.value("format","PNG").toString());
    connect(fmtCombo, &QComboBox::currentTextChanged, [](const QString &v){ QSettings ss; ss.setValue("format",v); });
    saveLay->addLayout(makeRow("Default format:", fmtCombo));

    QSpinBox *qualSpin = new QSpinBox(); qualSpin->setObjectName("optSpinBox");
    qualSpin->setRange(10,100); qualSpin->setValue(s.value("quality",95).toInt()); qualSpin->setSuffix("%");
    connect(qualSpin, QOverload<int>::of(&QSpinBox::valueChanged), [](int v){ QSettings ss; ss.setValue("quality",v); });
    saveLay->addLayout(makeRow("JPEG quality:", qualSpin));

    QCheckBox *addShadowS  = new QCheckBox("Add drop shadow to screenshots");
    QCheckBox *addBorderS  = new QCheckBox("Add border around screenshots");
    QCheckBox *watermarkS  = new QCheckBox("Add watermark to screenshots");
    addShadowS->setObjectName("optCheck"); addShadowS->setChecked(s.value("addShadow",false).toBool());
    addBorderS->setObjectName("optCheck"); addBorderS->setChecked(s.value("addBorder",false).toBool());
    watermarkS->setObjectName("optCheck"); watermarkS->setChecked(s.value("watermark",false).toBool());
    connect(addShadowS,&QCheckBox::toggled,[](bool v){QSettings ss;ss.setValue("addShadow",v);});
    connect(addBorderS,&QCheckBox::toggled,[](bool v){QSettings ss;ss.setValue("addBorder",v);});
    connect(watermarkS,&QCheckBox::toggled,[](bool v){QSettings ss;ss.setValue("watermark",v);});
    saveLay->addWidget(addShadowS);
    saveLay->addWidget(addBorderS);
    saveLay->addWidget(watermarkS);
    scLay->addWidget(saveBox);

    // ── 4. TOOLS & ANNOTATION ────────────────────────────────────
    QGroupBox *toolBox = new QGroupBox("✏️  Tools & Annotation");
    toolBox->setObjectName("settingsGroup");
    QVBoxLayout *toolLay = new QVBoxLayout(toolBox);
    toolLay->setSpacing(8);

    QSpinBox *defStroke = new QSpinBox(); defStroke->setObjectName("optSpinBox");
    defStroke->setRange(1,20); defStroke->setValue(s.value("defaultStroke",3).toInt());
    connect(defStroke,QOverload<int>::of(&QSpinBox::valueChanged),[](int v){QSettings ss;ss.setValue("defaultStroke",v);});
    toolLay->addLayout(makeRow("Default stroke width:", defStroke));

    QComboBox *defTool = new QComboBox(); defTool->setObjectName("optCombo");
    defTool->addItems({"Arrow","Rectangle","Ellipse","Freehand","Text","Highlight","Blur"});
    defTool->setCurrentText(s.value("defaultTool","Arrow").toString());
    connect(defTool,&QComboBox::currentTextChanged,[](const QString &v){QSettings ss;ss.setValue("defaultTool",v);});
    toolLay->addLayout(makeRow("Default tool:", defTool));

    QCheckBox *antiAlias  = new QCheckBox("Anti-aliased drawing");
    QCheckBox *undoLevels = new QCheckBox("Unlimited undo history");
    antiAlias->setObjectName("optCheck");  antiAlias->setChecked(s.value("antiAlias",true).toBool());
    undoLevels->setObjectName("optCheck"); undoLevels->setChecked(s.value("unlimitedUndo",true).toBool());
    connect(antiAlias,  &QCheckBox::toggled,[](bool v){QSettings ss;ss.setValue("antiAlias",v);});
    connect(undoLevels, &QCheckBox::toggled,[](bool v){QSettings ss;ss.setValue("unlimitedUndo",v);});
    toolLay->addWidget(antiAlias);
    toolLay->addWidget(undoLevels);
    scLay->addWidget(toolBox);

    // ── 5. RECORDING ─────────────────────────────────────────────
    QGroupBox *recBox = new QGroupBox("🎬  Recording");
    recBox->setObjectName("settingsGroup");
    QVBoxLayout *recLay = new QVBoxLayout(recBox);
    recLay->setSpacing(10);

    QComboBox *recFps = new QComboBox(); recFps->setObjectName("optCombo");
    recFps->addItems({"15 FPS","24 FPS","30 FPS","60 FPS"});
    recFps->setCurrentText(s.value("recFps","30 FPS").toString());
    connect(recFps,&QComboBox::currentTextChanged,[](const QString &v){QSettings ss;ss.setValue("recFps",v);});
    recLay->addLayout(makeRow("Default FPS:", recFps));

    QComboBox *recFmt = new QComboBox(); recFmt->setObjectName("optCombo");
    recFmt->addItems({"Image Sequence (PNG)","Image Sequence (JPG)"});
    recFmt->setCurrentText(s.value("recFormat","Image Sequence (PNG)").toString());
    connect(recFmt,&QComboBox::currentTextChanged,[](const QString &v){QSettings ss;ss.setValue("recFormat",v);});
    recLay->addLayout(makeRow("Output format:", recFmt));

    QLineEdit *recPath = new QLineEdit(s.value("recPath",
        QStandardPaths::writableLocation(QStandardPaths::MoviesLocation)).toString());
    recPath->setObjectName("settingsLineEdit");
    QPushButton *recBrowse = new QPushButton("Browse"); recBrowse->setObjectName("secondaryBtn"); recBrowse->setFixedWidth(80);
    connect(recBrowse,&QPushButton::clicked,this,[recPath](){
        QString d=QFileDialog::getExistingDirectory(nullptr,"Select Recordings Folder");
        if(!d.isEmpty()){recPath->setText(d);QSettings ss;ss.setValue("recPath",d);}});
    QHBoxLayout *recPathRow=new QHBoxLayout();
    QLabel *rpl=new QLabel("Save folder:"); rpl->setObjectName("optLabel"); rpl->setFixedWidth(140);
    recPathRow->addWidget(rpl); recPathRow->addWidget(recPath,1); recPathRow->addWidget(recBrowse);
    recLay->addLayout(recPathRow);

    QCheckBox *recCursor = new QCheckBox("Show cursor in recording");
    QCheckBox *recCountdown = new QCheckBox("3-second countdown before recording starts");
    QCheckBox *recNotify = new QCheckBox("Show recording badge on screen");
    recCursor->setObjectName("optCheck");    recCursor->setChecked(s.value("recCursor",true).toBool());
    recCountdown->setObjectName("optCheck"); recCountdown->setChecked(s.value("recCountdown",true).toBool());
    recNotify->setObjectName("optCheck");    recNotify->setChecked(s.value("recNotify",true).toBool());
    connect(recCursor,    &QCheckBox::toggled,[](bool v){QSettings ss;ss.setValue("recCursor",v);});
    connect(recCountdown, &QCheckBox::toggled,[](bool v){QSettings ss;ss.setValue("recCountdown",v);});
    connect(recNotify,    &QCheckBox::toggled,[](bool v){QSettings ss;ss.setValue("recNotify",v);});
    recLay->addWidget(recCursor);
    recLay->addWidget(recCountdown);
    recLay->addWidget(recNotify);
    scLay->addWidget(recBox);

    // ── 6. HOTKEYS ───────────────────────────────────────────────
    QGroupBox *hkBox = new QGroupBox("⌨️  Keyboard Shortcuts");
    hkBox->setObjectName("settingsGroup");
    QGridLayout *hkGrid = new QGridLayout(hkBox);
    hkGrid->setSpacing(8);
    hkGrid->setColumnStretch(0,1);

    struct HK { QString action; QString key; };
    QList<HK> hks = {
        {"Full Screen Capture",      "Print Screen"},
        {"Region Capture",           "Ctrl+Shift+S"},
        {"Active Window Capture",    "Alt+Print Screen"},
        {"Scrolling Capture",        "Ctrl+Shift+W"},
        {"Timed Capture",            "Ctrl+Shift+T"},
        {"Start / Stop Recording",   "Ctrl+Shift+R"},
        {"Color Picker",             "Ctrl+Shift+C"},
        {"Copy Last Capture",        "Ctrl+C"},
        {"Save As",                  "Ctrl+S"},
        {"Open Annotation Editor",   "Ctrl+E"},
    };
    for (int i=0;i<hks.size();++i) {
        QLabel *al=new QLabel(hks[i].action); al->setObjectName("optLabel");
        QLineEdit *ke=new QLineEdit(hks[i].key); ke->setObjectName("hotkeyEdit"); ke->setFixedWidth(200);
        hkGrid->addWidget(al,i,0); hkGrid->addWidget(ke,i,1);
    }
    scLay->addWidget(hkBox);

    // ── 7. APPEARANCE ────────────────────────────────────────────
    QGroupBox *appBox = new QGroupBox("🎨  Appearance");
    appBox->setObjectName("settingsGroup");
    QVBoxLayout *appLay = new QVBoxLayout(appBox);
    appLay->setSpacing(8);

    QComboBox *themeCombo = new QComboBox(); themeCombo->setObjectName("optCombo");
    themeCombo->addItems({"Dark (default)","Dark Blue","Dark Purple","OLED Black"});
    themeCombo->setCurrentText(s.value("theme","Dark (default)").toString());
    connect(themeCombo, &QComboBox::currentTextChanged, this, [this](const QString &v){
        QSettings ss; ss.setValue("theme", v);
        applyTheme(v);
    });
    appLay->addLayout(makeRow("Theme:", themeCombo));

    QComboBox *langCombo = new QComboBox(); langCombo->setObjectName("optCombo");
    langCombo->addItems({"English","Bosanski / BCS","Deutsch","Français","Español"});
    langCombo->setCurrentText(s.value("language","English").toString());
    connect(langCombo, &QComboBox::currentTextChanged, this, [this](const QString &v){
        QSettings ss; ss.setValue("language", v);
        Translator::instance().setLanguage(v);
    });
    appLay->addLayout(makeRow("Language:", langCombo));

    QCheckBox *startTrayS = new QCheckBox("Start minimized to system tray");
    QCheckBox *startupS   = new QCheckBox("Launch on Windows startup");
    QCheckBox *alwaysTop  = new QCheckBox("Always keep window on top");
    startTrayS->setObjectName("optCheck"); startTrayS->setChecked(s.value("startTray",false).toBool());
    startupS->setObjectName("optCheck");   startupS->setChecked(s.value("launchOnStartup",false).toBool());
    alwaysTop->setObjectName("optCheck");  alwaysTop->setChecked(s.value("alwaysOnTop",false).toBool());
    connect(startTrayS,&QCheckBox::toggled,[](bool v){QSettings ss;ss.setValue("startTray",v);});
    connect(startupS,  &QCheckBox::toggled,[](bool v){QSettings ss;ss.setValue("launchOnStartup",v);});
    connect(alwaysTop, &QCheckBox::toggled,[](bool v){QSettings ss;ss.setValue("alwaysOnTop",v);});
    appLay->addWidget(startTrayS);
    appLay->addWidget(startupS);
    appLay->addWidget(alwaysTop);
    scLay->addWidget(appBox);

    // ── 8. ADVANCED ──────────────────────────────────────────────
    QGroupBox *advBox = new QGroupBox("🔧  Advanced");
    advBox->setObjectName("settingsGroup");
    QVBoxLayout *advLay = new QVBoxLayout(advBox);
    advLay->setSpacing(8);

    QSpinBox *histSpin = new QSpinBox(); histSpin->setObjectName("optSpinBox");
    histSpin->setRange(10,500); histSpin->setValue(s.value("historyLimit",100).toInt()); histSpin->setSuffix(" items");
    advLay->addLayout(makeRow("History limit:", histSpin));

    QCheckBox *gpuCheck   = new QCheckBox("Use GPU acceleration (if available)");
    QCheckBox *debugCheck = new QCheckBox("Enable debug logging");
    gpuCheck->setObjectName("optCheck");   gpuCheck->setChecked(s.value("gpuAccel",true).toBool());
    debugCheck->setObjectName("optCheck"); debugCheck->setChecked(s.value("debugLog",false).toBool());
    connect(gpuCheck,   &QCheckBox::toggled,[](bool v){QSettings ss;ss.setValue("gpuAccel",v);});
    connect(debugCheck, &QCheckBox::toggled,[](bool v){QSettings ss;ss.setValue("debugLog",v);});
    advLay->addWidget(gpuCheck);
    advLay->addWidget(debugCheck);

    QPushButton *clearHistBtn = new QPushButton("🗑  Clear Capture History");
    clearHistBtn->setObjectName("dangerBtn"); clearHistBtn->setFixedHeight(36);
    QPushButton *resetAllBtn  = new QPushButton("↺  Reset All Settings to Default");
    resetAllBtn->setObjectName("dangerBtn"); resetAllBtn->setFixedHeight(36);
    connect(resetAllBtn,&QPushButton::clicked,this,[this](){
        QSettings ss; ss.clear();
        QMessageBox::information(this,"Reset","All settings reset to defaults. Restart to apply."); });
    advLay->addWidget(clearHistBtn);
    advLay->addWidget(resetAllBtn);
    scLay->addWidget(advBox);

    scLay->addStretch();
    scroll->setWidget(scrollContent);
    lay->addWidget(scroll, 1);

    // Save button row
    QHBoxLayout *saveBtnRow = new QHBoxLayout();
    QLabel *savedLbl = new QLabel("Settings are saved automatically");
    savedLbl->setObjectName("infoBar");
    QPushButton *closeBtn = new QPushButton("✔  Done");
    closeBtn->setObjectName("primaryBtn");
    closeBtn->setFixedHeight(44);
    closeBtn->setFixedWidth(120);
    connect(closeBtn, &QPushButton::clicked, this, [this](){
        QSettings s;
        applyTheme(s.value("theme","Dark (default)").toString());
        applyAllSettings();
        m_statusLabel->setText(TR("Settings saved ✔"));
        onNavigate(0);
    });
    saveBtnRow->addWidget(savedLbl,1);
    saveBtnRow->addWidget(closeBtn);
    lay->addLayout(saveBtnRow);
}

void MainWindow::onNavigate(int index) {
    m_pageStack->setCurrentIndex(index);
    for (int i = 0; i < m_navButtons.size(); ++i)
        m_navButtons[i]->setChecked(i == index);
}

void MainWindow::onFullScreenCapture() {

    hide();
    QTimer::singleShot(m_currentDelay * 1000 + 600, this, [this]() {
        int monitor = m_monitorCombo->currentIndex();
        m_captureManager->captureFullScreen(monitor);
        show(); raise(); activateWindow();
    });
}

void MainWindow::onRegionCapture() {
    // 2. Hide main window
    hide();
    QTimer::singleShot(500, this, [this]() {
        QScreen *screen = QGuiApplication::primaryScreen();
        QList<QScreen*> screens = QGuiApplication::screens();
        QRect totalGeom;
        for (auto *s : screens) totalGeom = totalGeom.united(s->geometry());
        QPixmap cleanScreen = screen->grabWindow(0,
            totalGeom.x(), totalGeom.y(), totalGeom.width(), totalGeom.height());
        // 5. Pass clean screen to RegionSelector (it won't grab again)
        QSettings st;
        bool autoCapture = st.value("regionAutoCapture", false).toBool();
        m_captureManager->captureRegion(autoCapture, cleanScreen);
    });
}

void MainWindow::onWindowCapture() {

    hide();
    QTimer::singleShot(600, this, [this]() {
        m_captureManager->captureWindow();
        show(); raise(); activateWindow();
    });
}

void MainWindow::onManualScrollCapture() {
    hide();
    QTimer::singleShot(300, this, [this]() {
        ManualScrollCapture *msc = new ManualScrollCapture(nullptr);
        connect(msc, &ManualScrollCapture::captureComplete,
                this, [this](const QPixmap &px) {
            QString path = m_captureManager->buildSavePathPublic("png");
            px.save(path, "PNG");
            m_captureManager->addToHistoryPublic(px, path, "PNG");
            emit m_captureManager->captureComplete(px, path);
            show(); raise(); activateWindow();
        });
        connect(msc, &ManualScrollCapture::cancelled, this, [this]() {
            show(); raise(); activateWindow();
        });
        msc->show();
    });
}

void MainWindow::onScrollCapture() {
    m_statusLabel->setText("Scroll capture: 4 second countdown, then scroll your page...");
    QTimer::singleShot(500, this, [this]() { hide(); });
    m_captureManager->captureScrolling();
}

void MainWindow::onDelayedCapture() {
    int delay = m_delaySpin->value();
    if (delay == 0) delay = 3;

    hide();
    CountdownOverlay *overlay = new CountdownOverlay(delay, nullptr);
    overlay->show();
    connect(overlay, &CountdownOverlay::finished, this, [this, overlay]() {
        overlay->close();
        overlay->deleteLater();
        int monitor = m_monitorCombo->currentIndex();
        m_captureManager->captureFullScreen(monitor);
        show(); raise(); activateWindow();
    });
}

void MainWindow::onColorPickerTool() {
    hide();
    QTimer::singleShot(200, this, [this]() {
        m_captureManager->startColorPicker();
        show(); raise(); activateWindow();
    });
}

void MainWindow::onCaptureComplete(const QPixmap &pixmap, const QString &filepath) {
    m_lastCapture = pixmap;
    m_lastSavePath = filepath;

    // Apply shadow effect if enabled
    QPixmap finalPixmap = pixmap;
    if (SM.addShadow()) {
        QPixmap shadow(pixmap.width() + 20, pixmap.height() + 20);
        shadow.fill(Qt::transparent);
        QPainter sp(&shadow);
        sp.setRenderHint(QPainter::Antialiasing);
        // Draw shadow
        sp.setBrush(QColor(0,0,0,80));
        sp.setPen(Qt::NoPen);
        for (int i = 6; i >= 1; --i) {
            sp.setOpacity(0.06 * i);
            sp.drawRoundedRect(i+2, i+2, pixmap.width(), pixmap.height(), 4, 4);
        }
        sp.setOpacity(1.0);
        sp.drawPixmap(0, 0, pixmap);
        sp.end();
        finalPixmap = shadow;
    }

    // Apply border if enabled
    if (SM.addBorder()) {
        QPixmap bordered(finalPixmap.width()+4, finalPixmap.height()+4);
        bordered.fill(Qt::transparent);
        QPainter bp(&bordered);
        bp.drawPixmap(2, 2, finalPixmap);
        bp.setPen(QPen(QColor(0,174,255), 2));
        bp.drawRect(1, 1, bordered.width()-2, bordered.height()-2);
        bp.end();
        finalPixmap = bordered;
    }

    // Apply watermark if enabled
    if (SM.watermark()) {
        QPainter wp(&finalPixmap);
        wp.setRenderHint(QPainter::Antialiasing);
        QFont wf("Arial", 14, QFont::Bold);
        wp.setFont(wf);
        wp.setOpacity(0.5);
        wp.setPen(Qt::white);
        QFontMetrics fm(wf);
        QRect wr = fm.boundingRect(SM.watermarkText());
        wp.drawText(finalPixmap.width() - wr.width() - 12,
                    finalPixmap.height() - 10, SM.watermarkText());
        wp.end();
    }

    m_lastCapture = finalPixmap;
    updatePreview(finalPixmap);
    m_statusLabel->setText(TR("Captured") + ": " + QFileInfo(filepath).fileName());

    // Clear pending pins list (old pins stay hidden — new screenshot replaces them)

    // Flash effect
    if (SM.flashEffect()) {
        FlashOverlay::flash();
    }

    // Sound - only if enabled
    if (SM.captureSound()) {
        QApplication::beep();
    }

    // Auto copy to clipboard
    if (SM.autoCopy()) {
        QApplication::clipboard()->setPixmap(finalPixmap);
    }

    // Auto save
    if (SM.autoSave()) {
        m_captureManager->saveCapture(finalPixmap, SM.format(), SM.quality());
    }

    // Notification
    if (SM.notify()) {
        showNotification(TR("Screenshot Captured"),
            QString("%1×%2px").arg(pixmap.width()).arg(pixmap.height()));
    }

    // Auto annotate
    if (SM.autoAnnotate()) {
        QTimer::singleShot(300, this, &MainWindow::onAnnotateCapture);
    }

    // Minimize after capture
    if (SM.miniAfter()) {
        QTimer::singleShot(400, this, &QMainWindow::showMinimized);
    }

    // Show main window if hidden (unless minimizing)
    if (!SM.miniAfter()) {
        show(); raise(); activateWindow();
    }
}

void MainWindow::updatePreview(const QPixmap &pixmap) {
    if (pixmap.isNull()) return;
    QSize previewSize = m_previewLabel->size();
    QPixmap scaled = pixmap.scaled(previewSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    m_previewLabel->setPixmap(scaled);
}

void MainWindow::onStartRecording() {
    // Parse FPS
    int fps = 30;
    QString fpsStr = m_recFpsCombo ? m_recFpsCombo->currentText() : "30 FPS";
    if (fpsStr.contains("15")) fps = 15;
    else if (fpsStr.contains("24")) fps = 24;
    else if (fpsStr.contains("60")) fps = 60;

    // Parse quality
    QString quality = "High";
    if (m_recQualityCombo) {
        QString q = m_recQualityCombo->currentText();
        if (q.contains("Low"))    quality = "Low";
        else if (q.contains("Medium")) quality = "Medium";
        else if (q.contains("Ultra"))  quality = "Ultra";
    }

    m_recorder->setFps(fps);
    m_recorder->setQuality(quality);
    m_recorder->setRecordAudio(m_recAudioCheck ? m_recAudioCheck->isChecked() : false);
    m_recorder->setOutputPath(SM.recPath());
    m_recorder->startRecording();
}

void MainWindow::onStopRecording() {
    m_recorder->stopRecording();
}

void MainWindow::onPauseRecording() {
    if (m_isPaused) {
        m_recorder->resumeRecording();
        m_pauseRecBtn->setText("⏸  Pause");
        m_isPaused = false;
        m_dotTimer->start(600);
    } else {
        m_recorder->pauseRecording();
        m_pauseRecBtn->setText("▶  Resume");
        m_isPaused = true;
        m_dotTimer->stop();
        m_recordingDot->setVisible(true);
    }
}

void MainWindow::updateRecordingUI(bool recording, bool paused) {
    m_isRecording = recording;
    m_startRecBtn->setEnabled(!recording);
    m_stopRecBtn->setEnabled(recording);
    m_pauseRecBtn->setEnabled(recording);
    m_recordingBadge->setVisible(recording);
    if (!recording) m_recordingTimeLabel->setText("00:00:00");
}

void MainWindow::onRecordingTick(int seconds) {
    int h = seconds/3600, m = (seconds%3600)/60, s = seconds%60;
    m_recordingTimeLabel->setText(QString("%1:%2:%3")
        .arg(h,2,10,QChar('0')).arg(m,2,10,QChar('0')).arg(s,2,10,QChar('0')));
}

void MainWindow::onCopyToClipboard() {
    if (!m_lastCapture.isNull()) {
        QApplication::clipboard()->setPixmap(m_lastCapture);
        showNotification("Copied", "Screenshot copied to clipboard.");
        m_statusLabel->setText("Copied to clipboard");
    }
}

void MainWindow::onSaveAs() {
    if (m_lastCapture.isNull()) return;
    QString fmt = m_formatCombo->currentText().toLower();
    QString filter = QString("%1 Files (*.%2);;All Files (*)").arg(m_formatCombo->currentText(), fmt);
    QString path = QFileDialog::getSaveFileName(this, "Save Screenshot",
        QDir::homePath() + "/screenshot." + fmt, filter);
    if (!path.isEmpty()) {
        m_lastCapture.save(path, fmt.toUpper().toLocal8Bit(), m_qualitySpinBox->value());
        showNotification("Saved", "Screenshot saved to " + QFileInfo(path).fileName());
    }
}

void MainWindow::onOcrCapture() {
    if (m_lastCapture.isNull()) {
        QMessageBox::information(this, "OCR",
            "Take a screenshot first, then use OCR to extract text.");
        return;
    }

    m_statusLabel->setText("Running OCR...");
    QApplication::setOverrideCursor(Qt::WaitCursor);

    OcrEngine engine;
    QString text = engine.recognize(m_lastCapture);

    QApplication::restoreOverrideCursor();
    m_statusLabel->setText("OCR complete");

    OcrDialog *dlg = new OcrDialog(m_lastCapture, text, this);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->exec();
}

void MainWindow::onAnnotateCapture() {
    if (m_lastCapture.isNull()) {
        QMessageBox::information(this, "Annotate", "Take a screenshot first, then annotate it.");
        return;
    }
    AnnotationWidget *aw = new AnnotationWidget(m_lastCapture, nullptr);
    aw->setAttribute(Qt::WA_DeleteOnClose);
    connect(aw, &AnnotationWidget::annotationComplete, this, [this](const QPixmap &px) {
        m_lastCapture = px;
        updatePreview(px);
    });
    aw->showMaximized();
}

void MainWindow::onUploadToCloud() {
    QMessageBox::information(this, "Upload", "Cloud upload integration.\nConfigure your API keys in Settings.");
}

void MainWindow::onOpenHistory() {
    onNavigate(2);
}

void MainWindow::onOpenSettings() {
    onNavigate(3);
}

void MainWindow::onDelayChanged(int v) { m_currentDelay = v; }
void MainWindow::onFormatChanged(const QString &) {}

void MainWindow::showNotification(const QString &title, const QString &msg) {
    if (m_trayManager) m_trayManager->showMessage(title, msg);
}

void MainWindow::setupShortcuts() {
    auto addSc = [this](const QKeySequence &keys, auto slot) {
        QShortcut *sc = new QShortcut(keys, this);
        connect(sc, &QShortcut::activated, this, slot);
    };
    addSc(Qt::Key_Print,                           &MainWindow::onFullScreenCapture);
    addSc(QKeySequence("Ctrl+Shift+S"),             &MainWindow::onRegionCapture);
    addSc(QKeySequence(Qt::ALT | Qt::Key_Print),   &MainWindow::onWindowCapture);
    addSc(QKeySequence("Ctrl+Shift+W"),             &MainWindow::onScrollCapture);
    addSc(QKeySequence("Ctrl+Shift+T"),             &MainWindow::onDelayedCapture);
    addSc(QKeySequence("Ctrl+Shift+C"),             &MainWindow::onColorPickerTool);
    addSc(QKeySequence("Ctrl+Shift+R"),             [this]() {
        if (m_isRecording) onStopRecording(); else onStartRecording();
    });
    addSc(QKeySequence("Ctrl+Shift+M"),             &MainWindow::onManualScrollCapture);
    addSc(QKeySequence("Ctrl+C"),  &MainWindow::onCopyToClipboard);
    addSc(QKeySequence("Ctrl+S"),  &MainWindow::onSaveAs);
}

void MainWindow::setupTray() {
    if (!m_trayManager) return;
    m_trayManager->setup(this);
    connect(m_trayManager, &TrayManager::activated,
            this, &MainWindow::onTrayActivated);
}

void MainWindow::onTrayActivated(QSystemTrayIcon::ActivationReason reason) {
    if (reason == QSystemTrayIcon::DoubleClick) {
        show(); raise(); activateWindow();
    }
}

void MainWindow::closeEvent(QCloseEvent *event) {
    if (m_isRecording) {
        auto btn = QMessageBox::question(this, "Recording Active",
            "A recording is in progress. Stop and exit?");
        if (btn == QMessageBox::No) { event->ignore(); return; }
        onStopRecording();
    }
    saveSettings();
    event->accept();
}

void MainWindow::keyPressEvent(QKeyEvent *e) {
    if (e->key() == Qt::Key_Escape && m_isRecording) onStopRecording();
    QMainWindow::keyPressEvent(e);
}

void MainWindow::resizeEvent(QResizeEvent *e) {
    QMainWindow::resizeEvent(e);
    if (!m_lastCapture.isNull() && m_previewLabel)
        updatePreview(m_lastCapture);
}

void MainWindow::saveSettings() {
    QSettings s;
    s.setValue("geometry", saveGeometry());
    if (m_formatCombo)     s.setValue("format",   m_formatCombo->currentText());
    if (m_qualitySpinBox)  s.setValue("quality",  m_qualitySpinBox->value());
    if (m_delaySpin)       s.setValue("delay",    m_delaySpin->value());
    if (m_autoCopyCheck)   s.setValue("autoCopy", m_autoCopyCheck->isChecked());
    if (m_autoSaveCheck)   s.setValue("autoSave", m_autoSaveCheck->isChecked());
}

void MainWindow::loadSettings() {
    QSettings s;
    if (s.contains("geometry")) restoreGeometry(s.value("geometry").toByteArray());
    if (m_formatCombo)    m_formatCombo->setCurrentText(SM.format());
    if (m_qualitySpinBox) m_qualitySpinBox->setValue(SM.quality());
    if (m_delaySpin)      m_delaySpin->setValue(s.value("delay",0).toInt());
    if (m_autoCopyCheck)  m_autoCopyCheck->setChecked(SM.autoCopy());
    if (m_autoSaveCheck)  m_autoSaveCheck->setChecked(SM.autoSave());
    applyTheme(SM.theme());
    // Apply settings after UI is built
    QTimer::singleShot(100, this, &MainWindow::applyAllSettings);
}

void MainWindow::retranslateUi() {
    // Sidebar nav buttons
    QStringList navKeys = {"Screenshot","Recording","History","Settings"};
    QStringList navIcons = {"🖥️ ","🎬 ","🕐 ","⚙️ "};
    for (int i = 0; i < m_navButtons.size() && i < navKeys.size(); ++i)
        m_navButtons[i]->setText(navIcons[i] + TR(navKeys[i]));

    // Capture mode buttons
    QStringList capKeys = {"Full Screen","Region","Window","Scrolling","Timed","Color Picker"};
    QStringList capIcons = {"🖥️","✂️","🪟","📜","🕐","🎨"};
    QStringList capShortcuts = {"PrtSc","Ctrl+⇧+S","Alt+PrtSc","Ctrl+⇧+W","Ctrl+⇧+T","Ctrl+⇧+C"};
    for (int i = 0; i < m_captureBtns.size() && i < capKeys.size(); ++i)
        m_captureBtns[i]->setText(capIcons[i] + "\n" + TR(capKeys[i]) + "\n" + capShortcuts[i]);

    // Labels
    if (m_captureTitleLabel) m_captureTitleLabel->setText(TR("Capture Mode"));
    if (m_previewTitleLabel) m_previewTitleLabel->setText(TR("Preview"));
    if (m_recTitleLabel)     m_recTitleLabel->setText(TR("Recording Controls"));
    if (m_infoBarLabel)      m_infoBarLabel->setText(TR("Tip: Use Ctrl+Shift+S for instant region capture anywhere"));
    if (m_statusLabel)       m_statusLabel->setText(TR("Ready"));
    if (m_historyPanel)      m_historyPanel->retranslate();

    // Window title stays English (professional standard)
    setWindowTitle("ScreenMaster Pro");
}

void MainWindow::applyAllSettings() {
    // Always on top
    if (SM.alwaysOnTop())
        setWindowFlag(Qt::WindowStaysOnTopHint, true);
    else
        setWindowFlag(Qt::WindowStaysOnTopHint, false);
    show();

    // Windows startup registry
#ifdef Q_OS_WIN
    QSettings reg("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run",
                  QSettings::NativeFormat);
    if (SM.alwaysOnTop() == false) {} // placeholder
    bool launchOnStartup = QSettings().value("launchOnStartup", false).toBool();
    if (launchOnStartup) {
        reg.setValue("ScreenMasterPro", QApplication::applicationFilePath().replace("/","\\"));
    } else {
        reg.remove("ScreenMasterPro");
    }
#endif

    // Apply save path to capture manager
    m_captureManager->setSavePath(SM.savePath());
    m_captureManager->setFilenameTemplate(SM.filenameTemplate());

    // Apply recording settings
    int fps = 30;
    QString fpsStr = SM.recFps();
    if (fpsStr.contains("15")) fps = 15;
    else if (fpsStr.contains("24")) fps = 24;
    else if (fpsStr.contains("60")) fps = 60;
    m_recorder->setFps(fps);
    m_recorder->setRecordAudio(SM.recCursor()); // reuse setting
    m_recorder->setOutputPath(SM.recPath());

    // Sync UI controls with settings
    if (m_autoCopyCheck)   m_autoCopyCheck->setChecked(SM.autoCopy());
    if (m_autoSaveCheck)   m_autoSaveCheck->setChecked(SM.autoSave());
    if (m_showCursorCheck) m_showCursorCheck->setChecked(SM.recCursor());
    if (m_formatCombo)     m_formatCombo->setCurrentText(SM.format());
    if (m_qualitySpinBox)  m_qualitySpinBox->setValue(SM.quality());
}

void MainWindow::applyStyles() {
    QSettings s;
    applyTheme(s.value("theme", "Dark (default)").toString());
}

void MainWindow::applyTheme(const QString &theme) {
    // Base accent color per theme
    QString accent   = "#00aeff";
    QString bg0      = "#12121f";
    QString bg1      = "#0f0f1e";
    QString bg2      = "#1a1a30";
    QString bg3      = "#1e1e3a";
    QString border   = "#252545";
    QString textMain = "#e2e2f0";
    QString textSub  = "#8888aa";

    if (theme == "Dark Blue") {
        accent = "#00cfff"; bg0 = "#0a1020"; bg1 = "#080e1a";
        bg2 = "#101828"; bg3 = "#162030"; border = "#1e3050";
        textSub = "#6688bb";
    } else if (theme == "Dark Purple") {
        accent = "#aa77ff"; bg0 = "#12101e"; bg1 = "#0e0c18";
        bg2 = "#1a1630"; bg3 = "#221e3a"; border = "#302850";
        textSub = "#8866bb";
    } else if (theme == "OLED Black") {
        accent = "#00ff99"; bg0 = "#000000"; bg1 = "#000000";
        bg2 = "#0a0a0a"; bg3 = "#111111"; border = "#1a1a1a";
        textSub = "#668866";
    }

    QString qss = QString(R"(
QWidget { background-color: %1; color: %6; font-family: "Segoe UI", Arial, sans-serif; font-size: 13px; }
QMainWindow { background-color: %1; }
QWidget#sidebar { background: qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 %2,stop:1 %1); border-right:1px solid %5; }
QLabel#appTitle { font-size:18px; font-weight:bold; color:#ffffff; }
QLabel#appVersion { font-size:10px; color:%4; font-weight:bold; letter-spacing:2px; }
QFrame#separator { color: %5; }
QPushButton#navButton { background:transparent; border:none; border-radius:10px; color:%7; font-size:13px; padding:0 12px; text-align:left; }
QPushButton#navButton:hover { background:rgba(0,150,255,0.10); color:#c0c0e0; }
QPushButton#navButton:checked { background:qlineargradient(x1:0,y1:0,x2:1,y2:0,stop:0 rgba(0,150,255,0.25),stop:1 rgba(0,150,255,0.05)); color:%4; border-left:3px solid %4; font-weight:bold; }
QStackedWidget#pageStack { background: %1; }
QLabel#sectionTitle { font-size:16px; font-weight:bold; color:#ffffff; padding-bottom:4px; border-bottom:2px solid %4; margin-bottom:8px; }
QPushButton#captureModeBtn { background:qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 %3,stop:1 %2); border:1px solid %5; border-radius:12px; color:#ccccee; font-size:12px; padding:8px 4px; }
QPushButton#captureModeBtn:hover { border-color:%4; color:#ffffff; }
QGroupBox#optionsBox, QGroupBox#settingsGroup { background:%3; border:1px solid %5; border-radius:10px; margin-top:12px; padding-top:8px; font-weight:bold; color:%7; font-size:12px; }
QGroupBox#optionsBox::title, QGroupBox#settingsGroup::title { subcontrol-origin:margin; subcontrol-position:top left; padding:0 8px; left:12px; color:%7; }
QLabel#optLabel { color:%7; font-size:12px; min-width:80px; }
QSpinBox#optSpinBox, QComboBox#optCombo { background:%3; border:1px solid %5; border-radius:6px; color:#d0d0f0; padding:4px 8px; }
QSpinBox#optSpinBox:focus, QComboBox#optCombo:focus { border-color:%4; }
QComboBox#optCombo QAbstractItemView { background:%3; border:1px solid %5; selection-background-color:%4; color:#d0d0f0; }
QCheckBox#optCheck, QRadioButton#optCheck { color:%7; font-size:12px; spacing:8px; }
QCheckBox#optCheck::indicator, QRadioButton#optCheck::indicator { width:16px; height:16px; border-radius:4px; border:1px solid #3a3a5a; background:%3; }
QCheckBox#optCheck::indicator:checked { background:%4; border-color:%4; }
QRadioButton#optCheck::indicator { border-radius:8px; }
QRadioButton#optCheck::indicator:checked { background:%4; border-color:%4; }
QWidget#previewFrame { background:#0d0d1e; border:1px solid %5; border-radius:12px; }
QLabel#previewLabel { color:#3a3a5a; font-size:14px; background:transparent; }
QWidget#actionBar { background:%3; border-radius:10px; border:1px solid %5; }
QPushButton#actionBtn { background:%3; border:1px solid %5; border-radius:8px; color:#ccccee; font-size:18px; }
QPushButton#actionBtn:hover { background:#2a2a50; border-color:%4; }
QLabel#infoBar { background:rgba(0,150,255,0.06); color:%7; font-size:11px; border-radius:6px; padding:6px; border:1px solid rgba(0,150,255,0.15); }
QPushButton#startRecordBtn { background:qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 #cc2222,stop:1 #aa1818); border:none; border-radius:10px; color:white; font-size:14px; font-weight:bold; }
QPushButton#startRecordBtn:hover { background:qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 #ee2222,stop:1 #cc1818); }
QPushButton#startRecordBtn:disabled { background:#3a2020; color:#663333; }
QPushButton#pauseRecordBtn { background:#2a2a1a; border:1px solid #3a3a20; border-radius:8px; color:#dddd44; font-size:13px; }
QPushButton#pauseRecordBtn:hover { background:#3a3a22; border-color:#dddd44; }
QPushButton#pauseRecordBtn:disabled { background:#1e1e14; color:#555533; }
QPushButton#stopRecordBtn { background:#1e2a1e; border:1px solid #2a3a2a; border-radius:8px; color:#88dd88; font-size:13px; }
QPushButton#stopRecordBtn:hover { background:#253025; border-color:#88dd88; }
QPushButton#stopRecordBtn:disabled { background:#141e14; color:#335533; }
QWidget#statusCard { background:%3; border:1px solid %5; border-radius:16px; }
QLabel#bigIcon, QLabel#statusCardTitle, QLabel#statusCardDesc { background:transparent; }
QLabel#statusCardTitle { font-size:18px; font-weight:bold; color:#ccccee; }
QLabel#statusCardDesc { font-size:13px; color:%7; }
QListWidget#historyList { background:%2; border:1px solid %5; border-radius:8px; color:#ccccee; }
QListWidget#historyList::item { padding:8px; border-radius:6px; }
QListWidget#historyList::item:hover { background:rgba(0,150,255,0.08); }
QListWidget#historyList::item:selected { background:rgba(0,150,255,0.20); color:white; }
QPushButton#secondaryBtn { background:%3; border:1px solid %5; border-radius:8px; color:#9999cc; padding:6px 14px; }
QPushButton#secondaryBtn:hover { background:%5; border-color:%4; color:#ccccff; }
QPushButton#primaryBtn { background:qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 #0099dd,stop:1 #007bb5); border:none; border-radius:8px; color:white; font-weight:bold; padding:8px 20px; }
QPushButton#primaryBtn:hover { background:qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 #00aaee,stop:1 #0088cc); }
QPushButton#dangerBtn { background:rgba(180,40,40,0.25); border:1px solid rgba(180,40,40,0.5); border-radius:8px; color:#ee6666; padding:6px 14px; }
QPushButton#dangerBtn:hover { background:rgba(220,50,50,0.35); color:#ffaaaa; }
QScrollArea#settingsScroll { background:transparent; border:none; }
QLineEdit#settingsLineEdit, QLineEdit#hotkeyEdit { background:%3; border:1px solid %5; border-radius:6px; color:#d0d0f0; padding:6px 10px; }
QLineEdit#settingsLineEdit:focus, QLineEdit#hotkeyEdit:focus { border-color:%4; }
QLineEdit#hotkeyEdit { font-family:"Consolas",monospace; color:#88ddff; }
QScrollBar:vertical { background:%1; width:8px; }
QScrollBar::handle:vertical { background:#2a2a4a; border-radius:4px; min-height:30px; }
QScrollBar::handle:vertical:hover { background:%4; }
QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height:0; }
QScrollBar:horizontal { background:%1; height:8px; }
QScrollBar::handle:horizontal { background:#2a2a4a; border-radius:4px; min-width:30px; }
QScrollBar::handle:horizontal:hover { background:%4; }
QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width:0; }
QToolTip { background:%3; color:#ddddff; border:1px solid %4; border-radius:6px; padding:5px 10px; }
QMessageBox { background:%3; color:%6; }
QMessageBox QPushButton { background:%3; border:1px solid %5; border-radius:6px; color:#ccccee; padding:6px 20px; min-width:80px; }
QMessageBox QPushButton:default { background:#003366; border-color:%4; color:white; }
QMenu { background:%3; color:%6; border:1px solid %5; border-radius:8px; padding:4px; }
QMenu::item { padding:8px 24px; border-radius:5px; }
QMenu::item:selected { background:rgba(0,150,255,0.25); color:white; }
QMenu::separator { height:1px; background:%5; margin:4px 8px; }
QWidget#recordingBadge { background:rgba(255,60,60,0.15); border-radius:8px; border:1px solid rgba(255,60,60,0.4); }
QLabel#recordingDot { color:#ff3c3c; font-size:16px; }
QLabel#recordingTime { color:#ff8080; font-size:12px; font-weight:bold; font-family:"Consolas",monospace; }
QLabel#statusLabel { color:%7; font-size:11px; padding:4px; }
QLabel#leftPanel { background:transparent; }
QSpinBox#optSpinBox::up-button, QSpinBox#optSpinBox::down-button { background:%5; border:none; width:16px; border-radius:3px; }
QSpinBox#optSpinBox::up-button:hover, QSpinBox#optSpinBox::down-button:hover { background:%4; }
    )").arg(bg0, bg1, bg2, accent, border, textMain, textSub);

    qApp->setStyleSheet(qss);
}
void MainWindow::setupRecordingTab() {}
void MainWindow::setupHistoryTab() {}
void MainWindow::setupSettingsTab() {}
void MainWindow::setupStatusBar() {}
