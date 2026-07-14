#include "Commands.h"
#include "EditorScene.h"
#include "BaseEditorItem.h"
#include "TextEditorItem.h"
#include "ImageEditorItem.h"
#include "ShapeEditorItem.h"

#include <QGraphicsItem>

// ============================================================
// 文件局部辅助函数
//
// 以下static函数仅在本.cpp文件内可见，用于：
//   1. 通过elementId在场景中查找对应的BaseEditorItem
//   2. 根据元素数据创建对应的编辑器Item（镜像EditorScene::createItemForElement）
//   3. 重建Item以应用rect/text变更（boundingRect依赖元素rect，无法原地修改）
//   4. 应用位置/旋转/层级变更到Item并同步到数据模型
// ============================================================

// 通过elementId在场景中查找对应的BaseEditorItem
// 遍历场景所有Item，dynamic_cast过滤出BaseEditorItem，比较elementData()->id()
static BaseEditorItem* findItemByElementId(EditorScene* scene, const QString& id)
{
    if (!scene || id.isEmpty()) return nullptr;

    const QList<QGraphicsItem*> allItems = scene->items();
    for (QGraphicsItem* it : allItems) {
        if (auto* baseItem = dynamic_cast<BaseEditorItem*>(it)) {
            if (baseItem->elementData().constData()->id() == id) {
                return baseItem;
            }
        }
    }
    return nullptr;
}

// 根据元素数据创建对应的编辑器Item
// 镜像EditorScene::createItemForElement的实现（该方法为private，命令类无法直接调用）
// 各Item构造函数内部会clone元素数据，确保Item拥有独立副本
static BaseEditorItem* createItemForElement(const PageElementPtr& element)
{
    if (!element) return nullptr;

    switch (element->elementType()) {
    case PageElementData::Text: {
        TextElementData* clone =
            static_cast<TextElementData*>(element.constData()->clone());
        return new TextEditorItem(TextElementPtr(clone));
    }
    case PageElementData::Image: {
        ImageElementData* clone =
            static_cast<ImageElementData*>(element.constData()->clone());
        return new ImageEditorItem(ImageElementPtr(clone));
    }
    case PageElementData::Shape: {
        ShapeElementData* clone =
            static_cast<ShapeElementData*>(element.constData()->clone());
        return new ShapeEditorItem(ShapeElementPtr(clone));
    }
    }
    return nullptr;
}

// 重建Item以应用新的矩形
// 由于BaseEditorItem::boundingRect()依赖m_element->rect()，
// 且m_element为protected成员无法外部修改，
// 尺寸变更必须通过"移除旧Item→创建新Item"实现。
// 保留选中状态以便undo/redo后用户选择不丢失。
static void rebuildItemWithRect(EditorScene* scene, const QString& elementId,
                                const QRectF& newRect)
{
    BaseEditorItem* oldItem = findItemByElementId(scene, elementId);
    if (!oldItem) return;

    bool wasSelected = oldItem->isSelected();

    // clone元素数据并设置新rect（clone保留id及其他属性）
    PageElementData* clone = oldItem->elementData().constData()->clone();
    clone->setRect(newRect);
    PageElementPtr newElem(clone);

    // 移除并销毁旧Item
    scene->removeItem(oldItem);
    delete oldItem;

    // 创建新Item并添加到场景
    BaseEditorItem* newItem = createItemForElement(newElem);
    if (newItem) {
        scene->addItem(newItem);
        newItem->setSelected(wasSelected);
    }
}

// 重建文本Item以应用新的文本内容
// 文本内容影响渲染布局，需重建Item。
static void rebuildItemWithText(EditorScene* scene, const QString& elementId,
                                const QString& newText)
{
    BaseEditorItem* oldItem = findItemByElementId(scene, elementId);
    if (!oldItem) return;

    // 仅文本元素支持文本编辑
    if (oldItem->elementData().constData()->elementType() != PageElementData::Text) return;

    bool wasSelected = oldItem->isSelected();

    // clone文本元素数据并设置新文本
    TextElementData* clone = static_cast<TextElementData*>(
        oldItem->elementData().constData()->clone());
    clone->setText(newText);
    PageElementPtr newElem(clone);

    // 移除并销毁旧Item
    scene->removeItem(oldItem);
    delete oldItem;

    // 创建新Item并添加到场景
    BaseEditorItem* newItem = createItemForElement(newElem);
    if (newItem) {
        scene->addItem(newItem);
        newItem->setSelected(wasSelected);
    }
}

