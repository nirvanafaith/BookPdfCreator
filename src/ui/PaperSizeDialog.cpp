#include "PaperSizeDialog.h"

#include <QDoubleSpinBox>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QDialogButtonBox>

// ============================================================
// 构造函数
//
// 布局：表单（宽度/高度输入）+ 按钮行（确定/取消）
// 默认值：宽度210mm，高度297mm（A4）
// ============================================================
PaperSizeDialog::PaperSizeDialog(QWidget* parent)
    : QDialog(parent)
    , m_widthSpin(nullptr)
    , m_heightSpin(nullptr)
    , m_okBtn(nullptr)
    , m_cancelBtn(nullptr)
{
    setWindowTitle(tr("自定义纸张尺寸"));
    setMinimumWidth(280);

    // 宽度输入框
    m_widthSpin = new QDoubleSpinBox(this);
    m_widthSpin->setRange(10.0, 2000.0);    // 范围10~2000mm
    m_widthSpin->setDecimals(1);            // 精度0.1mm
    m_widthSpin->setSingleStep(1.0);        // 步长1mm
    m_widthSpin->setValue(210.0);           // 默认A4宽度
    m_widthSpin->setSuffix(tr(" mm"));

    // 高度输入框
    m_heightSpin = new QDoubleSpinBox(this);
    m_heightSpin->setRange(10.0, 2000.0);
    m_heightSpin->setDecimals(1);
    m_heightSpin->setSingleStep(1.0);
    m_heightSpin->setValue(297.0);          // 默认A4高度
    m_heightSpin->setSuffix(tr(" mm"));

    // 表单布局：标签+输入框
    QFormLayout* formLayout = new QFormLayout;
    formLayout->addRow(tr("宽度(mm):"), m_widthSpin);
    formLayout->addRow(tr("高度(mm):"), m_heightSpin);

    // 按钮行
    m_okBtn = new QPushButton(tr("确定"), this);
    m_cancelBtn = new QPushButton(tr("取消"), this);
    m_okBtn->setDefault(true);

    QHBoxLayout* btnLayout = new QHBoxLayout;
    btnLayout->addStretch();
    btnLayout->addWidget(m_okBtn);
    btnLayout->addWidget(m_cancelBtn);

    // 主布局
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->addLayout(formLayout);
    mainLayout->addLayout(btnLayout);

    // 信号连接
    connect(m_okBtn, &QPushButton::clicked, this, &QDialog::accept);
    connect(m_cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
}

// 获取宽度（毫米）
qreal PaperSizeDialog::widthMm() const
{
    return m_widthSpin->value();
}

// 获取高度（毫米）
qreal PaperSizeDialog::heightMm() const
{
    return m_heightSpin->value();
}
