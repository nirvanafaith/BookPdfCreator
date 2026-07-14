#include "PainterHelpers.h"
#include <QTextLayout>
#include <QLinearGradient>
#include <QPainterPath>
#include <QFontMetrics>

void PainterHelpers::drawRoundedRect(QPainter* p, const QRectF& rect, const QColor& fillColor,
                                     qreal radius, const QColor& borderColor, int borderWidth)
{
    if (!p) return;

    p->save();
    p->setRenderHint(QPainter::Antialiasing, true);

    QPainterPath path;
    path.addRoundedRect(rect, radius, radius);

    if (fillColor.isValid()) {
        p->fillPath(path, fillColor);
    }

    if (borderColor.isValid() && borderWidth > 0) {
        QPen pen(borderColor, borderWidth);
        p->setPen(pen);
        p->drawPath(path);
    }

    p->restore();
}

void PainterHelpers::drawGradientBanner(QPainter* p, const QRectF& rect, const QColor& color,
                                        const QString& title, const QColor& textColor)
{
    if (!p) return;

    p->save();
    p->setRenderHint(QPainter::Antialiasing, true);

    // 绘制渐变背景（从上到下，主色到稍亮的渐变
    QLinearGradient gradient(rect.topLeft(), rect.bottomLeft());
    gradient.setColorAt(0.0, color);
    gradient.setColorAt(1.0, color.lighter(110));

    p->setBrush(gradient);
    p->setPen(Qt::NoPen);
    p->drawRoundedRect(rect, CORNER_RADIUS, CORNER_RADIUS);

    // 绘制居中标题文字
    if (!title.isEmpty()) {
        QFont font = p->font();
        font.setPixelSize(FONT_TITLE_SIZE);
        font.setBold(true);
        p->setFont(font);
        p->setPen(textColor);

        QFontMetrics fm(font);
        QRectF textRect = fm.boundingRect(title);
        qreal x = rect.x() + (rect.width() - textRect.width()) / 2.0;
        qreal y = rect.y() + (rect.height() - textRect.height()) / 2.0 + fm.ascent();
        p->drawText(QPointF(x, y), title);
    }

    p->restore();
}

void PainterHelpers::drawSectionTitle(QPainter* p, const QRectF& rect, const QString& title,
                                      const QColor& bgColor, const QColor& dotColor)
{
    if (!p) return;

    p->save();
    p->setRenderHint(QPainter::Antialiasing, true);

    // 绘制背景圆角矩形
    p->setBrush(bgColor);
    p->setPen(Qt::NoPen);
    p->drawRoundedRect(rect, CORNER_RADIUS, CORNER_RADIUS);

    // 绘制红色圆点
    qreal dotRadius = 4.0;
    qreal dotX = rect.x() + 10.0 + dotRadius;
    qreal dotY = rect.y() + rect.height() / 2.0;
    p->setBrush(dotColor);
    p->drawEllipse(QPointF(dotX, dotY), dotRadius, dotRadius);

    // 绘制标题文字
    if (!title.isEmpty()) {
        QFont font = p->font();
        font.setPixelSize(FONT_LABEL_SIZE);
        font.setBold(true);
        p->setFont(font);
        p->setPen(COLOR_TEXT_BLACK);

        qreal textX = dotX + dotRadius + 8.0;
        QFontMetrics fm(font);
        qreal textY = rect.y() + (rect.height() - fm.height()) / 2.0 + fm.ascent();
        p->drawText(QPointF(textX, textY), title);
    }

    p->restore();
}

void PainterHelpers::drawBookCover3D(QPainter* p, const QPixmap& cover, const QRectF& rect)
{
    if (!p) return;

    p->save();
    p->setRenderHint(QPainter::Antialiasing, true);
    p->setRenderHint(QPainter::SmoothPixmapTransform, true);

    qreal shadowOffsetX = 8.0;
    qreal shadowOffsetY = 6.0;
    qreal spineWidth = 5.0;

    QRectF coverRect = rect.adjusted(0, 0, -shadowOffsetX, -shadowOffsetY);

    {
        QRectF shadowRect = coverRect.adjusted(shadowOffsetX, shadowOffsetY, shadowOffsetX, shadowOffsetY);
        QLinearGradient shadowGradient(shadowRect.topLeft(), shadowRect.bottomRight());
        shadowGradient.setColorAt(0.0, QColor(0, 0, 0, 100));
        shadowGradient.setColorAt(1.0, QColor(0, 0, 0, 40));
        p->fillRect(shadowRect, shadowGradient);
    }

    {
        QRectF rightShadowRect(coverRect.right(), coverRect.y() + 2, spineWidth, coverRect.height() - 2);
        QLinearGradient rightSpineGradient(rightShadowRect.topLeft(), rightShadowRect.topRight());
        rightSpineGradient.setColorAt(0.0, QColor(0, 0, 0, 90));
        rightSpineGradient.setColorAt(1.0, QColor(0, 0, 0, 30));
        p->fillRect(rightShadowRect, rightSpineGradient);
    }

    {
        QRectF bottomShadowRect(coverRect.x() + 2, coverRect.bottom(), coverRect.width() - 2, shadowOffsetY);
        QLinearGradient bottomSpineGradient(bottomShadowRect.topLeft(), bottomShadowRect.bottomLeft());
        bottomSpineGradient.setColorAt(0.0, QColor(0, 0, 0, 70));
        bottomSpineGradient.setColorAt(1.0, QColor(0, 0, 0, 20));
        p->fillRect(bottomShadowRect, bottomSpineGradient);
    }

    if (!cover.isNull()) {
        p->drawPixmap(coverRect.toRect(), cover);
    } else {
        drawPlaceholder(p, coverRect);
    }

    {
        QRectF leftHighlightRect(coverRect.x(), coverRect.y(), 2.0, coverRect.height());
        QLinearGradient leftHighlight(leftHighlightRect.topLeft(), leftHighlightRect.topRight());
        leftHighlight.setColorAt(0.0, QColor(255, 255, 255, 60));
        leftHighlight.setColorAt(1.0, QColor(255, 255, 255, 0));
        p->fillRect(leftHighlightRect, leftHighlight);
    }

    p->setPen(QColor(0, 0, 0, 100));
    p->drawRect(coverRect);

    p->restore();
}

