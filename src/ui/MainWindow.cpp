#include "MainWindow.h"
#include "editor/EditorScene.h"
#include "editor/BaseEditorItem.h"
#include "editor/TextEditorItem.h"
#include "editor/ImageEditorItem.h"
#include "editor/ShapeEditorItem.h"
#include "editor/Commands.h"
#include "pdf/PdfExporter.h"
#include "layout/SingleColumnLayout.h"
#include "layout/MultiColumnLayout.h"
#include "layout/AutoLayoutDetector.h"
#include "layout/LayoutConstants.h"
#include "ui/PaperSizeDialog.h"
#include "ui/LayoutLibraryWidget.h"
#include "ui/FixedAssetWidget.h"
#include "parsers/ExcelParser.h"
#include "parsers/ImageFinder.h"
#include "parsers/ImageCache.h"
#include "utils/ZipReader.h"

#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QSplitter>
#include <QTabWidget>
#include <QHBoxLayout>
#include <QComboBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QProgressDialog>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QToolButton>
#include <QApplication>
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QtConcurrent>
#include <QFutureWatcher>
#include <QDockWidget>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>
#include <QFile>
#include <QDateTime>
#include <QUuid>
#include <QGraphicsItem>
#include <QMenu>
#include <QSettings>
#include <QKeyEvent>
#include <QSignalBlocker>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      m_fileMenu(nullptr),
      m_editMenu(nullptr),
      m_helpMenu(nullptr),
      m_recentProjectsMenu(nullptr),
      m_mainToolBar(nullptr),
      m_editToolBar(nullptr),
      m_openAction(nullptr),
      m_exportPdfAction(nullptr),
      m_saveProjectAction(nullptr),
      m_loadProjectAction(nullptr),
      m_exitAction(nullptr),
      m_aboutAction(nullptr),
      m_prevPageAction(nullptr),
      m_nextPageAction(nullptr),
      m_zoomFitAction(nullptr),
      m_undoAction(nullptr),
      m_redoAction(nullptr),
      m_snapToggleAction(nullptr),
      m_splitter(nullptr),
      m_assetTree(nullptr),
      m_fixedAssetWidget(nullptr),
      m_layoutLibrary(nullptr),
      m_editorView(nullptr),
      m_layoutCombo(nullptr),
      m_zoomCombo(nullptr),
      m_statusBooksLabel(nullptr),
      m_statusPagesLabel(nullptr),
      m_statusCurrentLabel(nullptr),
      m_pageSpinBox(nullptr),
      m_layerPanel(nullptr),
      m_propertyPanel(nullptr),
      m_textFormatToolbar(nullptr),
      m_undoStack(nullptr),
      m_layerDock(nullptr),
      m_propertyDock(nullptr),
      m_currentPage(0),
      m_exporter(nullptr)
{
    setWindowTitle(tr("书目PDF生成器"));
    resize(1280, 800);

    createActions();
    createMenus();
    createToolBars();
    createStatusBar();
    createCentralWidget();      // 创建中央控件（书列表+编辑器视图）
    createEditorPanels();       // 创建编辑器面板和停靠窗口
    createEditToolBar();        // 创建编辑工具栏
    connectSignals();           // 连接编辑器信号

    m_exporter = new PdfExporter(this);
    connect(m_exporter, &PdfExporter::exportProgress, this, &MainWindow::onExportProgress);

    // 加载最近项目列表并更新菜单
    loadRecentProjects();
    updateRecentProjectsMenu();

    updateStatusBar();
    statusBar()->showMessage(tr("就绪"), 3000);
}

MainWindow::~MainWindow()
{
}

void MainWindow::createActions()
{
    m_openAction = new QAction(tr("打开(&O)..."), this);
    m_openAction->setShortcut(QKeySequence::Open);
    m_openAction->setStatusTip(tr("打开文件夹或ZIP文件"));
    connect(m_openAction, &QAction::triggered, this, &MainWindow::onOpenFile);

    m_exportPdfAction = new QAction(tr("导出PDF(&E)..."), this);
    m_exportPdfAction->setStatusTip(tr("导出PDF文件"));
    connect(m_exportPdfAction, &QAction::triggered, this, &MainWindow::onExportPdf);

    m_saveProjectAction = new QAction(tr("保存项目(&S)..."), this);
    m_saveProjectAction->setShortcut(QKeySequence::Save);
    m_saveProjectAction->setStatusTip(tr("保存编辑项目到JSON文件"));
    connect(m_saveProjectAction, &QAction::triggered, this, &MainWindow::onSaveProject);

    m_loadProjectAction = new QAction(tr("加载项目(&L)..."), this);
    m_loadProjectAction->setShortcut(QKeySequence(tr("Ctrl+Shift+O")));
    m_loadProjectAction->setStatusTip(tr("从JSON文件加载编辑项目"));
    connect(m_loadProjectAction, &QAction::triggered, this, &MainWindow::onLoadProject);

    m_exitAction = new QAction(tr("退出(&X)"), this);
    m_exitAction->setShortcut(QKeySequence::Quit);
    m_exitAction->setStatusTip(tr("退出应用程序"));
    connect(m_exitAction, &QAction::triggered, this, &MainWindow::onExit);

    m_aboutAction = new QAction(tr("关于(&A)..."), this);
    m_aboutAction->setStatusTip(tr("显示应用程序信息"));
    connect(m_aboutAction, &QAction::triggered, this, &MainWindow::onAbout);

    m_prevPageAction = new QAction(tr("上一页"), this);
    m_prevPageAction->setStatusTip(tr("显示上一页"));
    connect(m_prevPageAction, &QAction::triggered, this, &MainWindow::onPrevPage);

    m_nextPageAction = new QAction(tr("下一页"), this);
    m_nextPageAction->setStatusTip(tr("显示下一页"));
    connect(m_nextPageAction, &QAction::triggered, this, &MainWindow::onNextPage);

    m_zoomFitAction = new QAction(tr("适合窗口"), this);
    m_zoomFitAction->setStatusTip(tr("缩放到适合窗口"));
    connect(m_zoomFitAction, &QAction::triggered, this, &MainWindow::onZoomFit);
}

void MainWindow::createMenus()
{
    m_fileMenu = menuBar()->addMenu(tr("文件(&F)"));
    m_fileMenu->addAction(m_openAction);
    m_fileMenu->addAction(m_loadProjectAction);
    // 最近项目子菜单
    m_recentProjectsMenu = m_fileMenu->addMenu(tr("最近打开(&R)"));
    m_fileMenu->addSeparator();
    m_fileMenu->addAction(m_saveProjectAction);
    m_fileMenu->addAction(m_exportPdfAction);
    m_fileMenu->addSeparator();
    m_fileMenu->addAction(tr("导出版式(&E)..."), this, &MainWindow::onExportLayout, QKeySequence(tr("Ctrl+Shift+E")));
    m_fileMenu->addAction(tr("导入版式(&I)..."), this, &MainWindow::onImportLayout, QKeySequence(tr("Ctrl+Shift+I")));
    m_fileMenu->addSeparator();
    m_fileMenu->addAction(m_exitAction);

    // 编辑菜单：撤销/重做/复制/删除
    // 撤销/重做动作在createEditToolBar()中创建（依赖撤销栈），
    // 此处先创建菜单框架，撤销/重做动作稍后插入
    m_editMenu = menuBar()->addMenu(tr("编辑(&E)"));

    // 占位分隔符，撤销/重做动作将插入到此分隔符之前
    m_editMenu->addSeparator();

    // 复制（Ctrl+D）：向编辑器视图投递按键事件，触发场景的复制逻辑
    QAction* copyAction = new QAction(tr("复制(&D)"), this);
    copyAction->setShortcut(QKeySequence(tr("Ctrl+D")));
    copyAction->setStatusTip(tr("复制选中元素"));
    connect(copyAction, &QAction::triggered, this, [this]() {
        if (m_editorView) {
            QKeyEvent* keyEvent = new QKeyEvent(QEvent::KeyPress, Qt::Key_D, Qt::ControlModifier);
            QApplication::postEvent(m_editorView, keyEvent);
        }
    });
    m_editMenu->addAction(copyAction);

    // 删除（Delete）：向编辑器视图投递按键事件
    // 注意：不设置 QKeySequence::Delete 快捷键，否则 Delete 键会在应用层被拦截，
    // 导致原始按键事件无法到达 EditorScene。改由 EditorView 的 keyPressEvent
    // 自然转发给场景处理；菜单点击时仍通过合成事件作为兜底。
    QAction* deleteAction = new QAction(tr("删除(&Del)"), this);
    deleteAction->setStatusTip(tr("删除选中元素"));
    connect(deleteAction, &QAction::triggered, this, [this]() {
        if (m_editorView) {
            QKeyEvent* keyEvent = new QKeyEvent(QEvent::KeyPress, Qt::Key_Delete, Qt::NoModifier);
            QApplication::postEvent(m_editorView, keyEvent);
        }
    });
    m_editMenu->addAction(deleteAction);

    m_helpMenu = menuBar()->addMenu(tr("帮助(&H)"));
    m_helpMenu->addAction(m_aboutAction);
}

void MainWindow::createToolBars()
{
    m_mainToolBar = addToolBar(tr("主工具栏"));
    m_mainToolBar->setObjectName("MainToolBar");
    m_mainToolBar->addAction(m_openAction);
    m_mainToolBar->addSeparator();

    QLabel* layoutLabel = new QLabel(tr("排版模式:"), this);
    m_mainToolBar->addWidget(layoutLabel);

    m_layoutCombo = new QComboBox(this);
    m_layoutCombo->addItem(tr("自动"), static_cast<int>(LayoutMode::Auto));
    m_layoutCombo->addItem(tr("单图排版"), static_cast<int>(LayoutMode::SingleColumn));
    m_layoutCombo->addItem(tr("多图排版"), static_cast<int>(LayoutMode::MultiColumn));
    m_layoutCombo->setStatusTip(tr("选择排版模式"));
    connect(m_layoutCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onLayoutModeChanged);
    m_mainToolBar->addWidget(m_layoutCombo);
    m_mainToolBar->addSeparator();

    m_mainToolBar->addAction(m_prevPageAction);
    m_mainToolBar->addAction(m_nextPageAction);
    m_mainToolBar->addSeparator();

    QLabel* zoomLabel = new QLabel(tr("缩放:"), this);
    m_mainToolBar->addWidget(zoomLabel);

    m_zoomCombo = new QComboBox(this);
    m_zoomCombo->setEditable(true);
    QStringList zoomLevels;
    zoomLevels << "50%" << "75%" << "100%" << "125%" << "150%" << "200%" << "300%";
    m_zoomCombo->addItems(zoomLevels);
    m_zoomCombo->setCurrentText("100%");
    m_zoomCombo->setStatusTip(tr("设置缩放比例"));
    connect(m_zoomCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onZoomChanged);
    m_mainToolBar->addWidget(m_zoomCombo);
    m_mainToolBar->addAction(m_zoomFitAction);
    m_mainToolBar->addSeparator();

    m_mainToolBar->addAction(m_exportPdfAction);
}

void MainWindow::createStatusBar()
{
    m_statusBooksLabel = new QLabel(tr("总书籍: 0 本"), this);
    m_statusPagesLabel = new QLabel(tr("总页数: 0"), this);
    m_statusCurrentLabel = new QLabel(tr("当前: 第0页"), this);

    statusBar()->addWidget(m_statusBooksLabel);
    statusBar()->addPermanentWidget(m_statusPagesLabel);
    statusBar()->addPermanentWidget(m_statusCurrentLabel);

    // 页码跳转SpinBox
    m_pageSpinBox = new QSpinBox(this);
    m_pageSpinBox->setMinimum(1);
    m_pageSpinBox->setMaximum(1);
    m_pageSpinBox->setFixedWidth(80);
    m_pageSpinBox->setStatusTip(tr("跳转到指定页"));
    connect(m_pageSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &MainWindow::onPageJump);
    statusBar()->addPermanentWidget(new QLabel(tr("跳转:")));
    statusBar()->addPermanentWidget(m_pageSpinBox);
}

