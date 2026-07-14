#include "SnapGuide.h"
#include "layout/LayoutConstants.h"

#include <QtGlobal>

// ============================================================
// 构造函数
// ============================================================
SnapGuide::SnapGuide()
    : m_threshold(10.0)
    , m_enabled(true)
{
}

// ============================================================
// 阈值/启用开关
// ============================================================
void SnapGuide::setThreshold(qreal threshold) { m_threshold = threshold; }
qreal SnapGuide::threshold() const { return m_threshold; }

void SnapGuide::setEnabled(bool enabled) { m_enabled = enabled; }
bool SnapGuide::enabled() const { return m_enabled; }

// ============================================================
// pageGuides - 返回页面参考线（中心线+边距线）
//
// 垂直线（isHorizontal=false，position=x值）：
//   PageCenterV:     x = A4_WIDTH/2
//   PageMarginLeft:  x = PAGE_MARGIN
//   PageMarginRight: x = A4_WIDTH - PAGE_MARGIN
// 水平线（isHorizontal=true，position=y值）：
//   PageCenterH:      y = A4_HEIGHT/2
//   PageMarginTop:    y = PAGE_MARGIN
//   PageMarginBottom: y = A4_HEIGHT - PAGE_MARGIN
// ============================================================
QList<GuideLine> SnapGuide::pageGuides() const
{
    QList<GuideLine> guides;
    // 页面中心（垂直线+水平线）
    guides << GuideLine(PageCenterV, false, A4_WIDTH / 2.0);
    guides << GuideLine(PageCenterH, true, A4_HEIGHT / 2.0);
    // 边距线（左右垂直、上下水平）
    guides << GuideLine(PageMarginLeft, false, PAGE_MARGIN);
    guides << GuideLine(PageMarginRight, false, A4_WIDTH - PAGE_MARGIN);
    guides << GuideLine(PageMarginTop, true, PAGE_MARGIN);
    guides << GuideLine(PageMarginBottom, true, A4_HEIGHT - PAGE_MARGIN);
    return guides;
}

// ============================================================
// generateGuides - 生成所有候选参考线
//
// 页面参考线 + 每个其他元素的6条线（左/右/中心V/上/下/中心H）。
// otherRects已排除被拖拽元素本身，故无需额外过滤。
// ============================================================
QList<GuideLine> SnapGuide::generateGuides(const QRectF& draggedRect,
                                           const QList<QRectF>& otherRects) const
{
    Q_UNUSED(draggedRect);  // 当前实现不依赖被拖拽元素尺寸生成参考线
    QList<GuideLine> guides = pageGuides();
    for (const QRectF& r : otherRects) {
        // 垂直线（x值）
        guides << GuideLine(ElementLeft, false, r.left());
        guides << GuideLine(ElementRight, false, r.right());
        guides << GuideLine(ElementCenterV, false, r.center().x());
        // 水平线（y值）
        guides << GuideLine(ElementTop, true, r.top());
        guides << GuideLine(ElementBottom, true, r.bottom());
        guides << GuideLine(ElementCenterH, true, r.center().y());
    }
    return guides;
}

// ============================================================
// trySnap - 检查单点吸附
//
// 若 |value - guide| <= threshold，则snapped=true并返回guide（吸附位置）；
// 否则snapped=false并返回原value。
// ============================================================
qreal SnapGuide::trySnap(qreal value, qreal guide, qreal threshold, bool& snapped) const
{
    qreal delta = guide - value;
    if (qAbs(delta) <= threshold) {
        snapped = true;
        return guide;
    }
    snapped = false;
    return value;
}

// ============================================================
// calculateSnap - 计算吸附位置
//
// 算法：
//   1. 由targetPos和draggedRect尺寸构造目标矩形
//   2. 生成候选参考线（页面+其他元素）
//   3. X轴：被拖拽元素的3个锚点(left/right/centerX)对每条垂直线
//      做吸附检查，取|delta|最小的吸附，调整targetPos.x
//   4. Y轴：同理，3个锚点(top/bottom/centerY)对水平线吸附
//   5. 返回调整后位置和活跃参考线
//
// X、Y轴独立吸附：可只吸附X、只吸附Y、两者都吸附或都不吸附。
// 每个轴只取最近的一条参考线（避免多重吸附冲突）。
//
// 位移计算：把锚点移到参考线，
//   newTargetPos.x = targetPos.x + (guideX - anchorX)
// 因为 targetRect.left()=targetPos.x，故左锚点位移=guideX-targetPos.x，
// 新位置=targetPos.x+位移=guideX，恰好使左边缘对齐参考线。其他锚点同理。
// ============================================================
SnapGuide::SnapResult SnapGuide::calculateSnap(const QRectF& draggedRect,
                                               const QPointF& targetPos,
                                               const QList<QRectF>& otherRects) const
{
    SnapResult result;
    result.adjustedPos = targetPos;

    if (!m_enabled) {
        return result;
    }

    // 目标矩形：保持尺寸，移动到targetPos
    QRectF targetRect(targetPos, draggedRect.size());

    // 生成候选参考线
    QList<GuideLine> guides = generateGuides(draggedRect, otherRects);

    // ---- X轴吸附（垂直线，position=x值）----
    {
        qreal anchorsX[3] = {
            targetRect.left(),
            targetRect.right(),
            targetRect.center().x()
        };

        bool xSnapped = false;
        qreal bestDeltaX = 0.0;
        GuideLine bestGuideX;

        for (const GuideLine& g : guides) {
            if (g.isHorizontal) continue;  // 仅处理垂直线
            for (int i = 0; i < 3; ++i) {
                bool snapped = false;
                trySnap(anchorsX[i], g.position, m_threshold, snapped);
                if (!snapped) continue;
                qreal delta = g.position - anchorsX[i];
                if (!xSnapped || qAbs(delta) < qAbs(bestDeltaX)) {
                    xSnapped = true;
                    bestDeltaX = delta;
                    bestGuideX = g;
                }
            }
        }

        if (xSnapped) {
            // 把锚点移到参考线：newPos.x = targetPos.x + delta
            result.adjustedPos.setX(targetPos.x() + bestDeltaX);
            bestGuideX.active = true;
            result.activeGuides << bestGuideX;
        }
    }

    // ---- Y轴吸附（水平线，position=y值）----
    {
        qreal anchorsY[3] = {
            targetRect.top(),
            targetRect.bottom(),
            targetRect.center().y()
        };

        bool ySnapped = false;
        qreal bestDeltaY = 0.0;
        GuideLine bestGuideY;

        for (const GuideLine& g : guides) {
            if (!g.isHorizontal) continue;  // 仅处理水平线
            for (int i = 0; i < 3; ++i) {
                bool snapped = false;
                trySnap(anchorsY[i], g.position, m_threshold, snapped);
                if (!snapped) continue;
                qreal delta = g.position - anchorsY[i];
                if (!ySnapped || qAbs(delta) < qAbs(bestDeltaY)) {
                    ySnapped = true;
                    bestDeltaY = delta;
                    bestGuideY = g;
                }
            }
        }

        if (ySnapped) {
            result.adjustedPos.setY(targetPos.y() + bestDeltaY);
            bestGuideY.active = true;
            result.activeGuides << bestGuideY;
        }
    }

    return result;
}