void PainterHelpers::drawDivider(QPainter* p, const QLineF& line, const QColor& color, int width)
{
    if (!p) return;

    p->save();
    QPen pen(color, width);
    p->setPen(pen);
    p->drawLine(line);
    p->restore();
}

qreal PainterHelpers::drawWrappedText(QPainter* p, const QRectF& rect, const QString& text,
                                      const QFont& font, const QColor& color,
                                      Qt::Alignment alignment, int lineHeight)
{
    if (!p || text.isEmpty()) return 0.0;

    p->save();
    p->setFont(font);
    p->setPen(color);

    QFontMetrics fm(font);
    qreal leading = fm.leading();
    qreal height = 0.0;

    if (lineHeight <= 0) {
        lineHeight = fm.height();
    }

    // 按\n分段处理
    QStringList paragraphs = text.split(QLatin1Char('\n'));
    qreal y = rect.y();

    for (const QString& paragraph : paragraphs) {
        if (paragraph.isEmpty()) {
            y += lineHeight;
            height += lineHeight;
            continue;
        }

        QTextLayout textLayout(paragraph, font, p->device());
        textLayout.beginLayout();

        qreal paragraphHeight = 0.0;
        while (true) {
            QTextLine line = textLayout.createLine();
            if (!line.isValid()) break;

            line.setLineWidth(rect.width());
            qreal lineX = rect.x();
            qreal lineY = y;

            // 处理对齐方式
            if (alignment & Qt::AlignHCenter) {
                lineX += (rect.width() - line.naturalTextWidth()) / 2.0;
            } else if (alignment & Qt::AlignRight) {
                lineX += rect.width() - line.naturalTextWidth();
            }

            line.setPosition(QPointF(lineX, lineY));
            line.draw(p, QPointF(0, 0));

            y += lineHeight;
            paragraphHeight += lineHeight;
            height += lineHeight;
        }

        textLayout.endLayout();
    }

    p->restore();
    return height;
}

qreal PainterHelpers::textHeight(QPainter* p, qreal width, const QString& text,
                                 const QFont& font, int lineHeight)
{
    if (!p || text.isEmpty() || width <= 0) return 0.0;

    QFontMetrics fm(font);
    if (lineHeight <= 0) {
        lineHeight = fm.height();
    }

    qreal totalHeight = 0.0;
    QStringList paragraphs = text.split(QLatin1Char('\n'));

    for (const QString& paragraph : paragraphs) {
        if (paragraph.isEmpty()) {
            totalHeight += lineHeight;
            continue;
        }

        QTextLayout textLayout(paragraph, font, p->device());
        textLayout.beginLayout();

        while (true) {
            QTextLine line = textLayout.createLine();
            if (!line.isValid()) break;
            line.setLineWidth(width);
            totalHeight += lineHeight;
        }

        textLayout.endLayout();
    }

    return totalHeight;
}

void PainterHelpers::drawPlaceholder(QPainter* p, const QRectF& rect, const QString& text)
{
    if (!p) return;

    p->save();
    p->setRenderHint(QPainter::Antialiasing, true);

    // 绘制灰色背景
    p->fillRect(rect, COLOR_PLACEHOLDER);

    // 绘制边框
    QPen pen(QColor(180, 180, 180));
    pen.setStyle(Qt::DashLine);
    p->setPen(pen);
    p->drawRect(rect);

    // 绘制居中文字
    if (!text.isEmpty()) {
        QFont font;
        font.setPixelSize(FONT_SMALL_SIZE);
        p->setFont(font);
        p->setPen(QColor(150, 150, 150));

        QFontMetrics fm(font);
        QRectF textRect = fm.boundingRect(text);
        qreal x = rect.x() + (rect.width() - textRect.width()) / 2.0;
        qreal y = rect.y() + (rect.height() - textRect.height()) / 2.0 + fm.ascent();
        p->drawText(QPointF(x, y), text);
    }

    p->restore();
}
