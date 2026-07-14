#include "TextFormatToolbar.h"

#include <QHBoxLayout>
#include <QComboBox>
#include <QFontComboBox>
#include <QSpinBox>
#include <QPushButton>
#include <QButtonGroup>
#include <QColorDialog>
#include <QApplication>
#include <QGuiApplication>
#include <QScreen>
#include <QEvent>
#include <QMouseEvent>
#include <QShowEvent>
#include <QHideEvent>

// ============================================================
// 构造函数
//
// 设置工具窗口标志与属性，构建 UI 并连接信号槽。
// ============================================================
TextFormatToolbar::TextFormatToolbar(QWidget* parent)
    : QWidget(parent)
    , m_fontCombo(nullptr)
    , m_sizeSpin(nullptr)
    , m_boldBtn(nullptr)
    , m_italicBtn(nullptr)
    , m_colorBtn(nullptr)
    , m_alignLeftBtn(nullptr)
    , m_alignCenterBtn(nullptr)
    , m_alignRightBtn(nullptr)
    , m_alignJustifyBtn(nullptr)
    , m_alignGroup(nullptr)
    , m_currentColor(Qt::black)
{
    // 工具窗口标志：无边框、置顶、工具类型
    setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    // 显示时不抢占主窗口激活（避免文本编辑器因焦点丢失退出编辑模式）
    setAttribute(Qt::WA_ShowWithoutActivating);

    setupUi();
    setupConnections();
}

// ============================================================
// showAt - 在指定全局坐标显示工具栏
//
// 自动调整位置以避免超出当前屏幕可用区域。
// 使用 QGuiApplication::screenAt 获取目标屏幕（Qt 5.10+），
// 兜底回退到主屏幕。
// ============================================================
void TextFormatToolbar::showAt(const QPoint& globalPos)
{
    adjustSize();
    const QSize sz = size();
    QPoint pos = globalPos;

    QScreen* screen = QGuiApplication::screenAt(globalPos);
    if (!screen) {
        screen = QGuiApplication::primaryScreen();
    }
    if (screen) {
        const QRect avail = screen->availableGeometry();
        // 右边界超出：左移
        if (pos.x() + sz.width() > avail.right() + 1) {
            pos.setX(avail.right() + 1 - sz.width());
        }
        // 下边界超出：上移
        if (pos.y() + sz.height() > avail.bottom() + 1) {
            pos.setY(avail.bottom() + 1 - sz.height());
        }
        // 左/上边界限制
        if (pos.x() < avail.left()) pos.setX(avail.left());
        if (pos.y() < avail.top())  pos.setY(avail.top());
    }

    move(pos);
    show();
    raise();
}

// ============================================================
// updateFormatState - 同步按钮状态到当前格式
//
// 阻塞所有控件信号，避免更新过程中触发格式化信号形成反馈循环。
// 对齐方式通过 AlignHorizontal_Mask 过滤，兼容携带垂直对齐标志的
// 复合对齐（如 AlignCenter = AlignHCenter | AlignVCenter）。
// ============================================================
void TextFormatToolbar::updateFormatState(const QFont& currentFont, const QColor& currentColor,
                                          Qt::Alignment currentAlignment)
{
    // 阻塞信号
    m_fontCombo->blockSignals(true);
    m_sizeSpin->blockSignals(true);
    m_boldBtn->blockSignals(true);
    m_italicBtn->blockSignals(true);
    const auto alignButtons = m_alignGroup->buttons();
    for (QAbstractButton* btn : alignButtons) {
        btn->blockSignals(true);
    }

    // 字体与字号
    m_fontCombo->setCurrentFont(currentFont);
    const int ps = currentFont.pointSize();
    m_sizeSpin->setValue(ps > 0 ? ps : 12);

    // 粗体 / 斜体
    m_boldBtn->setChecked(currentFont.bold());
    m_italicBtn->setChecked(currentFont.italic());

    // 颜色
    m_currentColor = currentColor;
    updateColorButton();

    // 对齐方式（仅取水平部分）
    const int horiz = currentAlignment & Qt::AlignHorizontal_Mask;
    if (horiz == static_cast<int>(Qt::AlignHCenter)) {
        m_alignCenterBtn->setChecked(true);
    } else if (horiz == static_cast<int>(Qt::AlignRight)) {
        m_alignRightBtn->setChecked(true);
    } else if (horiz == static_cast<int>(Qt::AlignJustify)) {
        m_alignJustifyBtn->setChecked(true);
    } else {
        // AlignLeft 或默认(0) 均归为左对齐
        m_alignLeftBtn->setChecked(true);
    }

    // 恢复信号
    m_fontCombo->blockSignals(false);
    m_sizeSpin->blockSignals(false);
    m_boldBtn->blockSignals(false);
    m_italicBtn->blockSignals(false);
    for (QAbstractButton* btn : alignButtons) {
        btn->blockSignals(false);
    }
}

