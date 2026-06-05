#!/bin/bash
set -euo pipefail

# Note: we do not use all CPU cores. Set to 48 simply for convenience (CPU sweeping configuration).
SMP=48
MEM=64G
L2_ENV_ROOT=${L2_ENV_ROOT:-/path/to/l2-env}

# L2 QEMU should be launched with:
#   ./L2.sh
#
# L2 vCPU layout inside L1:
#   L2 vCPU 0-15  -> L1 CPU 16-31, host NUMA 2
#   L2 vCPU 16-47 -> L1 CPU 32-63, host NUMA 3
#   L2 QEMU auxiliary threads can use remaining allowed CPUs, mainly 64-67

L2_QEMU_CPUS=16-67

SHM_ADDR=0x2b8000000
SHM_SIZE=540M

# td_part_l2_qemu
QEMU=${QEMU:-${L2_ENV_ROOT}/qemu-td-partitioning/build/qemu-system-x86_64}

# L2 Kernel: td_part_l2_vmm
KERNEL=${KERNEL:-${L2_ENV_ROOT}/td-partitioning/arch/x86/boot/bzImage}

APPEND="root=/dev/vda4 rw console=tty0 console=ttyS0 ignore_loglevel nopat nokaslr earlyprintk=ttyS0 memmap=${SHM_SIZE}\$${SHM_ADDR}"

VCPU_ARGS=()
for ((i=0; i<SMP; i++)); do
    if [ "${i}" -lt 16 ]; then
        VCPU_ARGS+=("-vcpu" "vcpunum=${i},affinity=$((16 + i))")
    else
        VCPU_ARGS+=("-vcpu" "vcpunum=${i},affinity=$((32 + i - 16))")
    fi
done

sudo taskset -c ${L2_QEMU_CPUS} $QEMU \
    -smp ${SMP},sockets=1,maxcpus=${SMP} \
    "${VCPU_ARGS[@]}" \
    -no-hpet -nographic -vga none \
    -nodefaults \
    -m ${MEM} \
    -kernel ${KERNEL} \
    -append "${APPEND}" \
    -monitor telnet:127.0.0.1:1234,server,nowait \
    -object memory-backend-file,mem-path=/dev/mem,size=${MEM},share=on,id=mem0 \
    -device vfio-pci,host=00:03.0,x-no-mmap=true \
    -device vfio-pci,host=00:04.0,x-no-mmap=true \
    -chardev stdio,id=stdio0,signal=off,mux=on \
    -serial chardev:stdio0 \
    -mon chardev=stdio0,mode=readline \
    -cpu host,host-phys-bits \
    -M q35,accel=kvm,sata=off,kvm-type=td-part-enlighten,vfio-identity-bars=true,vfio-allow-noiommu=true,memory-backend=mem0,max-ram-below-4g=1G
