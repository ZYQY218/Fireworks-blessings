# Fireworks Blessings

使用 C++17 和 raylib 制作的生日祝福动画，以烟花、星空、气球和背景音乐呈现祝福。

## 运行效果

![生日烟花祝福运行截图](docs/preview.png)

## 项目特色

- 16 套配对的烟花升空与绽放素材，多种烟花形状和粒子效果。
- 星空、气球、花瓣、彩色纸屑与动态祝福文字。
- 背景音乐及烟花音效，可在 `firework.cpp` 中修改祝福语和姓名。

## 编译运行

1. 安装 Visual Studio 2022 的“使用 C++ 的桌面开发”工作负载，并准备 raylib 6.0 Windows/MSVC x64 开发包。
2. 将 `raylib.local.props.example` 复制为 `raylib.local.props`，把 `RaylibRoot` 改为 raylib 的实际目录。
3. 打开 `wish.sln`，选择 `x64` 平台，生成后按 F5 或 Ctrl+F5 运行。

按 Esc 或关闭窗口退出。分发程序时，请保留可执行文件旁的资源文件夹及所需 DLL。

## 致谢

本项目使用 [raylib](https://www.raylib.com/) 提供图形和音频支持。
## 说明

本项目在 AI 辅助下完成开发，部分烟花素材由 AI 生成。
