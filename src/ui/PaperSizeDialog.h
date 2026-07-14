#ifndef PAPERSIZEDIALOG_H
#define PAPERSIZEDIALOG_H

#include <QDialog>

class QDoubleSpinBox;
class QPushButton;
class QLabel;

// ============================================================
// PaperSizeDialog - 自定义纸张尺寸对话框
//
// 提供宽度/高度（毫米）输入，用于自定义纸张尺寸。
// 默认值为A4尺寸（210x297mm），范围10~2000mm，精度0.1mm。
// 点击"确定"返回 Accepted，"取消"返回 Rejected。
// ============================================================
class PaperSizeDialog : public QDialog
{
    Q_OBJECT
public:
    explicit PaperSizeDialog(QWidget* parent = nullptr);

    // 获取输入的宽度和高度（毫米）
    qreal widthMm() const;
    qreal heightMm() const;

private:
    QDoubleSpinBox* m_widthSpin;    // 宽度输入框
    QDoubleSpinBox* m_heightSpin;   // 高度输入框
    QPushButton* m_okBtn;           // 确定按钮
    QPushButton* m_cancelBtn;       // 取消按钮
};

#endif // PAPERSIZEDIALOG_H
