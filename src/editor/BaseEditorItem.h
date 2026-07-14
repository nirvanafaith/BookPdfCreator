#ifndef BASEEDITORITEM_H
#define BASEEDITORITEM_H

#include <QGraphicsItem>
#include "PageElement.h"

// ============================================================
// BaseEditorItem - 编辑器Item基类
//
// 所有页面元素在GraphicsScene中的可视表示基类。
// 负责将PageElementData数据模型与QGraphicsItem交互模型桥接：
//   - syncFromData(): 数据 -> Item（位置、尺寸、旋转等）
//   - syncToData():   Item -> 数据（交互修改后写回，通过clone实现）
//
// 渲染策略：
//   paint()内调用ElementRenderer::renderElement绘制元素内容，
//   选中时额外绘制蓝色虚线边框。
//
// 旋转处理：
//   QGraphicsItem通过setRotation()处理旋转（绕中心），
//   paint()内反向旋转抵消ElementRenderer内部的旋转，
//   避免双重旋转（详见paint()实现注释）。
//
// 坐标系：
//   item的pos()=元素rect左上角（scene坐标），
//   item本地坐标系原点(0,0)对应元素rect左上角，
//   boundingRect从(-margin,-margin)到(width+margin,height+margin)。
// ============================================================
class BaseEditorItem : public QGraphicsItem
{
public:
    BaseEditorItem(const PageElementPtr& element, QGraphicsItem* parent = nullptr);

    // 获取关联的数据元素（共享引用，只读访问）
    PageElementPtr elementData() const;

    // 原地替换元素数据（不删除/重建Item）
    // 用于属性面板修改属性时避免selectionChanged打断PropertyPanel状态。
    // 调用syncFromData()+update()刷新Item显示。
    void setElementData(const PageElementPtr& element);

    // 数据 -> Item：根据元素数据更新Item的位置、尺寸、旋转等
    virtual void syncFromData();
    // Item -> 数据：将Item当前状态写回元素数据（通过clone创建新副本）
    virtual void syncToData();

    // 临时更新元素rect（拉伸过程中实时调用，不入撤销栈）
    // clone元素数据并setRect，触发prepareGeometryChange和重绘
    void updateRectTemporary(const QRectF& newRect);

    // QGraphicsItem接口
    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
               QWidget* widget) override;

    // 类型标识（用于qgraphicsitem_cast安全转换）
    enum { Type = UserType + 1 };
    int type() const override { return Type; }

protected:
    PageElementPtr m_element;  // 关联的元素数据（隐式共享，修改时clone）

    // 选中时绘制蓝色虚线边框（在本地坐标系，0,0到width,height）
    // 虚函数：子类可覆盖以将边框贴合到实际渲染内容（如图片/文本）而非容器rect
    virtual void drawSelectionBorder(QPainter* painter);

    // 选中边框边距（为虚线留出空间）
    static const qreal SELECTION_MARGIN;
};

#endif // BASEEDITORITEM_H
