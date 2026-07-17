#include "PropertyPanel.h"

#include <QVBoxLayout>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QAbstractSpinBox>
#include <QSlider>
#include <QLabel>
#include <QPushButton>
#include <QFontComboBox>
#include <QButtonGroup>
#include <QColorDialog>
#include <QCheckBox>
#include <QTextEdit>
#include <QEvent>
#include <QFocusEvent>

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
    , m_lineHeightSpin(nullptr)
    , m_letterSpacingSpin(nullptr)
    , m_contentEdit(nullptr)
    , m_imageGroup(nullptr)
    , m_opacitySlider(nullptr)
    , m_scaleSlider(nullptr)
    , m_opacityValue(nullptr)
    , m_scaleValue(nullptr)
    , m_shapeGroup(nullptr)
    , m_fillCheck(nullptr)
    , m_fillColorBtn(nullptr)
    , m_borderColorBtn(nullptr)
    , m_borderWidthSpin(nullptr)
    , m_fillWidget(nullptr)
    , m_currentFillColor(Qt::black)
    , m_currentBorderColor(Qt::black)
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

    // 内容编辑框（多行文本，固定高度，垂直滚动条）
    m_contentEdit = new QTextEdit(m_textGroup);
    m_contentEdit->setFixedHeight(80);
    m_contentEdit->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_contentEdit->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_contentEdit->setAcceptRichText(false);
    m_contentEdit->setPlaceholderText(QStringLiteral("输入文本内容..."));
    textLayout->addRow(QStringLiteral("内容:"), m_contentEdit);

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

    m_alignLeftBtn = new QPushButton(QStringLiteral("居左"), alignWidget);
    m_alignLeftBtn->setToolTip(QStringLiteral("左对齐"));
    m_alignLeftBtn->setCheckable(true);
    m_alignLeftBtn->setFixedSize(44, 24);

    m_alignCenterBtn = new QPushButton(QStringLiteral("居中"), alignWidget);
    m_alignCenterBtn->setToolTip(QStringLiteral("居中对齐"));
    m_alignCenterBtn->setCheckable(true);
    m_alignCenterBtn->setFixedSize(44, 24);

    m_alignRightBtn = new QPushButton(QStringLiteral("居右"), alignWidget);
    m_alignRightBtn->setToolTip(QStringLiteral("右对齐"));
    m_alignRightBtn->setCheckable(true);
    m_alignRightBtn->setFixedSize(44, 24);

    m_alignJustifyBtn = new QPushButton(QStringLiteral("两端"), alignWidget);
    m_alignJustifyBtn->setToolTip(QStringLiteral("两端对齐"));
    m_alignJustifyBtn->setCheckable(true);
    m_alignJustifyBtn->setFixedSize(44, 24);

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

    // 行间距：0=自动，0.1-100像素
    m_lineHeightSpin = new QDoubleSpinBox(m_textGroup);
    m_lineHeightSpin->setRange(0.0, 100.0);
    m_lineHeightSpin->setDecimals(1);
    m_lineHeightSpin->setSingleStep(0.5);
    m_lineHeightSpin->setValue(0.0);
    m_lineHeightSpin->setSuffix(QStringLiteral(" px"));
    m_lineHeightSpin->setSpecialValueText(QStringLiteral("自动"));  // 0显示为"自动"
    textLayout->addRow(QStringLiteral("行间距:"), m_lineHeightSpin);

    // 字间距：0=默认，-50~200百分比偏移
    m_letterSpacingSpin = new QDoubleSpinBox(m_textGroup);
    m_letterSpacingSpin->setRange(-50.0, 200.0);
    m_letterSpacingSpin->setDecimals(1);
    m_letterSpacingSpin->setSingleStep(1.0);
    m_letterSpacingSpin->setValue(0.0);
    m_letterSpacingSpin->setSuffix(QStringLiteral("%"));
    m_letterSpacingSpin->setSpecialValueText(QStringLiteral("默认"));  // 0显示为"默认"
    textLayout->addRow(QStringLiteral("字间距:"), m_letterSpacingSpin);

    mainLayout->addWidget(m_textGroup);

    // ============================================================
    // 图片属性组
    // ============================================================
    m_imageGroup = new QGroupBox(QStringLiteral("图片"), this);
    QFormLayout* imageLayout = new QFormLayout(m_imageGroup);

    // 不透明度滑块：0-100，百分比
    QWidget* opacityWidget = new QWidget(m_imageGroup);
    QHBoxLayout* opacityLayout = new QHBoxLayout(opacityWidget);
    opacityLayout->setContentsMargins(0, 0, 0, 0);
    opacityLayout->setSpacing(4);

    m_opacitySlider = new QSlider(Qt::Horizontal, opacityWidget);
    m_opacitySlider->setRange(0, 100);
    m_opacitySlider->setValue(100);
    m_opacityValue = new QSpinBox(opacityWidget);
    m_opacityValue->setRange(0, 100);
    m_opacityValue->setValue(100);
    m_opacityValue->setSuffix(QStringLiteral("%"));
    m_opacityValue->setFixedWidth(55);
    m_opacityValue->setButtonSymbols(QAbstractSpinBox::NoButtons);
    m_opacityValue->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    opacityLayout->addWidget(m_opacitySlider);
    opacityLayout->addWidget(m_opacityValue);
    imageLayout->addRow(QStringLiteral("不透明度:"), opacityWidget);

    // 缩放滑块：10-500，百分比
    QWidget* scaleWidget = new QWidget(m_imageGroup);
    QHBoxLayout* scaleLayout = new QHBoxLayout(scaleWidget);
    scaleLayout->setContentsMargins(0, 0, 0, 0);
    scaleLayout->setSpacing(4);

    m_scaleSlider = new QSlider(Qt::Horizontal, scaleWidget);
    m_scaleSlider->setRange(10, 500);
    m_scaleSlider->setValue(100);
    m_scaleValue = new QSpinBox(scaleWidget);
    m_scaleValue->setRange(10, 500);
    m_scaleValue->setValue(100);
    m_scaleValue->setSuffix(QStringLiteral("%"));
    m_scaleValue->setFixedWidth(55);
    m_scaleValue->setButtonSymbols(QAbstractSpinBox::NoButtons);
    m_scaleValue->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    scaleLayout->addWidget(m_scaleSlider);
    scaleLayout->addWidget(m_scaleValue);
    imageLayout->addRow(QStringLiteral("缩放:"), scaleWidget);

    mainLayout->addWidget(m_imageGroup);

    // ============================================================
    // 形状属性组
    // ============================================================
    m_shapeGroup = new QGroupBox(QStringLiteral("形状"), this);
    QFormLayout* shapeLayout = new QFormLayout(m_shapeGroup);

    // 填充勾选（独占一行）
    m_fillCheck = new QCheckBox(QStringLiteral("填充颜色"), m_shapeGroup);
    shapeLayout->addRow(m_fillCheck);

    // 填充色 - 用容器widget方便整体隐藏（label+button）
    m_fillWidget = new QWidget(m_shapeGroup);
    QHBoxLayout* fillLayout = new QHBoxLayout(m_fillWidget);
    fillLayout->setContentsMargins(0, 0, 0, 0);
    fillLayout->setSpacing(4);
    QLabel* fillLabel = new QLabel(QStringLiteral("填充色:"), m_fillWidget);
    m_fillColorBtn = new QPushButton(m_fillWidget);
    m_fillColorBtn->setMinimumHeight(24);
    fillLayout->addWidget(fillLabel);
    fillLayout->addWidget(m_fillColorBtn);
    shapeLayout->addRow(m_fillWidget);

    // 边框色 - 用容器widget保持布局一致
    QWidget* borderWidget = new QWidget(m_shapeGroup);
    QHBoxLayout* borderLayout = new QHBoxLayout(borderWidget);
    borderLayout->setContentsMargins(0, 0, 0, 0);
    borderLayout->setSpacing(4);
    QLabel* borderLabel = new QLabel(QStringLiteral("边框色:"), borderWidget);
    m_borderColorBtn = new QPushButton(borderWidget);
    m_borderColorBtn->setMinimumHeight(24);
    borderLayout->addWidget(borderLabel);
    borderLayout->addWidget(m_borderColorBtn);
    shapeLayout->addRow(borderWidget);

    // 边框磅数：范围0.5~20.0，精度0.5，单位"磅"
    m_borderWidthSpin = new QDoubleSpinBox(m_shapeGroup);
    m_borderWidthSpin->setRange(0.5, 20.0);
    m_borderWidthSpin->setDecimals(1);
    m_borderWidthSpin->setSingleStep(0.5);
    m_borderWidthSpin->setSuffix(QStringLiteral(" 磅"));
    shapeLayout->addRow(QStringLiteral("边框粗细:"), m_borderWidthSpin);

    mainLayout->addWidget(m_shapeGroup);

    // 弹性空间填充底部
    mainLayout->addStretch();

    // 面板最小宽度
    setMinimumWidth(240);

    // 初始化颜色按钮外观
    updateColorButton();
    updateShapeColorButtons();

    // 设置内容编辑框的事件过滤器
    setupContentEditConnections();
}

