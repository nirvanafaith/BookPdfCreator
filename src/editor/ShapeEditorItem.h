#ifndef SHAPEEDITORITEM_H
#define SHAPEEDITORITEM_H

#include "BaseEditorItem.h"
#include "PageElement.h"

// ============================================================
// ShapeEditorItem - 形状编辑Item
//
// 形状元素（矩形/圆角矩形/直线/椭圆）的编辑器表示。
// 渲染委托给ElementRenderer，后者已处理：
//   - 四种形状类型的绘制
//   - 纯色/渐变填充
//   - 边框（颜色、宽度）
//   - 圆角半径（RoundedRect）
//
// 此类提供独立的类型标识，便于qgraphicsitem_cast区分元素类型，
// 并为未来形状特定编辑功能（如顶点编辑）预留扩展点。
// ============================================================
class ShapeEditorItem : public BaseEditorItem
{
public:
    ShapeEditorItem(const ShapeElementPtr& element, QGraphicsItem* parent = nullptr);

    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
               QWidget* widget) override;

    // 类型标识
    enum { Type = UserType + 4 };
    int type() const override { return Type; }
};

#endif // SHAPEEDITORITEM_H