void MainWindow::createCentralWidget()
{
    m_splitter = new QSplitter(Qt::Horizontal, this);

    // 创建左侧标签页容器
    m_leftTabWidget = new QTabWidget(this);
    m_leftTabWidget->setMinimumWidth(200);
    m_leftTabWidget->setMaximumWidth(350);

    // 可变素材标签页 (原"素材")
    m_assetTree = new AssetTreeWidget(this);
    connect(m_assetTree, &AssetTreeWidget::bookCheckChanged,
            this, &MainWindow::onBookItemChanged);
    m_leftTabWidget->addTab(m_assetTree, tr("可变素材"));

    // 固定素材标签页
    m_fixedAssetWidget = new FixedAssetWidget(this);
    m_leftTabWidget->addTab(m_fixedAssetWidget, tr("固定素材"));

    // 版式标签页
    m_layoutLibrary = new LayoutLibraryWidget(this);
    connect(m_layoutLibrary, &LayoutLibraryWidget::layoutSelected,
            this, &MainWindow::onLayoutSelected);
    m_leftTabWidget->addTab(m_layoutLibrary, tr("版式"));

    // 工具面板（迁移到顶部编辑工具栏，此处仅创建实例作为状态管理器）
    m_toolsPanel = new ToolsPanel(this);

    // 切换到可变素材(0)或固定素材(1)时重置为选择工具
    connect(m_leftTabWidget, &QTabWidget::currentChanged, this, [this](int index) {
        // 切换到可变素材(0)或固定素材(1)时重置为选择工具
        if (index == 0 || index == 1) {
            if (m_editorView && m_editorView->editorScene()) {
                m_editorView->editorScene()->setCurrentTool(ToolSelect);
            }
            if (m_toolsPanel) {
                m_toolsPanel->selectTool(ToolSelect);
            }
        }
    });

    // 创建编辑器视图
    m_editorView = new EditorView(this);
    connect(m_editorView, &EditorView::currentPageChanged, this, &MainWindow::onPreviewPageChanged);

    m_splitter->addWidget(m_leftTabWidget);
    m_splitter->addWidget(m_editorView);
    m_splitter->setStretchFactor(0, 0);
    m_splitter->setStretchFactor(1, 1);
    m_splitter->setSizes(QList<int>() << 220 << 1060);

    setCentralWidget(m_splitter);
}

void MainWindow::createEditorPanels()
{
    // 图层面板停靠窗口
    m_layerPanel = new LayerPanel(this);
    m_layerDock = new QDockWidget(tr("图层"), this);
    m_layerDock->setObjectName("LayerDock");
    m_layerDock->setWidget(m_layerPanel);
    m_layerDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    addDockWidget(Qt::RightDockWidgetArea, m_layerDock);

    // 属性面板停靠窗口
    m_propertyPanel = new PropertyPanel(this);
    m_propertyDock = new QDockWidget(tr("属性"), this);
    m_propertyDock->setObjectName("PropertyDock");
    m_propertyDock->setWidget(m_propertyPanel);
    m_propertyDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    addDockWidget(Qt::RightDockWidgetArea, m_propertyDock);

    // 文本格式化工具栏（浮动，初始隐藏）
    m_textFormatToolbar = new TextFormatToolbar(this);
    m_textFormatToolbar->hide();

    // 撤销栈：使用编辑场景自带的撤销栈
    if (m_editorView && m_editorView->editorScene()) {
        m_undoStack = m_editorView->editorScene()->undoStack();
        if (m_undoStack) {
            m_undoStack->setUndoLimit(50);
        }
    }
}

void MainWindow::createEditToolBar()
{
    m_editToolBar = addToolBar(tr("编辑"));
    m_editToolBar->setObjectName("EditToolBar");

    // 撤销/重做动作（从撤销栈创建，自动处理启用状态和文本）
    if (m_undoStack) {
        m_undoAction = m_undoStack->createUndoAction(this, tr("撤销"));
        m_undoAction->setShortcut(QKeySequence::Undo);
        m_editToolBar->addAction(m_undoAction);

        m_redoAction = m_undoStack->createRedoAction(this, tr("重做"));
        m_redoAction->setShortcut(QKeySequence::Redo);
        m_editToolBar->addAction(m_redoAction);

        m_editToolBar->addSeparator();

        // 将撤销/重做动作插入编辑菜单最前面（占位分隔符之前）
        if (m_editMenu) {
            QList<QAction*> actions = m_editMenu->actions();
            if (!actions.isEmpty()) {
                m_editMenu->insertAction(actions.first(), m_undoAction);
                m_editMenu->insertAction(actions.first(), m_redoAction);
                m_editMenu->insertSeparator(actions.first());
            } else {
                m_editMenu->addAction(m_undoAction);
                m_editMenu->addAction(m_redoAction);
                m_editMenu->addSeparator();
            }
        }
    }

    // 吸附开关
    m_snapToggleAction = new QAction(tr("对齐吸附"), this);
    m_snapToggleAction->setCheckable(true);
    m_snapToggleAction->setChecked(true);
    m_snapToggleAction->setStatusTip(tr("启用/禁用网格吸附"));
    m_editToolBar->addAction(m_snapToggleAction);

    // 应用到所有同类页
    QAction* applyAllAction = new QAction(tr("应用到所有同类页"), this);
    applyAllAction->setStatusTip(tr("将当前页元素应用到所有同类页"));
    connect(applyAllAction, &QAction::triggered, this, &MainWindow::onApplyToAllPages);
    m_editToolBar->addAction(applyAllAction);

    m_editToolBar->addSeparator();

    // ---- 纸张选择按钮 ----
    QToolButton* paperButton = new QToolButton(this);
    paperButton->setText(tr("纸张"));
    paperButton->setToolTip(tr("切换纸张尺寸"));
    paperButton->setPopupMode(QToolButton::InstantPopup);

    QMenu* paperMenu = new QMenu(paperButton);
    QAction* a4Action = paperMenu->addAction(QStringLiteral("A4 (210×297mm)"));
    QAction* a5Action = paperMenu->addAction(QStringLiteral("A5 (148×210mm)"));
    QAction* a3Action = paperMenu->addAction(QStringLiteral("A3 (297×420mm)"));
    paperMenu->addSeparator();
    QAction* customAction = paperMenu->addAction(tr("自定义..."));

    paperButton->setMenu(paperMenu);
    m_editToolBar->addWidget(paperButton);

    // 连接纸张切换信号
    connect(a4Action, &QAction::triggered, this, [this]() { changePaperSize(PAPER_A4); });
    connect(a5Action, &QAction::triggered, this, [this]() { changePaperSize(PAPER_A5); });
    connect(a3Action, &QAction::triggered, this, [this]() { changePaperSize(PAPER_A3); });
    connect(customAction, &QAction::triggered, this, &MainWindow::onCustomPaper);

    // ---- 工具栏第二行：编辑工具 ----
    // 将ToolsPanel从左侧标签页迁移到顶部工具栏
    QToolBar* toolsToolBar = addToolBar(tr("工具"));
    toolsToolBar->setObjectName("ToolsToolBar");
    toolsToolBar->setMovable(false);

    // 调整ToolsPanel为水平布局以适应工具栏（不修改ToolsPanel源文件，仅运行时调整实例）
    if (QLayout* oldLayout = m_toolsPanel->layout()) {
        QList<QWidget*> widgets;
        while (oldLayout->count() > 0) {
            QLayoutItem* item = oldLayout->takeAt(0);
            if (QWidget* w = item->widget()) {
                widgets.append(w);
            }
            delete item;
        }
        delete oldLayout;

        QHBoxLayout* hlayout = new QHBoxLayout();
        hlayout->setContentsMargins(2, 2, 2, 2);
        hlayout->setSpacing(2);
        for (QWidget* w : widgets) {
            hlayout->addWidget(w);
        }
        m_toolsPanel->setLayout(hlayout);
        m_toolsPanel->setMinimumWidth(0);
        m_toolsPanel->setMaximumWidth(QWIDGETSIZE_MAX);
    }

    // 缩小工具按钮尺寸以适应工具栏（原为96x96）
    const QSize toolbarBtnSize(32, 32);
    const QSize toolbarIconSize(24, 24);
    for (QToolButton* btn : m_toolsPanel->findChildren<QToolButton*>()) {
        btn->setFixedSize(toolbarBtnSize);
        btn->setIconSize(toolbarIconSize);
    }

    // 缩小前景色按钮（原为84x84）
    for (QPushButton* btn : m_toolsPanel->findChildren<QPushButton*>()) {
        btn->setFixedSize(28, 28);
    }

    toolsToolBar->addWidget(m_toolsPanel);
}

// ============================================================
// changePaperSize - 切换纸张尺寸
//
// 更新 PageSizeManager 单例的当前纸张，刷新编辑场景的
// 场景矩形与背景重绘，并适配视图窗口，最后在状态栏提示。
// ============================================================
void MainWindow::changePaperSize(const PaperSize& paper)
{
    PageSizeManager::instance().setPaper(paper);

    if (m_editorView && m_editorView->editorScene()) {
        EditorScene* scene = m_editorView->editorScene();
        // 更新页面背景Item（白纸+边距虚线）和场景矩形
        scene->updatePageBackground();
        scene->update();
        // 适配窗口以完整显示新尺寸页面
        m_editorView->fitToWindow();
    }

    statusBar()->showMessage(tr("纸张已切换为 %1 (%2×%3mm)")
        .arg(paper.name).arg(paper.widthMm).arg(paper.heightMm));
}

// ============================================================
// onCustomPaper - 弹出自定义纸张对话框
//
// 用户确认后以输入的宽高构造自定义纸张并切换。
// ============================================================
void MainWindow::onCustomPaper()
{
    PaperSizeDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        PaperSize custom;
        custom.name = QStringLiteral("自定义");
        custom.widthMm = dialog.widthMm();
        custom.heightMm = dialog.heightMm();
        changePaperSize(custom);
    }
}

