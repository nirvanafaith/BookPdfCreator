#ifndef MULTICOLUMNLAYOUT_H
#define MULTICOLUMNLAYOUT_H

#include "LayoutEngine.h"
#include "PainterHelpers.h"
#include "ImageCache.h"

// 多图排版引擎
// 紧凑列表布局：每页约3本书，封面在右侧，左侧文字信息
class MultiColumnLayout : public LayoutEngine
{
public:
    MultiColumnLayout();
    ~MultiColumnLayout() override;

    // 计算布局
    void calculateLayout() override;

    // 获取页数
    int pageCount() const override;

    // 渲染指定页面
    void renderPage(QPainter* painter, int pageIndex, const QRectF& pageRect) override;

    // 生成页面元素列表（用于编辑器和PDF导出）
    // 将renderPage的绘制逻辑转换为PageElement数据元素列表
    QList<PageElementPtr> generateElements(int pageIndex, const QRectF& pageRect) override;

    // 获取页面类型：1=多图排版
    int pageType(int pageIndex) const override;

private:
    // 计算单本书条目所需高度
    qreal calculateBookItemHeight(QPainter* painter, const BookPtr& book, qreal contentWidth);

    // 绘制单本书条目
    void drawBookItem(QPainter* painter, const BookPtr& book, const QRectF& itemRect);

    // 生成单本书条目的页面元素
    // 对应drawBookItem的元素生成版本
    void generateBookItemElements(QList<PageElementPtr>& elements, int& zOrder,
                                  const BookPtr& book, const QRectF& itemRect,
                                  QPainter* tempPainter);

    // 计算内容简介正文高度（用于元素rect计算）
    qreal calculateDescHeight(QPainter* painter, const BookPtr& book, qreal textWidth, qreal labelWidth);

    QList<QList<BookPtr>> m_pages; // 分页后的书籍列表
    QList<QList<qreal>> m_pageHeights; // 每页各书籍条目高度（与m_pages对应，确保分页与渲染一致）
    int m_pageCount;               // 总页数

    // ========== 布局间距常量 ==========
    static const qreal SPACING_BANNER_BOTTOM;  // 横幅下方间距
    static const qreal SPACING_BOOK_GAP;       // 书籍条目间底部间距
    static const qreal COVER_WIDTH;            // 封面小图宽度
    static const qreal SPACING_TEXT_COVER_GAP; // 文字区与封面间距
    static const qreal SPACING_LINE_GAP;       // 文字行间间距
    static const qreal DESC_LINE_HEIGHT;       // 内容简介行高
    static const qreal DIVIDER_SPACING;        // 分隔线上下间距
};

#endif // MULTICOLUMNLAYOUT_H
