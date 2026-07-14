#include "EditorScene.h"

#include "TextEditorItem.h"
#include "ImageEditorItem.h"
#include "ShapeEditorItem.h"
#include "SelectionDecorator.h"
#include "Commands.h"
#include "GuideLineItem.h"
#include "layout/LayoutConstants.h"

#include <QGraphicsRectItem>
#include <QPainter>
#include <QPen>

#include <QGraphicsSceneMouseEvent>
#include <QKeyEvent>
#include <QPainter>
#include <QPen>
#include <QBrush>
#include <QColor>
#include <QUndoStack>
#include <QPainterPath>
#include <QPair>
#include <QTransform>
#include <QLineF>
#include <QtMath>
#include <QGraphicsTextItem>
#include <QTextDocument>
#include <QTextCursor>
#include <QTextBlock>
#include <QTextLayout>
#include <QTextLine>
#include <QUuid>

// ============================================================
// 常量定义
// ============================================================

// 吸附网格间距（点单位）
static const qreal SNAP_GRID = 5.0;
// 旋转吸附角度步长（度）
static const qreal SNAP_ANGLE_STEP = 15.0;
// 元素最小尺寸（防止缩放到负值或过小）
static const qreal MIN_ELEMENT_SIZE = 10.0;
// 装饰器Z值（置于所有元素之上）
static const qreal DECORATOR_Z = 9999.0;

// 无效手柄位置标记（用于handleAt返回值判断）
static const int INVALID_HANDLE = -1;

// ============================================================
// 构造函数
//
// 初始化场景矩形为A4尺寸，创建撤销栈（父子关系自动管理生命周期），
// 连接基类selectionChanged信号以转发带元素列表的自定义信号。
// ============================================================
EditorScene::EditorScene(QObject* parent)
    : QGraphicsScene(parent)
    , m_currentPage(-1)
    , m_undoStack(new QUndoStack(this))
    , m_pageBackgroundItem(nullptr)
    , m_selectionDecorator(nullptr)
    , m_dragMode(None)
    , m_resizeHandle(TopLeft)
    , m_snapEnabled(true)
    , m_editingTextItem(nullptr)
{
    // 设置页面场景矩形（动态尺寸，默认A4）
    setSceneRect(0, 0, PageSizeManager::instance().pageWidth(),
                       PageSizeManager::instance().pageHeight());

    // 性能优化：使用BSP树索引加速大量元素的碰撞检测与重绘
    // BSP树适合静态布局较多的情况（PDF编辑器元素位置相对稳定），
    // 相比默认的NoIndex（线性遍历），在元素数量增加时查询性能更优。
    setItemIndexMethod(QGraphicsScene::BspTreeIndex);

    // 同步吸附开关到SnapGuide
    m_snapGuide.setEnabled(m_snapEnabled);

    // 连接基类选中变化信号，转发为带元素列表的自定义信号
    // &QGraphicsScene::selectionChanged明确引用无参版本，避免歧义
    connect(this, &QGraphicsScene::selectionChanged, this, [this]() {
        QList<PageElementPtr> elements;
        const QList<QGraphicsItem*> selItems = selectedItems();
        elements.reserve(selItems.size());
        for (QGraphicsItem* it : selItems) {
            if (auto* baseItem = dynamic_cast<BaseEditorItem*>(it)) {
                elements.append(baseItem->elementData());
            }
        }
        // emit调用带参版本（编译器根据实参选择重载）
        emit selectionChanged(elements);
    });
}

// ============================================================
// setLayoutEngine - 设置排版引擎
// ============================================================
void EditorScene::setLayoutEngine(LayoutEnginePtr engine)
{
    m_engine = engine;
}

// ============================================================
// setPageData - 设置当前页码和页面数据
//
// 设置后需调用loadPage才会重建场景中的Item。
// ============================================================
void EditorScene::setPageData(int pageIndex, const PageDataPtr& pageData)
{
    m_currentPage = pageIndex;
    m_currentPageData = pageData;
}

// ============================================================
// loadPage - 加载指定页
//
// 1. clearPage() 清除所有Item
// 2. 优先从已有PageData创建Item（恢复编辑状态）
// 3. 否则调用LayoutEngine生成元素，构造PageData后再创建Item
// 4. 刷新背景层
// ============================================================
void EditorScene::loadPage(int pageIndex)
{
    clearPage();
    m_currentPage = pageIndex;

    // 创建页面背景Item（白纸+边距虚线），必须在clearPage之后、创建元素Item之前
    updatePageBackground();

    if (m_currentPageData) {
        // 从已有PageData恢复元素
        createItemsFromPageData(m_currentPageData);
    } else if (m_engine) {
        // 从排版引擎生成元素
        QRectF pageRect(0, 0, PageSizeManager::instance().pageWidth(),
                             PageSizeManager::instance().pageHeight());
        QList<PageElementPtr> elements = m_engine->generateElements(pageIndex, pageRect);

        // 构造PageData
        PageData* page = new PageData();
        page->setPageIndex(pageIndex);
        page->setPageType(m_engine->pageType(pageIndex));
        for (const PageElementPtr& elem : elements) {
            page->addElement(elem);
        }
        m_currentPageData = PageDataPtr(page);
        createItemsFromPageData(m_currentPageData);
    }

    // 强制全场景重绘 + 失效背景层，确保白色纸张正确显示
    update(sceneRect());
    invalidate(sceneRect(), BackgroundLayer);
}

// ============================================================
// exportPageData - 导出当前页数据
//
// 同步所有编辑器Item的状态（位置、旋转、尺寸）到元素数据，
// 构造新的PageData返回。const方法通过非const指针修改Item。
//
// 注意：Item的syncToData()会clone元素数据并更新位置/旋转，
// 尺寸变化通过Item重建时已写入元素数据（见mouseReleaseEvent的Resize分支）。
// ============================================================
PageDataPtr EditorScene::exportPageData() const
{
    PageData* page = new PageData();
    page->setPageIndex(m_currentPage);
    if (m_currentPageData) {
        page->setPageType(m_currentPageData->pageType());
    }

    // 遍历所有编辑器Item，同步状态并收集元素数据
    const QList<QGraphicsItem*> allItems = items();
    for (QGraphicsItem* it : allItems) {
        if (auto* baseItem = dynamic_cast<BaseEditorItem*>(it)) {
            // syncToData: clone元素并更新位置/旋转/可见性/层级
            baseItem->syncToData();
            page->addElement(baseItem->elementData());
        }
    }

    return PageDataPtr(page);
}

// ============================================================
// currentPage - 返回当前页码
// ============================================================
int EditorScene::currentPage() const
{
    return m_currentPage;
}

