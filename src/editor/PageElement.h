#ifndef PAGEELEMENT_H
#define PAGEELEMENT_H

#include <QSharedData>
#include <QSharedDataPointer>
#include <QString>
#include <QRectF>
#include <QPointF>
#include <QSizeF>
#include <QFont>
#include <QColor>
#include <QList>
#include <QJsonObject>
#include <QJsonArray>
#include <QPainterPath>
#include <Qt>

// ============================================================
// 页面元素基类（隐式共享数据）
// 继承QSharedData以支持QSharedDataPointer的引用计数隐式共享。
//
// 设计说明：
// PageElementData是抽象类（含纯虚clone()），这意味着
// QSharedDataPointer<PageElementData>只能通过const方法访问
// （constData/const operator->/const operator*）。
// 非const访问会触发detach()，而抽象类无法实例化，
// 因此会在编译期报错——这是一种安全机制，防止对象切片。
//
// 修改元素的推荐模式：
//   1. 通过constData()只读访问元素属性
//   2. 需要修改时，调用clone()创建深拷贝
//   3. 修改深拷贝（通过派生类指针）
//   4. 用新元素替换原列表中的元素
// ============================================================
class PageElementData : public QSharedData
{
public:
    // 元素类型枚举
    enum ElementType { Text, Image, Shape };

    PageElementData();
    PageElementData(const PageElementData& other);
    virtual ~PageElementData();

    // ---- 通用属性 ----
    ElementType elementType() const;

    QString id() const;               // 唯一标识符（UUID）
    void setId(const QString& id);    // 设置ID（用于clone后重新生成）

    QString name() const;             // 显示名称（用于图层面板）
    void setName(const QString& name);

    QRectF rect() const;              // 位置和尺寸
    void setRect(const QRectF& rect);
    QPointF pos() const;              // 左上角位置
    void setPos(const QPointF& pos);
    QSizeF size() const;              // 尺寸
    void setSize(const QSizeF& size);

    qreal rotation() const;           // 旋转角度（度）
    void setRotation(qreal angle);

    bool isVisible() const;
    void setVisible(bool visible);
    bool isLocked() const;
    void setLocked(bool locked);

    int zOrder() const;               // 层级顺序（值越大越靠上）
    void setZOrder(int z);

    // ---- JSON序列化 ----
    virtual QJsonObject toJson() const;
    virtual void fromJson(const QJsonObject& json);

    // ---- 克隆（深拷贝） ----
    // 返回new创建的对象，调用者负责管理（或交给QSharedDataPointer）
    // 用于"应用到所有同类页"等功能
    virtual PageElementData* clone() const = 0;

protected:
    ElementType m_type;     // 元素类型（由派生类构造函数设置）
    QString m_id;           // 唯一标识符（UUID）
    QString m_name;         // 显示名称
    QRectF m_rect;          // 位置和尺寸（点单位，72DPI）
    qreal m_rotation;       // 旋转角度（度）
    bool m_visible;         // 是否可见
    bool m_locked;          // 是否锁定
    int m_zOrder;           // 层级顺序
};

// ============================================================
// 文本元素
// 支持纯文本和字符级富文本格式（选区格式）
// ============================================================
class TextElementData : public PageElementData
{
public:
    // 字符级格式（用于富文本选区格式）
    struct CharFormat {
        int start;         // 起始字符位置
        int length;        // 长度
        QFont font;        // 字体（含粗体/斜体/字号）
        QColor color;      // 颜色
    };

    TextElementData();
    TextElementData(const TextElementData& other);

    // 文本内容（支持\n多段）
    QString text() const;
    void setText(const QString& text);

    // 字体格式（整体默认字体）
    QFont font() const;
    void setFont(const QFont& font);

    // 文字颜色
    QColor textColor() const;
    void setTextColor(const QColor& color);

    // 对齐方式
    Qt::Alignment alignment() const;
    void setAlignment(Qt::Alignment align);

    // 行高（0=自动，>0=固定像素值）
    qreal lineHeight() const;
    void setLineHeight(qreal height);

    // 富文本字符级格式（选中文本的混合格式）
    QList<CharFormat> charFormats() const;
    void setCharFormats(const QList<CharFormat>& formats);
    void clearCharFormats();