void MainWindow::connectSignals()
{
    if (!m_editorView || !m_editorView->editorScene()) {
        return;
    }

    EditorScene* scene = m_editorView->editorScene();

    // ---- 编辑场景信号 ----
    // 选中元素变化（重载信号，需用QOverload指定带参版本）
    connect(scene, QOverload<QList<PageElementPtr>>::of(&EditorScene::selectionChanged),
            this, &MainWindow::onSelectionChanged);

    // 页面被修改（移动/缩放/旋转/删除等操作后触发）
    connect(scene, &EditorScene::pageModified, this, &MainWindow::onPageModified);

    // 文本格式化请求（双击编辑文本时显示浮动工具栏）
    connect(scene, &EditorScene::textFormatRequested, this, [this](QPointF scenePos) {
        if (m_textFormatToolbar && m_editorView) {
            // 将场景坐标转换为全局屏幕坐标
            // mapFromScene返回视口坐标，需通过viewport()->mapToGlobal转为全局坐标
            QPoint viewportPos = m_editorView->mapFromScene(scenePos);
            QPoint globalPos = m_editorView->viewport()->mapToGlobal(viewportPos);
            m_textFormatToolbar->showAt(globalPos);
        }
    });

    // ---- 缩放变化信号 ----
    connect(m_editorView, &EditorView::zoomChanged, this, [this](qreal percent) {
        if (m_zoomCombo) {
            m_zoomCombo->blockSignals(true);
            m_zoomCombo->setCurrentText(tr("%1%").arg(qRound(percent)));
            m_zoomCombo->blockSignals(false);
        }
    });

    // ---- 图层面板信号 ----
    connect(m_layerPanel, &LayerPanel::elementSelected, this, [this](const QString& elementId) {
        // 图层点击选中 → 在场景中选中对应元素
        if (!m_editorView || !m_editorView->editorScene()) return;
        BaseEditorItem* item = findItemById(elementId);
        if (item) {
            m_editorView->editorScene()->clearSelection();
            item->setSelected(true);
        }
    });

    connect(m_layerPanel, &LayerPanel::elementVisibilityChanged,
            this, &MainWindow::onElementVisibilityChanged);
    connect(m_layerPanel, &LayerPanel::elementLockChanged,
            this, &MainWindow::onElementLockChanged);
    connect(m_layerPanel, &LayerPanel::elementZOrderChanged,
            this, &MainWindow::onElementZOrderChanged);
    connect(m_layerPanel, &LayerPanel::elementZOrderBatchChanged,
            this, &MainWindow::onElementZOrderBatchChanged);
    connect(m_layerPanel, &LayerPanel::elementDeleted,
            this, &MainWindow::onElementDeleted);
    connect(m_layerPanel, &LayerPanel::elementDuplicated,
            this, &MainWindow::onElementDuplicated);

    // 层级快速调整信号
    connect(m_layerPanel, &LayerPanel::bringToFront, this, [this](const QString& elementId) {
        if (!m_editorView || !m_editorView->editorScene()) return;
        // 查找当前所有BaseEditorItem中的最大z值（排除页面背景z<=-10000）
        int maxZ = -10001;
        const auto items = m_editorView->editorScene()->items();
        for (auto* it : items) {
            if (auto* e = dynamic_cast<BaseEditorItem*>(it)) {
                int z = static_cast<int>(e->zValue());
                if (z > -10000 && z > maxZ) maxZ = z;
            }
        }
        onElementZOrderChanged(elementId, maxZ + 1);
    });
    connect(m_layerPanel, &LayerPanel::sendToBack, this, [this](const QString& elementId) {
        if (!m_editorView || !m_editorView->editorScene()) return;
        // 查找当前所有BaseEditorItem中的最小z值（排除页面背景z<=-10000）
        int minZ = 10000;
        const auto items = m_editorView->editorScene()->items();
        for (auto* it : items) {
            if (auto* e = dynamic_cast<BaseEditorItem*>(it)) {
                int z = static_cast<int>(e->zValue());
                if (z > -10000 && z < minZ) minZ = z;
            }
        }
        onElementZOrderChanged(elementId, minZ - 1);
    });
    connect(m_layerPanel, &LayerPanel::moveUp, this, [this](const QString& elementId) {
        BaseEditorItem* item = findItemById(elementId);
        if (item) {
            onElementZOrderChanged(elementId, item->zValue() + 1);
        }
    });
    connect(m_layerPanel, &LayerPanel::moveDown, this, [this](const QString& elementId) {
        BaseEditorItem* item = findItemById(elementId);
        if (item) {
            onElementZOrderChanged(elementId, qMax(0, static_cast<int>(item->zValue()) - 1));
        }
    });

    // 图层重命名：双击图层条目名称编辑后提交
    connect(m_layerPanel, &LayerPanel::layerRenamed,
            this, [this](const QString& elementId, const QString& newName) {
        updateElementInPlace(elementId, [newName](PageElementData* elem) {
            elem->setName(newName);
        });
    });

    // ---- 属性面板信号 ----
    connect(m_propertyPanel, &PropertyPanel::positionChanged,
            this, &MainWindow::onElementMoved);
    connect(m_propertyPanel, &PropertyPanel::sizeChanged,
            this, &MainWindow::onElementResized);
    connect(m_propertyPanel, &PropertyPanel::rotationChanged,
            this, &MainWindow::onElementRotated);

    // 文本属性变更信号
    // 原地更新元素数据（不删除/重建Item），避免selectionChanged打断PropertyPanel状态
    connect(m_propertyPanel, &PropertyPanel::fontChanged,
            this, [this](const QString& elementId, const QFont& font) {
        updateElementInPlace(elementId, [&font](PageElementData* elem) {
            if (elem->elementType() == PageElementData::Text) {
                static_cast<TextElementData*>(elem)->setFont(font);
            }
        });
    });
    connect(m_propertyPanel, &PropertyPanel::fontSizeChanged,
            this, [this](const QString& elementId, int size) {
        updateElementInPlace(elementId, [size](PageElementData* elem) {
            if (elem->elementType() == PageElementData::Text) {
                QFont f = static_cast<TextElementData*>(elem)->font();
                f.setPointSize(size);
                static_cast<TextElementData*>(elem)->setFont(f);
            }
        });
    });
    connect(m_propertyPanel, &PropertyPanel::textColorChanged,
            this, [this](const QString& elementId, const QColor& color) {
        updateElementInPlace(elementId, [&color](PageElementData* elem) {
            if (elem->elementType() == PageElementData::Text) {
                static_cast<TextElementData*>(elem)->setTextColor(color);
            }
        });
    });
    // 文本内容变更（属性面板直接编辑文本）
    connect(m_propertyPanel, &PropertyPanel::textContentChanged,
            this, [this](const QString& elementId, const QString& newText) {
        if (!m_editorView || !m_editorView->editorScene()) return;
        EditorScene* scene = m_editorView->editorScene();

        // 从场景中查找元素，获取旧文本
        QString oldText;
        bool found = false;
        const QList<QGraphicsItem*> allItems = scene->items();
        for (QGraphicsItem* it : allItems) {
            auto* baseItem = dynamic_cast<BaseEditorItem*>(it);
            if (baseItem && baseItem->elementData().constData()->id() == elementId) {
                if (baseItem->elementData().constData()->elementType() == PageElementData::Text) {
                    const TextElementData* textElem =
                        static_cast<const TextElementData*>(baseItem->elementData().constData());
                    oldText = textElem->text();
                    found = true;
                }
                break;
            }
        }

        if (!found) return;

        // 文本未变化时不提交命令（避免无意义的撤销记录）
        if (oldText == newText) return;

        // 通过TextEditCommand入撤销栈
        // redo()会自动clone元素数据、设置新文本、删除旧Item、创建新Item
        if (scene->undoStack()) {
            scene->undoStack()->push(new TextEditCommand(scene, elementId, oldText, newText));
        }
    });
    connect(m_propertyPanel, &PropertyPanel::alignmentChanged,
            this, [this](const QString& elementId, Qt::Alignment align) {
        updateElementInPlace(elementId, [align](PageElementData* elem) {
            if (elem->elementType() == PageElementData::Text) {
                static_cast<TextElementData*>(elem)->setAlignment(align);
            }
        });
    });

    // 行间距变更
    connect(m_propertyPanel, &PropertyPanel::lineHeightChanged,
            this, [this](const QString& elementId, qreal height) {
        updateElementInPlace(elementId, [height](PageElementData* elem) {
            if (elem->elementType() == PageElementData::Text) {
                static_cast<TextElementData*>(elem)->setLineHeight(height);
            }
        });
    });

    // 字间距变更
    connect(m_propertyPanel, &PropertyPanel::letterSpacingChanged,
            this, [this](const QString& elementId, qreal spacing) {
        updateElementInPlace(elementId, [spacing](PageElementData* elem) {
            if (elem->elementType() == PageElementData::Text) {
                static_cast<TextElementData*>(elem)->setLetterSpacing(spacing);
            }
        });
    });

    // 图片属性变更信号
    connect(m_propertyPanel, &PropertyPanel::opacityChanged,
            this, [this](const QString& elementId, qreal opacity) {
        updateElementInPlace(elementId, [opacity](PageElementData* elem) {
            if (elem->elementType() == PageElementData::Image) {
                static_cast<ImageElementData*>(elem)->setOpacity(opacity);
            }
        });
    });
    connect(m_propertyPanel, &PropertyPanel::scaleFactorChanged,
            this, [this](const QString& elementId, qreal scale) {
        updateElementInPlace(elementId, [scale](PageElementData* elem) {
            if (elem->elementType() == PageElementData::Image) {
                static_cast<ImageElementData*>(elem)->setScaleFactor(scale);
            }
        });
    });

    // ---- 形状属性信号 ----
    connect(m_propertyPanel, &PropertyPanel::fillEnabledChanged,
            this, [this](const QString& elementId, bool enabled) {
        if (!m_editorView || !m_editorView->editorScene()) return;
        const QList<BaseEditorItem*> items = m_editorView->editorScene()->selectedEditorItems();
        for (BaseEditorItem* item : items) {
            if (item->elementData().constData()->id() == elementId) {
                if (auto* shapeItem = qgraphicsitem_cast<ShapeEditorItem*>(item)) {
                    ShapeElementData* clone = static_cast<ShapeElementData*>(
                        shapeItem->elementData().constData()->clone());
                    clone->setHasFill(enabled);
                    shapeItem->setElementData(PageElementPtr(clone));
                    shapeItem->update();
                    emit m_editorView->editorScene()->pageModified();
                }
                break;
            }
        }
    });

    connect(m_propertyPanel, &PropertyPanel::fillColorChanged,
            this, [this](const QString& elementId, const QColor& color) {
        if (!m_editorView || !m_editorView->editorScene()) return;
        const QList<BaseEditorItem*> items = m_editorView->editorScene()->selectedEditorItems();
        for (BaseEditorItem* item : items) {
            if (item->elementData().constData()->id() == elementId) {
                if (auto* shapeItem = qgraphicsitem_cast<ShapeEditorItem*>(item)) {
                    ShapeElementData* clone = static_cast<ShapeElementData*>(
                        shapeItem->elementData().constData()->clone());
                    clone->setFillColor(color);
                    clone->setHasFill(true);
                    shapeItem->setElementData(PageElementPtr(clone));
                    shapeItem->update();
                    emit m_editorView->editorScene()->pageModified();
                }
                break;
            }
        }
    });

    connect(m_propertyPanel, &PropertyPanel::borderColorChanged,
            this, [this](const QString& elementId, const QColor& color) {
        if (!m_editorView || !m_editorView->editorScene()) return;
        const QList<BaseEditorItem*> items = m_editorView->editorScene()->selectedEditorItems();
        for (BaseEditorItem* item : items) {
            if (item->elementData().constData()->id() == elementId) {
                if (auto* shapeItem = qgraphicsitem_cast<ShapeEditorItem*>(item)) {
                    ShapeElementData* clone = static_cast<ShapeElementData*>(
                        shapeItem->elementData().constData()->clone());
                    clone->setBorderColor(color);
                    clone->setHasBorder(true);
                    shapeItem->setElementData(PageElementPtr(clone));
                    shapeItem->update();
                    emit m_editorView->editorScene()->pageModified();
                }
                break;
            }
        }
    });

    connect(m_propertyPanel, &PropertyPanel::borderWidthChanged,
            this, [this](const QString& elementId, qreal width) {
        if (!m_editorView || !m_editorView->editorScene()) return;
        const QList<BaseEditorItem*> items = m_editorView->editorScene()->selectedEditorItems();
        for (BaseEditorItem* item : items) {
            if (item->elementData().constData()->id() == elementId) {
                if (auto* shapeItem = qgraphicsitem_cast<ShapeEditorItem*>(item)) {
                    ShapeElementData* clone = static_cast<ShapeElementData*>(
                        shapeItem->elementData().constData()->clone());
                    clone->setBorderWidth(width);
                    clone->setHasBorder(true);
                    shapeItem->setElementData(PageElementPtr(clone));
                    shapeItem->update();
                    emit m_editorView->editorScene()->pageModified();
                }
                break;
            }
        }
    });

    // ---- 工具面板信号 ----
    connect(m_toolsPanel, &ToolsPanel::toolChanged,
            this, [this](int tool) {
        if (m_editorView && m_editorView->editorScene()) {
            m_editorView->editorScene()->setCurrentTool(tool);
            // 切换到非选择工具时清除选择
            if (tool != ToolSelect) {
                m_editorView->editorScene()->clearSelection();
            }
        }
    });

    connect(m_toolsPanel, &ToolsPanel::foregroundColorChanged,
            this, [this](const QColor& color) {
        if (m_editorView && m_editorView->editorScene()) {
            m_editorView->editorScene()->setForegroundColor(color);
        }
    });

    // 吸管拾取颜色后更新工具面板前景色按钮
    connect(m_editorView->editorScene(), &EditorScene::foregroundColorPicked,
            this, [this](const QColor& color) {
        m_toolsPanel->setForegroundColor(color);
    });

    // 文本工具创建文本元素并进入编辑模式后，切换回选择工具
    connect(m_editorView->editorScene(), &EditorScene::textCreatedAndEditRequested,
            this, [this]() {
        if (m_toolsPanel) {
            m_toolsPanel->selectTool(ToolSelect);
        }
        if (m_editorView && m_editorView->editorScene()) {
            m_editorView->editorScene()->setCurrentTool(ToolSelect);
        }
    });

    // ---- 文本格式化工具栏信号 ----
    connect(m_textFormatToolbar, &TextFormatToolbar::fontChanged,
            this, [this](const QFont& font) {
        Q_UNUSED(font);
        onPageModified();
    });
    connect(m_textFormatToolbar, &TextFormatToolbar::fontSizeChanged,
            this, [this](int size) {
        Q_UNUSED(size);
        onPageModified();
    });
    connect(m_textFormatToolbar, &TextFormatToolbar::boldToggled,
            this, [this](bool bold) {
        Q_UNUSED(bold);
        onPageModified();
    });
    connect(m_textFormatToolbar, &TextFormatToolbar::italicToggled,
            this, [this](bool italic) {
        Q_UNUSED(italic);
        onPageModified();
    });
    connect(m_textFormatToolbar, &TextFormatToolbar::colorChanged,
            this, [this](const QColor& color) {
        Q_UNUSED(color);
        onPageModified();
    });
    connect(m_textFormatToolbar, &TextFormatToolbar::alignmentChanged,
            this, [this](Qt::Alignment align) {
        Q_UNUSED(align);
        onPageModified();
    });

    // ---- 吸附开关 ----
    connect(m_snapToggleAction, &QAction::toggled, this, [this](bool checked) {
        if (m_editorView && m_editorView->editorScene()) {
            m_editorView->editorScene()->setSnapEnabled(checked);
            statusBar()->showMessage(checked ? tr("吸附已启用") : tr("吸附已禁用"), 2000);
        }
    });
}

