#include "EditorView.h"
#include "EditorScene.h"
#include "PageElement.h"
#include "ui/AssetTreeWidget.h"   // AssetMimeData, AssetType
#include "layout/LayoutConstants.h"

#include <QWheelEvent>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QResizeEvent>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QTransform>
#include <QScrollBar>
#include <QPainter>
#include <QColor>
#include <QRectF>
#include <QImageReader>
#include <QFontMetrics>
#include <QFile>
#include <QUuid>
#include <QFileInfo>
#include <QtGlobal>
#include <QtMath>

// ============================================================
// 常量定义
// ============================================================

// 缩放范围（百分比）
static const qreal MIN_ZOOM = 10.0;    // 最小10%
static const qreal MAX_ZOOM = 500.0;   // 最大500%

// 滚轮缩放因子
static const qreal ZOOM_IN_FACTOR = 1.1;   // 放大1.1倍
static const qreal ZOOM_OUT_FACTOR = 0.9;  // 缩小0.9倍

// ============================================================
// 构造函数
//
// 创建EditorScene并设置为当前场景，配置渲染提示、拖拽模式、
// 滚动条策略和灰色背景（PS风格）。连接场景的pageModified信号
// 转发给视图层。
// ============================================================
EditorView::EditorView(QWidget* parent)
    : QGraphicsView(parent)
    , m_scene(new EditorScene(this))
    , m_spacePressed(false)
    , m_isPanning(false)
    , m_zoomLevel(100.0)
{
    // 设置场景（场景矩形由EditorScene构造函数设置为A4尺寸）
    setScene(m_scene);

    // 渲染提示：抗锯齿 + 文本抗锯齿 + 平滑位图变换
    setRenderHint(QPainter::Antialiasing);
    setRenderHint(QPainter::TextAntialiasing);
    setRenderHint(QPainter::SmoothPixmapTransform);

    // 拖拽模式：不使用内置RubberBand，选中/编辑交互由EditorScene处理
    setDragMode(QGraphicsView::NoDrag);

    // 性能优化：使用边界矩形更新模式，平衡性能与拖拽流畅度
    // MinimalViewportUpdate在快速拖拽时可能导致重绘延迟
    setViewportUpdateMode(QGraphicsView::BoundingRectViewportUpdate);

    // 不缓存背景：CacheBackground会缓存view的灰色背景，
    // 覆盖Scene::drawBackground绘制的白色纸张矩形，导致白纸不显示。
    // 背景仅是纯色填充+矩形，绘制开销极小，无需缓存。
    setCacheMode(QGraphicsView::CacheNone);

    // 滚动条策略：按需显示
    setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    // 浅灰色背景，清晰衬托白色纸张
    setBackgroundBrush(QColor(180, 180, 180));

    // 焦点策略：接收键盘事件（空格平移）
    setFocusPolicy(Qt::StrongFocus);

    // 启用拖放接收：允许从素材树拖入图片/文本/TXT文件
    // QGraphicsView基于QAbstractScrollArea，实际接收拖拽事件的是viewport子部件，
    // 必须同时启用viewport的drops，事件才会经事件过滤器转发到本类的drag*/drop处理函数。
    setAcceptDrops(true);
    viewport()->setAcceptDrops(true);

    // 转发场景的pageModified信号
    connect(m_scene, &EditorScene::pageModified, this, &EditorView::pageModified);

    // 应用初始缩放（100%）
    QTransform transform;
    transform.scale(m_zoomLevel / 100.0, m_zoomLevel / 100.0);
    setTransform(transform);
}

// ============================================================
// setLayoutEngine - 设置排版引擎（兼容PdfPreviewWidget API）
//
// 同时保存引擎引用（用于pageCount查询），并转发给场景。
// ============================================================
void EditorView::setLayoutEngine(LayoutEnginePtr engine)
{
    m_engine = engine;
    m_scene->setLayoutEngine(engine);
}

// ============================================================
// setCurrentPage - 设置当前页（兼容PdfPreviewWidget API）
//
// 对页码进行边界约束后转发给场景的loadPage，
// 并发射currentPageChanged信号。
// ============================================================
void EditorView::setCurrentPage(int page)
{
    int maxPage = pageCount() - 1;
    int newPage = qBound(0, page, qMax(0, maxPage));
    if (newPage != currentPage()) {
        m_scene->loadPage(newPage);
        emit currentPageChanged(newPage);
    }
}

// ============================================================
// currentPage - 返回当前页码（转发给场景）
// ============================================================
int EditorView::currentPage() const
{
    return m_scene->currentPage();
}

