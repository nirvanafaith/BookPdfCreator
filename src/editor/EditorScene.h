#ifndef EDITORSCENE_H
#define EDITORSCENE_H

#include <QGraphicsScene>
#include <QMap>
#include <QList>
#include <QPointF>
#include <QRectF>

#include "PageElement.h"
#include "PageData.h"
#include "BaseEditorItem.h"
#include "SelectionHandle.h"
#include "SnapGuide.h"
#include "layout/LayoutEngine.h"

class QUndoStack;
class QGraphicsSceneMouseEvent;
class QKeyEvent;
class SelectionDecorator;
class QTextCursor;
class QGraphicsTextItem;
class GuideLineItem;
class QGraphicsRectItem;

// ============================================================
// EditorScene - 编辑场景
//
// 继承QGraphicsScene，作为PDF书目编辑器的核心场景。
// 负责：
//   - 管理页面元素的可视表示（BaseEditorItem及其子类）
//   - 绘制A4白色背景和边距虚线（drawBackground）
//   - 处理鼠标交互：选择、移动、缩放、旋转、框选
//   - 维护选中装饰器（SelectionDecorator）的显示
//   - 桥接LayoutEngine（生成元素）与PageData（持久化）
//   - 提供撤销栈接口（Task 8集成具体命令）
//
// 交互模型：
//   鼠标事件统一由场景处理（Item不处理PressEvent），
//   场景通过itemAt识别点击目标，据此分派操作。
//   双击事件透传给Item（用于文本编辑模式）。
//
// 选中状态：
//   单选：点击元素
//   多选：Ctrl+点击追加，或框选空白区域
//   选中后显示SelectionDecorator（蓝色虚线边框+8缩放手柄+1旋转手柄）
//
// 坐标系：
//   场景坐标 = 页面坐标（72DPI点单位），原点(0,0)为页面左上角
//   场景矩形 = QRectF(0, 0, A4_WIDTH, A4_HEIGHT)
// ============================================================
class EditorScene : public QGraphicsScene
{
    Q_OBJECT
public:
    EditorScene(QObject* parent = nullptr);

    // 设置排版引擎和数据
    void setLayoutEngine(LayoutEnginePtr engine);
    void setPageData(int pageIndex, const PageDataPtr& pageData);

    // 加载指定页（从LayoutEngine生成元素或从已有PageData恢复）
    void loadPage(int pageIndex);

    // 获取当前页数据（同步场景状态到数据模型）
    PageDataPtr exportPageData() const;

    // 当前页码
    int currentPage() const;

    // 撤销栈
    QUndoStack* undoStack() const;

    // 添加元素到当前页面（通过AddCommand入撤销栈）
    // redo()会自动创建Item并添加到场景；传入空指针时安全跳过。
    void addElement(const PageElementPtr& element);

    // 吸附开关
    bool snapEnabled() const;
    void setSnapEnabled(bool enabled);

    // 更新页面背景Item（白纸+边距虚线），在loadPage和changePaperSize后调用
    void updatePageBackground();

    // 获取页面背景Item（白纸），供图层面板控制可见性
    QGraphicsRectItem* pageBackgroundItem() const { return m_pageBackgroundItem; }

    // 选中状态：返回当前选中的编辑器Item列表
    QList<BaseEditorItem*> selectedEditorItems() const;

    // 更新选中装饰器位置和尺寸（供MainWindow在原地更新元素后调用）
    void updateSelectionDecorator();

signals:
    // 选中元素变化（携带元素数据列表）
    // 注意：此信号重载了QGraphicsScene::selectionChanged()（无参版本），
    // 连接时需用QOverload<QList<PageElementPtr>>::of(&EditorScene::selectionChanged)
    // 或显式指定函数指针类型。
    void selectionChanged(QList<PageElementPtr> elements);

    // 页面数据被修改（移动/缩放/旋转/删除等操作后触发）
    void pageModified();

    // 文本格式化请求（双击编辑文本时选中文本）
    void textFormatRequested(QPointF scenePos);

    // 元素被双击
    void elementDoubleClicked(BaseEditorItem* item);

protected:
    // 绘制页面背景（A4白底+边距虚线）
    void drawBackground(QPainter* painter, const QRectF& rect) override;
    // 绘制前景层（框选半透明矩形等交互反馈）
    void drawForeground(QPainter* painter, const QRectF& rect) override;

    // 鼠标事件
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;
    // 双击事件：透传给Item处理（如文本进入编辑模式），并发射elementDoubleClicked
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event) override;

    // 键盘事件
    void keyPressEvent(QKeyEvent* event) override;

private:
    // 清除当前页所有Item
    void clearPage();
    // 从PageData创建QGraphicsItem
    void createItemsFromPageData(const PageDataPtr& pageData);
    // 创建单个元素对应的Item（根据elementType分派到Text/Image/Shape）
    BaseEditorItem* createItemForElement(const PageElementPtr& element);
    // 变换手柄hit-test：返回手柄位置，无手柄返回-1（强转为HandlePosition的负值）
    HandlePosition handleAt(const QPointF& pos) const;
    // 开始拖拽操作
    void startMove(const QPointF& startPos);
    void startResize(HandlePosition handle, const QPointF& startPos);
    void startRotate(const QPointF& startPos);

    // 吸附辅助
    QPointF snapPoint(const QPointF& pos) const;  // 点吸附到网格
    qreal snapAngle(qreal angle) const;           // 角度吸附到步长

    // 文本编辑模式：选区变化时触发格式化工具栏
    void onEditCursorChanged(const QTextCursor& cursor);

    // 计算缩放后的矩形（根据手柄方向和位移增量）
    // keepAspectRatio=true时以对角/对边为锚点等比缩放（四角默认等比，四边Shift时等比）
    static QRectF computeResizedRect(const QRectF& initial, const QPointF& delta,
                                     HandlePosition handle, bool keepAspectRatio = false);

    LayoutEnginePtr m_engine;
    PageDataPtr m_currentPageData;
    int m_currentPage;
    QUndoStack* m_undoStack;

    // 页面背景Item（白色纸张+边距虚线），zValue=-10000位于最底层
    QGraphicsRectItem* m_pageBackgroundItem;

    // 选中装饰器
    SelectionDecorator* m_selectionDecorator;

    // 拖拽状态
    enum DragMode { None, Move, Resize, Rotate, RubberBand };
    DragMode m_dragMode;
    QPointF m_dragStartPos;
    HandlePosition m_resizeHandle;
    QMap<BaseEditorItem*, QPointF> m_initialPositions;   // 拖拽开始时各元素位置
    QMap<BaseEditorItem*, QRectF> m_initialRects;        // 拖拽开始时各元素矩形
    QMap<BaseEditorItem*, qreal> m_initialRotations;     // 拖拽开始时各元素旋转角度
    QRectF m_rubberBandRect;                              // 框选矩形

    // 吸附
    bool m_snapEnabled;
    SnapGuide m_snapGuide;  // 智能吸附管理器（移动时提供对齐参考线吸附）

    // 吸附参考线（拖拽时动态创建/移除）
    QList<GuideLineItem*> m_activeGuideLines;

    // 文本编辑模式跟踪
    QGraphicsTextItem* m_editingTextItem;  // 当前正在编辑的QGraphicsTextItem（监听选区变化）

    // 剪贴板：存储复制/剪切的元素数据
    QList<PageElementPtr> m_clipboard;
};

#endif // EDITORSCENE_H
