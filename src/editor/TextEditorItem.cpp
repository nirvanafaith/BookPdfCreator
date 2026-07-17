#include "TextEditorItem.h"
#include "Commands.h"
#include "EditorScene.h"

#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <QGraphicsSceneMouseEvent>
#include <QEvent>
#include <QFocusEvent>
#include <QTextLayout>
#include <QTextLine>
#include <QTextCursor>
#include <QTextBlockFormat>
#include <QTextDocument>
#include <QAbstractTextDocumentLayout>
#include <QFontMetrics>
#include <QBrush>
#include <QPen>
#include <QColor>
#include <QtGlobal>

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

    if (!m_editItem) {
        m_editItem = new QGraphicsTextItem(this);
        m_editItem->installEventFilter(this);
    }

    const TextElementData* textData =
        static_cast<const TextElementData*>(m_element.constData());

    QFont editFont = textData->font();
    qreal letterSpacing = textData->letterSpacing();
    if (qAbs(letterSpacing) > 0.001) {
        editFont.setLetterSpacing(QFont::PercentageSpacing, 100.0 + letterSpacing);
    }

    m_editItem->setPlainText(textData->text());
    m_editItem->setFont(editFont);

    QTextDocument* doc = m_editItem->document();
    doc->setDocumentMargin(0);
    doc->setDefaultFont(editFont);
    m_editItem->setDefaultTextColor(textData->textColor());

    QRectF rect = textData->rect();
    Qt::Alignment align = textData->alignment();

    qreal lineHeight = textData->lineHeight();
    QFontMetrics fm(editFont);
    if (lineHeight <= 0) {
        lineHeight = fm.height();
    }

    m_editItem->setTextWidth(rect.width());

    QTextOption option = doc->defaultTextOption();
    option.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
    option.setAlignment(align);
    option.setUseDesignMetrics(false);
    doc->setDefaultTextOption(option);

    {
        QTextCursor cursor(doc);
        cursor.select(QTextCursor::Document);
        QTextBlockFormat fmt;
        fmt.setLineHeight(lineHeight, QTextBlockFormat::FixedHeight);
        fmt.setAlignment(align);
        fmt.setTopMargin(0);
        fmt.setBottomMargin(0);
        fmt.setLeftMargin(0);
        fmt.setRightMargin(0);
        fmt.setIndent(0);
        cursor.mergeBlockFormat(fmt);
    }

    doc->setTextWidth(rect.width());
    doc->adjustSize();
    qreal docHeight = doc->size().height();
    if (docHeight <= 0) {
        docHeight = lineHeight;
    }

    qreal vOffset = 0.0;
    if (align & Qt::AlignVCenter) {
        vOffset = (rect.height() - docHeight) / 2.0;
        if (vOffset < 0) vOffset = 0.0;
    } else if (align & Qt::AlignBottom) {
        vOffset = rect.height() - docHeight;
        if (vOffset < 0) vOffset = 0.0;
    }

    m_editItem->setTextInteractionFlags(Qt::TextEditorInteraction);
    m_editItem->setPos(0, vOffset);
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
        // 将光标定位到双击位置（而非文本起始），优化编辑手感
        if (m_editItem && m_editItem->document() && m_editItem->document()->documentLayout()) {
            QPointF localPos = event->pos();
            QTextCursor cursor = m_editItem->textCursor();
            int pos = m_editItem->document()->documentLayout()->hitTest(localPos, Qt::FuzzyHit);
            if (pos >= 0) {
                cursor.setPosition(pos);
                m_editItem->setTextCursor(cursor);
            }
        }
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

