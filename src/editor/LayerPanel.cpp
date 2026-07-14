#include "LayerPanel.h"

#include <QListWidget>
#include <QListWidgetItem>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QToolButton>
#include <QMenu>
#include <QAction>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QMouseEvent>
#include <QEvent>
#include <QDropEvent>
#include <QTimer>
#include <QPair>

// ============================================================
// 图标生成辅助函数（匿名命名空间，仅本文件可见）
//
// 不依赖资源文件或主题图标，全部用 QPainter 绘制，
// 保证在 Windows 7 + Qt 5.12 环境下一致渲染。
// ============================================================
namespace {

// ---- 类型图标 ----
QIcon makeTypeIcon(PageElementData::ElementType type)
{
    QPixmap pm(18, 18);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing, true);

    const QColor c("#2B7CCE");
    QPen pen(c, 1.6);
    p.setPen(pen);
    p.setBrush(Qt::NoBrush);

    switch (type) {
    case PageElementData::Text: {
        // 绘制 "T" 字形
        QPainterPath path;
        path.moveTo(3, 4);
        path.lineTo(15, 4);   // 横线
        path.moveTo(9, 4);
        path.lineTo(9, 14);   // 竖线
        p.drawPath(path);
        break;
    }
    case PageElementData::Image: {
        // 绘制图片框：圆角矩形 + 太阳 + 山形
        p.drawRoundedRect(2, 3, 14, 12, 2, 2);
        p.setBrush(c);
        p.setPen(Qt::NoPen);
        p.drawEllipse(QPointF(6, 7), 1.3, 1.3);  // 太阳
        p.setPen(pen);
        p.setBrush(Qt::NoBrush);
        QPainterPath mtn;
        mtn.moveTo(3, 14);
        mtn.lineTo(7, 9);
        mtn.lineTo(10, 12);
        mtn.lineTo(13, 8);
        mtn.lineTo(16, 14);
        mtn.closeSubpath();
        p.drawPath(mtn);
        break;
    }
    case PageElementData::Shape: {
        // 绘制形状：矩形 + 圆
        p.drawRect(2, 4, 8, 8);
        p.drawEllipse(QRectF(10, 6, 6, 6));
        break;
    }
    }
    return QIcon(pm);
}

// ---- 可见性图标（眼睛）----
QIcon makeEyeIcon(bool visible)
{
    QPixmap pm(18, 18);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing, true);

    const QColor c(visible ? "#333333" : "#999999");
    QPen pen(c, 1.5);
    p.setPen(pen);
    p.setBrush(Qt::NoBrush);

    // 眼睛轮廓：两条三次贝塞尔曲线构成橄榄形
    QPainterPath eye;
    eye.moveTo(2, 9);
    eye.cubicTo(5, 4, 13, 4, 16, 9);
    eye.cubicTo(13, 14, 5, 14, 2, 9);
    p.drawPath(eye);

    if (visible) {
        // 瞳孔
        p.setBrush(c);
        p.setPen(Qt::NoPen);
        p.drawEllipse(QPointF(9, 9), 2.0, 2.0);
    } else {
        // 斜线表示隐藏
        p.setPen(QPen(c, 1.5));
        p.drawLine(3, 15, 15, 3);
    }
    return QIcon(pm);
}

// ---- 锁定图标 ----
QIcon makeLockIcon(bool locked)
{
    QPixmap pm(18, 18);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing, true);

    const QColor c(locked ? "#CC6600" : "#999999");
    QPen pen(c, 1.5);
    p.setPen(pen);
    p.setBrush(Qt::NoBrush);

    // 锁体
    p.setBrush(QBrush(c.lighter(160)));
    p.drawRoundedRect(4, 8, 10, 7, 1, 1);
    p.setBrush(Qt::NoBrush);

    // 锁扣：上方半圆弧
    if (locked) {
        // 闭合锁：完整半圆弧
        p.drawArc(QRect(5, 3, 8, 8), 0 * 16, 180 * 16);
    } else {
        // 开锁：弧偏向右上方
        p.drawArc(QRect(7, 3, 8, 8), 0 * 16, 90 * 16);
    }

    if (locked) {
        // 锁孔
        p.setBrush(QColor("#FFFFFF"));
        p.setPen(Qt::NoPen);
        p.drawEllipse(QPointF(9, 11), 1.0, 1.0);
    }
    return QIcon(pm);
}

} // namespace