// ============================================================
// undoStack - 返回撤销栈
// ============================================================
QUndoStack* EditorScene::undoStack() const
{
    return m_undoStack;
}

// ============================================================
// addElement - 添加元素到当前页面
//
// 通过AddCommand入撤销栈，redo()会自动创建Item并添加到场景。
// 传入空指针时直接返回，避免构造无效命令。
// 与Ctrl+D复制流程共用同一命令路径，保证撤销/重做行为一致。
// ============================================================
void EditorScene::addElement(const PageElementPtr& element)
{
    if (!element) return;
    m_undoStack->push(new AddCommand(this, element));
}

// ============================================================
// snapEnabled / setSnapEnabled - 吸附开关
// ============================================================
bool EditorScene::snapEnabled() const
{
    return m_snapEnabled;
}

void EditorScene::setSnapEnabled(bool enabled)
{
    m_snapEnabled = enabled;
    m_snapGuide.setEnabled(enabled);
}

// ============================================================
// selectedEditorItems - 返回当前选中的编辑器Item列表
//
// 过滤掉装饰器、手柄等非元素Item。
// ============================================================
QList<BaseEditorItem*> EditorScene::selectedEditorItems() const
{
    QList<BaseEditorItem*> result;
    const QList<QGraphicsItem*> selItems = selectedItems();
    for (QGraphicsItem* it : selItems) {
        if (auto* baseItem = dynamic_cast<BaseEditorItem*>(it)) {
            result.append(baseItem);
        }
    }
    return result;
}

// ============================================================
// drawBackground - 绘制页面背景
//
// 绘制白色页面背景矩形 + 页面边距虚线矩形。
// 页面尺寸与边距均从 PageSizeManager 动态获取，支持纸张切换。
// painter已变换到场景坐标系，直接使用页面坐标绘制。
// ============================================================
void EditorScene::drawBackground(QPainter* painter, const QRectF& rect)
{
    Q_UNUSED(painter);
    Q_UNUSED(rect);
    // 白色页面背景和边距虚线现由 m_pageBackgroundItem (QGraphicsRectItem) 渲染。
    // 不再使用drawBackground绘制，避免QGraphicsView缓存/更新模式导致背景不显示的问题。
    // view的backgroundBrush(灰色)负责页面外围区域。
}

// ============================================================
// updatePageBackground - 创建/更新页面背景Item
//
// 使用QGraphicsRectItem作为白纸背景，替代drawBackground方案。
// Item位于zValue=-10000（最底层），不可选中/不可移动。
// 边距虚线作为子Item附加。
// 在loadPage()和changePaperSize()后调用。
// ============================================================
void EditorScene::updatePageBackground()
{
    qreal pageW = PageSizeManager::instance().pageWidth();
    qreal pageH = PageSizeManager::instance().pageHeight();

    // 创建背景Item（首次或clearPage后被删除时）
    if (!m_pageBackgroundItem) {
        m_pageBackgroundItem = new QGraphicsRectItem();
        m_pageBackgroundItem->setZValue(-10000);  // 最底层，所有元素Item之下
        m_pageBackgroundItem->setFlag(QGraphicsItem::ItemIsSelectable, false);
        m_pageBackgroundItem->setFlag(QGraphicsItem::ItemIsMovable, false);
        addItem(m_pageBackgroundItem);
    }

    // 白色页面背景 + 浅灰边框
    m_pageBackgroundItem->setRect(0, 0, pageW, pageH);
    m_pageBackgroundItem->setBrush(QBrush(COLOR_WHITE));
    m_pageBackgroundItem->setPen(QPen(QColor(180, 180, 180), 1));

    // 更新边距虚线（先删除旧子Item再创建新的）
    for (auto* child : m_pageBackgroundItem->childItems()) {
        delete child;
    }
    qreal ml = PageSizeManager::instance().leftMargin();
    qreal mt = PageSizeManager::instance().topMargin();
    qreal mr = PageSizeManager::instance().rightMargin();
    qreal mb = PageSizeManager::instance().bottomMargin();
    QRectF marginRect(ml, mt, pageW - ml - mr, pageH - mt - mb);
    auto* marginItem = new QGraphicsRectItem(marginRect, m_pageBackgroundItem);
    marginItem->setPen(QPen(QColor(200, 200, 200), 1, Qt::DashLine));
    marginItem->setBrush(Qt::NoBrush);
    marginItem->setFlag(QGraphicsItem::ItemIsSelectable, false);

    // 更新场景矩形
    setSceneRect(0, 0, pageW, pageH);
}

// ============================================================
// drawForeground - 绘制前景层
//
// 绘制框选橡皮筋矩形（半透明蓝色填充+蓝色虚线边框）。
// 仅在RubberBand拖拽模式下且框选矩形有效时绘制。
// painter已变换到场景坐标系，直接使用场景坐标绘制。
// ============================================================
void EditorScene::drawForeground(QPainter* painter, const QRectF& rect)
{
    Q_UNUSED(rect);

    if (m_dragMode != RubberBand || !m_rubberBandRect.isValid() ||
        m_rubberBandRect.isNull()) {
        return;
    }

    painter->save();
    // 半透明蓝色填充（alpha=40，约16%不透明度）
    painter->setBrush(QBrush(QColor(0, 120, 215, 40)));
    // 蓝色虚线边框
    QPen bandPen(QColor(0, 120, 215), 1, Qt::DashLine);
    painter->setPen(bandPen);
    painter->drawRect(m_rubberBandRect);
    painter->restore();
}

