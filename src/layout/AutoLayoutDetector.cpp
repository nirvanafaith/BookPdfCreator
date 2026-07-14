#include "AutoLayoutDetector.h"

// 检测布局模式
LayoutMode AutoLayoutDetector::detectLayout(const BookList& books)
{
    // 检查是否有任意一本书包含样章配图或二维码
    for (const BookPtr& book : books) {
        if (book->hasSampleImages() || book->hasQrCodes()) {
            // 存在图片或二维码时使用单栏布局，确保内容展示完整
            return LayoutMode::SingleColumn;
        }
    }

    // 所有书籍都没有样章配图和二维码时
    if (books.size() >= 3) {
        // 书籍数量≥3时使用多栏布局，提高页面利用率
        return LayoutMode::MultiColumn;
    }

    // 其他情况默认使用单栏布局
    return LayoutMode::SingleColumn;
}