// ============================================================
// LayerItemWidget - 单个图层条目的自定义控件
//
// 不使用 Q_OBJECT 宏（避免在 .cpp 中引入额外 MOC 依赖）。
// 可见性与锁定按钮为 checkable QToolButton，其 toggled 信号
// 由 LayerPanel 在创建条目时连接到对应 lambda。
//
// 点击非按钮区域会选中对应的 QListWidgetItem（默认情况下
// setItemWidget 的控件不会把鼠标事件转发给列表视图选中条目，
// 需在此手动处理）。
// ============================================================
class LayerItemWidget : public QWidget
{
public:
    LayerItemWidget(const QString& elementId,
                    PageElementData::ElementType type,
                    const QString& name,
                    bool visible, bool locked,
                    QWidget* parent = nullptr)
        : QWidget(parent)
        , m_elementId(elementId)
        , m_type(type)
        , m_visible(visible)
        , m_locked(locked)
        , m_item(nullptr)
    {
        // 类型图标
        m_typeIcon = new QLabel(this);
        m_typeIcon->setPixmap(makeTypeIcon(type).pixmap(16, 16));
        m_typeIcon->setFixedSize(20, 20);

        // 元素名称（过长显示省略号）
        m_nameLabel = new QLabel(name, this);
        m_nameLabel->setToolTip(name);
        m_nameLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        // 让名称标签的鼠标点击穿透到本 widget（以便选中条目）
        m_nameLabel->setAttribute(Qt::WA_TransparentForMouseEvents, true);
        m_typeIcon->setAttribute(Qt::WA_TransparentForMouseEvents, true);

        // 可见性按钮
        m_visibilityBtn = new QToolButton(this);
        m_visibilityBtn->setFixedSize(22, 22);
        m_visibilityBtn->setIcon(makeEyeIcon(visible));
        m_visibilityBtn->setToolTip(visible ? LayerPanel::tr("隐藏") : LayerPanel::tr("显示"));
        m_visibilityBtn->setAutoRaise(true);
        m_visibilityBtn->setCheckable(true);
        m_visibilityBtn->setChecked(visible);

        // 锁定按钮
        m_lockBtn = new QToolButton(this);
        m_lockBtn->setFixedSize(22, 22);
        m_lockBtn->setIcon(makeLockIcon(locked));
        m_lockBtn->setToolTip(locked ? LayerPanel::tr("解锁") : LayerPanel::tr("锁定"));
        m_lockBtn->setAutoRaise(true);
        m_lockBtn->setCheckable(true);
        m_lockBtn->setChecked(locked);

        // 水平布局
        QHBoxLayout* layout = new QHBoxLayout(this);
        layout->setContentsMargins(4, 1, 4, 1);
        layout->setSpacing(4);
        layout->addWidget(m_typeIcon);
        layout->addWidget(m_nameLabel, 1);
        layout->addWidget(m_visibilityBtn);
        layout->addWidget(m_lockBtn);

        // 按钮状态变化时同步图标与提示
        connect(m_visibilityBtn, &QToolButton::toggled, this, [this](bool checked) {
            m_visible = checked;
            m_visibilityBtn->setIcon(makeEyeIcon(checked));
            m_visibilityBtn->setToolTip(checked ? LayerPanel::tr("隐藏") : LayerPanel::tr("显示"));
        });
        connect(m_lockBtn, &QToolButton::toggled, this, [this](bool checked) {
            m_locked = checked;
            m_lockBtn->setIcon(makeLockIcon(checked));
            m_lockBtn->setToolTip(checked ? LayerPanel::tr("解锁") : LayerPanel::tr("锁定"));
        });
    }

