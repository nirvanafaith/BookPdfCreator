#ifndef IMAGEFINDER_H
#define IMAGEFINDER_H

#include <QString>
#include <QStringList>
#include <QSharedPointer>

#include "Book.h"

class ZipReader;

// 图片查找器
// 支持从本地目录或ZIP文件中查找书籍相关图片
class ImageFinder
{
public:
    // 从本地目录构造
    explicit ImageFinder(const QString &dirPath);

    // 从ZipReader构造（指定ZIP内的基础路径）
    ImageFinder(ZipReader *zipReader, const QString &basePathInZip = QString());

    // 析构函数
    ~ImageFinder();

    // 禁止拷贝
    ImageFinder(const ImageFinder &) = delete;
    ImageFinder &operator=(const ImageFinder &) = delete;

    // 根据ISBN查找书籍图片并填充到Book对象中
    void findImagesForBook(BookPtr book);

    // 返回支持的图片格式扩展名列表（小写，不带点）
    static QStringList supportedImageFormats();

private:
    // 从目录中查找封面图片
    QString findCoverInDir(const QString &isbn, const QString &dirPath);

    // 从ZIP中查找封面图片
    QString findCoverInZip(const QString &isbn);

    // 从目录中查找匹配模式的图片（样章/二维码）
    QStringList findPatternImagesInDir(const QString &isbn, const QString &pattern, const QString &dirPath);

    // 从ZIP中查找匹配模式的图片（样章/二维码）
    QStringList findPatternImagesInZip(const QString &isbn, const QString &pattern);

    // 在目录中按文件名查找（大小写不敏感）
    QString findFileByName(const QString& fileName, const QString& dirPath);

    // 从样章/二维码文件名中提取ISBN前缀
    // 例如 "978-7-113-29211-9pic1.png" 提取出 "978-7-113-29211-9"
    QString extractIsbnFromImageNames(BookPtr book);

    // 构建完整文件路径（目录模式）
    QString buildFilePath(const QString &dirPath, const QString &fileName) const;

    // 构建ZIP内完整路径
    QString buildZipPath(const QString &fileName) const;

    // 提取文件名中的数字（用于排序）
    static int extractNumberFromFileName(const QString &fileName, const QString &prefix);

    // 比较两个图片文件名（按数字排序）
    static bool compareImageFileNames(const QString &a, const QString &b, const QString &prefix);

    bool m_isZipMode;              // 是否为ZIP模式
    QString m_dirPath;             // 本地目录路径
    ZipReader *m_zipReader;        // ZIP读取器（不持有所有权）
    QString m_basePathInZip;       // ZIP内基础路径
    QString m_tempDirPath;         // ZIP解压后的临时目录路径
};

#endif // IMAGEFINDER_H
