#include "ImageCache.h"

#include <QImageReader>
#include <QBuffer>
#include <QPainter>
#include <QFileInfo>
#include <QMutexLocker>

ImageCache &ImageCache::instance()
{
    static ImageCache s_instance;
    return s_instance;
}

ImageCache::ImageCache()
    : m_maxCacheSizeKb(DEFAULT_MAX_CACHE_KB)
{
    // 设置缓存大小（QCache使用字节作为单位）
    m_pixmapCache.setMaxCost(m_maxCacheSizeKb * 1024);
}

ImageCache::~ImageCache()
{
}

QPixmap ImageCache::getPixmap(const QString &path, const QSize &targetSize)
{
    if (path.isEmpty()) {
        return placeholderPixmap(targetSize.isEmpty() ? QSize(120, 160) : targetSize);
    }

    QMutexLocker locker(&m_mutex);

    // 生成缓存键
    QString cacheKey = makeCacheKey(path, targetSize);

    // 先查找缓存
    QPixmap *cached = m_pixmapCache.object(cacheKey);
    if (cached) {
        return *cached;
    }

    // 加载图片
    QPixmap pixmap = loadPixmapFromFile(path, targetSize);

    // 计算缓存开销（以字节为单位）
    int cost = pixmap.width() * pixmap.height() * ((pixmap.depth() + 7) / 8);
    if (cost > 0) {
        m_pixmapCache.insert(cacheKey, new QPixmap(pixmap), cost);
    }

    return pixmap;
}

QPixmap ImageCache::getThumbnail(const QString &path, const QSize &thumbSize)
{
    if (path.isEmpty()) {
        return placeholderPixmap(thumbSize);
    }

    QMutexLocker locker(&m_mutex);

    // 生成缩略图缓存键
    QString cacheKey = makeCacheKey(path, thumbSize, true);

    // 先查找缓存
    QPixmap *cached = m_pixmapCache.object(cacheKey);
    if (cached) {
        return *cached;
    }

    // 加载图片（loadPixmapFromFile内部已通过QImageReader::setScaledSize预缩放，
    // 保持宽高比缩放到thumbSize内，无需再次scaled）
    QPixmap pixmap = loadPixmapFromFile(path, thumbSize);

    if (pixmap.isNull()) {
        pixmap = createPlaceholder(thumbSize);
    }

    // 计算缓存开销
    int cost = pixmap.width() * pixmap.height() * ((pixmap.depth() + 7) / 8);
    if (cost > 0) {
        m_pixmapCache.insert(cacheKey, new QPixmap(pixmap), cost);
    }

    return pixmap;
}

QPixmap ImageCache::getPixmapFromData(const QByteArray &data, const QString &cacheKey, const QSize &targetSize)
{
    if (data.isEmpty() || cacheKey.isEmpty()) {
        return placeholderPixmap(targetSize.isEmpty() ? QSize(120, 160) : targetSize);
    }

    QMutexLocker locker(&m_mutex);

    // 生成缓存键
    QString key = makeCacheKey(cacheKey, targetSize);

    // 先查找缓存
    QPixmap *cached = m_pixmapCache.object(key);
    if (cached) {
        return *cached;
    }

    // 从内存数据加载
    QPixmap pixmap = loadPixmapFromData(data, targetSize);

    // 计算缓存开销
    int cost = pixmap.width() * pixmap.height() * ((pixmap.depth() + 7) / 8);
    if (cost > 0) {
        m_pixmapCache.insert(key, new QPixmap(pixmap), cost);
    }

    return pixmap;
}

void ImageCache::setMaxCacheSize(int kb)
{
    QMutexLocker locker(&m_mutex);
    m_maxCacheSizeKb = kb;
    m_pixmapCache.setMaxCost(kb * 1024);
}

int ImageCache::maxCacheSize() const
{
    QMutexLocker locker(&m_mutex);
    return m_maxCacheSizeKb;
}