// ============================================================
// mousePressEvent - 鼠标按下事件
//
// 处理流程：
//   1. 检查是否点中变换手柄 → 启动缩放/旋转
//   2. 检查是否点中元素Item → 选中并准备移动
//   3. 点击空白 → 清除选择并开始框选
//
// Item本身不处理PressEvent（setAcceptedMouseButtons由场景统一管理），
// 双击事件透传给Item（见mouseDoubleClickEvent）。
// ============================================================
void EditorScene::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
    // 如果有Item拥有焦点（文本编辑模式），先清除焦点以退出编辑模式。
    // clearFocus()触发FocusOut事件，TextEditorItem的eventFilter据此调用finishEditing()。
    if (focusItem()) {
        focusItem()->clearFocus();
    }

    // 文本编辑模式结束：断开光标变化信号
    if (m_editingTextItem) {
        disconnect(m_editingTextItem->document(), &QTextDocument::cursorPositionChanged,
                   this, &EditorScene::onEditCursorChanged);
        m_editingTextItem = nullptr;
    }

    if (event->button() != Qt::LeftButton) {
        QGraphicsScene::mousePressEvent(event);
        return;
    }

    QPointF pos = event->scenePos();

    // 1. 检查是否点中变换手柄（缩放/旋转）
    HandlePosition hp = handleAt(pos);
    if (static_cast<int>(hp) != INVALID_HANDLE) {
        if (hp == Rotation) {
            startRotate(pos);
        } else {
            startResize(hp, pos);
        }
        event->accept();
        return;
    }

    // 2. 查找点击位置下的编辑器元素（跳过装饰器和手柄）
    BaseEditorItem* baseItem = nullptr;
    const QList<QGraphicsItem*> itemList = items(pos);
    for (QGraphicsItem* it : itemList) {
        if (auto* bi = dynamic_cast<BaseEditorItem*>(it)) {
            baseItem = bi;
            break;
        }
    }

    if (baseItem) {
        // 点中元素
        if (event->modifiers() & Qt::ControlModifier) {
            // Ctrl+点击：切换选中状态（追加/移除）
            baseItem->setSelected(!baseItem->isSelected());
        } else if (!baseItem->isSelected()) {
            // 非多选模式下点中未选中元素：仅选中此元素
            clearSelection();
            baseItem->setSelected(true);
        }
        // 准备移动（无论是否刚选中）
        startMove(pos);
        updateSelectionDecorator();
        event->accept();
        return;
    }

    // 3. 点击空白：清除选择并开始框选
    clearSelection();
    updateSelectionDecorator();
    m_dragMode = RubberBand;
    m_dragStartPos = pos;
    m_rubberBandRect = QRectF(pos, pos);
    event->accept();
}

// ============================================================
// mouseMoveEvent - 鼠标移动事件
//
// 根据当前拖拽模式分派：
//   Move: 移动所有选中元素，应用网格吸附
//   Resize: 更新装饰器显示目标矩形（Item在release时重建）
//   Rotate: 计算旋转角度并应用到选中元素
//   RubberBand: 更新框选区域，选中相交元素
//   None: 交给基类处理（如悬停）
// ============================================================
void EditorScene::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
    QPointF pos = event->scenePos();

    switch (m_dragMode) {
    case Move: {
        // 计算位移增量
        QPointF delta = pos - m_dragStartPos;

        // 启用吸附时，以选中元素联合包围盒为整体计算SnapGuide吸附偏移
        // （保证多选时各元素相对位置不变，整体对齐参考线）
        if (m_snapEnabled && !m_initialPositions.isEmpty()) {
            // 计算选中元素在新位置的联合包围盒
            QRectF unionRect;
            bool first = true;
            for (auto it = m_initialPositions.begin(); it != m_initialPositions.end(); ++it) {
                BaseEditorItem* item = it.key();
                QPointF newPos = it.value() + delta;
                QRectF r(newPos, item->elementData().constData()->rect().size());
                if (first) { unionRect = r; first = false; }
                else { unionRect = unionRect.united(r); }
            }
            // 收集未选中元素的rect（作为吸附参考）
            QList<QRectF> otherRects;
            const QList<QGraphicsItem*> allItems = items();
            for (QGraphicsItem* gi : allItems) {
                if (auto* bi = dynamic_cast<BaseEditorItem*>(gi)) {
                    if (!m_initialPositions.contains(bi)) {
                        otherRects.append(bi->sceneBoundingRect());
                    }
                }
            }
            // 调用SnapGuide计算吸附位置
            SnapGuide::SnapResult result = m_snapGuide.calculateSnap(
                unionRect, unionRect.topLeft(), otherRects);
            // 将吸附调整量叠加到delta，所有元素同步偏移
            delta += (result.adjustedPos - unionRect.topLeft());

            // 清除旧参考线
            for (GuideLineItem* guide : m_activeGuideLines) {
                removeItem(guide);
                delete guide;
            }
            m_activeGuideLines.clear();

            // 绘制活跃参考线（吸附发生时高亮显示对齐线）
            for (const GuideLine& gl : result.activeGuides) {
                GuideLineItem* guideItem = new GuideLineItem(gl);
                addItem(guideItem);
                m_activeGuideLines.append(guideItem);
            }
        }

        // 移动所有选中元素（保持相对位置）
        for (auto it = m_initialPositions.begin(); it != m_initialPositions.end(); ++it) {
            BaseEditorItem* item = it.key();
            item->setPos(it.value() + delta);
            // 显式触发Item重绘，确保拖拽过程中文字实时更新位置
            item->update();
        }
        updateSelectionDecorator();
        break;
    }
    case Resize: {
        // 更新装饰器显示目标矩形（提供视觉反馈）
        // Item实际重建在mouseReleaseEvent中完成
        QPointF delta = pos - m_dragStartPos;
        // Shift键：四边也保持宽高比；四角默认保持宽高比
        bool isCorner = (m_resizeHandle == TopLeft || m_resizeHandle == TopRight ||
                         m_resizeHandle == BottomLeft || m_resizeHandle == BottomRight);
        bool keepRatio = isCorner || (event->modifiers() & Qt::ShiftModifier);

        const QList<BaseEditorItem*> selected = selectedEditorItems();
        if (!selected.isEmpty() && m_selectionDecorator) {
            // 计算选中元素的初始联合包围盒
            QRectF initialUnion;
            bool first = true;
            for (BaseEditorItem* item : selected) {
                QRectF r = m_initialRects.value(item);
                if (first) {
                    initialUnion = r;
                    first = false;
                } else {
                    initialUnion = initialUnion.united(r);
                }
            }
            // 计算缩放后的联合矩形
            QRectF newUnion = computeResizedRect(initialUnion, delta, m_resizeHandle, keepRatio);
            // 最小尺寸限制（视觉反馈阶段也应用，避免装饰器缩到不可见）
            if (newUnion.width() < MIN_ELEMENT_SIZE) {
                newUnion.setWidth(MIN_ELEMENT_SIZE);
            }
            if (newUnion.height() < MIN_ELEMENT_SIZE) {
                newUnion.setHeight(MIN_ELEMENT_SIZE);
            }
            // 更新装饰器（pos=0,0时rect即scene坐标）
            m_selectionDecorator->setPos(0, 0);
            m_selectionDecorator->setRect(newUnion);

            // 单元素拉伸时实时更新元素rect，实现文字流式重排
            if (selected.size() == 1) {
                BaseEditorItem* item = selected.first();
                QRectF initRect = m_initialRects.value(item);
                // 按联合矩形缩放比例计算元素新rect
                qreal scaleX = (initialUnion.width() > 0)
                               ? newUnion.width() / initialUnion.width() : 1.0;
                qreal scaleY = (initialUnion.height() > 0)
                               ? newUnion.height() / initialUnion.height() : 1.0;
                QRectF newRect(newUnion.left(), newUnion.top(),
                               initRect.width() * scaleX, initRect.height() * scaleY);
                item->updateRectTemporary(newRect);
            }
        }
        break;
    }
    case Rotate: {
        const QList<BaseEditorItem*> selected = selectedEditorItems();
        if (!selected.isEmpty()) {
            // 以选中元素联合包围盒中心为旋转中心
            QRectF centerRect;
            bool first = true;
            for (BaseEditorItem* item : selected) {
                QRectF r = item->sceneBoundingRect();
                if (first) {
                    centerRect = r;
                    first = false;
                } else {
                    centerRect = centerRect.united(r);
                }
            }
            QPointF center = centerRect.center();

            // 计算角度增量（QLineF::angle返回度数，顺时针为正）
            QLineF initialLine(center, m_dragStartPos);
            QLineF currentLine(center, pos);
            qreal angleDelta = currentLine.angle() - initialLine.angle();

            // Shift键：角度对齐到15度增量
            bool shiftPressed = event->modifiers() & Qt::ShiftModifier;

            // 应用到每个选中元素
            for (BaseEditorItem* item : selected) {
                qreal initialAngle = m_initialRotations.value(item, 0.0);
                qreal newAngle = initialAngle + angleDelta;
                // Shift：对齐到15度增量（round(angle/15)*15）
                if (shiftPressed) {
                    newAngle = qRound(newAngle / SNAP_ANGLE_STEP) * SNAP_ANGLE_STEP;
                }
                item->setRotation(newAngle);
            }
            updateSelectionDecorator();
        }
        break;
    }
    case RubberBand: {
        // 更新框选矩形
        m_rubberBandRect = QRectF(m_dragStartPos, pos).normalized();
        // 拖拽过程中使用IntersectsItemShape提供视觉反馈（相交即高亮）
        QPainterPath path;
        path.addRect(m_rubberBandRect);
        setSelectionArea(path, Qt::IntersectsItemShape);
        // 触发前景层重绘以显示半透明蓝色矩形
        invalidate(sceneRect(), ForegroundLayer);
        updateSelectionDecorator();
        break;
    }
    case None:
        // 无拖拽操作，交给基类处理（悬停等）
        QGraphicsScene::mouseMoveEvent(event);
        return;
    }

    event->accept();
}

