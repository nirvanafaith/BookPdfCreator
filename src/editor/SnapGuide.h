#ifndef SNAPGUIDE_H
#define SNAPGUIDE_H

#include <QPointF>
#include <QRectF>
#include <QList>

// ============================================================
// 对齐参考线与智能吸附系统
//
// 在元素拖拽时提供智能吸附：
//   - 页面参考线：水平/垂直中心线、四条边距线
//   - 元素参考线：其他元素的左/右/上/下边缘及水平/垂直中心
// 当被拖拽元素的锚点（左/右/上/下边缘、中心）接近参考线在阈值内时，
// 自动吸附到参考线，并返回活跃参考线供界面高亮显示。
//
// 坐标系：场景坐标 = 页面坐标（72DPI点单位），原点(0,0)为页面左上角
// ============================================================

// 参考线类型
enum GuideType {
    None = 0,
    PageCenterH,      // 页面水平中心
    PageCenterV,      // 页面垂直中心
    PageMarginLeft,   // 左边距
    PageMarginRight,  // 右边距
    PageMarginTop,    // 上边距
    PageMarginBottom, // 下边距
    ElementLeft,      // 其他元素左边缘
    ElementRight,     // 其他元素右边缘
    ElementTop,       // 其他元素上边缘
    ElementBottom,    // 其他元素下边缘
    ElementCenterH,   // 其他元素水平中心
    ElementCenterV    // 其他元素垂直中心
};

// 参考线（水平或垂直）
struct GuideLine
{
    GuideType type;       // 参考线类型
    bool isHorizontal;    // true=水平线(y固定), false=垂直线(x固定)
    qreal position;       // 线的坐标（水平线=y值，垂直线=x值）
    bool active;          // 是否正在吸附

    GuideLine()
        : type(None), isHorizontal(true), position(0.0), active(false) {}
    GuideLine(GuideType t, bool horiz, qreal pos)
        : type(t), isHorizontal(horiz), position(pos), active(false) {}
};

// ============================================================
// SnapGuide - 智能吸附管理器
//
// 纯逻辑类（非QObject），由EditorScene在拖拽过程中调用。
// 计算吸附位置后，EditorScene据此移动元素并创建/更新GuideLineItem
// 以高亮显示活跃参考线。
// ============================================================
class SnapGuide
{
public:
    SnapGuide();

    // 设置吸附阈值（像素/点）
    void setThreshold(qreal threshold);
    qreal threshold() const;

    // 设置是否启用吸附
    void setEnabled(bool enabled);
    bool enabled() const;

    // 吸附结果
    struct SnapResult {
        QPointF adjustedPos;           // 调整后的位置（元素左上角）
        QList<GuideLine> activeGuides; // 活跃的参考线列表
    };

    // 计算吸附位置
    //   draggedRect: 被拖拽元素的当前rect（用于获取尺寸）
    //   targetPos:   拖拽目标位置（元素左上角，场景坐标）
    //   otherRects:  页面上其他元素的rect列表
    // 返回：调整后的位置和活跃的参考线列表
    SnapResult calculateSnap(const QRectF& draggedRect, const QPointF& targetPos,
                             const QList<QRectF>& otherRects) const;

    // 获取页面参考线（中心线、边距线）
    QList<GuideLine> pageGuides() const;

private:
    qreal m_threshold;  // 吸附阈值，默认10.0
    bool m_enabled;     // 是否启用吸附

    // 生成所有候选参考线（页面+其他元素）
    QList<GuideLine> generateGuides(const QRectF& draggedRect,
                                    const QList<QRectF>& otherRects) const;
    // 检查单点吸附：|value-guide|<=threshold时snapped=true并返回guide，
    // 否则snapped=false并返回原value
    qreal trySnap(qreal value, qreal guide, qreal threshold, bool& snapped) const;
};

#endif // SNAPGUIDE_H
