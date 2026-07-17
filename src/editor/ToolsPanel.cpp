#include "ToolsPanel.h"

#include <QVBoxLayout>
#include <QToolButton>
#include <QButtonGroup>
#include <QPushButton>
#include <QColorDialog>
#include <QFrame>
#include <QIcon>
#include <QPixmap>
#include <QPainter>
#include <QPainterPath>

enum ToolIconType {
    IconSelect,
    IconRect,
    IconEllipse,
    IconLine,
    IconRoundedRect,
    IconBrush,
    IconBucket,
    IconEyedropper,
    IconText
};

static void drawSelectIcon(QPainter* p, const QRect& r)
{
    p->setRenderHint(QPainter::Antialiasing, true);
    int cx = r.center().x();
    int cy = r.center().y();
    int s = qMin(r.width(), r.height()) / 2 - 6;
    p->setPen(QPen(p->pen().color(), 4, Qt::SolidLine, Qt::RoundCap));
    p->drawLine(cx, cy - s, cx, cy - s/3);
    p->drawLine(cx, cy + s/3, cx, cy + s);
    p->drawLine(cx - s, cy, cx - s/3, cy);
    p->drawLine(cx + s/3, cy, cx + s, cy);
    p->drawLine(cx - s, cy - s, cx - s/2, cy - s/2);
    p->drawLine(cx + s, cy - s, cx + s/2, cy - s/2);
    p->drawLine(cx - s, cy + s, cx - s/2, cy + s/2);
    p->drawLine(cx + s, cy + s, cx + s/2, cy + s/2);
}

static void drawRectIcon(QPainter* p, const QRect& r)
{
    p->setRenderHint(QPainter::Antialiasing, true);
    int m = 8;
    p->setPen(QPen(p->pen().color(), 4));
    p->setBrush(Qt::NoBrush);
    p->drawRect(r.adjusted(m, m, -m, -m));
}

static void drawEllipseIcon(QPainter* p, const QRect& r)
{
    p->setRenderHint(QPainter::Antialiasing, true);
    int m = 8;
    p->setPen(QPen(p->pen().color(), 4));
    p->setBrush(Qt::NoBrush);
    p->drawEllipse(r.adjusted(m, m, -m, -m));
}

static void drawLineIcon(QPainter* p, const QRect& r)
{
    p->setRenderHint(QPainter::Antialiasing, true);
    int m = 10;
    p->setPen(QPen(p->pen().color(), 4, Qt::SolidLine, Qt::RoundCap));
    p->drawLine(r.left() + m, r.bottom() - m, r.right() - m, r.top() + m);
}

static void drawRoundedRectIcon(QPainter* p, const QRect& r)
{
    p->setRenderHint(QPainter::Antialiasing, true);
    int m = 8;
    int radius = 12;
    p->setPen(QPen(p->pen().color(), 4));
    p->setBrush(Qt::NoBrush);
    p->drawRoundedRect(r.adjusted(m, m, -m, -m), radius, radius);
}