    // 绑定对应的 QListWidgetItem（用于点击选中）
    void setListItem(QListWidgetItem* item) { m_item = item; }

    QString elementId() const { return m_elementId; }
    QString elementName() const { return m_nameLabel->text(); }
    void setElementName(const QString& name) {
        m_nameLabel->setText(name);
        m_nameLabel->setToolTip(name);
    }

    bool isElementVisible() const { return m_visible; }
    bool isElementLocked() const { return m_locked; }

    void setElementVisible(bool visible) { m_visibilityBtn->setChecked(visible); }
    void setElementLocked(bool locked) { m_lockBtn->setChecked(locked); }

    QToolButton* visibilityButton() const { return m_visibilityBtn; }
    QToolButton* lockButton() const { return m_lockBtn; }

protected:
    void mousePressEvent(QMouseEvent* event) override
    {
        // 点击非按钮区域时选中对应的列表条目
        if (event->button() == Qt::LeftButton && m_item) {
            QListWidget* list = m_item->listWidget();
            if (list) {
                list->setCurrentItem(m_item, QItemSelectionModel::ClearAndSelect);
            }
        }
        // 必须ignore而非调用QWidget::mousePressEvent(event)，否则事件被accept，
        // QListWidget viewport无法接收鼠标按下事件，导致InternalMove拖拽无法启动。
        // ignore后事件传播到QListWidget viewport，由其处理选中+拖拽发起。
        event->ignore();
    }

private:
    QString m_elementId;
    PageElementData::ElementType m_type;
    bool m_visible;
    bool m_locked;
    QListWidgetItem* m_item;
    QLabel* m_typeIcon;
    QLabel* m_nameLabel;
    QToolButton* m_visibilityBtn;
    QToolButton* m_lockBtn;
};

// ============================================================
// LayerPanel 实现
// ============================================================

// ---- 数据角色常量 ----
// Qt::UserRole: 存储元素 ID (QString)
// Qt::UserRole + 1: 存储元素当前 zOrder (int)，用于拖拽后比较变化
static const int kElementIdRole = Qt::UserRole;
static const int kZOrderRole    = Qt::UserRole + 1;
static const int kIsBackgroundRole = Qt::UserRole + 2;  // 标记是否为背景项

LayerPanel::LayerPanel(QWidget* parent)
    : QWidget(parent)
    , m_listWidget(nullptr)
    , m_syncing(false)
    , m_backgroundItem(nullptr)
{
    setupUi();
    setupConnections();
    setMinimumWidth(180);
}

void LayerPanel::setupUi()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    m_listWidget = new QListWidget(this);
    m_listWidget->setObjectName(QStringLiteral("layerListWidget"));
    // 拖拽重排序：内部移动模式
    m_listWidget->setDragDropMode(QAbstractItemView::InternalMove);
    m_listWidget->setDragDropOverwriteMode(false);
    m_listWidget->setDefaultDropAction(Qt::MoveAction);
    m_listWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    m_listWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_listWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    m_listWidget->setAlternatingRowColors(true);
    m_listWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    // 隐藏内置复选框列（使用自定义按钮）
    m_listWidget->setUniformItemSizes(false);

    mainLayout->addWidget(m_listWidget);
    setLayout(mainLayout);
}

void LayerPanel::setupConnections()
{
    connect(m_listWidget, &QListWidget::itemSelectionChanged,
            this, &LayerPanel::onItemSelectionChanged);
    connect(m_listWidget, &QListWidget::itemChanged,
            this, &LayerPanel::onItemChanged);
    connect(m_listWidget, &QWidget::customContextMenuRequested,
            this, &LayerPanel::onContextMenu);

    // 监听视口的 Drop 事件以触发拖拽重排序处理
    m_listWidget->viewport()->installEventFilter(this);
}

