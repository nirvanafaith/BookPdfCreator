#include "PageElement.h"

#include <QUuid>

// ============================================================
// 辅助函数（匿名命名空间，仅本文件可见）
// ============================================================
namespace {

// 将QColor序列化为字符串（#AARRGGBB格式，含alpha通道）
QString colorToString(const QColor& color)
{
    if (!color.isValid()) {
        return QString();
    }
    return color.name(QColor::HexArgb);
}

// 从字符串反序列化QColor
// 支持#RRGGBB和#AARRGGBB格式
// 解析失败时返回defaultColor
QColor stringToColor(const QString& str, const QColor& defaultColor)
{
    if (str.isEmpty()) {
        return defaultColor;
    }
    QColor color;
    color.setNamedColor(str);
    if (!color.isValid()) {
        return defaultColor;
    }
    return color;
}

// 将QFont序列化为字符串
QString fontToString(const QFont& font)
{
    return font.toString();
}

// 从字符串反序列化QFont
// 解析失败时返回defaultFont
QFont stringToFont(const QString& str, const QFont& defaultFont)
{
    if (str.isEmpty()) {
        return defaultFont;
    }
    QFont font;
    if (font.fromString(str)) {
        return font;
    }
    return defaultFont;
}

} // namespace

// ============================================================
// PageElementData 基类实现
// ============================================================

PageElementData::PageElementData()
    : QSharedData()
    , m_type(Text)
    , m_id(QUuid::createUuid().toString())
    , m_name()
    , m_rect(0.0, 0.0, 100.0, 50.0)
    , m_rotation(0.0)
    , m_visible(true)
    , m_locked(false)
    , m_zOrder(0)
{
}

PageElementData::PageElementData(const PageElementData& other)
    : QSharedData(other)
    , m_type(other.m_type)
    , m_id(other.m_id)
    , m_name(other.m_name)
    , m_rect(other.m_rect)
    , m_rotation(other.m_rotation)
    , m_visible(other.m_visible)
    , m_locked(other.m_locked)
    , m_zOrder(other.m_zOrder)
{
}

PageElementData::~PageElementData()
{
}

PageElementData::ElementType PageElementData::elementType() const
{
    return m_type;
}

QString PageElementData::id() const
{
    return m_id;
}

void PageElementData::setId(const QString& id)
{
    m_id = id;
}

QString PageElementData::name() const
{
    return m_name;
}

void PageElementData::setName(const QString& name)
{
    m_name = name;
}

QRectF PageElementData::rect() const
{
    return m_rect;
}

void PageElementData::setRect(const QRectF& rect)
{
    m_rect = rect;
}

QPointF PageElementData::pos() const
{
    return m_rect.topLeft();
}

void PageElementData::setPos(const QPointF& pos)
{
    m_rect.moveTopLeft(pos);
}

QSizeF PageElementData::size() const
{
    return m_rect.size();
}

void PageElementData::setSize(const QSizeF& size)
{
    m_rect.setSize(size);
}

qreal PageElementData::rotation() const
{
    return m_rotation;
}

void PageElementData::setRotation(qreal angle)
{
    m_rotation = angle;
}

bool PageElementData::isVisible() const
{
    return m_visible;
}

void PageElementData::setVisible(bool visible)
{
    m_visible = visible;
}

bool PageElementData::isLocked() const
{
    return m_locked;
}

void PageElementData::setLocked(bool locked)
{
    m_locked = locked;
}

int PageElementData::zOrder() const
{
    return m_zOrder;
}

void PageElementData::setZOrder(int z)
{
    m_zOrder = z;
}

QJsonObject PageElementData::toJson() const
{
    QJsonObject json;

    // 元素类型字符串
    QString typeStr;
    switch (m_type) {
    case Text:  typeStr = QStringLiteral("text");  break;
    case Image: typeStr = QStringLiteral("image"); break;
    case Shape: typeStr = QStringLiteral("shape"); break;
    }
    json["type"] = typeStr;

    // 通用属性
    json["id"] = m_id;
    json["name"] = m_name;

    // 位置和尺寸
    QJsonObject rectJson;
    rectJson["x"] = m_rect.x();
    rectJson["y"] = m_rect.y();
    rectJson["width"] = m_rect.width();
    rectJson["height"] = m_rect.height();
    json["rect"] = rectJson;

    json["rotation"] = m_rotation;
    json["visible"] = m_visible;
    json["locked"] = m_locked;
    json["zOrder"] = m_zOrder;

    return json;
}

