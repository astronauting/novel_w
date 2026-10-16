# novel_w

[![Windows CI](https://github.com/astronauting/novel_w/actions/workflows/windows.yml/badge.svg)](https://github.com/astronauting/novel_w/actions/workflows/windows.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)

原生 Win32 / C++17 的轻量中文写作编辑器。以单个 `.txt` 保存小说，自动生成章节目录，正文连续编辑；不依赖 Electron、Qt 或在线服务。

> 早期开发项目。保存会覆盖原文件，尚无版本备份或崩溃恢复保证；重要文稿请独立备份。

## 功能

- 按中文章节标题生成目录，点击跳转；显示全文与当前章节字数（不计空白）。
- 读取 UTF-8、带 BOM 的 UTF-16 LE/BE 和 GBK，保存为带 BOM 的 UTF-8 / CRLF。
- 查找、撤销/重做、章节导航与插入标题。
- 可调字体、背景颜色、吸管取色、整窗透明度与无边框模式。
- 应用内修改快捷键，支持全局隐藏/唤回。
- 已有文件路径时，每 30 秒将未保存更改自动写回；未命名文稿需先手工保存。

## 运行

目标系统：Windows 10/11 x64，中文界面。当前没有正式 Release 或签名安装包。

1. 按下文从源码构建，或在 [Actions](https://github.com/astronauting/novel_w/actions/workflows/windows.yml) 中下载成功运行的 `novel_w-windows-x64` 构建产物（通常需要登录 GitHub）。
2. 将 `novel_w.exe` 放到自己可写的目录运行，无需安装。
3. 用“文件 → 打开”打开 `samples/example.txt`，或新建自己的临时文稿。

`settings.json` 会写入 exe 同目录，记录外观、快捷键、窗口位置及上次文件路径。不要将此文件随程序公开分享。

## 从源码构建

```bat
git clone https://github.com/astronauting/novel_w.git
cd novel_w
build.bat
```

任选一种工具链：

- Visual Studio 2019/2022：安装“使用 C++ 的桌面开发”和 Windows SDK。脚本通过 `vswhere` 查找 MSBuild，构建 `NovelW.sln` 的 `Release | x64`。
- MinGW-w64：将 `g++`、`windres`（或 `x86_64-w64-mingw32-windres`）加入 PATH，运行 `build-mingw.bat`。也兼容原有 `%USERPROFILE%\.conda\envs\novelw-build` 环境。

产物：`bin\Release\novel_w.exe`。只使用 Windows 系统组件；MinGW 构建静态链接 GCC/C++ 运行库，MSVC 项目采用静态 CRT。

## 测试

在 VS x64 Native Tools 命令提示符，或已配置 MinGW-w64 的终端运行：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tests/run.ps1
```

测试编译后运行真实 C++ 核心逻辑，覆盖章节解析/定位、字数、换行规范化、UTF-8 保存及 UTF-8 / UTF-16 / GBK 读取。临时文件位于忽略的 `obj/tests/`，测试结束清理。

GitHub Actions 使用 Windows 2022 / MSVC 完成 Release 构建、核心测试并上传 exe。GUI、中文输入法、全局热键和多显示器行为仍须按 [贡献指南](CONTRIBUTING.md) 手工验收。

## 常用操作

| 操作 | 默认快捷键 |
| --- | --- |
| 新建 / 打开 / 保存 | Ctrl+N / Ctrl+O / Ctrl+S |
| 另存为 | Ctrl+Shift+S |
| 查找 | Ctrl+F |
| 上一章 / 下一章 | Ctrl+PageUp / Ctrl+PageDown |
| 插入章节标题 | Ctrl+Shift+I |
| 无边框切换 | F12 |
| 全局隐藏 / 唤回 | Alt+H |
| 背景透明度 | Ctrl+鼠标滚轮 |

在“设置 → 快捷键”改键，在“帮助 → 快捷键一览”查看当前绑定。Alt+H 若与其他软件冲突，可改绑。
章节识别支持“第X章 / 卷 / 部 / 节”和“楔子”“序章”等标题形式；目录识别为启发式，正文中类似标题的行也可能进入目录。

## 目录

```text
src/                 Win32 窗口、编辑控件、章节、文件、设置与资源
samples/example.txt     可公开的演示文稿
tests/               无第三方测试框架的核心回归测试
NovelW.sln           Visual Studio 解决方案
build*.bat           MSVC / MinGW 构建入口
.github/             Windows CI、Issue / PR 模板与 Actions 更新配置
```

## 贡献与安全

问题和建议请使用 [Issues](https://github.com/astronauting/novel_w/issues)。提交前阅读 [CONTRIBUTING.md](CONTRIBUTING.md)；安全漏洞及数据保护边界见 [SECURITY.md](SECURITY.md)。请勿上传私人文稿或个人配置。

## 许可证

[MIT License](LICENSE)，Copyright (c) 2026 astronauting。