void MainWindow::updateBookList()
{
    // 委托给素材树组件填充书籍及素材节点
    m_assetTree->populateMaterials(m_allBooks, m_basePath);
}

void MainWindow::updateSelectedBooks()
{
    m_selectedBooks.clear();

    // 从素材树获取勾选的书籍索引
    QList<int> checkedIndices = m_assetTree->checkedBookIndices();
    for (int index : checkedIndices) {
        if (index >= 0 && index < m_allBooks.size()) {
            m_selectedBooks.append(m_allBooks[index]);
        }
    }
}

void MainWindow::updateLayoutEngine()
{
    int index = m_layoutCombo->currentIndex();
    LayoutMode mode = static_cast<LayoutMode>(m_layoutCombo->itemData(index).toInt());

    if (mode == LayoutMode::Auto) {
        mode = AutoLayoutDetector::detectLayout(m_selectedBooks);
    }

    if (mode == LayoutMode::SingleColumn) {
        m_layoutEngine = LayoutEnginePtr(new SingleColumnLayout());
    } else {
        m_layoutEngine = LayoutEnginePtr(new MultiColumnLayout());
    }

    m_layoutEngine->setBooks(m_selectedBooks);
    m_layoutEngine->calculateLayout();
}

void MainWindow::updateStatusBar()
{
    int bookCount = m_allBooks.size();
    int selectedCount = m_selectedBooks.size();
    int pageCount = m_layoutEngine ? m_layoutEngine->pageCount() : 0;
    int currentPage = m_editorView ? m_editorView->currentPage() : 0;

    m_statusBooksLabel->setText(tr("总书籍: %1 本 (已选: %2)").arg(bookCount).arg(selectedCount));
    m_statusPagesLabel->setText(tr("总页数: %1").arg(pageCount));
    m_statusCurrentLabel->setText(tr("当前: 第%1页").arg(currentPage + 1));

    // 同步页码跳转SpinBox的范围和值
    if (m_pageSpinBox) {
        m_pageSpinBox->blockSignals(true);
        m_pageSpinBox->setMinimum(pageCount > 0 ? 1 : 0);
        m_pageSpinBox->setMaximum(qMax(1, pageCount));
        m_pageSpinBox->setValue(pageCount > 0 ? currentPage + 1 : 0);
        m_pageSpinBox->blockSignals(false);
    }

    bool hasBooks = !m_allBooks.isEmpty();
    m_exportPdfAction->setEnabled(hasBooks && !m_selectedBooks.isEmpty());
    m_saveProjectAction->setEnabled(hasBooks);
    m_prevPageAction->setEnabled(hasBooks && currentPage > 0);
    m_nextPageAction->setEnabled(hasBooks && currentPage < pageCount - 1);
}

void MainWindow::recalculateLayout()
{
    updateSelectedBooks();
    updateLayoutEngine();

    if (m_editorView && m_layoutEngine) {
        // 重置编辑数据列表，匹配新的页数
        int newCount = m_layoutEngine->pageCount();
        m_editedPages.clear();
        for (int i = 0; i < newCount; ++i) {
            m_editedPages.append(PageDataPtr());   // null表示尚未编辑
        }

        m_editorView->setLayoutEngine(m_layoutEngine);
        m_editorView->setCurrentPage(0, true);  // forceReload=true 确保新引擎元素被加载
        m_editorView->fitToWindow();
        m_currentPage = 0;
    }

    updateStatusBar();
}

bool MainWindow::loadFromDirectory(const QString& dirPath, QString* errorMessage)
{
    m_zipReader.reset();

    QDir dir(dirPath);
    if (!dir.exists()) {
        if (errorMessage) *errorMessage = tr("目录不存在");
        return false;
    }

    QString xlsxPath;
    QStringList xlsxFilters;
    xlsxFilters << "bookpic.xlsx" << "book.xlsx" << "*.xlsx";

    for (const QString& filter : xlsxFilters) {
        QStringList files = dir.entryList(QStringList() << filter, QDir::Files);
        if (!files.isEmpty()) {
            xlsxPath = dir.absoluteFilePath(files.first());
            break;
        }
    }

    if (xlsxPath.isEmpty()) {
        if (errorMessage) *errorMessage = tr("目录中未找到Excel文件(bookpic.xlsx、book.xlsx或其他.xlsx文件)");
        return false;
    }

    ExcelParser parser;
    QString parseError;
    BookList books = parser.parseFromFile(xlsxPath, &parseError);
    if (books.isEmpty()) {
        if (errorMessage) *errorMessage = tr("解析Excel失败: %1").arg(parseError);
        return false;
    }

    ImageFinder finder(dirPath);
    for (BookPtr book : books) {
        finder.findImagesForBook(book);
    }

    // 扫描TXT文件并关联到书籍
    scanTxtFiles(books, dirPath);

    m_allBooks = books;
    m_basePath = dirPath;
    return true;
}

bool MainWindow::loadFromZip(const QString& zipPath, QString* errorMessage)
{
    m_zipReader.reset(new ZipReader());
    if (!m_zipReader->open(zipPath)) {
        m_zipReader.reset();
        if (errorMessage) *errorMessage = tr("无法打开ZIP文件");
        return false;
    }

    QString tempDir = m_zipReader->extractToTempDir();
    if (tempDir.isEmpty()) {
        m_zipReader.reset();
        if (errorMessage) *errorMessage = tr("解压ZIP失败");
        return false;
    }

    QDir dir(tempDir);
    QString xlsxPath;

    QStringList nameFilters;
    nameFilters << "bookpic.xlsx" << "book.xlsx" << "*.xlsx";
    QStringList xlsxFiles = dir.entryList(nameFilters, QDir::Files);
    if (!xlsxFiles.isEmpty()) {
        xlsxPath = dir.absoluteFilePath(xlsxFiles.first());
    } else {
        QStringList subDirs = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        for (const QString& subDir : subDirs) {
            QDir subDirPath(dir.absoluteFilePath(subDir));
            QStringList subXlsxFiles = subDirPath.entryList(nameFilters, QDir::Files);
            if (!subXlsxFiles.isEmpty()) {
                xlsxPath = subDirPath.absoluteFilePath(subXlsxFiles.first());
                break;
            }
        }
    }

    if (xlsxPath.isEmpty()) {
        m_zipReader.reset();
        if (errorMessage) *errorMessage = tr("ZIP中未找到Excel文件(bookpic.xlsx、book.xlsx或其他.xlsx文件)");
        return false;
    }

    ExcelParser parser;
    QString parseError;
    BookList books = parser.parseFromFile(xlsxPath, &parseError);
    if (books.isEmpty()) {
        m_zipReader.reset();
        if (errorMessage) *errorMessage = tr("解析Excel失败: %1").arg(parseError);
        return false;
    }

    QFileInfo xlsxInfo(xlsxPath);
    QString imageDir = xlsxInfo.absolutePath();
    ImageFinder finder(imageDir);
    for (BookPtr book : books) {
        finder.findImagesForBook(book);
    }

    // 扫描TXT文件并关联到书籍
    scanTxtFiles(books, imageDir);

    m_allBooks = books;
    m_basePath = imageDir;
    return true;
}

void MainWindow::scanTxtFiles(const QList<BookPtr>& books, const QString& basePath)
{
    if (books.isEmpty()) return;

    QStringList txtFilters;
    txtFilters << "*.txt" << "*.TXT";
    // 递归扫描目录下所有TXT文件（使用QDirIterator支持子目录）
    QDirIterator it(basePath, txtFilters, QDir::Files, QDirIterator::Subdirectories);
    QFileInfoList txtFiles;
    while (it.hasNext()) {
        it.next();
        txtFiles.append(it.fileInfo());
    }

    if (txtFiles.isEmpty()) return;

    // 第一阶段：按ISBN和书名关联TXT文件到对应书籍
    for (BookPtr book : books) {
        QStringList matchedTxts;
        QString isbn = book->isbn();
        QString title = book->title();

        for (const QFileInfo& txtInfo : txtFiles) {
            QString txtName = txtInfo.baseName();  // 文件名（不含扩展名）

            // 关联策略：
            // 1. 文件名包含ISBN → 关联到对应书籍
            // 2. 文件名包含书名 → 关联到对应书籍
            if ((!isbn.isEmpty() && txtName.contains(isbn, Qt::CaseInsensitive)) ||
                (!title.isEmpty() && txtName.contains(title, Qt::CaseInsensitive))) {
                matchedTxts.append(txtInfo.absoluteFilePath());
            }
        }

        book->setTxtFiles(matchedTxts);
    }

    // 第二阶段：未关联的TXT文件分配到所有书
    QStringList unmatchedTxts;
    for (const QFileInfo& txtInfo : txtFiles) {
        bool matched = false;
        for (BookPtr book : books) {
            if (book->txtFiles().contains(txtInfo.absoluteFilePath())) {
                matched = true;
                break;
            }
        }
        if (!matched) {
            unmatchedTxts.append(txtInfo.absoluteFilePath());
        }
    }

    // 未关联的TXT添加到每本书（所有书共享）
    if (!unmatchedTxts.isEmpty()) {
        for (BookPtr book : books) {
            QStringList all = book->txtFiles();
            all.append(unmatchedTxts);
            book->setTxtFiles(all);
        }
    }
}

