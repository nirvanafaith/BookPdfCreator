#include "ShapeEditorItem.h"

#include <QPainter>
#include <QStyleOptionGraphicsItem>

// ============================================================
// 构造函数
//
// ShapeElementPtr -> PageElementPtr 转换：
// 通过clone()获取非const的ShapeElementData*，上转为PageElementData*，
// 构造PageElementPtr传给基类。
// ============================================================
ShapeEditorItem::ShapeEditorItem(const ShapeElementPtr& element, QGraphicsItem* parent)
    : BaseEditorItem(PageElementPtr(element.constData()->clone()), parent)
{
}

// ============================================================
// paint - 形状渲染
//
// 委托给基类paint，后者调用ElementRenderer::renderElement。
// ElementRenderer::renderShape已完整处理：
//   - Rectangle / RoundedRect / Line / Ellipse 四种形状
//   - 纯色填充与线性渐变填充
//   - 边框绘制（QPen: borderColor, borderWidth）
//   - 圆角半径（仅RoundedRect）
// ============================================================
void ShapeEditorItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
                            QWidget* widget)
{
    BaseEditorItem::paint(painter, option, widget);
}
