#ifndef IMAGEEDITORITEM_H
#define IMAGEEDITORITEM_H

#include "BaseEditorItem.h"
#include "PageElement.h"

// ============================================================
// ImageEditorItem - 图片编辑Item
//
// 图片元素的编辑器表示。渲染委托给基类 BaseEditorItem
// （内部经由 ElementRenderer 处理不透明度、宽高比、缩放，
// 以及图片加载失败时的占位符显示）。
//
// 此类保留类型标识（Type = UserType + 3），paint() 委托给基类。
// boundingRect() 与 drawSelectionBorder() 使用基类实现
// （基于元素 rect），保证与 Qt QGraphicsItem 契约一致。
// ============================================================
class ImageEditorItem : public BaseEditorItem
{
public:
    ImageEditorItem(const ImageElementPtr& element, QGraphicsItem* parent = nullptr);

    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
               QWidget* widget) override;

    // 类型标识
    enum { Type = UserType + 3 };
    int type() const override { return Type; }
};

#endif // IMAGEEDITORITEM_H
