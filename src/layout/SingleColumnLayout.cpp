#include "SingleColumnLayout.h"
#include <QFontMetrics>
#include <QImage>
#include <QMarginsF>
#include <QtMath>

// ========== 布局间距常量 ==========
static const qreal SPACING_BANNER_BOTTOM = 20.0;    // 横幅下方间距
static const qreal UPPER_SECTION_HEIGHT = 250.0;    // 上半部分高度
static const qreal SPACING_INFO_COVER_GAP = 20.0;   // 封面与信息区域间距
static const qreal INFO_LINE_HEIGHT = 20.0;         // 信息行高
static const qreal SPACING_UPPER_BOTTOM = 20.0;     // 上半部分下方间距
static const qreal SECTION_TITLE_HEIGHT = 30.0;     // 内容简介标题栏高度
static const qreal SPACING_TITLE_BOTTOM = 10.0;     // 标题栏下方间距
static const qreal BODY_LINE_HEIGHT = 16.0;         // 正文行高
static const qreal SPACING_IMAGE_TITLE = 5.0;       // 图片标题下方间距
static const qreal SPACING_BOTTOM_GAP = 10.0;       // 底部区域分栏间距
static const qreal IMAGE_PADDING = 5.0;             // 图片间间距

SingleColumnLayout::SingleColumnLayout()
    : m_pageCount(0)
{
}

SingleColumnLayout::~SingleColumnLayout()
{
}

void SingleColumnLayout::calculateLayout()
{
    m_pageCount = m_books.size();
    m_dirty = false;
}

int SingleColumnLayout::pageCount() const
{
    return m_pageCount;
}

