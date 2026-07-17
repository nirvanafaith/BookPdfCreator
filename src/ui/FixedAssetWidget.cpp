#include "FixedAssetWidget.h"
#include "AssetTreeWidget.h"
#include "parsers/ImageCache.h"

#include <QDrag>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDragLeaveEvent>
#include <QDropEvent>
#include <QPainter>
#include <QPaintEvent>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImageReader>
#include <QUrl>
#include <QMimeData>
#include <QListWidgetItem>
#include <QCoreApplication>
#include <QPoint>

// ============================================================
// FixedAssetWidget
// ============================================================
FixedAssetWidget::FixedAssetWidget(QWidget* parent)
    : QListWidget(parent)
    , m_dragHighlight(false)
{
    setupUi();

    // 获取固定素材目录并确保存在
    m_assetsDir = fixedAssetsDir();
    QDir().mkpath(m_assetsDir);

    // 加载已有图片
    loadFixedAssets();
}

void FixedAssetWidget::setupUi()
{
    setAcceptDrops(true);                       // 接受拖入
    setDragEnabled(true);                       // 允许拖出
    setDragDropMode(QAbstractItemView::DragDrop);  // 支持拖入和拖出
    setDropIndicatorShown(false);
    setSelectionMode(QAbstractItemView::SingleSelection);

    setIconSize(QSize(80, 80));                 // 缩略图尺寸
    setViewMode(QListWidget::IconMode);         // 图标模式
    setResizeMode(QListWidget::Adjust);         // 随窗口调整布局
    setMovement(QListWidget::Static);           // 不允许拖动重排
    setSpacing(8);

    // 紧凑样式
    setStyleSheet(QString::fromUtf8(
        "QListWidget { background: #fafafa; border: 1px solid #e0e0e0; }"
        "QListWidget::item { padding: 4px; }"
        "QListWidget::item:selected { background: #d0e4f7; }"));
}

QString FixedAssetWidget::fixedAssetsDir() const
{
    return QCoreApplication::applicationDirPath() + QString::fromUtf8("/fixed_assets");
}

bool FixedAssetWidget::isImageFile(const QString& filePath) const
{
    QString suffix = QFileInfo(filePath).suffix().toLower();
    if (suffix.isEmpty()) return false;

    // 获取Qt支持的图片格式列表
    const QList<QByteArray> supported = QImageReader::supportedImageFormats();
    for (const QByteArray& fmt : supported) {
        if (QString::fromLatin1(fmt).toLower() == suffix) {
            return true;
        }
    }
    return false;
}

void FixedAssetWidget::loadFixedAssets()
{
    if (m_assetsDir.isEmpty()) return;

    clear();

    QDir dir(m_assetsDir);
    if (!dir.exists()) return;

    // 遍历目录中的所有文件
    const QStringList files = dir.entryList(QDir::Files);
    for (const QString& fileName : files) {
        QString filePath = dir.absoluteFilePath(fileName);
        if (!isImageFile(filePath)) continue;

        QListWidgetItem* item = new QListWidgetItem(this);
        item->setText(QFileInfo(fileName).baseName());
        QPixmap thumb = ImageCache::instance().getThumbnail(filePath, QSize(80, 80));
        if (!thumb.isNull()) {
            item->setIcon(QIcon(thumb));
        }
        item->setData(Qt::UserRole, filePath);              // 图片绝对路径
        item->setData(Qt::UserRole + 1, ImageAsset);        // 素材类型
        item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled);
    }
}

void FixedAssetWidget::addImageAsset(const QString& filePath)
{
    if (!isImageFile(filePath)) return;

    QFileInfo srcInfo(filePath);
    QString targetPath = m_assetsDir + QDir::separator() + srcInfo.fileName();

    // 若源文件已在固定素材目录中，无需复制
    if (QFileInfo(filePath).absoluteFilePath() != QFileInfo(targetPath).absoluteFilePath()) {
        // 目标已存在则添加数字后缀
        if (QFile::exists(targetPath)) {
            QString baseName = srcInfo.baseName();
            QString suffix = srcInfo.completeSuffix();
            int counter = 1;
            do {
                targetPath = m_assetsDir + QDir::separator()
                             + baseName + QString::fromUtf8("_%1").arg(counter)
                             + (suffix.isEmpty() ? QString() : QString::fromUtf8(".") + suffix);
                counter++;
            } while (QFile::exists(targetPath));
        }

        if (!QFile::copy(filePath, targetPath)) {
            return;   // 复制失败则放弃
        }
    }

    // 创建列表项
    QListWidgetItem* item = new QListWidgetItem(this);
    item->setText(srcInfo.baseName());
    QPixmap thumb = ImageCache::instance().getThumbnail(targetPath, QSize(80, 80));
    if (!thumb.isNull()) {
        item->setIcon(QIcon(thumb));
    }
    item->setData(Qt::UserRole, targetPath);               // 图片绝对路径
    item->setData(Qt::UserRole + 1, ImageAsset);           // 素材类型
    item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled);
}

