#ifndef ZIPREADER_H
#define ZIPREADER_H

#include <QString>
#include <QStringList>
#include <QByteArray>
#include <QScopedPointer>

class QIODevice;
class QTemporaryDir;

// ZIP解压工具类
// 使用miniz库实现ZIP文件读取，支持从文件路径或QIODevice（内存数据）打开
class ZipReader
{
public:
    // 构造函数
    ZipReader();

    // 析构函数，自动关闭ZIP
    ~ZipReader();

    // 禁止拷贝
    ZipReader(const ZipReader &) = delete;
    ZipReader &operator=(const ZipReader &) = delete;

    // 从文件路径打开ZIP
    bool open(const QString &filePath);

    // 从QIODevice打开ZIP（支持内存数据）
    bool open(QIODevice *device);

    // 关闭ZIP
    void close();

    // 判断ZIP是否已打开
    bool isOpen() const;

    // 获取ZIP内所有文件名列表
    QStringList fileList() const;

    // 判断文件是否存在于ZIP中
    bool fileExists(const QString &fileName) const;

    // 提取文件内容到QByteArray
    // 如果文件不存在或读取失败，返回空QByteArray
    QByteArray fileData(const QString &fileName) const;

    // 解压所有文件到指定目录
    // 返回true表示全部解压成功
    bool extractToDir(const QString &dirPath);

    // 解压ZIP到临时目录
    // 返回临时目录路径，临时目录在ZipReader析构时自动清理
    // 如果解压失败返回空字符串
    QString extractToTempDir();

    // 获取临时目录路径（如果已创建）
    QString tempDirPath() const;

private:
    // 前向声明私有实现类（d-pointer模式）
    class Impl;
    QScopedPointer<Impl> d_ptr;
};

#endif // ZIPREADER_H