void MainWindow::onOpenFile()
{
    QString filePath;

    QMessageBox msgBox(this);
    msgBox.setWindowTitle(tr("选择打开方式"));
    msgBox.setText(tr("请选择要打开的内容:"));
    msgBox.setIcon(QMessageBox::Question);
    msgBox.addButton(tr("打开文件夹"), QMessageBox::AcceptRole);
    msgBox.addButton(tr("打开ZIP文件"), QMessageBox::ActionRole);
    QPushButton* cancelBtn = msgBox.addButton(tr("取消"), QMessageBox::RejectRole);
    msgBox.exec();

    if (msgBox.clickedButton() == cancelBtn) {
        return;
    }

    if (msgBox.buttonRole(msgBox.clickedButton()) == QMessageBox::AcceptRole) {
        filePath = QFileDialog::getExistingDirectory(this,
            tr("选择文件夹"), m_openDir,
            QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    } else {
        filePath = QFileDialog::getOpenFileName(this,
            tr("选择ZIP文件"), m_openDir,
            tr("ZIP文件 (*.zip);;所有文件 (*.*)"));
    }

    if (filePath.isEmpty()) {
        return;
    }

    QFileInfo fileInfo(filePath);
    m_openDir = fileInfo.isDir() ? filePath : fileInfo.absolutePath();

    QApplication::setOverrideCursor(Qt::WaitCursor);

    // 使用QtConcurrent在工作线程中执行文件加载，避免阻塞UI
    using LoadResult = QPair<bool, QString>;
    QFutureWatcher<LoadResult>* watcher = new QFutureWatcher<LoadResult>(this);
    connect(watcher, &QFutureWatcher<LoadResult>::finished, this, [this, watcher, filePath]() {
        QApplication::restoreOverrideCursor();
        LoadResult result = watcher->result();
        bool ok = result.first;
        QString errorMessage = result.second;
        watcher->deleteLater();

        if (!ok) {
            QMessageBox::warning(this, tr("打开失败"), errorMessage);
            return;
        }

        // 记录原始数据源路径（用于项目保存/恢复）
        m_sourcePath = filePath;

        updateBookList();
        // 不再自动生成页面，等待用户在版式库选择版式
        // 清除现有页面
        if (m_editorView && m_editorView->editorScene()) {
            m_editorView->editorScene()->clearPage();
            m_editorView->editorScene()->updatePageBackground();
        }
        m_editedPages.clear();
        if (m_layoutEngine) {
            m_layoutEngine.clear();
        }
        m_layoutCombo->setCurrentIndex(0);

        statusBar()->showMessage(tr("成功加载 %1 本书，请在版式库选择版式生成页面").arg(m_allBooks.size()), 5000);
    });

    QFuture<LoadResult> future = QtConcurrent::run([this, filePath]() -> LoadResult {
        QString errorMessage;
        QFileInfo info(filePath);
        bool ok = false;
        if (info.isDir()) {
            ok = loadFromDirectory(filePath, &errorMessage);
        } else if (info.suffix().toLower() == "zip") {
            ok = loadFromZip(filePath, &errorMessage);
        } else {
            errorMessage = tr("请选择文件夹或ZIP文件");
        }
        return qMakePair(ok, errorMessage);
    });
    watcher->setFuture(future);
}

void MainWindow::onExportPdf()
{
    // 导出前保存当前页编辑数据
    saveCurrentPageData();

    if (!m_layoutEngine || m_selectedBooks.isEmpty()) {
        QMessageBox::warning(this, tr("提示"), tr("没有可导出的内容，请先打开文件"));
        return;
    }

    QString filter = tr("PDF文件 (*.pdf)");
    QString defaultName = tr("书目.pdf");
    QString savePath = QFileDialog::getSaveFileName(this,
        tr("导出PDF"), m_openDir.isEmpty() ? defaultName : m_openDir + "/" + defaultName, filter);

    if (savePath.isEmpty()) {
        return;
    }

    if (!savePath.endsWith(".pdf", Qt::CaseInsensitive)) {
        savePath += ".pdf";
    }

    m_exporter->setLayoutEngine(m_layoutEngine);

    QProgressDialog progress(tr("正在导出PDF..."), tr("取消"), 0, m_layoutEngine->pageCount(), this);
    progress.setWindowModality(Qt::WindowModal);
    progress.setMinimumDuration(0);
    progress.setValue(0);

    disconnect(m_exporter, &PdfExporter::exportProgress, this, &MainWindow::onExportProgress);
    connect(m_exporter, &PdfExporter::exportProgress,
            [&progress](int current, int total) {
                progress.setMaximum(total);
                progress.setValue(current);
                QApplication::processEvents();
            });

    QApplication::processEvents();

    bool exportSuccess = false;
    QString exportError;

    // 使用双参数版本导出，传入编辑后的页面数据
    // 若m_editedPages为空，PdfExporter内部会回退到renderPage模式
    exportSuccess = m_exporter->exportToFile(savePath, m_editedPages, &exportError);

    disconnect(m_exporter, &PdfExporter::exportProgress, nullptr, nullptr);
    connect(m_exporter, &PdfExporter::exportProgress, this, &MainWindow::onExportProgress);

    progress.setValue(m_layoutEngine->pageCount());

    if (exportSuccess && !progress.wasCanceled()) {
        QMessageBox::information(this, tr("导出成功"),
            tr("PDF已成功导出到:\n%1\n共 %2 页").arg(savePath).arg(m_layoutEngine->pageCount()));
        statusBar()->showMessage(tr("PDF导出成功: %1").arg(savePath), 5000);
    } else if (!exportSuccess && !progress.wasCanceled()) {
        QMessageBox::warning(this, tr("导出失败"),
            tr("导出PDF失败:\n%1").arg(exportError));
    }
}

void MainWindow::onSaveProject()
{
    // 保存前先保存当前页编辑数据
    saveCurrentPageData();

    if (m_editedPages.isEmpty()) {
        QMessageBox::warning(this, tr("提示"), tr("没有可保存的项目数据"));
        return;
    }

    // 默认文件名
    QString defaultName = QStringLiteral("untitled.bpc");
    QString defaultPath = m_openDir.isEmpty() ? defaultName : (m_openDir + "/" + defaultName);

    QString savePath = QFileDialog::getSaveFileName(this,
        tr("保存项目"), defaultPath, tr("项目文件 (*.bpc)"));
    if (savePath.isEmpty()) {
        return;
    }

    // 确保扩展名为.bpc
    if (!savePath.endsWith(".bpc", Qt::CaseInsensitive)) {
        savePath += ".bpc";
    }

    // 构建JSON项目文件
    QJsonObject project;
    project["version"] = QStringLiteral("1.0");
    project["sourcePath"] = m_sourcePath;          // 原始数据源路径（目录或ZIP）
    project["pageCount"] = m_editedPages.size();

    // 序列化所有页面（包括未编辑的null页），保留页码索引
    QJsonArray pagesArray;
    for (int i = 0; i < m_editedPages.size(); ++i) {
        const PageDataPtr& page = m_editedPages[i];
        if (page) {
            QJsonObject pageJson = page->toJson();
            pageJson["pageIndex"] = i;   // 显式写入页码索引，确保加载时位置正确
            pagesArray.append(pageJson);
        } else {
            // 未编辑的页用null占位，保持页码索引一致
            pagesArray.append(QJsonValue::Null);
        }
    }
    project["pages"] = pagesArray;

    // 写入文件
    QJsonDocument doc(project);
    QFile file(savePath);
    if (!file.open(QIODevice::WriteOnly)) {
        QMessageBox::warning(this, tr("保存失败"),
            tr("无法写入文件: %1\n%2").arg(savePath).arg(file.errorString()));
        return;
    }

    qint64 written = file.write(doc.toJson(QJsonDocument::Indented));
    file.close();

    if (written < 0) {
        QMessageBox::warning(this, tr("保存失败"),
            tr("写入文件失败: %1").arg(file.errorString()));
        return;
    }

    // 更新最近打开目录
    m_openDir = QFileInfo(savePath).absolutePath();

    statusBar()->showMessage(tr("项目已保存: %1").arg(savePath), 3000);

    // 添加到最近项目列表
    addToRecentProjects(savePath);
}

// ============================================================
// onExportLayout - 导出版式
//
// 将当前页面的所有元素序列化为 .layout 版式文件。
// 版式文件包含版本号、创建时间、纸张尺寸及元素列表，
// 可用于跨项目复用页面布局。
// ============================================================
void MainWindow::onExportLayout()
{
    // 保存当前页数据到m_editedPages
    saveCurrentPageData();

    EditorScene* scene = m_editorView ? m_editorView->editorScene() : nullptr;
    if (!scene) return;

    // 获取当前页元素
    if (m_currentPage < 0 || m_currentPage >= m_editedPages.size()) {
        QMessageBox::warning(this, tr("提示"), tr("当前没有有效页面"));
        return;
    }
    PageDataPtr currentPage = m_editedPages.value(m_currentPage);
    if (!currentPage || currentPage->elements().isEmpty()) {
        QMessageBox::warning(this, tr("提示"), tr("当前页面没有元素可导出"));
        return;
    }

    // 文件保存对话框
    QString defaultName = QStringLiteral("layout.layout");
    QString defaultPath = m_openDir.isEmpty() ? defaultName : (m_openDir + "/" + defaultName);
    QString savePath = QFileDialog::getSaveFileName(this,
        tr("导出版式"), defaultPath, tr("版式文件 (*.layout)"));
    if (savePath.isEmpty()) return;

    // 确保扩展名为.layout
    if (!savePath.endsWith(".layout", Qt::CaseInsensitive)) {
        savePath += ".layout";
    }

    // 构建版式JSON
    QJsonObject layout;
    layout["version"] = QStringLiteral("1.0");
    layout["created"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    layout["paperSize"] = PageSizeManager::instance().currentPaper().name;
    layout["paperWidthMm"] = PageSizeManager::instance().currentPaper().widthMm;
    layout["paperHeightMm"] = PageSizeManager::instance().currentPaper().heightMm;
    layout["elementCount"] = currentPage->elements().size();

    // 序列化元素列表（使用constData()只读访问，避免触发detach）
    QJsonArray elemArray;
    for (const PageElementPtr& elem : currentPage->elements()) {
        elemArray.append(elem.constData()->toJson());
    }
    layout["elements"] = elemArray;

    // 写入文件
    QJsonDocument doc(layout);
    QFile file(savePath);
    if (!file.open(QIODevice::WriteOnly)) {
        QMessageBox::warning(this, tr("导出失败"),
            tr("无法写入文件: %1\n%2").arg(savePath).arg(file.errorString()));
        return;
    }

    qint64 written = file.write(doc.toJson(QJsonDocument::Indented));
    file.close();

    if (written < 0) {
        QMessageBox::warning(this, tr("导出失败"),
            tr("写入文件失败: %1").arg(file.errorString()));
        return;
    }

    // 更新最近打开目录
    m_openDir = QFileInfo(savePath).absolutePath();
    statusBar()->showMessage(tr("版式已导出: %1 (%2个元素)")
        .arg(savePath).arg(elemArray.size()), 3000);
}

// ============================================================
// onImportLayout - 导入版式
//
// 从 .layout 版式文件加载元素列表，替换当前页面的所有元素。
// 导入操作通过撤销栈宏封装（删除现有元素 + 添加版式元素），
// 支持一次撤销恢复。可选切换纸张尺寸以匹配版式。
// ============================================================
void MainWindow::onImportLayout()
{
    EditorScene* scene = m_editorView ? m_editorView->editorScene() : nullptr;
    if (!scene) return;

    QString loadPath = QFileDialog::getOpenFileName(this,
        tr("导入版式"), m_openDir, tr("版式文件 (*.layout);;所有文件 (*.*)"));
    if (loadPath.isEmpty()) return;

    // 读取文件
    QFile file(loadPath);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, tr("导入失败"),
            tr("无法读取文件: %1\n%2").arg(loadPath).arg(file.errorString()));
        return;
    }

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
    file.close();

    if (parseError.error != QJsonParseError::NoError) {
        QMessageBox::warning(this, tr("导入失败"),
            tr("JSON解析错误: %1").arg(parseError.errorString()));
        return;
    }

    if (!doc.isObject()) {
        QMessageBox::warning(this, tr("导入失败"), tr("无效的版式文件格式"));
        return;
    }

    QJsonObject layout = doc.object();
    QJsonArray elemArray = layout.value("elements").toArray();
    if (elemArray.isEmpty()) {
        QMessageBox::warning(this, tr("提示"), tr("版式文件中没有元素"));
        return;
    }

    // 确认替换
    QMessageBox::StandardButton reply = QMessageBox::question(this,
        tr("导入版式"),
        tr("导入将清空当前页面的所有元素并替换为版式文件中的元素。\n是否继续？"),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
    if (reply != QMessageBox::Yes) return;

    // 如果版式文件包含纸张尺寸，切换纸张
    if (layout.contains("paperWidthMm") && layout.contains("paperHeightMm")) {
        qreal wMm = layout.value("paperWidthMm").toDouble();
        qreal hMm = layout.value("paperHeightMm").toDouble();
        PaperSize paper;
        paper.name = layout.value("paperSize").toString();
        if (paper.name.isEmpty()) paper.name = tr("自定义");
        paper.widthMm = wMm;
        paper.heightMm = hMm;
        changePaperSize(paper);
    }

    // 收集场景中所有现有的编辑器Item对应的元素数据
    QList<PageElementPtr> elementsToDelete;
    const QList<QGraphicsItem*> allItems = scene->items();
    for (QGraphicsItem* gi : allItems) {
        BaseEditorItem* bei = dynamic_cast<BaseEditorItem*>(gi);
        if (bei) {
            elementsToDelete.append(bei->elementData());
        }
    }

    // 通过撤销栈宏封装：删除现有元素 + 添加版式元素
    scene->undoStack()->beginMacro(QStringLiteral("导入版式"));

    if (!elementsToDelete.isEmpty()) {
        scene->undoStack()->push(new DeleteCommand(scene, elementsToDelete));
    }

    // 添加版式中的元素（按type字段分派创建对应类型的元素）
    for (const QJsonValue& val : elemArray) {
        QJsonObject elemJson = val.toObject();
        QString typeStr = elemJson.value("type").toString();

        PageElementData* rawElem = nullptr;
        if (typeStr == QStringLiteral("text")) {
            rawElem = new TextElementData();
        } else if (typeStr == QStringLiteral("image")) {
            rawElem = new ImageElementData();
        } else if (typeStr == QStringLiteral("shape")) {
            rawElem = new ShapeElementData();
        } else {
            // 跳过未知类型的元素
            continue;
        }

        // 从JSON填充元素属性
        rawElem->fromJson(elemJson);
        // 生成新ID避免冲突
        rawElem->setId(QUuid::createUuid().toString());

        // addElement内部通过AddCommand入撤销栈
        scene->addElement(PageElementPtr(rawElem));
    }

    scene->undoStack()->endMacro();

    // 更新当前页数据
    saveCurrentPageData();

    // 更新最近打开目录
    m_openDir = QFileInfo(loadPath).absolutePath();
    int count = elemArray.size();
    statusBar()->showMessage(tr("版式已导入: %1个元素").arg(count), 3000);
}

void MainWindow::onLoadProject()
{
    QString loadPath = QFileDialog::getOpenFileName(this,
        tr("加载项目"), m_openDir, tr("项目文件 (*.bpc);;所有文件 (*.*)"));
    if (loadPath.isEmpty()) {
        return;
    }

    QFile file(loadPath);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, tr("加载失败"),
            tr("无法打开文件: %1\n%2").arg(loadPath).arg(file.errorString()));
        return;
    }

    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &error);
    file.close();

    if (error.error != QJsonParseError::NoError) {
        QMessageBox::warning(this, tr("加载失败"),
            tr("JSON解析错误: %1").arg(error.errorString()));
        return;
    }

    if (!doc.isObject()) {
        QMessageBox::warning(this, tr("加载失败"),
            tr("文件格式错误: 不是有效的项目文件"));
        return;
    }

    QJsonObject project = doc.object();

    // 版本兼容性检查
    QString version = project.value("version").toString();
    if (version.isEmpty()) {
        QMessageBox::warning(this, tr("加载失败"),
            tr("文件格式错误: 缺少版本号，可能不是有效的项目文件"));
        return;
    }
    // 按主版本号判断兼容性（1.x 相互兼容）
    int majorVer = version.section('.', 0, 0).toInt();
    if (majorVer != 1) {
        QMessageBox::warning(this, tr("版本不兼容"),
            tr("项目文件版本 %1 与当前程序版本不兼容。\n当前支持版本: 1.x")
            .arg(version));
        return;
    }

    QString sourcePath = project.value("sourcePath").toString();
    QJsonArray pagesArray = project.value("pages").toArray();

    QApplication::setOverrideCursor(Qt::WaitCursor);

    bool sourceLoaded = false;
    if (!sourcePath.isEmpty()) {
        // 尝试重新加载数据源，恢复书籍数据和图片查找路径
        QFileInfo sourceInfo(sourcePath);
        QString errMsg;
        if (sourceInfo.isDir()) {
            sourceLoaded = loadFromDirectory(sourcePath, &errMsg);
        } else if (sourceInfo.exists() && sourceInfo.suffix().toLower() == QStringLiteral("zip")) {
            sourceLoaded = loadFromZip(sourcePath, &errMsg);
        } else {
            errMsg = tr("路径不存在");
        }

        if (sourceLoaded) {
            m_sourcePath = sourcePath;
            updateBookList();
            m_layoutCombo->setCurrentIndex(0);
            recalculateLayout();   // 创建正确数量的null页
        } else {
            // 数据源加载失败，警告但继续加载编辑数据
            QApplication::restoreOverrideCursor();
            QMessageBox::warning(this, tr("提示"),
                tr("无法重新加载数据源: %1\n原因: %2\n将仅加载编辑数据，图片可能无法显示。")
                .arg(sourcePath).arg(errMsg));
            QApplication::setOverrideCursor(Qt::WaitCursor);
        }
    }

    // 加载编辑数据
    if (sourceLoaded) {
        // 数据源已加载，按页码索引覆盖到m_editedPages（保持索引一致）
        for (int i = 0; i < pagesArray.size(); ++i) {
            QJsonValue val = pagesArray.at(i);
            if (!val.isObject()) {
                continue;   // null页保持原状
            }
            if (i >= m_editedPages.size()) {
                break;      // 超出当前页数，忽略
            }
            PageDataPtr page(new PageData());
            page->fromJson(val.toObject());
            m_editedPages[i] = page;
        }
    } else {
        // 数据源未加载，按数组顺序重建编辑页面列表
        m_editedPages.clear();
        for (int i = 0; i < pagesArray.size(); ++i) {
            QJsonValue val = pagesArray.at(i);
            if (val.isObject()) {
                PageDataPtr page(new PageData());
                page->fromJson(val.toObject());
                m_editedPages.append(page);
            } else {
                m_editedPages.append(PageDataPtr());   // null占位
            }
        }
    }

    // 更新最近打开目录
    m_openDir = QFileInfo(loadPath).absolutePath();

    // 加载第一页到编辑器
    m_currentPage = 0;
    if (m_currentPage >= 0 && m_currentPage < m_editedPages.size()) {
        loadPageData(m_currentPage);
        refreshLayerPanel();
    }
    if (m_pageSpinBox) {
        m_pageSpinBox->setValue(1);
    }
    updateStatusBar();

    QApplication::restoreOverrideCursor();

    statusBar()->showMessage(tr("项目已加载: %1（共 %2 页）")
        .arg(loadPath).arg(m_editedPages.size()), 3000);

    // 添加到最近项目列表
    addToRecentProjects(loadPath);
}

