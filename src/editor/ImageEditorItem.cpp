#include "ImageEditorItem.h"
#include "ImageCache.h"

#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <QRectF>
#include <QSizeF>
#include <QtGlobal>

// ============================================================
// 构造函数
//
// ImageElementPtr -> PageElementPtr 转换：
// 通过clone()获取非const的ImageElementData*，上转为PageElementData*，
// 构造PageElementPtr传给基类。
// ============================================================
ImageEditorItem::ImageEditorItem(const ImageElementPtr& element, QGraphicsItem* parent)
    : BaseEditorItem(PageElementPtr(element.constData()->clone()), parent)
{
}

// ============================================================
// paint - 图片渲染
//
// 委托给基类paint，后者调用ElementRenderer::renderElement。
// ElementRenderer::renderImage已完整处理：
//   - 通过ImageCache获取图片（带缓存）
//   - 不透明度（painter->setOpacity）
//   - scaleFactor / keepAspectRatio 缩放策略
//   - 图片加载失败时绘制占位符
//
// 注意：基类paint内部调用drawSelectionBorder()，由于该方法是虚函数，
// 实际会调用本类的drawSelectionBorder()实现（贴合图片实际区域）。
// ============================================================
void ImageEditorItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
                            QWidget* widget)
{
    BaseEditorItem::paint(painter, option, widget);
}

// ============================================================
// computeActualImageRect - 计算图片实际渲染区域
//
// 复制ElementRenderer::renderImage中的目标区域计算逻辑，
// 得到图片在item本地坐标系（0,0=rect.topLeft）中的实际位置。
//
// 缩放策略：
//   - scaleFactor != 1.0：原始尺寸 * scaleFactor
//   - scaleFactor == 1.0 且 keepAspectRatio：保持宽高比缩放到rect内
//   - scaleFactor == 1.0 且 !keepAspectRatio：拉伸填充rect
// 最后左上角对齐rect.topLeft（setTopLeft(rect.topLeft)）。
//
// 返回无效QRectF表示图片未加载，调用方应回退到基类实现。
// ============================================================
QRectF ImageEditorItem::computeActualImageRect() const
{
    const ImageElementData* image =
        static_cast<const ImageElementData*>(m_element.constData());
    QRectF rect = image->rect();
    QString path = image->imagePath();

    if (path.isEmpty()) {
        return QRectF();  // 无路径，回退
    }

    // 通过ImageCache获取原始尺寸图片（与渲染时一致，不预缩放避免双重缩放）
    QPixmap pixmap = ImageCache::instance().getPixmap(path, QSize());
    if (pixmap.isNull()) {
        return QRectF();  // 图片加载失败，回退
    }

    // 计算实际绘制尺寸（逻辑与ElementRenderer::renderImage完全一致）
    QSizeF actualSize;
    qreal scaleFactor = image->scaleFactor();

    if (qAbs(scaleFactor - 1.0) > 0.001) {
        // 使用scaleFactor缩放原始图片尺寸
        actualSize = QSizeF(pixmap.width() * scaleFactor,
                            pixmap.height() * scaleFactor);
    } else {
        if (image->keepAspectRatio()) {
            // 保持宽高比：缩放到rect内
            actualSize = QSizeF(pixmap.size());
            actualSize.scale(rect.size(), Qt::KeepAspectRatio);
        } else {
            // 拉伸填充整个rect
            actualSize = rect.size();
        }
    }

    // 左上角对齐rect.topLeft（与ElementRenderer::renderImage保持一致）
    QRectF targetRect;
    targetRect.setSize(actualSize);
    targetRect.setTopLeft(rect.topLeft());
    // 转换到item本地坐标系：平移使rect.topLeft成为原点
    targetRect.translate(-rect.topLeft());

    return targetRect;
}

// ============================================================
// boundingRect - 贴合图片实际渲染区域的包围矩形
//
// 当keepAspectRatio=true时，图片在rect内居中并留白，
// 此方法返回图片实际区域+SELECTION_MARGIN，而非整个rect。
// 图片未加载时回退到基类实现（整个rect+margin）。
// ============================================================
QRectF ImageEditorItem::boundingRect() const
{
    QRectF imageRect = computeActualImageRect();
    if (!imageRect.isValid()) {
        // 图片未加载，回退到基类（容器rect + margin）
        return BaseEditorItem::boundingRect();
    }

    const qreal m = SELECTION_MARGIN;
    return QRectF(imageRect.x() - m, imageRect.y() - m,
                  imageRect.width() + 2.0 * m, imageRect.height() + 2.0 * m);
}

// ============================================================
// drawSelectionBorder - 在图片实际渲染区域绘制选中边框
//
// 覆盖基类实现，将边框绘制在图片实际区域而非容器rect边缘。
// 图片未加载时回退到基类实现。
// ============================================================
void ImageEditorItem::drawSelectionBorder(QPainter* painter)
{
    QRectF imageRect = computeActualImageRect();
    if (!imageRect.isValid()) {
        // 图片未加载，回退到基类（在容器rect边缘绘制）
        BaseEditorItem::drawSelectionBorder(painter);
        return;
    }

    painter->save();
    QPen pen(QColor(25, 118, 210), 1, Qt::DashLine);
    painter->setPen(pen);
    painter->setBrush(Qt::NoBrush);

    painter->drawRect(imageRect);

    painter->restore();
}
