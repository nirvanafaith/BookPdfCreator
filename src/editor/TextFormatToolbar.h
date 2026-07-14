#ifndef TEXTFORMATTOOLBAR_H
#define TEXTFORMATTOOLBAR_H

#include <QWidget>
#include <QFont>
#include <QColor>
#include <Qt>

class QFontComboBox;
class QSpinBox;
class QPushButton;
class QButtonGroup;

// ============================================================
// TextFormatToolbar - 文本格式化浮动工具栏
//
// 在文本编辑模式下选中文本时，浮动显示在选区上方。
// 提供字体、字号、粗体、斜体、颜色、对齐方式的快速格式化操作。
//
// 使用方式：
//   1. 创建实例（parent 通常为 EditorView 或主窗口）
//   2. 连接各格式化信号到实际处理槽（应用到选中文本/元素）
//   3. 选中文本时调用 updateFormatState() 同步按钮状态
//   4. 调用 showAt(globalPos) 在指定位置显示
//   5. 鼠标点击工具栏外部自动隐藏
//
// 窗口属性：
//   Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint
//   无边框、置顶、工具类型。
//   额外设置 Qt::WA_ShowWithoutActivating，显示时不抢占主窗口激活，
//   避免文本编辑器因焦点丢失而过早退出编辑模式。
//
// 设计说明：
//   - 工具栏为顶层 QWidget（独立窗口），由调用方持有所有权。
//   - updateFormatState() 内部阻塞信号，避免回写时触发格式化信号
//     形成反馈循环。
//   - 对齐按钮使用 QButtonGroup 互斥，并通过水平对齐掩码过滤
//     可能携带的垂直对齐标志（如 AlignVCenter）。
// ============================================================
class TextFormatToolbar : public QWidget
{
    Q_OBJECT
public:
    explicit TextFormatToolbar(QWidget* parent = nullptr);

    // 显示在指定位置（全局坐标），自动调整避免超出屏幕
    void showAt(const QPoint& globalPos);

    // 设置当前选中文本的格式状态（更新按钮状态，不发射信号）
    void updateFormatState(const QFont& currentFont, const QColor& currentColor,
                           Qt::Alignment currentAlignment);

signals:
    // 格式化请求信号
    void fontChanged(const QFont& font);
    void fontSizeChanged(int size);
    void boldToggled(bool bold);
    void italicToggled(bool italic);
    void colorChanged(const QColor& color);
    void alignmentChanged(Qt::Alignment align);

protected:
    // 事件过滤器：监听全局鼠标按下，点击工具栏外部时隐藏
    bool eventFilter(QObject* watched, QEvent* event) override;
    // 显示事件：安装全局事件过滤器
    void showEvent(QShowEvent* event) override;
    // 隐藏事件：移除全局事件过滤器
    void hideEvent(QHideEvent* event) override;

private:
    QFontComboBox* m_fontCombo;      // 字体选择
    QSpinBox* m_sizeSpin;            // 字号
    QPushButton* m_boldBtn;          // 粗体
    QPushButton* m_italicBtn;        // 斜体
    QPushButton* m_colorBtn;         // 颜色
    QPushButton* m_alignLeftBtn;     // 左对齐
    QPushButton* m_alignCenterBtn;   // 居中
    QPushButton* m_alignRightBtn;    // 右对齐
    QPushButton* m_alignJustifyBtn;  // 两端对齐
    QButtonGroup* m_alignGroup;      // 对齐按钮组（互斥）

    QColor m_currentColor;           // 当前颜色

    // 构建 UI
    void setupUi();
    // 建立信号槽连接
    void setupConnections();
    // 更新颜色按钮的显示样式（显示当前颜色）
    void updateColorButton();
};

#endif // TEXTFORMATTOOLBAR_H
