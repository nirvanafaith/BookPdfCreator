#include "PageData.h"

#include <QDebug>
#include <QJsonArray>
#include <QJsonValue>

// ============================================================
// PageData 页面数据实现
// ============================================================

PageData::PageData()
    : m_pageIndex(0)
    , m_pageType(0)
{
}

PageData::PageData(const PageData& other)
    : m_pageIndex(other.m_pageIndex)
    , m_pageType(other.m_pageType)
    , m_elements(other.m_elements)  // QList隐式共享，元素引用计数+1
{
}

PageData::~PageData()
{
}

int PageData::pageIndex() const
{
    return m_pageIndex;
}

void PageData::setPageIndex(int index)
{
    m_pageIndex = index;
}

int PageData::pageType() const
{
    return m_pageType;
}

void PageData::setPageType(int type)
{
    m_pageType = type;
}

QList<PageElementPtr> PageData::elements() const
{
    return m_elements;  // QList隐式共享，返回浅拷贝
}

void PageData::setElements(const QList<PageElementPtr>& elements)
{
    m_elements = elements;
}

void PageData::addElement(PageElementPtr element)
{
    // 按zOrder升序插入，保持列表有序
    // 使用constData()进行只读访问，避免触发detach
    int z = element.constData()->zOrder();
    int i = 0;
    for (; i < m_elements.size(); ++i) {
        if (m_elements[i].constData()->zOrder() > z) {
            break;
        }
    }
    m_elements.insert(i, element);
}

void PageData::removeElement(const QString& id)
{
    // 使用constData()进行只读访问
    for (int i = 0; i < m_elements.size(); ++i) {
        if (m_elements[i].constData()->id() == id) {
            m_elements.removeAt(i);
            return;
        }
    }
}

PageElementPtr PageData::elementById(const QString& id) const
{
    // 使用constData()进行只读访问，避免触发detach
    for (const PageElementPtr& elem : m_elements) {
        if (elem.constData()->id() == id) {
            return elem;  // 返回共享副本（引用计数+1）
        }
    }
    return PageElementPtr();  // 未找到，返回空指针
}

QJsonObject PageData::toJson() const
{
    QJsonObject json;
    json["pageIndex"] = m_pageIndex;
    json["pageType"] = m_pageType;

    // 序列化元素列表
    QJsonArray elemArray;
    for (const PageElementPtr& elem : m_elements) {
        // 使用constData()进行只读访问，调用虚函数toJson()
        elemArray.append(elem.constData()->toJson());
    }
    json["elements"] = elemArray;

    return json;
}

void PageData::fromJson(const QJsonObject& json)
{
    m_pageIndex = json.value("pageIndex").toInt(0);
    m_pageType = json.value("pageType").toInt(0);

    m_elements.clear();

    // 反序列化元素列表（根据type字段创建对应类型的元素）
    QJsonArray elemArray = json.value("elements").toArray();
    for (const QJsonValue& val : elemArray) {
        QJsonObject elemJson = val.toObject();
        QString typeStr = elemJson.value("type").toString();

        // 根据类型字符串创建对应的元素对象
        PageElementData* elem = nullptr;
        if (typeStr == QStringLiteral("text")) {
            elem = new TextElementData();
        } else if (typeStr == QStringLiteral("image")) {
            elem = new ImageElementData();
        } else if (typeStr == QStringLiteral("shape")) {
            elem = new ShapeElementData();
        } else {
            qWarning() << "PageData::fromJson: 未知元素类型:" << typeStr;
            continue;
        }

        // 从JSON填充元素属性
        elem->fromJson(elemJson);

        // 添加到列表（PageElementPtr接管所有权，并按zOrder排序插入）
        addElement(PageElementPtr(elem));
    }
}