static void drawBrushIcon(QPainter* p, const QRect& r)
{
    p->setRenderHint(QPainter::Antialiasing, true);
    p->setPen(QPen(p->pen().color(), 4, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    int x1 = r.left() + 4, y1 = r.bottom() - 4;
    int x2 = r.right() - 4, y2 = r.top() + 6;
    p->drawLine(x1, y1, x2, y2);
    p->drawLine(x2, y2, x2 - 4, y2 - 4);
    int bx = x2, by = y2;
    QPointF points[3] = {
        QPointF(bx, by - 4),
        QPointF(bx + 5, by + 1),
        QPointF(bx - 1, by + 5)
    };
    QPainterPath path;
    path.moveTo(points[0]);
    path.lineTo(points[1]);
    path.lineTo(points[2]);
    path.closeSubpath();
    p->setBrush(p->pen().color());
    p->drawPath(path);
    p->setBrush(Qt::NoBrush);
}

static void drawBucketIcon(QPainter* p, const QRect& r)
{
    p->setRenderHint(QPainter::Antialiasing, true);
    p->setPen(QPen(p->pen().color(), 4, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    int l = r.left() + 4, t = r.top() + 5;
    int w = r.width() - 8, h = r.height() - 10;
    QPainterPath path;
    path.moveTo(l, t + 3);
    path.lineTo(l + w, t + 3);
    path.lineTo(l + w - 2, t + h);
    path.lineTo(l + 2, t + h);
    path.closeSubpath();
    p->drawPath(path);
    p->drawLine(l + 2, t + 3, l + 3, t);
    p->drawLine(l + w - 2, t + 3, l + w - 3, t);
    p->drawLine(l + 3, t, l + w - 3, t);
    p->drawLine(l + w - 1, t + 5, l + w + 2, t + 4);
    QPainterPath drip;
    drip.addEllipse(QPoint(l + w + 1, t + 8), 2, 3);
    p->setBrush(p->pen().color());
    p->drawPath(drip);
    p->setBrush(Qt::NoBrush);
}

static void drawEyedropperIcon(QPainter* p, const QRect& r)
{
    p->setRenderHint(QPainter::Antialiasing, true);
    p->setPen(QPen(p->pen().color(), 4, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    int x1 = r.left() + 5, y1 = r.bottom() - 5;
    int x2 = r.right() - 4, y2 = r.top() + 4;
    p->drawLine(x1, y1, x1 + 4, y1 - 4);
    p->drawLine(x1, y1, x1 - 3, y1 + 3);
    QPainterPath tube;
    tube.moveTo(x1 + 4, y1 - 4);
    tube.lineTo(x2 - 3, y2 - 1);
    tube.lineTo(x2 + 2, y2 + 4);
    tube.lineTo(x1 + 9, y1 + 1);
    tube.closeSubpath();
    p->drawPath(tube);
    p->drawLine(x2 - 3, y2 - 1, x2 + 2, y2 + 4);
}

static void drawTextIcon(QPainter* p, const QRect& r)
{
    p->setRenderHint(QPainter::Antialiasing, true);
    p->setPen(QPen(p->pen().color(), 4, Qt::SolidLine, Qt::RoundCap));
    QFont f = p->font();
    f.setBold(true);
    f.setPixelSize(r.height() - 16);
    p->setFont(f);
    p->drawText(r, Qt::AlignCenter, QStringLiteral("T"));
}

static QPixmap createToolIcon(ToolIconType type, const QColor& color, int size)
{
    QPixmap pixmap(size, size);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setPen(QPen(color, 4));
    painter.setBrush(Qt::NoBrush);
    QRect r(0, 0, size, size);
    switch (type) {
    case IconSelect:       drawSelectIcon(&painter, r); break;
    case IconRect:         drawRectIcon(&painter, r); break;
    case IconEllipse:      drawEllipseIcon(&painter, r); break;
    case IconLine:         drawLineIcon(&painter, r); break;
    case IconRoundedRect:  drawRoundedRectIcon(&painter, r); break;
    case IconBrush:        drawBrushIcon(&painter, r); break;
    case IconBucket:       drawBucketIcon(&painter, r); break;
    case IconEyedropper:   drawEyedropperIcon(&painter, r); break;
    case IconText:         drawTextIcon(&painter, r); break;
    }
    painter.end();
    return pixmap;
}

static QIcon createToolIcon(ToolIconType type)
{
    const int s = 60;
    QIcon icon;
    QPixmap normal = createToolIcon(type, QColor(60, 60, 60), s);
    QPixmap white = createToolIcon(type, QColor(255, 255, 255), s);
    icon.addPixmap(normal, QIcon::Normal, QIcon::Off);
    icon.addPixmap(white, QIcon::Normal, QIcon::On);
    icon.addPixmap(white, QIcon::Selected, QIcon::On);
    icon.addPixmap(white, QIcon::Active, QIcon::On);
    return icon;
}

ToolsPanel::ToolsPanel(QWidget* parent)
    : QWidget(parent)
    , m_toolGroup(new QButtonGroup(this))
    , m_colorBtn(new QPushButton(this))
    , m_foregroundColor(Qt::black)
    , m_lastToolId(ToolSelect)
{
    m_toolGroup->setExclusive(false);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(2, 2, 2, 2);
    layout->setSpacing(1);

    setFixedWidth(104);

    struct ToolDef {
        ToolIconType icon;
        QString tooltip;
        int id;
    };

    const ToolDef tools[] = {
        {IconSelect,      QStringLiteral("选择工具 (V)"),    ToolSelect},
        {IconRect,        QStringLiteral("矩形工具 (R)"),    ToolRectangle},
        {IconEllipse,     QStringLiteral("椭圆工具 (O)"),    ToolEllipse},
        {IconLine,        QStringLiteral("直线工具 (L)"),    ToolLine},
        {IconRoundedRect, QStringLiteral("圆角矩形 (U)"),    ToolRoundedRect},
        {IconBrush,       QStringLiteral("画笔工具 (B)"),    ToolBrush},
        {IconBucket,      QStringLiteral("油漆桶 (G)"),      ToolPaintBucket},
        {IconEyedropper,  QStringLiteral("吸管工具 (I)"),    ToolEyedropper},
        {IconText,        QStringLiteral("文字工具 (T)"),    ToolText},
    };

    for (const auto& tool : tools) {
        QToolButton* btn = createToolButton(tool.icon, tool.tooltip, tool.id);
        layout->addWidget(btn);
    }

    layout->addStretch();

    m_colorBtn->setFixedSize(84, 84);
    m_colorBtn->setToolTip(QStringLiteral("前景色 - 点击修改"));
    layout->addWidget(m_colorBtn, 0, Qt::AlignCenter);

    updateColorButton();

    QToolButton* selectBtn = qobject_cast<QToolButton*>(m_toolGroup->button(ToolSelect));
    if (selectBtn) {
        selectBtn->setChecked(true);
    }

    connect(m_toolGroup, QOverload<int>::of(&QButtonGroup::idClicked),
            this, [this](int id) {
        QToolButton* clickedBtn = qobject_cast<QToolButton*>(m_toolGroup->button(id));
        if (!clickedBtn) return;

        if (id == ToolSelect) {
            // 选择工具：确保选中
            selectTool(ToolSelect);
            emit toolChanged(ToolSelect);
        } else {
            if (clickedBtn->isChecked()) {
                // 按钮刚被选中（首次点击）→ 切换到该工具
                selectTool(id);
                emit toolChanged(id);
            } else {
                // 按钮刚被取消选中（二次点击）→ 切换回选择工具
                selectTool(ToolSelect);
                emit toolChanged(ToolSelect);
            }
        }
    });

    connect(m_colorBtn, &QPushButton::clicked, this, [this]() {
        QColor newColor = QColorDialog::getColor(m_foregroundColor, this, QStringLiteral("选择前景色"));
        if (newColor.isValid()) {
            setForegroundColor(newColor);
            emit foregroundColorChanged(m_foregroundColor);
        }
    });
}

int ToolsPanel::currentTool() const
{
    int id = m_toolGroup->checkedId();
    return (id >= 0) ? id : ToolSelect;
}

QColor ToolsPanel::foregroundColor() const
{
    return m_foregroundColor;
}

void ToolsPanel::setForegroundColor(const QColor& color)
{
    m_foregroundColor = color;
    updateColorButton();
}

void ToolsPanel::selectTool(int toolId)
{
    // 取消所有按钮选中
    for (auto* btn : m_toolGroup->buttons()) {
        QToolButton* tb = qobject_cast<QToolButton*>(btn);
        if (tb) {
            btn->setChecked(false);
        }
    }
    // 选中目标按钮
    QToolButton* targetBtn = qobject_cast<QToolButton*>(m_toolGroup->button(toolId));
    if (targetBtn) {
        targetBtn->setChecked(true);
    }
    m_lastToolId = toolId;
}

QToolButton* ToolsPanel::createToolButton(int iconId, const QString& tooltip, int toolId)
{
    QToolButton* btn = new QToolButton(this);
    btn->setIcon(createToolIcon(static_cast<ToolIconType>(iconId)));
    btn->setIconSize(QSize(60, 60));
    btn->setToolTip(tooltip);
    btn->setCheckable(true);
    btn->setFixedSize(96, 96);
    btn->setAutoRaise(true);

    btn->setStyleSheet(QStringLiteral(
        "QToolButton {"
        "  border: 1px solid transparent;"
        "  border-radius: 3px;"
        "  background: transparent;"
        "}"
        "QToolButton:hover {"
        "  background: #d0d0d0;"
        "  border-color: #b0b0b0;"
        "}"
        "QToolButton:checked {"
        "  background: #5a5a5a;"
        "  border-color: #3a3a3a;"
        "}"
        "QToolButton:pressed {"
        "  background: #4a4a4a;"
        "}"
    ));

    m_toolGroup->addButton(btn, toolId);
    return btn;
}

void ToolsPanel::updateColorButton()
{
    if (m_foregroundColor.isValid()) {
        m_colorBtn->setStyleSheet(
            QString::fromUtf8("background-color: %1; border: 2px solid #888; border-radius: 3px;")
                .arg(m_foregroundColor.name()));
    } else {
        m_colorBtn->setStyleSheet(
            QString::fromUtf8("background-color: #000000; border: 2px solid #888; border-radius: 3px;"));
    }
}
