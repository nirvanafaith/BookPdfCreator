#ifndef IMAGECACHE_H
#define IMAGECACHE_H

#include <QObject>
#include <QPixmap>
#include <QCache>
#include <QHash>
#include <QSize>
#include <QByteArray>
#include <QMutex>

// 图片缓存类（单例模式）
// 提供图片加载、缓存、缩略图生成功能
// 使用QImageReader进行预缩放优化内存占用
class ImageCache
{
public:
    // 获取单例实例
    static ImageCache &instance();

    // 禁止拷贝和赋值
    ImageCache(const ImageCache &) = delete;
    ImageCache &operator=(const ImageCache &) = delete;

    // 获取图片（自动缓存）
    // path: 图片文件路径
    // targetSize: 目标尺寸，如果为QSize()则加载原始尺寸
    QPixmap getPixmap(const QString &path, const QSize &targetSize = QSize());

    // 获取缩略图（用于列表显示）
    // path: 图片文件路径
    // thumbSize: 缩略图尺寸，默认120x160
    QPixmap getThumbnail(const QString &path, const QSize &thumbSize = QSize(120, 160));

    // 从内存数据加载图片
    // data: 图片二进制数据
    // cacheKey: 缓存键（用于唯一标识这张图片）
    // targetSize: 目标尺寸，如果为QSize()则加载原始尺寸
    QPixmap getPixmapFromData(const QByteArray &data, const QString &cacheKey, const QSize &targetSize = QSize());

    // 设置最大缓存大小（单位：KB）
    // 默认100MB (102400 KB)
    void setMaxCacheSize(int kb);

    // 获取当前最大缓存大小（单位：KB）
    int maxCacheSize() const;

    // 清空所有缓存
    void clearCache();

    // 移除指定路径的缓存
    void removeCache(const QString &path);

    // 获取图片原始尺寸（带缓存，不加载图片内容）
    QSize getOriginalSize(const QString& path);

    // 获取占位图（加载失败时使用）
    QPixmap placeholderPixmap(const QSize &size = QSize(120, 160)) const;

private:
    // 私有构造函数（单例模式）
    ImageCache();
    ~ImageCache();

    // 生成缓存键
    static QString makeCacheKey(const QString &path, const QSize &size);
    static QString makeCacheKey(const QString &baseKey, const QSize &size, bool isThumbnail);

    // 实际加载图片的内部方法
    QPixmap loadPixmapFromFile(const QString &path, const QSize &targetSize);
    QPixmap loadPixmapFromData(const QByteArray &data, const QSize &targetSize);

    // 创建占位图
    QPixmap createPlaceholder(const QSize &size) const;

    mutable QMutex m_mutex;                  // 互斥锁（线程安全）
    QCache<QString, QPixmap> m_pixmapCache;  // 图片缓存
    QHash<QString, QSize> m_sizeCache;       // 图片原始尺寸缓存
    int m_maxCacheSizeKb;                    // 最大缓存大小（KB）

    static const int DEFAULT_MAX_CACHE_KB = 102400; // 默认100MB
};

#endif // IMAGECACHE_H
