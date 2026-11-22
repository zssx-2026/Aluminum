# Aluminum

Aluminum 是自研的 x86 内核，**不基于 Linux**，不包含任何 Linux 源码。它自带引导协议（Multiboot2）、图形输出（VBE/GOP framebuffer）、PS/2 键盘、ATA/AHCI PIO 读盘、MBR/GPT 分区识别、CPU/内存探测，以及一套内置图形仪表盘。

仓库只放内核。Alumine 系统层与界面是闭源软件，只随发布提供二进制，禁止反编译。

## 现状（v1.0）

- BIOS 与 UEFI 双启动（GRUB 只承担装载，进入内核后完全自主）
- 首屏语言选择：`[1] English` / `[2] 中文`
- 1024x768x32 图形界面：CPU 型号/核心数、内存、磁盘与分区数、安装面板
- 键盘快捷键：`1` 切换语言，`2` 重绘，`6` 重启，`7` 关机
- 串口日志（115200 8N1），便于无屏调试

## 构建

```
./build.sh          # 产出 dist/Aluminum-1.0-x86_64.iso
```

依赖：`gcc -m32`、`ld`、`grub-mkimage`、`xorriso`（或 `grub-mkrescue`）。

## 目录

- `src/boot.S`：Multiboot2 头（含 framebuffer 请求）、栈、入口
- `src/kernel.c`：内核主体（串口、framebuffer、字体、键盘、CPU/内存/磁盘探测、界面）
- `src/font8.h` / `src/cjk.h`：ASCII 8x8 与中文 16x16 点阵
- `grub/menu.cfg`：引导菜单（语言选择在这里也可选）

## 路线（未完成）

驱动层继续补 AHCI/NVMe/USB、FAT32 写入、安装器与自有引导项；网络层自研隧道；GPU 加速；用户态组件与 Chromium 预览。内核按模块用 C/汇编实现，Rust 与 C++ 模块通过固定 ABI 接入；Python/Java/C# 只作为用户态运行时，不进内核。

## 许可

内核源码 GPL-2.0（见 `LICENSE`）。
