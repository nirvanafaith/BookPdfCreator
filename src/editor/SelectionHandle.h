#ifndef SELECTIONHANDLE_H
#define SELECTIONHANDLE_H

#include <QGraphicsItem>

// ============================================================
// HandlePosition - 手柄位置枚举
//
// 8个缩放手柄（四角+四边中点）+ 1个旋转手柄。
// EditorScene通过itemAt判断鼠标点击的手柄类型，
// 据此执行对应的缩放/旋转操作。
// ============================================================
enum HandlePosition {
    TopLeft,     // 左上角 - 双向缩放
    Top,         // 顶部中点 - 垂直缩放
    TopRight,    // 右上角 - 双向缩放
    Left,        // 左侧中点 - 水平缩放
    Right,       // 右侧中点 - 水平缩放
    BottomLeft,  // 左下角 - 双向缩放
    Bottom,      // 底部中点 - 垂直缩放
    BottomRight, // 右下角 - 双向缩放
    Rotation     // 旋转手柄 - 旋转操作
};

// ============================================================
// SelectionHandle - 变换手柄
//
// 选中元素的视觉交互手柄。作为SelectionDecorator的子Item，
// 显示在选中包围盒的8个缩放位置和1个旋转位置。
//
// 设计要点：
//   - 缩放手柄：8x8像素白色方块，蓝色边框
//   - 旋转手柄：圆形（同尺寸），位于顶部上方20像素
//   - 手柄本身不处理鼠标事件（setAcceptedMouseButtons=NoButton），
//     由EditorScene的mousePressEvent通过itemAt判断并处理
//   - 手柄的(0,0)为中心，setPos到边缘点后中心对齐
// ============================================================
class SelectionHandle : public QGraphicsItem
{
public:
    SelectionHandle(HandlePosition pos, QGraphicsItem* parent = nullptr);

    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
               QWidget* widget) override;

    HandlePosition handlePosition() const;

    // 类型标识
    enum { Type = UserType + 10 };
    int type() const override { return Type; }

    // 手柄尺寸（像素）
    static const qreal HANDLE_SIZE;

private:
    HandlePosition m_pos;  // 手柄位置类型
};

#endif // SELECTIONHANDLE_H
