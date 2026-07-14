#include "ExcelParser.h"
#include "xlsxdocument.h"
#include "xlsxcell.h"
#include <QBuffer>
#include <QVariant>
#include <QDebug>
#include <QtMath>
#include <memory>

ExcelParser::ExcelParser()
{
    initColumnMapping();
}

ExcelParser::~ExcelParser()
{
}

void ExcelParser::initColumnMapping()
{
    // 初始化所有列索引为-1（未找到）
    for (int i = 0; i < COL_COUNT; ++i) {
        m_columns[i] = -1;
    }
}

BookList ExcelParser::parseFromFile(const QString& filePath, QString* errorMessage)
{
    // 从文件路径加载QXlsx文档
    QXlsx::Document xlsx(filePath);
    
    // 检查是否加载成功
    if (!xlsx.isLoadPackage()) {
        if (errorMessage) {
            *errorMessage = QString("无法打开Excel文件: %1").arg(filePath);
        }
        return BookList();
    }
    
    return parseInternal(xlsx, errorMessage);
}

BookList ExcelParser::parseFromData(const QByteArray& data, QString* errorMessage)
{
    // 使用QBuffer包装QByteArray，以便QIODevice接口读取
    QBuffer buffer;
    buffer.setData(data);
    if (!buffer.open(QIODevice::ReadOnly)) {
        if (errorMessage) {
            *errorMessage = "无法打开内存数据缓冲区";
        }
        return BookList();
    }
    
    // 从QIODevice加载QXlsx文档
    QXlsx::Document xlsx(&buffer);
    
    // 检查是否加载成功
    if (!xlsx.isLoadPackage()) {
        if (errorMessage) {
            *errorMessage = "无法从内存数据解析Excel文件";
        }
        return BookList();
    }
    
    return parseInternal(xlsx, errorMessage);
}

QString ExcelParser::readCellAsString(QXlsx::Document& xlsx, int row, int col, bool isNumericIsbn)
{
    QVariant value = xlsx.read(row, col);
    
    // 空单元格返回空字符串
    if (!value.isValid() || value.isNull()) {
        return QString();
    }
    
    // 处理单元格类型
    QXlsx::Cell* cell = xlsx.cellAt(row, col).get();
    
    if (cell) {
        // 如果是ISBN字段且是数字类型，需要特殊处理保留前导零
        if (isNumericIsbn && (value.type() == QVariant::Int || 
                              value.type() == QVariant::UInt ||
                              value.type() == QVariant::LongLong ||
                              value.type() == QVariant::ULongLong ||
                              value.type() == QVariant::Double)) {
            // 将数字转换为字符串，避免科学计数法
            // ISBN-10是10位，ISBN-13是13位
            QString isbnStr;
            if (value.type() == QVariant::Double) {
                // 双精度浮点数，使用固定格式输出，保留0位小数
                double num = value.toDouble();
                // 检查是否是整数（防止浮点数精度问题）
                if (qAbs(num - qRound64(num)) < 0.000001) {
                    isbnStr = QString::number(qRound64(num), 'f', 0);
                } else {
                    isbnStr = value.toString();
                }
            } else {
                isbnStr = value.toString();
            }
            
            // 移除可能的小数点和后面的零（如果是Double转来的）
            int dotPos = isbnStr.indexOf('.');
            if (dotPos != -1) {
                isbnStr = isbnStr.left(dotPos);
            }
            
            return isbnStr;
        }
        
        // 处理字符串类型，保留换行符
        if (value.type() == QVariant::String) {
            return value.toString();
        }
    }
    
    // 默认转换为字符串
    return value.toString();
}

