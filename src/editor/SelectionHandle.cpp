#include "SelectionHandle.h"

#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <QPen>
#include <QBrush>
#include <QColor>

// 手柄尺寸：8x8像素方块/圆形
const qreal SelectionHandle::HANDLE_SIZE = 8.0;

// ============================================================
// 构造函数
//
// 手柄不处理鼠标事件：setAcceptedMouseButtons(NoButton)确保
// 鼠标点击穿透到EditorScene，由场景通过itemAt识别手柄。
// 同时禁用选中和移动标志，避免干扰场景的选择逻辑。
// ============================================================
SelectionHandle::SelectionHandle(HandlePosition pos, QGraphicsItem* parent)
    : QGraphicsItem(parent)
    , m_pos(pos)
{
    // 手柄不接收鼠标事件（由EditorScene通过itemAt统一处理）
    setAcceptedMouseButtons(Qt::NoButton);
    setFlag(ItemIsSelectable, false);
    setFlag(ItemIsMovable, false);
    // 手柄不影响光标形状（光标由EditorScene根据itemAt结果设置）
}

// ============================================================
// handlePosition - 返回手柄位置类型
// ============================================================
HandlePosition SelectionHandle::handlePosition() const
{
    return m_pos;
}

// ============================================================
// boundingRect - 手柄包围矩形
//
// 手柄中心在本地坐标(0,0)，boundingRect以中心为原点，
// 四周延伸HANDLE_SIZE/2。旋转手柄同尺寸（绘制为圆形）。
// ============================================================
QRectF SelectionHandle::boundingRect() const
{
    qreal half = HANDLE_SIZE / 2.0;
    return QRectF(-half, -half, HANDLE_SIZE, HANDLE_SIZE);
}

// ============================================================
// paint - 绘制手柄
//
// 缩放手柄：白色填充方块，蓝色边框
// 旋转手柄：白色填充圆形，蓝色边框
// ============================================================
void SelectionHandle::paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
                            QWidget* widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);

    // 白色填充，蓝色边框（#1976D2）
    painter->setBrush(QBrush(QColor(255, 255, 255)));
    painter->setPen(QPen(QColor(25, 118, 210), 1));

    qreal half = HANDLE_SIZE / 2.0;
    QRectF rect(-half, -half, HANDLE_SIZE, HANDLE_SIZE);

    if (m_pos == Rotation) {
        // 旋转手柄：圆形
        painter->drawEllipse(rect);
    } else {
        // 缩放手柄：方块
        painter->drawRect(rect);
    }

    painter->restore();
}
