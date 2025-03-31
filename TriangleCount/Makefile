#
# Copyright 2019-2021 Xilinx, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
# makefile-generator v1.0.3
#

############################## Help Section ##############################
.PHONY: help

help::
	$(ECHO) "Makefile Usage:"
	$(ECHO) "  make all TARGET=<sw_emu/hw_emu/hw> PLATFORM=<FPGA platform>"
	$(ECHO) "      Command to generate the design for specified Target and Shell."
	$(ECHO) ""
	$(ECHO) "  make clean "
	$(ECHO) "      Command to remove the generated non-hardware files."
	$(ECHO) ""
	$(ECHO) "  make cleanall"
	$(ECHO) "      Command to remove all the generated files."
	$(ECHO) ""
	$(ECHO) "  make test PLATFORM=<FPGA platform>"
	$(ECHO) "      Command to run the application. This is same as 'run' target but does not have any makefile dependency."
	$(ECHO) ""
	$(ECHO) "  make run TARGET=<sw_emu/hw_emu/hw> PLATFORM=<FPGA platform>"
	$(ECHO) "      Command to run application in emulation."
	$(ECHO) ""
	$(ECHO) "  make build TARGET=<sw_emu/hw_emu/hw> PLATFORM=<FPGA platform>"
	$(ECHO) "      Command to build xclbin application."
	$(ECHO) ""
	$(ECHO) "  make host"
	$(ECHO) "      Command to build host application."
	$(ECHO) ""

############################## Setting up Project Variables ##############################
# # Points to top directory of Git repository
# MK_PATH := $(abspath $(lastword $(MAKEFILE_LIST)))
# COMMON_REPO ?= $(shell bash -c 'export MK_PATH=$(MK_PATH); echo $${MK_PATH%host_xrt/streaming_free_running_k2k_xrt/*}')
# PWD = $(shell readlink -f .)
# XF_PROJ_ROOT = $(shell readlink -f $(COMMON_REPO))

TARGET := hw
HOST_ARCH := x86
SYSROOT := 
FREQ := 280 ## default frequency = 250 MHz

include ./utils.mk

XSA := 
ifneq ($(PLATFORM), )
XSA := $(call device2xsa, $(PLATFORM))
endif
TEMP_DIR := ./_x.$(TARGET).$(XSA)
BUILD_DIR := ./build_dir.$(TARGET).$(XSA)

# SoC variables
RUN_APP_SCRIPT = ./run_app.sh
PACKAGE_OUT = ./package.$(TARGET)

LAUNCH_EMULATOR = $(PACKAGE_OUT)/launch_$(TARGET).sh
RESULT_STRING = TEST PASSED

VPP := v++
VIVADO := $(XILINX_VIVADO)/bin/vivado
VPP_PFLAGS := 
CMD_ARGS = -x $(BUILD_DIR)/triangle_count.xclbin

include ../common/includes/opencl/opencl.mk
CXXFLAGS += $(opencl_CXXFLAGS) -Wall -O0 -g -std=c++1y
LDFLAGS += $(opencl_LDFLAGS)


########################## Checking if PLATFORM in whitelist #######################
PLATFORM_BLOCKLIST += 2018 qep aws-vu9p-f1 samsung vck zc u2_ nodma 
############################## Setting up Host Variables ##############################
#Include Required Host Source Files
CXXFLAGS += -I../common/includes/cmdparser
CXXFLAGS += -I../common/includes/logger
CXXFLAGS += -I/tools/Xilinx/Vitis_HLS/2022.2/include
HOST_SRCS += ../common/includes/cmdparser/cmdlineparser.cpp ../common/includes/logger/logger.cpp ./src/host.cpp 
# Host compiler global settings
CXXFLAGS += -fmessage-length=0
LDFLAGS += -lrt -lstdc++ 
LDFLAGS += -luuid -lxrt_coreutil

############################## Setting up Kernel Variables ##############################
# Kernel compiler global settings
VPP_FLAGS += -t $(TARGET) --platform $(PLATFORM) --save-temps --kernel_frequency $(FREQ)
ifneq ($(TARGET), hw)
	VPP_FLAGS += -g
endif


# Kernel linker flags
VPP_LDFLAGS_triangle_count += --config ./tc.cfg

EXECUTABLE = ./triangle_count
EMCONFIG_DIR = $(TEMP_DIR)

############################## Declaring Binary Containers ##############################
BINARY_CONTAINERS += $(BUILD_DIR)/triangle_count.xclbin
BINARY_CONTAINER_triangle_count_OBJS += $(TEMP_DIR)/rtl_cam.xo
BINARY_CONTAINER_triangle_count_OBJS += $(TEMP_DIR)/triangle_count.xo
BINARY_CONTAINER_triangle_count_OBJS += $(TEMP_DIR)/router_tc.xo
BINARY_CONTAINER_triangle_count_OBJS += $(TEMP_DIR)/post_router.xo
BINARY_CONTAINER_triangle_count_OBJS += $(TEMP_DIR)/sum_count.xo
############################## Setting Targets ##############################
CP = cp -rf