// ============================================================
// setupContentEditConnections - 设置内容编辑框的连接
//
// 安装事件过滤器以检测QTextEdit失去焦点事件，
// 在失去焦点时发射textContentChanged信号。
// ============================================================
void PropertyPanel::setupContentEditConnections()
{
    // 安装事件过滤器以检测QTextEdit失去焦点
    m_contentEdit->installEventFilter(this);
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

    // ---- 文本属性：行间距 ----
    connect(m_lineHeightSpin, &QDoubleSpinBox::editingFinished, this, [this]() {
        if (m_updating || m_currentElementId.isEmpty()) return;
        emit lineHeightChanged(m_currentElementId, m_lineHeightSpin->value());
    });

    // ---- 文本属性：字间距 ----
    connect(m_letterSpacingSpin, &QDoubleSpinBox::editingFinished, this, [this]() {
        if (m_updating || m_currentElementId.isEmpty()) return;
        emit letterSpacingChanged(m_currentElementId, m_letterSpacingSpin->value());
    });

    // ---- 图片属性：不透明度 ----
    connect(m_opacitySlider, &QSlider::valueChanged, this, [this](int value) {
        m_opacityValue->setValue(value);
        if (m_updating || m_currentElementId.isEmpty()) return;
        emit opacityChanged(m_currentElementId, value / 100.0);
    });

    connect(m_opacityValue, &QSpinBox::editingFinished, this, [this]() {
        if (m_updating || m_currentElementId.isEmpty()) return;
        int value = m_opacityValue->value();
        m_opacitySlider->blockSignals(true);
        m_opacitySlider->setValue(value);
        m_opacitySlider->blockSignals(false);
        emit opacityChanged(m_currentElementId, value / 100.0);
    });

    // ---- 图片属性：缩放 ----
    connect(m_scaleSlider, &QSlider::valueChanged, this, [this](int value) {
        m_scaleValue->setValue(value);
        if (m_updating || m_currentElementId.isEmpty()) return;
        emit scaleFactorChanged(m_currentElementId, value / 100.0);
    });

    connect(m_scaleValue, &QSpinBox::editingFinished, this, [this]() {
        if (m_updating || m_currentElementId.isEmpty()) return;
        int value = m_scaleValue->value();
        m_scaleSlider->blockSignals(true);
        m_scaleSlider->setValue(value);
        m_scaleSlider->blockSignals(false);
        emit scaleFactorChanged(m_currentElementId, value / 100.0);
    });

    // ---- 形状属性：填充勾选 ----
    connect(m_fillCheck, &QCheckBox::toggled, this, [this](bool checked) {
        m_fillColorBtn->setEnabled(checked);
        if (m_updating || m_currentElementId.isEmpty()) return;
        emit fillEnabledChanged(m_currentElementId, checked);
    });

    // ---- 形状属性：填充色 ----
    connect(m_fillColorBtn, &QPushButton::clicked, this, [this]() {
        if (m_currentElementId.isEmpty()) return;
        QColor newColor = QColorDialog::getColor(m_currentFillColor, this, QStringLiteral("选择填充颜色"));
        if (newColor.isValid()) {
            m_currentFillColor = newColor;
            updateShapeColorButtons();
            if (!m_updating) {
                emit fillColorChanged(m_currentElementId, m_currentFillColor);
            }
        }
    });

    // ---- 形状属性：边框色 ----
    connect(m_borderColorBtn, &QPushButton::clicked, this, [this]() {
        if (m_currentElementId.isEmpty()) return;
        QColor newColor = QColorDialog::getColor(m_currentBorderColor, this, QStringLiteral("选择边框颜色"));
        if (newColor.isValid()) {
            m_currentBorderColor = newColor;
            updateShapeColorButtons();
            if (!m_updating) {
                emit borderColorChanged(m_currentElementId, m_currentBorderColor);
            }
        }
    });

    // ---- 形状属性：边框磅数 ----
    connect(m_borderWidthSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this](double value) {
        if (m_updating || m_currentElementId.isEmpty()) return;
        emit borderWidthChanged(m_currentElementId, value);
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
    showShapeGroup(type == PageElementData::Shape);

    // ---- 填充文本属性 ----
    if (type == PageElementData::Text) {
        const TextElementData* textElem =
            static_cast<const TextElementData*>(element.constData());

        // 文本内容
        m_contentEdit->setPlainText(textElem->text());

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

        // 行间距
        m_lineHeightSpin->setValue(textElem->lineHeight());

        // 字间距
        m_letterSpacingSpin->setValue(textElem->letterSpacing());
    }

    // ---- 填充图片属性 ----
    if (type == PageElementData::Image) {
        const ImageElementData* imageElem =
            static_cast<const ImageElementData*>(element.constData());

        // 不透明度：0.0~1.0 → 0~100
        int opacityPercent = qRound(imageElem->opacity() * 100.0);
        opacityPercent = qBound(0, opacityPercent, 100);
        m_opacitySlider->setValue(opacityPercent);
        m_opacityValue->setValue(opacityPercent);

        // 缩放：scaleFactor → 百分比
        int scalePercent = qRound(imageElem->scaleFactor() * 100.0);
        scalePercent = qBound(10, scalePercent, 500);
        m_scaleSlider->setValue(scalePercent);
        m_scaleValue->setValue(scalePercent);
    }

    // ---- 填充形状属性 ----
    if (type == PageElementData::Shape) {
        const ShapeElementData* shapeElem =
            static_cast<const ShapeElementData*>(element.constData());

        // 根据形状类型控制填充控件的显示：
        // Line/Path 始终无填充，隐藏填充勾选和填充色按钮
        ShapeElementData::ShapeType shapeType = shapeElem->shapeType();
        bool canFill = (shapeType != ShapeElementData::Line &&
                        shapeType != ShapeElementData::Path);
        m_fillCheck->setVisible(canFill);
        m_fillWidget->setVisible(canFill);

        // 填充勾选
        m_fillCheck->setChecked(shapeElem->hasFill());

        // 填充色
        m_currentFillColor = shapeElem->fillColor();
        m_fillColorBtn->setEnabled(shapeElem->hasFill());

        // 边框色
        m_currentBorderColor = shapeElem->borderColor();

        // 边框磅数
        m_borderWidthSpin->setValue(shapeElem->borderWidth());

        updateShapeColorButtons();
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
    m_shapeGroup->hide();
    m_hintLabel->show();
    if (m_contentEdit) {
        m_contentEdit->clear();
    }
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
// showShapeGroup - 显示/隐藏形状属性组
// ============================================================
void PropertyPanel::showShapeGroup(bool show)
{
    m_shapeGroup->setVisible(show);
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

// ============================================================
// updateShapeColorButtons - 更新形状颜色按钮外观
//
// 将填充色和边框色按钮的背景设为对应颜色，便于直观识别。
// ============================================================
void PropertyPanel::updateShapeColorButtons()
{
    if (m_currentFillColor.isValid()) {
        m_fillColorBtn->setStyleSheet(
            QString::fromUtf8("background-color: %1; border: 2px solid #888; border-radius: 3px;")
                .arg(m_currentFillColor.name()));
    }
    if (m_currentBorderColor.isValid()) {
        m_borderColorBtn->setStyleSheet(
            QString::fromUtf8("background-color: %1; border: 2px solid #888; border-radius: 3px;")
                .arg(m_currentBorderColor.name()));
    }
}

// ============================================================
// eventFilter - 事件过滤器
//
// 监听m_contentEdit的焦点丢失事件，在失去焦点时发射
// textContentChanged信号，通知MainWindow更新文本元素内容。
// 使用m_updating标志避免在程序化填充时触发信号。
// ============================================================
bool PropertyPanel::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == m_contentEdit && event->type() == QEvent::FocusOut) {
        if (!m_updating && !m_currentElementId.isEmpty()) {
            QString newText = m_contentEdit->toPlainText();
            emit textContentChanged(m_currentElementId, newText);
        }
    }
    return QWidget::eventFilter(watched, event);
}
