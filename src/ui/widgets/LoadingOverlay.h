#pragma once

#include <QWidget>

class QLabel;
class QProgressBar;

// 半透明加载遮罩：居中显示进度条与提示文字，覆盖在父控件之上。
class LoadingOverlay : public QWidget {
    Q_OBJECT

public:
    explicit LoadingOverlay(QWidget *parent = nullptr);
    void setText(const QString &text);

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void showEvent(QShowEvent *event) override;

private:
    void center();

    QLabel *m_label = nullptr;
    QProgressBar *m_bar = nullptr;
    QWidget *m_container = nullptr;
};
