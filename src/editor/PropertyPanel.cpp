#include "PropertyPanel.h"

#include <QVBoxLayout>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QSlider>
#include <QLabel>
#include <QPushButton>
#include <QFontComboBox>
#include <QButtonGroup>
#include <QColorDialog>

// ============================================================
// 构造函数
// ============================================================
PropertyPanel::PropertyPanel(QWidget* parent)
    : QWidget(parent)
    , m_transformGroup(nullptr)
    , m_xSpin(nullptr)
    , m_ySpin(nullptr)
    , m_wSpin(nullptr)
    , m_hSpin(nullptr)
    , m_rotationSpin(nullptr)
    , m_textGroup(nullptr)
    , m_fontCombo(nullptr)
    , m_fontSizeSpin(nullptr)
    , m_textColorBtn(nullptr)
    , m_alignGroup(nullptr)
    , m_alignLeftBtn(nullptr)
    , m_alignCenterBtn(nullptr)
    , m_alignRightBtn(nullptr)
    , m_alignJustifyBtn(nullptr)
    , m_imageGroup(nullptr)
    , m_opacitySlider(nullptr)
    , m_scaleSlider(nullptr)
    , m_opacityValue(nullptr)
    , m_scaleValue(nullptr)
    , m_hintLabel(nullptr)
    , m_currentTextColor(Qt::black)
    , m_updating(false)
{
    setupUi();
    setupConnections();
    clearProperties();  // 初始状态：无选中
}

