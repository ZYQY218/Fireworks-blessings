# Fireworks Blessings

使用 C++17 和 raylib 制作的生日祝福动画，以烟花、星空、气球和背景音乐呈现祝福。

## 运行效果

![生日烟花祝福运行截图](docs/preview.png)

## 项目特色

- 16 套配对的烟花升空与绽放素材，多种烟花形状和粒子效果。
- 星空、气球、花瓣、彩色纸屑与动态祝福文字。
- 鼠标点击指定位置放烟花，支持自动播放与手动模式。
- 通过配置文件修改祝福语、姓名和音量，无需改代码。

## 编译运行

1. 安装 Visual Studio 2022 的“使用 C++ 的桌面开发”工作负载，并准备 raylib 6.0 Windows/MSVC x64 开发包。
2. 将 `raylib.local.props.example` 复制为 `raylib.local.props`，把 `RaylibRoot` 改为 raylib 的实际目录。
3. 打开 `wish.sln`，选择 `x64` 平台，生成后按 F5 或 Ctrl+F5 运行。

按 Esc 或关闭窗口退出。分发程序时，请保留可执行文件旁的资源文件夹及所需 DLL。

## 自定义祝福

复制 `config.ini.example` 为 `config.ini`，修改后保存为 UTF-8，重启程序生效：

```ini
title=Happy Birthday
subtitle=Breeze in heart, ease in life
name=FRIEND
volume=0.5
auto_fireworks=true
```

没有配置文件或缺少字段时使用默认值；文字留空可隐藏对应内容。文字无需加引号，支持的字符取决于现有字体。
程序优先读取工作目录下的配置，其次读取可执行文件旁的配置。构建会将配置示例和存在的本地配置复制到输出目录。

## 操作

| 操作 | 功能 |
| --- | --- |
| 鼠标左键 | 从点击位置正下方垂直升空，在点击位置绽放（暂停时不可发射） |
| 空格 | 暂停 / 继续画面与声音 |
| A | 开关自动发射，已发射的烟花继续播放 |
| M | 静音 / 恢复声音 |
| ↑ / ↓ | 调节音量 |
| F11 | 切换无边框全屏，保持画面比例 |
| F12 | 保存截图到运行目录的 `screenshots/` |
| H | 显示 / 隐藏操作提示 |
| Esc | 退出 |

## 致谢

本项目使用 [raylib](https://www.raylib.com/) 提供图形和音频支持。

## 说明

本项目在 AI 辅助下完成开发，部分烟花素材由 AI 生成。
