#ifndef PDFEXPORTER_H
#define PDFEXPORTER_H

#include <QObject>
#include <functional>
#include "LayoutEngine.h"
#include "editor/PageData.h"

class QString;
class QPrinter;
class QPainter;

class PdfExporter : public QObject
{
    Q_OBJECT

public:
    explicit PdfExporter(QObject* parent = nullptr);

    // ---- 原有API（向后兼容）----
    void setLayoutEngine(LayoutEnginePtr engine);
    // 使用已设置的排版引擎导出PDF（renderPage模式）
    bool exportToFile(const QString& filePath, QString* errorMessage = nullptr);

    // ---- 新增：从编辑后的PageData列表导出 ----
    // 通过ElementRenderer渲染每个页面的元素列表
    // 如果pages为空且已设置排版引擎，回退到renderPage模式
    bool exportToFile(const QString& filePath, const QList<PageDataPtr>& pages,
                      QString* errorMessage = nullptr);

    // ---- 新增：进度回调 ----
    // 每完成一页渲染后调用（与exportProgress信号同时触发）
    using ProgressCallback = std::function<void(int current, int total)>;
    void setProgressCallback(ProgressCallback callback);

    // ---- 新增：取消导出 ----
    // 在导出过程中调用，循环检测到取消标志后中止导出
    void cancel();

signals:
    void exportProgress(int currentPage, int totalPages);
    void exportFinished(bool success, const QString& message);

private:
    // 初始化打印机和绘图器（A4页面、零边距、抗锯齿）
    // 成功返回true，失败时写入errorMessage
    bool initPrinter(const QString& filePath, QPrinter* printer, QPainter* painter,
                     QString* errorMessage);

    LayoutEnginePtr m_engine;
    bool m_cancelled = false;
    ProgressCallback m_progressCallback;
};

#endif // PDFEXPORTER_H
