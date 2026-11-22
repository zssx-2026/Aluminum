# Aluminum 内核构建

## 依赖

`gcc`（x86_64）、`make`、`bison`、`flex`、`bc`、`xz`、`libelf-dev`、`libssl-dev`（或用脚本里的禁用签名开关跳过）。

## 步骤

`build.sh` 做四件事：

1. 下载 `https://cdn.kernel.org/pub/linux/kernel/v6.x/linux-6.12.tar.xz` 并解包到 `/tmp/aluminum-kbuild`（避免在慢速网络盘上编译）
2. `make ARCH=x86_64 tinyconfig`
3. `scripts/kconfig/merge_config.sh -m .config aluminum.config` → `make olddefconfig`
4. `make -j$(nproc) bzImage`

## 已验证环境

- gcc 11.4 / binutils 2.38，Ubuntu 22.04 容器，2 核
- 纯 TCG（无 KVM）下编译约 25 分钟，产物 4,097,024 B

## 注意

- 配置里 `CONFIG_MODULES=n`，任何 `=m` 项都会被 `olddefconfig` 提升为 `=y` 或关闭
- 内核不带签名密钥（`MODULE_SIG`、`SYSTEM_TRUSTED_KEYS` 均关闭），自行启用需自备证书