// ============================================================
// eventFilter - 全局事件过滤器
//
// 监听全局鼠标按下事件：
//   - 若存在模态对话框（如 QColorDialog），不隐藏，避免选择颜色时
//     误触发隐藏。
//   - 否则，按下位置不在工具栏几何范围内时隐藏工具栏。
// 返回 false 不拦截事件，保证正常派发。
// ============================================================
bool TextFormatToolbar::eventFilter(QObject* watched, QEvent* event)
{
    if (event->type() == QEvent::MouseButtonPress) {
        // 模态对话框打开时不隐藏（如颜色选择对话框）
        if (!QApplication::activeModalWidget()) {
            QMouseEvent* me = static_cast<QMouseEvent*>(event);
            if (!geometry().contains(me->globalPos())) {
                hide();
            }
        }
    }
    return QWidget::eventFilter(watched, event);
}

// ============================================================
// showEvent - 显示时安装全局事件过滤器
// ============================================================
void TextFormatToolbar::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);
    qApp->installEventFilter(this);
}

// ============================================================
// hideEvent - 隐藏时移除全局事件过滤器
// ============================================================
void TextFormatToolbar::hideEvent(QHideEvent* event)
{
    QWidget::hideEvent(event);
    qApp->removeEventFilter(this);
}

// ============================================================
// setupUi - 构建 UI
//
// 水平紧凑布局：字体下拉框 | 字号 | B | I | 颜色 | L | C | R | J
// 按钮使用文字标签（B/I/L/C/R/J）配合工具提示，无需图片资源。
// ============================================================
void TextFormatToolbar::setupUi()
{
    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins(4, 2, 4, 2);
    layout->setSpacing(3);

    // ---- 字体选择下拉框 ----
    m_fontCombo = new QFontComboBox(this);
    m_fontCombo->setFixedWidth(150);
    m_fontCombo->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
    m_fontCombo->setMinimumContentsLength(12);
    layout->addWidget(m_fontCombo);

    // ---- 字号 ----
    m_sizeSpin = new QSpinBox(this);
    m_sizeSpin->setRange(6, 72);
    m_sizeSpin->setValue(12);
    m_sizeSpin->setFixedWidth(52);
    m_sizeSpin->setAlignment(Qt::AlignCenter);
    layout->addWidget(m_sizeSpin);

    // ---- 粗体按钮 ----
    m_boldBtn = new QPushButton(QStringLiteral("B"), this);
    m_boldBtn->setCheckable(true);
    m_boldBtn->setFixedSize(26, 26);
    m_boldBtn->setToolTip(QStringLiteral("粗体"));
    {
        QFont f = m_boldBtn->font();
        f.setBold(true);
        m_boldBtn->setFont(f);
    }
    layout->addWidget(m_boldBtn);

    // ---- 斜体按钮 ----
    m_italicBtn = new QPushButton(QStringLiteral("I"), this);
    m_italicBtn->setCheckable(true);
    m_italicBtn->setFixedSize(26, 26);
    m_italicBtn->setToolTip(QStringLiteral("斜体"));
    {
        QFont f = m_italicBtn->font();
        f.setBold(true);
        f.setItalic(true);
        m_italicBtn->setFont(f);
    }
    layout->addWidget(m_italicBtn);

    // ---- 颜色按钮 ----
    m_colorBtn = new QPushButton(this);
    m_colorBtn->setFixedSize(26, 26);
    m_colorBtn->setToolTip(QStringLiteral("文字颜色"));
    updateColorButton();
    layout->addWidget(m_colorBtn);

    // ---- 对齐按钮组（互斥） ----
    m_alignGroup = new QButtonGroup(this);
    m_alignGroup->setExclusive(true);

    m_alignLeftBtn = new QPushButton(QStringLiteral("L"), this);
    m_alignLeftBtn->setCheckable(true);
    m_alignLeftBtn->setFixedSize(26, 26);
    m_alignLeftBtn->setToolTip(QStringLiteral("左对齐"));
    m_alignGroup->addButton(m_alignLeftBtn, 0);
    layout->addWidget(m_alignLeftBtn);

    m_alignCenterBtn = new QPushButton(QStringLiteral("C"), this);
    m_alignCenterBtn->setCheckable(true);
    m_alignCenterBtn->setFixedSize(26, 26);
    m_alignCenterBtn->setToolTip(QStringLiteral("居中对齐"));
    m_alignGroup->addButton(m_alignCenterBtn, 1);
    layout->addWidget(m_alignCenterBtn);

    m_alignRightBtn = new QPushButton(QStringLiteral("R"), this);
    m_alignRightBtn->setCheckable(true);
    m_alignRightBtn->setFixedSize(26, 26);
    m_alignRightBtn->setToolTip(QStringLiteral("右对齐"));
    m_alignGroup->addButton(m_alignRightBtn, 2);
    layout->addWidget(m_alignRightBtn);

    m_alignJustifyBtn = new QPushButton(QStringLiteral("J"), this);
    m_alignJustifyBtn->setCheckable(true);
    m_alignJustifyBtn->setFixedSize(26, 26);
    m_alignJustifyBtn->setToolTip(QStringLiteral("两端对齐"));
    m_alignGroup->addButton(m_alignJustifyBtn, 3);
    layout->addWidget(m_alignJustifyBtn);

    // 默认选中左对齐
    m_alignLeftBtn->setChecked(true);

    // 工具栏整体样式：浅灰底、圆角边框
    setStyleSheet(QStringLiteral(
        "TextFormatToolbar {"
        "  background-color: #f5f5f5;"
        "  border: 1px solid #aaaaaa;"
        "  border-radius: 6px;"
        "}"
    ));
    // 紧凑高度（按钮 26px + 上下边距约 30px）
    setFixedHeight(34);
}