// ============================================================
// computeActualTextRect - 计算文本实际内容区域
//
// 复制ElementRenderer::renderText中的QTextLayout布局逻辑，
// 得到文本内容在item本地坐标系（0,0=rect.topLeft）中的实际包围盒。
//
// 布局参数与渲染完全一致：
//   - lineWidth = rect.width()
//   - lineHeight = text->lineHeight()（0时自动用QFontMetrics::height()）
//   - 水平对齐：左/中/右（justify按左处理，与渲染一致）
//   - 垂直对齐：上/中/下
//   - charFormats：通过setAdditionalFormats应用混合格式
//
// 包围盒计算：
//   - 宽度 = 所有行naturalTextWidth的最大值
//   - 高度 = totalHeight（行数 * lineHeight，与渲染的vOffset计算一致）
//   - X偏移：根据对齐方式和最大行宽计算（与最宽行的渲染位置一致）
//   - Y偏移 = vOffset（根据垂直对齐和totalHeight计算）
//
// 返回无效QRectF表示文本为空，调用方应回退到基类实现。
// ============================================================
QRectF TextEditorItem::computeActualTextRect() const
{
    const TextElementData* text =
        static_cast<const TextElementData*>(m_element.constData());

    if (text->text().isEmpty()) {
        return QRectF();  // 空文本，回退
    }

    QRectF rect = text->rect();
    QFont font = text->font();
    Qt::Alignment align = text->alignment();

    // 应用字间距（与渲染一致）
    qreal letterSpacing = text->letterSpacing();
    if (qAbs(letterSpacing) > 0.001) {
        font.setLetterSpacing(QFont::PercentageSpacing, 100.0 + letterSpacing);
    }

    // 行高计算：0=自动用QFontMetrics::height()，>0=固定值（与渲染一致）
    QFontMetrics fm(font);
    qreal lineHeight = text->lineHeight();
    if (lineHeight <= 0) {
        lineHeight = fm.height();
    }

    // 创建文本布局（device=nullptr，仅用于布局计算不用于绘制）
    QTextLayout textLayout(text->text(), font, nullptr);

    // 应用字符级格式（CharFormat：混合粗体/斜体/字号/颜色，与渲染一致）
    QList<TextElementData::CharFormat> charFormats = text->charFormats();
    if (!charFormats.isEmpty()) {
        QList<QTextLayout::FormatRange> formats;
        for (const TextElementData::CharFormat& cf : charFormats) {
            QTextLayout::FormatRange range;
            range.start = cf.start;
            range.length = cf.length;
            range.format.setFont(cf.font);
            range.format.setForeground(QBrush(cf.color));
            formats.append(range);
        }
        textLayout.setAdditionalFormats(formats);
    }

    // 布局文本行（与渲染逻辑完全一致）
    textLayout.beginLayout();
    qreal y = 0.0;
    qreal maxLineWidth = 0.0;
    int lineCount = 0;

    while (true) {
        QTextLine line = textLayout.createLine();
        if (!line.isValid()) break;

        line.setLineWidth(rect.width());
        maxLineWidth = qMax(maxLineWidth, line.naturalTextWidth());
        y += lineHeight;
        lineCount++;
    }
    textLayout.endLayout();

    if (lineCount == 0) {
        return QRectF();  // 无有效行，回退
    }

    qreal totalHeight = y;  // = lineCount * lineHeight（与渲染的totalHeight一致）

    // 垂直对齐：计算Y偏移（与渲染逻辑完全一致）
    qreal vOffset = 0.0;
    if (align & Qt::AlignVCenter) {
        vOffset = (rect.height() - totalHeight) / 2.0;
        if (vOffset < 0) vOffset = 0.0;
    } else if (align & Qt::AlignBottom) {
        vOffset = rect.height() - totalHeight;
        if (vOffset < 0) vOffset = 0.0;
    }

    // 水平对齐：根据最宽行计算包围盒X偏移
    // （最宽行的渲染位置即为包围盒的左/右边界，各对齐方式下均成立）
    qreal x = 0.0;
    if (align & Qt::AlignHCenter) {
        x = (rect.width() - maxLineWidth) / 2.0;
    } else if (align & Qt::AlignRight) {
        x = rect.width() - maxLineWidth;
    }
    // else（AlignLeft / AlignJustify）: x = 0（与渲染一致）

    // 包围盒在item本地坐标系（0,0 = rect.topLeft）
    return QRectF(x, vOffset, maxLineWidth, totalHeight);
}

// ============================================================
// boundingRect - 贴合文本实际内容区域的包围矩形
//
// 元素rect通常比文本内容宽（尤其左对齐时右侧留白），
// 此方法返回文本实际区域+SELECTION_MARGIN。
// 文本为空时回退到基类实现（整个rect+margin）。
// ============================================================
QRectF TextEditorItem::boundingRect() const
{
    QRectF textRect = computeActualTextRect();
    if (!textRect.isValid()) {
        // 文本为空或无有效行，回退到基类（容器rect + margin）
        return BaseEditorItem::boundingRect();
    }

    const qreal m = SELECTION_MARGIN;
    return QRectF(textRect.x() - m, textRect.y() - m,
                  textRect.width() + 2.0 * m, textRect.height() + 2.0 * m);
}

// ============================================================
// drawSelectionBorder - 在文本实际内容区域绘制选中边框
//
// 覆盖基类实现，将边框绘制在文本实际内容区域而非容器rect边缘。
// 文本为空时回退到基类实现。
// ============================================================
void TextEditorItem::drawSelectionBorder(QPainter* painter)
{
    QRectF textRect = computeActualTextRect();
    if (!textRect.isValid()) {
        // 文本为空或无有效行，回退到基类（在容器rect边缘绘制）
        BaseEditorItem::drawSelectionBorder(painter);
        return;
    }

    painter->save();
    QPen pen(QColor(25, 118, 210), 1, Qt::DashLine);
    painter->setPen(pen);
    painter->setBrush(Qt::NoBrush);

    painter->drawRect(textRect);

    painter->restore();
}
