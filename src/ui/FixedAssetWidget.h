#ifndef FIXEDASSETWIDGET_H
#define FIXEDASSETWIDGET_H

#include <QListWidget>
#include <QString>

// ============================================================
// 固定素材组件
// 以IconMode展示图片缩略图，支持：
//   - 持久化存储图片到exe同目录的fixed_assets文件夹
//   - 从外部拖入图片文件（拖入时显示高亮提示边框）
//   - 拖出图片到PDF编辑页面（复用AssetMimeData机制）
//   - 启动时从固定素材目录加载已有图片
//
// 节点数据存储（QListWidgetItem::data）：
//   Qt::UserRole    : 图片绝对路径
//   Qt::UserRole+1  : AssetType（固定为ImageAsset，用于拖出时识别）
// ============================================================
class FixedAssetWidget : public QListWidget
{
    Q_OBJECT
public:
    explicit FixedAssetWidget(QWidget* parent = nullptr);

    // 从持久化目录加载已有图片
    void loadFixedAssets();

protected:
    void startDrag(Qt::DropActions supportedActions) override;
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dragLeaveEvent(QDragLeaveEvent* event) override;
    void dropEvent(QDropEvent* event) override;
    void paintEvent(QPaintEvent* event) override;

private:
    QString m_assetsDir;   // 固定素材存储目录路径
    bool m_dragHighlight;  // 拖拽高亮标志

    void setupUi();
    QString fixedAssetsDir() const;             // 获取固定素材目录路径
    bool isImageFile(const QString& filePath) const;  // 判断是否为支持的图片格式
    void addImageAsset(const QString& filePath);      // 添加图片到列表和持久化目录
};

#endif // FIXEDASSETWIDGET_H