void ImageCache::clearCache()
{
    QMutexLocker locker(&m_mutex);
    m_pixmapCache.clear();
}

void ImageCache::removeCache(const QString &path)
{
    QMutexLocker locker(&m_mutex);

    // 移除所有与该路径相关的缓存
    QStringList keysToRemove;
    QList<QString> keys = m_pixmapCache.keys();
    for (const QString &key : keys) {
        if (key.startsWith(path)) {
            keysToRemove.append(key);
        }
    }

    for (const QString &key : keysToRemove) {
        m_pixmapCache.remove(key);
    }
}

QPixmap ImageCache::placeholderPixmap(const QSize &size) const
{
    return createPlaceholder(size);
}

QString ImageCache::makeCacheKey(const QString &path, const QSize &size)
{
    if (size.isEmpty()) {
        return path + "_original";
    }
    return QString("%1_%2x%3").arg(path).arg(size.width()).arg(size.height());
}

QString ImageCache::makeCacheKey(const QString &baseKey, const QSize &size, bool isThumbnail)
{
    QString prefix = isThumbnail ? "thumb_" : "data_";
    if (size.isEmpty()) {
        return prefix + baseKey + "_original";
    }
    return QString("%1%2_%3x%4").arg(prefix).arg(baseKey).arg(size.width()).arg(size.height());
}

QPixmap ImageCache::loadPixmapFromFile(const QString &path, const QSize &targetSize)
{
    QImageReader reader(path);

    // 设置缩放尺寸（预缩放优化内存）
    if (!targetSize.isEmpty()) {
        // 读取原始尺寸计算缩放比例
        QSize originalSize = reader.size();
        if (originalSize.isValid()) {
            // 计算保持宽高比的缩放尺寸
            QSize scaledSize = originalSize.scaled(targetSize, Qt::KeepAspectRatio);
            reader.setScaledSize(scaledSize);
        } else {
            reader.setScaledSize(targetSize);
        }
    }

    // 自动检测图片格式
    reader.setAutoTransform(true);

    QImage image = reader.read();
    if (image.isNull()) {
        QSize placeholderSize = targetSize.isEmpty() ? QSize(120, 160) : targetSize;
        return createPlaceholder(placeholderSize);
    }

    return QPixmap::fromImage(image);
}

QPixmap ImageCache::loadPixmapFromData(const QByteArray &data, const QSize &targetSize)
{
    QBuffer buffer;
    buffer.setData(data);
    buffer.open(QIODevice::ReadOnly);

    QImageReader reader(&buffer);

    // 设置缩放尺寸
    if (!targetSize.isEmpty()) {
        QSize originalSize = reader.size();
        if (originalSize.isValid()) {
            QSize scaledSize = originalSize.scaled(targetSize, Qt::KeepAspectRatio);
            reader.setScaledSize(scaledSize);
        } else {
            reader.setScaledSize(targetSize);
        }
    }

    reader.setAutoTransform(true);

    QImage image = reader.read();
    if (image.isNull()) {
        QSize placeholderSize = targetSize.isEmpty() ? QSize(120, 160) : targetSize;
        return createPlaceholder(placeholderSize);
    }

    return QPixmap::fromImage(image);
}

QPixmap ImageCache::createPlaceholder(const QSize &size) const
{
    QPixmap placeholder(size);
    placeholder.fill(QColor(200, 200, 200)); // 灰色背景

    QPainter painter(&placeholder);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QColor(120, 120, 120));

    // 绘制边框
    painter.drawRect(0, 0, size.width() - 1, size.height() - 1);

    // 绘制"无图"文字
    QFont font = painter.font();
    font.setPointSize(12);
    painter.setFont(font);

    QString text = "无图";
    QFontMetrics fm(font);
    QRect textRect = fm.boundingRect(text);

    painter.drawText(
        (size.width() - textRect.width()) / 2,
        (size.height() - textRect.height()) / 2 + fm.ascent(),
        text
    );

    painter.end();
    return placeholder;
}
