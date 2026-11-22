#!/bin/bash
set -e
ROOT=${ROOT:-/usr}
OUT=${OUT:-$PWD/dist}
mkdir -p "$OUT" build/iso/boot/grub build/iso/EFI/BOOT
gcc -m32 -ffreestanding -fno-pie -fno-stack-protector -fno-builtin -nostdlib -O2 -c src/boot.S -o build/boot.o
gcc -m32 -ffreestanding -fno-pie -fno-stack-protector -fno-builtin -nostdlib -O2 -c src/kernel.c -o build/kernel.o
ld -m elf_i386 -T src/linker.ld -o build/aluminum.elf build/boot.o build/kernel.o
cp build/aluminum.elf build/iso/boot/
cp grub/menu.cfg build/iso/boot/grub/grub.cfg
MODS="biosdisk multiboot2 normal iso9660 part_msdos part_gpt fat search echo serial terminal all_video vbe video_bochs video_cirrus"
grub-mkimage -O i386-pc -c grub/menu.cfg -o build/core.img -p /boot/grub -d "$ROOT/lib/grub/i386-pc" $MODS
cat "$ROOT/lib/grub/i386-pc/cdboot.img" build/core.img > build/iso/boot/grub/bios.img
MODS_EFI="multiboot2 normal iso9660 part_gpt part_msdos fat search echo serial terminal all_video efi_gop efi_uga video_bochs video_cirrus"
grub-mkimage -O x86_64-efi -c grub/menu.cfg -o build/iso/EFI/BOOT/BOOTX64.EFI -p /EFI/BOOT -d "$ROOT/lib/grub/x86_64-efi" $MODS_EFI
truncate -s $(( $(stat -c%s build/iso/EFI/BOOT/BOOTX64.EFI) + 1048576 )) build/efi.img
mformat -i build/efi.img -v ALUMINUM ::
mmd -i build/efi.img ::/EFI
mmd -i build/efi.img ::/EFI/BOOT
mcopy -i build/efi.img build/iso/EFI/BOOT/BOOTX64.EFI ::/EFI/BOOT/BOOTX64.EFI
xorriso -as mkisofs -quiet -b boot/grub/bios.img -no-emul-boot -boot-load-size 4 -boot-info-table \
  -eltorito-alt-boot -e ../efi.img -no-emul-boot -isohybrid-gpt-basdat -o "$OUT/Aluminum-1.0-x86_64.iso" build/iso
