#include "AssetTreeWidget.h"
#include "parsers/ImageCache.h"

#include <QDrag>
#include <QTreeWidgetItem>
#include <QDir>
#include <QFileInfo>
#include <QPoint>
#include <QPalette>

// ============================================================
// AssetMimeData
// ============================================================
AssetMimeData::AssetMimeData()
    : assetType(ImageAsset)
    , bookIndex(-1)
{
}

// ============================================================
// AssetTreeWidget
// ============================================================
AssetTreeWidget::AssetTreeWidget(QWidget* parent)
    : QTreeWidget(parent)
    , m_populating(false)
    , m_updating(false)
{
    setupTree();
}

void AssetTreeWidget::setupTree()
{
    setHeaderHidden(true);                          // 隐藏表头
    setRootIsDecorated(true);                        // 显示展开/折叠箭头
    setUniformRowHeights(true);                      // 统一行高优化性能
    setAnimated(true);                              // 展开/折叠动画
    setColumnCount(1);                              // 单列
    setDragEnabled(true);                           // 启用拖拽
    setDragDropMode(QAbstractItemView::DragOnly);    // 仅拖出
    setAcceptDrops(false);                           // 不接受拖入
    setSelectionMode(QAbstractItemView::SingleSelection);

    // 紧凑行高样式
    setStyleSheet(QString::fromUtf8("QTreeView::item { padding: 2px; }"));

    connect(this, &QTreeWidget::itemChanged, this, &AssetTreeWidget::onItemChanged);
}

void AssetTreeWidget::populateMaterials(const QList<BookPtr>& books, const QString& basePath)
{
    m_populating = true;   // 抑制填充过程中的勾选信号
    clear();
    m_basePath = basePath;

    for (int i = 0; i < books.size(); ++i) {
        if (books[i].isNull()) continue;
        addBookNode(invisibleRootItem(), books[i], i);
    }

    m_populating = false;
}

void AssetTreeWidget::addBookNode(QTreeWidgetItem* parent, BookPtr book, int index)
{
    QTreeWidgetItem* bookNode = new QTreeWidgetItem(parent);

    // 计算图片素材数
    int imgCount = 0;
    if (!book->coverImagePath().isEmpty()) imgCount++;
    imgCount += book->sampleImages().size();
    imgCount += book->qrCodes().size();

    // 计算文本素材数
    int txtCount = 0;
    if (!book->title().isEmpty()) txtCount++;
    if (!book->author().isEmpty()) txtCount++;
    if (!book->isbn().isEmpty()) txtCount++;
    if (!book->description().isEmpty()) txtCount++;
    if (book->hasSeries() && !book->seriesName().isEmpty()) txtCount++;

    // 书籍节点文本：📖 书名 (3图+5文)
    bookNode->setText(0, QString::fromUtf8("📖 %1 (%2图+%3文)")
                      .arg(book->title()).arg(imgCount).arg(txtCount));

    // 书籍节点数据：存储索引和书名（UserRole留空表示不可拖拽）
    bookNode->setData(0, Qt::UserRole + 4, index);
    bookNode->setData(0, Qt::UserRole + 5, book->title());

    // 设置可勾选，默认勾选（书籍节点不可拖拽）
    bookNode->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable);
    bookNode->setCheckState(0, Qt::Checked);

    // 添加子节点
    addImageAssets(bookNode, book);
    addTextAssets(bookNode, book);

    // 默认展开
    bookNode->setExpanded(true);
}