void LayerPanel::updateLayers(const QList<PageElementPtr>& elements)
{
    // 记录当前选中元素 ID，用于重建后恢复选中
    QString selectedId;
    if (QListWidgetItem* cur = m_listWidget->currentItem()) {
        selectedId = cur->data(kElementIdRole).toString();
    }

    m_syncing = true;

    // 清除旧条目（setItemWidget 设置的 widget 随 item 一并删除）
    m_listWidget->clear();
    m_backgroundItem = nullptr;  // clear()已删除该项，重置指针

    // 按 zOrder 降序填充：elements 已升序，从末尾向前遍历，
    // 最先添加（z 最大）出现在列表顶部（row 0）。
    for (int i = elements.size() - 1; i >= 0; --i) {
        const PageElementPtr& elem = elements.at(i);

        QListWidgetItem* item = new QListWidgetItem;
        item->setData(kElementIdRole, elem->id());
        item->setData(kZOrderRole, elem->zOrder());
        // 不设置 item 文本——显示由自定义 widget 负责
        m_listWidget->addItem(item);

        // 创建自定义控件
        LayerItemWidget* widget = new LayerItemWidget(
            elem->id(), elem->elementType(), elem->name(),
            elem->isVisible(), elem->isLocked(), m_listWidget);
        widget->setListItem(item);
        m_listWidget->setItemWidget(item, widget);

        // 确保行高匹配控件尺寸
        item->setSizeHint(widget->sizeHint());

        // 连接可见性按钮
        connect(widget->visibilityButton(), &QToolButton::toggled, this,
                [this, widget](bool checked) {
                    if (m_syncing) return;
                    emit elementVisibilityChanged(widget->elementId(), checked);
                });
        // 连接锁定按钮
        connect(widget->lockButton(), &QToolButton::toggled, this,
                [this, widget](bool checked) {
                    if (m_syncing) return;
                    emit elementLockChanged(widget->elementId(), checked);
                });
    }

    // 恢复选中状态
    if (!selectedId.isEmpty()) {
        for (int i = 0; i < m_listWidget->count(); ++i) {
            QListWidgetItem* it = m_listWidget->item(i);
            if (it->data(kElementIdRole).toString() == selectedId) {
                m_listWidget->setCurrentRow(i, QItemSelectionModel::ClearAndSelect);
                break;
            }
        }
    }

    m_syncing = false;
}

void LayerPanel::clearLayers()
{
    m_syncing = true;
    m_listWidget->clear();
    m_backgroundItem = nullptr;  // clear()已删除该项，重置指针
    m_syncing = false;
}

void LayerPanel::setBackgroundItem(const QString& name, bool visible)
{
    // 移除旧的背景项（若存在）
    if (m_backgroundItem) {
        delete m_listWidget->takeItem(m_listWidget->row(m_backgroundItem));
        m_backgroundItem = nullptr;
    }

    m_syncing = true;

    // 在列表末尾（最底层）插入背景项
    QListWidgetItem* item = new QListWidgetItem;
    item->setData(kElementIdRole, QStringLiteral("__background__"));
    item->setData(kZOrderRole, -10000);
    item->setData(kIsBackgroundRole, true);
    m_listWidget->addItem(item);

    // 背景项使用Shape类型图标（矩形）表示纸张
    LayerItemWidget* widget = new LayerItemWidget(
        QStringLiteral("__background__"), PageElementData::Shape, name,
        visible, false, m_listWidget);
    widget->setListItem(item);
    m_listWidget->setItemWidget(item, widget);
    item->setSizeHint(widget->sizeHint());

    // 连接可见性按钮
    connect(widget->visibilityButton(), &QToolButton::toggled, this,
            [this, widget](bool checked) {
                if (m_syncing) return;
                emit elementVisibilityChanged(widget->elementId(), checked);
            });

    m_backgroundItem = item;
    m_syncing = false;
}