// ============================================================
// pageCount - 返回总页数（转发给排版引擎）
// ============================================================
int EditorView::pageCount() const
{
    if (m_engine) {
        return m_engine->pageCount();
    }
    return 0;
}

// ============================================================
// zoomLevel - 返回当前缩放百分比（100=100%）
// ============================================================
qreal EditorView::zoomLevel() const
{
    return m_zoomLevel;
}

// ============================================================
// setZoomLevel - 设置缩放百分比（以视口中心为缩放中心）
// ============================================================
void EditorView::setZoomLevel(qreal percent)
{
    percent = qBound(MIN_ZOOM, percent, MAX_ZOOM);
    if (qAbs(m_zoomLevel - percent) < 0.01) {
        return;
    }

    // 记录缩放前视口中心对应的场景坐标
    QPointF sceneCenterBefore = mapToScene(viewport()->rect().center());

    m_zoomLevel = percent;
    QTransform transform;
    transform.scale(m_zoomLevel / 100.0, m_zoomLevel / 100.0);
    setTransform(transform);

    // 保持视口中心对应的场景点不变
    centerOn(sceneCenterBefore);

    emit zoomChanged(m_zoomLevel);
}

// ============================================================
// fitToWindow - 适配窗口
//
// 计算适合视口的缩放比例（取宽高比的较小值），
// 并将页面居中显示。页面尺寸从 PageSizeManager 动态获取，
// 支持纸张切换后正确适配。
// ============================================================
void EditorView::fitToWindow()
{
    // 优先使用场景矩形（已随纸张切换更新），无场景时回退到 PageSizeManager
    QRectF pageRect;
    if (scene() && !scene()->sceneRect().isNull()) {
        pageRect = scene()->sceneRect();
    } else {
        pageRect = QRectF(0, 0, PageSizeManager::instance().pageWidth(),
                                PageSizeManager::instance().pageHeight());
    }

    QRectF viewRect = viewport()->rect();

    // 避免除零（视口尚未初始化时宽度/高度可能为0）
    if (viewRect.width() <= 0 || viewRect.height() <= 0 ||
        pageRect.width() <= 0 || pageRect.height() <= 0) {
        return;
    }

    qreal xRatio = viewRect.width() / pageRect.width();
    qreal yRatio = viewRect.height() / pageRect.height();
    qreal ratio = qMin(xRatio, yRatio);

    m_zoomLevel = qBound(MIN_ZOOM, ratio * 100.0, MAX_ZOOM);

    QTransform transform;
    transform.scale(m_zoomLevel / 100.0, m_zoomLevel / 100.0);
    setTransform(transform);

    // 居中显示页面
    centerOn(pageRect.center());

    emit zoomChanged(m_zoomLevel);
}

// ============================================================
// resetZoom - 重置缩放为100%
// ============================================================
void EditorView::resetZoom()
{
    setZoomLevel(100.0);
}

// ============================================================
// editorScene - 返回编辑场景
// ============================================================
EditorScene* EditorView::editorScene() const
{
    return m_scene;
}

// ============================================================
// updateZoom - 以指定视图坐标为中心进行缩放
//
// 算法：
//   1. 记录缩放前鼠标位置对应的场景坐标 scenePosBefore
//   2. 应用新缩放矩阵
//   3. 计算缩放后同一鼠标位置对应的场景坐标 scenePosAfter
//   4. 平移视图，使鼠标位置对应的场景点保持不变
// ============================================================
void EditorView::updateZoom(qreal factor, const QPointF& center)
{
    // center是视图坐标，记录缩放前鼠标对应的场景坐标
    QPointF scenePosBefore = mapToScene(center.toPoint());

    m_zoomLevel *= factor;
    m_zoomLevel = qBound(MIN_ZOOM, m_zoomLevel, MAX_ZOOM);

    QTransform transform;
    transform.scale(m_zoomLevel / 100.0, m_zoomLevel / 100.0);
    setTransform(transform);

    // 缩放后同一鼠标位置对应的场景坐标发生变化
    QPointF scenePosAfter = mapToScene(center.toPoint());
    QPointF diff = scenePosAfter - scenePosBefore;

    // 平移视图，使鼠标位置对应的场景点保持不变
    centerOn(mapToScene(viewport()->rect().center()) - diff);

    emit zoomChanged(m_zoomLevel);
}

