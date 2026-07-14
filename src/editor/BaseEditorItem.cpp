#include "BaseEditorItem.h"
#include "ElementRenderer.h"

#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <QPen>
#include <QColor>

// 选中边框边距：boundingRect比元素rect大一圈，为虚线边框留空间
const qreal BaseEditorItem::SELECTION_MARGIN = 2.0;

// ============================================================
// 构造函数
//
// m_element通过QSharedDataPointer的隐式共享存储（引用计数+1），
// 修改时通过clone()创建深拷贝（见syncToData）。
// ============================================================
BaseEditorItem::BaseEditorItem(const PageElementPtr& element, QGraphicsItem* parent)
    : QGraphicsItem(parent)
    , m_element(element)
{
    // 允许选中（选中时绘制边框，QGraphicsItem自动在选中状态变化时调用update）
    setFlag(ItemIsSelectable, true);
    // 允许几何变更通知（itemChange可捕获位置/尺寸变化，供EditorScene使用）
    setFlag(ItemSendsGeometryChanges, true);

    // 根据元素数据初始化Item状态
    syncFromData();
}

// ============================================================
// elementData - 返回关联的元素数据
// ============================================================
PageElementPtr BaseEditorItem::elementData() const
{
    return m_element;
}

// ============================================================
// setElementData - 原地替换元素数据
//
// 用于属性面板修改属性时，避免rebuildItemWithModifiedElement
// 删除/重建Item触发selectionChanged打断PropertyPanel状态。
// 替换后调用syncFromData()刷新位置/尺寸/旋转/可见性，
// 再update()触发重绘。
// ============================================================
void BaseEditorItem::setElementData(const PageElementPtr& element)
{
    m_element = element;
    syncFromData();
}

// ============================================================
// syncFromData - 数据 -> Item
//
// 从PageElementData读取位置、尺寸、旋转、可见性、层级等，
// 同步到QGraphicsItem的对应属性。
// ============================================================
void BaseEditorItem::syncFromData()
{
    prepareGeometryChange();

    QRectF r = m_element.constData()->rect();
    // item的pos()对应元素rect左上角（scene坐标）
    setPos(r.topLeft());
    // 旋转中心设为元素中心，与ElementRenderer的旋转中心一致
    setTransformOriginPoint(r.width() / 2.0, r.height() / 2.0);
    setRotation(m_element.constData()->rotation());
    setZValue(m_element.constData()->zOrder());
    setVisible(m_element.constData()->isVisible());

    update();
}

// ============================================================
// syncToData - Item -> 数据
//
// 将Item的当前位置、旋转、可见性、层级写回元素数据。
// 由于PageElementData为抽象类（QSharedDataPointer只读访问），
// 通过clone()创建可修改的深拷贝，修改后替换m_element。
// 调用方（EditorScene）应通过elementData()获取新元素并写回PageData。
// ============================================================
void BaseEditorItem::syncToData()
{
    PageElementData* cloned = m_element.constData()->clone();

    // 保留原尺寸，更新位置为Item当前pos()
    QRectF r = m_element.constData()->rect();
    r.moveTopLeft(pos());
    cloned->setRect(r);
    cloned->setRotation(rotation());
    cloned->setVisible(isVisible());
    cloned->setZOrder(static_cast<int>(zValue()));

    m_element = PageElementPtr(cloned);
}

// ============================================================
// updateRectTemporary - 拉伸过程中临时更新元素rect
//
// 在mouseMoveEvent的Resize分支中调用，实现文字流式重排。
// clone元素数据并setRect，更新Item位置和包围矩形，触发重绘。
// 不入撤销栈，最终尺寸在mouseReleaseEvent中通过ResizeCommand入栈。
// ============================================================
void BaseEditorItem::updateRectTemporary(const QRectF& newRect)
{
    prepareGeometryChange();
    PageElementData* cloned = m_element.constData()->clone();
    cloned->setRect(newRect);
    m_element = PageElementPtr(cloned);
    setPos(newRect.topLeft());
    update();
}

// ============================================================
// boundingRect - Item本地坐标系下的包围矩形
//
// 元素内容从(0,0)到(width,height)，四周留SELECTION_MARGIN边距
// 给选中虚线边框。QGraphicsItem用此矩形进行重绘裁剪和碰撞检测。
// ============================================================
QRectF BaseEditorItem::boundingRect() const
{
    QRectF r = m_element.constData()->rect();
    const qreal m = SELECTION_MARGIN;
    return QRectF(-m, -m, r.width() + 2.0 * m, r.height() + 2.0 * m);
}

// ============================================================
// paint - 绘制元素内容和选中边框
//
// 渲染流程：
//   1. 平移painter使ElementRenderer的绝对坐标映射到item本地坐标
//   2. 反向旋转抵消ElementRenderer内部的旋转（避免双重旋转）
//   3. 调用ElementRenderer::renderElement绘制元素内容
//   4. 选中时在本地坐标系绘制蓝色虚线边框
//
// 旋转抵消原理：
//   QGraphicsItem已通过setRotation()绕中心旋转painter，
//   ElementRenderer::renderElement内部也会绕rect.center旋转。
//   在此预先反向旋转（绕同一中心），使renderElement的旋转净效果为0，
//   最终旋转完全由QGraphicsItem的transform负责。
//   对无旋转元素（rotation≈0），两处旋转均跳过，无额外开销。
// ============================================================
void BaseEditorItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
                           QWidget* widget)
{
    Q_UNUSED(widget);

    // 移除QGraphicsItem默认的选中虚线框（使用自定义边框）
    QStyleOptionGraphicsItem opt(*option);
    opt.state &= ~QStyle::State_Selected;

    painter->save();

    // ElementRenderer使用绝对页面坐标渲染，
    // item本地坐标原点对应rect.topLeft，需平移使绝对坐标映射到本地坐标
    QRectF elemRect = m_element.constData()->rect();
    painter->translate(-elemRect.topLeft());

    // 反向旋转抵消ElementRenderer内部的旋转（仅对有旋转的元素生效）
    qreal rot = m_element.constData()->rotation();
    if (qAbs(rot) > 0.001) {
        QPointF center = elemRect.center();
        painter->translate(center);
        painter->rotate(-rot);
        painter->translate(-center);
    }

    // 渲染元素内容（ElementRenderer处理可见性检查、类型分派）
    ElementRenderer::renderElement(painter, m_element);

    painter->restore();

    // 选中时绘制自定义蓝色虚线边框
    if (option->state & QStyle::State_Selected) {
        drawSelectionBorder(painter);
    }
}

// ============================================================
// drawSelectionBorder - 绘制选中边框
//
// 在item本地坐标系绘制，元素区域为(0,0)到(width,height)。
// 使用蓝色（#1976D2）1像素虚线，与项目主题色一致。
// ============================================================
void BaseEditorItem::drawSelectionBorder(QPainter* painter)
{
    painter->save();
    QPen pen(QColor(25, 118, 210), 1, Qt::DashLine);
    painter->setPen(pen);
    painter->setBrush(Qt::NoBrush);

    QRectF r = m_element.constData()->rect();
    painter->drawRect(QRectF(0, 0, r.width(), r.height()));

    painter->restore();
}
