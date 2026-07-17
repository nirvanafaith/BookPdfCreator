#include "MultiColumnLayout.h"
#include "parsers/ImageCache.h"
#include <QFontMetrics>
#include <QImage>
#include <QMarginsF>
#include <QTextLayout>
#include <QtMath>

namespace {
// 在原 rect 范围内居中放置按图片宽高比调整后的 rect
QRectF adjustRectToAspect(const QRectF& srcRect, const QSize& imageSize)
{
    if (imageSize.isEmpty() || imageSize.width() <= 0 || imageSize.height() <= 0) {
        return srcRect;  // 图片尺寸无效，返回原 rect
    }

    qreal imageAspect = static_cast<qreal>(imageSize.width()) / static_cast<qreal>(imageSize.height());
    qreal rectAspect = srcRect.width() / srcRect.height();

    qreal newW, newH;
    if (imageAspect > rectAspect) {
        // 图片更宽：保持宽度，调整高度
        newW = srcRect.width();
        newH = srcRect.width() / imageAspect;
    } else {
        // 图片更高：保持高度，调整宽度
        newH = srcRect.height();
        newW = srcRect.height() * imageAspect;
    }

    qreal offsetX = (srcRect.width() - newW) / 2.0;
    qreal offsetY = (srcRect.height() - newH) / 2.0;
    return QRectF(srcRect.left() + offsetX, srcRect.top() + offsetY, newW, newH);
}
}  // namespace

// ========== 布局间距常量 ==========
const qreal MultiColumnLayout::SPACING_BANNER_BOTTOM = 15.0;  // 横幅下方间距
const qreal MultiColumnLayout::SPACING_BOOK_GAP = 10.0;       // 书籍条目间底部间距
const qreal MultiColumnLayout::COVER_WIDTH = 80.0;            // 封面小图宽度
const qreal MultiColumnLayout::SPACING_TEXT_COVER_GAP = 12.0; // 文字区与封面间距
const qreal MultiColumnLayout::SPACING_LINE_GAP = 4.0;        // 文字行间间距
const qreal MultiColumnLayout::DESC_LINE_HEIGHT = 14.0;       // 内容简介行高
const qreal MultiColumnLayout::DIVIDER_SPACING = 10.0;        // 分隔线上下间距

MultiColumnLayout::MultiColumnLayout()
    : m_pageCount(0)
{
}

MultiColumnLayout::~MultiColumnLayout()
{
}

void MultiColumnLayout::calculateLayout()
{
    m_pages.clear();
    m_pageHeights.clear();

    if (m_books.isEmpty()) {
        m_pageCount = 0;
        m_dirty = false;
        return;
    }

    // 创建临时QImage和QPainter用于计算文本高度
    QImage tempImage(1, 1, QImage::Format_ARGB32);
    QPainter tempPainter(&tempImage);

    // 计算内容区域尺寸（A4减去50点边距）
    qreal contentWidth = A4_WIDTH - PAGE_MARGIN * 2.0;
    qreal contentHeight = A4_HEIGHT - PAGE_MARGIN * 2.0;
    qreal availableHeight = contentHeight - BANNER_HEIGHT - SPACING_BANNER_BOTTOM;

    // 先计算每本书的高度
    QList<qreal> bookHeights;
    int globalBookIndex = 0;
    for (const BookPtr& book : m_books) {
        bool imageOnRight = (globalBookIndex % 2 == 0);
        qreal height = calculateBookItemHeight(&tempPainter, book, contentWidth, imageOnRight);
        bookHeights.append(height);
        globalBookIndex++;
    }

    // 分页：每页尽量放3本，或放满可用高度
    QList<BookPtr> currentPage;
    QList<qreal> currentPageHeights;
    qreal currentPageHeight = 0.0;
    const int TARGET_BOOKS_PER_PAGE = 3;

    for (int i = 0; i < m_books.size(); ++i) {
        qreal bookHeight = bookHeights.at(i);

        // 计算添加这本书需要的额外高度
        qreal spacingNeeded = 0.0;
        if (!currentPage.isEmpty()) {
            spacingNeeded = SPACING_BOOK_GAP + DIVIDER_SPACING * 2.0;
        }
        qreal totalNeeded = currentPageHeight + spacingNeeded + bookHeight;

        // 判断是否能在当前页放下：
        // 1. 未超过目标每页数量
        // 2. 当前页为空（必须放，即使超过高度也单独占一页）
        // 3. 或者总高度不超过可用高度
        bool canAdd = false;
        if (currentPage.size() < TARGET_BOOKS_PER_PAGE) {
            if (currentPage.isEmpty() || totalNeeded <= availableHeight) {
                canAdd = true;
            }
        }

        // 如果放不下，先保存当前页
        if (!canAdd && !currentPage.isEmpty()) {
            m_pages.append(currentPage);
            m_pageHeights.append(currentPageHeights);
            currentPage.clear();
            currentPageHeights.clear();
            currentPageHeight = 0.0;
        }

        // 添加书籍到当前页
        if (!currentPage.isEmpty()) {
            currentPageHeight += SPACING_BOOK_GAP + DIVIDER_SPACING * 2.0;
        }
        currentPage.append(m_books.at(i));
        currentPageHeights.append(bookHeight);
        currentPageHeight += bookHeight;
    }

    // 添加最后一页
    if (!currentPage.isEmpty()) {
        m_pages.append(currentPage);
        m_pageHeights.append(currentPageHeights);
    }

    m_pageCount = m_pages.size();
    m_dirty = false;
}