    QJsonObject toJson() const override;
    void fromJson(const QJsonObject& json) override;
    PageElementData* clone() const override;

private:
    QString m_text;                   // 文本内容
    QFont m_font;                     // 默认字体
    QColor m_textColor;               // 默认文字颜色
    Qt::Alignment m_alignment;        // 对齐方式
    qreal m_lineHeight;               // 行高
    QList<CharFormat> m_charFormats;  // 字符级格式列表
};

// ============================================================
// 图片元素
// ============================================================
class ImageElementData : public PageElementData
{
public:
    ImageElementData();
    ImageElementData(const ImageElementData& other);

    QString imagePath() const;
    void setImagePath(const QString& path);

    qreal scaleFactor() const;         // 缩放比例（1.0=原始大小）
    void setScaleFactor(qreal scale);

    qreal opacity() const;             // 不透明度（0.0-1.0）
    void setOpacity(qreal opacity);

    bool keepAspectRatio() const;      // 是否保持宽高比
    void setKeepAspectRatio(bool keep);

    QJsonObject toJson() const override;
    void fromJson(const QJsonObject& json) override;
    PageElementData* clone() const override;

private:
    QString m_imagePath;        // 图片文件路径
    qreal m_scaleFactor;        // 缩放比例
    qreal m_opacity;            // 不透明度
    bool m_keepAspectRatio;     // 是否保持宽高比
};

// ============================================================
// 形状元素
// 支持矩形、圆角矩形、直线、椭圆
// 支持纯色填充、渐变填充、边框
// ============================================================
class ShapeElementData : public PageElementData
{
public:
    // 形状类型枚举
    enum ShapeType { Rectangle, RoundedRect, Line, Ellipse, Path };

    ShapeElementData();
    ShapeElementData(const ShapeElementData& other);

    ShapeType shapeType() const;
    void setShapeType(ShapeType type);

    // 填充（QColor无效=无填充）
    QColor fillColor() const;
    void setFillColor(const QColor& color);
    bool hasFill() const;
    void setHasFill(bool has);

    // 渐变填充（从fillColor到gradientColor的线性渐变）
    QColor gradientColor() const;       // 渐变结束色
    void setGradientColor(const QColor& color);
    bool hasGradient() const;
    void setHasGradient(bool has);

    // 边框
    QColor borderColor() const;
    void setBorderColor(const QColor& color);
    qreal borderWidth() const;
    void setBorderWidth(qreal width);
    bool hasBorder() const;
    void setHasBorder(bool has);

    // 圆角半径（仅RoundedRect有效）
    qreal cornerRadius() const;
    void setCornerRadius(qreal radius);

    // 自定义路径（仅Path类型有效，编辑器内部使用，不序列化）
    QPainterPath painterPath() const;
    void setPainterPath(const QPainterPath& path);

    QJsonObject toJson() const override;
    void fromJson(const QJsonObject& json) override;
    PageElementData* clone() const override;

private:
    ShapeType m_shapeType;       // 形状类型
    QColor m_fillColor;          // 填充颜色
    bool m_hasFill;              // 是否填充
    QColor m_gradientColor;      // 渐变结束色
    bool m_hasGradient;          // 是否渐变填充
    QColor m_borderColor;        // 边框颜色
    qreal m_borderWidth;         // 边框宽度
    bool m_hasBorder;            // 是否有边框
    qreal m_cornerRadius;        // 圆角半径
    QPainterPath m_path;         // 自定义路径
};

// ============================================================
// 智能指针类型别名
// ============================================================

// PageElementPtr用于在列表中存储多态元素（只读访问）。
// 修改时通过clone()获取可修改副本。
using PageElementPtr = QSharedDataPointer<PageElementData>;

// 以下类型别名用于创建和修改具体类型的元素（支持读写）。
// 创建元素示例：
//   TextElementPtr textElem(new TextElementData());
//   textElem->setText("Hello");  // 可修改，TextElementData非抽象
using TextElementPtr = QSharedDataPointer<TextElementData>;
using ImageElementPtr = QSharedDataPointer<ImageElementData>;
using ShapeElementPtr = QSharedDataPointer<ShapeElementData>;

#endif // PAGEELEMENT_H
