# Third-Party Licenses

The root `LICENSE` applies to ZENO's own source code unless a file or directory carries a different license notice.

This repository also includes third-party components:

- Parts of ZENO's protected-operator implementation are derived from HEDB's operator framework. HEDB is available at <https://github.com/SJTU-IPADS/HEDB>.
- `benchmark/tpcc/plain/` and `benchmark/tpcc/zeno/`: sysbench/TPC-C Lua scripts derived from Percona code, licensed under the GNU General Public License version 2 or later as stated in their file headers.
- `tools/drivers/ivshmem-driver/uio.c`: Linux UIO driver code, licensed under GPL-2.0 as stated by its SPDX header.
- `tools/drivers/ivshmem-driver/uio-ivshmem.c` and `tools/drivers/arm-pmu-driver/pmu_el0_cycle_counter.c`: Linux kernel modules, licensed under GPL as stated by their module license declarations.
- `src/cmake/FindPostgreSQL.cmake`: CMake PostgreSQL discovery module, licensed under the Apache License, Version 2.0 as stated in its file header.

For these components, the license notice in the individual file or upstream component controls.