// ============================================================
// setupUi - 构建界面
//
// 布局结构：
//   QVBoxLayout（主布局）
//   ├── m_hintLabel（提示文字，无选中时显示）
//   ├── m_transformGroup（通用属性组）
//   ├── m_textGroup（文本属性组）
//   └── m_imageGroup（图片属性组）
// ============================================================
void PropertyPanel::setupUi()
{
    // 主布局
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(4, 4, 4, 4);
    mainLayout->setSpacing(6);

    // ---- 提示标签 ----
    m_hintLabel = new QLabel(QStringLiteral("选择一个元素查看属性"), this);
    m_hintLabel->setAlignment(Qt::AlignCenter);
    m_hintLabel->setStyleSheet(QStringLiteral("color: #888; padding: 20px;"));
    mainLayout->addWidget(m_hintLabel);

    // ============================================================
    // 通用属性组（变换）
    // ============================================================
    m_transformGroup = new QGroupBox(QStringLiteral("变换"), this);
    QFormLayout* transformLayout = new QFormLayout(m_transformGroup);

    // X坐标：范围-1000~1000，精度0.1，单位"点"
    m_xSpin = new QDoubleSpinBox(m_transformGroup);
    m_xSpin->setRange(-1000.0, 1000.0);
    m_xSpin->setDecimals(1);
    m_xSpin->setSingleStep(0.1);
    m_xSpin->setSuffix(QStringLiteral(" 点"));
    transformLayout->addRow(QStringLiteral("X:"), m_xSpin);

    // Y坐标：范围-1000~1000，精度0.1，单位"点"
    m_ySpin = new QDoubleSpinBox(m_transformGroup);
    m_ySpin->setRange(-1000.0, 1000.0);
    m_ySpin->setDecimals(1);
    m_ySpin->setSingleStep(0.1);
    m_ySpin->setSuffix(QStringLiteral(" 点"));
    transformLayout->addRow(QStringLiteral("Y:"), m_ySpin);

    // 宽度：范围1~2000，精度0.1，单位"点"
    m_wSpin = new QDoubleSpinBox(m_transformGroup);
    m_wSpin->setRange(1.0, 2000.0);
    m_wSpin->setDecimals(1);
    m_wSpin->setSingleStep(0.1);
    m_wSpin->setSuffix(QStringLiteral(" 点"));
    transformLayout->addRow(QStringLiteral("宽:"), m_wSpin);

    // 高度：范围1~2000，精度0.1，单位"点"
    m_hSpin = new QDoubleSpinBox(m_transformGroup);
    m_hSpin->setRange(1.0, 2000.0);
    m_hSpin->setDecimals(1);
    m_hSpin->setSingleStep(0.1);
    m_hSpin->setSuffix(QStringLiteral(" 点"));
    transformLayout->addRow(QStringLiteral("高:"), m_hSpin);

    // 旋转：范围-360~360，精度1（整数），后缀"°"
    m_rotationSpin = new QDoubleSpinBox(m_transformGroup);
    m_rotationSpin->setRange(-360.0, 360.0);
    m_rotationSpin->setDecimals(0);
    m_rotationSpin->setSingleStep(1.0);
    m_rotationSpin->setSuffix(QStringLiteral("°"));  // 度符号
    transformLayout->addRow(QStringLiteral("旋转:"), m_rotationSpin);

    mainLayout->addWidget(m_transformGroup);

    // ============================================================
    // 文本属性组
    // ============================================================
    m_textGroup = new QGroupBox(QStringLiteral("文本"), this);
    QFormLayout* textLayout = new QFormLayout(m_textGroup);

    // 字体选择
    m_fontCombo = new QFontComboBox(m_textGroup);
    textLayout->addRow(QStringLiteral("字体:"), m_fontCombo);

    // 字号：范围6~72
    m_fontSizeSpin = new QSpinBox(m_textGroup);
    m_fontSizeSpin->setRange(6, 72);
    m_fontSizeSpin->setValue(12);
    m_fontSizeSpin->setSuffix(QStringLiteral(" pt"));
    textLayout->addRow(QStringLiteral("字号:"), m_fontSizeSpin);

    // 文字颜色按钮
    m_textColorBtn = new QPushButton(m_textGroup);
    m_textColorBtn->setMinimumHeight(24);
    textLayout->addRow(QStringLiteral("颜色:"), m_textColorBtn);

    // 对齐按钮组（4个checkable按钮，互斥）
    QWidget* alignWidget = new QWidget(m_textGroup);
    QHBoxLayout* alignLayout = new QHBoxLayout(alignWidget);
    alignLayout->setContentsMargins(0, 0, 0, 0);
    alignLayout->setSpacing(2);

    m_alignLeftBtn = new QPushButton(QStringLiteral("L"), alignWidget);
    m_alignLeftBtn->setToolTip(QStringLiteral("左对齐"));
    m_alignLeftBtn->setCheckable(true);
    m_alignLeftBtn->setFixedSize(32, 24);

    m_alignCenterBtn = new QPushButton(QStringLiteral("C"), alignWidget);
    m_alignCenterBtn->setToolTip(QStringLiteral("居中对齐"));
    m_alignCenterBtn->setCheckable(true);
    m_alignCenterBtn->setFixedSize(32, 24);

    m_alignRightBtn = new QPushButton(QStringLiteral("R"), alignWidget);
    m_alignRightBtn->setToolTip(QStringLiteral("右对齐"));
    m_alignRightBtn->setCheckable(true);
    m_alignRightBtn->setFixedSize(32, 24);

    m_alignJustifyBtn = new QPushButton(QStringLiteral("J"), alignWidget);
    m_alignJustifyBtn->setToolTip(QStringLiteral("两端对齐"));
    m_alignJustifyBtn->setCheckable(true);
    m_alignJustifyBtn->setFixedSize(32, 24);

    alignLayout->addWidget(m_alignLeftBtn);
    alignLayout->addWidget(m_alignCenterBtn);
    alignLayout->addWidget(m_alignRightBtn);
    alignLayout->addWidget(m_alignJustifyBtn);
    alignLayout->addStretch();

    textLayout->addRow(QStringLiteral("对齐:"), alignWidget);

    // QButtonGroup：互斥，使用Qt对齐标志作为按钮ID
    m_alignGroup = new QButtonGroup(this);
    m_alignGroup->setExclusive(true);
    m_alignGroup->addButton(m_alignLeftBtn, static_cast<int>(Qt::AlignLeft));
    m_alignGroup->addButton(m_alignCenterBtn, static_cast<int>(Qt::AlignHCenter));
    m_alignGroup->addButton(m_alignRightBtn, static_cast<int>(Qt::AlignRight));
    m_alignGroup->addButton(m_alignJustifyBtn, static_cast<int>(Qt::AlignJustify));

    mainLayout->addWidget(m_textGroup);

    // ============================================================
    // 图片属性组
    // ============================================================
    m_imageGroup = new QGroupBox(QStringLiteral("图片"), this);
    QFormLayout* imageLayout = new QFormLayout(m_imageGroup);

    // 不透明度滑块：0-100，显示百分比
    QWidget* opacityWidget = new QWidget(m_imageGroup);
    QHBoxLayout* opacityLayout = new QHBoxLayout(opacityWidget);
    opacityLayout->setContentsMargins(0, 0, 0, 0);
    opacityLayout->setSpacing(4);

    m_opacitySlider = new QSlider(Qt::Horizontal, opacityWidget);
    m_opacitySlider->setRange(0, 100);
    m_opacitySlider->setValue(100);
    m_opacityValue = new QLabel(QStringLiteral("100%"), opacityWidget);
    m_opacityValue->setMinimumWidth(40);
    m_opacityValue->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    opacityLayout->addWidget(m_opacitySlider);
    opacityLayout->addWidget(m_opacityValue);
    imageLayout->addRow(QStringLiteral("不透明度:"), opacityWidget);

    // 缩放滑块：10-500，显示百分比
    QWidget* scaleWidget = new QWidget(m_imageGroup);
    QHBoxLayout* scaleLayout = new QHBoxLayout(scaleWidget);
    scaleLayout->setContentsMargins(0, 0, 0, 0);
    scaleLayout->setSpacing(4);

    m_scaleSlider = new QSlider(Qt::Horizontal, scaleWidget);
    m_scaleSlider->setRange(10, 500);
    m_scaleSlider->setValue(100);
    m_scaleValue = new QLabel(QStringLiteral("100%"), scaleWidget);
    m_scaleValue->setMinimumWidth(40);
    m_scaleValue->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    scaleLayout->addWidget(m_scaleSlider);
    scaleLayout->addWidget(m_scaleValue);
    imageLayout->addRow(QStringLiteral("缩放:"), scaleWidget);

    mainLayout->addWidget(m_imageGroup);

    // 弹性空间填充底部
    mainLayout->addStretch();

    // 面板最小宽度
    setMinimumWidth(240);

    // 初始化颜色按钮外观
    updateColorButton();
}

