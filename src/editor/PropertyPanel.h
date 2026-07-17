#ifndef PROPERTYPANEL_H
#define PROPERTYPANEL_H

#include <QWidget>
#include "PageElement.h"

class QGroupBox;
class QDoubleSpinBox;
class QSpinBox;
class QPushButton;
class QFontComboBox;
class QButtonGroup;
class QSlider;
class QLabel;
class QCheckBox;
class QTextEdit;

// ============================================================
// PropertyPanel - 属性面板
//
// 显示和编辑选中页面元素的属性。
// 根据元素类型（文本/图片/形状）动态显示不同的属性组：
//   - 通用属性组（变换）：位置、尺寸、旋转 —— 选中元素时始终显示
//   - 文本属性组：字体、字号、颜色、对齐 —— 仅选中文本元素时显示
//   - 图片属性组：不透明度、缩放 —— 仅选中图片元素时显示
//
// 信号流：
//   用户编辑控件 → 检查m_updating → 发射对应*Changed信号
//   外部调用showElementProperties() → 置m_updating=true → 填充控件值 → 置m_updating=false
//   m_updating标志防止"设置控件值→触发信号→再次设置"的循环。
//
// 数据访问：
//   PageElementPtr是QSharedDataPointer<PageElementData>，
//   PageElementData为抽象类（含纯虚clone()），只能const访问。
//   类型特定属性通过constData() + static_cast<const派生类*>获取。
// ============================================================
class PropertyPanel : public QWidget
{
    Q_OBJECT
public:
    explicit PropertyPanel(QWidget* parent = nullptr);

    // 显示指定元素的属性（null时等同于clearProperties）
    void showElementProperties(const PageElementPtr& element);

    // 清空属性面板（无选中时调用，显示提示文字）
    void clearProperties();

signals:
    // 通用属性变更信号
    void positionChanged(const QString& elementId, const QPointF& pos);
    void sizeChanged(const QString& elementId, const QSizeF& size);
    void rotationChanged(const QString& elementId, qreal rotation);

    // 文本属性变更信号
    void fontChanged(const QString& elementId, const QFont& font);
    void fontSizeChanged(const QString& elementId, int size);
    void textColorChanged(const QString& elementId, const QColor& color);
    void alignmentChanged(const QString& elementId, Qt::Alignment align);
    void lineHeightChanged(const QString& elementId, qreal height);
    void letterSpacingChanged(const QString& elementId, qreal spacing);

    // 文本内容变更信号（属性面板编辑文本内容时发射）
    void textContentChanged(const QString& elementId, const QString& newText);

    // 图片属性变更信号
    void opacityChanged(const QString& elementId, qreal opacity);
    void scaleFactorChanged(const QString& elementId, qreal scale);

    // 形状属性变更信号
    void fillEnabledChanged(const QString& elementId, bool enabled);
    void fillColorChanged(const QString& elementId, const QColor& color);
    void borderColorChanged(const QString& elementId, const QColor& color);
    void borderWidthChanged(const QString& elementId, qreal width);

private:
    // ---- 通用属性组（变换） ----
    QGroupBox* m_transformGroup;
    QDoubleSpinBox* m_xSpin;
    QDoubleSpinBox* m_ySpin;
    QDoubleSpinBox* m_wSpin;
    QDoubleSpinBox* m_hSpin;
    QDoubleSpinBox* m_rotationSpin;

    // ---- 文本属性组 ----
    QGroupBox* m_textGroup;
    QFontComboBox* m_fontCombo;
    QSpinBox* m_fontSizeSpin;
    QPushButton* m_textColorBtn;
    QButtonGroup* m_alignGroup;       // 互斥对齐按钮组
    QPushButton* m_alignLeftBtn;
    QPushButton* m_alignCenterBtn;
    QPushButton* m_alignRightBtn;
    QPushButton* m_alignJustifyBtn;
    QDoubleSpinBox* m_lineHeightSpin;      // 行间距
    QDoubleSpinBox* m_letterSpacingSpin;   // 字间距
    QTextEdit* m_contentEdit;          // 文本内容编辑框（固定高度，垂直滚动条）

    // ---- 图片属性组 ----
    QGroupBox* m_imageGroup;
    QSlider* m_opacitySlider;
    QSlider* m_scaleSlider;
    QSpinBox* m_opacityValue;         // 不透明度百分比输入
    QSpinBox* m_scaleValue;           // 缩放百分比输入

    // ---- 形状属性组 ----
    QGroupBox* m_shapeGroup;
    QCheckBox* m_fillCheck;
    QPushButton* m_fillColorBtn;
    QPushButton* m_borderColorBtn;
    QDoubleSpinBox* m_borderWidthSpin;
    QWidget* m_fillWidget;            // 填充label+button容器，便于整体显隐
    QColor m_currentFillColor;
    QColor m_currentBorderColor;

    // ---- 提示标签（无选中时显示） ----
    QLabel* m_hintLabel;

    // ---- 状态 ----
    QString m_currentElementId;       // 当前编辑的元素ID
    QColor m_currentTextColor;        // 当前文字颜色（用于颜色按钮）
    bool m_updating;                  // 防止信号循环标志

    // ---- 私有方法 ----
    void setupUi();
    void setupConnections();
    void showTextGroup(bool show);
    void showImageGroup(bool show);
    void showShapeGroup(bool show);
    void updateColorButton();         // 更新颜色按钮外观
    void updateShapeColorButtons();   // 更新形状颜色按钮外观
    void setupContentEditConnections();  // 设置内容编辑框的连接（事件过滤器）

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;
};

#endif // PROPERTYPANEL_H
