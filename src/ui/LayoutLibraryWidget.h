#ifndef LAYOUTLIBRARYWIDGET_H
#define LAYOUTLIBRARYWIDGET_H

#include <QListWidget>
#include "models/LayoutMode.h"

// ============================================================
// LayoutLibraryWidget - 版式库列表组件
//
// 展示可选的页面版式条目（单书版式 / 多书版式），
// 用户点击条目时发射 layoutSelected 信号，携带对应的 LayoutMode。
// 条目数据存储（QListWidgetItem::setData）：
//   Qt::UserRole : LayoutMode 的整数值
// ============================================================
class LayoutLibraryWidget : public QListWidget
{
    Q_OBJECT
public:
    explicit LayoutLibraryWidget(QWidget* parent = nullptr);

signals:
    // 用户点击版式条目时发射，携带版式模式
    // 注意：使用 ::LayoutMode 限定全局枚举，避免被 QListView::LayoutMode 遮蔽
    void layoutSelected(::LayoutMode mode);

private:
    void setupItems();
};

#endif // LAYOUTLIBRARYWIDGET_H