void AssetTreeWidget::addImageAssets(QTreeWidgetItem* bookNode, BookPtr book)
{
    int bookIndex = bookNode->data(0, Qt::UserRole + 4).toInt();
    QString bookTitle = bookNode->data(0, Qt::UserRole + 5).toString();

    // 统计图片数量
    int imgCount = 0;
    if (!book->coverImagePath().isEmpty()) imgCount++;
    imgCount += book->sampleImages().size();
    imgCount += book->qrCodes().size();

    // 图片分组节点
    QTreeWidgetItem* imgGroup = new QTreeWidgetItem(bookNode);
    imgGroup->setText(0, QString::fromUtf8("🖼️ 图片 (%1)").arg(imgCount));
    // 组节点UserRole为空（不可拖拽），仅可选中
    imgGroup->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);

    // 封面图（缩略图24x32）
    if (!book->coverImagePath().isEmpty()) {
        QString path = resolvePath(book->coverImagePath());
        QTreeWidgetItem* node = new QTreeWidgetItem(imgGroup);
        node->setText(0, QString::fromUtf8("封面图"));
        QPixmap thumb = ImageCache::instance().getThumbnail(path, QSize(24, 32));
        if (!thumb.isNull()) node->setIcon(0, QIcon(thumb));
        node->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled);
        node->setData(0, Qt::UserRole, ImageAsset);
        node->setData(0, Qt::UserRole + 1, path);
        node->setData(0, Qt::UserRole + 4, bookIndex);
        node->setData(0, Qt::UserRole + 5, bookTitle);
    }

    // 样章图（缩略图24x32）
    QStringList samples = book->sampleImages();
    for (int i = 0; i < samples.size(); ++i) {
        QString path = resolvePath(samples[i]);
        QTreeWidgetItem* node = new QTreeWidgetItem(imgGroup);
        node->setText(0, QString::fromUtf8("样章图%1").arg(i + 1));
        QPixmap thumb = ImageCache::instance().getThumbnail(path, QSize(24, 32));
        if (!thumb.isNull()) node->setIcon(0, QIcon(thumb));
        node->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled);
        node->setData(0, Qt::UserRole, ImageAsset);
        node->setData(0, Qt::UserRole + 1, path);
        node->setData(0, Qt::UserRole + 4, bookIndex);
        node->setData(0, Qt::UserRole + 5, bookTitle);
    }

    // 二维码（缩略图24x24）
    QStringList qrs = book->qrCodes();
    for (int i = 0; i < qrs.size(); ++i) {
        QString path = resolvePath(qrs[i]);
        QTreeWidgetItem* node = new QTreeWidgetItem(imgGroup);
        node->setText(0, QString::fromUtf8("二维码%1").arg(i + 1));
        QPixmap thumb = ImageCache::instance().getThumbnail(path, QSize(24, 24));
        if (!thumb.isNull()) node->setIcon(0, QIcon(thumb));
        node->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled);
        node->setData(0, Qt::UserRole, ImageAsset);
        node->setData(0, Qt::UserRole + 1, path);
        node->setData(0, Qt::UserRole + 4, bookIndex);
        node->setData(0, Qt::UserRole + 5, bookTitle);
    }
}

void AssetTreeWidget::addTextAssets(QTreeWidgetItem* bookNode, BookPtr book)
{
    int bookIndex = bookNode->data(0, Qt::UserRole + 4).toInt();
    QString bookTitle = bookNode->data(0, Qt::UserRole + 5).toString();

    // 文本分组节点
    QTreeWidgetItem* txtGroup = new QTreeWidgetItem(bookNode);
    txtGroup->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);

    // 收集非空文本字段
    struct Field { QString label; QString value; };
    QList<Field> allFields;
    allFields << Field{QString::fromUtf8("书名"), book->title()};
    allFields << Field{QString::fromUtf8("作者"), book->author()};
    allFields << Field{QString::fromUtf8("ISBN"), book->isbn()};
    allFields << Field{QString::fromUtf8("描述"), book->description()};
    if (book->hasSeries() && !book->seriesName().isEmpty()) {
        allFields << Field{QString::fromUtf8("丛书名"), book->seriesName()};
    }

    QList<Field> fields;
    for (const Field& f : allFields) {
        if (!f.value.isEmpty()) fields << f;
    }

    txtGroup->setText(0, QString::fromUtf8("📝 文本 (%1)").arg(fields.size()));

    // 创建文本叶子节点，格式：标签: "内容预览..."
    for (const Field& f : fields) {
        QTreeWidgetItem* node = new QTreeWidgetItem(txtGroup);
        node->setText(0, QString::fromUtf8("%1: \"%2\"")
                      .arg(f.label).arg(truncateText(f.value)));
        node->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled);
        node->setData(0, Qt::UserRole, TextAsset);
        node->setData(0, Qt::UserRole + 2, f.value);   // 完整文本内容
        node->setData(0, Qt::UserRole + 3, f.label);   // 文本标签
        node->setData(0, Qt::UserRole + 4, bookIndex);
        node->setData(0, Qt::UserRole + 5, bookTitle);
    }

    // 添加TXT文件素材节点（关联的TXT文件，可拖拽到编辑器）
    QStringList txtFiles = book->txtFiles();
    for (int i = 0; i < txtFiles.size(); ++i) {
        QFileInfo info(txtFiles[i]);
        QTreeWidgetItem* txtItem = new QTreeWidgetItem(txtGroup);
        txtItem->setText(0, QString::fromUtf8("📄 %1").arg(info.fileName()));
        txtItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled);
        txtItem->setData(0, Qt::UserRole, TxtFileAsset);
        txtItem->setData(0, Qt::UserRole + 1, txtFiles[i]);   // TXT文件路径
        txtItem->setData(0, Qt::UserRole + 4, bookIndex);
        txtItem->setData(0, Qt::UserRole + 5, bookTitle);
    }

    // 若存在TXT文件，更新文本分组计数（包含文本字段和TXT文件）
    if (!txtFiles.isEmpty()) {
        int totalCount = fields.size() + txtFiles.size();
        txtGroup->setText(0, QString::fromUtf8("📝 文本 (%1)").arg(totalCount));
    }
}

