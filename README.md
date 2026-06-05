# ZENO

This repository contains the artifact for the following paper:

**Paper title:** *Accelerating Confidential Databases with Crypto-free Mappings*

**Authors:** Wenxuan Huang, Zhanbo Wang, and Mingyu Li

**Affiliations:** Key Laboratory of System Software, Chinese Academy of Sciences; Institute of Software, Chinese Academy of Sciences; University of Chinese Academy of Sciences

**Venue:** OSDI 2026

**Paper link:** To be added after publication

## 1. Overview

ZENO is a confidential database system that accelerates secure query processing over sensitive data in untrusted clouds. Many industry-deployed confidential databases use a split architecture where sensitive operators run inside TEEs, but each DBMS/TEE crossing still requires frequent synchronous encryption and decryption, causing high computational, memory, and I/O overhead.

ZENO introduces **crypto-free mappings**: the DBMS stores compact, data-independent field identifiers (FIDs), while the trusted domain securely maps FIDs to plaintext secrets. This decouples **indirection** from **protection**, removing cryptographic operations from the query-execution critical path while preserving the DBA maintainability enabled by the split architecture.

This artifact includes:

- ZENO source code;
- PostgreSQL integration patches (including the commit protocol) and configuration files;
- TPC-C and TPC-H benchmark scripts.

ZENO's protected-operator implementation builds on HEDB's operator framework; see <https://github.com/SJTU-IPADS/HEDB>.

The industrial workload is not included because it is derived from proprietary commercial workloads.

## 2. Repository Contents

The repository is organized as follows. Only the main artifact-facing directories are shown:

```text
.
├── src/                        # ZENO core source code
│   ├── include/                # Header files for core components
│   ├── integrity_zone/         # DBMS-side integrity zone components
│   │   └── postgresql-patches/ # PostgreSQL patches for ZENO integration
│   ├── privacy_zone/           # Privacy zone components for protected operators
│   └── utils/                  # Utility functions
├── benchmark/                  # Benchmark scripts and tools
│   ├── tpcc/                   # TPC-C benchmark
│   └── tpch/                   # TPC-H benchmark
├── test/                       # SQL test cases
├── tools/                      # Utility and analysis tools
│   └── drivers/                # Kernel drivers used by VM deployments
└── doc/                        # Documentation
```

The main directories and their purposes are:

- `src/`: Core ZENO implementation comprising integrity-zone and privacy-zone components, along with utility functions and CMake build configuration.

- `benchmark/`: Benchmark infrastructure for TPC-C and TPC-H workloads, including data generation and test implementations.

- `test/`: SQL test cases.

- `tools/`: Development utilities for VM-based deployments, including kernel-driver setup for shared memory.

- `doc/`: Documentation for advanced deployments and configurations.

## 3. Quick Start

This quick start runs ZENO in a **single-machine, dual-process simulation mode**. This mode is intended to check that the artifact builds correctly.
For full dual-VM deployments, please refer to:

- `doc/vm-setup-aarch64.md`
- `doc/vm-setup-x86_64.md`

### 3.1 Environment

The artifact has been tested on:

```text
OS:            Ubuntu 24.04
Kernel:        Linux 6.8.0
PostgreSQL:    PostgreSQL 14.22
Python:        Python 3.12.3
```

Install dependencies:

```bash
sudo apt update
sudo apt install -y build-essential cmake git pkg-config
sudo apt install -y libmbedtls-dev libboost-all-dev
sudo apt install -y postgresql postgresql-client postgresql-contrib postgresql-server-dev-all
sudo apt install -y pgtap libtap-parser-sourcehandler-pgtap-perl sysbench
sudo apt install -y python3 python3-pip
sudo apt install -y python3-tqdm python3-psycopg2
```

Start PostgreSQL and set the default password for the postgres user:

```bash
sudo service postgresql restart
sudo -u postgres psql -c "ALTER USER postgres WITH PASSWORD 'postgres';"
```

### 3.2 Build

Build and install ZENO from the `ZENO` directory:

Before building, choose where the privacy-zone mapping files are stored. By default, ZENO uses `/tmp/kvmap_` as `KV_MMAP_FILE_PREFIX` in `src/privacy_zone/include/file-map-operator.hpp`. For a full protected-storage setup, update this prefix to the encrypted storage mount point before running `make build`; see `doc/disk_setup.md`.

