#pragma once
#include <QDialog>
#include <QPixmap>
#include <QTextEdit>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QApplication>
#include <QClipboard>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>

class OcrDialog : public QDialog {
    Q_OBJECT
public:
    explicit OcrDialog(const QPixmap &pixmap, const QString &text, QWidget *parent = nullptr)
        : QDialog(parent)
    {
        setWindowTitle("OCR — Extracted Text");
        setMinimumSize(700, 500);
        resize(800, 600);
        setStyleSheet(R"(
            QDialog { background:#1a1a2e; color:#e0e0f0; }
            QTextEdit { background:#12121f; color:#e8e8ff; border:1px solid #303055;
                        border-radius:8px; padding:10px; font-family:"Consolas",monospace;
                        font-size:13px; line-height:1.5; }
            QPushButton { background:#1e1e3a; border:1px solid #303055; border-radius:6px;
                          padding:8px 16px; color:#ccccee; }
            QPushButton:hover { background:#2a2a50; border-color:#00aeff; color:#fff; }
            QPushButton#primary { background:#0099dd; color:white; font-weight:bold; border:none; }
            QPushButton#primary:hover { background:#00aaee; }
            QLabel { color:#aaa; }
        )");

        QVBoxLayout *lay = new QVBoxLayout(this);
        lay->setSpacing(12);
        lay->setContentsMargins(16,16,16,16);

        // Header
        QLabel *title = new QLabel("🔍  OCR — Recognized Text");
        title->setStyleSheet("font-size:15px; font-weight:bold; color:#00aeff;");
        lay->addWidget(title);

        // Image preview (small)
        if (!pixmap.isNull()) {
            QLabel *imgLbl = new QLabel();
            imgLbl->setPixmap(pixmap.scaledToWidth(400, Qt::SmoothTransformation));
            imgLbl->setAlignment(Qt::AlignCenter);
            imgLbl->setStyleSheet("background:#0d0d1e; border:1px solid #252545; border-radius:6px; padding:4px;");
            lay->addWidget(imgLbl);
        }

        // Text area
        QLabel *textLbl = new QLabel("Extracted text:");
        textLbl->setStyleSheet("color:#888; font-size:11px;");
        lay->addWidget(textLbl);

        m_textEdit = new QTextEdit();
        m_textEdit->setPlainText(text.isEmpty() ? "(No text detected)" : text);
        m_textEdit->setReadOnly(false); // allow editing
        lay->addWidget(m_textEdit, 1);

        // Buttons
        QHBoxLayout *btnRow = new QHBoxLayout();
        QPushButton *copyBtn = new QPushButton("📋  Copy All");
        copyBtn->setObjectName("primary");
        copyBtn->setFixedHeight(40);
        QPushButton *saveBtn = new QPushButton("💾  Save as .txt");
        saveBtn->setFixedHeight(40);
        QPushButton *closeBtn = new QPushButton("✖  Close");
        closeBtn->setFixedHeight(40);

        connect(copyBtn, &QPushButton::clicked, this, [this](){
            QApplication::clipboard()->setText(m_textEdit->toPlainText());
        });
        connect(saveBtn, &QPushButton::clicked, this, [this](){
            QString path = QFileDialog::getSaveFileName(this,"Save Text","ocr_result.txt","Text (*.txt)");
            if (!path.isEmpty()) {
                QFile f(path); f.open(QFile::WriteOnly|QFile::Text);
                QTextStream ts(&f); ts << m_textEdit->toPlainText();
            }
        });
        connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);

        btnRow->addWidget(copyBtn);
        btnRow->addWidget(saveBtn);
        btnRow->addStretch();
        btnRow->addWidget(closeBtn);
        lay->addLayout(btnRow);
    }

private:
    QTextEdit *m_textEdit;
};