void LayerPanel::onItemSelectionChanged()
{
    if (m_syncing) return;
    QListWidgetItem* cur = m_listWidget->currentItem();
    if (!cur) return;
    emit elementSelected(cur->data(kElementIdRole).toString());
}

void LayerPanel::onItemChanged(QListWidgetItem* item)
{
    // 当前未使用 QListWidgetItem 的内建复选框或文本编辑功能，
    // 可见性与锁定由 LayerItemWidget 的按钮处理。
    // 此槽保留以支持未来扩展（如双击重命名）。重建期间由 m_syncing 屏蔽。
    if (!item || m_syncing) return;
    // no-op
}

void LayerPanel::onContextMenu(const QPoint& pos)
{
    QListWidgetItem* item = m_listWidget->itemAt(pos);
    if (!item) return;

    // 背景项不支持右键菜单操作
    if (item->data(kIsBackgroundRole).toBool()) return;

    QString id = item->data(kElementIdRole).toString();

    QMenu menu(this);
    QAction* actTop    = menu.addAction(tr("置顶"));
    QAction* actBottom = menu.addAction(tr("置底"));
    menu.addSeparator();
    QAction* actUp   = menu.addAction(tr("上移一层"));
    QAction* actDown = menu.addAction(tr("下移一层"));
    menu.addSeparator();
    QAction* actDup = menu.addAction(tr("复制"));
    QAction* actDel = menu.addAction(tr("删除"));

    QAction* sel = menu.exec(m_listWidget->viewport()->mapToGlobal(pos));
    if (!sel) return;

    if (sel == actTop)         emit bringToFront(id);
    else if (sel == actBottom) emit sendToBack(id);
    else if (sel == actUp)     emit moveUp(id);
    else if (sel == actDown)   emit moveDown(id);
    else if (sel == actDup)    emit elementDuplicated(id);
    else if (sel == actDel)    emit elementDeleted(id);
}

void LayerPanel::onDragReordered()
{
    const int count = m_listWidget->count();
    if (count == 0) return;

    m_syncing = true;

    // 背景项必须始终在最底层（最后一行）。如果被拖动则移回末尾。
    if (m_backgroundItem) {
        int bgRow = m_listWidget->row(m_backgroundItem);
        int lastRow = count - 1;
        if (bgRow != lastRow) {
            // 将背景项移到最后一行
            m_listWidget->takeItem(bgRow);
            m_listWidget->insertItem(lastRow, m_backgroundItem);
        }
    }

    // 收集 zOrder 变化
    // 列表顶部（row 0）对应最高 zOrder
    // 非背景项数量 = count - 1（若存在背景项），否则 = count
    QList<QPair<QString, int>> changes;
    int nonBgCount = m_backgroundItem ? count - 1 : count;
    for (int i = 0; i < count; ++i) {
        QListWidgetItem* item = m_listWidget->item(i);
        // 跳过背景项，不参与zOrder重分配
        if (item->data(kIsBackgroundRole).toBool()) continue;
        QString id = item->data(kElementIdRole).toString();
        int newZ = nonBgCount - 1 - i;
        int oldZ = item->data(kZOrderRole).toInt();
        if (newZ != oldZ) {
            item->setData(kZOrderRole, newZ);
            changes.append(qMakePair(id, newZ));
        }
    }

    m_syncing = false;

    // 发出批量zOrder变化信号，控制器用beginMacro/endMacro包裹实现一次性撤销
    if (!changes.isEmpty()) {
        emit elementZOrderBatchChanged(changes);
    }
}

bool LayerPanel::eventFilter(QObject* obj, QEvent* event)
{
    if (obj == m_listWidget->viewport() && event->type() == QEvent::Drop) {
        // 让默认 drop 处理先执行，再异步触发重排序
        QTimer::singleShot(0, this, [this]() { onDragReordered(); });
    }
    return QWidget::eventFilter(obj, event);
}
