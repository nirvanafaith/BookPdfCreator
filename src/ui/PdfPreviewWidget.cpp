#include "PdfPreviewWidget.h"
#include "LayoutEngine.h"
#include "LayoutConstants.h"

#include <QPainter>
#include <QPaintEvent>
#include <QResizeEvent>
#include <QWheelEvent>
#include <QImage>
#include <QFont>
#include <QFontMetrics>

PdfPreviewWidget::PdfPreviewWidget(QWidget *parent)
    : QWidget(parent)
    , m_currentPage(0)
    , m_zoom(1.0)
    , m_highDpiScale(2.0)
{
    setBackgroundRole(QPalette::Light);
    setAutoFillBackground(true);
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
}

void PdfPreviewWidget::setLayoutEngine(LayoutEnginePtr engine)
{
    m_engine = engine;
    m_currentPage = 0;
    if (m_engine && m_engine->isDirty()) {
        m_engine->calculateLayout();
    }
    zoomToFit();
    update();
    emit pageChanged(m_currentPage);
}

void PdfPreviewWidget::setCurrentPage(int page)
{
    int maxPage = pageCount() - 1;
    int newPage = qBound(0, page, qMax(0, maxPage));
    if (newPage != m_currentPage) {
        m_currentPage = newPage;
        update();
        emit pageChanged(m_currentPage);
    }
}

int PdfPreviewWidget::currentPage() const
{
    return m_currentPage;
}

int PdfPreviewWidget::pageCount() const
{
    if (!m_engine) {
        return 0;
    }
    return m_engine->pageCount();
}

void PdfPreviewWidget::setZoom(qreal zoom)
{
    qreal newZoom = qBound(0.1, zoom, 5.0);
    if (!qFuzzyCompare(newZoom, m_zoom)) {
        m_zoom = newZoom;
        update();
        emit zoomChanged(m_zoom);
    }
}

qreal PdfPreviewWidget::zoom() const
{
    return m_zoom;
}

void PdfPreviewWidget::zoomToFit()
{
    if (width() <= 0 || height() <= 0) {
        m_zoom = 1.0;
        return;
    }

    const int margin = 40;
    qreal availableWidth = width() - margin * 2;
    qreal availableHeight = height() - margin * 2;

    if (availableWidth <= 0 || availableHeight <= 0) {
        m_zoom = 1.0;
        return;
    }

    qreal zoomX = availableWidth / A4_WIDTH;
    qreal zoomY = availableHeight / A4_HEIGHT;
    m_zoom = qMin(zoomX, zoomY);
    emit zoomChanged(m_zoom);
}

void PdfPreviewWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter widgetPainter(this);
    widgetPainter.setRenderHint(QPainter::Antialiasing, true);
    widgetPainter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    widgetPainter.fillRect(rect(), QColor(230, 230, 230));

    int totalPages = pageCount();
    if (totalPages <= 0 || !m_engine) {
        QFont font;
        font.setPointSize(14);
        widgetPainter.setFont(font);
        widgetPainter.setPen(QColor(120, 120, 120));
        QString text = QStringLiteral("暂无预览内容");
        QFontMetrics fm(font);
#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
        int textWidth = fm.horizontalAdvance(text);
#else
        int textWidth = fm.width(text);
#endif
        int textHeight = fm.height();
        widgetPainter.drawText((width() - textWidth) / 2, (height() - textHeight) / 2 + fm.ascent(), text);
        return;
    }

    qreal scaledWidth = A4_WIDTH * m_zoom;
    qreal scaledHeight = A4_HEIGHT * m_zoom;
    int x = (width() - static_cast<int>(scaledWidth)) / 2;
    int y = (height() - static_cast<int>(scaledHeight)) / 2;

    QRectF pageRect(x, y, scaledWidth, scaledHeight);

    int shadowOffset = 8;
    widgetPainter.setPen(Qt::NoPen);
    widgetPainter.setBrush(QColor(0, 0, 0, 60));
    widgetPainter.drawRoundedRect(
        pageRect.adjusted(-shadowOffset / 2, -shadowOffset / 2, shadowOffset, shadowOffset),
        4, 4
    );

    widgetPainter.setBrush(Qt::white);
    widgetPainter.setPen(QPen(QColor(180, 180, 180), 1));
    widgetPainter.drawRoundedRect(pageRect, 2, 2);

    int imageWidth = static_cast<int>(A4_WIDTH * m_highDpiScale);
    int imageHeight = static_cast<int>(A4_HEIGHT * m_highDpiScale);
    QImage pageImage(imageWidth, imageHeight, QImage::Format_RGB32);
    pageImage.fill(Qt::white);

    QPainter imagePainter(&pageImage);
    imagePainter.setRenderHint(QPainter::Antialiasing, true);
    imagePainter.setRenderHint(QPainter::TextAntialiasing, true);
    imagePainter.scale(m_highDpiScale, m_highDpiScale);

    m_engine->renderPage(&imagePainter, m_currentPage, QRectF(0, 0, A4_WIDTH, A4_HEIGHT));
    imagePainter.end();

    widgetPainter.drawImage(pageRect, pageImage);

    QFont pageFont;
    pageFont.setPointSize(10);
    widgetPainter.setFont(pageFont);
    widgetPainter.setPen(QColor(80, 80, 80));
    QString pageText = QStringLiteral("第%1页 / 共%2页  缩放:%3%")
        .arg(m_currentPage + 1)
        .arg(totalPages)
        .arg(static_cast<int>(m_zoom * 100));
    QFontMetrics fm(pageFont);
#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
    int textWidth = fm.horizontalAdvance(pageText);
#else
    int textWidth = fm.width(pageText);
#endif
    widgetPainter.drawText(width() - textWidth - 20, height() - 10, pageText);
}

void PdfPreviewWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    zoomToFit();
}

void PdfPreviewWidget::wheelEvent(QWheelEvent *event)
{
    if (event->modifiers() & Qt::ControlModifier) {
        QPoint numDegrees = event->angleDelta() / 8;
        qreal zoomFactor = 1.0;
        if (numDegrees.y() > 0) {
            zoomFactor = 1.15;
        } else if (numDegrees.y() < 0) {
            zoomFactor = 1.0 / 1.15;
        }
        qreal newZoom = m_zoom * zoomFactor;
        setZoom(newZoom);
        event->accept();
    } else {
        int delta = event->angleDelta().y();
        if (delta > 0) {
            setCurrentPage(m_currentPage - 1);
        } else if (delta < 0) {
            setCurrentPage(m_currentPage + 1);
        }
        event->accept();
    }
}
