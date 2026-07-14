# BookPdfCreator - 书目PDF生成器

基于 Qt 5.12 LTS 开发的书目PDF生成工具，支持从Excel导入书目数据并生成精美的PDF文档，兼容 Windows 7 操作系统。

## 系统要求

- Qt 5.12 LTS 或更高版本（Qt5 系列）
- qmake 构建系统
- C++11 兼容编译器（MSVC 2017/2019、MinGW 7.3+）
- Windows 7 及以上版本（已配置Win7兼容）

## 目录结构

```
BookPdfCreator/
├── src/
│   ├── models/          # 数据模型
│   ├── parsers/         # 解析器（Excel、文本等）
│   ├── layout/          # PDF排版布局
│   ├── pdf/             # PDF生成相关
│   ├── ui/              # 界面代码
│   └── utils/           # 工具类
├── third_party/
│   └── qxlsx/           # QXlsx Excel读写库
├── resources/           # 资源文件（图片、图标等）
├── build/               # 构建输出目录（自动生成）
├── BookPdfCreator.pro   # qmake项目文件
└── README.md            # 本文件
```

## 构建方法

### 使用 Qt Creator（推荐）

1. 打开 Qt Creator
2. 选择 "文件" -> "打开文件或项目"
3. 选择 `BookPdfCreator.pro` 文件
4. 配置构建套件（选择 Qt 5.12 或更高版本的套件）
5. 点击 "构建" -> "构建项目 BookPdfCreator"
6. 点击 "运行" 启动应用程序

### 使用命令行（Windows）

```cmd
# 打开 Qt 命令提示符（Qt 5.12 对应的命令行环境）
cd d:\PDF-create\BookPdfCreator

# 创建构建目录
mkdir build
cd build

# 运行qmake生成Makefile
qmake ..\BookPdfCreator.pro

# 编译（根据使用的编译器选择）
# MSVC:
nmake
# 或 MinGW:
mingw32-make
```

### 使用命令行（Linux/macOS）

```bash
cd /path/to/BookPdfCreator
mkdir build && cd build
qmake ../BookPdfCreator.pro
make -j$(nproc)
```

## 功能特性

- [x] 主窗口界面骨架（菜单栏、工具栏、状态栏）
- [ ] Excel书目数据导入（使用QXlsx库）
- [ ] PDF文档生成
- [ ] 自定义排版布局
- [ ] 预览功能
- [ ] 模板系统

## 依赖库

项目已集成以下第三方库：

- **QXlsx**: Qt Excel读写库（位于 `third_party/qxlsx/`）

## Qt模块配置

项目使用以下Qt模块：

- `QtCore`: 核心功能
- `QtGui`: GUI基础
- `QtWidgets`: 控件库
- `QtPrintSupport`: 打印和PDF导出支持

## Windows 7 兼容说明

项目已在 `.pro` 文件中配置了 Windows 7 兼容性设置：

- 定义了 `WINVER=0x0601` 和 `_WIN32_WINNT=0x0601`
- 使用 C++11 标准
- 启用高DPI缩放支持

## 许可证

待确定

## 开发说明

代码使用 C++11 标准，中文注释，遵循Qt编码规范。

- 所有UI相关代码放在 `src/ui/` 目录
- 数据模型放在 `src/models/`
- 业务逻辑与UI分离
- 使用Qt信号槽机制进行通信
