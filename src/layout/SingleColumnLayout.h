#ifndef SINGLECOLUMNLAYOUT_H
#define SINGLECOLUMNLAYOUT_H

#include "LayoutEngine.h"
#include "PainterHelpers.h"
#include "ImageCache.h"

// 单图排版引擎
// 每页1本书，纵向A4布局：顶部横幅、上半部分封面+信息、内容简介、底部样章/二维码
class SingleColumnLayout : public LayoutEngine
{
public:
    SingleColumnLayout();
    ~SingleColumnLayout() override;

    // 计算布局
    void calculateLayout() override;

    // 获取页数
    int pageCount() const override;

    // 渲染指定页面
    void renderPage(QPainter* painter, int pageIndex, const QRectF& pageRect) override;

    // 生成页面元素列表（用于编辑器和PDF导出）
    // 将renderPage的绘制逻辑转换为PageElement数据元素列表
    QList<PageElementPtr> generateElements(int pageIndex, const QRectF& pageRect) override;

    // 获取页面类型：0=单图排版
    int pageType(int pageIndex) const override;

private:
    // 绘制底部图片区域（样章/二维码）
    void drawImageSection(QPainter* painter, const QRectF& rect,
                          const QString& title, const QStringList& imagePaths);

    // 在指定区域内自适应等比缩放绘制多张图片
    void drawImagesInRect(QPainter* painter, const QRectF& rect,
                          const QStringList& imagePaths);

    // 生成底部图片区域的页面元素（样章/二维码）
    // 对应drawImageSection + drawImagesInRect的元素生成版本
    void generateImageSectionElements(QList<PageElementPtr>& elements, int& zOrder,
                                      const QRectF& rect,
                                      const QString& title, const QStringList& imagePaths);

    int m_pageCount; // 总页数
};

#endif // SINGLECOLUMNLAYOUT_H
