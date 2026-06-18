# XCFI Artifact

This repository contains the artifact for **XCFI**, a cross-domain control-flow integrity mechanism for Armv8.1-M TrustZone-M systems.  
The prototype is evaluated on an **MPS3 board (AN555 FPGA image)** featuring an **Armv8.1-M–based Cortex-M85** core.

XCFI is implemented by extending the LLVM toolchain and integrating with **Zephyr** and **Trusted Firmware-M (TF-M)**. Both Secure and Non-Secure application code, as well as runtime libraries, are compiled using the modified toolchain.

---

## Citation

If you use this work or parts of it, please cite our paper as follows:

```
<!-- TODO: add citation / BibTeX entry once available -->
```

---

## Directory Description

### `XCFI-LLVM/`

This directory contains the modified LLVM toolchain used to implement XCFI.

---

### `XCFI-zephyr/`

This directory contains the Zephyr-based build environment and application code used for evaluating XCFI.

---

### `XCFI-tfm/`

This directory contains the Trusted Firmware-M (TF-M) integration for XCFI.

---

## Build and Setup

To set up the build environment, run the following scripts in the repository root:

- `install_llvm.sh` installs the modified LLVM toolchain required for XCFI.
- `zephyr_setup.sh` sets up the Zephyr development environment.

After the setup is complete, navigate to the `XCFI-zephyr/` directory.
This directory contains the script `build_with_tfm.sh`, which builds the system together with TF-M and produces three images:
- a BL2 bootloader image,
- a TF-M Secure image, and
- a Zephyr Non-Secure image.

To deploy the built images to the target board, use the `flash_with_tfm.sh` script.
This script flashes the TF-M, Secure, and Non-Secure images to the MPS3 (AN555) board in a single step.

After flashing, **reset the board** so the new image set is loaded, and open the
serial console to observe the output.

---

## Compile Modes

The build scripts take a `<mode>` argument that selects how the project (and the
XCFI runtime libraries) are instrumented. Two scripts are provided:

- `build.sh <project> <mode>` / `flash.sh` — builds and flashes a **single
  Zephyr image** (no TF-M).
- `build_with_tfm.sh <project> <mode>` / `flash_with_tfm.sh` — builds and
  flashes the **full TF-M image set** (BL2 + Secure + Non-Secure).

The same project is built in different modes:

| Mode       | Branch protection            | XCFI       | Purpose                                              |
| ---------- | ---------------------------- | ---------- | ---------------------------------------------------- |
| `base`     | none                         | off        | Unprotected baseline                                 |
| `xcfi`     | `bti+pac-ret`                | all        | Full XCFI (forward + backward + cross-domain)        |

---

## Claims and Expected Results

This section walks through three examples — a BEEBS benchmark, a TF-M
integration sample, and a ret2ns security test — and the serial output you
should expect for each. All examples are built in the `xcfi` mode (full XCFI
protection); the `*** XCFI enabled ***` line at boot confirms the instrumentation
is active.

### Working with three terminals

The MPS3 (AN555) board exposes two serial ports: `/dev/ttyUSB0` is the board's
configuration/management console (used to reboot it after flashing), and
`/dev/ttyUSB1` carries the application output. We therefore use three terminals:

- **Terminal 1 — build & flash.** Run the build and flash scripts from the
  `XCFI-zephyr/` directory.
- **Terminal 2 — board console (`/dev/ttyUSB0`).** Connect with
  `picocom -b 115200 /dev/ttyUSB0` and issue `reboot` after each flash to load
  the new image.
- **Terminal 3 — application output (`/dev/ttyUSB1`).** Connect with
  `picocom -b 115200 /dev/ttyUSB1` to observe the running example.

In each claim below, Terminal 1 shows the build/flash commands and Terminal 3
shows the expected output. After flashing in Terminal 1, switch to Terminal 2
and run `reboot`.

### Claim 1 — Benchmark execution (BEEBS / bubblesort)

This runs one BEEBS workload (`bubblesort`), which reports its cycle count over
the serial console. BEEBS is a standalone image, so it uses `build.sh` /
`flash.sh`.

Terminal 1:

```
$ ./build.sh benchmark/beebs/bubblesort xcfi
$ ./flash.sh
```

Terminal 2 (after flashing):

```
$ picocom -b 115200 /dev/ttyUSB0
> reboot
```

Terminal 3 — expected output:

```
$ picocom -b 115200 /dev/ttyUSB1
*** Booting Zephyr OS build 804db5a226a5 ***
*** XCFI enabled ***
bubblesort cycle count: 404402225
```