void SingleColumnLayout::renderPage(QPainter* painter, int pageIndex, const QRectF& pageRect)
{
    if (!painter || pageIndex < 0 || pageIndex >= m_books.size()) {
        return;
    }

    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setRenderHint(QPainter::TextAntialiasing, true);
    painter->setRenderHint(QPainter::SmoothPixmapTransform, true);
#if QT_VERSION >= QT_VERSION_CHECK(5, 13, 0)
    painter->setRenderHint(QPainter::LosslessImageRendering, true);
#endif

    const BookPtr& book = m_books.at(pageIndex);
    QRectF contentRect = pageRect.marginsRemoved(QMarginsF(PAGE_MARGIN, PAGE_MARGIN, PAGE_MARGIN, PAGE_MARGIN));
    qreal currentY = contentRect.top();

    // ========== 1. 顶部横幅 ==========
    QRectF bannerRect(contentRect.left(), currentY, contentRect.width(), BANNER_HEIGHT);
    PainterHelpers::drawGradientBanner(painter, bannerRect, COLOR_BANNER_BLUE,
                                       QString::fromUtf8("重点书推荐"));
    currentY += BANNER_HEIGHT + SPACING_BANNER_BOTTOM;

    // ========== 2. 上半部分（封面+信息） ==========
    QRectF upperRect(contentRect.left(), currentY, contentRect.width(), UPPER_SECTION_HEIGHT);
    qreal coverWidth = upperRect.width() / 3.0;

    // 左侧封面区域
    QRectF coverRect(upperRect.left(), upperRect.top(), coverWidth, upperRect.height());
    QPixmap cover;
    if (!book->coverImagePath().isEmpty()) {
        cover = ImageCache::instance().getPixmap(book->coverImagePath());
    }
    if (!cover.isNull()) {
        // 等比缩放计算实际封面显示区域
        qreal scaleX = coverRect.width() / cover.width();
        qreal scaleY = coverRect.height() / cover.height();
        qreal scale = qMin(scaleX, scaleY);
        qreal actualWidth = cover.width() * scale;
        qreal actualHeight = cover.height() * scale;
        qreal offsetX = (coverRect.width() - actualWidth) / 2.0;
        qreal offsetY = (coverRect.height() - actualHeight) / 2.0;
        QRectF actualCoverRect(coverRect.x() + offsetX, coverRect.y() + offsetY,
                               actualWidth, actualHeight);
        PainterHelpers::drawBookCover3D(painter, cover, actualCoverRect);
    } else {
        PainterHelpers::drawBookCover3D(painter, QPixmap(), coverRect);
    }

    // 右侧信息区域
    qreal infoX = coverRect.right() + SPACING_INFO_COVER_GAP;
    qreal infoY = upperRect.top();
    QFont labelFont;
    labelFont.setPixelSize(FONT_LABEL_SIZE);
    labelFont.setBold(true);
    QFont contentFont;
    contentFont.setPixelSize(FONT_LABEL_SIZE);
    QFontMetrics labelFm(labelFont);

    auto drawInfoLine = [&](const QString& label, const QString& content) {
        if (content.isEmpty()) {
            return;
        }
        painter->setFont(labelFont);
        painter->setPen(COLOR_LABEL_RED);
        painter->drawText(QPointF(infoX, infoY + labelFm.ascent()), label);
        qreal labelWidth = labelFm.boundingRect(label).width();
        painter->setFont(contentFont);
        painter->setPen(COLOR_TEXT_BLACK);
        painter->drawText(QPointF(infoX + labelWidth, infoY + labelFm.ascent()), content);
        infoY += INFO_LINE_HEIGHT;
    };

    drawInfoLine(QString::fromUtf8("书 名："), book->title());
    if (book->hasSeries()) {
        drawInfoLine(QString::fromUtf8("丛 书 名："), book->seriesName());
    }
    drawInfoLine(QString::fromUtf8("书 号："), book->isbn());
    drawInfoLine(QString::fromUtf8("作 者："), book->author());
    drawInfoLine(QString::fromUtf8("定 价："), book->price());
    drawInfoLine(QString::fromUtf8("出版时间："), book->publishDate());

    currentY += UPPER_SECTION_HEIGHT + SPACING_UPPER_BOTTOM;

    // ========== 3. 内容简介区域 ==========
    QRectF introTitleRect(contentRect.left(), currentY, contentRect.width(), SECTION_TITLE_HEIGHT);
    PainterHelpers::drawSectionTitle(painter, introTitleRect, QString::fromUtf8("内容简介："));
    currentY += SECTION_TITLE_HEIGHT + SPACING_TITLE_BOTTOM;

    QFont bodyFont;
    bodyFont.setPixelSize(FONT_BODY_SIZE);
    qreal introHeight = PainterHelpers::textHeight(painter, contentRect.width(),
                                                    book->description(), bodyFont,
                                                    static_cast<int>(BODY_LINE_HEIGHT));
    QRectF introTextRect(contentRect.left(), currentY, contentRect.width(), introHeight);
    PainterHelpers::drawWrappedText(painter, introTextRect, book->description(),
                                    bodyFont, COLOR_TEXT_BLACK,
                                    Qt::AlignLeft | Qt::AlignTop,
                                    static_cast<int>(BODY_LINE_HEIGHT));
    currentY += introHeight + SPACING_UPPER_BOTTOM;

    // ========== 4. 底部区域（样章/二维码） ==========
    QRectF bottomRect(contentRect.left(), currentY,
                      contentRect.width(), contentRect.bottom() - currentY);
    bool hasSample = book->hasSampleImages();
    bool hasQr = book->hasQrCodes();

    if (hasSample && hasQr) {
        qreal halfWidth = (bottomRect.width() - SPACING_BOTTOM_GAP) / 2.0;
        QRectF sampleRect(bottomRect.left(), bottomRect.top(), halfWidth, bottomRect.height());
        QRectF qrRect(bottomRect.left() + halfWidth + SPACING_BOTTOM_GAP,
                      bottomRect.top(), halfWidth, bottomRect.height());
        drawImageSection(painter, sampleRect, QString::fromUtf8("样章："), book->sampleImages());
        drawImageSection(painter, qrRect, QString::fromUtf8("二维码："), book->qrCodes());
    } else if (hasSample) {
        drawImageSection(painter, bottomRect, QString::fromUtf8("样章："), book->sampleImages());
    } else if (hasQr) {
        drawImageSection(painter, bottomRect, QString::fromUtf8("二维码："), book->qrCodes());
    }

    QFont pageNumFont;
    pageNumFont.setPixelSize(FONT_SMALL_SIZE);
    painter->setFont(pageNumFont);
    painter->setPen(QColor(128, 128, 128));
    QRectF pageNumRect(pageRect.right() - 80, pageRect.bottom() - 30, 70, 20);
    QString pageNumText = QString::fromUtf8("第%1页").arg(pageIndex + 1);
    painter->drawText(pageNumRect, Qt::AlignCenter, pageNumText);
}