int MultiColumnLayout::pageCount() const
{
    return m_pageCount;
}

qreal MultiColumnLayout::calculateBookItemHeight(QPainter* painter, const BookPtr& book, qreal contentWidth, bool imageOnRight)
{
    Q_UNUSED(imageOnRight)
    qreal textWidth = contentWidth - COVER_WIDTH - SPACING_TEXT_COVER_GAP;

    // 书名字体
    QFont bookNameFont;
    bookNameFont.setPixelSize(FONT_BOOKNAME_SIZE);
    bookNameFont.setBold(true);
    QFontMetrics bookNameFm(bookNameFont);
    qreal bookNameHeight = bookNameFm.height();

    // 书号定价字体
    QFont smallFont;
    smallFont.setPixelSize(FONT_SMALL_SIZE);
    QFontMetrics smallFm(smallFont);
    qreal infoLineHeight = smallFm.height();

    // 内容简介标签宽度
    QFont descLabelFont;
    descLabelFont.setPixelSize(FONT_SMALL_SIZE);
    descLabelFont.setBold(true);
    QFontMetrics descLabelFm(descLabelFont);
    qreal labelWidth = descLabelFm.boundingRect(QString::fromUtf8("内容简介：")).width();

    // 计算内容简介高度（第一行在标签右侧，宽度较小）
    QFont descFont;
    descFont.setPixelSize(FONT_SMALL_SIZE);
    qreal descHeight = 0.0;
    if (!book->description().isEmpty()) {
        QStringList paragraphs = book->description().split(QLatin1Char('\n'));
        bool firstLineDrawn = false;

        for (const QString& paragraph : paragraphs) {
            if (paragraph.isEmpty()) {
                descHeight += DESC_LINE_HEIGHT;
                firstLineDrawn = true;
                continue;
            }

            QTextLayout textLayout(paragraph, descFont, painter->device());
            textLayout.beginLayout();
            qreal firstLineWidth = firstLineDrawn ? textWidth : (textWidth - labelWidth);

            while (true) {
                QTextLine line = textLayout.createLine();
                if (!line.isValid()) break;
                line.setLineWidth(firstLineWidth);
                descHeight += DESC_LINE_HEIGHT;
                firstLineWidth = textWidth;
                firstLineDrawn = true;
            }

            textLayout.endLayout();
        }
    }

    // 计算封面高度（按3:4比例，减去阴影边距）
    qreal shadowMarginX = 8.0;
    qreal actualCoverWidth = COVER_WIDTH - shadowMarginX;
    qreal coverHeight = actualCoverWidth * 4.0 / 3.0 + 6.0;

    // 文字区总高度
    qreal textHeight = bookNameHeight + SPACING_LINE_GAP + infoLineHeight + SPACING_LINE_GAP + descHeight;

    // 条目高度取封面和文字区的较大值
    qreal itemHeight = qMax(coverHeight, textHeight);

    return itemHeight;
}

