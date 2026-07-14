#include "PdfExporter.h"
#include <QPrinter>
#include <QPainter>
#include <QPageSize>
#include <QPageLayout>
#include <QMarginsF>
#include <QString>
#include "LayoutConstants.h"
#include "editor/ElementRenderer.h"

PdfExporter::PdfExporter(QObject* parent)
    : QObject(parent)
{
}

void PdfExporter::setLayoutEngine(LayoutEnginePtr engine)
{
    m_engine = engine;
}

void PdfExporter::setProgressCallback(ProgressCallback callback)
{
    m_progressCallback = callback;
}

void PdfExporter::cancel()
{
    m_cancelled = true;
}

bool PdfExporter::initPrinter(const QString& filePath, QPrinter* printer, QPainter* painter,
                              QString* errorMessage)
{
    printer->setOutputFormat(QPrinter::PdfFormat);
    printer->setOutputFileName(filePath);

    // 使用 PageSizeManager 的动态纸张尺寸（毫米）设置页面大小
    const PaperSize paper = PageSizeManager::instance().currentPaper();
    QPageSize pageSize(QSizeF(paper.widthMm, paper.heightMm), QPageSize::Millimeter);
    printer->setPageSize(pageSize);
    printer->setPageMargins(QMarginsF(0, 0, 0, 0), QPageLayout::Point);

    if (!painter->begin(printer)) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("无法初始化绘图器");
        }
        return false;
    }

    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setRenderHint(QPainter::TextAntialiasing, true);
    painter->setRenderHint(QPainter::SmoothPixmapTransform, true);
#if QT_VERSION >= QT_VERSION_CHECK(5, 13, 0)
    painter->setRenderHint(QPainter::LosslessImageRendering, true);
#endif
    return true;
}

bool PdfExporter::exportToFile(const QString& filePath, QString* errorMessage)
{
    // 重置取消标志
    m_cancelled = false;

    if (!m_engine) {
        QString msg = QStringLiteral("未设置排版引擎");
        if (errorMessage) *errorMessage = msg;
        emit exportFinished(false, msg);
        return false;
    }

    if (filePath.isEmpty()) {
        QString msg = QStringLiteral("文件路径不能为空");
        if (errorMessage) *errorMessage = msg;
        emit exportFinished(false, msg);
        return false;
    }

    QPrinter printer;
    QPainter painter;
    if (!initPrinter(filePath, &printer, &painter, errorMessage)) {
        QString msg = QStringLiteral("无法初始化绘图器");
        emit exportFinished(false, msg);
        return false;
    }

    const int totalPages = m_engine->pageCount();
    bool success = true;
    QString message;

    for (int i = 0; i < totalPages; ++i) {
        // 检查取消标志
        if (m_cancelled) {
            success = false;
            message = QStringLiteral("导出已取消");
            break;
        }

        // 设置裁剪区域为纸张边界，纸张外内容不导出
        painter.save();
        painter.setClipRect(QRectF(0, 0, PageSizeManager::instance().pageWidth(),
                                         PageSizeManager::instance().pageHeight()));

        m_engine->renderPage(&painter, i,
            QRectF(0, 0, PageSizeManager::instance().pageWidth(),
                         PageSizeManager::instance().pageHeight()));

        painter.restore();

        // 通知进度（回调+信号）
        if (m_progressCallback) m_progressCallback(i + 1, totalPages);
        emit exportProgress(i + 1, totalPages);

        if (i < totalPages - 1) {
            if (!printer.newPage()) {
                success = false;
                message = QStringLiteral("新建页面失败");
                break;
            }
        }
    }

    painter.end();

    if (success) {
        message = QStringLiteral("PDF导出成功");
    } else {
        if (errorMessage) *errorMessage = message;
    }

    emit exportFinished(success, message);
    return success;
}

bool PdfExporter::exportToFile(const QString& filePath, const QList<PageDataPtr>& pages,
                               QString* errorMessage)
{
    // 没有编辑数据时，回退到原有renderPage模式
    if (pages.isEmpty()) {
        return exportToFile(filePath, errorMessage);
    }

    // 重置取消标志
    m_cancelled = false;

    if (filePath.isEmpty()) {
        QString msg = QStringLiteral("文件路径不能为空");
        if (errorMessage) *errorMessage = msg;
        emit exportFinished(false, msg);
        return false;
    }

    QPrinter printer;
    QPainter painter;
    if (!initPrinter(filePath, &printer, &painter, errorMessage)) {
        QString msg = QStringLiteral("无法初始化绘图器");
        emit exportFinished(false, msg);
        return false;
    }

    const int totalPages = pages.size();
    bool success = true;
    QString message;

    for (int i = 0; i < totalPages; ++i) {
        // 检查取消标志
        if (m_cancelled) {
            success = false;
            message = QStringLiteral("导出已取消");
            break;
        }

        // 通过ElementRenderer渲染编辑后的页面元素
        // 不可见元素由ElementRenderer::renderPage内部过滤
        ElementRenderer::renderPage(&painter, pages.at(i));

        // 通知进度（回调+信号）
        if (m_progressCallback) m_progressCallback(i + 1, totalPages);
        emit exportProgress(i + 1, totalPages);

        if (i < totalPages - 1) {
            if (!printer.newPage()) {
                success = false;
                message = QStringLiteral("新建页面失败");
                break;
            }
        }
    }

    painter.end();

    if (success) {
        message = QStringLiteral("PDF导出成功");
    } else {
        if (errorMessage) *errorMessage = message;
    }

    emit exportFinished(success, message);
    return success;
}