void MainWindow::onOpenRecentProject()
{
    // 由最近项目菜单动作触发，根据发送者获取项目路径
    QAction* action = qobject_cast<QAction*>(sender());
    if (!action) {
        return;
    }

    QString path = action->data().toString();
    if (path.isEmpty()) {
        return;
    }

    // 检查文件是否存在
    if (!QFile::exists(path)) {
        QMessageBox::warning(this, tr("文件不存在"),
            tr("项目文件不存在或已被移动:\n%1").arg(path));
        // 从最近列表中移除失效项
        m_recentProjects.removeAll(path);
        saveRecentProjects();
        updateRecentProjectsMenu();
        return;
    }

    // 直接调用加载项目逻辑：临时构造一个打开流程
    // 通过QFile读取并复用onLoadProject的核心逻辑
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, tr("加载失败"),
            tr("无法打开文件: %1\n%2").arg(path).arg(file.errorString()));
        return;
    }

    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &error);
    file.close();

    if (error.error != QJsonParseError::NoError) {
        QMessageBox::warning(this, tr("加载失败"),
            tr("JSON解析错误: %1").arg(error.errorString()));
        return;
    }

    if (!doc.isObject()) {
        QMessageBox::warning(this, tr("加载失败"),
            tr("文件格式错误: 不是有效的项目文件"));
        return;
    }

    QJsonObject project = doc.object();

    // 版本兼容性检查
    QString version = project.value("version").toString();
    if (version.isEmpty()) {
        QMessageBox::warning(this, tr("加载失败"),
            tr("文件格式错误: 缺少版本号，可能不是有效的项目文件"));
        return;
    }
    int majorVer = version.section('.', 0, 0).toInt();
    if (majorVer != 1) {
        QMessageBox::warning(this, tr("版本不兼容"),
            tr("项目文件版本 %1 与当前程序版本不兼容。\n当前支持版本: 1.x")
            .arg(version));
        return;
    }

    QString sourcePath = project.value("sourcePath").toString();
    QJsonArray pagesArray = project.value("pages").toArray();

    QApplication::setOverrideCursor(Qt::WaitCursor);

    bool sourceLoaded = false;
    if (!sourcePath.isEmpty()) {
        QFileInfo sourceInfo(sourcePath);
        QString errMsg;
        if (sourceInfo.isDir()) {
            sourceLoaded = loadFromDirectory(sourcePath, &errMsg);
        } else if (sourceInfo.exists() && sourceInfo.suffix().toLower() == QStringLiteral("zip")) {
            sourceLoaded = loadFromZip(sourcePath, &errMsg);
        } else {
            errMsg = tr("路径不存在");
        }

        if (sourceLoaded) {
            m_sourcePath = sourcePath;
            updateBookList();
            m_layoutCombo->setCurrentIndex(0);
            recalculateLayout();
        } else {
            QApplication::restoreOverrideCursor();
            QMessageBox::warning(this, tr("提示"),
                tr("无法重新加载数据源: %1\n原因: %2\n将仅加载编辑数据，图片可能无法显示。")
                .arg(sourcePath).arg(errMsg));
            QApplication::setOverrideCursor(Qt::WaitCursor);
        }
    }

    // 加载编辑数据
    if (sourceLoaded) {
        for (int i = 0; i < pagesArray.size(); ++i) {
            QJsonValue val = pagesArray.at(i);
            if (!val.isObject()) {
                continue;
            }
            if (i >= m_editedPages.size()) {
                break;
            }
            PageDataPtr page(new PageData());
            page->fromJson(val.toObject());
            m_editedPages[i] = page;
        }
    } else {
        m_editedPages.clear();
        for (int i = 0; i < pagesArray.size(); ++i) {
            QJsonValue val = pagesArray.at(i);
            if (val.isObject()) {
                PageDataPtr page(new PageData());
                page->fromJson(val.toObject());
                m_editedPages.append(page);
            } else {
                m_editedPages.append(PageDataPtr());
            }
        }
    }

    m_openDir = QFileInfo(path).absolutePath();

    m_currentPage = 0;
    if (m_currentPage >= 0 && m_currentPage < m_editedPages.size()) {
        loadPageData(m_currentPage);
        refreshLayerPanel();
    }
    if (m_pageSpinBox) {
        m_pageSpinBox->setValue(1);
    }
    updateStatusBar();

    QApplication::restoreOverrideCursor();

    statusBar()->showMessage(tr("项目已加载: %1（共 %2 页）")
        .arg(path).arg(m_editedPages.size()), 3000);

    // 刷新最近项目列表（将该项移到最前）
    addToRecentProjects(path);
}

