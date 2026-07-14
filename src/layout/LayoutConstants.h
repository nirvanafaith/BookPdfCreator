#ifndef LAYOUTCONSTANTS_H
#define LAYOUTCONSTANTS_H

#include <QColor>
#include <QtGlobal>
#include <QString>

// A4页面尺寸（72DPI下的点单位）
// 注意：保留这些常量用于向后兼容；新代码应使用 PageSizeManager::instance().pageWidth() 等动态接口
const qreal A4_WIDTH = 595.0;
const qreal A4_HEIGHT = 842.0;

// 页面边距（左右上下均50点）
const qreal PAGE_MARGIN = 50.0;

// ============================================================
// 纸张尺寸预设（毫米）
// ============================================================
struct PaperSize {
    QString name;       // 纸张名称（如 "A4"）
    qreal widthMm;      // 宽度（毫米）
    qreal heightMm;     // 高度（毫米）
};

// 预设纸张
static const PaperSize PAPER_A4  = { "A4",  210.0, 297.0 };
static const PaperSize PAPER_A5  = { "A5",  148.0, 210.0 };
static const PaperSize PAPER_A3  = { "A3",  297.0, 420.0 };

// 毫米转点（1mm = 72/25.4 点，基于72DPI）
inline qreal mmToPoints(qreal mm) { return mm * 72.0 / 25.4; }
// 点转毫米
inline qreal pointsToMm(qreal pt) { return pt * 25.4 / 72.0; }

// ============================================================
// PageSizeManager - 动态页面尺寸管理（单例）
//
// 集中管理当前纸张尺寸与边距，编辑场景、视图、PDF导出器
// 均通过此单例获取页面尺寸，实现纸张尺寸的动态切换。
// 默认纸张为A4，默认边距沿用 PAGE_MARGIN（50点）。
//
// 注意：方法均为 inline 实现，无需独立 .cpp 文件，
// 避免修改 .pro 工程文件。
// ============================================================
class PageSizeManager
{
public:
    // 获取单例实例
    static PageSizeManager& instance() {
        static PageSizeManager s_instance;
        return s_instance;
    }

    // 获取当前纸张
    PaperSize currentPaper() const { return m_paper; }

    // 设置当前纸张
    void setPaper(const PaperSize& paper) { m_paper = paper; }

    // 设置自定义纸张尺寸（毫米）
    void setCustomPaper(qreal widthMm, qreal heightMm) {
        m_paper.name = QStringLiteral("自定义");
        m_paper.widthMm = widthMm;
        m_paper.heightMm = heightMm;
    }

    // 获取当前页面尺寸（点，72DPI）
    qreal pageWidth() const { return mmToPoints(m_paper.widthMm); }   // 宽度（点）
    qreal pageHeight() const { return mmToPoints(m_paper.heightMm); } // 高度（点）

    // 获取边距（点，当前固定为 PAGE_MARGIN）
    qreal leftMargin() const { return PAGE_MARGIN; }
    qreal topMargin() const { return PAGE_MARGIN; }
    qreal rightMargin() const { return PAGE_MARGIN; }
    qreal bottomMargin() const { return PAGE_MARGIN; }

private:
    PageSizeManager() : m_paper(PAPER_A4) {}
    PaperSize m_paper;
};

// 颜色定义
// 单图横幅蓝色 #1976D2
const QColor COLOR_BANNER_BLUE(25, 118, 210);
// 信息标签红色 #C62828
const QColor COLOR_LABEL_RED(198, 40, 40);
// 内容简介标题黄色背景 #FFF9C4
const QColor COLOR_INTRO_BG(255, 249, 196);
// 多图横幅橙红色 #E64A19
const QColor COLOR_BANNER_ORANGE(230, 74, 25);
// 分隔线灰色
const QColor COLOR_DIVIDER(200, 200, 200);
// 正文黑色
const QColor COLOR_TEXT_BLACK(50, 50, 50);
// 白色
const QColor COLOR_WHITE(255, 255, 255);
// 占位图灰色
const QColor COLOR_PLACEHOLDER(220, 220, 220);

// 字体大小定义
const int FONT_TITLE_SIZE = 18;    // 横幅标题
const int FONT_LABEL_SIZE = 12;    // 信息标签
const int FONT_BOOKNAME_SIZE = 14; // 书名
const int FONT_BODY_SIZE = 10;     // 正文
const int FONT_SMALL_SIZE = 9;     // 小字

// 其他常量
const int BANNER_HEIGHT = 50;
const int CORNER_RADIUS = 5;

#endif // LAYOUTCONSTANTS_H