```bash
rm -rf build
make build
sudo make install
```

### 3.3 Start the Runtime

Start the privacy-zone server in the background:

```bash
make run
```

This creates `/dev/shm/ipcshm` and starts `build/tee_server`. To stop it:

```bash
make stop
```

### 3.4 Smoke Test

Keep `tee_server` running in a separate terminal, then run the test:

```bash
sudo -u postgres psql
```

```sql
DROP EXTENSION IF EXISTS kvdb CASCADE;
CREATE EXTENSION kvdb;

DROP TABLE IF EXISTS test;
CREATE TABLE test (i kv_int4, f kv_float4, s kv_text, t kv_timestamp);

INSERT INTO test VALUES ('1'::kv_int4, '1.1'::kv_float4, 'ZENO'::kv_text, '2026-05-06'::kv_timestamp);
INSERT INTO test VALUES ('2'::kv_int4, '3.1'::kv_float4, 'OSDI'::kv_text, '2026-07-13'::kv_timestamp);

SELECT * FROM test;
```

You can also run the smoke-test SQL script:

```bash
cat test/test.sql | sudo -u postgres psql
```

ZENO currently exposes four opaque PostgreSQL types for selectively protected data:

| Plain type | ZENO type      |
|------------|----------------|
| int        | `kv_int4`      |
| float      | `kv_float4`    |
| text       | `kv_text`      |
| timestamp  | `kv_timestamp` |

The quick start is only a demo configuration. A full security setup also requires protected storage; see `doc/disk_setup.md` and the VM setup documents.

## 4. Running TPC-H and TPC-C

This artifact includes scripts for running the public TPC-H and TPC-C benchmarks used in the paper. The scripts below are entry points for small-scale runs.

### 4.1 TPC-H

Configure the TPC-H runner:

```bash
cd benchmark/tpch
vi tpch-config.json
```

Then generate data, load it, and run the configured queries:

```bash
python3 run.py -l
python3 run.py -sg
```

Use `python3 run.py -h` to list all TPC-H runner options.

### 4.2 TPC-C

Prepare a small TPC-C database:

```bash
cd benchmark/tpcc/zeno
cat init.sql | sudo -u postgres psql
./tpcc.lua --pgsql-user=postgres --pgsql-password=postgres --pgsql-db=test_kvdb \
    --time=30 --threads=4 --report-interval=1 --tables=1 --scale=4 \
    --db-driver=pgsql prepare
```

Run a small TPC-C experiment:

```bash
./tpcc.lua --pgsql-user=postgres --pgsql-password=postgres --pgsql-db=test_kvdb \
    --time=30 --threads=4 --report-interval=1 --tables=1 --scale=4 \
    --db-driver=pgsql run
```

### 4.3 Notes on Differences from the Paper

The commands above are intended to exercise the benchmark infrastructure and ZENO implementation. Reproducing the full paper-scale results may require:

- the same CPU and memory configuration as described in the paper;
- the same TEE or dual-VM deployment mode;
- larger TPC-C and TPC-H datasets;
- longer benchmark durations;
- isolated machine access to reduce performance variance.

Paper-scale runs also require a self-built PostgreSQL with ZENO's PostgreSQL patches applied. The patches are provided in `src/integrity_zone/postgresql-patches/`.

The industrial workload is not included in this artifact because it is derived from proprietary commercial workloads.

This artifact also uses PostgreSQL triggers to migrate newly inserted protected fields from the temporal partition to the persistent partition described in the paper. This trigger-based path is convenient for configuring protected columns in benchmark schemas, but it is not the fully rigorous placement of the protocol boundary. A stricter implementation should drive the migration from the PostgreSQL patch at the point where it sends the WAL materialize request; see `src/integrity_zone/postgresql-patches/pg14-commit-protocol.patch`. Only then is the new DBMS state known to have become durable. The trigger implementation is therefore kept as a practical configuration mechanism rather than as the precise durability boundary used in the paper design.

## 5. Contact

For artifact-related questions, please contact:

- Wenxuan Huang: <https://github.com/xiunianjun>
- Zhanbo Wang: <https://github.com/wangerforcs>
- Mingyu Li: <https://github.com/Maxul>


Issues and pull requests can also be opened on this repository.