// ============================================================
// setupConnections - 建立信号槽连接
//
// 每个控件变化均发射对应格式化信号，由调用方连接到实际处理逻辑。
// ============================================================
void TextFormatToolbar::setupConnections()
{
    // 字体改变
    connect(m_fontCombo, &QFontComboBox::currentFontChanged,
            this, [this](const QFont& font) { emit fontChanged(font); });

    // 字号改变
    connect(m_sizeSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, [this](int size) { emit fontSizeChanged(size); });

    // 粗体
    connect(m_boldBtn, &QPushButton::toggled,
            this, [this](bool bold) { emit boldToggled(bold); });

    // 斜体
    connect(m_italicBtn, &QPushButton::toggled,
            this, [this](bool italic) { emit italicToggled(italic); });

    // 颜色按钮：弹出 QColorDialog
    connect(m_colorBtn, &QPushButton::clicked, this, [this]() {
        const QColor c = QColorDialog::getColor(
            m_currentColor, this, QStringLiteral("选择文字颜色"));
        if (c.isValid()) {
            m_currentColor = c;
            updateColorButton();
            emit colorChanged(c);
        }
    });

    // 对齐按钮组：按钮 ID 映射到 Qt::Alignment
    connect(m_alignGroup, QOverload<int>::of(&QButtonGroup::buttonClicked),
            this, [this](int id) {
        Qt::Alignment align = Qt::AlignLeft;
        switch (id) {
            case 0: align = Qt::AlignLeft;     break;
            case 1: align = Qt::AlignHCenter;  break;
            case 2: align = Qt::AlignRight;    break;
            case 3: align = Qt::AlignJustify;  break;
            default: break;
        }
        emit alignmentChanged(align);
    });
}

// ============================================================
// updateColorButton - 更新颜色按钮显示当前颜色
// ============================================================
void TextFormatToolbar::updateColorButton()
{
    m_colorBtn->setStyleSheet(QStringLiteral(
        "QPushButton {"
        "  background-color: %1;"
        "  border: 1px solid #888888;"
        "  border-radius: 3px;"
        "}"
    ).arg(m_currentColor.name()));
}