// ============================================================
// wheelEvent - 滚轮事件
//
// Ctrl+滚轮：缩放（以鼠标位置为中心）
// 否则：正常滚动（垂直/水平）
// ============================================================
void EditorView::wheelEvent(QWheelEvent* event)
{
    if (event->modifiers() & Qt::ControlModifier) {
        // Ctrl+滚轮：缩放
        QPoint numDegrees = event->angleDelta() / 8;
        qreal factor = 1.0;
        if (numDegrees.y() > 0) {
            factor = ZOOM_IN_FACTOR;   // 放大1.1倍
        } else if (numDegrees.y() < 0) {
            factor = ZOOM_OUT_FACTOR;  // 缩小0.9倍
        }
        if (factor != 1.0) {
            // 以鼠标位置为中心缩放（Qt5使用event->pos()）
            updateZoom(factor, QPointF(event->pos()));
        }
        event->accept();
    } else {
        // 正常滚动
        QGraphicsView::wheelEvent(event);
    }
}

// ============================================================
// mousePressEvent - 鼠标按下
//
// 空格+左键：进入平移模式
// 否则：交给基类处理（场景交互）
// ============================================================
void EditorView::mousePressEvent(QMouseEvent* event)
{
    if (m_spacePressed && event->button() == Qt::LeftButton) {
        m_isPanning = true;
        m_lastPanPos = event->pos();
        setCursor(Qt::ClosedHandCursor);
        event->accept();
        return;
    }
    QGraphicsView::mousePressEvent(event);
}

// ============================================================
// mouseReleaseEvent - 鼠标释放
//
// 平移中释放左键：结束平移，恢复手型光标
// ============================================================
void EditorView::mouseReleaseEvent(QMouseEvent* event)
{
    if (m_isPanning && event->button() == Qt::LeftButton) {
        m_isPanning = false;
        if (m_spacePressed) {
            setCursor(Qt::OpenHandCursor);
        } else {
            unsetCursor();
        }
        event->accept();
        return;
    }
    QGraphicsView::mouseReleaseEvent(event);
}

// ============================================================
// mouseMoveEvent - 鼠标移动
//
// 平移模式下：通过调整滚动条值实现视图平移
// 否则：交给基类处理
// ============================================================
void EditorView::mouseMoveEvent(QMouseEvent* event)
{
    if (m_isPanning) {
        QPoint delta = event->pos() - m_lastPanPos;
        horizontalScrollBar()->setValue(horizontalScrollBar()->value() - delta.x());
        verticalScrollBar()->setValue(verticalScrollBar()->value() - delta.y());
        m_lastPanPos = event->pos();
        event->accept();
        return;
    }
    QGraphicsView::mouseMoveEvent(event);
}

// ============================================================
// keyPressEvent - 键盘按下
//
// 空格键：进入平移准备状态（OpenHandCursor）
// 注意忽略自动重复事件，避免状态抖动。
// ============================================================
void EditorView::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Space && !event->isAutoRepeat()) {
        m_spacePressed = true;
        setCursor(Qt::OpenHandCursor);
        event->accept();
        return;
    }
    QGraphicsView::keyPressEvent(event);
}

// ============================================================
// keyReleaseEvent - 键盘释放
//
// 空格键：退出平移准备状态，清除光标
// ============================================================
void EditorView::keyReleaseEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Space && !event->isAutoRepeat()) {
        m_spacePressed = false;
        m_isPanning = false;
        unsetCursor();
        event->accept();
        return;
    }
    QGraphicsView::keyReleaseEvent(event);
}

// ============================================================
// resizeEvent - 视图大小变化
//
// 调用基类实现，保持用户当前缩放与位置（不自动适配）。
// ============================================================
void EditorView::resizeEvent(QResizeEvent* event)
{
    QGraphicsView::resizeEvent(event);
}

// ============================================================
// 拖放事件 - 接收来自素材树的拖拽
//
// dragEnterEvent / dragMoveEvent：仅接受携带
// "application/x-bookpdf-asset" 自定义MIME类型的拖拽。
// dropEvent：根据 AssetMimeData.assetType 分派到对应的
// create* 辅助方法，在落点位置创建页面元素并经 EditorScene
// 入撤销栈。
// ============================================================
void EditorView::dragEnterEvent(QDragEnterEvent* event)
{
    // 检查是否是素材拖拽
    if (event->mimeData()->hasFormat("application/x-bookpdf-asset")) {
        event->acceptProposedAction();
    } else {
        event->ignore();
    }
}

void EditorView::dragMoveEvent(QDragMoveEvent* event)
{
    if (event->mimeData()->hasFormat("application/x-bookpdf-asset")) {
        event->acceptProposedAction();
    } else {
        event->ignore();
    }
}