.PHONY: all clean cleanall docs emconfig
all: check-platform check-device check-vitis $(EXECUTABLE) $(BINARY_CONTAINERS) emconfig

.PHONY: host
host: $(EXECUTABLE)

.PHONY: build
build: check-vitis check-device $(BINARY_CONTAINERS)

.PHONY: xclbin
xclbin: build

############################## Setting Rules for Binary Containers (Building Kernels) ##############################
$(TEMP_DIR)/rtl_cam.xo: scripts/package_kernel.tcl scripts/gen_rtl_cam_xo.tcl src/hdl/*.sv src/hdl/*.v
	mkdir -p $(TEMP_DIR)
	$(VIVADO) -mode batch -source scripts/gen_rtl_cam_xo.tcl -tclargs $(TEMP_DIR)/rtl_cam.xo cam $(TARGET) $(PLATFORM) $(XSA)
$(TEMP_DIR)/post_router.xo: src/post_router.cpp
	mkdir -p $(TEMP_DIR)
	$(VPP) $(VPP_FLAGS) -c -k post_router --temp_dir $(TEMP_DIR)  -I'$(<D)' -o'$@' '$<'
$(TEMP_DIR)/triangle_count.xo: src/triangle_count.cpp
	mkdir -p $(TEMP_DIR)
	$(VPP) $(VPP_FLAGS) -c -k triangle_count --temp_dir $(TEMP_DIR)  -I'$(<D)' -o'$@' '$<'
$(TEMP_DIR)/router_tc.xo: src/router_tc.cpp
	mkdir -p $(TEMP_DIR)
	$(VPP) $(VPP_FLAGS) -c -k router_tc --temp_dir $(TEMP_DIR)  -I'$(<D)' -o'$@' '$<'
$(TEMP_DIR)/sum_count.xo: src/sum_count.cpp
	mkdir -p $(TEMP_DIR)
	$(VPP) $(VPP_FLAGS) -c -k sum_count --temp_dir $(TEMP_DIR)  -I'$(<D)' -o'$@' '$<'
$(BUILD_DIR)/triangle_count.xclbin: $(BINARY_CONTAINER_triangle_count_OBJS)
	mkdir -p $(BUILD_DIR)
	$(VPP) $(VPP_FLAGS) -l $(VPP_LDFLAGS) --temp_dir $(TEMP_DIR) $(VPP_LDFLAGS_triangle_count) -o'$(BUILD_DIR)/triangle_count.link.xclbin' $(+)
	$(VPP) -p $(BUILD_DIR)/triangle_count.link.xclbin -t $(TARGET) --platform $(PLATFORM) --package.out_dir $(PACKAGE_OUT) -o $(BUILD_DIR)/triangle_count.xclbin

############################## Setting Rules for Host (Building Host Executable) ##############################
$(EXECUTABLE): $(HOST_SRCS) | check-xrt
		$(CXX) -o $@ $^ $(CXXFLAGS) $(LDFLAGS)

emconfig:$(EMCONFIG_DIR)/emconfig.json
$(EMCONFIG_DIR)/emconfig.json:
	emconfigutil --platform $(PLATFORM) --od $(EMCONFIG_DIR)

############################## Setting Essential Checks and Running Rules ##############################
run: all
ifeq ($(TARGET),$(filter $(TARGET),sw_emu hw_emu))
	$(CP) $(EMCONFIG_DIR)/emconfig.json .
	XCL_EMULATION_MODE=$(TARGET) $(EXECUTABLE) $(CMD_ARGS)
else
	$(EXECUTABLE) $(CMD_ARGS)
endif


.PHONY: test
test: $(EXECUTABLE)
ifeq ($(TARGET),$(filter $(TARGET),sw_emu hw_emu))
	XCL_EMULATION_MODE=$(TARGET) $(EXECUTABLE) $(CMD_ARGS)
else
	$(EXECUTABLE) $(CMD_ARGS)
endif


############################## Cleaning Rules ##############################
# Cleaning stuff
clean:
	-$(RMDIR) $(EXECUTABLE) $(XCLBIN)/{*sw_emu*,*hw_emu*} 
	-$(RMDIR) profile_* TempConfig system_estimate.xtxt *.rpt *.csv 
	-$(RMDIR) src/*.ll *v++* .Xil emconfig.json dltmp* xmltmp* *.log *.jou *.wcfg *.wdb

cleanall: clean
	-$(RMDIR) build_dir*
	-$(RMDIR) package.*
	-$(RMDIR) _x* *xclbin.run_summary qemu-memory-_* emulation _vimage pl* start_simulation.sh *.xclbin

