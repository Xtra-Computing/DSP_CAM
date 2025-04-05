# Configurable DSP-Based CAM on FPGAs

## Overview

This project implements a **Configurable DSP-Based Content Addressable Memory (CAM) Architecture** tailored for FPGA platforms. The architecture emphasizes **scalability**, **efficiency**, and **multi-query support**, optimized for applications in **graph analytics** and **databases**.

---

## Features

- **Hierarchical Design**:
  - **CAM Cell**: Utilizes FPGA DSP slices for efficient storage and comparison operations.
  - **CAM Block**: Supports rapid search and update operations across multiple CAM cells.
  - **CAM Unit**: Manages multiple CAM blocks, facilitating larger data spaces and concurrent searches.
- **Scalability**: Highly parameterizable design to maximize DSP resource utilization.
- **Efficiency**: Optimized search and update performance.
- **Multi-query Support**: Enables concurrent searches, maximizing throughput and memory efficiency.
- **Real-world Applications**: Ideal for graph-based analytics, notably triangle counting.

---

## Project Structure

Project structure

```
DSP_CAM/
├── CAMUnit/                 # The CAM unit design and CAM block design, together with the testing project files
├── TriangleCount/           # The triangle counting application with our CAM unit and user-defined kernels
├── common/                  # The common folder for a Vitis project
├── figure/                  # Repository figures
├── CONTRIBUTING.md
└── README.md
```

The project comprises two main parts:

1. **CAM Unit Customization** (`CAMUnit`): Contains `cam_block` and `cam_unit` designs.
   - Each folder includes:
     - **`param.cfg`**: Configuration file for architectural parameters.
     - **`configure.sh`**: Script to auto-generate source files based on provided parameters.
     - **`template`**: Templates for module code generation.

2. **Triangle Counting Accelerator** (`TriangleCount`): Application built using the CAM Unit.

---

## Getting Started

### Step 1: Configure Parameters

Edit `param.cfg` to define CAM architecture settings:

```cfg
CUSTOMIZED_BUS_WIDTH=520
CUSTOMIZED_CAM_SIZE=128
CUSTOMIZED_ALUMODE=4'b0100
CUSTOMIZED_OPMODE=9'b000110011
CUSTOMIZED_MASK=48'h0
CUSTOMIZED_ENCODE_SCHEME=PRIORITY_ENCODE
```

Then run the configuration script:

```bash
./configure.sh
```

### Step 2: Compilation

Set up your environment:

```bash
source /opt/xilinx/xrt/setup.sh
source /YOUR_PATH/Vitis/2022.2/settings64.sh  # Update with your Vitis path
```

Compile the project using `make`:

```bash
make all TARGET=<hw_emu/hw> PLATFORM=<platform_path>
```

**Example:**

```bash
make all TARGET=hw_emu PLATFORM=/opt/xilinx/platforms/xilinx_u250_gen3x16_xdma_3_1_202020_1/xilinx_u250_gen3x16_xdma_3_1_202020_1.xpfm
```

### Step 3: Running the Application

Run the executable with:

```bash
./executable -x <xclbin_file>
```

**Sample Output (`cam_block`):**

```bash
Mapping input buffer
Mapping output buffer
Copying input data to device global memory
Launching RTL kernel
Copying output data to device global memory
Comparing results
i = 0 Software result = 1 Device result = 1
...
TEST PASSED
```

**Sample Output (`cam_unit`):**

```bash
...
match results = 48
TEST PASSED

Result Analysis (48):
---------------------
Breakdown:
- First 3 multi-query operations: 16 results each
- Next 3 operations: 0 results each (CAM reset)
- Total = (3 × 16) + (3 × 0) = 48
```

---
## Triangle Counting on FPGA

This example utilizes a CAM-based edge-centric algorithm for efficient triangle counting in graphs.

### Key Functionalities

- **Adjacency List Extraction**: Retrieves adjacency lists of source and destination vertices per edge.
- **Intersection Computation**: Uses CAM-based parallel search to compute intersections (triangle detection).

Source files: `./TriangleCount`

### Running the Triangle Counting Example

0. **Dataset Preparation & Environment Setup**:

You can download the datasets used in our paper directly from this [Google Drive Link](https://drive.google.com/drive/folders/10qyk-ASlPxW-PwoP_6kM_AAGgDlgJRGb?usp=drive_link). If you would like to apply our accelerator to other graph datasets, please use the dataset generation script:

```bash
python ./dataset/partition_tc.py
```
This script converts the original CSR (Compressed Sparse Row) format into the following three files (edge_list.txt, csr_col.txt, csr_row_2.txt). To ensure compatibility with the CAM architecture, the script includes a **virtual node generation** method that prevents adjacency list lengths from exceeding the CAM size. More details can be found directly in the script.

Set up the environment:
```bash
source /opt/xilinx/xrt/setup.sh
source /YOUR_PATH/Vitis/2021.2/settings64.sh  # Update with your Vitis path
```



1. **Compile the Triangle Counting Application:**

```bash
cd ./TriangleCount
make all TARGET=hw_emu PLATFORM=<platform_path>
```
We also provide the compiled bitstream and host executable (for U250 FPGA, TARGET=hw) in this [Google Drive Link](https://drive.google.com/drive/folders/10qyk-ASlPxW-PwoP_6kM_AAGgDlgJRGb?usp=drive_link).

2. **Run in Hardware Emulation Mode:**

```bash
XCL_EMULATION_MODE=hw_emu ./triangle_count -x ./triangle_count.xclbin
```

**Sample Output:**

```bash
...
Execution completed successfully!
Triangle count: 155
```

3. **Run in Hardware Mode (example for Xilinx U250):**

```bash
make TARGET=hw PLATFORM=<platform_path>

./triangle_count -x ./build_dir.hw.<platform_name>/triangle_count.xclbin -s ./dataset/as20000102
```

**Sample Hardware Output:**

```bash
...
TriangleCount completed, execution time: 422 us
Execution completed successfully!
Triangle count: 6584
```

### Performance Results

| Dataset             | Triangles    | CAM Solution (ms) | Baseline (ms) | Speedup |
|---------------------|--------------|-------------------|---------------|---------|
| facebook_combined   | 1,612,010    | 5.054             | 18.7          | 3.70x   |
| amazon0302          | 717,719      | 23.086            | 89.5          | 3.88x   |
| amazon0601          | 3,986,507    | 71.210            | 230.3         | 3.23x   |
| as20000102          | 6,584        | 0.422             | 7.4           | 17.54x  |
| cit-Patents         | 7,515,023    | 415.808           | 800           | 1.92x   |
| ca-cit-HepPh        | 195,758,685  | 1,526.05          | 5,361.1       | 3.51x   |
| roadNet-CA          | 120,676      | 62.058            | 108.8         | 1.75x   |
| roadNet-PA          | 67,150       | 34.559            | 88.7          | 2.57x   |
| roadNet-TX          | 82,869       | 42.323            | 96.8          | 2.29x   |
| soc-Slashdot0811    | 551,724      | 29.402            | 259.7         | 8.83x   |

- Baseline measurements from AMD Vitis official implementation.
- Detailed results in `CAM/TriangleCount/results/`

---

## Contributing

We welcome contributions!

- Fork this repository.
- Implement features or optimizations.
- Submit a pull request following the guidelines in [`CONTRIBUTING.md`](./CONTRIBUTING.md).

Thank you for contributing!

## 📄 Citation

If you find this work useful in your research, please consider citing our paper:

> **Yao Chen, Feng Yu, Di Wu, Weng-Fai Wong, and Bingsheng He.**  
> *Configurable DSP-Based CAM Architecture for Data-Intensive Applications on FPGAs*.  
> In Proceedings of the 62nd Design Automation Conference (DAC), 2025.

### BibTeX
```bibtex
@inproceedings{dsp_cam,
  title={Configurable DSP-Based CAM Architecture for Data-Intensive Applications on FPGAs},
  author={Yao Chen, Feng Yu, Di Wu, Bingsheng He, Weng-fai Wong},
  booktitle={Proceedings of the 62nd Design Automation Conference (DAC)},
  year={2025},
  organization={ACM/IEEE}
}

