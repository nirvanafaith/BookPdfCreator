#include "SelectionDecorator.h"

#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <QPen>
#include <QBrush>
#include <QColor>

// 旋转手柄距顶部边框的距离：20像素
const qreal SelectionDecorator::ROTATION_HANDLE_OFFSET = 20.0;

// ============================================================
// 构造函数
//
// 装饰器不参与选择和移动（由EditorScene统一管理交互）。
// 初始时不创建手柄，首次setRect+updateHandles时懒创建。
// ============================================================
SelectionDecorator::SelectionDecorator(QGraphicsItem* parent)
    : QGraphicsItem(parent)
{
    setFlag(ItemIsSelectable, false);
    setFlag(ItemIsMovable, false);
    setAcceptedMouseButtons(Qt::NoButton);
}

// ============================================================
// setRect - 设置装饰器覆盖矩形并更新手柄位置
// ============================================================
void SelectionDecorator::setRect(const QRectF& rect)
{
    prepareGeometryChange();
    m_rect = rect;
    updateHandles();
    update();
}

// ============================================================
// rect - 返回当前覆盖矩形
// ============================================================
QRectF SelectionDecorator::rect() const
{
    return m_rect;
}

// ============================================================
// boundingRect - 装饰器包围矩形
//
// 需覆盖：m_rect + 四周手柄尺寸 + 旋转手柄上方偏移。
// 旋转手柄在顶部上方ROTATION_HANDLE_OFFSET处，加上手柄半径。
// ============================================================
QRectF SelectionDecorator::boundingRect() const
{
    qreal halfHandle = SelectionHandle::HANDLE_SIZE / 2.0;
    // 顶部需额外容纳旋转手柄（偏移+手柄半径）
    qreal topExtra = ROTATION_HANDLE_OFFSET + halfHandle;

    return QRectF(
        m_rect.x() - halfHandle,
        m_rect.y() - topExtra,
        m_rect.width() + 2.0 * halfHandle,
        m_rect.height() + topExtra + halfHandle
    );
}

// ============================================================
// paint - 绘制蓝色虚线边框
//
// 手柄作为子Item自行绘制，此方法仅绘制选中边框。
// ============================================================
void SelectionDecorator::paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
                               QWidget* widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    painter->save();
    // 蓝色虚线边框（#1976D2），与BaseEditorItem选中边框风格一致
    QPen pen(QColor(25, 118, 210), 1, Qt::DashLine);
    painter->setPen(pen);
    painter->setBrush(Qt::NoBrush);
    painter->drawRect(m_rect);
    painter->restore();
}

// ============================================================
// updateHandles - 创建/更新9个手柄的位置
//
// 8个缩放手柄位于m_rect的四个角和四条边的中点，
// 1个旋转手柄位于顶部中点上方ROTATION_HANDLE_OFFSET处。
// 手柄首次调用时懒创建，后续仅更新位置。
// ============================================================
void SelectionDecorator::updateHandles()
{
    // 首次调用时创建9个手柄
    if (m_handles.isEmpty()) {
        for (int i = 0; i <= Rotation; ++i) {
            m_handles.append(new SelectionHandle(HandlePosition(i), this));
        }
    }

    // 计算各手柄位置（相对于装饰器本地坐标系）
    qreal x = m_rect.x();
    qreal y = m_rect.y();
    qreal w = m_rect.width();
    qreal h = m_rect.height();
    qreal cx = x + w / 2.0;   // 水平中心
    qreal cy = y + h / 2.0;   // 垂直中心

    // 四角
    m_handles[TopLeft]->setPos(x, y);
    m_handles[TopRight]->setPos(x + w, y);
    m_handles[BottomLeft]->setPos(x, y + h);
    m_handles[BottomRight]->setPos(x + w, y + h);

    // 四边中点
    m_handles[Top]->setPos(cx, y);
    m_handles[Bottom]->setPos(cx, y + h);
    m_handles[Left]->setPos(x, cy);
    m_handles[Right]->setPos(x + w, cy);

    // 旋转手柄：顶部中点上方20像素
    m_handles[Rotation]->setPos(cx, y - ROTATION_HANDLE_OFFSET);
}

// ============================================================
// handleAt - 获取指定位置类型的手柄
// ============================================================
SelectionHandle* SelectionDecorator::handleAt(HandlePosition pos) const
{
    if (pos >= 0 && pos < m_handles.size()) {
        return m_handles.at(pos);
    }
    return nullptr;
}

// ============================================================
// handles - 返回所有手柄
// ============================================================
QList<SelectionHandle*> SelectionDecorator::handles() const
{
    return m_handles;
}
