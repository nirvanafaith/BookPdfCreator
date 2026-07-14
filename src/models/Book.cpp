#include "Book.h"

BookData::BookData()
    : QSharedData()
    , m_serialNumber(0)
{
}

BookData::BookData(const BookData &other)
    : QSharedData(other)
    , m_serialNumber(other.m_serialNumber)
    , m_category(other.m_category)
    , m_title(other.m_title)
    , m_seriesName(other.m_seriesName)
    , m_isbn(other.m_isbn)
    , m_author(other.m_author)
    , m_price(other.m_price)
    , m_publishDate(other.m_publishDate)
    , m_description(other.m_description)
    , m_coverImagePath(other.m_coverImagePath)
    , m_sampleImages(other.m_sampleImages)
    , m_qrCodes(other.m_qrCodes)
    , m_coverImageName(other.m_coverImageName)
    , m_sampleImageNames(other.m_sampleImageNames)
    , m_qrCodeNames(other.m_qrCodeNames)
    , m_txtFiles(other.m_txtFiles)
{
}

BookData::~BookData()
{
}

int BookData::serialNumber() const
{
    return m_serialNumber;
}

void BookData::setSerialNumber(int number)
{
    m_serialNumber = number;
}

QString BookData::category() const
{
    return m_category;
}

void BookData::setCategory(const QString &category)
{
    m_category = category;
}

QString BookData::title() const
{
    return m_title;
}

void BookData::setTitle(const QString &title)
{
    m_title = title;
}

QString BookData::seriesName() const
{
    return m_seriesName;
}

void BookData::setSeriesName(const QString &series)
{
    m_seriesName = series;
}

QString BookData::isbn() const
{
    return m_isbn;
}

void BookData::setIsbn(const QString &isbn)
{
    m_isbn = isbn;
}

QString BookData::author() const
{
    return m_author;
}

void BookData::setAuthor(const QString &author)
{
    m_author = author;
}

QString BookData::price() const
{
    return m_price;
}

void BookData::setPrice(const QString &price)
{
    m_price = price;
}

QString BookData::publishDate() const
{
    return m_publishDate;
}

void BookData::setPublishDate(const QString &date)
{
    m_publishDate = date;
}

QString BookData::description() const
{
    return m_description;
}

void BookData::setDescription(const QString &desc)
{
    m_description = desc;
}

QString BookData::coverImagePath() const
{
    return m_coverImagePath;
}

void BookData::setCoverImagePath(const QString &path)
{
    m_coverImagePath = path;
}

QStringList BookData::sampleImages() const
{
    return m_sampleImages;
}

void BookData::setSampleImages(const QStringList &images)
{
    m_sampleImages = images;
}

void BookData::addSampleImage(const QString &path)
{
    if (!path.isEmpty() && !m_sampleImages.contains(path)) {
        m_sampleImages.append(path);
    }
}

QStringList BookData::qrCodes() const
{
    return m_qrCodes;
}

void BookData::setQrCodes(const QStringList &codes)
{
    m_qrCodes = codes;
}

void BookData::addQrCode(const QString &path)
{
    if (!path.isEmpty() && !m_qrCodes.contains(path)) {
        m_qrCodes.append(path);
    }
}

QString BookData::coverImageName() const
{
    return m_coverImageName;
}

void BookData::setCoverImageName(const QString& name)
{
    m_coverImageName = name;
}

QStringList BookData::sampleImageNames() const
{
    return m_sampleImageNames;
}

void BookData::setSampleImageNames(const QStringList& names)
{
    m_sampleImageNames = names;
}

QStringList BookData::qrCodeNames() const
{
    return m_qrCodeNames;
}

void BookData::setQrCodeNames(const QStringList& names)
{
    m_qrCodeNames = names;
}

bool BookData::hasSeries() const
{
    return !m_seriesName.trimmed().isEmpty();
}

bool BookData::hasSampleImages() const
{
    return !m_sampleImages.isEmpty();
}

bool BookData::hasQrCodes() const
{
    return !m_qrCodes.isEmpty();
}

QStringList BookData::txtFiles() const
{
    return m_txtFiles;
}

void BookData::setTxtFiles(const QStringList& files)
{
    m_txtFiles = files;
}

void BookData::addTxtFile(const QString& path)
{
    if (!path.isEmpty() && !m_txtFiles.contains(path)) {
        m_txtFiles.append(path);
    }
}

bool BookData::hasTxtFiles() const
{
    return !m_txtFiles.isEmpty();
}
