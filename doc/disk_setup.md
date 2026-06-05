## Disk Provisioning

Apply dm-integrity to the integrity zone so each block carries an integrity tag, and layer dm-crypto on top of dm-integrity for the privacy zone so every block is encrypted before it is validated.

Run the following from the project root.

### Integrity-Only Disk (Integrity Zone)

```bash
mkdir -p storage  # ensure the backing directory exists
fallocate -l 50G storage/ssd_integrity.img
openssl rand -out storage/integrity-key.bin 32

sudo modprobe sha256_ssse3 || true
LOOPDEV=$(sudo losetup --find --show --direct-io=on storage/ssd_integrity.img)
echo "Using loop device: $LOOPDEV"

sudo integritysetup format  \
    "$LOOPDEV" \
    --integrity hmac-sha256 \
    --integrity-key-file storage/integrity-key.bin \
    --integrity-key-size 32 \
    --tag-size 32    \
    --sector-size 4096

sudo integritysetup open \
    "$LOOPDEV" plain-ssd \
    --integrity hmac-sha256 \
    --integrity-key-file storage/integrity-key.bin \
    --integrity-key-size 32 \
    --integrity-no-journal

# Create the filesystem and mount point
sudo mkfs.ext4 /dev/mapper/plain-ssd
sudo mkdir -p /mnt/plain-ssd
sudo mount /dev/mapper/plain-ssd /mnt/plain-ssd
```

Afterward create the database:

```bash
# Allow PostgreSQL to access
sudo mkdir /mnt/plain-ssd/postgresql
sudo chown postgres:postgres /mnt/plain-ssd
sudo chmod 700 /mnt/plain-ssd
sudo chown -R postgres:postgres /mnt/plain-ssd/postgresql

sudo chown ubuntu:ubuntu /mnt/plain-ssd/postgresql/
sudo chmod 700 /mnt/plain-ssd/
sudo chown -R postgres:postgres /mnt/plain-ssd/postgresql

sudo -u postgres /usr/lib/postgresql/14/bin/initdb -D /mnt/plain-ssd/postgresql/
```

### Encrypted + Integrity Disk (Privacy Zone)

```bash
mkdir -p storage
fallocate -l 20G storage/ssd_integrity.img
fallocate -l 2M storage/crypthdr.img
openssl rand -base64 16 > storage/luks-passphrase.txt

LOOPDEV=$(sudo losetup --find --show --direct-io=on storage/ssd_integrity.img)
echo "Using loop device: $LOOPDEV"

sudo cryptsetup luksFormat -q \
    --type luks2 \
    --integrity hmac-sha256 \
    --integrity-no-journal \
    --sector-size 4096 \
    "$LOOPDEV" storage/luks-passphrase.txt

sudo cryptsetup luksOpen \
    --perf-same_cpu_crypt \
    --perf-submit_from_crypt_cpus \
    --perf-no_read_workqueue \
    --perf-no_write_workqueue \
    --integrity-no-journal \
    "$LOOPDEV" encrypted-ssd \
    --key-file storage/luks-passphrase.txt

# Create the filesystem and mount point
sudo mkfs.ext4 /dev/mapper/encrypted-ssd
sudo mkdir -p /mnt/encrypted-ssd
sudo mount /dev/mapper/encrypted-ssd /mnt/encrypted-ssd

# KVMAP
sudo chmod 777 /mnt/encrypted-ssd/
mkdir /mnt/encrypted-ssd/kvmap
sudo chmod 777 /mnt/encrypted-ssd/kvmap
```

Afterward update the file-map path in `src/privacy_zone/include/file-map-operator.hpp`: set `KV_MMAP_FILE_PREFIX` to `/mnt/encrypted-ssd/kvmap/kvmap_` before building ZENO.
