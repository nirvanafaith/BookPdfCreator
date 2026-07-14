#ifndef COMMANDS_H
#define COMMANDS_H

#include <QUndoCommand>
#include <QMap>
#include <QList>
#include <QString>
#include <QPointF>
#include <QRectF>

#include "PageElement.h"
#include "PageData.h"

class BaseEditorItem;
class EditorScene;

// ============================================================
// 撤销/重做命令模块
//
// 基于Qt Undo Framework（QUndoCommand + QUndoStack）实现。
// 每个命令封装一次用户操作（移动、缩放、旋转、删除、添加、
// 文本编辑、层级调整），提供undo()/redo()实现。
//
// 设计要点：
//   1. 头文件仅包含EditorScene和BaseEditorItem的前向声明，
//      避免循环依赖；.cpp中包含完整头文件。
//   2. 命令通过elementId在场景中查找对应的BaseEditorItem，
//      而非持有Item指针（Item可能在undo/redo过程中被销毁重建）。
//   3. 位置/旋转/层级变更：直接修改Item属性后调用syncToData()。
//   4. 尺寸/文本内容变更：需重建Item（boundingRect依赖元素rect），
//      镜像EditorScene::createItemForElement的创建逻辑。
//   5. 删除/添加：存储PageElementPtr（隐式共享，深拷贝安全），
//      undo/redo时从元素数据重建Item。
//
// 幂等性说明：
//   QUndoStack::push()首次调用redo()。对于"先执行操作再push命令"
//   的模式（移动/缩放/旋转/删除），首次redo为幂等无操作；
//   对于"命令即操作"的模式（添加），首次redo执行实际添加。
// ============================================================

// ============================================================
// MoveCommand - 移动命令
//
// 记录多个元素的旧位置和新位置，支持批量移动撤销/重做。
// ============================================================
class MoveCommand : public QUndoCommand
{
public:
    MoveCommand(EditorScene* scene, const QList<BaseEditorItem*>& items,
                const QMap<QString, QPointF>& oldPositions,
                const QMap<QString, QPointF>& newPositions,
                QUndoCommand* parent = nullptr);

    void undo() override;
    void redo() override;

private:
    EditorScene* m_scene;
    QMap<QString, QPointF> m_oldPositions;  // elementId -> 旧位置
    QMap<QString, QPointF> m_newPositions;  // elementId -> 新位置
};

// ============================================================
// ResizeCommand - 缩放命令
//
// 记录单个元素的旧矩形和新矩形。由于boundingRect依赖元素rect，
// undo/redo时需重建Item。
// ============================================================
class ResizeCommand : public QUndoCommand
{
public:
    ResizeCommand(EditorScene* scene, const QString& elementId,
                  const QRectF& oldRect, const QRectF& newRect,
                  QUndoCommand* parent = nullptr);

    void undo() override;
    void redo() override;

private:
    EditorScene* m_scene;
    QString m_elementId;
    QRectF m_oldRect;
    QRectF m_newRect;
};

// ============================================================
// RotateCommand - 旋转命令
//
// 记录单个元素的旧旋转角度和新旋转角度。旋转不影响本地包围矩形，
// 无需重建Item。
// ============================================================
class RotateCommand : public QUndoCommand
{
public:
    RotateCommand(EditorScene* scene, const QString& elementId,
                  qreal oldRotation, qreal newRotation,
                  QUndoCommand* parent = nullptr);

    void undo() override;
    void redo() override;

private:
    EditorScene* m_scene;
    QString m_elementId;
    qreal m_oldRotation;
    qreal m_newRotation;
};

// ============================================================
// DeleteCommand - 删除命令
//
// 存储被删除元素的深拷贝（PageElementPtr），undo时从数据重建Item。
// 首次redo为幂等操作（元素已被交互层删除）。
// ============================================================
class DeleteCommand : public QUndoCommand
{
public:
    DeleteCommand(EditorScene* scene, const QList<PageElementPtr>& elements,
                  QUndoCommand* parent = nullptr);

    void undo() override;
    void redo() override;

private:
    EditorScene* m_scene;
    QList<PageElementPtr> m_elements;  // 被删除的元素（用于恢复）
};

// ============================================================
// AddCommand - 添加命令
//
// 存储待添加的元素数据，redo时创建Item并添加到场景，undo时移除。
// 采用"命令即操作"模式：首次redo执行实际添加。
// ============================================================
class AddCommand : public QUndoCommand
{
public:
    AddCommand(EditorScene* scene, const PageElementPtr& element,
               QUndoCommand* parent = nullptr);

    void undo() override;
    void redo() override;

private:
    EditorScene* m_scene;
    PageElementPtr m_element;
};

// ============================================================
// TextEditCommand - 文本编辑命令
//
// 记录文本元素的新旧文本内容。由于文本内容影响渲染，
// undo/redo时需重建Item。
// ============================================================
class TextEditCommand : public QUndoCommand
{
public:
    TextEditCommand(EditorScene* scene, const QString& elementId,
                    const QString& oldText, const QString& newText,
                    QUndoCommand* parent = nullptr);

    void undo() override;
    void redo() override;

private:
    EditorScene* m_scene;
    QString m_elementId;
    QString m_oldText;
    QString m_newText;
};

// ============================================================
// ZOrderCommand - 层级调整命令
//
// 记录单个元素的新旧Z值。Z值变更不影响包围矩形，无需重建Item。
// ============================================================
class ZOrderCommand : public QUndoCommand
{
public:
    ZOrderCommand(EditorScene* scene, const QString& elementId,
                  int oldZ, int newZ,
                  QUndoCommand* parent = nullptr);

    void undo() override;
    void redo() override;

private:
    EditorScene* m_scene;
    QString m_elementId;
    int m_oldZ;
    int m_newZ;
};

// ============================================================
// NudgeCommand - 方向键微移命令
//
// 记录多个元素通过方向键微移的旧位置和新位置，支持批量撤销/重做。
// 位置变更不影响包围矩形，无需重建Item。
// ============================================================
class NudgeCommand : public QUndoCommand
{
public:
    NudgeCommand(EditorScene* scene,
                 const QMap<QString, QPointF>& oldPositions,
                 const QMap<QString, QPointF>& newPositions,
                 QUndoCommand* parent = nullptr);

    void undo() override;
    void redo() override;

private:
    EditorScene* m_scene;
    QMap<QString, QPointF> m_oldPositions;  // elementId -> 旧位置
    QMap<QString, QPointF> m_newPositions;  // elementId -> 新位置
};

#endif // COMMANDS_H
