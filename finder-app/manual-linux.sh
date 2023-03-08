#!/bin/bash
# Script outline to install and build kernel.
# Author: Siddhant Jajoo.

set -e
set -u

OUTDIR=${1:-/tmp/aeld}
KERNEL_REPO=git://git.kernel.org/pub/scm/linux/kernel/git/stable/linux-stable.git
KERNEL_VERSION=v6.1.14
BUSYBOX_VERSION=1_33_1
SRCDIR=$(realpath "$(dirname $0)/..")
ARCH=arm64
CROSS_COMPILE=aarch64-none-linux-gnu-
SYSROOT=$(realpath "$(${CROSS_COMPILE}gcc -print-sysroot)")
CORES=$(grep -c ^processor /proc/cpuinfo)

echo_header () {
  echo "
--------------------------------------------------------------------
$@
--------------------------------------------------------------------
  "
}

echo_sub_header () {
  echo "
** $@ **
"
}

echo_header "
Source Directory: ${SRCDIR}
Output Directory: ${OUTDIR}
         Sysroot: ${SYSROOT}
     Kernel Repo: ${KERNEL_REPO}
 Kernvel Version: ${KERNEL_VERSION}
 Busybox Version: ${BUSYBOX_VERSION}
    Architecture: ${ARCH}
"
mkdir -p ${OUTDIR}

echo_header "Building Kernel"
cd "$OUTDIR"
if [ ! -d "${OUTDIR}/linux-stable" ]; then
  #Clone only if the repository does not exist.
	echo_sub_header "Cloning git Linux stable version ${KERNEL_VERSION} in ${OUTDIR}"
	git clone ${KERNEL_REPO} --depth 1 --single-branch --branch ${KERNEL_VERSION}
fi
if [ ! -e ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ]; then
  cd linux-stable
  echo_sub_header "Checking out version ${KERNEL_VERSION}"
  git checkout ${KERNEL_VERSION}

  echo_sub_header "Cleaning build tree"
  make ARCH=$ARCH CROSS_COMPILE=$CROSS_COMPILE mrproper
  echo_sub_header "Generating defconfig"
  make ARCH=$ARCH CROSS_COMPILE=$CROSS_COMPILE defconfig
  echo_sub_header "Building vmlinux image"
  make -j${CORES} ARCH=$ARCH CROSS_COMPILE=$CROSS_COMPILE all
fi

echo_sub_header "Adding the Image in outdir"
cd "$OUTDIR"
test -f Image || cp linux-stable/arch/arm64/boot/Image .

echo_header "Creating root filesystem"
cd "$OUTDIR"
if [ -d "${OUTDIR}/rootfs" ] ; then
	echo_sub_header "Deleting rootfs directory at ${OUTDIR}/rootfs and starting over"
  sudo rm -rf ${OUTDIR}/rootfs
fi

echo_sub_header "Creating base file tree"
base_dirs=( bin dev etc home lib lib64 proc sys sbin tmp usr usr/bin usr/sbin var var/log )
for dir in ${base_dirs[@]} ; do
  mkdir -p ${OUTDIR}/rootfs/${dir}
done
find ${OUTDIR}/rootfs/

echo_sub_header "Installing busybox (v${BUSYBOX_VERSION})"
cd "$OUTDIR"
if [ ! -d "${OUTDIR}/busybox" ] ; then
    git clone git://busybox.net/busybox.git
    cd busybox
    git checkout ${BUSYBOX_VERSION}
else
    cd busybox
fi

make distclean
make defconfig
make -j${CORES} ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE}
make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} CONFIG_PREFIX=${OUTDIR}/rootfs install

echo "Library dependencies"
interpreter="$(${CROSS_COMPILE}readelf -a ${OUTDIR}/rootfs/bin/busybox | grep 'program interpreter' | grep -o 'ld-linux[a-zA-Z0-9.-]*')"
libraries="$(${CROSS_COMPILE}readelf -a ${OUTDIR}/rootfs/bin/busybox | grep 'Shared library' | grep -o 'lib[a-z]*\.so\.[0-9]')"


cp $(find $SYSROOT -name "$interpreter") $OUTDIR/rootfs/lib/
for lib in $libraries ; do
  cp $(find $SYSROOT -name "$lib") $OUTDIR/rootfs/lib64/
done

echo_sub_header "Creating device nodes"
sudo mknod -m 666 ${OUTDIR}/rootfs/dev/null c 1 3
sudo mknod -m 600 ${OUTDIR}/rootfs/dev/console c 5 1

echo_sub_header "Installing writer utility"
cd $SRCDIR/finder-app
make clean
make CROSS_COMPILE=${CROSS_COMPILE}
cp writer ${OUTDIR}/rootfs/home

echo_sub_header "Installing finder scripts"
cd $SRCDIR/finder-app
cp autorun-qemu.sh finder.sh finder-test.sh ${OUTDIR}/rootfs/home
mkdir ${OUTDIR}/rootfs/home/conf
cp conf/* ${OUTDIR}/rootfs/home/conf

echo_sub_header "Setting rootfs owner"
cd ${OUTDIR}/rootfs
sudo chown -R root:root *

echo_header "Creating cpio initramfs"
find . | cpio -H newc -ov --owner root:root > ${OUTDIR}/initramfs.cpio
cd ${OUTDIR} && gzip -f initramfs.cpio
