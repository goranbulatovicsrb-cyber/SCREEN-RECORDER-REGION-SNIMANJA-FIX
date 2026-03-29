#include "HistoryPanel.h"
#include "CaptureManager.h"
#include "Translator.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QListWidgetItem>
#include <QPixmap>
#include <QApplication>
#include <QClipboard>
#include <QDesktopServices>
#include <QUrl>
#include <QFileInfo>
#include <QFile>
#include <QDir>
#include <QFileDialog>
#include <QMessageBox>
#include <QDateTime>
#include <QFrame>
#include <QSettings>
#include <QStandardPaths>

HistoryPanel::HistoryPanel(CaptureManager *mgr, QWidget *parent)
    : QWidget(parent), m_manager(mgr)
{
    QHBoxLayout *mainLay = new QHBoxLayout(this);
    mainLay->setContentsMargins(0,0,0,0);
    mainLay->setSpacing(16);

    // ── Left: list ──────────────────────────────────
    QWidget *leftWidget = new QWidget();
    leftWidget->setFixedWidth(280);
    QVBoxLayout *leftLay = new QVBoxLayout(leftWidget);
    leftLay->setContentsMargins(0,0,0,0);
    leftLay->setSpacing(8);

    m_list = new QListWidget();
    m_list->setObjectName("historyList");
    m_list->setIconSize(QSize(90, 68));
    m_list->setSpacing(3);
    m_list->setAlternatingRowColors(false);
    connect(m_list, &QListWidget::itemClicked,
            this, &HistoryPanel::onItemSelected);
    connect(m_list, &QListWidget::itemDoubleClicked,
            this, &HistoryPanel::onItemDoubleClicked);

    // Empty state label
    m_emptyLabel = new QLabel(TR("No captures yet.\nTake a screenshot to start!"));
    m_emptyLabel->setObjectName("statusCardDesc");
    m_emptyLabel->setAlignment(Qt::AlignCenter);
    m_emptyLabel->setFixedHeight(80);

    leftLay->addWidget(m_list, 1);
    leftLay->addWidget(m_emptyLabel);

    // ── Right: preview + actions ─────────────────────
    QWidget *rightPanel = new QWidget();
    QVBoxLayout *rightLay = new QVBoxLayout(rightPanel);
    rightLay->setContentsMargins(0,0,0,0);
    rightLay->setSpacing(10);

    // Preview frame
    QWidget *previewFrame = new QWidget();
    previewFrame->setObjectName("previewFrame");
    QVBoxLayout *pfl = new QVBoxLayout(previewFrame);
    pfl->setContentsMargins(6,6,6,6);

    m_preview = new QLabel(TR("Select a capture to preview"));
    m_preview->setObjectName("previewLabel");
    m_preview->setAlignment(Qt::AlignCenter);
    m_preview->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    pfl->addWidget(m_preview);
    rightLay->addWidget(previewFrame, 1);

    // Info bar
    m_infoLabel = new QLabel("");
    m_infoLabel->setObjectName("infoBar");
    m_infoLabel->setAlignment(Qt::AlignCenter);
    m_infoLabel->setVisible(false);
    rightLay->addWidget(m_infoLabel);

    // Action bar
    QWidget *actBar = new QWidget();
    actBar->setObjectName("actionBar");
    QHBoxLayout *abLay = new QHBoxLayout(actBar);
    abLay->setContentsMargins(12,10,12,10);
    abLay->setSpacing(8);

    auto makeBtn = [](const QString &emoji, const QString &label,
                      const QString &tip, const QString &obj) {
        QPushButton *b = new QPushButton(emoji + "  " + label);
        b->setObjectName(obj);
        b->setToolTip(tip);
        b->setFixedHeight(40);
        b->setCursor(Qt::PointingHandCursor);
        b->setEnabled(false);
        return b;
    };

    m_copyBtn   = makeBtn("📋", TR("Copy"),        TR("Copy to clipboard"), "actionBtn");
    m_saveBtn   = makeBtn("💾", TR("Save As"),     TR("Save as file"),      "secondaryBtn");
    m_openBtn   = makeBtn("📁", TR("Open Folder"), TR("Open save folder"),  "secondaryBtn");
    m_deleteBtn = makeBtn("🗑", TR("Delete"),      TR("Delete this capture"),"dangerBtn");

    m_openBtn->setEnabled(true); // Always enabled

    connect(m_copyBtn,   &QPushButton::clicked, this, &HistoryPanel::onCopyClicked);
    connect(m_saveBtn,   &QPushButton::clicked, this, &HistoryPanel::onSaveAsClicked);
    connect(m_openBtn,   &QPushButton::clicked, this, &HistoryPanel::onOpenFolderClicked);
    connect(m_deleteBtn, &QPushButton::clicked, this, &HistoryPanel::onDeleteClicked);

    abLay->addWidget(m_copyBtn);
    abLay->addWidget(m_saveBtn);
    abLay->addWidget(m_openBtn);
    abLay->addStretch();
    abLay->addWidget(m_deleteBtn);
    rightLay->addWidget(actBar);

    mainLay->addWidget(leftWidget);
    mainLay->addWidget(rightPanel, 1);

    // Connect to capture manager — auto-add new captures
    if (m_manager) {
        connect(m_manager, &CaptureManager::captureComplete,
                this, [this](const QPixmap &px, const QString &path) {
            addCapture(px, path);
        });
    }

    updateButtons();
}

