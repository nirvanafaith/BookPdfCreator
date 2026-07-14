#ifndef EDITORVIEW_H
#define EDITORVIEW_H

#include <QGraphicsView>
#include <QPointF>
#include <QPoint>

#include "layout/LayoutEngine.h"

class EditorScene;

// ============================================================
// EditorView - 编辑视图
//
// 继承QGraphicsView，作为PDF书目编辑器的可视化视图。
// 负责：
//   - 承载EditorScene并显示A4页面
//   - Ctrl+滚轮缩放（以鼠标位置为缩放中心）
//   - 空格+左键拖拽平移（PS风格手型光标）
//   - 缩放比例显示与设置（百分比，100=100%）
//   - 兼容原有PdfPreviewWidget的关键API
//     （setLayoutEngine/setCurrentPage/currentPage/pageCount）
//
// 交互模型：
//   - 缩放：按住Ctrl滚动鼠标滚轮，放大1.1倍/缩小0.9倍，范围10%~500%
//   - 平移：按住空格键后按住左键拖动；松开空格或左键结束
//   - 框选/编辑：交给EditorScene处理（setDragMode(NoDrag)）
//
// 坐标系：
//   视图坐标 = viewport像素坐标
//   场景坐标 = 页面坐标（72DPI点单位），原点(0,0)为页面左上角
//
// Qt 5.12 兼容：
//   滚轮事件使用 angleDelta()，鼠标位置使用 pos()，
//   不使用 Qt6 的 position() API。
// ============================================================
class EditorView : public QGraphicsView
{
    Q_OBJECT
public:
    EditorView(QWidget* parent = nullptr);

    // ---- 兼容原有 PdfPreviewWidget 的 API ----
    void setLayoutEngine(LayoutEnginePtr engine);
    void setCurrentPage(int page);
    int currentPage() const;
    int pageCount() const;

    // ---- 缩放控制 ----
    qreal zoomLevel() const;       // 百分比，100=100%
    void setZoomLevel(qreal percent);
    void fitToWindow();            // 适配窗口
    void resetZoom();              // 重置为100%

    // ---- 场景访问 ----
    EditorScene* editorScene() const;

signals:
    // 当前页变化（page从0开始）
    void currentPageChanged(int page);
    // 缩放比例变化（percent为百分比，100=100%）
    void zoomChanged(qreal percent);
    // 页面数据被修改（转发自EditorScene）
    void pageModified();

protected:
    void wheelEvent(QWheelEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    // 拖放接收：从素材树拖入图片/文本/TXT文件到画布
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dropEvent(QDropEvent* event) override;

private:
    EditorScene* m_scene;     // 编辑场景（父子关系自动管理生命周期）
    LayoutEnginePtr m_engine; // 排版引擎引用（用于pageCount查询）
    bool m_spacePressed;      // 空格键按下状态
    bool m_isPanning;         // 正在平移
    QPoint m_lastPanPos;      // 上次平移位置（视图坐标）
    qreal m_zoomLevel;        // 缩放百分比（100=100%）

    // 以指定视图坐标为中心进行缩放
    void updateZoom(qreal factor, const QPointF& center);

    // 拖放元素创建辅助方法（由dropEvent按素材类型分派调用）
    void createImageElement(EditorScene* scene, const QString& imagePath, const QPointF& pos);
    void createTextElement(EditorScene* scene, const QString& text, const QString& label, const QPointF& pos);
    void createTextFromTxtFile(EditorScene* scene, const QString& filePath, const QPointF& pos);
};

#endif // EDITORVIEW_H
