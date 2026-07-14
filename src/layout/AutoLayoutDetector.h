#ifndef AUTOLAYOUTDETECTOR_H
#define AUTOLAYOUTDETECTOR_H

#include "models/Book.h"
#include "models/LayoutMode.h"

// 自动排版检测器
// 根据书籍内容自动判断适合单栏还是多栏布局
class AutoLayoutDetector
{
public:
    // 检测布局模式
    // 参数: books - 书籍列表
    // 返回值: 推荐的布局模式
    static LayoutMode detectLayout(const BookList& books);
};

#endif // AUTOLAYOUTDETECTOR_H
