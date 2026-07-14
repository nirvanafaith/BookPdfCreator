#ifndef LAYOUTENGINE_H
#define LAYOUTENGINE_H

#include <QPainter>
#include <QRectF>
#include <QSharedPointer>
#include <QList>
#include "Book.h"
#include "editor/PageElement.h"
#include "editor/PageData.h"

// 排版引擎基类
class LayoutEngine
{
public:
    virtual ~LayoutEngine() {}

    // 设置书籍列表
    void setBooks(const BookList& books) { m_books = books; m_dirty = true; }
    // 获取书籍列表
    BookList books() const { return m_books; }

    // 计算布局（纯虚函数）
    virtual void calculateLayout() = 0;

    // 获取页数（纯虚函数）
    virtual int pageCount() const = 0;

    // 渲染指定页面（纯虚函数）
    virtual void renderPage(QPainter* painter, int pageIndex, const QRectF& pageRect) = 0;

    // 生成页面元素列表（用于编辑器和PDF导出）
    // 默认实现返回空列表，子类应该重写
    // pageIndex：页码索引（从0开始）
    // pageRect：页面矩形区域（72DPI下的点单位）
    // 返回：按zOrder排序的页面元素列表
    virtual QList<PageElementPtr> generateElements(int pageIndex, const QRectF& pageRect);

    // 获取页面类型（0=单图排版, 1=多图排版），用于"应用到同类页"功能
    // 默认实现返回0，子类应该重写
    virtual int pageType(int pageIndex) const;

    // 判断布局是否需要重新计算
    bool isDirty() const { return m_dirty; }

protected:
    BookList m_books;     // 书籍列表
    bool m_dirty = true;  // 布局脏标记
};

// 排版引擎智能指针类型别名
using LayoutEnginePtr = QSharedPointer<LayoutEngine>;

#endif // LAYOUTENGINE_H
