# Aluminum

Aluminum 是 ISXo 操作系统使用的定制 Linux 6.12 内核（x86_64），面向虚拟机与低资源设备做了极限裁剪：无模块、全内建、无 DRM/声卡/无线/调试，启动即可用。

本仓库**只包含内核**。ISXo 系统层（用户态、安装器、管理面、图形界面）是闭源商业软件，不在此仓库，也不允许反编译——见发布附件 `LICENSE-ISXO.txt`。

## 构成

| 文件 | 说明 |
| --- | --- |
| `kernel/aluminum.config` | 完整内核配置（`make tinyconfig` + 合并后约 975 项 `=y`） |
| `kernel/build.sh` | 复现构建脚本（拉取 linux-6.12、合并配置、编译 bzImage） |
| `kernel/README.md` | 构建细节、内建命令行、已知取舍 |
| `docs/` | 启动参数、磁盘与引导布局说明 |

## 和上游的关系

内核源码为 **Linux 6.12（vanilla，kernel.org）**，没有源码级补丁；Aluminum 的差异全部体现在 `aluminum.config`（编译选项）与内建命令行。因此按 GPL-2.0 提供源码的方式是：`kernel/build.sh` 下载与 `linux-6.12.tar.xz` 完全一致的官方源码，套用本仓库配置即可得到逐字节等价的构建结果。

```
cd kernel && ./build.sh          # 需要 gcc、make、bison、flex、bc、xz、libelf
```

产物：`arch/x86/boot/bzImage`（约 3.9 MB）。

## 主要裁剪

- `CONFIG_MODULES=n`，驱动全部内建；无 initrd 依赖也可启动（本系统仍带 initramfs 用于挂载与安装）
- 关闭 DRM、声卡、无线、蓝牙、swap、BPF、KASAN/KCSAN、debug info
- `HZ_100`、`NO_HZ_IDLE`、`PREEMPT_NONE`、`CC_OPTIMIZE_FOR_PERFORMANCE`
- 保留 `FB_VESA`、`FB_EFI`、`FRAMEBUFFER_CONSOLE`，供 BIOS(VBE) 与 UEFI(GOP) 双模式的图形控制台
- 内建命令行：`console=tty0 console=ttyS0,115200n8 isxo=auto`

## 许可

内核部分为 GPL-2.0（见 `LICENSE`）。ISXo 用户态为专有软件，单独授权。