void MainWindow::loadRecentProjects()
{
    // 使用QSettings从用户配置读取最近项目列表
    QSettings settings;
    settings.beginGroup(QStringLiteral("RecentProjects"));
    m_recentProjects = settings.value(QStringLiteral("list")).toStringList();
    settings.endGroup();

    // 限制最多5个
    while (m_recentProjects.size() > 5) {
        m_recentProjects.removeLast();
    }
}

void MainWindow::saveRecentProjects()
{
    // 持久化最近项目列表到QSettings
    QSettings settings;
    settings.beginGroup(QStringLiteral("RecentProjects"));
    settings.setValue(QStringLiteral("list"), m_recentProjects);
    settings.endGroup();
}

void MainWindow::updateRecentProjectsMenu()
{
    if (!m_recentProjectsMenu) {
        return;
    }

    // 清除旧的动作
    m_recentProjectsMenu->clear();
    for (QAction* action : m_recentProjectActions) {
        action->deleteLater();
    }
    m_recentProjectActions.clear();

    if (m_recentProjects.isEmpty()) {
        // 无最近项目时显示禁用的占位项
        QAction* emptyAction = m_recentProjectsMenu->addAction(tr("(无)"));
        emptyAction->setEnabled(false);
        m_recentProjectActions.append(emptyAction);
        m_recentProjectsMenu->setEnabled(false);
        return;
    }

    m_recentProjectsMenu->setEnabled(true);

    // 为每个最近项目创建动作（显示序号+文件名，最多5个）
    for (int i = 0; i < m_recentProjects.size(); ++i) {
        const QString& projectPath = m_recentProjects.at(i);
        QString displayName = tr("%1: %2").arg(i + 1).arg(QFileInfo(projectPath).fileName());
        QAction* action = m_recentProjectsMenu->addAction(displayName);
        action->setData(projectPath);
        action->setToolTip(projectPath);    // 鼠标悬停显示完整路径
        action->setStatusTip(projectPath);
        connect(action, &QAction::triggered, this, &MainWindow::onOpenRecentProject);
        m_recentProjectActions.append(action);
    }
}

void MainWindow::addToRecentProjects(const QString& path)
{
    if (path.isEmpty()) {
        return;
    }

    // 移除已存在的相同路径（去重）
    m_recentProjects.removeAll(path);

    // 插入到列表最前
    m_recentProjects.prepend(path);

    // 限制最多5个
    while (m_recentProjects.size() > 5) {
        m_recentProjects.removeLast();
    }

    // 持久化并更新菜单
    saveRecentProjects();
    updateRecentProjectsMenu();
}

void MainWindow::onExit()
{
    close();
}

void MainWindow::onAbout()
{
    QMessageBox::about(this, tr("关于书目PDF生成器"),
        tr("<h2>书目PDF生成器 1.0.0</h2>"
           "<p>用于从Excel导入书目数据并生成精美的PDF文档。</p>"
           "<p>支持单图排版和多图排版两种模式，内置可视化编辑器。</p>"
           "<p>基于 Qt 5 开发。</p>"
           "<p>Copyright &copy; 2026</p>"));
}

void MainWindow::onLayoutModeChanged(int index)
{
    Q_UNUSED(index);
    if (!m_allBooks.isEmpty()) {
        recalculateLayout();
    }
}

void MainWindow::onPrevPage()
{
    if (m_editorView) {
        int page = m_editorView->currentPage();
        if (page > 0) {
            // 翻页前保存当前页编辑数据
            saveCurrentPageData();
            m_editorView->setCurrentPage(page - 1);
        }
    }
}

void MainWindow::onNextPage()
{
    if (m_editorView && m_layoutEngine) {
        int page = m_editorView->currentPage();
        if (page < m_layoutEngine->pageCount() - 1) {
            // 翻页前保存当前页编辑数据
            saveCurrentPageData();
            m_editorView->setCurrentPage(page + 1);
        }
    }
}

void MainWindow::onZoomChanged(int index)
{
    Q_UNUSED(index);
    if (m_editorView) {
        QString zoomText = m_zoomCombo->currentText();
        zoomText.remove('%');
        bool ok;
        qreal zoom = zoomText.toDouble(&ok);   // 百分比值
        if (ok && zoom > 0) {
            // EditorView::setZoomLevel 接受百分比值（100=100%）
            m_editorView->setZoomLevel(zoom);
        }
    }
}

void MainWindow::onZoomFit()
{
    if (m_editorView) {
        m_editorView->fitToWindow();
        // 同步缩放比例显示（zoomLevel返回百分比）
        if (m_zoomCombo) {
            m_zoomCombo->blockSignals(true);
            m_zoomCombo->setCurrentText(tr("%1%").arg(qRound(m_editorView->zoomLevel())));
            m_zoomCombo->blockSignals(false);
        }
    }
}

void MainWindow::onBookItemChanged(int index, bool checked)
{
    Q_UNUSED(index);
    Q_UNUSED(checked);
    // 仅在已有布局引擎时重新计算（即用户已选择过版式）
    if (!m_allBooks.isEmpty() && m_layoutEngine) {
        recalculateLayout();
    }
}

void MainWindow::onLayoutSelected(LayoutMode mode)
{
    // 保存当前页面的编辑数据
    if (m_currentPage >= 0 && m_currentPage < m_editedPages.size()) {
        saveCurrentPageData();
    }

    // 设置布局模式（阻塞信号避免触发onLayoutModeChanged导致双重recalculateLayout调用）
    // m_layoutCombo 的 itemData: 0=Auto, 1=SingleColumn, 2=MultiColumn
    // 与 LayoutMode 枚举值一一对应
    {
        QSignalBlocker blocker(m_layoutCombo);
        m_layoutCombo->setCurrentIndex(static_cast<int>(mode));
    }

    // 重新计算布局
    recalculateLayout();
}

void MainWindow::onPreviewPageChanged(int page)
{
    // 页码已变化，更新当前页码
    m_currentPage = page;

    // 总是加载页面数据，确保翻页时数据独立：
    // - 若该页有已保存的编辑数据，则从编辑数据恢复
    // - 若该页无已保存数据，则从LayoutEngine::generateElements生成默认元素
    // 这样避免翻页后场景残留其他页的元素
    loadPageData(page);

    // 刷新图层面板和状态栏
    refreshLayerPanel();
    updateStatusBar();
}

void MainWindow::onPageJump(int page)
{
    if (!m_editorView) {
        return;
    }
    // SpinBox的值是1-based，编辑器是0-based
    int targetPage = page - 1;
    int maxPage = m_layoutEngine ? m_layoutEngine->pageCount() - 1 : 0;
    targetPage = qBound(0, targetPage, qMax(0, maxPage));
    if (targetPage != m_editorView->currentPage()) {
        saveCurrentPageData();
        m_editorView->setCurrentPage(targetPage);
    }
}

void MainWindow::onExportProgress(int currentPage, int totalPages)
{
    Q_UNUSED(currentPage);
    Q_UNUSED(totalPages);
}

// ============================================================
// 编辑器相关槽函数实现
// ============================================================

void MainWindow::onSelectionChanged(QList<PageElementPtr> elements)
{
    // 单选时更新属性面板
    if (elements.size() == 1) {
        m_propertyPanel->showElementProperties(elements.first());
    } else {
        m_propertyPanel->clearProperties();
    }

    // 更新状态栏提示
    if (elements.size() > 0) {
        statusBar()->showMessage(tr("已选中 %1 个元素").arg(elements.size()));
    } else {
        statusBar()->showMessage(tr("就绪"), 2000);
    }

    refreshLayerPanel();

    // 正向同步：场景选中→图层高亮
    if (m_layerPanel) {
        if (elements.size() == 1) {
            // 使用constData()避免QSharedDataPointer对抽象类PageElementData的detach/clone
            m_layerPanel->highlightElement(elements.first().constData()->id());
        } else if (elements.size() > 1) {
            QStringList ids;
            for (const PageElementPtr& elem : elements) {
                ids.append(elem.constData()->id());
            }
            m_layerPanel->highlightElements(ids);
        } else {
            // 无选中时清除高亮
            m_layerPanel->highlightElement(QString());
        }
    }

    // 确保选中装饰器同步刷新（拖放添加元素等场景）
    if (m_editorView && m_editorView->editorScene()) {
        m_editorView->editorScene()->updateSelectionDecorator();
    }
}

void MainWindow::onPageModified()
{
    // 页面被修改时，保存当前页编辑数据到m_editedPages
    saveCurrentPageData();
    // 刷新图层面板（元素位置/尺寸/属性可能变化）
    refreshLayerPanel();
}

void MainWindow::onElementMoved(const QString& elementId, const QPointF& pos)
{
    if (!m_editorView || !m_editorView->editorScene()) return;

    BaseEditorItem* item = findItemById(elementId);
    if (!item) return;

    QPointF oldPos = item->pos();
    // 位置未变化则不push命令
    if (oldPos == pos) return;

    // 通过MoveCommand实现撤销/重做
    // MoveCommand::redo()会执行setPos+syncToData，push时自动调用redo
    QMap<QString, QPointF> oldPositions, newPositions;
    oldPositions[elementId] = oldPos;
    newPositions[elementId] = pos;

    EditorScene* scene = m_editorView->editorScene();
    scene->undoStack()->push(new MoveCommand(scene, {item}, oldPositions, newPositions));
}

void MainWindow::onElementResized(const QString& elementId, const QSizeF& size)
{
    if (!m_editorView || !m_editorView->editorScene()) return;

    BaseEditorItem* item = findItemById(elementId);
    if (!item) return;

    QRectF oldRect = item->elementData().constData()->rect();
    // 尺寸未变化则不push命令
    if (oldRect.size() == size) return;

    // 通过ResizeCommand实现撤销/重做
    // ResizeCommand::redo()会重建Item（boundingRect依赖元素rect）
    QRectF newRect = oldRect;
    newRect.setSize(size);

    EditorScene* scene = m_editorView->editorScene();
    scene->undoStack()->push(new ResizeCommand(scene, elementId, oldRect, newRect));
}

void MainWindow::onElementRotated(const QString& elementId, qreal rotation)
{
    if (!m_editorView || !m_editorView->editorScene()) return;

    BaseEditorItem* item = findItemById(elementId);
    if (!item) return;

    qreal oldRotation = item->rotation();
    // 旋转角度未变化则不push命令
    if (qFuzzyCompare(oldRotation, rotation)) return;

    // 通过RotateCommand实现撤销/重做
    // RotateCommand::redo()会执行setRotation+syncToData
    EditorScene* scene = m_editorView->editorScene();
    scene->undoStack()->push(new RotateCommand(scene, elementId, oldRotation, rotation));
}

void MainWindow::onElementVisibilityChanged(const QString& elementId, bool visible)
{
    if (!m_editorView || !m_editorView->editorScene()) return;

    // 拦截背景白纸图层
    if (elementId == QStringLiteral("__background__")) {
        if (auto* bg = m_editorView->editorScene()->pageBackgroundItem())
            bg->setVisible(visible);
        return;
    }

    BaseEditorItem* item = findItemById(elementId);
    if (item) {
        // 隐藏选中元素时清除选中，避免装饰器残留
        if (!visible && item->isSelected()) {
            item->setSelected(false);
        }
        item->setVisible(visible);
        item->syncToData();
        item->update();
        // 仅持久化到m_editedPages，不调用onPageModified()以避免refreshLayerPanel()重建列表
        // 重建列表会m_listWidget->clear()删除正在发射toggled信号的QToolButton发送者，
        // 导致Qt信号处理异常。眼睛图标已由用户点击更新，无需重建面板。
        saveCurrentPageData();
    }
}

