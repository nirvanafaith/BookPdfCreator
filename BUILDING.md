# 构建说明

## 环境要求

- Qt 5.12 LTS 或更高版本（兼容 Windows 7）
- 支持的编译器：MinGW 或 MSVC（对应 Qt 版本）
- qmake 构建系统

## 使用 Qt Creator 构建（推荐）

1. 打开 Qt Creator
2. 选择 "文件" -> "打开文件或项目"
3. 选择项目根目录下的 `BookPdfCreator.pro` 文件
4. 配置构建套件（选择已安装的 Qt 版本和编译器）
5. 点击 "构建" 按钮（或按 Ctrl+B）

## 命令行构建

### Windows (MinGW)

```cmd
mkdir build
cd build
qmake ../BookPdfCreator.pro
mingw32-make
```

### Windows (MSVC / nmake)

```cmd
mkdir build
cd build
qmake ../BookPdfCreator.pro
nmake
```

### Windows (MSVC / jom - 推荐用于多核编译)

```cmd
mkdir build
cd build
qmake ../BookPdfCreator.pro
jom
```

## 项目结构

```
BookPdfCreator/
├── src/                    # 源代码目录
│   ├── layout/            # 排版布局模块
│   ├── models/            # 数据模型
│   ├── parsers/           # Excel解析和图片查找
│   ├── pdf/               # PDF导出模块
│   ├── ui/                # 用户界面
│   └── utils/             # 工具类
├── third_party/           # 第三方库（QXlsx）
├── resources/             # 资源文件
└── BookPdfCreator.pro     # qmake项目文件
```

## 依赖说明

项目使用以下第三方库：
- QXlsx（已包含在 `third_party/QXlsx-master` 目录中，用于Excel文件读写）

所有依赖项均已包含在项目中，无需额外下载。
