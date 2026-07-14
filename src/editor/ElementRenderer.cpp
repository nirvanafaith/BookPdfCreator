#include "ElementRenderer.h"
#include "ImageCache.h"
#include "PainterHelpers.h"
#include "layout/LayoutConstants.h"

#include <QTextLayout>
#include <QTextLine>
#include <QFontMetrics>
#include <QLinearGradient>
#include <QBrush>
#include <QPen>
#include <QtGlobal>

// ============================================================
// renderElement - 渲染单个元素（入口）
//
// 处理通用逻辑：可见性检查、旋转变换、类型分派。
// 旋转绕元素中心点进行，使用 translate-rotate-translate 模式。
// ============================================================
void ElementRenderer::renderElement(QPainter* painter, const PageElementPtr& element)
{
    if (!painter) return;

    // 安全获取元素指针（PageElementData为抽象类，只能const访问）
    const PageElementData* data = element.constData();
    if (!data || !data->isVisible()) return;

    painter->save();

    // 应用旋转（绕元素中心旋转）
    qreal rotation = data->rotation();
    if (qAbs(rotation) > 0.001) {
        QPointF center = data->rect().center();
        painter->translate(center);
        painter->rotate(rotation);
        painter->translate(-center);
    }

    // 根据元素类型分派到对应的渲染方法
    switch (data->elementType()) {
    case PageElementData::Text:
        renderText(painter, static_cast<const TextElementData*>(data));
        break;
    case PageElementData::Image:
        renderImage(painter, static_cast<const ImageElementData*>(data));
        break;
    case PageElementData::Shape:
        renderShape(painter, static_cast<const ShapeElementData*>(data));
        break;
    }

    painter->restore();
}

// ============================================================
// renderPage - 渲染整个页面
//
// 遍历页面中的所有元素（PageData已按zOrder排序列表），
// 依次调用renderElement进行渲染。
// 不可见元素由renderElement内部跳过。
// ============================================================
void ElementRenderer::renderPage(QPainter* painter, const PageDataPtr& pageData)
{
    if (!painter || !pageData) return;

    // 设置裁剪区域为纸张边界，纸张外元素不渲染
    qreal pageW = PageSizeManager::instance().pageWidth();
    qreal pageH = PageSizeManager::instance().pageHeight();
    painter->save();
    painter->setClipRect(QRectF(0, 0, pageW, pageH));

    // 元素列表已按zOrder排序
    QList<PageElementPtr> elements = pageData->elements();
    for (const PageElementPtr& element : elements) {
        renderElement(painter, element);
    }

    painter->restore();
}

// ============================================================
// renderText - 文本渲染
//
// 使用QTextLayout处理多行文本换行和富文本格式。
// 支持水平对齐（左/中/右）和垂直对齐（上/中/下）。
// 行高：0=自动用QFontMetrics::height()，>0=固定值。
// CharFormat列表为空时用统一font/color渲染；
// 非空时通过setAdditionalFormats应用混合格式。
// ============================================================
void ElementRenderer::renderText(QPainter* painter, const TextElementData* text)
{
    if (!painter || !text || text->text().isEmpty()) return;

    QRectF rect = text->rect();
    QFont font = text->font();
    QColor color = text->textColor();
    Qt::Alignment align = text->alignment();

    // 行高计算：0=自动，>0=固定值
    QFontMetrics fm(font);
    qreal lineHeight = text->lineHeight();
    if (lineHeight <= 0) {
        lineHeight = fm.height();
    }

    painter->save();
    painter->setFont(font);
    painter->setPen(color);

    // 创建文本布局（QTextLayout自动处理\n多段和自动换行）
    QTextLayout textLayout(text->text(), font, painter->device());

    // 应用字符级格式（CharFormat：混合粗体/斜体/字号/颜色）
    QList<TextElementData::CharFormat> charFormats = text->charFormats();
    if (!charFormats.isEmpty()) {
        QList<QTextLayout::FormatRange> formats;
        for (const TextElementData::CharFormat& cf : charFormats) {
            QTextLayout::FormatRange range;
            range.start = cf.start;
            range.length = cf.length;
            range.format.setFont(cf.font);
            range.format.setForeground(QBrush(cf.color));
            formats.append(range);
        }
        textLayout.setAdditionalFormats(formats);
    }

    // 布局文本行（位置相对于布局原点）
    textLayout.beginLayout();
    qreal y = 0.0;

    while (true) {
        QTextLine line = textLayout.createLine();
        if (!line.isValid()) break;

        line.setLineWidth(rect.width());

        // 水平对齐：计算每行的X偏移
        qreal x = 0.0;
        if (align & Qt::AlignHCenter) {
            x = (rect.width() - line.naturalTextWidth()) / 2.0;
        } else if (align & Qt::AlignRight) {
            x = rect.width() - line.naturalTextWidth();
        }

        line.setPosition(QPointF(x, y));
        y += lineHeight;
    }
    textLayout.endLayout();

    // 垂直对齐：计算Y偏移
    qreal totalHeight = y;
    qreal vOffset = 0.0;
    if (align & Qt::AlignVCenter) {
        vOffset = (rect.height() - totalHeight) / 2.0;
        if (vOffset < 0) vOffset = 0.0;
    } else if (align & Qt::AlignBottom) {
        vOffset = rect.height() - totalHeight;
        if (vOffset < 0) vOffset = 0.0;
    }

    // 绘制文本（偏移到元素rect位置）
    textLayout.draw(painter, QPointF(rect.x(), rect.y() + vOffset));

    painter->restore();
}

