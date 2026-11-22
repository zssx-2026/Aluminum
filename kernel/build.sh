#!/bin/bash
# Build Aluminum 6.12 (x86_64) from vanilla Linux sources + aluminum.config.
set -e
VER=6.12
WORK=${WORK:-/tmp/aluminum-kbuild}
SRC=$WORK/linux-$VER
CONFIG=$(cd "$(dirname "$0")" && pwd)/aluminum.config

mkdir -p "$WORK"
if [ ! -d "$SRC" ]; then
  echo ">> fetching linux-$VER"
  curl -fL --retry 3 -o "$WORK/linux-$VER.tar.xz" "https://cdn.kernel.org/pub/linux/kernel/v6.x/linux-$VER.tar.xz"
  tar -xf "$WORK/linux-$VER.tar.xz" -C "$WORK"
fi

cd "$SRC"
export ARCH=x86_64
make tinyconfig
scripts/kconfig/merge_config.sh -m -O . .config "$CONFIG"
./scripts/config -d MODULE_SIG -d SYSTEM_TRUSTED_KEYRING \
  --set-str SYSTEM_TRUSTED_KEYS "" --set-str SYSTEM_BLACKLIST_HASH_LIST ""
make olddefconfig
make -j"$(nproc)" bzImage
echo ">> built: $SRC/arch/x86/boot/bzImage"