// ============================================================
// setupConnections - 连接信号与槽
//
// 所有用户编辑触发的信号都检查m_updating标志，
// 仅在非更新状态下才发射属性变更信号。
// ============================================================
void PropertyPanel::setupConnections()
{
    // ---- 通用属性：位置 ----
    // X变化时，连同Y一起发射positionChanged
    connect(m_xSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this](double) {
        if (m_updating || m_currentElementId.isEmpty()) return;
        emit positionChanged(m_currentElementId, QPointF(m_xSpin->value(), m_ySpin->value()));
    });

    // Y变化时，连同X一起发射positionChanged
    connect(m_ySpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this](double) {
        if (m_updating || m_currentElementId.isEmpty()) return;
        emit positionChanged(m_currentElementId, QPointF(m_xSpin->value(), m_ySpin->value()));
    });

    // ---- 通用属性：尺寸 ----
    // 宽变化时，连同高一起发射sizeChanged
    connect(m_wSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this](double) {
        if (m_updating || m_currentElementId.isEmpty()) return;
        emit sizeChanged(m_currentElementId, QSizeF(m_wSpin->value(), m_hSpin->value()));
    });

    // 高变化时，连同宽一起发射sizeChanged
    connect(m_hSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this](double) {
        if (m_updating || m_currentElementId.isEmpty()) return;
        emit sizeChanged(m_currentElementId, QSizeF(m_wSpin->value(), m_hSpin->value()));
    });

    // ---- 通用属性：旋转 ----
    connect(m_rotationSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this](double value) {
        if (m_updating || m_currentElementId.isEmpty()) return;
        emit rotationChanged(m_currentElementId, value);
    });

    // ---- 文本属性：字体 ----
    connect(m_fontCombo, &QFontComboBox::currentFontChanged, this, [this](const QFont& font) {
        if (m_updating || m_currentElementId.isEmpty()) return;
        // 发射时保持当前字号
        QFont f = font;
        f.setPointSize(m_fontSizeSpin->value());
        emit fontChanged(m_currentElementId, f);
    });

    // ---- 文本属性：字号 ----
    connect(m_fontSizeSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int value) {
        if (m_updating || m_currentElementId.isEmpty()) return;
        emit fontSizeChanged(m_currentElementId, value);
    });

    // ---- 文本属性：颜色 ----
    connect(m_textColorBtn, &QPushButton::clicked, this, [this]() {
        if (m_currentElementId.isEmpty()) return;
        QColor newColor = QColorDialog::getColor(m_currentTextColor, this, QStringLiteral("选择文字颜色"));
        if (newColor.isValid()) {
            m_currentTextColor = newColor;
            updateColorButton();
            if (!m_updating) {
                emit textColorChanged(m_currentElementId, m_currentTextColor);
            }
        }
    });

    // ---- 文本属性：对齐 ----
    // QButtonGroup::buttonClicked(int)是重载信号，需用QOverload指定
    connect(m_alignGroup, QOverload<int>::of(&QButtonGroup::buttonClicked), this, [this](int id) {
        if (m_updating || m_currentElementId.isEmpty()) return;
        // int → Qt::AlignmentFlag（显式转换）→ Qt::Alignment（QFlags隐式构造）
        emit alignmentChanged(m_currentElementId, static_cast<Qt::AlignmentFlag>(id));
    });

    // ---- 图片属性：不透明度 ----
    connect(m_opacitySlider, &QSlider::valueChanged, this, [this](int value) {
        m_opacityValue->setText(QString::number(value) + QStringLiteral("%"));
        if (m_updating || m_currentElementId.isEmpty()) return;
        emit opacityChanged(m_currentElementId, value / 100.0);
    });

    // ---- 图片属性：缩放 ----
    connect(m_scaleSlider, &QSlider::valueChanged, this, [this](int value) {
        m_scaleValue->setText(QString::number(value) + QStringLiteral("%"));
        if (m_updating || m_currentElementId.isEmpty()) return;
        emit scaleFactorChanged(m_currentElementId, value / 100.0);
    });
}