// ============================================================
// mouseReleaseEvent - 鼠标释放事件
//
// 根据拖拽模式完成操作：
//   Move: 同步位置到元素数据（通过syncToData）
//   Resize: 重建Item以应用新尺寸（BaseEditorItem未暴露修改rect的接口）
//   Rotate: 旋转已通过setRotation实时更新，此处同步到数据
//   RubberBand: 框选已在mouseMoveEvent中完成
//
// 撤销命令集成预留：TODO Task 8 创建Move/Resize/Rotate命令并push
// ============================================================
void EditorScene::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
    switch (m_dragMode) {
    case Move: {
        // 仅在确实发生移动时同步数据和触发命令
        QPointF delta = event->scenePos() - m_dragStartPos;
        if (delta.manhattanLength() > 0.5) {
            // 收集旧/新位置，构建MoveCommand（按elementId索引，Item可能被重建）
            const QList<BaseEditorItem*> selected = selectedEditorItems();
            QMap<QString, QPointF> oldPositions, newPositions;
            for (BaseEditorItem* item : selected) {
                QString id = item->elementData().constData()->id();
                oldPositions[id] = m_initialPositions.value(item);
                newPositions[id] = item->pos();
                item->syncToData();
            }
            // push命令：首次redo()为幂等（Item已在拖拽时移动到新位置）
            m_undoStack->push(new MoveCommand(this, selected, oldPositions, newPositions));
            emit pageModified();
        }
        break;
    }
    case Resize: {
        // 仅在确实发生缩放时重建Item和触发命令
        QPointF pos = event->scenePos();
        QPointF delta = pos - m_dragStartPos;
        if (delta.manhattanLength() > 0.5) {
            // Shift键：四边也保持宽高比；四角默认保持宽高比
            bool isCorner = (m_resizeHandle == TopLeft || m_resizeHandle == TopRight ||
                             m_resizeHandle == BottomLeft || m_resizeHandle == BottomRight);
            bool keepRatio = isCorner || (event->modifiers() & Qt::ShiftModifier);

            const QList<BaseEditorItem*> selected = selectedEditorItems();
            if (!selected.isEmpty()) {
                // 计算初始联合包围盒
                QRectF initialUnion;
                bool first = true;
                for (BaseEditorItem* item : selected) {
                    QRectF r = m_initialRects.value(item);
                    if (first) {
                        initialUnion = r;
                        first = false;
                    } else {
                        initialUnion = initialUnion.united(r);
                    }
                }
                // 计算缩放后的联合矩形
                QRectF newUnion = computeResizedRect(initialUnion, delta, m_resizeHandle, keepRatio);

                // 限制最小尺寸
                if (newUnion.width() < MIN_ELEMENT_SIZE) {
                    newUnion.setWidth(MIN_ELEMENT_SIZE);
                }
                if (newUnion.height() < MIN_ELEMENT_SIZE) {
                    newUnion.setHeight(MIN_ELEMENT_SIZE);
                }

                // 计算缩放比例
                qreal scaleX = (initialUnion.width() > 0)
                               ? newUnion.width() / initialUnion.width() : 1.0;
                qreal scaleY = (initialUnion.height() > 0)
                               ? newUnion.height() / initialUnion.height() : 1.0;

                // 收集待重建的Item信息（在删除前捕获id和新旧rect，供ResizeCommand使用）
                struct ResizeInfo {
                    BaseEditorItem* item;   // 重建后失效
                    QString id;
                    QRectF oldRect;
                    QRectF newRect;
                };
                QList<ResizeInfo> resizeInfos;
                for (BaseEditorItem* item : selected) {
                    QRectF initRect = m_initialRects.value(item);
                    // 相对联合包围盒缩放
                    qreal newX = initialUnion.left()
                                 + (initRect.left() - initialUnion.left()) * scaleX;
                    qreal newY = initialUnion.top()
                                 + (initRect.top() - initialUnion.top()) * scaleY;
                    qreal newW = initRect.width() * scaleX;
                    qreal newH = initRect.height() * scaleY;
                    resizeInfos.append({item, item->elementData().constData()->id(),
                                        initRect, QRectF(newX, newY, newW, newH)});
                }

                // 重建每个Item（clone元素数据并设置新rect，删除旧Item创建新Item）
                for (const auto& info : resizeInfos) {
                    BaseEditorItem* oldItem = info.item;

                    // clone元素数据并设置新rect（clone保留id及其他属性）
                    PageElementData* clone = oldItem->elementData().constData()->clone();
                    clone->setRect(info.newRect);
                    PageElementPtr newElem(clone);

                    // 移除旧Item
                    removeItem(oldItem);
                    delete oldItem;

                    // 创建新Item
                    BaseEditorItem* newItem = createItemForElement(newElem);
                    if (newItem) {
                        addItem(newItem);
                        newItem->setSelected(true);
                    }
                }

                // 集成撤销命令：单元素直接push，多元素用宏包裹
                if (resizeInfos.size() == 1) {
                    m_undoStack->push(new ResizeCommand(this, resizeInfos[0].id,
                                                        resizeInfos[0].oldRect,
                                                        resizeInfos[0].newRect));
                } else if (resizeInfos.size() > 1) {
                    m_undoStack->beginMacro(QStringLiteral("缩放 %1 个元素").arg(resizeInfos.size()));
                    for (const auto& info : resizeInfos) {
                        m_undoStack->push(new ResizeCommand(this, info.id,
                                                            info.oldRect, info.newRect));
                    }
                    m_undoStack->endMacro();
                }
            }
            emit pageModified();
        }
        break;
    }
    case Rotate: {
        // 仅在确实发生旋转时同步数据和触发命令
        QPointF delta = event->scenePos() - m_dragStartPos;
        if (delta.manhattanLength() > 0.5) {
            const QList<BaseEditorItem*> selected = selectedEditorItems();
            if (!selected.isEmpty()) {
                // 收集id和旧/新旋转角度（RotateCommand按elementId索引）
                struct RotateInfo {
                    QString id;
                    qreal oldRotation;
                    qreal newRotation;
                };
                QList<RotateInfo> rotateInfos;
                for (BaseEditorItem* item : selected) {
                    rotateInfos.append({item->elementData().constData()->id(),
                                        m_initialRotations.value(item, 0.0),
                                        item->rotation()});
                    item->syncToData();
                }

                // 集成撤销命令：单元素直接push，多元素用宏包裹
                if (rotateInfos.size() == 1) {
                    m_undoStack->push(new RotateCommand(this, rotateInfos[0].id,
                                                        rotateInfos[0].oldRotation,
                                                        rotateInfos[0].newRotation));
                } else if (rotateInfos.size() > 1) {
                    m_undoStack->beginMacro(QStringLiteral("旋转 %1 个元素").arg(rotateInfos.size()));
                    for (const auto& info : rotateInfos) {
                        m_undoStack->push(new RotateCommand(this, info.id,
                                                            info.oldRotation, info.newRotation));
                    }
                    m_undoStack->endMacro();
                }
            }
            emit pageModified();
        }
        break;
    }
    case RubberBand: {
        // 释放时重新过滤：仅保留被框选框完全包含的元素（ContainsItemShape）
        QPainterPath path;
        path.addRect(m_rubberBandRect);
        setSelectionArea(path, Qt::ContainsItemShape);
        // 清除框选矩形视觉
        m_rubberBandRect = QRectF();
        invalidate(sceneRect(), ForegroundLayer);
        break;
    }
    case None:
        QGraphicsScene::mouseReleaseEvent(event);
        return;
    }

    // 清除吸附参考线（Move模式拖拽时创建）
    for (GuideLineItem* guide : m_activeGuideLines) {
        removeItem(guide);
        delete guide;
    }
    m_activeGuideLines.clear();

    // 重置拖拽状态
    m_dragMode = None;
    m_initialPositions.clear();
    m_initialRects.clear();
    m_initialRotations.clear();
    updateSelectionDecorator();
    event->accept();
}