void MainWindow::onElementLockChanged(const QString& elementId, bool locked)
{
    if (!m_editorView || !m_editorView->editorScene()) return;

    BaseEditorItem* item = findItemById(elementId);
    if (!item) return;

    // 通过rebuildItemWithModifiedElement将locked状态写入元素数据并重建Item
    // 重建后新Item默认是可选中的，需根据locked状态设置Flag
    BaseEditorItem* newItem = rebuildItemWithModifiedElement(elementId,
        [locked](PageElementData* elem) {
            elem->setLocked(locked);
        });

    if (newItem) {
        // 锁定的元素不可选中、不可移动
        newItem->setFlag(QGraphicsItem::ItemIsSelectable, !locked);
        newItem->setFlag(QGraphicsItem::ItemIsMovable, !locked);
        // 仅持久化，不调用onPageModified()以避免refreshLayerPanel()在toggled信号
        // 发射期间删除发送者锁定按钮。锁定图标已由用户点击更新。
        saveCurrentPageData();
    }
}

void MainWindow::onElementZOrderChanged(const QString& elementId, int newZ)
{
    if (!m_editorView || !m_editorView->editorScene()) return;

    BaseEditorItem* item = findItemById(elementId);
    if (!item) return;

    int oldZ = static_cast<int>(item->zValue());
    // Z值未变化则不push命令
    if (oldZ == newZ) return;

    // 通过ZOrderCommand实现撤销/重做
    // ZOrderCommand::redo()会执行setZValue+syncToData
    EditorScene* scene = m_editorView->editorScene();
    scene->undoStack()->push(new ZOrderCommand(scene, elementId, oldZ, newZ));
    onPageModified();
}

void MainWindow::onElementZOrderBatchChanged(const QList<QPair<QString, int>>& changes)
{
    if (!m_editorView || !m_editorView->editorScene()) return;
    if (changes.isEmpty()) return;

    EditorScene* scene = m_editorView->editorScene();
    scene->undoStack()->beginMacro(QStringLiteral("调整图层顺序"));
    for (const auto& c : changes) {
        BaseEditorItem* item = findItemById(c.first);
        if (!item) continue;
        int oldZ = static_cast<int>(item->zValue());
        if (oldZ == c.second) continue;
        scene->undoStack()->push(new ZOrderCommand(scene, c.first, oldZ, c.second));
    }
    scene->undoStack()->endMacro();
    onPageModified();
}

void MainWindow::onElementDeleted(const QString& elementId)
{
    if (!m_editorView || !m_editorView->editorScene()) return;

    BaseEditorItem* item = findItemById(elementId);
    if (item) {
        m_editorView->editorScene()->removeItem(item);
        delete item;
        m_propertyPanel->clearProperties();
        onPageModified();
    }
}

void MainWindow::onElementDuplicated(const QString& elementId)
{
    Q_UNUSED(elementId);
    // 元素复制需要通过场景的命令系统（AddCommand）处理
    // 此处触发页面修改通知
    onPageModified();
}

void MainWindow::onApplyToAllPages()
{
    // 保存当前页编辑数据，确保源数据是最新的
    saveCurrentPageData();

    if (!m_layoutEngine) {
        QMessageBox::warning(this, tr("提示"), tr("未加载排版引擎"));
        return;
    }

    if (m_currentPage < 0 || m_currentPage >= m_editedPages.size()) {
        QMessageBox::warning(this, tr("提示"), tr("没有可应用的当前页数据"));
        return;
    }

    PageDataPtr currentData = m_editedPages[m_currentPage];
    if (!currentData || currentData->elements().isEmpty()) {
        QMessageBox::warning(this, tr("提示"), tr("当前页无元素可应用"));
        return;
    }

    // 从排版引擎获取当前页的真实类型（权威来源，比PageData中存储的更可靠）
    int currentType = m_layoutEngine->pageType(m_currentPage);
    QString typeName = (currentType == 0) ? tr("单图排版") : tr("多图排版");

    // 统计同类页数量（包括当前页），预先计算影响范围
    int totalPages = m_layoutEngine->pageCount();
    int scanLimit = qMin(m_editedPages.size(), totalPages);
    int sameTypeCount = 0;
    for (int i = 0; i < scanLimit; ++i) {
        if (m_layoutEngine->pageType(i) == currentType) {
            sameTypeCount++;
        }
    }

    if (sameTypeCount <= 1) {
        QMessageBox::information(this, tr("提示"),
            tr("没有其他同类页面（%1）可应用").arg(typeName));
        return;
    }

    // 确认对话框，说明影响范围
    QMessageBox::StandardButton reply = QMessageBox::question(this,
        tr("应用到所有同类页"),
        tr("将当前页的元素应用到所有同类页（%1）。\n"
           "将影响 %2 个同类页面，此操作不可撤销。")
            .arg(typeName).arg(sameTypeCount),
        QMessageBox::Yes | QMessageBox::No);
    if (reply != QMessageBox::Yes) {
        return;
    }

    // 遍历所有页，将同类页的元素替换为当前页元素的深拷贝
    // 使用clone()确保每页独立维护PageData，互不影响
    int appliedCount = 0;
    for (int i = 0; i < scanLimit; ++i) {
        if (i == m_currentPage) {
            continue;   // 跳过当前页本身
        }
        // 同类页判断：以排版引擎的pageType为准
        if (m_layoutEngine->pageType(i) != currentType) {
            continue;
        }

        PageDataPtr page = m_editedPages[i];
        if (!page) {
            // 未编辑过的同类页，创建新的PageData
            page = PageDataPtr(new PageData());
            page->setPageIndex(i);
            page->setPageType(currentType);
            m_editedPages[i] = page;
        }

        // 克隆当前页的所有元素（深拷贝）
        // clone()保留元素的rect/rotation等属性，即保持元素相对于页面的位置不变
        QList<PageElementPtr> newElements;
        for (const PageElementPtr& elem : currentData->elements()) {
            if (elem) {
                PageElementData* cloned = elem.constData()->clone();
                newElements.append(PageElementPtr(cloned));
            }
        }
        page->setElements(newElements);
        appliedCount++;
    }

    statusBar()->showMessage(
        tr("已应用到 %1 个同类页（共 %2 个同类页）").arg(appliedCount).arg(sameTypeCount), 3000);
}

void MainWindow::onUndo()
{
    if (m_undoStack) {
        m_undoStack->undo();
    }
}

void MainWindow::onRedo()
{
    if (m_undoStack) {
        m_undoStack->redo();
    }
}

// ============================================================
// 编辑器辅助函数实现
// ============================================================

void MainWindow::saveCurrentPageData()
{
    if (!m_editorView || !m_editorView->editorScene()) return;
    if (m_currentPage < 0 || m_currentPage >= m_editedPages.size()) return;

    // 从场景导出当前页数据
    PageDataPtr data = m_editorView->editorScene()->exportPageData();
    if (data) {
        m_editedPages[m_currentPage] = data;
    }
}

void MainWindow::loadPageData(int page)
{
    if (!m_editorView || !m_editorView->editorScene()) return;
    if (page < 0 || page >= m_editedPages.size()) return;

    EditorScene* scene = m_editorView->editorScene();
    PageDataPtr data = m_editedPages[page];

    // 无论data是否为null，都设置pageData并重新加载：
    // - data非null：从已保存的编辑数据恢复元素
    // - data为null：setPageData传入null清空场景的当前页数据，
    //   随后loadPage会调用LayoutEngine::generateElements生成默认元素
    // 这样确保翻页时每页数据独立，不会残留其他页的元素
    scene->setPageData(page, data);
    scene->loadPage(page);
}

void MainWindow::refreshLayerPanel()
{
    if (!m_layerPanel || !m_editorView || !m_editorView->editorScene()) {
        return;
    }

    // 从场景获取当前页数据，更新图层面板
    PageDataPtr data = m_editorView->editorScene()->exportPageData();
    if (data) {
        m_layerPanel->updateLayers(data->elements());
        // 注入背景白纸图层（固定最底层）
        if (auto* bg = m_editorView->editorScene()->pageBackgroundItem()) {
            m_layerPanel->setBackgroundItem(tr("白纸背景"), bg->isVisible());
        }
    } else {
        m_layerPanel->clearLayers();
    }
}

BaseEditorItem* MainWindow::findItemById(const QString& id) const
{
    if (!m_editorView || !m_editorView->editorScene() || id.isEmpty()) {
        return nullptr;
    }

    QList<QGraphicsItem*> items = m_editorView->editorScene()->items();
    for (QGraphicsItem* gitem : items) {
        BaseEditorItem* eitem = dynamic_cast<BaseEditorItem*>(gitem);
        if (eitem) {
            PageElementPtr elem = eitem->elementData();
            // 使用constData()而非直接在布尔上下文中使用elem，
            // 避免触发QSharedDataPointer非const的operator T*()→detach()→clone()，
            // 因为PageElementData是抽象类无法new
            if (elem.constData() && elem.constData()->id() == id) {
                return eitem;
            }
        }
    }
    return nullptr;
}

// ============================================================
// updateElementInPlace - 原地更新元素数据
//
// 用于属性面板修改属性时，避免rebuildItemWithModifiedElement
// 删除/重建Item触发selectionChanged→clearProperties/showProperties
// 打断PropertyPanel状态（清空m_currentElementId或设m_updating=true）。
//
// 流程：findItemById → clone元素 → 应用modifier → setElementData替换
// → saveCurrentPageData持久化 → updateSelectionDecorator刷新装饰器
// 不触发selectionChanged，不重建图层面板。
// ============================================================
void MainWindow::updateElementInPlace(const QString& elementId,
    std::function<void(PageElementData*)> modifier)
{
    if (!m_editorView || !m_editorView->editorScene()) return;

    BaseEditorItem* item = findItemById(elementId);
    if (!item) return;

    // clone元素数据并应用修改
    PageElementData* clone = item->elementData().constData()->clone();
    modifier(clone);
    PageElementPtr newElem(clone);

    // 原地替换元素数据（不删除/重建Item）
    item->setElementData(newElem);

    // 持久化到m_editedPages
    saveCurrentPageData();

    // 刷新选中装饰器（位置/尺寸可能变化）
    if (m_editorView->editorScene()) {
        m_editorView->editorScene()->updateSelectionDecorator();
    }
}

BaseEditorItem* MainWindow::rebuildItemWithModifiedElement(const QString& elementId,
    std::function<void(PageElementData*)> modifier)
{
    if (!m_editorView || !m_editorView->editorScene()) return nullptr;

    BaseEditorItem* oldItem = findItemById(elementId);
    if (!oldItem) return nullptr;

    // 记录旧Item状态
    bool wasSelected = oldItem->isSelected();
    PageElementData::ElementType type = oldItem->elementData().constData()->elementType();
    bool wasLocked = oldItem->elementData().constData()->isLocked();

    // 根据元素类型clone数据并应用修改
    PageElementData* clone = oldItem->elementData().constData()->clone();
    modifier(clone);

    // 移除旧Item
    EditorScene* scene = m_editorView->editorScene();
    scene->removeItem(oldItem);
    delete oldItem;

    // 根据类型创建新Item
    BaseEditorItem* newItem = nullptr;
    switch (type) {
    case PageElementData::Text: {
        TextElementPtr textPtr(static_cast<TextElementData*>(clone));
        newItem = new TextEditorItem(textPtr);
        break;
    }
    case PageElementData::Image: {
        ImageElementPtr imagePtr(static_cast<ImageElementData*>(clone));
        newItem = new ImageEditorItem(imagePtr);
        break;
    }
    case PageElementData::Shape: {
        ShapeElementPtr shapePtr(static_cast<ShapeElementData*>(clone));
        newItem = new ShapeEditorItem(shapePtr);
        break;
    }
    }

    if (newItem) {
        scene->addItem(newItem);
        // 恢复选中状态
        newItem->setSelected(wasSelected);
        // 恢复锁定状态（锁定元素不可选中、不可移动）
        if (wasLocked) {
            newItem->setFlag(QGraphicsItem::ItemIsSelectable, false);
            newItem->setFlag(QGraphicsItem::ItemIsMovable, false);
        }
    } else {
        // 创建失败，释放clone避免内存泄漏
        delete clone;
    }

    scene->update();
    onPageModified();
    return newItem;
}