void HistoryPanel::addCapture(const QPixmap &px, const QString &path) {
    QListWidgetItem *item = new QListWidgetItem();
    QPixmap thumb = px.scaled(90, 68, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    item->setIcon(QIcon(thumb));

    QString filename = QFileInfo(path).fileName();
    QString size     = QString("%1 × %2 px").arg(px.width()).arg(px.height());
    QString time     = QDateTime::currentDateTime().toString("hh:mm:ss");
    item->setText(filename + "\n" + size + "\n" + time);
    item->setToolTip(path);

    // Store full pixmap and path in item data
    item->setData(Qt::UserRole,     QVariant::fromValue(px));
    item->setData(Qt::UserRole + 1, path);

    m_list->insertItem(0, item);
    m_filePaths.prepend(path);
    m_list->setCurrentRow(0);

    // Hide empty label, show list
    m_emptyLabel->setVisible(false);
    onItemSelected(item);
    updateButtons();
}

void HistoryPanel::refresh() {
    m_list->clear();
    m_filePaths.clear();
    if (!m_manager) return;

    auto history = m_manager->history();
    m_emptyLabel->setVisible(history.isEmpty());

    for (const auto &rec : history) {
        QListWidgetItem *item = new QListWidgetItem();
        QPixmap thumb = rec.pixmap.scaled(90,68,Qt::KeepAspectRatio,Qt::SmoothTransformation);
        item->setIcon(QIcon(thumb));
        QString size = QString("%1 × %2 px").arg(rec.size.width()).arg(rec.size.height());
        item->setText(QFileInfo(rec.filepath).fileName() + "\n" + size);
        item->setToolTip(rec.filepath);
        item->setData(Qt::UserRole,     QVariant::fromValue(rec.pixmap));
        item->setData(Qt::UserRole + 1, rec.filepath);
        m_list->addItem(item);
        m_filePaths.append(rec.filepath);
    }
    updateButtons();
}

void HistoryPanel::clearAll() {
    if (m_list->count() == 0) return;

    auto btn = QMessageBox::question(this,
        TR("Clear All"),
        TR("Delete all captures from history?\n(Files on disk will NOT be deleted)"),
        QMessageBox::Yes | QMessageBox::No);
    if (btn != QMessageBox::Yes) return;

    m_list->clear();
    m_filePaths.clear();
    if (m_manager) m_manager->clearHistory();

    m_preview->setPixmap(QPixmap());
    m_preview->setText(TR("Select a capture to preview"));
    m_infoLabel->setVisible(false);
    m_emptyLabel->setVisible(true);
    updateButtons();
}

void HistoryPanel::deleteSelected() {
    onDeleteClicked();
}

void HistoryPanel::onDeleteClicked() {
    int row = m_list->currentRow();
    if (row < 0 || row >= m_list->count()) return;

    QString filepath = m_filePaths.value(row);
    QString fname    = QFileInfo(filepath).fileName();

    auto btn = QMessageBox::question(this,
        TR("Delete"),
        TR("Remove") + " \"" + fname + "\" " + TR("from history?") +
        "\n" + TR("(File on disk will NOT be deleted)"),
        QMessageBox::Yes | QMessageBox::No);
    if (btn != QMessageBox::Yes) return;

    delete m_list->takeItem(row);
    if (row < m_filePaths.size()) m_filePaths.removeAt(row);
    if (m_manager) m_manager->removeFromHistory(row);

    // Clear preview if nothing left
    if (m_list->count() == 0) {
        m_preview->setPixmap(QPixmap());
        m_preview->setText(TR("Select a capture to preview"));
        m_infoLabel->setVisible(false);
        m_emptyLabel->setVisible(true);
    } else {
        // Select next item
        int newRow = qMin(row, m_list->count() - 1);
        m_list->setCurrentRow(newRow);
        onItemSelected(m_list->currentItem());
    }
    updateButtons();
}

void HistoryPanel::onItemSelected(QListWidgetItem *item) {
    if (!item) return;

    QPixmap px = item->data(Qt::UserRole).value<QPixmap>();
    if (!px.isNull()) {
        QSize sz = m_preview->size().isEmpty()
                   ? QSize(400, 300) : m_preview->size();
        m_preview->setPixmap(
            px.scaled(sz, Qt::KeepAspectRatio, Qt::SmoothTransformation));

        QString path = item->data(Qt::UserRole + 1).toString();
        QString info = QString("%1 × %2 px  |  %3")
            .arg(px.width()).arg(px.height())
            .arg(QFileInfo(path).fileName());
        m_infoLabel->setText(info);
        m_infoLabel->setVisible(true);
    }
    updateButtons();
}

void HistoryPanel::onItemDoubleClicked(QListWidgetItem *item) {
    // Double click = open in default viewer
    if (!item) return;
    QString path = item->data(Qt::UserRole + 1).toString();
    if (!path.isEmpty() && QFile::exists(path))
        QDesktopServices::openUrl(QUrl::fromLocalFile(path));
}

void HistoryPanel::onCopyClicked() {
    auto *item = m_list->currentItem();
    if (!item) return;
    QPixmap px = item->data(Qt::UserRole).value<QPixmap>();
    if (!px.isNull()) QApplication::clipboard()->setPixmap(px);
}

void HistoryPanel::onSaveAsClicked() {
    auto *item = m_list->currentItem();
    if (!item) return;
    QPixmap px = item->data(Qt::UserRole).value<QPixmap>();
    if (px.isNull()) return;

    QString defaultPath = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation)
                          + "/screenshot.png";
    QString path = QFileDialog::getSaveFileName(this,
        TR("Save As"), defaultPath,
        "PNG (*.png);;JPEG (*.jpg);;BMP (*.bmp);;All Files (*)");
    if (!path.isEmpty()) {
        QString ext = QFileInfo(path).suffix().toUpper();
        if (ext.isEmpty()) ext = "PNG";
        px.save(path, ext.toLocal8Bit());
    }
}