void SingleColumnLayout::drawImageSection(QPainter* painter, const QRectF& rect,
                                           const QString& title, const QStringList& imagePaths)
{
    if (imagePaths.isEmpty()) {
        return;
    }

    QFont labelFont;
    labelFont.setPixelSize(FONT_LABEL_SIZE);
    labelFont.setBold(true);
    QFontMetrics fm(labelFont);

    painter->setFont(labelFont);
    painter->setPen(COLOR_LABEL_RED);
    painter->drawText(QPointF(rect.left(), rect.top() + fm.ascent()), title);

    qreal titleHeight = fm.height();
    QRectF imageRect(rect.left(), rect.top() + titleHeight + SPACING_IMAGE_TITLE,
                     rect.width(), rect.height() - titleHeight - SPACING_IMAGE_TITLE);
    drawImagesInRect(painter, imageRect, imagePaths);
}

void SingleColumnLayout::drawImagesInRect(QPainter* painter, const QRectF& rect,
                                           const QStringList& imagePaths)
{
    if (imagePaths.isEmpty()) {
        return;
    }

    int count = imagePaths.size();
    qreal areaWidth = rect.width();
    qreal areaHeight = rect.height();

    // 计算最佳网格布局：尝试1xN, 2xM等
    int cols = 1;
    int rows = 1;

    if (count <= 2) {
        cols = count;
        rows = 1;
    } else if (count <= 4) {
        cols = 2;
        rows = (count + 1) / 2;
    } else {
        cols = static_cast<int>(qCeil(qSqrt(static_cast<qreal>(count) * areaWidth / areaHeight)));
        if (cols < 1) cols = 1;
        rows = (count + cols - 1) / cols;
    }

    qreal cellWidth = (areaWidth - IMAGE_PADDING * (cols - 1)) / cols;
    qreal cellHeight = (areaHeight - IMAGE_PADDING * (rows - 1)) / rows;

    for (int i = 0; i < count; ++i) {
        int row = i / cols;
        int col = i % cols;
        qreal x = rect.left() + col * (cellWidth + IMAGE_PADDING);
        qreal y = rect.top() + row * (cellHeight + IMAGE_PADDING);
        QRectF cellRect(x, y, cellWidth, cellHeight);

        QPixmap pixmap = ImageCache::instance().getPixmap(imagePaths.at(i));
        if (!pixmap.isNull()) {
            QPixmap scaled = pixmap.scaled(cellRect.size().toSize(),
                                           Qt::KeepAspectRatio,
                                           Qt::SmoothTransformation);
            qreal offsetX = (cellRect.width() - scaled.width()) / 2.0;
            qreal offsetY = (cellRect.height() - scaled.height()) / 2.0;
            painter->drawPixmap(QPointF(x + offsetX, y + offsetY), scaled);
        } else {
            PainterHelpers::drawPlaceholder(painter, cellRect);
        }
    }
}

