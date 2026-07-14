#ifndef BOOK_H
#define BOOK_H

#include <QSharedData>
#include <QSharedDataPointer>
#include <QSharedPointer>
#include <QString>
#include <QStringList>
#include <QList>

// 书籍数据类，支持隐式数据共享
class BookData : public QSharedData
{
public:
    BookData();
    BookData(const BookData &other);
    ~BookData();

    // 基本信息字段
    int serialNumber() const;
    void setSerialNumber(int number);

    QString category() const;
    void setCategory(const QString &category);

    QString title() const;
    void setTitle(const QString &title);

    QString seriesName() const;
    void setSeriesName(const QString &series);

    QString isbn() const;
    void setIsbn(const QString &isbn);

    QString author() const;
    void setAuthor(const QString &author);

    QString price() const;
    void setPrice(const QString &price);

    QString publishDate() const;
    void setPublishDate(const QString &date);

    QString description() const;
    void setDescription(const QString &desc);

    // 图片路径字段
    QString coverImagePath() const;
    void setCoverImagePath(const QString &path);

    QStringList sampleImages() const;
    void setSampleImages(const QStringList &images);
    void addSampleImage(const QString &path);

    QStringList qrCodes() const;
    void setQrCodes(const QStringList &codes);
    void addQrCode(const QString &path);

    // Excel中显式指定的图片文件名（用于跨ISBN查找图片）
    QString coverImageName() const;
    void setCoverImageName(const QString& name);
    QStringList sampleImageNames() const;
    void setSampleImageNames(const QStringList& names);
    QStringList qrCodeNames() const;
    void setQrCodeNames(const QStringList& names);

    // TXT文件路径字段（导入时扫描关联的TXT文件）
    QStringList txtFiles() const;
    void setTxtFiles(const QStringList& files);
    void addTxtFile(const QString& path);

    // 便捷判断方法
    bool hasSeries() const;
    bool hasSampleImages() const;
    bool hasQrCodes() const;
    bool hasTxtFiles() const;

private:
    int m_serialNumber;          // 序号
    QString m_category;          // 类别
    QString m_title;             // 书名
    QString m_seriesName;        // 丛书名
    QString m_isbn;              // ISBN
    QString m_author;            // 作者
    QString m_price;             // 定价
    QString m_publishDate;       // 出版时间
    QString m_description;       // 内容简介（支持\n分段）

    QString m_coverImagePath;    // 封面图路径
    QStringList m_sampleImages;  // 样章配图路径列表
    QStringList m_qrCodes;       // 二维码路径列表

    QString m_coverImageName;        // Excel中指定的封面文件名（可选）
    QStringList m_sampleImageNames;  // Excel中指定的样章图片文件名
    QStringList m_qrCodeNames;       // Excel中指定的二维码图片文件名

    QStringList m_txtFiles;          // TXT文件路径列表
};

// 书籍智能指针类型别名
using BookPtr = QSharedPointer<BookData>;
// 书籍列表类型别名
using BookList = QList<BookPtr>;

// 隐式共享指针类型别名（可选使用）
using BookDataPtr = QSharedDataPointer<BookData>;

#endif // BOOK_H