void AssetTreeWidget::onItemChanged(QTreeWidgetItem* item, int column)
{
    // 填充或程序化更新时抑制信号
    if (m_populating || m_updating) return;
    if (!item || column != 0) return;
    // 仅顶级书籍节点可勾选
    if (item->parent()) return;

    int bookIndex = item->data(0, Qt::UserRole + 4).toInt();
    bool checked = (item->checkState(0) == Qt::Checked);
    applyBookCheckVisual(item, checked);
    emit bookCheckChanged(bookIndex, checked);
}

QList<int> AssetTreeWidget::checkedBookIndices() const
{
    QList<int> result;
    for (int i = 0; i < topLevelItemCount(); ++i) {
        QTreeWidgetItem* node = topLevelItem(i);
        if (node->checkState(0) == Qt::Checked) {
            result.append(node->data(0, Qt::UserRole + 4).toInt());
        }
    }
    return result;
}

void AssetTreeWidget::setBookChecked(int index, bool checked)
{
    // 按存储的bookIndex查找书籍节点
    for (int i = 0; i < topLevelItemCount(); ++i) {
        QTreeWidgetItem* node = topLevelItem(i);
        if (node->data(0, Qt::UserRole + 4).toInt() == index) {
            m_updating = true;   // 抑制onItemChanged信号
            node->setCheckState(0, checked ? Qt::Checked : Qt::Unchecked);
            m_updating = false;
            applyBookCheckVisual(node, checked);
            return;
        }
    }
}

void AssetTreeWidget::startDrag(Qt::DropActions supportedActions)
{
    Q_UNUSED(supportedActions);

    QTreeWidgetItem* item = currentItem();
    // 组节点和书籍节点的UserRole为空，不产生拖拽数据
    if (!item || item->data(0, Qt::UserRole).isNull()) return;

    AssetType type = static_cast<AssetType>(item->data(0, Qt::UserRole).toInt());

    AssetMimeData* mimeData = new AssetMimeData;
    mimeData->assetType = type;
    mimeData->imagePath = item->data(0, Qt::UserRole + 1).toString();
    mimeData->textContent = item->data(0, Qt::UserRole + 2).toString();
    mimeData->textLabel = item->data(0, Qt::UserRole + 3).toString();
    mimeData->txtFilePath = item->data(0, Qt::UserRole + 1).toString();
    mimeData->bookIndex = item->data(0, Qt::UserRole + 4).toInt();
    mimeData->bookTitle = item->data(0, Qt::UserRole + 5).toString();

    // 设置自定义MIME类型标识，便于接收方识别
    mimeData->setData("application/x-bookpdf-asset", QByteArray("asset"));

    QDrag* drag = new QDrag(this);
    drag->setMimeData(mimeData);

    // 设置拖拽预览图（图片素材用缩略图，文本素材用默认）
    if (type == ImageAsset && !mimeData->imagePath.isEmpty()) {
        QPixmap thumb = ImageCache::instance().getThumbnail(mimeData->imagePath, QSize(60, 80));
        if (!thumb.isNull()) {
            drag->setPixmap(thumb);
            drag->setHotSpot(QPoint(thumb.width() / 2, thumb.height() / 2));
        }
    }

    drag->exec(Qt::CopyAction);
}

// ============================================================
// 辅助方法
// ============================================================
QString AssetTreeWidget::resolvePath(const QString& path) const
{
    if (path.isEmpty()) return path;
    QFileInfo fi(path);
    if (fi.isAbsolute()) return path;   // 绝对路径直接返回
    return QDir(m_basePath).filePath(path);   // 相对路径基于basePath解析
}

QString AssetTreeWidget::truncateText(const QString& text, int maxChars) const
{
    // 合并换行，简化空白
    QString s = text;
    s.replace(QLatin1Char('\n'), QLatin1Char(' '));
    s.replace(QLatin1Char('\r'), QLatin1Char(' '));
    s = s.simplified();
    if (s.length() > maxChars) {
        return s.left(maxChars) + QString::fromUtf8("...");
    }
    return s;
}

void AssetTreeWidget::setNodeEnabledRecursive(QTreeWidgetItem* node, bool enabled)
{
    if (!node) return;
    // 启用时使用调色板默认文字色，禁用时灰显
    QColor color = enabled ? palette().color(QPalette::Normal, QPalette::Text)
                           : QColor(Qt::gray);
    node->setForeground(0, color);
    for (int i = 0; i < node->childCount(); ++i) {
        setNodeEnabledRecursive(node->child(i), enabled);
    }
}

void AssetTreeWidget::applyBookCheckVisual(QTreeWidgetItem* bookNode, bool checked)
{
    if (!bookNode) return;
    // 书籍节点本身保持正常，仅灰显其子节点（素材）
    for (int i = 0; i < bookNode->childCount(); ++i) {
        setNodeEnabledRecursive(bookNode->child(i), checked);
    }
}