// ============================================================
// generateElements：生成页面元素列表
// 将renderPage的绘制逻辑转换为PageElement数据元素
// 位置/尺寸计算与renderPage保持一致
// ============================================================
QList<PageElementPtr> SingleColumnLayout::generateElements(int pageIndex, const QRectF& pageRect)
{
    QList<PageElementPtr> elements;

    if (pageIndex < 0 || pageIndex >= m_books.size()) {
        return elements;
    }

    const BookPtr& book = m_books.at(pageIndex);
    QRectF contentRect = pageRect.marginsRemoved(
        QMarginsF(PAGE_MARGIN, PAGE_MARGIN, PAGE_MARGIN, PAGE_MARGIN));
    qreal currentY = contentRect.top();
    int zOrder = 0;  // 层级计数器，从下到上递增

    // ========== 1. 顶部横幅 ==========
    QRectF bannerRect(contentRect.left(), currentY, contentRect.width(), BANNER_HEIGHT);

    // 横幅背景（渐变圆角矩形）
    {
        ShapeElementPtr shape(new ShapeElementData());
        shape->setShapeType(ShapeElementData::RoundedRect);
        shape->setFillColor(COLOR_BANNER_BLUE);
        shape->setHasFill(true);
        shape->setGradientColor(COLOR_BANNER_BLUE.lighter(110));
        shape->setHasGradient(true);
        shape->setCornerRadius(CORNER_RADIUS);
        shape->setHasBorder(false);
        shape->setRect(bannerRect);
        shape->setName(QString::fromUtf8("横幅"));
        shape->setZOrder(zOrder++);
        elements.append(PageElementPtr(shape.data()));
    }

    // 横幅标题文字
    {
        TextElementPtr text(new TextElementData());
        text->setText(QString::fromUtf8("重点书推荐"));
        QFont font;
        font.setPixelSize(FONT_TITLE_SIZE);
        font.setBold(true);
        text->setFont(font);
        text->setTextColor(COLOR_WHITE);
        text->setAlignment(Qt::AlignCenter);
        text->setRect(bannerRect);
        text->setName(QString::fromUtf8("横幅标题"));
        text->setZOrder(zOrder++);
        elements.append(PageElementPtr(text.data()));
    }

    currentY += BANNER_HEIGHT + SPACING_BANNER_BOTTOM;

    // ========== 2. 上半部分（封面+信息） ==========
    QRectF upperRect(contentRect.left(), currentY, contentRect.width(), UPPER_SECTION_HEIGHT);
    qreal coverWidth = upperRect.width() / 3.0;
    QRectF coverRect(upperRect.left(), upperRect.top(), coverWidth, upperRect.height());

    // 封面图片
    {
        ImageElementPtr image(new ImageElementData());
        image->setImagePath(book->coverImagePath());
        image->setRect(coverRect);
        image->setKeepAspectRatio(true);
        image->setScaleFactor(1.0);
        image->setOpacity(1.0);
        image->setName(QString::fromUtf8("封面"));
        image->setZOrder(zOrder++);
        elements.append(PageElementPtr(image.data()));
    }

    // 右侧信息区域
    qreal infoX = coverRect.right() + SPACING_INFO_COVER_GAP;
    qreal infoY = upperRect.top();

    QFont labelFont;
    labelFont.setPixelSize(FONT_LABEL_SIZE);
    labelFont.setBold(true);
    QFont contentFont;
    contentFont.setPixelSize(FONT_LABEL_SIZE);
    QFontMetrics labelFm(labelFont);

    // 辅助lambda：生成一行信息（标签+内容两个TextElement）
    auto addInfoLine = [&](const QString& label, const QString& content, const QString& name) {
        if (content.isEmpty()) {
            return;
        }
        qreal labelWidth = labelFm.boundingRect(label).width();

        // 标签元素（红色加粗）
        {
            TextElementPtr text(new TextElementData());
            text->setText(label);
            text->setFont(labelFont);
            text->setTextColor(COLOR_LABEL_RED);
            text->setAlignment(Qt::AlignLeft | Qt::AlignTop);
            text->setRect(QRectF(infoX, infoY, labelWidth, INFO_LINE_HEIGHT));
            text->setName(name + QString::fromUtf8("标签"));
            text->setZOrder(zOrder++);
            elements.append(PageElementPtr(text.data()));
        }

        // 内容元素（黑色常规）
        {
            TextElementPtr text(new TextElementData());
            text->setText(content);
            text->setFont(contentFont);
            text->setTextColor(COLOR_TEXT_BLACK);
            text->setAlignment(Qt::AlignLeft | Qt::AlignTop);
            qreal contentX = infoX + labelWidth;
            text->setRect(QRectF(contentX, infoY,
                                 upperRect.right() - contentX, INFO_LINE_HEIGHT));
            text->setName(name + QString::fromUtf8("内容"));
            text->setZOrder(zOrder++);
            elements.append(PageElementPtr(text.data()));
        }

        infoY += INFO_LINE_HEIGHT;
    };

    addInfoLine(QString::fromUtf8("书 名："), book->title(), QString::fromUtf8("书名"));
    if (book->hasSeries()) {
        addInfoLine(QString::fromUtf8("丛 书 名："), book->seriesName(), QString::fromUtf8("丛书名"));
    }
    addInfoLine(QString::fromUtf8("书 号："), book->isbn(), QString::fromUtf8("书号"));
    addInfoLine(QString::fromUtf8("作 者："), book->author(), QString::fromUtf8("作者"));
    addInfoLine(QString::fromUtf8("定 价："), book->price(), QString::fromUtf8("定价"));
    addInfoLine(QString::fromUtf8("出版时间："), book->publishDate(), QString::fromUtf8("出版时间"));

    currentY += UPPER_SECTION_HEIGHT + SPACING_UPPER_BOTTOM;

    // ========== 3. 内容简介区域 ==========
    QRectF introTitleRect(contentRect.left(), currentY, contentRect.width(), SECTION_TITLE_HEIGHT);

    // 内容简介标题栏背景（黄色圆角矩形）
    {
        ShapeElementPtr shape(new ShapeElementData());
        shape->setShapeType(ShapeElementData::RoundedRect);
        shape->setFillColor(COLOR_INTRO_BG);
        shape->setHasFill(true);
        shape->setHasGradient(false);
        shape->setCornerRadius(CORNER_RADIUS);
        shape->setHasBorder(false);
        shape->setRect(introTitleRect);
        shape->setName(QString::fromUtf8("内容简介标题栏"));
        shape->setZOrder(zOrder++);
        elements.append(PageElementPtr(shape.data()));
    }

    // 红色圆点装饰
    {
        qreal dotRadius = 4.0;
        qreal dotX = introTitleRect.x() + 10.0 + dotRadius;
        qreal dotY = introTitleRect.y() + introTitleRect.height() / 2.0;
        ShapeElementPtr dot(new ShapeElementData());
        dot->setShapeType(ShapeElementData::Ellipse);
        dot->setFillColor(COLOR_LABEL_RED);
        dot->setHasFill(true);
        dot->setHasGradient(false);
        dot->setHasBorder(false);
        dot->setRect(QRectF(dotX - dotRadius, dotY - dotRadius,
                            dotRadius * 2.0, dotRadius * 2.0));
        dot->setName(QString::fromUtf8("标题圆点"));
        dot->setZOrder(zOrder++);
        elements.append(PageElementPtr(dot.data()));
    }

    // 内容简介标题文字
    {
        TextElementPtr text(new TextElementData());
        text->setText(QString::fromUtf8("内容简介："));
        QFont font;
        font.setPixelSize(FONT_LABEL_SIZE);
        font.setBold(true);
        text->setFont(font);
        text->setTextColor(COLOR_TEXT_BLACK);
        text->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        text->setRect(introTitleRect);
        text->setName(QString::fromUtf8("内容简介标题"));
        text->setZOrder(zOrder++);
        elements.append(PageElementPtr(text.data()));
    }

    currentY += SECTION_TITLE_HEIGHT + SPACING_TITLE_BOTTOM;

    // 计算内容简介正文高度（需要临时QPainter用于QTextLayout）
    QFont bodyFont;
    bodyFont.setPixelSize(FONT_BODY_SIZE);
    qreal introHeight = 0.0;
    {
        QImage tempImage(1, 1, QImage::Format_ARGB32);
        QPainter tempPainter(&tempImage);
        introHeight = PainterHelpers::textHeight(&tempPainter, contentRect.width(),
                                                  book->description(), bodyFont,
                                                  static_cast<int>(BODY_LINE_HEIGHT));
    }

    // 内容简介正文
    {
        TextElementPtr text(new TextElementData());
        text->setText(book->description());
        text->setFont(bodyFont);
        text->setTextColor(COLOR_TEXT_BLACK);
        text->setAlignment(Qt::AlignLeft | Qt::AlignTop);
        text->setLineHeight(BODY_LINE_HEIGHT);
        text->setRect(QRectF(contentRect.left(), currentY,
                             contentRect.width(), introHeight));
        text->setName(QString::fromUtf8("内容简介正文"));
        text->setZOrder(zOrder++);
        elements.append(PageElementPtr(text.data()));
    }

    currentY += introHeight + SPACING_UPPER_BOTTOM;

    // ========== 4. 底部区域（样章/二维码） ==========
    QRectF bottomRect(contentRect.left(), currentY,
                      contentRect.width(), contentRect.bottom() - currentY);
    bool hasSample = book->hasSampleImages();
    bool hasQr = book->hasQrCodes();

    if (hasSample && hasQr) {
        qreal halfWidth = (bottomRect.width() - SPACING_BOTTOM_GAP) / 2.0;
        QRectF sampleRect(bottomRect.left(), bottomRect.top(), halfWidth, bottomRect.height());
        QRectF qrRect(bottomRect.left() + halfWidth + SPACING_BOTTOM_GAP,
                      bottomRect.top(), halfWidth, bottomRect.height());
        generateImageSectionElements(elements, zOrder, sampleRect,
                                     QString::fromUtf8("样章："), book->sampleImages());
        generateImageSectionElements(elements, zOrder, qrRect,
                                     QString::fromUtf8("二维码："), book->qrCodes());
    } else if (hasSample) {
        generateImageSectionElements(elements, zOrder, bottomRect,
                                     QString::fromUtf8("样章："), book->sampleImages());
    } else if (hasQr) {
        generateImageSectionElements(elements, zOrder, bottomRect,
                                     QString::fromUtf8("二维码："), book->qrCodes());
    }

    // ========== 5. 页码 ==========
    {
        TextElementPtr text(new TextElementData());
        text->setText(QString::fromUtf8("第%1页").arg(pageIndex + 1));
        QFont font;
        font.setPixelSize(FONT_SMALL_SIZE);
        text->setFont(font);
        text->setTextColor(QColor(128, 128, 128));
        text->setAlignment(Qt::AlignCenter);
        text->setRect(QRectF(pageRect.right() - 80, pageRect.bottom() - 30, 70, 20));
        text->setName(QString::fromUtf8("页码"));
        text->setZOrder(zOrder++);
        elements.append(PageElementPtr(text.data()));
    }

    return elements;
}

