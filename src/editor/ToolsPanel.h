#ifndef TOOLSPANEL_H
#define TOOLSPANEL_H

#include <QWidget>

class QToolButton;
class QButtonGroup;
class QPushButton;

// ============================================================
// ToolsPanel - PS风格工具面板
//
// 提供类似Photoshop的工具选择面板，包括：
//   - 选择工具（默认，用于选中/移动/缩放元素）
//   - 形状工具：矩形、椭圆、直线、圆角矩形
//   - 画笔工具（自由绘制路径）
//   - 油漆桶工具（填充形状颜色）
//   - 吸管工具（拾取画布颜色）
//   - 文字工具（插入文本元素）
//
// 底部提供前景色选择器。
// ============================================================

// 工具类型枚举
enum ToolType {
    ToolSelect = 0,      // 选择工具
    ToolRectangle,       // 矩形
    ToolEllipse,         // 椭圆
    ToolLine,            // 直线
    ToolRoundedRect,     // 圆角矩形
    ToolBrush,           // 画笔
    ToolPaintBucket,     // 油漆桶
    ToolEyedropper,      // 吸管
    ToolText             // 文字插入
};

class ToolsPanel : public QWidget
{
    Q_OBJECT
public:
    explicit ToolsPanel(QWidget* parent = nullptr);

    // 获取当前工具
    int currentTool() const;

    // 获取当前前景色
    QColor foregroundColor() const;

    // 设置前景色（外部更新，如吸管拾取后）
    void setForegroundColor(const QColor& color);

    // 程序化设置工具选中状态（供MainWindow调用）
    void selectTool(int toolId);

signals:
    // 工具切换信号
    void toolChanged(int toolType);
    // 前景色变化信号
    void foregroundColorChanged(const QColor& color);

private:
    QButtonGroup* m_toolGroup;       // 工具按钮互斥组
    QPushButton* m_colorBtn;         // 前景色显示按钮
    QColor m_foregroundColor;        // 当前前景色
    int m_lastToolId;                // 上次选中的工具ID

    // 创建工具按钮的辅助方法（iconId对应ToolIconType枚举）
    QToolButton* createToolButton(int iconId, const QString& tooltip, int toolId);
    void updateColorButton();
};

#endif // TOOLSPANEL_H