void MultiColumnLayout::renderPage(QPainter* painter, int pageIndex, const QRectF& pageRect)
{
    if (!painter || pageIndex < 0 || pageIndex >= m_pages.size()) {
        return;
    }

    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setRenderHint(QPainter::TextAntialiasing, true);
    painter->setRenderHint(QPainter::SmoothPixmapTransform, true);
#if QT_VERSION >= QT_VERSION_CHECK(5, 13, 0)
    painter->setRenderHint(QPainter::LosslessImageRendering, true);
#endif

    const QList<BookPtr>& pageBooks = m_pages.at(pageIndex);
    QRectF contentRect = pageRect.marginsRemoved(QMarginsF(PAGE_MARGIN, PAGE_MARGIN, PAGE_MARGIN, PAGE_MARGIN));
    qreal currentY = contentRect.top();

    int booksOnPreviousPages = 0;
    for (int p = 0; p < pageIndex; ++p) {
        booksOnPreviousPages += m_pages.at(p).size();
    }

    // ========== 1. 顶部橙红色弧形横幅 ==========
    QRectF bannerRect(contentRect.left(), currentY, contentRect.width(), BANNER_HEIGHT);
    PainterHelpers::drawCurvedBanner(painter, bannerRect, COLOR_BANNER_ORANGE,
                                     QString::fromUtf8("大众类重点推荐图书"));
    currentY += BANNER_HEIGHT + SPACING_BANNER_BOTTOM;

    // ========== 2. 遍历绘制该页书籍 ==========
    const QList<qreal>& pageHeights = m_pageHeights.at(pageIndex);
    for (int i = 0; i < pageBooks.size(); ++i) {
        const BookPtr& book = pageBooks.at(i);
        qreal itemHeight = pageHeights.at(i);
        QRectF itemRect(contentRect.left(), currentY, contentRect.width(), itemHeight);

        int globalBookIndex = booksOnPreviousPages + i;
        bool imageOnRight = (globalBookIndex % 2 == 0);

        // 绘制书籍条目
        drawBookItem(painter, book, itemRect, imageOnRight);
        currentY += itemHeight;

        // 绘制分隔线（最后一本书不画）
        if (i < pageBooks.size() - 1) {
            currentY += DIVIDER_SPACING;
            QLineF dividerLine(contentRect.left(), currentY, contentRect.right(), currentY);
            PainterHelpers::drawDivider(painter, dividerLine, COLOR_DIVIDER);
            currentY += DIVIDER_SPACING + SPACING_BOOK_GAP;
        }
    }

    QFont pageNumFont;
    pageNumFont.setPixelSize(FONT_SMALL_SIZE);
    painter->setFont(pageNumFont);
    painter->setPen(QColor(128, 128, 128));
    QRectF pageNumRect(pageRect.right() - 80, pageRect.bottom() - 30, 70, 20);
    QString pageNumText = QString::fromUtf8("%1").arg(pageIndex + 1);
    painter->drawText(pageNumRect, Qt::AlignCenter, pageNumText);
}

