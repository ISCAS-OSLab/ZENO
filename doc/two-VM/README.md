# Two-VM Deployment

This directory is reserved for notes about the classic two-VM split deployment, where PostgreSQL and the ZENO extension run in the integrity-zone VM and `tee_server` runs in the privacy-zone VM.

The detailed two-VM setup is derived from the HEDB deployment flow. See the upstream HEDB documentation for the general VM construction and networking steps:

```text
https://github.com/SJTU-IPADS/HEDB/tree/main/docs
```

This artifact does not include two-VM QEMU launch scripts. Those scripts are environment-specific and usually contain local disk-image paths, tap-device names, CPU pinning choices, NUMA placement, and host-only network configuration.
