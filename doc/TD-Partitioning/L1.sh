#!/bin/bash
SMP=68
MEM=96G
NUMA_CPUS=90-119,210-227,60-79
NUMA_NODE=3
QUEUES=$(echo "${SMP} / 2"|bc)
TD_PARTITIONING_ROOT=${TD_PARTITIONING_ROOT:-/path/to/td-pa}
QEMU_IFUP=${QEMU_IFUP:-./qemu-ifup}

# L1: 0-15 for privacy zone.
# L2: for integrity zone.
#   0-15:  user connections
#   16-31: PostgreSQL
#   32-63: L2 vCPUs
#   64-67: L2 QEMU auxiliary threads

# L1 QEMU: td_part_l1_qemu
QEMU=${QEMU:-${TD_PARTITIONING_ROOT}/qemu-td-partitioning/build/qemu-system-x86_64}
# L1 Kernel: td_part_l1_vmm
KERNEL=${KERNEL:-${TD_PARTITIONING_ROOT}/td-partitioning/arch/x86/boot/bzImage}
# L1 BIOS: td_part_l1_ovmf
BIOS=${BIOS:-${TD_PARTITIONING_ROOT}/ovmf-td-partitioning/Build/IntelTdx/DEBUG_GCC5/FV/OVMF.fd}
# L2 BIOS: td_part_l2_ovmf
L2BIOS=${L2BIOS:-${TD_PARTITIONING_ROOT}/ovmf-td-partitioning/Build/OvmfX64/DEBUG_GCC5/FV/OVMF.fd}
# L1 rootfs
L1_ROOT_DISK=${L1_ROOT_DISK:-${TD_PARTITIONING_ROOT}/L1_rootfs.img}
# L2 rootfs
L2_ROOT_DISK=${L2_ROOT_DISK:-${TD_PARTITIONING_ROOT}/L2_rootfs.img}
APPEND="root=/dev/vda4 rw console=hvc0 nomce no-kvmclock no-steal-acc ignore_loglevel nopat memmap=1023M\$1M memmap=63G\$4G earlyprintk=ttyS0 isolcpus=16-67"

pin_l1_vcpus() {
  local qemu_pid tid vcpu host_cpu pinned

  for _ in $(seq 1 60); do
    qemu_pid=$(pgrep -n -f "qemu-system-x86_64.*-name tdp,debug-threads=on" || true)
    if [ -z "${qemu_pid}" ]; then
      sleep 1
      continue
    fi

    pinned=0
    for vcpu in $(seq 0 67); do
      tid=$(ps -L -p "${qemu_pid}" -o tid=,comm= |
        sed -n "s/^[[:space:]]*\\([0-9][0-9]*\\)[[:space:]]\\+CPU ${vcpu}\\/KVM$/\\1/p")

      if [ -z "${tid}" ]; then
        break
      fi

      if [ "${vcpu}" -lt 16 ]; then
        host_cpu=$((90 + vcpu))
      elif [ "${vcpu}" -lt 32 ]; then
        host_cpu=$((60 + vcpu - 16))
      elif [ "${vcpu}" -lt 46 ]; then
        host_cpu=$((106 + vcpu - 32))
      elif [ "${vcpu}" -lt 64 ]; then
        host_cpu=$((210 + vcpu - 46))
      else
        host_cpu=$((76 + vcpu - 64))
      fi

      sudo -n taskset -pc "${host_cpu}" "${tid}" >/dev/null || return 1
      echo "[pin] vCPU ${vcpu} -> host CPU ${host_cpu} (TID ${tid})" >&2
      pinned=$((pinned + 1))
    done

    if [ "${pinned}" -eq "${SMP}" ]; then
      echo "[pin] L1 vCPU pinning complete." >&2
      return 0
    fi

    sleep 1
  done

  echo "[pin] ERROR: failed to find all ${SMP} L1 vCPU threads." >&2
  return 1
}

sudo -v
pin_l1_vcpus &

sudo numactl --physcpubind=${NUMA_CPUS} --membind=${NUMA_NODE} \
  $QEMU \
        -name tdp,debug-threads=on \
        -smp ${SMP},sockets=1 \
        -cpu host,host-phys-bits,pmu=off,pks=on,-monitor \
        -no-hpet -nographic -vga none \
        -nodefaults \
        -object memory-backend-memfd-private,id=ram1,size=${MEM},host-nodes=${NUMA_NODE},policy=bind \
        -object tdx-guest,id=tdx0,debug=on,sept-ve-disable=on,num-l2-vms=2 \
        -machine q35,accel=kvm,l2bios=${L2BIOS},kernel-irqchip=split,sata=off,pic=off,pit=off,confidential-guest-support=tdx0,memory-backend=ram1 \
        -bios ${BIOS} \
        -kernel ${KERNEL} \
        -append "${APPEND}" \
        -drive if=none,cache=none,file=${L1_ROOT_DISK},id=drive0 -device virtio-blk-pci,drive=drive0,iommu_platform=true,disable-legacy=on \
        -netdev tap,id=tap0,queues=${QUEUES},script=${QEMU_IFUP},downscript=no -device virtio-net-pci,mq=true,netdev=tap0,mac=52:54:00:22:34:50,iommu_platform=true,disable-legacy=on \
        -drive if=none,cache=none,file=${L2_ROOT_DISK},id=drive1 -device virtio-blk-pci,drive=drive1,iommu_platform=true,disable-legacy=on \
        -netdev tap,id=tap1,queues=${QUEUES},script=${QEMU_IFUP},downscript=no -device virtio-net-pci,mq=true,netdev=tap1,mac=52:54:00:23:35:50,iommu_platform=true,disable-legacy=on \
        -monitor telnet:127.0.0.1:1235,server,nowait \
        -chardev stdio,id=mux,mux=on,signal=off \
        -device virtio-serial,romfile= \
        -device virtconsole,chardev=mux -monitor chardev:mux \
        -serial chardev:mux
