#include "LayoutEngine.h"

// ============================================================
// LayoutEngine 基类默认实现
// ============================================================

// generateElements 默认实现：返回空列表
// 子类（SingleColumnLayout、MultiColumnLayout）应该重写此方法，
// 根据排版逻辑生成具体的页面元素列表。
QList<PageElementPtr> LayoutEngine::generateElements(int pageIndex, const QRectF& pageRect)
{
    Q_UNUSED(pageIndex)
    Q_UNUSED(pageRect)
    return QList<PageElementPtr>();
}

// pageType 默认实现：返回0（单图排版）
// 子类应该重写此方法，返回正确的页面类型。
// 0=单图排版（SingleColumnLayout）
// 1=多图排版（MultiColumnLayout）
int LayoutEngine::pageType(int pageIndex) const
{
    Q_UNUSED(pageIndex)
    return 0;
}
