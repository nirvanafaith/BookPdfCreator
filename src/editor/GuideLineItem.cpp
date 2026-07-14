#include "GuideLineItem.h"
#include "layout/LayoutConstants.h"

#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <QPen>

// 蓝色参考线（与项目选中边框主题色一致 #1976D2）
const QColor GuideLineItem::GUIDE_COLOR(25, 118, 210);
// 参考线宽度
const qreal GuideLineItem::GUIDE_WIDTH = 1.0;

// ============================================================
// 构造函数
//
// Item位于场景原点(0,0)，本地坐标直接对应页面坐标。
// Z值9998：在元素之上，选中装饰器之下。
// 不参与选择/移动/碰撞，仅作视觉提示。
// ============================================================
GuideLineItem::GuideLineItem(const GuideLine& guide, QGraphicsItem* parent)
    : QGraphicsItem(parent)
    , m_guide(guide)
{
    // 位于场景原点，本地坐标直接对应页面坐标
    setPos(0.0, 0.0);
    // Z值很高：在元素之上，选中装饰器之下
    setZValue(9998);
    // 不参与选择和移动
    setFlag(ItemIsSelectable, false);
    setFlag(ItemIsMovable, false);
    // 参考线频繁更新，不缓存
    setCacheMode(NoCache);
}

// ============================================================
// boundingRect - 包围矩形
//
// 水平线：横跨页面宽度，y=position附近一条窄带
// 垂直线：纵跨页面高度，x=position附近一条窄带
// 上下/左右各留(GUIDE_WIDTH+1)像素余量，避免虚线被裁剪。
// ============================================================
QRectF GuideLineItem::boundingRect() const
{
    const qreal pad = GUIDE_WIDTH + 1.0;
    if (m_guide.isHorizontal) {
        // 水平线：y=position
        return QRectF(0.0, m_guide.position - pad, A4_WIDTH, pad * 2.0);
    } else {
        // 垂直线：x=position
        return QRectF(m_guide.position - pad, 0.0, pad * 2.0, A4_HEIGHT);
    }
}

// ============================================================
// paint - 绘制参考线
//
// 蓝色虚线（Qt::DashLine）。
// 水平参考线：从(0, position)到(A4_WIDTH, position)
// 垂直参考线：从(position, 0)到(position, A4_HEIGHT)
// ============================================================
void GuideLineItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
                          QWidget* widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    painter->save();
    QPen pen(GUIDE_COLOR, GUIDE_WIDTH, Qt::DashLine);
    painter->setPen(pen);
    painter->setBrush(Qt::NoBrush);

    if (m_guide.isHorizontal) {
        painter->drawLine(QPointF(0.0, m_guide.position),
                          QPointF(A4_WIDTH, m_guide.position));
    } else {
        painter->drawLine(QPointF(m_guide.position, 0.0),
                          QPointF(m_guide.position, A4_HEIGHT));
    }

    painter->restore();
}

// ============================================================
// setGuide - 更新参考线（位置/方向变化时调用）
//
// prepareGeometryChange()确保旧区域被正确重绘，
// 随后更新m_guide并触发update()重绘新区域。
// ============================================================
void GuideLineItem::setGuide(const GuideLine& guide)
{
    prepareGeometryChange();
    m_guide = guide;
    update();
}

// ============================================================
// guide - 返回当前参考线
// ============================================================
GuideLine GuideLineItem::guide() const
{
    return m_guide;
}