void PageElementData::fromJson(const QJsonObject& json)
{
    // 注意：m_type由派生类构造函数设置，不从JSON读取
    // （type字段仅用于PageData::fromJson判断创建哪种元素）

    m_id = json.value("id").toString(m_id);
    m_name = json.value("name").toString();

    // 位置和尺寸
    QJsonObject rectJson = json.value("rect").toObject();
    m_rect = QRectF(
        rectJson.value("x").toDouble(0.0),
        rectJson.value("y").toDouble(0.0),
        rectJson.value("width").toDouble(100.0),
        rectJson.value("height").toDouble(50.0)
    );

    m_rotation = json.value("rotation").toDouble(0.0);
    m_visible = json.value("visible").toBool(true);
    m_locked = json.value("locked").toBool(false);
    m_zOrder = json.value("zOrder").toInt(0);
}

// ============================================================
// TextElementData 文本元素实现
// ============================================================

TextElementData::TextElementData()
    : PageElementData()
    , m_text()
    , m_font()
    , m_textColor(0, 0, 0)                           // 默认黑色
    , m_alignment(Qt::AlignLeft | Qt::AlignTop)       // 默认左上对齐
    , m_lineHeight(0.0)                               // 0=自动行高
    , m_letterSpacing(0.0)                            // 0=默认字间距
{
    m_type = Text;
}

TextElementData::TextElementData(const TextElementData& other)
    : PageElementData(other)
    , m_text(other.m_text)
    , m_font(other.m_font)
    , m_textColor(other.m_textColor)
    , m_alignment(other.m_alignment)
    , m_lineHeight(other.m_lineHeight)
    , m_letterSpacing(other.m_letterSpacing)
    , m_charFormats(other.m_charFormats)
{
}

QString TextElementData::text() const
{
    return m_text;
}

void TextElementData::setText(const QString& text)
{
    m_text = text;
}

QFont TextElementData::font() const
{
    return m_font;
}

void TextElementData::setFont(const QFont& font)
{
    m_font = font;
}

QColor TextElementData::textColor() const
{
    return m_textColor;
}

void TextElementData::setTextColor(const QColor& color)
{
    m_textColor = color;
}

Qt::Alignment TextElementData::alignment() const
{
    return m_alignment;
}

void TextElementData::setAlignment(Qt::Alignment align)
{
    m_alignment = align;
}

qreal TextElementData::lineHeight() const
{
    return m_lineHeight;
}

void TextElementData::setLineHeight(qreal height)
{
    m_lineHeight = height;
}

qreal TextElementData::letterSpacing() const
{
    return m_letterSpacing;
}

void TextElementData::setLetterSpacing(qreal spacing)
{
    m_letterSpacing = spacing;
}

QList<TextElementData::CharFormat> TextElementData::charFormats() const
{
    return m_charFormats;
}

void TextElementData::setCharFormats(const QList<CharFormat>& formats)
{
    m_charFormats = formats;
}

void TextElementData::clearCharFormats()
{
    m_charFormats.clear();
}

QJsonObject TextElementData::toJson() const
{
    QJsonObject json = PageElementData::toJson();  // 基类字段

    // 文本特有属性
    json["text"] = m_text;
    json["font"] = fontToString(m_font);
    json["textColor"] = colorToString(m_textColor);
    json["alignment"] = int(m_alignment);
    json["lineHeight"] = m_lineHeight;
    json["letterSpacing"] = m_letterSpacing;

    // 字符级格式列表
    QJsonArray formatsArray;
    for (const CharFormat& fmt : m_charFormats) {
        QJsonObject fmtJson;
        fmtJson["start"] = fmt.start;
        fmtJson["length"] = fmt.length;
        fmtJson["font"] = fontToString(fmt.font);
        fmtJson["color"] = colorToString(fmt.color);
        formatsArray.append(fmtJson);
    }
    json["charFormats"] = formatsArray;

    return json;
}

