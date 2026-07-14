#ifndef TEXTEDITORITEM_H
#define TEXTEDITORITEM_H

#include <QObject>
#include <QGraphicsItem>
#include <QGraphicsTextItem>

#include "BaseEditorItem.h"
#include "PageElement.h"

// ============================================================
// TextEditorItem - 文本编辑Item
//
// 继承QObject（用于信号）和BaseEditorItem（用于元素渲染）。
// 注意：多重继承时QObject必须在第一位，以满足MOC要求。
//
// 编辑模式：
//   双击进入编辑模式，创建QGraphicsTextItem作为子Item，
//   支持光标编辑、文本选择。点击外部（焦点丢失）自动退出编辑。
//   编辑完成后文本内容同步回TextElementData。
//
// 预留信号：
//   textFormatRequested - 编辑模式下选中文本时触发，
//   用于显示字符级格式化工具栏（工具栏在EditorScene/Task 9实现）。
// ============================================================
class TextEditorItem : public QObject, public BaseEditorItem
{
    Q_OBJECT
public:
    TextEditorItem(const TextElementPtr& element, QGraphicsItem* parent = nullptr);

    // 双击进入编辑模式
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event) override;

    // 编辑模式相关
    bool isEditing() const;
    void startEditing();
    void finishEditing();

    // 编辑模式下跳过元素渲染（由QGraphicsTextItem显示文本）
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
               QWidget* widget) override;

    // 覆盖基类：boundingRect贴合实际文本内容区域，
    // 而非容器rect（rect通常比文本宽）。
    QRectF boundingRect() const override;

    // 覆盖基类：在文本实际内容区域绘制选中边框
    void drawSelectionBorder(QPainter* painter) override;

    // 类型标识
    enum { Type = UserType + 2 };
    int type() const override { return Type; }

signals:
    // 编辑模式下选中文本时，请求显示格式化工具栏
    // globalPos: 请求显示工具栏的全局屏幕坐标
    void textFormatRequested(const QPointF& globalPos);

protected:
    // 事件过滤器：监听编辑子Item的焦点丢失事件
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    // 计算文本在item本地坐标系中的实际内容区域。
    // 复制ElementRenderer::renderText的QTextLayout布局逻辑，
    // 得到文本内容的实际包围盒（含对齐偏移）。
    // 返回无效QRectF表示文本为空（调用方应回退到基类行为）。
    QRectF computeActualTextRect() const;

    QGraphicsTextItem* m_editItem;  // 编辑模式用的文本编辑子Item（懒创建）
    bool m_editing;                 // 是否处于编辑模式
};

#endif // TEXTEDITORITEM_H