void MultiColumnLayout::drawBookItem(QPainter* painter, const BookPtr& book, const QRectF& itemRect, bool imageOnRight)
{
    qreal textWidth = itemRect.width() - COVER_WIDTH - SPACING_TEXT_COVER_GAP;
    qreal shadowMarginX = 8.0;
    qreal actualCoverWidth = COVER_WIDTH - shadowMarginX;
    qreal coverHeight = actualCoverWidth * 4.0 / 3.0;

    QRectF coverRect;
    QRectF actualCoverRect;
    qreal textX;

    if (imageOnRight) {
        coverRect = QRectF(itemRect.right() - COVER_WIDTH, itemRect.top(), COVER_WIDTH, itemRect.height());
        actualCoverRect = QRectF(coverRect.left(), coverRect.top(), actualCoverWidth, coverHeight);
        textX = itemRect.left();
    } else {
        coverRect = QRectF(itemRect.left(), itemRect.top(), COVER_WIDTH, itemRect.height());
        actualCoverRect = QRectF(coverRect.left(), coverRect.top(), actualCoverWidth, coverHeight);
        textX = coverRect.right() + SPACING_TEXT_COVER_GAP;
    }

    QPixmap cover;
    if (!book->coverImagePath().isEmpty()) {
        cover = ImageCache::instance().getPixmap(book->coverImagePath());
    }

    QPixmap scaledCover;
    if (!cover.isNull()) {
        qreal scaleX = actualCoverWidth / cover.width();
        qreal scaleY = coverHeight / cover.height();
        qreal scale = qMin(scaleX, scaleY);
        qreal actualWidth = cover.width() * scale;
        qreal actualHeight = cover.height() * scale;
        qreal offsetX = (actualCoverWidth - actualWidth) / 2.0;
        qreal offsetY = 0.0;
        actualCoverRect = QRectF(coverRect.x() + offsetX, coverRect.y() + offsetY,
                                 actualWidth, actualHeight);
        scaledCover = cover.scaled(actualCoverRect.size().toSize(),
                                   Qt::KeepAspectRatio,
                                   Qt::SmoothTransformation);
    }
    PainterHelpers::drawBookCover3D(painter, scaledCover, actualCoverRect);

    qreal textY = itemRect.top();

    QFont bookNameFont;
    bookNameFont.setPixelSize(FONT_BOOKNAME_SIZE);
    bookNameFont.setBold(true);
    QFontMetrics bookNameFm(bookNameFont);
    painter->setFont(bookNameFont);
    painter->setPen(COLOR_LABEL_RED);
    QString bookNameText = QString::fromUtf8("《") + book->title() + QString::fromUtf8("》");
    painter->drawText(QPointF(textX, textY + bookNameFm.ascent()), bookNameText);
    textY += bookNameFm.height() + SPACING_LINE_GAP;

    QFont smallFont;
    smallFont.setPixelSize(FONT_SMALL_SIZE);
    QFontMetrics smallFm(smallFont);
    painter->setFont(smallFont);
    painter->setPen(QColor(120, 120, 120));
    QString infoText = QString::fromUtf8("书号：") + book->isbn() +
                       QString::fromUtf8("    定价：") + book->price();
    painter->drawText(QPointF(textX, textY + smallFm.ascent()), infoText);
    textY += smallFm.height() + SPACING_LINE_GAP;

    QFont descLabelFont;
    descLabelFont.setPixelSize(FONT_SMALL_SIZE);
    descLabelFont.setBold(true);
    QFontMetrics descLabelFm(descLabelFont);
    painter->setFont(descLabelFont);
    painter->setPen(COLOR_TEXT_BLACK);
    QString introLabel = QString::fromUtf8("内容简介：");
    qreal labelWidth = descLabelFm.boundingRect(introLabel).width();
    painter->drawText(QPointF(textX, textY + descLabelFm.ascent()), introLabel);

    QFont descFont;
    descFont.setPixelSize(FONT_SMALL_SIZE);
    painter->setFont(descFont);
    painter->setPen(COLOR_TEXT_BLACK);

    if (!book->description().isEmpty()) {
        QString descText = book->description();
        QStringList paragraphs = descText.split(QLatin1Char('\n'));
        qreal drawY = textY;
        bool firstLineDrawn = false;

        for (const QString& paragraph : paragraphs) {
            if (paragraph.isEmpty()) {
                drawY += DESC_LINE_HEIGHT;
                firstLineDrawn = true;
                continue;
            }

            QTextLayout textLayout(paragraph, descFont, painter->device());
            textLayout.beginLayout();
            qreal firstLineWidth = firstLineDrawn ? textWidth : (textWidth - labelWidth);
            qreal lineX = firstLineDrawn ? textX : (textX + labelWidth);

            while (true) {
                QTextLine line = textLayout.createLine();
                if (!line.isValid()) break;

                line.setLineWidth(firstLineWidth);
                line.setPosition(QPointF(lineX, drawY));
                line.draw(painter, QPointF(0, 0));

                drawY += DESC_LINE_HEIGHT;
                firstLineWidth = textWidth;
                lineX = textX;
                firstLineDrawn = true;
            }

            textLayout.endLayout();
        }
    }
}

