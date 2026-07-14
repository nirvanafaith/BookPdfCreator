#include "ImageEditorItem.h"

#include <QPainter>
#include <QStyleOptionGraphicsItem>

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
// 委托给基类paint，后者调用ElementRenderer::renderElement。
// ElementRenderer::renderImage已完整处理：
//   - 通过ImageCache获取图片（带缓存）
//   - 不透明度（painter->setOpacity）
//   - scaleFactor / keepAspectRatio 缩放策略
//   - 图片加载失败时绘制占位符
// ============================================================
void ImageEditorItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
                            QWidget* widget)
{
    BaseEditorItem::paint(painter, option, widget);
}
