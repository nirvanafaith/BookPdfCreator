#ifndef LAYERPANEL_H
#define LAYERPANEL_H

#include <QWidget>
#include <QList>
#include <QPair>

#include "PageElement.h"

class QListWidget;
class QListWidgetItem;

// ============================================================
// LayerPanel - 图层面板
//
// 显示当前页面的所有元素，支持：
//   - 按 zOrder 排序显示（z 值高的在列表顶部，与 Photoshop 习惯一致）
//   - 切换元素可见性（眼睛按钮）
//   - 切换元素锁定状态（锁按钮）
//   - 拖拽重排序（重新分配 zOrder 并发出信号）
//   - 右键菜单：置顶 / 置底 / 上移一层 / 下移一层 / 复制 / 删除
//   - 选中同步（点击条目发出 elementSelected 信号）
//
// 数据模型约束：
//   PageElementPtr = QSharedDataPointer<PageElementData>
//   仅支持只读访问（元素数据为抽象类，非 const 访问会触发 detach
//   并在编译期报错）。本面板不直接修改元素数据，仅发出信号由
//   控制器（EditorScene / MainWindow 等）处理后再回灌 updateLayers。
//
// 元素顺序约定：
//   传入的 elements 已按 zOrder 升序排序（PageData::elements() 保证）。
//   面板按 zOrder 降序显示，因此从列表末尾向头部遍历构建条目，
//   使 z 值最大的元素出现在列表顶部。
// ============================================================
class LayerPanel : public QWidget
{
    Q_OBJECT
public:
    LayerPanel(QWidget* parent = nullptr);

    // 更新图层列表（按 zOrder 排序，z 高的显示在顶部）
    // 重建过程中会保留当前选中元素的选中状态。
    void updateLayers(const QList<PageElementPtr>& elements);

    // 高亮选中指定元素对应的图层条目（场景选中变化时调用）
    void highlightElement(const QString& elementId);
    // 高亮选中多个元素对应的图层条目
    void highlightElements(const QStringList& elementIds);

    // 清空图层列表
    void clearLayers();

    // 设置背景图层项（固定显示在列表最底层，不可拖拽/删除）
    // name为显示名称，visible控制眼睛按钮状态
    void setBackgroundItem(const QString& name, bool visible);

signals:
    void elementSelected(const QString& elementId);
    void elementVisibilityChanged(const QString& elementId, bool visible);
    void elementLockChanged(const QString& elementId, bool locked);
    void elementZOrderChanged(const QString& elementId, int newZ);
    void elementZOrderBatchChanged(const QList<QPair<QString, int>>& changes);
    void elementDeleted(const QString& elementId);
    void elementDuplicated(const QString& elementId);
    void bringToFront(const QString& elementId);
    void sendToBack(const QString& elementId);
    void moveUp(const QString& elementId);
    void moveDown(const QString& elementId);
    void layerRenamed(const QString& elementId, const QString& newName);

protected:
    // 事件过滤器：监听列表视口的 Drop 事件以触发拖拽重排序处理
    bool eventFilter(QObject* obj, QEvent* event) override;

private:
    QListWidget* m_listWidget;
    bool m_syncing;  // 同步更新标志，防止程序化修改触发反馈信号
    QListWidgetItem* m_backgroundItem;  // 背景图层项（固定最底层，不可拖拽/删除）

    void setupUi();
    void setupConnections();
    void onItemChanged(QListWidgetItem* item);
    void onContextMenu(const QPoint& pos);
    void onItemSelectionChanged();
    void onDragReordered();
};

#endif // LAYERPANEL_H
