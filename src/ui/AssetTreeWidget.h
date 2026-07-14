#ifndef ASSETTREEWIDGET_H
#define ASSETTREEWIDGET_H

#include <QTreeWidget>
#include <QMimeData>
#include <QList>
#include <QString>

#include "models/Book.h"

class QTreeWidgetItem;

// ============================================================
// 素材类型枚举
// ============================================================
enum AssetType {
    ImageAsset,   // 图片素材
    TextAsset,    // 文本素材（书目字段）
    TxtFileAsset  // TXT文件素材
};

// ============================================================
// 自定义MIME数据，拖拽时携带素材信息
// 接收方可通过 assetType 判断素材类型并读取相应字段
// ============================================================
class AssetMimeData : public QMimeData
{
    Q_OBJECT
public:
    AssetMimeData();

    AssetType assetType;
    QString imagePath;        // 图片路径（ImageAsset）
    QString textContent;      // 文本内容（TextAsset）
    QString textLabel;        // 文本标签（如"书名"、"作者"）
    QString txtFilePath;      // TXT文件路径（TxtFileAsset）
    int bookIndex;            // 所属书籍索引
    QString bookTitle;        // 所属书名
};

// ============================================================
// 素材树组件
// 展示书籍的图片/文本素材，支持拖拽到编辑器画布
// 树结构示例：
//   📖 书名A (3图+5文) [☑]
//   ├─ 🖼️ 图片 (3)
//   │  ├─ 封面图 [缩略图]
//   │  ├─ 样章图1 [缩略图]
//   │  └─ 二维码1 [缩略图]
//   └─ 📝 文本 (5)
//      ├─ 书名: "Python编程"
//      └─ 作者: "张三"
// 节点数据存储（QTreeWidgetItem::setData, column=0）：
//   Qt::UserRole    : AssetType（叶子节点；组/书籍节点为空，不可拖拽）
//   Qt::UserRole+1  : imagePath / txtFilePath
//   Qt::UserRole+2  : textContent
//   Qt::UserRole+3  : textLabel
//   Qt::UserRole+4  : bookIndex（书籍节点同样存储，便于检索）
//   Qt::UserRole+5  : bookTitle
// ============================================================
class AssetTreeWidget : public QTreeWidget
{
    Q_OBJECT
public:
    explicit AssetTreeWidget(QWidget* parent = nullptr);

    // 填充素材数据（清空原有内容，按books顺序构建树）
    void populateMaterials(const QList<BookPtr>& books, const QString& basePath);

    // 获取勾选的书籍索引列表（索引为原始books列表中的下标）
    QList<int> checkedBookIndices() const;

    // 设置书籍勾选状态（index为原始books列表中的下标）
    void setBookChecked(int index, bool checked);

signals:
    // 书籍勾选状态变化（用户交互触发，程序化设置不触发）
    void bookCheckChanged(int index, bool checked);

protected:
    void startDrag(Qt::DropActions supportedActions) override;

private:
    QString m_basePath;   // 图片基础路径（用于解析相对路径）
    bool m_populating;    // 填充中标志，抑制勾选信号
    bool m_updating;      // 程序化更新标志，抑制勾选信号

    void setupTree();
    void addBookNode(QTreeWidgetItem* parent, BookPtr book, int index);
    void addImageAssets(QTreeWidgetItem* bookNode, BookPtr book);
    void addTextAssets(QTreeWidgetItem* bookNode, BookPtr book);
    void onItemChanged(QTreeWidgetItem* item, int column);

    // 辅助方法
    QString resolvePath(const QString& path) const;             // 解析图片相对路径
    QString truncateText(const QString& text, int maxChars = 20) const;  // 文本预览截断
    void setNodeEnabledRecursive(QTreeWidgetItem* node, bool enabled);   // 递归灰显/恢复
    void applyBookCheckVisual(QTreeWidgetItem* bookNode, bool checked);  // 应用勾选视觉
};

#endif // ASSETTREEWIDGET_H