// ============================================================
// mouseDoubleClickEvent - 鼠标双击事件
//
// 发射elementDoubleClicked信号，并调用基类将事件透传给Item
// （TextEditorItem收到双击后进入文本编辑模式）。
// ============================================================
void EditorScene::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        QPointF pos = event->scenePos();
        // 查找双击位置的编辑器元素
        const QList<QGraphicsItem*> itemList = items(pos);
        for (QGraphicsItem* it : itemList) {
            if (auto* baseItem = dynamic_cast<BaseEditorItem*>(it)) {
                emit elementDoubleClicked(baseItem);
                break;
            }
        }
    }
    // 调用基类将双击事件透传给Item（Item自行处理编辑模式入口）
    QGraphicsScene::mouseDoubleClickEvent(event);

    // 文本编辑模式检测：双击后TextEditorItem进入编辑模式，
    // focusItem变为QGraphicsTextItem。连接其document的cursorPositionChanged信号，
    // 用于选中文本时显示字符级格式化工具栏。
    if (QGraphicsTextItem* textItem = qgraphicsitem_cast<QGraphicsTextItem*>(focusItem())) {
        if (m_editingTextItem != textItem) {
            // 切换到新的编辑Item，先断开旧连接
            if (m_editingTextItem) {
                disconnect(m_editingTextItem->document(), &QTextDocument::cursorPositionChanged,
                           this, &EditorScene::onEditCursorChanged);
            }
            m_editingTextItem = textItem;
            connect(m_editingTextItem->document(), &QTextDocument::cursorPositionChanged,
                    this, &EditorScene::onEditCursorChanged);
        }
    }
}

