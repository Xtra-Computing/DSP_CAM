VIVADO := $(XILINX_VIVADO)/bin/vivado

$(TEMP_DIR)/rtl_cam.xo: scripts/package_kernel.tcl scripts/gen_rtl_cam_xo.tcl src/hdl/*.sv src/hdl/*.v
	mkdir -p $(TEMP_DIR)
	$(VIVADO) -mode batch -source scripts/gen_rtl_cam_xo.tcl -tclargs $(TEMP_DIR)/rtl_cam.xo cam $(TARGET) $(PLATFORM) $(XSA)

$(TEMP_DIR)/krnl_output.xo: ./src/krnl_output.cpp
	mkdir -p $(TEMP_DIR)
	v++ $(VPP_FLAGS) -t $(TARGET) --platform $(PLATFORM) -c -k krnl_output -I'$(<D)' -o'$@' '$<' 

$(TEMP_DIR)/krnl_input.xo: ./src/krnl_input.cpp
	mkdir -p $(TEMP_DIR)
	v++ $(VPP_FLAGS) -t $(TARGET) --platform $(PLATFORM) -c -k krnl_input -I'$(<D)' -o'$@' '$<' 

$(TEMP_DIR)/post_router.xo: ./src/post_router.cpp
	mkdir -p $(TEMP_DIR)
	v++ $(VPP_FLAGS) -t $(TARGET) --platform $(PLATFORM) -c -k post_router -I'$(<D)' -o'$@' '$<' 

$(TEMP_DIR)/router_tc.xo: ./src/router_tc.cpp
	mkdir -p $(TEMP_DIR)
	v++ $(VPP_FLAGS) -t $(TARGET) --platform $(PLATFORM) -c -k router_tc -I'$(<D)' -o'$@' '$<' 