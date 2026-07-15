#ifndef PAINTERHELPERS_H
#define PAINTERHELPERS_H

#include <QPainter>
#include <QPixmap>
#include <QRectF>
#include <QLineF>
#include <QColor>
#include <QFont>
#include <QString>
#include <QtGlobal>
#include "LayoutConstants.h"

class PainterHelpers
{
public:
    // 绘制圆角矩形
    static void drawRoundedRect(QPainter* p, const QRectF& rect, const QColor& fillColor,
                                qreal radius = CORNER_RADIUS,
                                const QColor& borderColor = QColor(),
                                int borderWidth = 0);

    // 绘制渐变横幅（从上到下渐变，居中显示标题文字）
    static void drawGradientBanner(QPainter* p, const QRectF& rect, const QColor& color,
                                   const QString& title,
                                   const QColor& textColor = COLOR_WHITE);

    // 绘制右上角弧形条幅
    static void drawCurvedBanner(QPainter* p, const QRectF& rect, const QColor& color,
                                 const QString& title,
                                 const QColor& textColor = COLOR_WHITE);

    // 绘制带红色圆点的标题栏
    static void drawSectionTitle(QPainter* p, const QRectF& rect, const QString& title,
                                 const QColor& bgColor = COLOR_INTRO_BG,
                                 const QColor& dotColor = COLOR_LABEL_RED);

    // 绘制3D效果书籍封面
    static void drawBookCover3D(QPainter* p, const QPixmap& cover, const QRectF& rect);

    // 绘制分隔线
    static void drawDivider(QPainter* p, const QLineF& line,
                            const QColor& color = COLOR_DIVIDER, int width = 1);

    // 绘制自动换行文本，返回实际使用高度
    static qreal drawWrappedText(QPainter* p, const QRectF& rect, const QString& text,
                                 const QFont& font,
                                 const QColor& color = COLOR_TEXT_BLACK,
                                 Qt::Alignment alignment = Qt::AlignLeft | Qt::AlignTop,
                                 int lineHeight = 0);

    // 计算文本在指定宽度下的高度
    static qreal textHeight(QPainter* p, qreal width, const QString& text,
                            const QFont& font, int lineHeight = 0);

    // 绘制占位图
    static void drawPlaceholder(QPainter* p, const QRectF& rect,
                                const QString& text = QString::fromUtf8("无图"));
};

#endif // PAINTERHELPERS_H