// ============================================================
// keyPressEvent - 键盘事件
//
// Delete/Backspace: 删除选中元素
// Escape: 清除选择
// Ctrl+A: 全选
// Ctrl+Z/Y: 撤销/重做
// 方向键: 微移选中元素（Shift加速10pt）
// ============================================================
void EditorScene::keyPressEvent(QKeyEvent* event)
{
    // 如果有Item拥有焦点（如文本编辑模式下的QGraphicsTextItem），
    // 将键盘事件交给基类处理，使文本编辑器正常接收按键输入。
    if (focusItem()) {
        QGraphicsScene::keyPressEvent(event);
        return;
    }

    const QList<BaseEditorItem*> selected = selectedEditorItems();

    switch (event->key()) {
    case Qt::Key_Delete:
    case Qt::Key_Backspace: {
        // 删除选中元素：通过DeleteCommand实现，支持撤销/重做
        if (!selected.isEmpty()) {
            // 在Item删除前收集元素数据（共享指针保证数据存活）
            QList<PageElementPtr> elements;
            for (BaseEditorItem* item : selected) {
                elements.append(item->elementData());
            }
            // push DeleteCommand：redo执行删除（从场景移除Item），undo恢复（重建Item）
            m_undoStack->push(new DeleteCommand(this, elements));
            emit pageModified();
            updateSelectionDecorator();
        }
        event->accept();
        return;
    }
    case Qt::Key_Escape:
        clearSelection();
        updateSelectionDecorator();
        event->accept();
        return;
    case Qt::Key_A:
        if (event->modifiers() & Qt::ControlModifier) {
            // Ctrl+A: 全选编辑器元素
            const QList<QGraphicsItem*> allItems = items();
            for (QGraphicsItem* it : allItems) {
                if (dynamic_cast<BaseEditorItem*>(it)) {
                    it->setSelected(true);
                }
            }
            updateSelectionDecorator();
            event->accept();
            return;
        }
        break;
    case Qt::Key_Z:
        if (event->modifiers() & Qt::ControlModifier) {
            m_undoStack->undo();
            event->accept();
            return;
        }
        break;
    case Qt::Key_Y:
        if (event->modifiers() & Qt::ControlModifier) {
            m_undoStack->redo();
            event->accept();
            return;
        }
        break;
    case Qt::Key_D:
        // Ctrl+D: 复制选中元素（偏移20点）
        if (event->modifiers() & Qt::ControlModifier) {
            if (!selected.isEmpty()) {
                // 多元素用宏包裹，支持一次性撤销
                m_undoStack->beginMacro(QStringLiteral("复制 %1 个元素").arg(selected.size()));
                for (BaseEditorItem* item : selected) {
                    // clone元素数据并偏移位置，生成新ID避免与原元素冲突
                    PageElementData* cloned = item->elementData().constData()->clone();
                    QRectF r = cloned->rect();
                    cloned->setRect(QRectF(r.x() + 20.0, r.y() + 20.0, r.width(), r.height()));
                    cloned->setId(QUuid::createUuid().toString());
                    PageElementPtr newElem(cloned);
                    // AddCommand的redo创建Item并添加到场景，undo移除
                    m_undoStack->push(new AddCommand(this, newElem));
                }
                m_undoStack->endMacro();
                emit pageModified();
            }
            event->accept();
            return;
        }
        break;
    case Qt::Key_C:
        // Ctrl+C: 复制选中元素到剪贴板
        if (event->modifiers() & Qt::ControlModifier) {
            if (!selected.isEmpty()) {
                m_clipboard.clear();
                for (BaseEditorItem* item : selected) {
                    // 深拷贝元素数据，避免与场景中元素共享可变状态
                    PageElementData* cloned = item->elementData().constData()->clone();
                    m_clipboard.append(PageElementPtr(cloned));
                }
            }
            event->accept();
            return;
        }
        break;
    case Qt::Key_V:
        // Ctrl+V: 粘贴剪贴板元素到偏移位置
        if (event->modifiers() & Qt::ControlModifier) {
            if (!m_clipboard.isEmpty()) {
                m_undoStack->beginMacro(QStringLiteral("粘贴 %1 个元素").arg(m_clipboard.size()));
                for (const PageElementPtr& elem : m_clipboard) {
                    // 每次粘贴都深拷贝，生成新ID，偏移(20,20)
                    PageElementData* cloned = elem.constData()->clone();
                    QRectF r = cloned->rect();
                    cloned->setRect(QRectF(r.x() + 20.0, r.y() + 20.0, r.width(), r.height()));
                    cloned->setId(QUuid::createUuid().toString());
                    PageElementPtr newElem(cloned);
                    m_undoStack->push(new AddCommand(this, newElem));
                }
                m_undoStack->endMacro();
                emit pageModified();
            }
            event->accept();
            return;
        }
        break;
    case Qt::Key_X:
        // Ctrl+X: 剪切选中元素（复制到剪贴板后删除）
        if (event->modifiers() & Qt::ControlModifier) {
            if (!selected.isEmpty()) {
                // 先复制到剪贴板
                m_clipboard.clear();
                for (BaseEditorItem* item : selected) {
                    PageElementData* cloned = item->elementData().constData()->clone();
                    m_clipboard.append(PageElementPtr(cloned));
                }
                // 再通过DeleteCommand删除
                QList<PageElementPtr> elements;
                for (BaseEditorItem* item : selected) {
                    elements.append(item->elementData());
                }
                m_undoStack->push(new DeleteCommand(this, elements));
                emit pageModified();
            }
            event->accept();
            return;
        }
        break;
    case Qt::Key_Left:
    case Qt::Key_Right:
    case Qt::Key_Up:
    case Qt::Key_Down: {
        // 方向键微移（通过NudgeCommand入撤销栈，支持Ctrl+Z撤销）
        if (!selected.isEmpty()) {
            qreal step = (event->modifiers() & Qt::ShiftModifier) ? 10.0 : 1.0;
            qreal dx = 0, dy = 0;
            switch (event->key()) {
            case Qt::Key_Left:  dx = -step; break;
            case Qt::Key_Right: dx =  step; break;
            case Qt::Key_Up:    dy = -step; break;
            case Qt::Key_Down:  dy =  step; break;
            default: break;
            }
            // 记录旧位置和新位置，通过NudgeCommand实现撤销/重做
            QMap<QString, QPointF> oldPositions, newPositions;
            for (BaseEditorItem* item : selected) {
                QString id = item->elementData().constData()->id();
                QPointF oldPos = item->pos();
                oldPositions[id] = oldPos;
                newPositions[id] = QPointF(oldPos.x() + dx, oldPos.y() + dy);
            }
            // push NudgeCommand：redo()会执行setPos到新位置
            m_undoStack->push(new NudgeCommand(this, oldPositions, newPositions));
            emit pageModified();
            updateSelectionDecorator();
            event->accept();
            return;
        }
        break;
    }
    default:
        break;
    }

    QGraphicsScene::keyPressEvent(event);
}

// ============================================================
// clearPage - 清除当前页所有Item
//
// 使用QGraphicsScene::clear()移除并删除所有Item，
// 重置装饰器指针。
// ============================================================
void EditorScene::clearPage()
{
    clearSelection();
    clear();  // 移除并删除所有Item（包括m_pageBackgroundItem）
    m_pageBackgroundItem = nullptr;  // 悬空指针重置
    m_selectionDecorator = nullptr;
    // 编辑Item和参考线Item已被clear()删除，仅重置指针/列表
    m_editingTextItem = nullptr;
    m_activeGuideLines.clear();
}

// ============================================================
// onEditCursorChanged - 文本编辑模式光标变化处理
//
// 当编辑模式下的QGraphicsTextItem选区变化时，计算选区起始行
// 的场景坐标中心，emit textFormatRequested信号通知MainWindow
// 显示字符级格式化工具栏。
// ============================================================
void EditorScene::onEditCursorChanged(const QTextCursor& cursor)
{
    if (!m_editingTextItem || !cursor.hasSelection()) {
        return;
    }

    QTextDocument* doc = m_editingTextItem->document();
    // 选区起始位置所在文本块
    QTextBlock block = doc->findBlock(cursor.selectionStart());
    if (!block.isValid()) return;

    QTextLayout* layout = block.layout();
    if (!layout) return;

    // 选区起始位置在块内的偏移，找到对应行
    int posInBlock = cursor.selectionStart() - block.position();
    QTextLine line = layout->lineForTextPosition(posInBlock);
    if (!line.isValid()) return;

    // 行中心：layout->position()为文本块在文档坐标系中的位置，
    // line.rect()为行在块内坐标，两者相加得到文档坐标（= item本地坐标），
    // 再通过mapToScene映射到场景坐标
    QPointF blockOffset = layout->position();
    QPointF lineCenter = blockOffset + line.rect().center();
    QPointF sceneCenter = m_editingTextItem->mapToScene(lineCenter);

    emit textFormatRequested(sceneCenter);
}

