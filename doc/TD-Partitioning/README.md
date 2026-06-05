# TD Partitioning

This directory provides reference scripts for launching the L1 VM and the L2 VM used in a TD-partitioning deployment. The full TD-partitioning environment setup is outside the scope of this artifact; see [Intel's TD-partitioning documentation](https://github.com/intel/td-partitioning/blob/main/How_to_setup_TD_Partitioning_environment.pdf) for the base platform setup.

The scripts are templates. Set the path variables near the top of `L1.sh` and `L2.sh`, or override them through environment variables, before running them.

## L1 Passthrough and Networking

Enable VFIO passthrough for the virtio devices that will be assigned to L2:

```bash
sudo modprobe vfio-pci
echo 1 | sudo tee /sys/module/vfio/parameters/enable_unsafe_noiommu_mode
echo "0000:00:03.0" | sudo tee /sys/bus/pci/devices/0000:00:03.0/driver/unbind > /dev/null
echo "1af4 1042" | sudo tee /sys/bus/pci/drivers/vfio-pci/new_id > /dev/null
echo "0000:00:04.0" | sudo tee /sys/bus/pci/devices/0000:00:04.0/driver/unbind > /dev/null
echo "1af4 1041" | sudo tee /sys/bus/pci/drivers/vfio-pci/new_id > /dev/null
sudo modprobe kvm_intel ple_gap=0
echo 1 | sudo tee /sys/module/vfio_iommu_type1/parameters/allow_unsafe_interrupts > /dev/null
```

The PCI IDs above are examples. Use the last two virtio devices reported by `lspci | grep Virtio`, excluding the communication device. The device numbers can change between boots.

After passthrough, L1 may lose network connectivity. Bring the L1 network interface back up:

```bash
sudo ip link set enp0s2 up
sudo ip addr add 192.168.122.10/24 dev enp0s2
sudo ip route add default via 192.168.122.1
sudo systemctl start ssh
```

Replace `enp0s2` with the interface name shown by `ip a`. After this, the host can connect to L1 with:

```bash
ssh ubuntu@192.168.122.10
```

## L2 Networking

Inside L2, find the network interface:

```bash
ip a
lspci | grep -i virtio
```

Configure networking:

```bash
sudo ip link set enp0s2 up
sudo ip addr add 192.168.122.11/24 dev enp0s2
sudo ip route add default via 192.168.122.1
echo "nameserver 8.8.8.8" | sudo tee /etc/resolv.conf
```

If DNS resolution is still unavailable, reset `/etc/resolv.conf`:

```bash
sudo rm -f /etc/resolv.conf
printf "nameserver 223.5.5.5\nnameserver 114.114.114.114\nnameserver 8.8.8.8\n" | sudo tee /etc/resolv.conf
ping -c 3 192.168.122.1
ping -c 3 8.8.8.8
getent hosts mirrors.tuna.tsinghua.edu.cn
```

The host can then connect to L2 with:

```bash
ssh ubuntu@192.168.122.11
```

## L1-L2 Shared Memory

TD partitioning gives L2 the useful property that the TDP GPA is the same as the L1 VMM GPA. In practice, L1 and L2 can map the same physical address range directly.

For this setup we use a fixed physical address mapping instead of ivshmem. Some TD-partitioning L2 QEMU configurations reject an additional `memory-backend-file` for ivshmem with errors such as:

```text
memmap size doesn't match with file backend!
```

### 1. Reserve Memory in L1

Check the L1 kernel command line:

```bash
cat /proc/cmdline
```

For example:

```text
memmap=7G$4G
```

This reserves a range starting at physical address `4G`:

```text
0x100000000 - 0x2bfffffff
```

ZENO uses the end of this range for RPC shared memory:

```text
SHM_ADDR=0x2b8000000
SHM_SIZE=540M
```

### 2. Reserve the Same Range in L2

Do not add a separate `ivshmem-plain` backend or `/dev/shm/ipcshm` backend to the L2 QEMU command. Keep the main memory backend:

```bash
-object memory-backend-file,mem-path=/dev/mem,size=${MEM},share=on,id=mem0
```

Append the fixed shared-memory reservation to the L2 kernel command line:

```bash
SHM_ADDR=0x2b8000000
SHM_SIZE=540M
APPEND="root=/dev/vda4 rw console=tty0 console=ttyS0 ignore_loglevel nopat nokaslr earlyprintk=ttyS0 memmap=${SHM_SIZE}\$${SHM_ADDR}"
```

The `$` must be escaped as `\$` in a shell string. After L2 boots, check:

```bash
cat /proc/cmdline
dmesg | grep -Ei 'BIOS-e820|memmap|2b8000000|Memory:'
free -h
```

`/proc/iomem` can show zeroed ranges in a TD environment, so do not rely on it for this address check.

### 3. Build ZENO with Fixed TDX Shared Memory

Build ZENO on both sides with the same fixed shared-memory address:

```bash
cmake -B build -S src \
  -DUSE_TDX_FIXED_SHM=ON \
  -DTDX_SHM_PHYS_ADDR=0x2b8000000
cmake --build build --parallel
```

### 4. Allow PostgreSQL to Access `/dev/mem` in L1

PostgreSQL backends cannot open `/dev/mem` by default. The L1 PostgreSQL side needs:

1. the `postgres` user in the `kmem` group;
2. `CAP_SYS_RAWIO` on the PostgreSQL server binary;
3. group read-write permission on `/dev/mem`, because ZENO opens it with `O_RDWR`.

Example setup:

```bash
sudo usermod -aG kmem postgres
sudo setcap cap_sys_rawio+ep /usr/lib/postgresql/14/bin/postgres
sudo chmod 660 /dev/mem
sudo systemctl restart postgresql
```

Check the effective configuration:

```bash
id postgres
ls -l /dev/mem
getcap /usr/lib/postgresql/14/bin/postgres
PID=$(pgrep -u postgres -n postgres)
sudo cat /proc/$PID/status | grep -E 'Groups|CapEff|CapPrm|CapAmb|NoNewPrivs'
```

Expected state:

```text
postgres is in the kmem group
/dev/mem is crw-rw---- root kmem
postgres has cap_sys_rawio=ep
CapEff is nonzero
```

Common pitfalls:

- `setcap` alone is not enough if `/dev/mem` is only group-readable.
- `sudo -u postgres head -c 1 /dev/mem` is not a valid capability test, because that command does not execute the PostgreSQL binary.
- `chmod 660 /dev/mem` may be reset by udev after reboot. Reapply it before experiments or install a udev rule.
- Long term, prefer a small device driver such as `/dev/tdx_shm` that exposes only the shared-memory range instead of granting PostgreSQL access to all of `/dev/mem`.

### 5. Start the Privacy-Zone Server

Inside L2:

```bash
cd ~/ZENO
sudo ./build/tee_server
```

Root privileges are needed when the server maps `/dev/mem`.

## Protected Storage

Durable experiments still need dm-integrity and dm-crypt for the integrity-zone and privacy-zone storage devices. See `../disk_setup.md`.
