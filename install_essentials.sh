#!/bin/bash
set -e

# Configuration Variables
SSH_PORT=2222
SSH_USER=root
SSH_HOST=localhost
KERNEL_BUILD_DIR=~/kernel_dev/linux  # Adjust this path to your kernel build dir
GUEST_KERNEL_DIR=/root/kernel-debug  # Where to copy vmlinux & headers inside guest

# Function to SSH Execute Commands
ssh_exec() {
    sshpass -p "root" ssh -o StrictHostKeyChecking=no -p $SSH_PORT $SSH_USER@$SSH_HOST "$@"
}

# Function to Copy Files to Guest
scp_copy() {
    sshpass -p "root" scp -P $SSH_PORT -o StrictHostKeyChecking=no "$1" $SSH_USER@$SSH_HOST:"$2"
}

# Ensure sshpass is installed
if ! command -v sshpass &> /dev/null; then
    echo "[+] Installing sshpass..."
    sudo apt update && sudo apt install -y sshpass
fi

# run this only once
# echo "[+] Installing kernel debugging tools in guest..."
# ssh_exec "apt update && apt install -y gdb gdb-multiarch strace lsof bpfcc-tools bpftool bpftrace build-essential vim htop tmux"

echo "[+] Creating /root/kernel-debug/ directory in guest..."
sshpass -p "root" ssh -p 2222 -o StrictHostKeyChecking=no root@localhost "mkdir -p /root/kernel-debug"


echo "[+] Copying custom vmlinux and kernel headers into guest..."
# Ensure vmlinux exists
if [ ! -f "$KERNEL_BUILD_DIR/vmlinux" ]; then
    echo "[-] vmlinux not found in $KERNEL_BUILD_DIR"
    exit 1
fi

scp_copy "$KERNEL_BUILD_DIR/vmlinux" "$GUEST_KERNEL_DIR/vmlinux"

# Copy kernel headers (include/linux)
ssh_exec "mkdir -p $GUEST_KERNEL_DIR/include"
scp -r -P $SSH_PORT -o StrictHostKeyChecking=no "$KERNEL_BUILD_DIR/include/linux" $SSH_USER@$SSH_HOST:$GUEST_KERNEL_DIR/include/

# Optionally copy kernel .config for reference
if [ -f "$KERNEL_BUILD_DIR/.config" ]; then
    scp_copy "$KERNEL_BUILD_DIR/.config" "$GUEST_KERNEL_DIR/.config"
fi

echo "[+] Kernel debugging environment is ready in guest!"
echo ""
echo "[!] vmlinux and headers are in $GUEST_KERNEL_DIR inside guest."
echo "[!] You can now use gdb, perf, bpftool, etc."