### Claim 2 — TF-M integration example (tfm_ipc)

This runs the `tfm_ipc` sample, a full TF-M workload that uses the PSA IPC and
PSA Crypto services across the Secure/Non-Secure boundary. It uses the TF-M
build/flash scripts.

Terminal 1:

```
$ ./build_with_tfm.sh samples/tfm_integration/tfm_ipc xcfi
$ ./flash_with_tfm.sh
```

Terminal 2 (after flashing):

```
$ picocom -b 115200 /dev/ttyUSB0
> reboot
```

Terminal 3 — expected output:

```
$ picocom -b 115200 /dev/ttyUSB1
[INF] Starting bootloader
[WRN] This device was provisioned with dummy keys. This device is NOT SECURE
[INF] PSA Crypto init done, sig_type: EC-P256, using builtin keys
[INF] Image index: 1, Swap type: none
[INF] Image index: 0, Swap type: none
[INF] Bootloader chainload address offset: 0x0
[INF] Image version: v0.0.0
[INF] Jumping to the first image slot
Booting TF-M v2.2.1+g804db5a22
[WRN] This device was provisioned with dummy keys. This device is NOT SECURE
[Sec Thread] Secure image initializing!
TF-M isolation level is: 0x00000001
[INF][PS] Encryption alg: 0x5500200
[INF][Crypto] Provision entropy seed...
[INF][Crypto] Provision entropy seed... complete.
[DBG][Crypto] Init Mbed TLS 3.6.4...
[DBG][Crypto] Internal heap size is 12288 bytes
[DBG][Crypto] Init Mbed TLS 3.6.4... complete.
*** Booting Zephyr OS build 804db5a226a5 ***
*** XCFI enabled ***
TF-M IPC on mps3
The version of the PSA Framework API is 257.
The PSA Crypto service minor version is 1.
Generating 256 bytes of random data:
D0 B1 33 CC D9 2E FA 40 41 16 09 7E BB A4 96 49
...
39 D5 6C 63 80 8B 0B CD C4 21 A4 62 ED 0B 92 0A
tfm_ipc cycle count : 8228042
```

The full boot chain (BL2 → TF-M Secure → Zephyr Non-Secure) completes and the
PSA services return their results, confirming that XCFI does not interfere with
legitimate cross-domain control flow.

### Claim 3 — Security evaluation (ret2ns, forward-edge)

This runs the `ret2ns` sample, which mounts a ret2ns-style control-flow attack
against TF-M's Non-Secure-Callable veneers. We use the **attack 2** variant, a
BLXNS-style forward-edge attack that tries to redirect control across the Secure
boundary. See
[`samples/tfm_integration/ret2ns/README.rst`](XCFI-zephyr/samples/tfm_integration/ret2ns/README.rst)
for how the attack variant is selected.

Terminal 1:

```
$ ./build_with_tfm.sh samples/tfm_integration/ret2ns xcfi
$ ./flash_with_tfm.sh
```

Terminal 2 (after flashing):

```
$ picocom -b 115200 /dev/ttyUSB0
> reboot
```

Terminal 3 — expected output:

```
$ picocom -b 115200 /dev/ttyUSB1
[INF] Starting bootloader
[WRN] This device was provisioned with dummy keys. This device is NOT SECURE
[INF] PSA Crypto init done, sig_type: EC-P256, using builtin keys
[INF] Image index: 1, Swap type: none
[INF] Image index: 0, Swap type: none
[INF] Bootloader chainload address offset: 0x0
[INF] Image version: v0.0.0
[INF] Jumping to the first image slot
Booting TF-M v2.2.1+g804db5a22
[WRN] This device was provisioned with dummy keys. This device is NOT SECURE
[Sec Thread] Secure image initializing!
TF-M isolation level is: 0x00000001
[INF][PS] Encryption alg: 0x5500200
[INF][Crypto] Provision entropy seed...
[INF][Crypto] Provision entropy seed... complete.
[DBG][Crypto] Init Mbed TLS 3.6.4...
[DBG][Crypto] Internal heap size is 12288 bytes
[DBG][Crypto] Init Mbed TLS 3.6.4... complete.
*** Booting Zephyr OS build 804db5a226a5 ***
*** XCFI enabled ***
secure state
            msg ptr : 0x2102311c
                                msg size : 4
                                            [XCFI] Control-flow violation detected in Secure world!
```

XCFI detects the control-flow violation and stops the attack. (In `base` mode
the attack instead succeeds and `attacker controlled` is printed.)
