#ifndef GUIDELINEITEM_H
#define GUIDELINEITEM_H

#include <QGraphicsItem>
#include <QColor>

#include "SnapGuide.h"

// ============================================================
// GuideLineItem - 参考线显示Item
//
// 在场景中绘制单条对齐参考线（蓝色虚线）。
// 水平参考线横跨页面宽度，垂直参考线纵跨页面高度。
// Z值很高(9998)，显示在元素之上但选中装饰器之下。
//
// 坐标系：Item位于场景原点(0,0)，本地坐标直接对应页面/场景坐标，
// 因此水平线从(0, position)到(A4_WIDTH, position)，
// 垂直线从(position, 0)到(position, A4_HEIGHT)。
//
// 使用方式：EditorScene在吸附发生时为每条活跃参考线创建/更新
// GuideLineItem，拖拽结束后移除。
// ============================================================
class GuideLineItem : public QGraphicsItem
{
public:
    GuideLineItem(const GuideLine& guide, QGraphicsItem* parent = nullptr);

    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
               QWidget* widget) override;

    // 更新参考线（位置/方向变化时调用，触发重绘）
    void setGuide(const GuideLine& guide);
    GuideLine guide() const;

private:
    GuideLine m_guide;
    static const QColor GUIDE_COLOR;  // 蓝色
    static const qreal GUIDE_WIDTH;   // 线宽 1.0
};

#endif // GUIDELINEITEM_H