void EditorView::dropEvent(QDropEvent* event)
{
    if (!event->mimeData()->hasFormat("application/x-bookpdf-asset")) {
        event->ignore();
        return;
    }

    // 获取MIME数据（同进程拖拽，AssetMimeData对象直接传递）
    const AssetMimeData* mimeData = qobject_cast<const AssetMimeData*>(event->mimeData());
    if (!mimeData) {
        event->ignore();
        return;
    }

    // 计算放置位置（view视口坐标转scene页面坐标）
    QPointF dropPos = mapToScene(event->pos());

    EditorScene* scene = editorScene();
    if (!scene) {
        event->ignore();
        return;
    }

    switch (mimeData->assetType) {
    case ImageAsset:
        createImageElement(scene, mimeData->imagePath, dropPos);
        break;
    case TextAsset:
        createTextElement(scene, mimeData->textContent, mimeData->textLabel, dropPos);
        break;
    case TxtFileAsset:
        createTextFromTxtFile(scene, mimeData->txtFilePath, dropPos);
        break;
    }

    event->acceptProposedAction();
}

// ============================================================
// createImageElement - 创建图片元素
//
// 读取图片原始像素尺寸，按 72/96 比例换算为点尺寸（屏幕96DPI
// 假设），在落点位置创建 ImageElementData 并加入场景。
// ============================================================
void EditorView::createImageElement(EditorScene* scene, const QString& imagePath, const QPointF& pos)
{
    if (imagePath.isEmpty()) return;

    // 获取图片原始尺寸（像素）
    QImageReader reader(imagePath);
    QSize imageSize = reader.size();
    if (!imageSize.isValid()) {
        // 无法读取尺寸，使用默认
        imageSize = QSize(200, 200);
    }

    // 转换为点尺寸（1像素 = 72/96 点，假设96DPI屏幕图片）
    qreal scale = 72.0 / 96.0;
    qreal widthPt = imageSize.width() * scale;
    qreal heightPt = imageSize.height() * scale;

    // 创建图片元素（ImageElementData为具体类，可通过具体类型指针修改）
    ImageElementPtr imageElem(new ImageElementData());
    imageElem->setImagePath(imagePath);
    imageElem->setRect(QRectF(pos.x(), pos.y(), widthPt, heightPt));
    imageElem->setScaleFactor(1.0);
    imageElem->setOpacity(1.0);
    imageElem->setKeepAspectRatio(true);
    imageElem->setName(QFileInfo(imagePath).baseName());
    imageElem->setId(QUuid::createUuid().toString());

    // 通过EditorScene添加（入撤销栈）
    // .data()返回原始指针，PageElementPtr构造接管所有权（引用计数+1，
    // imageElem局部对象析构时-1，最终由scene持有）。新建元素引用计数为1，
    // detach()为无操作，安全。
    scene->addElement(PageElementPtr(imageElem.data()));
}

// ============================================================
// createTextElement - 创建文本元素
//
// 以固定宽度200点、宋体12pt排版，使用 QFontMetrics 计算文本
// 换行后的高度，在落点位置创建 TextElementData 并加入场景。
// ============================================================
void EditorView::createTextElement(EditorScene* scene, const QString& text,
                                    const QString& label, const QPointF& pos)
{
    if (text.isEmpty()) return;

    // 固定宽度200点
    qreal width = 200.0;

    // 计算高度：使用QFontMetrics
    QFont font(QString::fromUtf8("SimSun"), 12);  // 默认宋体12pt
    QFontMetrics fm(font);
    QRectF textRect = fm.boundingRect(QRect(0, 0, int(width), 10000),
                                       Qt::TextWordWrap | Qt::AlignLeft | Qt::AlignTop,
                                       text);
    qreal height = textRect.height() + 10;  // 上下各5点边距

    // 创建文本元素
    TextElementPtr textElem(new TextElementData());
    textElem->setText(text);
    textElem->setFont(font);
    textElem->setTextColor(QColor(0, 0, 0));  // 黑色
    textElem->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    textElem->setRect(QRectF(pos.x(), pos.y(), width, height));
    textElem->setName(label);
    textElem->setId(QUuid::createUuid().toString());

    // 通过EditorScene添加（入撤销栈）
    scene->addElement(PageElementPtr(textElem.data()));
}

// ============================================================
// createTextFromTxtFile - 从TXT文件创建文本元素
//
// 读取TXT文件全部内容（UTF-8），过长文本截断至1000字符并追加
// 省略号，文件名作为元素标签，复用 createTextElement 完成创建。
// ============================================================
void EditorView::createTextFromTxtFile(EditorScene* scene, const QString& filePath, const QPointF& pos)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return;
    }

    QString content = QString::fromUtf8(file.readAll());
    file.close();

    // 截断过长的文本（最多1000字符）
    if (content.length() > 1000) {
        content = content.left(1000) + QString::fromUtf8("...");
    }

    QFileInfo info(filePath);
    createTextElement(scene, content, info.baseName(), pos);
}