// ============================================================
// pageType：返回页面类型（0=单图排版）
// ============================================================
int SingleColumnLayout::pageType(int pageIndex) const
{
    Q_UNUSED(pageIndex)
    return 0;
}

// ============================================================
// generateImageSectionElements：生成底部图片区域的页面元素
// 对应drawImageSection + drawImagesInRect的元素生成版本
// 计算逻辑与绘制版本完全一致
// ============================================================
void SingleColumnLayout::generateImageSectionElements(
    QList<PageElementPtr>& elements, int& zOrder,
    const QRectF& rect, const QString& title, const QStringList& imagePaths)
{
    if (imagePaths.isEmpty()) {
        return;
    }

    // 标题文字（红色加粗）
    QFont labelFont;
    labelFont.setPixelSize(FONT_LABEL_SIZE);
    labelFont.setBold(true);
    QFontMetrics fm(labelFont);

    {
        TextElementPtr text(new TextElementData());
        text->setText(title);
        text->setFont(labelFont);
        text->setTextColor(COLOR_LABEL_RED);
        text->setAlignment(Qt::AlignLeft | Qt::AlignTop);
        text->setRect(QRectF(rect.left(), rect.top(),
                             rect.width(), fm.height()));
        text->setName(title.trimmed() + QString::fromUtf8("标题"));
        text->setZOrder(zOrder++);
        elements.append(PageElementPtr(text.data()));
    }

    // 计算图片网格布局（与drawImagesInRect一致）
    qreal titleHeight = fm.height();
    QRectF imageRect(rect.left(), rect.top() + titleHeight + SPACING_IMAGE_TITLE,
                     rect.width(), rect.height() - titleHeight - SPACING_IMAGE_TITLE);

    int count = imagePaths.size();
    qreal areaWidth = imageRect.width();
    qreal areaHeight = imageRect.height();

    int cols = 1;
    int rows = 1;
    if (count <= 2) {
        cols = count;
        rows = 1;
    } else if (count <= 4) {
        cols = 2;
        rows = (count + 1) / 2;
    } else {
        cols = static_cast<int>(qCeil(qSqrt(static_cast<qreal>(count) * areaWidth / areaHeight)));
        if (cols < 1) cols = 1;
        rows = (count + cols - 1) / cols;
    }

    qreal cellWidth = (areaWidth - IMAGE_PADDING * (cols - 1)) / cols;
    qreal cellHeight = (areaHeight - IMAGE_PADDING * (rows - 1)) / rows;

    // 为每张图片创建ImageElement
    for (int i = 0; i < count; ++i) {
        int row = i / cols;
        int col = i % cols;
        qreal x = imageRect.left() + col * (cellWidth + IMAGE_PADDING);
        qreal y = imageRect.top() + row * (cellHeight + IMAGE_PADDING);
        QRectF cellRect(x, y, cellWidth, cellHeight);

        ImageElementPtr image(new ImageElementData());
        image->setImagePath(imagePaths.at(i));
        image->setRect(cellRect);
        image->setKeepAspectRatio(true);
        image->setScaleFactor(1.0);
        image->setOpacity(1.0);
        image->setName(title.trimmed() + QString::fromUtf8("图片%1").arg(i + 1));
        image->setZOrder(zOrder++);
        elements.append(PageElementPtr(image.data()));
    }
}