void TextElementData::fromJson(const QJsonObject& json)
{
    PageElementData::fromJson(json);  // 基类字段

    m_text = json.value("text").toString();
    m_font = stringToFont(json.value("font").toString(), QFont());
    m_textColor = stringToColor(json.value("textColor").toString(), QColor(0, 0, 0));
    m_alignment = Qt::Alignment(json.value("alignment").toInt(
        int(Qt::AlignLeft | Qt::AlignTop)));
    m_lineHeight = json.value("lineHeight").toDouble(0.0);
    m_letterSpacing = json.value("letterSpacing").toDouble(0.0);

    // 字符级格式列表
    m_charFormats.clear();
    QJsonArray formatsArray = json.value("charFormats").toArray();
    for (const QJsonValue& val : formatsArray) {
        QJsonObject fmtJson = val.toObject();
        CharFormat fmt;
        fmt.start = fmtJson.value("start").toInt(0);
        fmt.length = fmtJson.value("length").toInt(0);
        fmt.font = stringToFont(fmtJson.value("font").toString(), QFont());
        fmt.color = stringToColor(fmtJson.value("color").toString(), QColor(0, 0, 0));
        m_charFormats.append(fmt);
    }
}

PageElementData* TextElementData::clone() const
{
    return new TextElementData(*this);
}

// ============================================================
// ImageElementData 图片元素实现
// ============================================================

ImageElementData::ImageElementData()
    : PageElementData()
    , m_imagePath()
    , m_scaleFactor(1.0)        // 原始大小
    , m_opacity(1.0)            // 完全不透明
    , m_keepAspectRatio(true)   // 默认保持宽高比
{
    m_type = Image;
}

ImageElementData::ImageElementData(const ImageElementData& other)
    : PageElementData(other)
    , m_imagePath(other.m_imagePath)
    , m_scaleFactor(other.m_scaleFactor)
    , m_opacity(other.m_opacity)
    , m_keepAspectRatio(other.m_keepAspectRatio)
{
}

QString ImageElementData::imagePath() const
{
    return m_imagePath;
}

void ImageElementData::setImagePath(const QString& path)
{
    m_imagePath = path;
}

qreal ImageElementData::scaleFactor() const
{
    return m_scaleFactor;
}

void ImageElementData::setScaleFactor(qreal scale)
{
    m_scaleFactor = scale;
}

qreal ImageElementData::opacity() const
{
    return m_opacity;
}

void ImageElementData::setOpacity(qreal opacity)
{
    m_opacity = opacity;
}

bool ImageElementData::keepAspectRatio() const
{
    return m_keepAspectRatio;
}

void ImageElementData::setKeepAspectRatio(bool keep)
{
    m_keepAspectRatio = keep;
}

QJsonObject ImageElementData::toJson() const
{
    QJsonObject json = PageElementData::toJson();  // 基类字段

    json["imagePath"] = m_imagePath;
    json["scaleFactor"] = m_scaleFactor;
    json["opacity"] = m_opacity;
    json["keepAspectRatio"] = m_keepAspectRatio;

    return json;
}

void ImageElementData::fromJson(const QJsonObject& json)
{
    PageElementData::fromJson(json);  // 基类字段

    m_imagePath = json.value("imagePath").toString();
    m_scaleFactor = json.value("scaleFactor").toDouble(1.0);
    m_opacity = json.value("opacity").toDouble(1.0);
    m_keepAspectRatio = json.value("keepAspectRatio").toBool(true);
}

PageElementData* ImageElementData::clone() const
{
    return new ImageElementData(*this);
}

// ============================================================
// ShapeElementData 形状元素实现
// ============================================================

ShapeElementData::ShapeElementData()
    : PageElementData()
    , m_shapeType(Rectangle)
    , m_fillColor(255, 255, 255)           // 默认白色填充
    , m_hasFill(true)
    , m_gradientColor(25, 118, 210)        // 默认渐变结束色（蓝色）
    , m_hasGradient(false)
    , m_borderColor(0, 0, 0)              // 默认黑色边框
    , m_borderWidth(1.0)
    , m_hasBorder(true)
    , m_cornerRadius(5.0)                  // 默认圆角半径
    , m_path()
{
    m_type = Shape;
}