// ============================================================
// showElementProperties - 显示指定元素的属性
//
// 根据元素类型填充对应控件值，并显示/隐藏相应属性组。
// 填充期间置m_updating=true，防止触发信号循环。
// ============================================================
void PropertyPanel::showElementProperties(const PageElementPtr& element)
{
    // 空元素 → 清空面板
    if (element.constData() == nullptr) {
        clearProperties();
        return;
    }

    m_updating = true;
    m_currentElementId = element->id();

    // ---- 填充通用属性（变换） ----
    QPointF pos = element->pos();
    QSizeF size = element->size();
    m_xSpin->setValue(pos.x());
    m_ySpin->setValue(pos.y());
    m_wSpin->setValue(size.width());
    m_hSpin->setValue(size.height());
    m_rotationSpin->setValue(element->rotation());

    // ---- 根据元素类型显示/隐藏属性组 ----
    PageElementData::ElementType type = element->elementType();
    showTextGroup(type == PageElementData::Text);
    showImageGroup(type == PageElementData::Image);

    // ---- 填充文本属性 ----
    if (type == PageElementData::Text) {
        const TextElementData* textElem =
            static_cast<const TextElementData*>(element.constData());

        // 字体
        QFont font = textElem->font();
        m_fontCombo->setCurrentFont(font);

        // 字号（pointSize可能为-1表示使用像素大小，此时用默认值）
        int fontSize = font.pointSize();
        if (fontSize <= 0) {
            fontSize = 12;
        }
        m_fontSizeSpin->setValue(fontSize);

        // 文字颜色
        m_currentTextColor = textElem->textColor();
        updateColorButton();

        // 对齐方式（提取水平对齐部分）
        Qt::Alignment align = textElem->alignment();
        int horizAlign = static_cast<int>(align & Qt::AlignHorizontal_Mask);
        if (horizAlign == 0) {
            horizAlign = static_cast<int>(Qt::AlignLeft);  // 默认左对齐
        }
        QAbstractButton* alignBtn = m_alignGroup->button(horizAlign);
        if (alignBtn) {
            alignBtn->setChecked(true);
        } else {
            m_alignLeftBtn->setChecked(true);  // 兜底
        }
    }

    // ---- 填充图片属性 ----
    if (type == PageElementData::Image) {
        const ImageElementData* imageElem =
            static_cast<const ImageElementData*>(element.constData());

        // 不透明度：0.0~1.0 → 0~100
        int opacityPercent = qRound(imageElem->opacity() * 100.0);
        opacityPercent = qBound(0, opacityPercent, 100);
        m_opacitySlider->setValue(opacityPercent);
        m_opacityValue->setText(QString::number(opacityPercent) + QStringLiteral("%"));

        // 缩放：scaleFactor → 百分比
        int scalePercent = qRound(imageElem->scaleFactor() * 100.0);
        scalePercent = qBound(10, scalePercent, 500);
        m_scaleSlider->setValue(scalePercent);
        m_scaleValue->setText(QString::number(scalePercent) + QStringLiteral("%"));
    }

    // ---- 显示属性组，隐藏提示 ----
    m_hintLabel->hide();
    m_transformGroup->show();

    m_updating = false;
}

// ============================================================
// clearProperties - 清空属性面板
//
// 隐藏所有属性组，显示提示文字。
// ============================================================
void PropertyPanel::clearProperties()
{
    m_updating = true;
    m_currentElementId.clear();
    m_transformGroup->hide();
    m_textGroup->hide();
    m_imageGroup->hide();
    m_hintLabel->show();
    m_updating = false;
}

// ============================================================
// showTextGroup - 显示/隐藏文本属性组
// ============================================================
void PropertyPanel::showTextGroup(bool show)
{
    m_textGroup->setVisible(show);
}

// ============================================================
// showImageGroup - 显示/隐藏图片属性组
// ============================================================
void PropertyPanel::showImageGroup(bool show)
{
    m_imageGroup->setVisible(show);
}

// ============================================================
// updateColorButton - 更新颜色按钮外观
//
// 将按钮背景色设为当前文字颜色，便于用户直观识别。
// ============================================================
void PropertyPanel::updateColorButton()
{
    if (m_currentTextColor.isValid()) {
        m_textColorBtn->setStyleSheet(
            QString::fromUtf8("background-color: %1; border: 1px solid #999;")
                .arg(m_currentTextColor.name()));
    } else {
        m_textColorBtn->setStyleSheet(
            QString::fromUtf8("background-color: #000000; border: 1px solid #999;"));
    }
}
