#pragma once
#include <QWidget>
#include <QTimer>
#include <QString>

class CountdownOverlay : public QWidget {
    Q_OBJECT
public:
    explicit CountdownOverlay(int seconds, QWidget *parent = nullptr,
                              const QString &hintText = "Capturing in... Press ESC to cancel");
    void setAutoClose(bool v) { m_autoClose = v; }

signals:
    void finished();

protected:
    void paintEvent(QPaintEvent *) override;
    void keyPressEvent(QKeyEvent *) override;

private slots:
    void onTick();

private:
    QTimer  *m_timer;
    int      m_remaining;
    QString  m_hintText;
    bool     m_autoClose;
};