ShapeElementData::ShapeElementData(const ShapeElementData& other)
    : PageElementData(other)
    , m_shapeType(other.m_shapeType)
    , m_fillColor(other.m_fillColor)
    , m_hasFill(other.m_hasFill)
    , m_gradientColor(other.m_gradientColor)
    , m_hasGradient(other.m_hasGradient)
    , m_borderColor(other.m_borderColor)
    , m_borderWidth(other.m_borderWidth)
    , m_hasBorder(other.m_hasBorder)
    , m_cornerRadius(other.m_cornerRadius)
    , m_path(other.m_path)
{
}

ShapeElementData::ShapeType ShapeElementData::shapeType() const
{
    return m_shapeType;
}

void ShapeElementData::setShapeType(ShapeType type)
{
    m_shapeType = type;
}

QColor ShapeElementData::fillColor() const
{
    return m_fillColor;
}

void ShapeElementData::setFillColor(const QColor& color)
{
    m_fillColor = color;
}

bool ShapeElementData::hasFill() const
{
    return m_hasFill;
}

void ShapeElementData::setHasFill(bool has)
{
    m_hasFill = has;
}

QColor ShapeElementData::gradientColor() const
{
    return m_gradientColor;
}

void ShapeElementData::setGradientColor(const QColor& color)
{
    m_gradientColor = color;
}

bool ShapeElementData::hasGradient() const
{
    return m_hasGradient;
}

void ShapeElementData::setHasGradient(bool has)
{
    m_hasGradient = has;
}

QColor ShapeElementData::borderColor() const
{
    return m_borderColor;
}

void ShapeElementData::setBorderColor(const QColor& color)
{
    m_borderColor = color;
}

qreal ShapeElementData::borderWidth() const
{
    return m_borderWidth;
}

void ShapeElementData::setBorderWidth(qreal width)
{
    m_borderWidth = width;
}

bool ShapeElementData::hasBorder() const
{
    return m_hasBorder;
}

void ShapeElementData::setHasBorder(bool has)
{
    m_hasBorder = has;
}

qreal ShapeElementData::cornerRadius() const
{
    return m_cornerRadius;
}

void ShapeElementData::setCornerRadius(qreal radius)
{
    m_cornerRadius = radius;
}

QPainterPath ShapeElementData::painterPath() const
{
    return m_path;
}

void ShapeElementData::setPainterPath(const QPainterPath& path)
{
    m_path = path;
}

QJsonObject ShapeElementData::toJson() const
{
    QJsonObject json = PageElementData::toJson();  // 基类字段

    json["shapeType"] = int(m_shapeType);
    json["fillColor"] = colorToString(m_fillColor);
    json["hasFill"] = m_hasFill;
    json["gradientColor"] = colorToString(m_gradientColor);
    json["hasGradient"] = m_hasGradient;
    json["borderColor"] = colorToString(m_borderColor);
    json["borderWidth"] = m_borderWidth;
    json["hasBorder"] = m_hasBorder;
    json["cornerRadius"] = m_cornerRadius;

    return json;
}

void ShapeElementData::fromJson(const QJsonObject& json)
{
    PageElementData::fromJson(json);  // 基类字段

    m_shapeType = static_cast<ShapeType>(
        json.value("shapeType").toInt(int(Rectangle)));
    m_fillColor = stringToColor(
        json.value("fillColor").toString(), QColor(255, 255, 255));
    m_hasFill = json.value("hasFill").toBool(true);
    m_gradientColor = stringToColor(
        json.value("gradientColor").toString(), QColor(25, 118, 210));
    m_hasGradient = json.value("hasGradient").toBool(false);
    m_borderColor = stringToColor(
        json.value("borderColor").toString(), QColor(0, 0, 0));
    m_borderWidth = json.value("borderWidth").toDouble(1.0);
    m_hasBorder = json.value("hasBorder").toBool(true);
    m_cornerRadius = json.value("cornerRadius").toDouble(5.0);
}

PageElementData* ShapeElementData::clone() const
{
    return new ShapeElementData(*this);
}
