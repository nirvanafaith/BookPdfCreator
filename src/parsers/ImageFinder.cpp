#include "ImageFinder.h"
#include "Book.h"
#include "ZipReader.h"

#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>
#include <QMap>
#include <algorithm>

ImageFinder::ImageFinder(const QString &dirPath)
    : m_isZipMode(false)
    , m_dirPath(dirPath)
    , m_zipReader(nullptr)
{
}

ImageFinder::ImageFinder(ZipReader *zipReader, const QString &basePathInZip)
    : m_isZipMode(true)
    , m_zipReader(zipReader)
    , m_basePathInZip(basePathInZip)
{
    // 如果是ZIP模式，先解压到临时目录
    if (m_zipReader && m_zipReader->isOpen()) {
        m_tempDirPath = m_zipReader->extractToTempDir();
    }
}

ImageFinder::~ImageFinder()
{
}

QStringList ImageFinder::supportedImageFormats()
{
    return QStringList() << "png" << "jpg" << "jpeg" << "bmp" << "webp" << "tif" << "tiff";
}

void ImageFinder::findImagesForBook(BookPtr book)
{
    if (!book) {
        return;
    }

    // ISBN为空时，仍尝试通过Excel指定的图片文件名查找
    QString isbn = book->isbn().trimmed();

    QString searchDir;
    if (m_isZipMode) {
        searchDir = m_tempDirPath;
        if (!m_basePathInZip.isEmpty()) {
            searchDir = QDir(searchDir).filePath(m_basePathInZip);
        }
    } else {
        searchDir = m_dirPath;
    }

    // 从样章/二维码文件名中提取ISBN前缀（用于跨ISBN查找图片）
    // 当Excel中ISBN与图片文件名中的ISBN不一致时，使用图片文件名中的ISBN
    QString imageIsbn = extractIsbnFromImageNames(book);

    // ========== 查找封面 ==========
    // 策略1：如果Excel显式指定了封面文件名，直接用文件名查找
    // 策略2：从样章/二维码文件名提取ISBN前缀，用该前缀查找封面
    // 策略3：用书的ISBN查找封面（原有逻辑）
    QString coverPath;
    QString coverName = book->coverImageName();
    if (!coverName.isEmpty()) {
        coverPath = findFileByName(coverName, searchDir);
    }
    if (coverPath.isEmpty() && !imageIsbn.isEmpty()) {
        coverPath = findCoverInDir(imageIsbn, searchDir);
    }
    if (coverPath.isEmpty() && !isbn.isEmpty()) {
        coverPath = findCoverInDir(isbn, searchDir);
    }
    if (!coverPath.isEmpty()) {
        book->setCoverImagePath(coverPath);
    }

    // ========== 查找样章图片 ==========
    QStringList sampleImages;
    QStringList sampleNames = book->sampleImageNames();
    if (!sampleNames.isEmpty()) {
        // 使用Excel指定的文件名查找
        for (const QString& name : sampleNames) {
            QString path = findFileByName(name, searchDir);
            if (!path.isEmpty()) {
                sampleImages.append(path);
            }
        }
    }
    if (sampleImages.isEmpty()) {
        // 回退到ISBN模式匹配
        QString patternIsbn = !imageIsbn.isEmpty() ? imageIsbn : isbn;
        if (!patternIsbn.isEmpty()) {
            sampleImages = findPatternImagesInDir(patternIsbn, "pic", searchDir);
        }
    }
    book->setSampleImages(sampleImages);

    // ========== 查找二维码 ==========
    QStringList qrCodes;
    QStringList qrNames = book->qrCodeNames();
    if (!qrNames.isEmpty()) {
        for (const QString& name : qrNames) {
            QString path = findFileByName(name, searchDir);
            if (!path.isEmpty()) {
                qrCodes.append(path);
            }
        }
    }
    if (qrCodes.isEmpty()) {
        QString patternIsbn = !imageIsbn.isEmpty() ? imageIsbn : isbn;
        if (!patternIsbn.isEmpty()) {
            qrCodes = findPatternImagesInDir(patternIsbn, "qr", searchDir);
        }
    }
    book->setQrCodes(qrCodes);
}

QString ImageFinder::findCoverInDir(const QString &isbn, const QString &dirPath)
{
    QDir dir(dirPath);
    if (!dir.exists()) {
        return QString();
    }

    QStringList formats = supportedImageFormats();
    QStringList files = dir.entryList(QDir::Files | QDir::NoDotAndDotDot);

    // 先尝试精确匹配（带正确大小写）
    for (const QString &format : formats) {
        QString targetName = isbn + "." + format;
        for (const QString &file : files) {
            if (file.compare(targetName, Qt::CaseInsensitive) == 0) {
                return dir.absoluteFilePath(file);
            }
        }
    }

    return QString();
}

QString ImageFinder::findCoverInZip(const QString &isbn)
{
    if (!m_zipReader || !m_zipReader->isOpen()) {
        return QString();
    }

    QStringList formats = supportedImageFormats();
    QStringList allFiles = m_zipReader->fileList();

    for (const QString &format : formats) {
        QString targetName = isbn + "." + format;
        for (const QString &file : allFiles) {
            QFileInfo fi(file);
            QString fileName = fi.fileName();
            if (fileName.compare(targetName, Qt::CaseInsensitive) == 0) {
                QString fullPath = buildZipPath(file);
                return QDir(m_tempDirPath).filePath(fullPath);
            }
        }
    }

    return QString();
}