void HistoryPanel::onOpenFolderClicked() {
    auto *item = m_list->currentItem();
    QString folder;
    if (item) {
        QString path = item->data(Qt::UserRole + 1).toString();
        if (!path.isEmpty())
            folder = QFileInfo(path).absolutePath();
    }
    if (folder.isEmpty()) {
        QSettings s;
        folder = s.value("savePath",
            QStandardPaths::writableLocation(QStandardPaths::PicturesLocation)
            + "/ScreenMasterPro").toString();
    }
    QDir().mkpath(folder);
    QDesktopServices::openUrl(QUrl::fromLocalFile(folder));
}

void HistoryPanel::updateButtons() {
    bool hasItem = (m_list->currentRow() >= 0);
    m_copyBtn->setEnabled(hasItem);
    m_saveBtn->setEnabled(hasItem);
    m_deleteBtn->setEnabled(hasItem);
    m_openBtn->setEnabled(true);
}

void HistoryPanel::retranslate() {
    m_emptyLabel->setText(TR("No captures yet.\nTake a screenshot to start!"));
    m_preview->setText(TR("Select a capture to preview"));
    m_copyBtn->setText("📋  " + TR("Copy"));
    m_saveBtn->setText("💾  " + TR("Save As"));
    m_openBtn->setText("📁  " + TR("Open Folder"));
    m_deleteBtn->setText("🗑  " + TR("Delete"));
}
