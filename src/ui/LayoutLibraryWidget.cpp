#include "LayoutLibraryWidget.h"

#include <QListWidgetItem>
#include <QPainter>
#include <QPen>
#include <QBrush>
#include <QColor>

namespace {
    // 绘制单书版式图标：一个大矩形框表示单页单书
    QIcon createSingleLayoutIcon()
    {
        QPixmap pix(32, 32);
        pix.fill(Qt::transparent);

        QPainter p(&pix);
        p.setRenderHint(QPainter::Antialiasing, true);
        QPen pen(QColor(80, 120, 200));
        pen.setWidth(2);
        p.setPen(pen);
        p.setBrush(QBrush(QColor(180, 210, 240)));
        // 居中绘制 16x24 矩形
        p.drawRect(QRect(8, 4, 16, 24));
        p.end();

        return QIcon(pix);
    }

    // 绘制多书版式图标：三个小矩形框表示多书排列
    QIcon createMultiLayoutIcon()
    {
        QPixmap pix(32, 32);
        pix.fill(Qt::transparent);

        QPainter p(&pix);
        p.setRenderHint(QPainter::Antialiasing, true);
        QPen pen(QColor(200, 120, 80));
        pen.setWidth(1);
        p.setPen(pen);
        p.setBrush(QBrush(QColor(240, 200, 170)));
        // 三个小矩形纵向排列
        p.drawRect(QRect(6, 4, 20, 7));
        p.drawRect(QRect(6, 13, 20, 7));
        p.drawRect(QRect(6, 22, 20, 7));
        p.end();

        return QIcon(pix);
    }
} // namespace

// ============================================================
// LayoutLibraryWidget
// ============================================================
LayoutLibraryWidget::LayoutLibraryWidget(QWidget* parent)
    : QListWidget(parent)
{
    // 使用列表模式（垂直列表，文字在图标右侧）
    setViewMode(QListWidget::ListMode);
    setIconSize(QSize(32, 32));
    setUniformItemSizes(true);
    setResizeMode(QListWidget::Adjust);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setMovement(QListWidget::Static);   // 禁止拖动重排
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    setupItems();

    // 点击条目时根据 UserRole 获取 LayoutMode 并发射信号
    connect(this, &QListWidget::itemClicked, this, [this](QListWidgetItem* item) {
        if (!item) return;
        ::LayoutMode mode = static_cast<::LayoutMode>(item->data(Qt::UserRole).toInt());
        emit layoutSelected(mode);
    });
}

void LayoutLibraryWidget::setupItems()
{
    // 单书版式条目
    QListWidgetItem* singleItem = new QListWidgetItem(this);
    singleItem->setText(QString::fromUtf8("单书版式"));
    singleItem->setToolTip(QString::fromUtf8("每页展示一本书的详细信息"));
    singleItem->setData(Qt::UserRole, static_cast<int>(::LayoutMode::SingleColumn));
    singleItem->setIcon(createSingleLayoutIcon());
    singleItem->setSizeHint(QSize(180, 50));
    singleItem->setTextAlignment(Qt::AlignCenter);
    singleItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);

    // 多书版式条目
    QListWidgetItem* multiItem = new QListWidgetItem(this);
    multiItem->setText(QString::fromUtf8("多书版式"));
    multiItem->setToolTip(QString::fromUtf8("每页紧凑展示多本书的列表信息"));
    multiItem->setData(Qt::UserRole, static_cast<int>(::LayoutMode::MultiColumn));
    multiItem->setIcon(createMultiLayoutIcon());
    multiItem->setSizeHint(QSize(180, 50));
    multiItem->setTextAlignment(Qt::AlignCenter);
    multiItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
}
