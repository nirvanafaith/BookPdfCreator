#include "ZipReader.h"

#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QIODevice>
#include <QTemporaryDir>

#include <cstring>
#include <memory>

#include "miniz.h"

// 私有实现类（d-pointer）
class ZipReader::Impl
{
public:
    Impl()
        : m_isOpen(false)
        , m_isMemory(false)
    {
        memset(&m_zip, 0, sizeof(m_zip));
    }

    ~Impl()
    {
        close();
    }

    // 从文件打开
    bool openFromFile(const QString &filePath)
    {
        close();

        QByteArray pathBytes = filePath.toUtf8();
        mz_bool status = mz_zip_reader_init_file(&m_zip, pathBytes.constData(), 0);
        if (!status) {
            return false;
        }

        m_isOpen = true;
        m_filePath = filePath;
        m_isMemory = false;
        return true;
    }

    // 从内存打开
    bool openFromMemory(const QByteArray &data)
    {
        close();

        m_memoryData = data;
        // 使用data()获取可写指针，miniz可能需要修改内存（用于读取时的内部操作）
        mz_bool status = mz_zip_reader_init_mem(&m_zip, m_memoryData.data(), static_cast<size_t>(m_memoryData.size()), 0);
        if (!status) {
            m_memoryData.clear();
            return false;
        }

        m_isOpen = true;
        m_isMemory = true;
        return true;
    }

    // 关闭ZIP
    void close()
    {
        if (m_isOpen) {
            mz_zip_reader_end(&m_zip);
            memset(&m_zip, 0, sizeof(m_zip));
            m_isOpen = false;
            m_filePath.clear();
            m_memoryData.clear();
            m_fileListCache.clear();
        }
    }

    bool isOpen() const
    {
        return m_isOpen;
    }

    // 获取文件列表（带缓存）
    QStringList fileList() const
    {
        if (!m_isOpen) {
            return QStringList();
        }

        if (!m_fileListCache.isEmpty()) {
            return m_fileListCache;
        }

        mz_uint numFiles = mz_zip_reader_get_num_files(const_cast<mz_zip_archive *>(&m_zip));
        QStringList list;
        list.reserve(numFiles);

        for (mz_uint i = 0; i < numFiles; ++i) {
            mz_zip_archive_file_stat fileStat;
            if (mz_zip_reader_file_stat(const_cast<mz_zip_archive *>(&m_zip), i, &fileStat)) {
                QString fileName = QString::fromUtf8(fileStat.m_filename);
                list.append(fileName);
            }
        }

        m_fileListCache = list;
        return list;
    }

    // 判断文件是否存在
    bool fileExists(const QString &fileName) const
    {
        if (!m_isOpen) {
            return false;
        }

        QByteArray nameBytes = fileName.toUtf8();
        int fileIndex = mz_zip_reader_locate_file(const_cast<mz_zip_archive *>(&m_zip), nameBytes.constData(), nullptr, 0);
        return fileIndex >= 0;
    }

    // 读取文件数据
    QByteArray fileData(const QString &fileName) const
    {
        if (!m_isOpen) {
            return QByteArray();
        }

        QByteArray nameBytes = fileName.toUtf8();
        size_t fileSize = 0;
        void *pData = mz_zip_reader_extract_file_to_heap(
            const_cast<mz_zip_archive *>(&m_zip),
            nameBytes.constData(),
            &fileSize,
            0
        );

        if (!pData) {
            return QByteArray();
        }

        QByteArray result(static_cast<const char *>(pData), static_cast<int>(fileSize));
        mz_free(pData);
        return result;
    }

    // 解压所有文件到目录
    bool extractToDir(const QString &dirPath)
    {
        if (!m_isOpen) {
            return false;
        }

        QDir dir(dirPath);
        if (!dir.exists()) {
            if (!dir.mkpath(".")) {
                return false;
            }
        }

        QStringList files = fileList();
        bool allSuccess = true;

        for (const QString &fileName : files) {
            // 处理目录
            if (fileName.endsWith('/') || fileName.endsWith('\\')) {
                QString subDirPath = dirPath + "/" + fileName;
                QDir subDir(subDirPath);
                if (!subDir.exists()) {
                    dir.mkpath(fileName);
                }
                continue;
            }

            // 创建父目录
            QFileInfo fileInfo(fileName);
            QString parentPath = fileInfo.path();
            if (!parentPath.isEmpty() && parentPath != ".") {
                QDir parentDir(dirPath + "/" + parentPath);
                if (!parentDir.exists()) {
                    dir.mkpath(parentPath);
                }
            }

            // 提取文件
            QByteArray data = fileData(fileName);
            if (data.isNull()) {
                allSuccess = false;
                continue;
            }

            QString outFilePath = dirPath + "/" + fileName;
            QFile outFile(outFilePath);
            if (!outFile.open(QIODevice::WriteOnly)) {
                allSuccess = false;
                continue;
            }

            outFile.write(data);
            outFile.close();
        }

        return allSuccess;
    }

    // 解压到临时目录
    QString extractToTempDir()
    {
        if (!m_isOpen) {
            return QString();
        }

        // 如果已经创建过临时目录，直接返回
        if (m_tempDir && m_tempDir->isValid()) {
            return m_tempDir->path();
        }

        // 创建新的临时目录
        m_tempDir.reset(new QTemporaryDir());
        if (!m_tempDir->isValid()) {
            m_tempDir.reset();
            return QString();
        }

        // 解压到临时目录
        if (!extractToDir(m_tempDir->path())) {
            m_tempDir.reset();
            return QString();
        }

        return m_tempDir->path();
    }

    // 获取临时目录路径
    QString tempDirPath() const
    {
        if (m_tempDir && m_tempDir->isValid()) {
            return m_tempDir->path();
        }
        return QString();
    }

private:
    mz_zip_archive m_zip;
    bool m_isOpen;
    bool m_isMemory;
    QString m_filePath;
    QByteArray m_memoryData;
    mutable QStringList m_fileListCache;
    QScopedPointer<QTemporaryDir> m_tempDir;
};

// ZipReader 公共接口实现

ZipReader::ZipReader()
    : d_ptr(new Impl)
{
}

ZipReader::~ZipReader()
{
    // QScopedPointer自动处理Impl的析构
}

bool ZipReader::open(const QString &filePath)
{
    return d_ptr->openFromFile(filePath);
}

bool ZipReader::open(QIODevice *device)
{
    if (!device) {
        return false;
    }

    // 如果设备未打开，尝试以只读方式打开
    if (!device->isOpen()) {
        if (!device->open(QIODevice::ReadOnly)) {
            return false;
        }
    }

    // 读取所有数据到内存
    QByteArray data = device->readAll();
    return d_ptr->openFromMemory(data);
}

void ZipReader::close()
{
    d_ptr->close();
}

bool ZipReader::isOpen() const
{
    return d_ptr->isOpen();
}

QStringList ZipReader::fileList() const
{
    return d_ptr->fileList();
}

bool ZipReader::fileExists(const QString &fileName) const
{
    return d_ptr->fileExists(fileName);
}

QByteArray ZipReader::fileData(const QString &fileName) const
{
    return d_ptr->fileData(fileName);
}

bool ZipReader::extractToDir(const QString &dirPath)
{
    return d_ptr->extractToDir(dirPath);
}

QString ZipReader::extractToTempDir()
{
    return d_ptr->extractToTempDir();
}

QString ZipReader::tempDirPath() const
{
    return d_ptr->tempDirPath();
}
