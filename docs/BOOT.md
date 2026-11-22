# 引导与磁盘布局

- 引导：GRUB 2，BIOS（`boot.img` + `core.img`，前缀 `/ISXo/boot/grub`）与 UEFI（`grubx64.efi` + `unicode.pf2`）
- GRUB 先设 `gfxmode=1024x768x32,1024x768,800x600`、`gfxpayload=keep`，再移交内核，以便 `vesafb`/`efifb` 接管
- 首个画面是语言选择：`[1] English`、`[2] Chinese (zh-CN)`；选择通过内核命令行 `lang=` 传给 init
- 安装目标：任意可写卷（NTFS/exFAT/FAT32/ext4），目录 `<卷>/ISXo/`
- UEFI 启动项写入 `\EFI\ISXo\grubx64.efi`，失败时回退到 `\EFI\BOOT\BOOTX64.EFI`
- BIOS 安装需要 512B~1MB 的 post-MBR 空隙；GPT 且无 BIOS Boot 分区时安全跳过