// ============================================================
// 拖出：复用AssetMimeData机制
// ============================================================
void FixedAssetWidget::startDrag(Qt::DropActions supportedActions)
{
    Q_UNUSED(supportedActions);

    QListWidgetItem* item = currentItem();
    if (!item) return;

    QString imagePath = item->data(Qt::UserRole).toString();
    if (imagePath.isEmpty()) return;

    AssetMimeData* mimeData = new AssetMimeData;
    mimeData->assetType = ImageAsset;
    mimeData->imagePath = imagePath;

    // 设置自定义MIME类型标识，便于接收方识别
    mimeData->setData("application/x-bookpdf-asset", QByteArray("asset"));

    QDrag* drag = new QDrag(this);
    drag->setMimeData(mimeData);

    // 设置拖拽预览图
    QIcon icon = item->icon();
    if (!icon.isNull()) {
        QPixmap pix = icon.pixmap(iconSize());
        if (!pix.isNull()) {
            drag->setPixmap(pix);
            drag->setHotSpot(QPoint(pix.width() / 2, pix.height() / 2));
        }
    }

    drag->exec(Qt::CopyAction);
}

// ============================================================
// 拖入：接收外部图片文件
// ============================================================
void FixedAssetWidget::dragEnterEvent(QDragEnterEvent* event)
{
    if (!event->mimeData()->hasUrls()) {
        event->ignore();
        return;
    }

    // 检查是否至少有一个URL是本地图片文件
    bool hasImage = false;
    for (const QUrl& url : event->mimeData()->urls()) {
        if (url.isLocalFile() && isImageFile(url.toLocalFile())) {
            hasImage = true;
            break;
        }
    }

    if (hasImage) {
        event->acceptProposedAction();
        m_dragHighlight = true;
        update();
    } else {
        event->ignore();
    }
}

void FixedAssetWidget::dragMoveEvent(QDragMoveEvent* event)
{
    if (!event->mimeData()->hasUrls()) {
        event->ignore();
        return;
    }

    // 检查是否至少有一个URL是本地图片文件
    bool hasImage = false;
    for (const QUrl& url : event->mimeData()->urls()) {
        if (url.isLocalFile() && isImageFile(url.toLocalFile())) {
            hasImage = true;
            break;
        }
    }

    if (hasImage) {
        event->acceptProposedAction();
        if (!m_dragHighlight) {
            m_dragHighlight = true;
            update();
        }
    } else {
        event->ignore();
    }
}

void FixedAssetWidget::dragLeaveEvent(QDragLeaveEvent* event)
{
    Q_UNUSED(event);
    m_dragHighlight = false;
    update();
}

void FixedAssetWidget::dropEvent(QDropEvent* event)
{
    m_dragHighlight = false;
    update();

    if (!event->mimeData()->hasUrls()) {
        event->ignore();
        return;
    }

    // 遍历所有URL，对图片文件调用addImageAsset
    for (const QUrl& url : event->mimeData()->urls()) {
        if (url.isLocalFile()) {
            QString filePath = url.toLocalFile();
            if (isImageFile(filePath)) {
                addImageAsset(filePath);
            }
        }
    }

    event->acceptProposedAction();
}

// ============================================================
// 绘制：拖拽高亮边框 + 空列表提示
// ============================================================
void FixedAssetWidget::paintEvent(QPaintEvent* event)
{
    QListWidget::paintEvent(event);

    QPainter painter(viewport());
    painter.setRenderHint(QPainter::Antialiasing, true);

    // 列表为空时显示提示文字
    if (count() == 0) {
        painter.setPen(QColor(180, 180, 180));
        QFont font = painter.font();
        font.setPointSize(10);
        painter.setFont(font);
        painter.drawText(viewport()->rect(), Qt::AlignCenter,
                         QString::fromUtf8("拖入图片文件到此处添加固定素材"));
    }

    // 拖拽高亮边框
    if (m_dragHighlight) {
        QPen pen(QColor(30, 120, 230));
        pen.setWidth(2);
        painter.setPen(pen);
        painter.drawRect(viewport()->rect().adjusted(0, 0, -1, -1));
    }
}