QStringList ImageFinder::findPatternImagesInDir(const QString &isbn, const QString &pattern, const QString &dirPath)
{
    QDir dir(dirPath);
    if (!dir.exists()) {
        return QStringList();
    }

    QStringList formats = supportedImageFormats();
    QStringList files = dir.entryList(QDir::Files | QDir::NoDotAndDotDot);
    QMap<int, QString> matchedFiles;

    // 构建正则表达式：isbn + pattern + 数字 + .扩展名
    // ISBN可能包含连字符，需要转义
    QString escapedIsbn = QRegularExpression::escape(isbn);
    QString patternStr = QString("^%1%2(\\d+)\\.(%3)$")
        .arg(escapedIsbn)
        .arg(pattern)
        .arg(formats.join("|"));
    QRegularExpression rx(patternStr, QRegularExpression::CaseInsensitiveOption);

    for (const QString &file : files) {
        QRegularExpressionMatch match = rx.match(file);
        if (match.hasMatch()) {
            int num = match.captured(1).toInt();
            matchedFiles[num] = dir.absoluteFilePath(file);
        }
    }

    // 按数字顺序排序
    QStringList result;
    for (auto it = matchedFiles.begin(); it != matchedFiles.end(); ++it) {
        result.append(it.value());
    }

    return result;
}

QStringList ImageFinder::findPatternImagesInZip(const QString &isbn, const QString &pattern)
{
    if (!m_zipReader || !m_zipReader->isOpen()) {
        return QStringList();
    }

    QStringList formats = supportedImageFormats();
    QStringList allFiles = m_zipReader->fileList();
    QMap<int, QString> matchedFiles;

    QString escapedIsbn = QRegularExpression::escape(isbn);
    QString patternStr = QString("^%1%2(\\d+)\\.(%3)$")
        .arg(escapedIsbn)
        .arg(pattern)
        .arg(formats.join("|"));
    QRegularExpression rx(patternStr, QRegularExpression::CaseInsensitiveOption);

    for (const QString &file : allFiles) {
        QFileInfo fi(file);
        QString fileName = fi.fileName();
        QRegularExpressionMatch match = rx.match(fileName);
        if (match.hasMatch()) {
            int num = match.captured(1).toInt();
            QString fullPath = buildZipPath(file);
            matchedFiles[num] = QDir(m_tempDirPath).filePath(fullPath);
        }
    }

    QStringList result;
    for (auto it = matchedFiles.begin(); it != matchedFiles.end(); ++it) {
        result.append(it.value());
    }

    return result;
}

QString ImageFinder::buildFilePath(const QString &dirPath, const QString &fileName) const
{
    return QDir(dirPath).filePath(fileName);
}

QString ImageFinder::findFileByName(const QString& fileName, const QString& dirPath)
{
    if (fileName.isEmpty()) {
        return QString();
    }
    QDir dir(dirPath);
    if (!dir.exists()) {
        return QString();
    }
    QStringList files = dir.entryList(QDir::Files | QDir::NoDotAndDotDot);
    for (const QString& file : files) {
        if (file.compare(fileName, Qt::CaseInsensitive) == 0) {
            return dir.absoluteFilePath(file);
        }
    }
    return QString();
}

QString ImageFinder::extractIsbnFromImageNames(BookPtr book)
{
    if (!book) {
        return QString();
    }
    QStringList allNames = book->sampleImageNames() + book->qrCodeNames();
    // 尝试匹配 ISBN + "pic" 或 "qr" + 数字的模式
    // ISBN格式：可能含连字符和数字
    QRegularExpression rx("^(.+?)(pic|qr)\\d+\\.", QRegularExpression::CaseInsensitiveOption);
    for (const QString& name : allNames) {
        QRegularExpressionMatch match = rx.match(name);
        if (match.hasMatch()) {
            return match.captured(1);
        }
    }
    return QString();
}

QString ImageFinder::buildZipPath(const QString &fileName) const
{
    if (m_basePathInZip.isEmpty()) {
        return fileName;
    }
    return QDir(m_basePathInZip).filePath(fileName);
}

int ImageFinder::extractNumberFromFileName(const QString &fileName, const QString &prefix)
{
    QFileInfo fi(fileName);
    QString baseName = fi.baseName();
    if (baseName.startsWith(prefix, Qt::CaseInsensitive)) {
        QString numStr = baseName.mid(prefix.length());
        bool ok;
        int num = numStr.toInt(&ok);
        if (ok) {
            return num;
        }
    }
    return -1;
}

bool ImageFinder::compareImageFileNames(const QString &a, const QString &b, const QString &prefix)
{
    int numA = extractNumberFromFileName(a, prefix);
    int numB = extractNumberFromFileName(b, prefix);
    if (numA >= 0 && numB >= 0) {
        return numA < numB;
    }
    return a < b;
}