// 应用位置到多个元素（用于MoveCommand）
// 直接调用QGraphicsItem::setPos()修改Item位置，再syncToData()写回数据模型
static void applyPositions(EditorScene* scene, const QMap<QString, QPointF>& positions)
{
    for (auto it = positions.constBegin(); it != positions.constEnd(); ++it) {
        BaseEditorItem* item = findItemByElementId(scene, it.key());
        if (item) {
            item->setPos(it.value());
            item->syncToData();
        }
    }
}

// 应用旋转角度到单个元素（用于RotateCommand）
// 旋转不影响本地boundingRect，无需重建Item
static void applyRotation(EditorScene* scene, const QString& elementId, qreal rotation)
{
    BaseEditorItem* item = findItemByElementId(scene, elementId);
    if (item) {
        item->setRotation(rotation);
        item->syncToData();
    }
}

// 应用Z值到单个元素（用于ZOrderCommand）
// Z值变更不影响boundingRect，无需重建Item
static void applyZOrder(EditorScene* scene, const QString& elementId, int z)
{
    BaseEditorItem* item = findItemByElementId(scene, elementId);
    if (item) {
        item->setZValue(z);
        item->syncToData();
    }
}

// ============================================================
// MoveCommand - 移动命令实现
// ============================================================

MoveCommand::MoveCommand(EditorScene* scene, const QList<BaseEditorItem*>& items,
                         const QMap<QString, QPointF>& oldPositions,
                         const QMap<QString, QPointF>& newPositions,
                         QUndoCommand* parent)
    : QUndoCommand(parent)
    , m_scene(scene)
    , m_oldPositions(oldPositions)
    , m_newPositions(newPositions)
{
    // 根据元素数量设置命令描述文本（用于菜单显示）
    int count = items.size();
    if (count > 1) {
        setText(QStringLiteral("移动 %1 个元素").arg(count));
    } else {
        setText(QStringLiteral("移动元素"));
    }
}

void MoveCommand::undo()
{
    if (!m_scene) return;
    applyPositions(m_scene, m_oldPositions);
    m_scene->update();
}

void MoveCommand::redo()
{
    if (!m_scene) return;
    applyPositions(m_scene, m_newPositions);
    m_scene->update();
}

// ============================================================
// ResizeCommand - 缩放命令实现
// ============================================================

ResizeCommand::ResizeCommand(EditorScene* scene, const QString& elementId,
                             const QRectF& oldRect, const QRectF& newRect,
                             QUndoCommand* parent)
    : QUndoCommand(parent)
    , m_scene(scene)
    , m_elementId(elementId)
    , m_oldRect(oldRect)
    , m_newRect(newRect)
{
    setText(QStringLiteral("缩放元素"));
}

void ResizeCommand::undo()
{
    if (!m_scene) return;
    rebuildItemWithRect(m_scene, m_elementId, m_oldRect);
    m_scene->update();
}

void ResizeCommand::redo()
{
    if (!m_scene) return;
    rebuildItemWithRect(m_scene, m_elementId, m_newRect);
    m_scene->update();
}

// ============================================================
// RotateCommand - 旋转命令实现
// ============================================================

RotateCommand::RotateCommand(EditorScene* scene, const QString& elementId,
                             qreal oldRotation, qreal newRotation,
                             QUndoCommand* parent)
    : QUndoCommand(parent)
    , m_scene(scene)
    , m_elementId(elementId)
    , m_oldRotation(oldRotation)
    , m_newRotation(newRotation)
{
    setText(QStringLiteral("旋转元素"));
}

void RotateCommand::undo()
{
    if (!m_scene) return;
    applyRotation(m_scene, m_elementId, m_oldRotation);
    m_scene->update();
}

void RotateCommand::redo()
{
    if (!m_scene) return;
    applyRotation(m_scene, m_elementId, m_newRotation);
    m_scene->update();
}

// ============================================================
// DeleteCommand - 删除命令实现
// ============================================================