// ============================================================
// renderImage - 图片渲染
//
// 通过ImageCache获取图片（带缓存），处理不透明度、宽高比、缩放。
// 图片加载失败时绘制占位符（灰色矩形+文字提示）。
//
// 缩放策略：
// - scaleFactor != 1.0：使用原始尺寸 * scaleFactor（自然保持宽高比）
// - scaleFactor == 1.0：适配到rect尺寸，keepAspectRatio控制是否保持宽高比
// ============================================================
void ElementRenderer::renderImage(QPainter* painter, const ImageElementData* image)
{
    if (!painter || !image) return;

    QRectF rect = image->rect();
    QString path = image->imagePath();

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setRenderHint(QPainter::SmoothPixmapTransform, true);

    // 通过ImageCache获取图片（自动缓存，传入目标尺寸优化内存）
    QPixmap pixmap;
    if (!path.isEmpty()) {
        pixmap = ImageCache::instance().getPixmap(path, rect.size().toSize());
    }

    if (pixmap.isNull()) {
        // 图片加载失败，绘制占位符（复用PainterHelpers保持视觉一致）
        PainterHelpers::drawPlaceholder(painter, rect,
                                        QString::fromUtf8("图片缺失"));
    } else {
        // 设置不透明度
        painter->setOpacity(image->opacity());

        // 计算目标绘制区域
        QRectF targetRect;
        qreal scaleFactor = image->scaleFactor();

        if (qAbs(scaleFactor - 1.0) > 0.001) {
            // 使用scaleFactor缩放原始图片尺寸
            QSizeF scaledSize(pixmap.width() * scaleFactor,
                              pixmap.height() * scaleFactor);
            targetRect.setSize(scaledSize);
        } else {
            // 适配到元素rect尺寸
            if (image->keepAspectRatio()) {
                // 保持宽高比：缩放到rect内并居中
                QSizeF aspectSize = pixmap.size();
                aspectSize.scale(rect.size(), Qt::KeepAspectRatio);
                targetRect.setSize(aspectSize);
            } else {
                // 拉伸填充整个rect
                targetRect.setSize(rect.size());
            }
        }

        // 在元素rect内居中
        targetRect.moveCenter(rect.center());

        // 绘制图片
        painter->drawPixmap(targetRect, pixmap,
                            QRectF(0, 0, pixmap.width(), pixmap.height()));
    }

    painter->restore();
}

// ============================================================
// renderShape - 形状渲染
//
// 支持四种形状：矩形、圆角矩形、直线、椭圆。
// 填充：纯色或线性渐变（从fillColor到gradientColor，上到下）。
// 边框：QPen(borderColor, borderWidth)。
// 圆角半径仅RoundedRect有效。
//
// 注意：Line形状的rect定义 bounding box，从左上角画到右下角。
// 水平线设height=0，垂直线设width=0，对角线两者均非零。
// ============================================================
void ElementRenderer::renderShape(QPainter* painter, const ShapeElementData* shape)
{
    if (!painter || !shape) return;

    QRectF rect = shape->rect();

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);

    // 设置填充画刷
    if (shape->hasFill()) {
        if (shape->hasGradient()) {
            // 线性渐变填充（从上到下：fillColor -> gradientColor）
            QLinearGradient gradient(rect.topLeft(), rect.bottomLeft());
            gradient.setColorAt(0.0, shape->fillColor());
            gradient.setColorAt(1.0, shape->gradientColor());
            painter->setBrush(QBrush(gradient));
        } else {
            // 纯色填充
            painter->setBrush(QBrush(shape->fillColor()));
        }
    } else {
        painter->setBrush(Qt::NoBrush);
    }

    // 设置边框画笔
    if (shape->hasBorder() && shape->borderWidth() > 0) {
        painter->setPen(QPen(shape->borderColor(), shape->borderWidth()));
    } else {
        painter->setPen(Qt::NoPen);
    }

    // 根据形状类型绘制
    switch (shape->shapeType()) {
    case ShapeElementData::Rectangle:
        painter->drawRect(rect);
        break;
    case ShapeElementData::RoundedRect:
        painter->drawRoundedRect(rect, shape->cornerRadius(), shape->cornerRadius());
        break;
    case ShapeElementData::Line:
        // 直线：从rect左上角到右下角
        painter->drawLine(rect.topLeft(), rect.bottomRight());
        break;
    case ShapeElementData::Ellipse:
        painter->drawEllipse(rect);
        break;
    }

    painter->restore();
}
