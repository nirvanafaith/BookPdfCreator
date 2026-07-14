#ifndef IMAGEEDITORITEM_H
#define IMAGEEDITORITEM_H

#include "BaseEditorItem.h"
#include "PageElement.h"

// ============================================================
// ImageEditorItem - 图片编辑Item
//
// 图片元素的编辑器表示。渲染委托给ElementRenderer，
// 后者已处理不透明度（setOpacity）、宽高比、缩放，
// 以及图片加载失败时的占位符显示（灰色矩形+"图片缺失"文字）。
//
// 此类的paint()覆盖基类实现，便于未来添加图片特定的
// 编辑视觉反馈（如裁剪框、对齐参考线等）。
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
