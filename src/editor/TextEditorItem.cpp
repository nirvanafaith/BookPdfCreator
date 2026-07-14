#include "TextEditorItem.h"
#include "Commands.h"
#include "EditorScene.h"

#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <QGraphicsSceneMouseEvent>
#include <QEvent>
#include <QFocusEvent>

// ============================================================
// 构造函数
//
// TextElementPtr -> PageElementPtr 转换：
// 通过clone()获取非const的TextElementData*（新建对象），
// 隐式上转为PageElementData*，构造PageElementPtr传给基类。
// BaseEditorItem共享此clone数据（引用计数），修改时再clone。
// ============================================================
TextEditorItem::TextEditorItem(const TextElementPtr& element, QGraphicsItem* parent)
    : QObject()
    , BaseEditorItem(PageElementPtr(element.constData()->clone()), parent)
    , m_editItem(nullptr)
    , m_editing(false)
{
}

// ============================================================
// isEditing - 是否处于编辑模式
// ============================================================
bool TextEditorItem::isEditing() const
{
    return m_editing;
}

// ============================================================
// startEditing - 进入编辑模式
//
// 创建（或复用）QGraphicsTextItem作为子Item，
// 设置为可编辑交互模式并获取焦点。
// 用户可在其中直接编辑文本，支持光标移动和文本选择。
// ============================================================
void TextEditorItem::startEditing()
{
    if (m_editing) return;
    m_editing = true;

    // 懒创建编辑子Item
    if (!m_editItem) {
        m_editItem = new QGraphicsTextItem(this);
        // 安装事件过滤器以监听焦点丢失（点击外部时退出编辑）
        m_editItem->installEventFilter(this);
    }

    // 从元素数据初始化编辑器内容
    const TextElementData* textData =
        static_cast<const TextElementData*>(m_element.constData());
    m_editItem->setPlainText(textData->text());
    m_editItem->setFont(textData->font());
    m_editItem->setDefaultTextColor(textData->textColor());
    // 文本宽度=元素宽度，实现自动换行
    m_editItem->setTextWidth(m_element.constData()->rect().width());
    // 启用文本编辑交互（光标、选择、键盘输入）
    m_editItem->setTextInteractionFlags(Qt::TextEditorInteraction);
    // 编辑器位于item本地坐标原点
    m_editItem->setPos(0, 0);
    m_editItem->show();
    m_editItem->setFocus();

    update();
}

// ============================================================
// finishEditing - 退出编辑模式
//
// 将QGraphicsTextItem中的文本通过TextEditCommand提交到撤销栈，
// 由命令的redo()完成元素数据修改与Item重建（命令即操作模式）。
// 隐藏编辑子Item（保留供下次复用，避免频繁创建销毁）。
// m_editing标志防止重入（clearFocus可能触发FocusOut）。
//
// 生命周期注意：
//   TextEditCommand::redo()内部会删除本Item并创建新Item，
//   因此push后不可再访问this的任何成员，必须立即返回。
// ============================================================
void TextEditorItem::finishEditing()
{
    if (!m_editing) return;
    m_editing = false;

    if (!m_editItem) {
        syncFromData();
        return;
    }

    // 获取新文本（来自编辑器）和旧文本（来自元素数据）
    QString newText = m_editItem->toPlainText();
    const TextElementData* textData =
        static_cast<const TextElementData*>(m_element.constData());
    QString oldText = textData->text();

    // 隐藏编辑子Item（不删除，供下次编辑复用）
    // 注意：若push命令，redo会重建并删除本Item，m_editItem作为子Item随之销毁
    m_editItem->setTextInteractionFlags(Qt::NoTextInteraction);
    m_editItem->clearFocus();
    m_editItem->hide();

    // 文本未变化：仅需同步显示，不修改数据，不push命令
    if (oldText == newText) {
        syncFromData();
        return;
    }

    // 通过撤销栈push TextEditCommand执行文本修改（命令即操作模式）
    // redo()会clone元素数据、设置新文本、删除旧Item、创建新Item
    // 注意：push后本Item被redo删除，不可再访问this
    EditorScene* editorScene = qobject_cast<EditorScene*>(scene());
    if (editorScene && editorScene->undoStack()) {
        QString elementId = m_element.constData()->id();
        editorScene->undoStack()->push(
            new TextEditCommand(editorScene, elementId, oldText, newText));
        return;  // 本Item已被删除，直接返回
    }

    // 回退路径：无撤销栈时，直接修改元素数据
    PageElementData* cloned = m_element.constData()->clone();
    TextElementData* textClone = static_cast<TextElementData*>(cloned);
    textClone->setText(newText);
    m_element = PageElementPtr(cloned);

    syncFromData();
}

// ============================================================
// paint - 编辑模式下跳过元素渲染
//
// 编辑模式时元素文本由QGraphicsTextItem显示，避免双重绘制。
// 非编辑模式委托给基类paint渲染元素内容。
// ============================================================
void TextEditorItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
                           QWidget* widget)
{
    if (m_editing) {
        // 编辑模式：仅绘制选中边框，文本由m_editItem显示
        if (option->state & QStyle::State_Selected) {
            drawSelectionBorder(painter);
        }
        return;
    }
    // 非编辑模式：基类正常渲染
    BaseEditorItem::paint(painter, option, widget);
}

// ============================================================
// mouseDoubleClickEvent - 双击进入编辑模式
// ============================================================
void TextEditorItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        startEditing();
        event->accept();
    } else {
        BaseEditorItem::mouseDoubleClickEvent(event);
    }
}

// ============================================================
// eventFilter - 监听编辑子Item的焦点丢失
//
// 当用户点击编辑区域外部时，QGraphicsTextItem失去焦点，
// 触发FocusOut事件，此时自动退出编辑模式。
// ============================================================
bool TextEditorItem::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == m_editItem && event->type() == QEvent::FocusOut) {
        finishEditing();
        // 不拦截事件，让QGraphicsTextItem正常处理焦点丢失
    }
    return QObject::eventFilter(watched, event);
}
