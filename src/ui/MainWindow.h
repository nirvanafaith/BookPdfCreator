#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSharedPointer>
#include <QScopedPointer>
#include <QUndoStack>
#include <functional>

#include "models/Book.h"
#include "models/LayoutMode.h"
#include "layout/LayoutEngine.h"
#include "editor/EditorView.h"
#include "editor/LayerPanel.h"
#include "editor/PropertyPanel.h"
#include "editor/TextFormatToolbar.h"
#include "editor/PageData.h"
#include "editor/PageElement.h"
#include "ui/AssetTreeWidget.h"

class QAction;
class QMenu;
class QToolBar;
class QSplitter;
class QComboBox;
class QLabel;
class QSpinBox;
class QDockWidget;
class PdfExporter;
class ZipReader;
class QProgressDialog;
class EditorScene;
class BaseEditorItem;
struct PaperSize;   // 纸张尺寸预设（定义于 LayoutConstants.h）

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    // 文件菜单槽函数
    void onOpenFile();
    void onExportPdf();
    void onSaveProject();   // 保存项目
    void onLoadProject();   // 加载项目
    void onOpenRecentProject(); // 打开最近项目
    void onExportLayout();  // 导出版式
    void onImportLayout();  // 导入版式
    void onExit();

    // 帮助菜单槽函数
    void onAbout();

    // 排版模式切换
    void onLayoutModeChanged(int index);

    // 页码导航
    void onPrevPage();
    void onNextPage();

    // 缩放控制
    void onZoomChanged(int index);
    void onZoomFit();

    // 书籍列表勾选变化
    void onBookItemChanged(int index, bool checked);

    // 预览页面变化
    void onPreviewPageChanged(int page);

    // 页码跳转
    void onPageJump(int page);

    // PDF导出进度
    void onExportProgress(int currentPage, int totalPages);

    // ---- 编辑器相关槽 ----
    void onSelectionChanged(QList<PageElementPtr> elements);
    void onPageModified();
    void onElementMoved(const QString& elementId, const QPointF& pos);
    void onElementResized(const QString& elementId, const QSizeF& size);
    void onElementRotated(const QString& elementId, qreal rotation);
    void onElementVisibilityChanged(const QString& elementId, bool visible);
    void onElementLockChanged(const QString& elementId, bool locked);
    void onElementZOrderChanged(const QString& elementId, int newZ);
    void onElementZOrderBatchChanged(const QList<QPair<QString, int>>& changes);
    void onElementDeleted(const QString& elementId);
    void onElementDuplicated(const QString& elementId);
    void onApplyToAllPages();   // 应用到所有同类页
    void onUndo();
    void onRedo();

private:
    // UI初始化函数
    void createActions();
    void createMenus();
    void createToolBars();
    void createStatusBar();
    void createCentralWidget();
    void createEditorPanels();      // 创建编辑器面板和停靠窗口
    void createEditToolBar();       // 创建编辑工具栏
    void connectSignals();          // 连接编辑器信号

    // 核心功能辅助函数
    void updateBookList();
    void updateSelectedBooks();
    void updateLayoutEngine();
    void updateStatusBar();
    void recalculateLayout();

    // 编辑器辅助函数
    void saveCurrentPageData();     // 保存当前页编辑数据到m_editedPages
    void loadPageData(int page);    // 加载指定页的编辑数据到场景
    void refreshLayerPanel();       // 刷新图层面板显示
    BaseEditorItem* findItemById(const QString& id) const;  // 按ID查找场景中的编辑器Item

    // 纸张尺寸切换
    void changePaperSize(const PaperSize& paper);   // 切换纸张尺寸并刷新场景/视图
    void onCustomPaper();                           // 弹出自定义纸张对话框
    // 重建Item以应用元素数据修改（字体/颜色/锁定等）
    // modifier对clone后的元素数据进行修改，然后根据类型创建新Item替换旧Item
    BaseEditorItem* rebuildItemWithModifiedElement(const QString& elementId,
        std::function<void(PageElementData*)> modifier);

    // 从目录加载书籍
    bool loadFromDirectory(const QString& dirPath, QString* errorMessage);
    // 从ZIP加载书籍
    bool loadFromZip(const QString& zipPath, QString* errorMessage);
    // 扫描TXT文件并关联到书籍（按ISBN/书名匹配，未匹配的分配到所有书）
    void scanTxtFiles(const QList<BookPtr>& books, const QString& basePath);

    // 最近项目相关
    void loadRecentProjects();              // 从QSettings加载最近项目列表
    void saveRecentProjects();              // 保存最近项目列表到QSettings
    void updateRecentProjectsMenu();        // 更新最近项目菜单显示
    void addToRecentProjects(const QString& path);  // 添加路径到最近项目列表

    // 菜单
    QMenu* m_fileMenu;
    QMenu* m_editMenu;                  // 编辑菜单（撤销/重做/复制/删除）
    QMenu* m_helpMenu;
    QMenu* m_recentProjectsMenu;        // 最近项目子菜单
    QList<QAction*> m_recentProjectActions; // 最近项目动作列表（最多5个）
    QStringList m_recentProjects;       // 最近项目路径列表（最多5个）

    // 工具栏
    QToolBar* m_mainToolBar;
    QToolBar* m_editToolBar;

    // 动作
    QAction* m_openAction;
    QAction* m_exportPdfAction;
    QAction* m_saveProjectAction;
    QAction* m_loadProjectAction;
    QAction* m_exitAction;
    QAction* m_aboutAction;
    QAction* m_prevPageAction;
    QAction* m_nextPageAction;
    QAction* m_zoomFitAction;
    QAction* m_undoAction;
    QAction* m_redoAction;
    QAction* m_snapToggleAction;

    // UI组件
    QSplitter* m_splitter;
    AssetTreeWidget* m_assetTree;   // 素材树组件（替代原QListWidget书列表）
    EditorView* m_editorView;       // 编辑器视图（替代PdfPreviewWidget）
    QComboBox* m_layoutCombo;
    QComboBox* m_zoomCombo;
    QLabel* m_statusBooksLabel;
    QLabel* m_statusPagesLabel;
    QLabel* m_statusCurrentLabel;
    QSpinBox* m_pageSpinBox;

    // 编辑器组件
    LayerPanel* m_layerPanel;
    PropertyPanel* m_propertyPanel;
    TextFormatToolbar* m_textFormatToolbar;
    QUndoStack* m_undoStack;

    // 停靠面板
    QDockWidget* m_layerDock;
    QDockWidget* m_propertyDock;

    // 编辑数据
    QList<PageDataPtr> m_editedPages;   // 所有页的编辑数据
    int m_currentPage;                  // 当前页码（从0开始）

    // 核心数据
    BookList m_allBooks;
    BookList m_selectedBooks;
    LayoutEnginePtr m_layoutEngine;
    PdfExporter* m_exporter;
    QScopedPointer<ZipReader> m_zipReader; // 持有ZIP读取器以保持临时目录生命周期

    // 基础路径（用于图片查找，目录模式为目录路径，ZIP模式为临时解压目录）
    QString m_basePath;
    QString m_sourcePath;   // 原始数据源路径（用户打开的目录或ZIP文件，用于项目保存/恢复）
    QString m_openDir; // 上次打开的目录
};

#endif // MAINWINDOW_H
