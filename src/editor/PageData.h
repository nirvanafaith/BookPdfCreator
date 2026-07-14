#ifndef PAGEDATA_H
#define PAGEDATA_H

#include <QSharedPointer>
#include <QList>
#include <QJsonObject>
#include <QString>

#include "PageElement.h"

// ============================================================
// 页面数据类
//
// 管理一个PDF页面的所有元素。使用QSharedPointer管理PageData
// 的生命周期（PageData本身不继承QSharedData，因为它由
// QSharedPointer管理而非QSharedDataPointer）。
//
// 页面中的元素列表使用QList<PageElementPtr>存储，
// PageElementPtr = QSharedDataPointer<PageElementData>
// 提供隐式共享（写时复制）语义。
// ============================================================
class PageData
{
public:
    PageData();
    PageData(const PageData& other);
    ~PageData();

    // ---- 页面属性 ----
    int pageIndex() const;              // 页码索引（从0开始）
    void setPageIndex(int index);

    int pageType() const;               // 0=单图排版, 1=多图排版
    void setPageType(int type);

    // ---- 元素管理 ----
    // 获取元素列表（按zOrder排序）
    QList<PageElementPtr> elements() const;
    void setElements(const QList<PageElementPtr>& elements);

    // 添加元素（按zOrder插入到合适位置，保持列表有序）
    void addElement(PageElementPtr element);

    // 移除指定ID的元素
    void removeElement(const QString& id);

    // 按ID查找元素（未找到返回空的PageElementPtr）
    PageElementPtr elementById(const QString& id) const;

    // ---- JSON序列化 ----
    QJsonObject toJson() const;
    void fromJson(const QJsonObject& json);

private:
    int m_pageIndex;                       // 页码索引
    int m_pageType;                        // 页面类型（0=单图, 1=多图）
    QList<PageElementPtr> m_elements;      // 元素列表（按zOrder排序）
};

// 页面数据智能指针类型别名
using PageDataPtr = QSharedPointer<PageData>;

#endif // PAGEDATA_H