BookList ExcelParser::parseInternal(QXlsx::Document& xlsx, QString* errorMessage)
{
    BookList result;
    
    // 重置列映射
    initColumnMapping();
    
    // 获取当前工作表
    QXlsx::Worksheet* worksheet = xlsx.currentWorksheet();
    if (!worksheet) {
        if (errorMessage) {
            *errorMessage = "无法获取当前工作表";
        }
        return result;
    }
    
    // 获取数据范围
    QXlsx::CellRange dimension = xlsx.dimension();
    int firstRow = dimension.firstRow();
    int lastRow = dimension.lastRow();
    int firstCol = dimension.firstColumn();
    int lastCol = dimension.lastColumn();
    
    // 检查是否有数据
    if (firstRow > lastRow || firstCol > lastCol) {
        if (errorMessage) {
            *errorMessage = "Excel文件是空的";
        }
        return result;
    }
    
    // 第一步：解析第一行表头，建立列名到列索引的映射
    const int headerRow = firstRow;
    for (int col = firstCol; col <= lastCol; ++col) {
        QString headerName = readCellAsString(xlsx, headerRow, col).trimmed();
        
        // 匹配中文表头
        if (headerName == "序号" || headerName == "编号" || headerName == "No." || headerName == "No") {
            m_columns[COL_SERIAL_NUMBER] = col;
        } else if (headerName == "类别" || headerName == "分类" || headerName == "图书类别") {
            m_columns[COL_CATEGORY] = col;
        } else if (headerName == "书名" || headerName == "图书名称" || headerName == "名称" || headerName == "标题") {
            m_columns[COL_TITLE] = col;
        } else if (headerName == "丛书名" || headerName == "丛书名称" || headerName == "系列名") {
            m_columns[COL_SERIES_NAME] = col;
        } else if (headerName == "ISBN" || headerName == "isbn" || headerName == "ISBN号" || headerName == "书号") {
            m_columns[COL_ISBN] = col;
        } else if (headerName == "作者" || headerName == "编著" || headerName == "著者") {
            m_columns[COL_AUTHOR] = col;
        } else if (headerName == "定价" || headerName == "价格" || headerName == "售价") {
            m_columns[COL_PRICE] = col;
        } else if (headerName == "出版时间" || headerName == "出版日期" || headerName == "出版年月" || headerName == "出版日期时间") {
            m_columns[COL_PUBLISH_DATE] = col;
        } else if (headerName == "内容简介" || headerName == "简介" || headerName == "内容介绍" || headerName == "图书简介") {
            m_columns[COL_DESCRIPTION] = col;
        } else if (headerName == "样章配图" || headerName == "样章" || headerName == "配图") {
            m_columns[COL_SAMPLE_IMAGES] = col;
        } else if (headerName == "二维码图片" || headerName == "二维码" || headerName == "二维码码") {
            m_columns[COL_QR_CODES] = col;
        }
        // 其他列忽略，图片通过文件发现机制处理
    }
    
    // 检查必须的列（书名是必须的）
    if (m_columns[COL_TITLE] == -1) {
        if (errorMessage) {
            *errorMessage = "未找到\"书名\"列，请检查表头是否正确";
        }
        return result;
    }
    
    // 第二步：从第二行开始读取数据
    for (int row = headerRow + 1; row <= lastRow; ++row) {
        // 创建书籍数据对象
        BookPtr book = BookPtr(new BookData());
        
        // 读取序号
        if (m_columns[COL_SERIAL_NUMBER] != -1) {
            QString serialStr = readCellAsString(xlsx, row, m_columns[COL_SERIAL_NUMBER]);
            bool ok = false;
            int serial = serialStr.toInt(&ok);
            if (ok) {
                book->setSerialNumber(serial);
            }
        }
        
        // 读取类别
        if (m_columns[COL_CATEGORY] != -1) {
            book->setCategory(readCellAsString(xlsx, row, m_columns[COL_CATEGORY]).trimmed());
        }
        
        // 读取书名（必填）
        QString title = readCellAsString(xlsx, row, m_columns[COL_TITLE]).trimmed();
        if (title.isEmpty()) {
            // 书名为空，跳过这一行（可能是空行）
            continue;
        }
        book->setTitle(title);
        
        // 读取丛书名
        if (m_columns[COL_SERIES_NAME] != -1) {
            book->setSeriesName(readCellAsString(xlsx, row, m_columns[COL_SERIES_NAME]).trimmed());
        }
        
        // 读取ISBN（特殊处理数字格式）
        if (m_columns[COL_ISBN] != -1) {
            book->setIsbn(readCellAsString(xlsx, row, m_columns[COL_ISBN], true).trimmed());
        }
        
        // 读取作者
        if (m_columns[COL_AUTHOR] != -1) {
            book->setAuthor(readCellAsString(xlsx, row, m_columns[COL_AUTHOR]).trimmed());
        }
        
        // 读取定价
        if (m_columns[COL_PRICE] != -1) {
            book->setPrice(readCellAsString(xlsx, row, m_columns[COL_PRICE]).trimmed());
        }
        
        // 读取出版时间
        if (m_columns[COL_PUBLISH_DATE] != -1) {
            book->setPublishDate(readCellAsString(xlsx, row, m_columns[COL_PUBLISH_DATE]).trimmed());
        }
        
        // 读取内容简介（保留换行符，只trim首尾空白）
        if (m_columns[COL_DESCRIPTION] != -1) {
            QString desc = readCellAsString(xlsx, row, m_columns[COL_DESCRIPTION]);
            // 只去除首尾空白，保留中间的换行符
            desc = desc.trimmed();
            book->setDescription(desc);
        }
        
        // 读取样章配图文件名（J列，逗号分隔）
        if (m_columns[COL_SAMPLE_IMAGES] != -1) {
            QString sampleStr = readCellAsString(xlsx, row, m_columns[COL_SAMPLE_IMAGES]).trimmed();
            if (!sampleStr.isEmpty()) {
                QStringList names = sampleStr.split(',', QString::SkipEmptyParts);
                for (QString& name : names) {
                    name = name.trimmed();
                }
                book->setSampleImageNames(names);
            }
        }

        // 读取二维码图片文件名（K列，逗号分隔）
        if (m_columns[COL_QR_CODES] != -1) {
            QString qrStr = readCellAsString(xlsx, row, m_columns[COL_QR_CODES]).trimmed();
            if (!qrStr.isEmpty()) {
                QStringList names = qrStr.split(',', QString::SkipEmptyParts);
                for (QString& name : names) {
                    name = name.trimmed();
                }
                book->setQrCodeNames(names);
            }
        }
        
        // 添加到结果列表
        result.append(book);
    }
    
    return result;
}