// ============================================================
// createItemsFromPageData - 从PageData创建所有编辑器Item
// ============================================================
void EditorScene::createItemsFromPageData(const PageDataPtr& pageData)
{
    if (!pageData) return;

    const QList<PageElementPtr> elements = pageData->elements();
    for (const PageElementPtr& elem : elements) {
        BaseEditorItem* item = createItemForElement(elem);
        if (item) {
            addItem(item);
        }
    }
}

// ============================================================
// createItemForElement - 根据元素类型创建对应的编辑器Item
//
// 各Item构造函数内部会clone元素数据（确保Item拥有独立副本）。
// 此处先clone一次以构造类型化智能指针，Item构造时再clone一次。
// ============================================================
BaseEditorItem* EditorScene::createItemForElement(const PageElementPtr& element)
{
    if (!element) return nullptr;

    BaseEditorItem* item = nullptr;
    switch (element->elementType()) {
    case PageElementData::Text: {
        // clone为TextElementData以构造TextElementPtr
        TextElementData* clone =
            static_cast<TextElementData*>(element.constData()->clone());
        item = new TextEditorItem(TextElementPtr(clone));
        break;
    }
    case PageElementData::Image: {
        ImageElementData* clone =
            static_cast<ImageElementData*>(element.constData()->clone());
        item = new ImageEditorItem(ImageElementPtr(clone));
        break;
    }
    case PageElementData::Shape: {
        ShapeElementData* clone =
            static_cast<ShapeElementData*>(element.constData()->clone());
        item = new ShapeEditorItem(ShapeElementPtr(clone));
        break;
    }
    }
    // BaseEditorItem构造函数已调用syncFromData()，无需重复调用
    return item;
}

// ============================================================
// updateSelectionDecorator - 更新选中装饰器
//
// 无选中：移除装饰器
// 有选中：创建（首次）或更新装饰器，覆盖所有选中元素的联合包围盒
//
// 装饰器pos设为(0,0)，rect使用scene坐标，确保本地坐标=scene坐标。
// Z值设为最高，覆盖在所有元素之上。
// ============================================================
void EditorScene::updateSelectionDecorator()
{
    const QList<BaseEditorItem*> selected = selectedEditorItems();

    if (selected.isEmpty()) {
        // 无选中：移除装饰器
        if (m_selectionDecorator) {
            removeItem(m_selectionDecorator);
            delete m_selectionDecorator;
            m_selectionDecorator = nullptr;
        }
        return;
    }

    // 计算所有选中元素的联合包围盒（scene坐标）
    QRectF unionRect;
    bool first = true;
    for (BaseEditorItem* item : selected) {
        QRectF r = item->sceneBoundingRect();
        if (first) {
            unionRect = r;
            first = false;
        } else {
            unionRect = unionRect.united(r);
        }
    }

    // 创建或更新装饰器
    if (!m_selectionDecorator) {
        m_selectionDecorator = new SelectionDecorator();
        addItem(m_selectionDecorator);
        m_selectionDecorator->setZValue(DECORATOR_Z);
    }
    // 装饰器pos设为(0,0)，rect使用scene坐标
    m_selectionDecorator->setPos(0, 0);
    m_selectionDecorator->setRect(unionRect);
}

// ============================================================
// handleAt - 变换手柄hit-test
//
// 遍历装饰器的所有手柄，返回包含pos的手柄位置。
// 无手柄命中时返回INVALID_HANDLE强转值（-1）。
// ============================================================
HandlePosition EditorScene::handleAt(const QPointF& pos) const
{
    if (!m_selectionDecorator) {
        return static_cast<HandlePosition>(INVALID_HANDLE);
    }

    const QList<SelectionHandle*> handles = m_selectionDecorator->handles();
    for (SelectionHandle* handle : handles) {
        if (handle->sceneBoundingRect().contains(pos)) {
            return handle->handlePosition();
        }
    }
    return static_cast<HandlePosition>(INVALID_HANDLE);
}

// ============================================================
// startMove - 开始移动操作
//
// 记录所有选中元素的初始位置，用于拖拽时计算增量。
// ============================================================
void EditorScene::startMove(const QPointF& startPos)
{
    m_dragMode = Move;
    m_dragStartPos = startPos;

    const QList<BaseEditorItem*> selected = selectedEditorItems();
    m_initialPositions.clear();
    for (BaseEditorItem* item : selected) {
        m_initialPositions.insert(item, item->pos());
    }
}

// ============================================================
// startResize - 开始缩放操作
//
// 记录手柄位置和所有选中元素的初始矩形。
// ============================================================
void EditorScene::startResize(HandlePosition handle, const QPointF& startPos)
{
    m_dragMode = Resize;
    m_resizeHandle = handle;
    m_dragStartPos = startPos;

    const QList<BaseEditorItem*> selected = selectedEditorItems();
    m_initialRects.clear();
    for (BaseEditorItem* item : selected) {
        // 使用元素的rect（包含正确的位置和尺寸）
        m_initialRects.insert(item, item->elementData().constData()->rect());
    }
}

// ============================================================
// startRotate - 开始旋转操作
//
// 记录所有选中元素的初始旋转角度。
// ============================================================
void EditorScene::startRotate(const QPointF& startPos)
{
    m_dragMode = Rotate;
    m_dragStartPos = startPos;

    const QList<BaseEditorItem*> selected = selectedEditorItems();
    m_initialRotations.clear();
    for (BaseEditorItem* item : selected) {
        m_initialRotations.insert(item, item->rotation());
    }
}

// ============================================================
// snapPoint - 点吸附到网格
//
// 吸附关闭时原样返回。
// ============================================================
QPointF EditorScene::snapPoint(const QPointF& pos) const
{
    if (!m_snapEnabled) return pos;
    return QPointF(
        qRound(pos.x() / SNAP_GRID) * SNAP_GRID,
        qRound(pos.y() / SNAP_GRID) * SNAP_GRID
    );
}

