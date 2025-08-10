#!/bin/bash
set -e

# Variables
IMG_SIZE_MB=2048
ROOTFS_DIR="debian-rootfs"
ROOTFS_IMG="rootfs.img"
DEBIAN_RELEASE="bookworm"  # You can change to bullseye

# 1. Install debootstrap if not present
if ! command -v debootstrap &> /dev/null; then
    echo "[+] Installing debootstrap..."
    sudo apt update
    sudo apt install -y debootstrap
fi

# 2. Create Debian RootFS with debootstrap
echo "[+] Creating Debian $DEBIAN_RELEASE rootfs..."
sudo debootstrap --arch=arm64 --variant=minbase $DEBIAN_RELEASE $ROOTFS_DIR http://deb.debian.org/debian

# 3. Create ext4 image for rootfs
echo "[+] Creating ext4 image $ROOTFS_IMG..."
dd if=/dev/zero of=$ROOTFS_IMG bs=1M count=$IMG_SIZE_MB
mkfs.ext4 $ROOTFS_IMG

# 4. Mount and copy rootfs
echo "[+] Populating rootfs.img..."
mkdir -p mnt
sudo mount $ROOTFS_IMG mnt
sudo cp -a $ROOTFS_DIR/* mnt/

# 5. Mount /dev, /proc, /sys and chroot to configure
echo "[+] Configuring inside rootfs (chroot)..."
sudo mount --bind /dev mnt/dev
sudo mount --bind /proc mnt/proc
sudo mount --bind /sys mnt/sys

sudo chroot mnt /bin/bash -c "
# Set root password
echo 'root:root' | chpasswd
echo 'debian-arm64' > /etc/hostname

# Minimal fstab
echo 'proc /proc proc defaults 0 0' > /etc/fstab
echo 'sysfs /sys sysfs defaults 0 0' >> /etc/fstab
echo 'devtmpfs /dev devtmpfs defaults 0 0' >> /etc/fstab

# Install packages
apt update
apt install -y iproute2 openssh-server strace net-tools systemd-sysv ifupdown

# SSH Config Fixes
sed -i 's/#\?PermitRootLogin.*/PermitRootLogin yes/' /etc/ssh/sshd_config
sed -i 's/#\?PasswordAuthentication.*/PasswordAuthentication yes/' /etc/ssh/sshd_config

# Enable SSH
systemctl enable ssh

# Setup interfaces file for DHCP on eth0
cat <<EOF > /etc/network/interfaces
auto lo
iface lo inet loopback

auto eth0
iface eth0 inet dhcp
EOF
"

# 6. Unmount everything and clean up
echo "[+] Cleaning up..."
sudo umount mnt/dev
sudo umount mnt/proc
sudo umount mnt/sys
sync
sudo umount mnt

echo "[+] rootfs.img is ready to boot!"
