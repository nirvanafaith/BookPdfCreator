#ifndef EXCELPARSER_H
#define EXCELPARSER_H

#include <QString>
#include <QByteArray>
#include "Book.h"

// QXlsx命名空间前向声明（避免在头文件中包含QXlsx实现头文件）
namespace QXlsx { class Document; }

// Excel解析器类，用于从xlsx文件解析书籍列表
class ExcelParser
{
public:
    ExcelParser();
    ~ExcelParser();

    // 从xlsx文件路径解析书籍列表
    // filePath: xlsx文件路径
    // errorMessage: 错误信息输出参数，解析失败时设置
    // 返回: 解析成功返回书籍列表，失败返回空列表
    BookList parseFromFile(const QString& filePath, QString* errorMessage = nullptr);

    // 从内存数据解析书籍列表（用于ZIP中的Excel文件）
    // data: xlsx文件的二进制数据
    // errorMessage: 错误信息输出参数，解析失败时设置
    // 返回: 解析成功返回书籍列表，失败返回空列表
    BookList parseFromData(const QByteArray& data, QString* errorMessage = nullptr);

private:
    // 列索引枚举，对应表头字段
    enum ColumnIndex {
        COL_SERIAL_NUMBER = 0,  // 序号
        COL_CATEGORY,           // 类别
        COL_TITLE,              // 书名
        COL_SERIES_NAME,        // 丛书名
        COL_ISBN,               // ISBN
        COL_AUTHOR,             // 作者
        COL_PRICE,              // 定价
        COL_PUBLISH_DATE,       // 出版时间
        COL_DESCRIPTION,        // 内容简介
        COL_SAMPLE_IMAGES,      // 样章配图（J列）
        COL_QR_CODES,           // 二维码图片（K列）
        COL_COUNT               // 列总数
    };

    // 内部解析实现
    BookList parseInternal(QXlsx::Document& xlsx, QString* errorMessage);

    // 读取单元格值并转换为字符串，处理各种数据类型
    // row: 行号（从1开始）
    // col: 列号（从1开始）
    // isNumericIsbn: 是否是ISBN字段（需要特殊处理数字转字符串保留前导零）
    QString readCellAsString(QXlsx::Document& xlsx, int row, int col, bool isNumericIsbn = false);

    // 初始化表头列索引为-1（未找到）
    void initColumnMapping();

    // 表头列索引数组，初始化为-1表示未找到
    int m_columns[COL_COUNT];
};

#endif // EXCELPARSER_H
