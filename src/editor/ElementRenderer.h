#ifndef ELEMENTRENDERER_H
#define ELEMENTRENDERER_H

#include <QPainter>
#include "PageElement.h"
#include "PageData.h"

// ============================================================
// 元素渲染器
//
// 将PageElement列表用QPainter渲染到任意设备（屏幕预览或PDF导出）。
// 支持文本、图片、形状三种元素类型，处理旋转、对齐、缩放等属性。
//
// 使用方式：
//   QPainter painter(device);
//   ElementRenderer::renderPage(&painter, pageData);
//
// 或渲染单个元素：
//   ElementRenderer::renderElement(&painter, element);
// ============================================================
class ElementRenderer
{
public:
    // 渲染单个元素（检查可见性，应用旋转）
    static void renderElement(QPainter* painter, const PageElementPtr& element);

    // 渲染整个页面（所有可见元素，按zOrder排序）
    static void renderPage(QPainter* painter, const PageDataPtr& pageData);

private:
    // 各类型元素的渲染方法
    static void renderText(QPainter* painter, const TextElementData* text);
    static void renderImage(QPainter* painter, const ImageElementData* image);
    static void renderShape(QPainter* painter, const ShapeElementData* shape);
};

#endif // ELEMENTRENDERER_H