// ============================================================
// snapAngle - 角度吸附到步长
//
// 吸附关闭时原样返回。
// ============================================================
qreal EditorScene::snapAngle(qreal angle) const
{
    if (!m_snapEnabled) return angle;
    return qRound(angle / SNAP_ANGLE_STEP) * SNAP_ANGLE_STEP;
}

// ============================================================
// computeResizedRect - 根据手柄方向和位移增量计算缩放后的矩形
//
// 参数：
//   initial        - 初始矩形（拖拽开始时的元素rect）
//   delta          - 鼠标位移增量（当前pos - 拖拽起始pos）
//   handle         - 被拖拽的手柄位置
//   keepAspectRatio - 是否保持宽高比（四角默认true，四边Shift时true）
//
// 无宽高比约束时：
//   TopLeft/Top/TopRight: 顶边随鼠标Y移动
//   BottomLeft/Bottom/BottomRight: 底边随鼠标Y移动
//   TopLeft/Left/BottomLeft: 左边随鼠标X移动
//   TopRight/Right/BottomRight: 右边随鼠标X移动
//   处理翻转（宽度或高度为负时校正为正值）
//
// 宽高比约束时：
//   以对角（四角）或对边中心（四边）为锚点等比缩放
//   四角：取X/Y方向较大相对位移作为缩放基准
//   四边：以该方向位移为基准，另一方向按比例同步
// ============================================================
QRectF EditorScene::computeResizedRect(const QRectF& initial,
                                       const QPointF& delta,
                                       HandlePosition handle,
                                       bool keepAspectRatio)
{
    // ---- 无宽高比约束：各边独立跟随鼠标 ----
    if (!keepAspectRatio) {
        QRectF r = initial;
        switch (handle) {
        case TopLeft:
            r.setLeft(initial.left() + delta.x());
            r.setTop(initial.top() + delta.y());
            break;
        case Top:
            r.setTop(initial.top() + delta.y());
            break;
        case TopRight:
            r.setRight(initial.right() + delta.x());
            r.setTop(initial.top() + delta.y());
            break;
        case Left:
            r.setLeft(initial.left() + delta.x());
            break;
        case Right:
            r.setRight(initial.right() + delta.x());
            break;
        case BottomLeft:
            r.setLeft(initial.left() + delta.x());
            r.setBottom(initial.bottom() + delta.y());
            break;
        case Bottom:
            r.setBottom(initial.bottom() + delta.y());
            break;
        case BottomRight:
            r.setRight(initial.right() + delta.x());
            r.setBottom(initial.bottom() + delta.y());
            break;
        case Rotation:
            // 旋转手柄不应触发缩放
            break;
        }
        // 校正翻转（宽高为负时取绝对值并调整起点）
        if (r.width() < 0) {
            r = QRectF(r.right(), r.top(), -r.width(), r.height());
        }
        if (r.height() < 0) {
            r = QRectF(r.left(), r.bottom(), r.width(), -r.height());
        }
        return r;
    }

    // ---- 宽高比约束：以锚点为基准等比缩放 ----
    qreal initW = initial.width();
    qreal initH = initial.height();
    if (initW <= 0 || initH <= 0) {
        return initial;
    }
    qreal ratio = initW / initH;  // 宽/高

    // 确定锚点（固定不动）和动点（跟随鼠标）
    // 四角锚点=对角顶点；四边锚点=对边中点
    QPointF anchor, moving;
    bool isCorner = (handle == TopLeft || handle == TopRight ||
                     handle == BottomLeft || handle == BottomRight);
    bool isEdgeH = (handle == Left || handle == Right);   // 水平边（左右）
    bool isEdgeV = (handle == Top || handle == Bottom);   // 垂直边（上下）

    switch (handle) {
    case TopLeft:
        anchor = initial.bottomRight();
        moving = initial.topLeft() + delta;
        break;
    case Top:
        anchor = QPointF(initial.center().x(), initial.bottom());
        moving = QPointF(initial.center().x(), initial.top() + delta.y());
        break;
    case TopRight:
        anchor = initial.bottomLeft();
        moving = initial.topRight() + delta;
        break;
    case Left:
        anchor = QPointF(initial.right(), initial.center().y());
        moving = QPointF(initial.left() + delta.x(), initial.center().y());
        break;
    case Right:
        anchor = QPointF(initial.left(), initial.center().y());
        moving = QPointF(initial.right() + delta.x(), initial.center().y());
        break;
    case BottomLeft:
        anchor = initial.topRight();
        moving = initial.bottomLeft() + delta;
        break;
    case Bottom:
        anchor = QPointF(initial.center().x(), initial.top());
        moving = QPointF(initial.center().x(), initial.bottom() + delta.y());
        break;
    case BottomRight:
        anchor = initial.topLeft();
        moving = initial.bottomRight() + delta;
        break;
    case Rotation:
        return initial;
    }

    // 动点相对于锚点的偏移
    qreal dx = moving.x() - anchor.x();
    qreal dy = moving.y() - anchor.y();

    qreal newW, newH;
    if (isCorner) {
        // 四角：取X/Y方向较大相对位移作为缩放基准，保持宽高比
        qreal sx = qAbs(dx) / initW;
        qreal sy = qAbs(dy) / initH;
        if (sx >= sy) {
            // X方向位移主导，高度按比例同步
            newW = qAbs(dx);
            newH = newW / ratio;
        } else {
            // Y方向位移主导，宽度按比例同步
            newH = qAbs(dy);
            newW = newH * ratio;
        }
    } else if (isEdgeH) {
        // 水平边：宽度驱动，高度按比例同步
        newW = qAbs(dx);
        newH = newW / ratio;
    } else { // isEdgeV
        // 垂直边：高度驱动，宽度按比例同步
        newH = qAbs(dy);
        newW = newH * ratio;
    }

    // 根据锚点和手柄方向计算新矩形位置
    // 动点在锚点右侧→矩形从锚点向右延伸；左侧→向左延伸；上下同理
    QRectF result;
    if (isCorner) {
        qreal left = (dx >= 0) ? anchor.x() : anchor.x() - newW;
        qreal top = (dy >= 0) ? anchor.y() : anchor.y() - newH;
        result = QRectF(left, top, newW, newH);
    } else if (isEdgeH) {
        // 水平边：垂直方向居中于锚点，水平方向跟随动点
        qreal left = (dx >= 0) ? anchor.x() : anchor.x() - newW;
        qreal top = anchor.y() - newH / 2.0;
        result = QRectF(left, top, newW, newH);
    } else { // isEdgeV
        // 垂直边：水平方向居中于锚点，垂直方向跟随动点
        qreal left = anchor.x() - newW / 2.0;
        qreal top = (dy >= 0) ? anchor.y() : anchor.y() - newH;
        result = QRectF(left, top, newW, newH);
    }

    return result;
}