// ============================================================
// generateElements：生成页面元素列表
// 将renderPage的绘制逻辑转换为PageElement数据元素
// 位置/尺寸计算与renderPage保持一致
// ============================================================
QList<PageElementPtr> MultiColumnLayout::generateElements(int pageIndex, const QRectF& pageRect)
{
    QList<PageElementPtr> elements;

    if (pageIndex < 0 || pageIndex >= m_pages.size()) {
        return elements;
    }

    // 创建临时QPainter用于文本高度计算
    QImage tempImage(1, 1, QImage::Format_ARGB32);
    QPainter tempPainter(&tempImage);

    const QList<BookPtr>& pageBooks = m_pages.at(pageIndex);
    QRectF contentRect = pageRect.marginsRemoved(
        QMarginsF(PAGE_MARGIN, PAGE_MARGIN, PAGE_MARGIN, PAGE_MARGIN));
    qreal currentY = contentRect.top();
    int zOrder = 0;

    int booksOnPreviousPages = 0;
    for (int p = 0; p < pageIndex; ++p) {
        booksOnPreviousPages += m_pages.at(p).size();
    }

    // ========== 1. 顶部橙红色弧形横幅 ==========
    QRectF bannerRect(contentRect.left(), currentY, contentRect.width(), BANNER_HEIGHT);

    {
        qreal bannerWidth = bannerRect.width() * 0.68;
        qreal bannerLeft = bannerRect.right() - bannerWidth;
        QRectF pathRect(bannerLeft, bannerRect.top(), bannerWidth, bannerRect.height());

        QPainterPath path;
        path.moveTo(pathRect.topRight());
        path.lineTo(pathRect.bottomRight());
        path.lineTo(pathRect.bottomLeft());
        QPointF ctrlPoint(pathRect.left() - 30.0, pathRect.center().y());
        QPointF endPoint(pathRect.topLeft());
        path.quadTo(ctrlPoint, endPoint);
        path.closeSubpath();

        ShapeElementPtr shape(new ShapeElementData());
        shape->setShapeType(ShapeElementData::Path);
        shape->setFillColor(COLOR_BANNER_ORANGE);
        shape->setHasFill(true);
        shape->setGradientColor(COLOR_BANNER_ORANGE.lighter(110));
        shape->setHasGradient(true);
        shape->setHasBorder(false);
        shape->setPainterPath(path);
        shape->setRect(bannerRect);
        shape->setName(QString::fromUtf8("横幅"));
        shape->setZOrder(zOrder++);
        elements.append(PageElementPtr(shape.data()));
    }

    {
        QFont font;
        font.setPixelSize(FONT_TITLE_SIZE);
        font.setBold(true);
        QFontMetrics fm(font);
        QString title = QString::fromUtf8("大众类重点推荐图书");
        QRectF textRect = fm.boundingRect(title);
        qreal rightMargin = 20.0;
        qreal textX = bannerRect.right() - rightMargin - textRect.width();
        qreal textY = bannerRect.y() + (bannerRect.height() - textRect.height()) / 2.0;

        TextElementPtr text(new TextElementData());
        text->setText(title);
        text->setFont(font);
        text->setTextColor(COLOR_WHITE);
        text->setAlignment(Qt::AlignLeft | Qt::AlignTop);
        text->setRect(QRectF(textX, textY, textRect.width(), textRect.height()));
        text->setName(QString::fromUtf8("横幅标题"));
        text->setZOrder(zOrder++);
        elements.append(PageElementPtr(text.data()));
    }

    currentY += BANNER_HEIGHT + SPACING_BANNER_BOTTOM;

    // ========== 2. 遍历生成该页书籍元素 ==========
    const QList<qreal>& pageHeights = m_pageHeights.at(pageIndex);
    for (int i = 0; i < pageBooks.size(); ++i) {
        const BookPtr& book = pageBooks.at(i);
        qreal itemHeight = pageHeights.at(i);
        QRectF itemRect(contentRect.left(), currentY, contentRect.width(), itemHeight);

        int globalBookIndex = booksOnPreviousPages + i;
        bool imageOnRight = (globalBookIndex % 2 == 0);

        generateBookItemElements(elements, zOrder, book, itemRect, &tempPainter, imageOnRight);
        currentY += itemHeight;

        if (i < pageBooks.size() - 1) {
            currentY += DIVIDER_SPACING;
            {
                ShapeElementPtr divider(new ShapeElementData());
                divider->setShapeType(ShapeElementData::Line);
                divider->setHasFill(false);
                divider->setHasGradient(false);
                divider->setBorderColor(COLOR_DIVIDER);
                divider->setBorderWidth(1.0);
                divider->setHasBorder(true);
                divider->setRect(QRectF(contentRect.left(), currentY,
                                        contentRect.width(), 1.0));
                divider->setName(QString::fromUtf8("分隔线%1").arg(i + 1));
                divider->setZOrder(zOrder++);
                elements.append(PageElementPtr(divider.data()));
            }
            currentY += DIVIDER_SPACING + SPACING_BOOK_GAP;
        }
    }

    // ========== 3. 页码 ==========
    {
        TextElementPtr text(new TextElementData());
        text->setText(QString::fromUtf8("%1").arg(pageIndex + 1));
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
// pageType：返回页面类型（1=多图排版）
// ============================================================
int MultiColumnLayout::pageType(int pageIndex) const
{
    Q_UNUSED(pageIndex)
    return 1;
}

// ============================================================
// calculateDescHeight：计算内容简介正文高度
// 从calculateBookItemHeight中提取的descHeight计算逻辑
// ============================================================
qreal MultiColumnLayout::calculateDescHeight(QPainter* painter, const BookPtr& book,
                                              qreal textWidth, qreal labelWidth)
{
    QFont descFont;
    descFont.setPixelSize(FONT_SMALL_SIZE);
    qreal descHeight = 0.0;

    if (!book->description().isEmpty()) {
        QStringList paragraphs = book->description().split(QLatin1Char('\n'));
        bool firstLineDrawn = false;

        for (const QString& paragraph : paragraphs) {
            if (paragraph.isEmpty()) {
                descHeight += DESC_LINE_HEIGHT;
                firstLineDrawn = true;
                continue;
            }

            QTextLayout textLayout(paragraph, descFont, painter->device());
            textLayout.beginLayout();
            qreal firstLineWidth = firstLineDrawn ? textWidth : (textWidth - labelWidth);

            while (true) {
                QTextLine line = textLayout.createLine();
                if (!line.isValid()) break;
                line.setLineWidth(firstLineWidth);
                descHeight += DESC_LINE_HEIGHT;
                firstLineWidth = textWidth;
                firstLineDrawn = true;
            }

            textLayout.endLayout();
        }
    }

    return descHeight;
}

// ============================================================
// generateBookItemElements：生成单本书条目的页面元素
// 对应drawBookItem的元素生成版本
// 位置/尺寸计算与drawBookItem保持一致
// ============================================================
void MultiColumnLayout::generateBookItemElements(
    QList<PageElementPtr>& elements, int& zOrder,
    const BookPtr& book, const QRectF& itemRect, QPainter* tempPainter, bool imageOnRight)
{
    qreal textWidth = itemRect.width() - COVER_WIDTH - SPACING_TEXT_COVER_GAP;
    qreal shadowMarginX = 8.0;
    qreal actualCoverWidth = COVER_WIDTH - shadowMarginX;
    qreal coverHeight = actualCoverWidth * 4.0 / 3.0;

    QRectF coverRect;
    QRectF actualCoverRect;
    qreal textX;

    if (imageOnRight) {
        coverRect = QRectF(itemRect.right() - COVER_WIDTH, itemRect.top(), COVER_WIDTH, itemRect.height());
        actualCoverRect = QRectF(coverRect.left(), coverRect.top(), actualCoverWidth, coverHeight);
        textX = itemRect.left();
    } else {
        coverRect = QRectF(itemRect.left(), itemRect.top(), COVER_WIDTH, itemRect.height());
        actualCoverRect = QRectF(coverRect.left(), coverRect.top(), actualCoverWidth, coverHeight);
        textX = coverRect.right() + SPACING_TEXT_COVER_GAP;
    }

    {
        ImageElementPtr image(new ImageElementData());
        image->setImagePath(book->coverImagePath());
        QSize imgSize = ImageCache::instance().getOriginalSize(book->coverImagePath());
        QRectF adjustedRect = adjustRectToAspect(actualCoverRect, imgSize);
        image->setRect(adjustedRect);
        image->setKeepAspectRatio(!imgSize.isValid());  // 有效时 false（拉伸填充），无效时 true 回退
        image->setScaleFactor(1.0);
        image->setOpacity(1.0);
        image->setName(QString::fromUtf8("封面"));
        image->setZOrder(zOrder++);
        elements.append(PageElementPtr(image.data()));
    }

    qreal textY = itemRect.top();

    QFont bookNameFont;
    bookNameFont.setPixelSize(FONT_BOOKNAME_SIZE);
    bookNameFont.setBold(true);
    QFontMetrics bookNameFm(bookNameFont);

    {
        TextElementPtr text(new TextElementData());
        text->setText(QString::fromUtf8("《") + book->title() + QString::fromUtf8("》"));
        text->setFont(bookNameFont);
        text->setTextColor(COLOR_LABEL_RED);
        text->setAlignment(Qt::AlignLeft | Qt::AlignTop);
        text->setRect(QRectF(textX, textY, textWidth, bookNameFm.height()));
        text->setName(QString::fromUtf8("书名"));
        text->setZOrder(zOrder++);
        elements.append(PageElementPtr(text.data()));
    }
    textY += bookNameFm.height() + SPACING_LINE_GAP;

    QFont smallFont;
    smallFont.setPixelSize(FONT_SMALL_SIZE);
    QFontMetrics smallFm(smallFont);

    {
        TextElementPtr text(new TextElementData());
        text->setText(QString::fromUtf8("书号：") + book->isbn() +
                      QString::fromUtf8("    定价：") + book->price());
        text->setFont(smallFont);
        text->setTextColor(QColor(120, 120, 120));
        text->setAlignment(Qt::AlignLeft | Qt::AlignTop);
        text->setRect(QRectF(textX, textY, textWidth, smallFm.height()));
        text->setName(QString::fromUtf8("书号定价"));
        text->setZOrder(zOrder++);
        elements.append(PageElementPtr(text.data()));
    }
    textY += smallFm.height() + SPACING_LINE_GAP;

    QFont descLabelFont;
    descLabelFont.setPixelSize(FONT_SMALL_SIZE);
    descLabelFont.setBold(true);
    QFontMetrics descLabelFm(descLabelFont);
    QString introLabel = QString::fromUtf8("内容简介：");
    qreal labelWidth = descLabelFm.boundingRect(introLabel).width();

    qreal descHeight = calculateDescHeight(tempPainter, book, textWidth, labelWidth);

    {
        TextElementPtr text(new TextElementData());
        text->setText(introLabel);
        text->setFont(descLabelFont);
        text->setTextColor(COLOR_TEXT_BLACK);
        text->setAlignment(Qt::AlignLeft | Qt::AlignTop);
        text->setRect(QRectF(textX, textY, labelWidth, descLabelFm.height()));
        text->setName(QString::fromUtf8("内容简介标签"));
        text->setZOrder(zOrder++);
        elements.append(PageElementPtr(text.data()));
    }

    {
        TextElementPtr text(new TextElementData());
        text->setText(book->description());
        QFont descFont;
        descFont.setPixelSize(FONT_SMALL_SIZE);
        text->setFont(descFont);
        text->setTextColor(COLOR_TEXT_BLACK);
        text->setAlignment(Qt::AlignLeft | Qt::AlignTop);
        text->setLineHeight(DESC_LINE_HEIGHT);
        // 修复：正文 rect 起点改为 textX + labelWidth，宽度改为 textWidth - labelWidth
        // 与 renderPage 中正文第一行缩进到标签右侧的视觉表现保持一致
        text->setRect(QRectF(textX + labelWidth, textY, textWidth - labelWidth, descHeight));
        text->setName(QString::fromUtf8("内容简介正文"));
        text->setZOrder(zOrder++);
        elements.append(PageElementPtr(text.data()));
    }
}
