#include "ImageEditorItem.h"

#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <QtGlobal>

// ============================================================
// 构造函数
//
// ImageElementPtr -> PageElementPtr 转换：
// 通过clone()获取非const的ImageElementData*，上转为PageElementData*，
// 构造PageElementPtr传给基类。
// ============================================================
ImageEditorItem::ImageEditorItem(const ImageElementPtr& element, QGraphicsItem* parent)
    : BaseEditorItem(PageElementPtr(element.constData()->clone()), parent)
{
}

// ============================================================
// paint - 图片渲染
//
// 委托给基类 BaseEditorItem::paint()，后者经由 ElementRenderer
// 完成图片绘制（不透明度、宽高比、缩放、占位符等）。
// 选中边框由基类 drawSelectionBorder() 在元素 rect 边缘绘制。
// ============================================================
void ImageEditorItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
                            QWidget* widget)
{
    BaseEditorItem::paint(painter, option, widget);
}
