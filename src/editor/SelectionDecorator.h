#ifndef SELECTIONDECORATOR_H
#define SELECTIONDECORATOR_H

#include <QGraphicsItem>
#include <QRectF>
#include <QList>

#include "SelectionHandle.h"

// ============================================================
// SelectionDecorator - 选中装饰器
//
// 管理选中元素的视觉装饰：蓝色虚线边框 + 8个缩放手柄 + 1个旋转手柄。
// 作为独立QGraphicsItem覆盖在选中元素上方。
//
// 使用场景：
//   - 单选：EditorScene创建SelectionDecorator覆盖选中元素的包围盒
//   - 多选：EditorScene创建SelectionDecorator覆盖所有选中元素的联合包围盒
//   - 取消选中时EditorScene移除SelectionDecorator
//
// 手柄交互：
//   手柄是此装饰器的子Item。EditorScene通过itemAt(hitTestPos)判断
//   鼠标是否在手柄上，据此启动缩放/旋转操作。
// ============================================================
class SelectionDecorator : public QGraphicsItem
{
public:
    SelectionDecorator(QGraphicsItem* parent = nullptr);

    // 设置装饰器覆盖的矩形（scene坐标系，由EditorScene根据选中元素计算）
    void setRect(const QRectF& rect);
    QRectF rect() const;

    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
               QWidget* widget) override;

    // 创建/更新9个手柄的位置（根据当前m_rect）
    void updateHandles();

    // 获取指定位置的手柄（用于hit-test）
    SelectionHandle* handleAt(HandlePosition pos) const;

    // 所有手柄列表
    QList<SelectionHandle*> handles() const;

    // 类型标识
    enum { Type = UserType + 11 };
    int type() const override { return Type; }

private:
    QRectF m_rect;                        // 装饰器覆盖的矩形
    QList<SelectionHandle*> m_handles;     // 9个手柄（8缩放+1旋转）

    // 旋转手柄距顶部边框的距离（像素）
    static const qreal ROTATION_HANDLE_OFFSET;
};

#endif // SELECTIONDECORATOR_H