DeleteCommand::DeleteCommand(EditorScene* scene, const QList<PageElementPtr>& elements,
                             QUndoCommand* parent)
    : QUndoCommand(parent)
    , m_scene(scene)
    , m_elements(elements)
{
    // 根据元素数量设置命令描述文本
    int count = m_elements.size();
    if (count > 1) {
        setText(QStringLiteral("删除 %1 个元素").arg(count));
    } else {
        setText(QStringLiteral("删除元素"));
    }
}

void DeleteCommand::undo()
{
    if (!m_scene) return;
    // 从存储的元素数据重建Item并添加到场景
    // clone()保留元素id，重建的Item与原Item具有相同id
    for (const PageElementPtr& elem : m_elements) {
        if (!elem) continue;
        BaseEditorItem* item = createItemForElement(elem);
        if (item) {
            m_scene->addItem(item);
        }
    }
    m_scene->update();
}

void DeleteCommand::redo()
{
    if (!m_scene) return;
    // 查找并移除每个元素对应的Item
    // 首次redo时Item已被交互层删除，findItemByElementId返回nullptr，为幂等无操作
    for (const PageElementPtr& elem : m_elements) {
        if (!elem) continue;
        BaseEditorItem* item = findItemByElementId(m_scene, elem->id());
        if (item) {
            m_scene->removeItem(item);
            delete item;
        }
    }
    m_scene->update();
}

// ============================================================
// AddCommand - 添加命令实现
// ============================================================

AddCommand::AddCommand(EditorScene* scene, const PageElementPtr& element,
                       QUndoCommand* parent)
    : QUndoCommand(parent)
    , m_scene(scene)
    , m_element(element)
{
    setText(QStringLiteral("添加元素"));
}

void AddCommand::undo()
{
    if (!m_scene || !m_element) return;
    // 通过elementId查找并移除Item
    BaseEditorItem* item = findItemByElementId(m_scene, m_element.constData()->id());
    if (item) {
        m_scene->removeItem(item);
        delete item;
    }
    m_scene->update();
}

void AddCommand::redo()
{
    if (!m_scene || !m_element) return;
    // 幂等性检查：若Item已存在则不重复添加
    // （首次redo创建Item，后续redo在undo删除后重新创建）
    BaseEditorItem* existing = findItemByElementId(m_scene, m_element.constData()->id());
    if (existing) {
        return;
    }
    BaseEditorItem* item = createItemForElement(m_element);
    if (item) {
        m_scene->addItem(item);
        // 选中新添加的元素，提供视觉反馈
        item->setSelected(true);
    }
    m_scene->update();
}

// ============================================================
// TextEditCommand - 文本编辑命令实现
// ============================================================

TextEditCommand::TextEditCommand(EditorScene* scene, const QString& elementId,
                                 const QString& oldText, const QString& newText,
                                 QUndoCommand* parent)
    : QUndoCommand(parent)
    , m_scene(scene)
    , m_elementId(elementId)
    , m_oldText(oldText)
    , m_newText(newText)
{
    setText(QStringLiteral("编辑文本"));
}

void TextEditCommand::undo()
{
    if (!m_scene) return;
    rebuildItemWithText(m_scene, m_elementId, m_oldText);
    m_scene->update();
}

void TextEditCommand::redo()
{
    if (!m_scene) return;
    rebuildItemWithText(m_scene, m_elementId, m_newText);
    m_scene->update();
}

// ============================================================
// ZOrderCommand - 层级调整命令实现
// ============================================================

ZOrderCommand::ZOrderCommand(EditorScene* scene, const QString& elementId,
                             int oldZ, int newZ,
                             QUndoCommand* parent)
    : QUndoCommand(parent)
    , m_scene(scene)
    , m_elementId(elementId)
    , m_oldZ(oldZ)
    , m_newZ(newZ)
{
    setText(QStringLiteral("调整层级"));
}

void ZOrderCommand::undo()
{
    if (!m_scene) return;
    applyZOrder(m_scene, m_elementId, m_oldZ);
    m_scene->update();
}

void ZOrderCommand::redo()
{
    if (!m_scene) return;
    applyZOrder(m_scene, m_elementId, m_newZ);
    m_scene->update();
}
