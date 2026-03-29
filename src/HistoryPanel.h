#pragma once
#include <QWidget>
#include <QListWidget>
#include <QLabel>
#include <QPushButton>
#include "Translator.h"

class CaptureManager;

class HistoryPanel : public QWidget {
    Q_OBJECT
public:
    explicit HistoryPanel(CaptureManager *mgr, QWidget *parent = nullptr);
    void refresh();
    void clearAll();
    void deleteSelected();
    void retranslate();

public slots:
    void onItemSelected(QListWidgetItem *item);
    void onItemDoubleClicked(QListWidgetItem *item);
    void onCopyClicked();
    void onSaveAsClicked();
    void onOpenFolderClicked();
    void onDeleteClicked();

private:
    void addCapture(const QPixmap &px, const QString &path);
    void updateButtons();

    CaptureManager *m_manager;
    QListWidget    *m_list;
    QLabel         *m_preview;
    QLabel         *m_infoLabel;
    QLabel         *m_emptyLabel;
    QPushButton    *m_copyBtn;
    QPushButton    *m_saveBtn;
    QPushButton    *m_openBtn;
    QPushButton    *m_deleteBtn;
    QStringList     m_filePaths;
};
