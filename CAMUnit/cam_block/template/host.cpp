#include "xcl2.hpp"
#include <vector>
#include <ap_int.h>
#include <iostream>
#include <cstring>
#include <thread>
#include <chrono>
#include "experimental/xrt-next.h"
#include "experimental/xrt_bo.h"
#include "experimental/xrt_device.h"
#include "experimental/xrt_kernel.h"
#include "cmdlineparser.h"

///// INSTRUCTIONS /////
#define IDLE                0x0
#define UPDATE_ALL          0x1
#define UPDATE_GROUP        0x2
#define UPDATE_ONE          0x3
#define SEARCH_ONE          0x4
#define SEARCH_MQ           0x5
#define SET_ROUTING_TABLE   0x6
#define RESET_CAM           0x7
#define UPDATE_DUPLICATE    0x8
#define END_OF_STREAM       0xf  // end of stream

int main(int argc, char** argv) {
    // Command Line Parser
    sda::utils::CmdLineParser parser;

    parser.addSwitch("--xclbin_file", "-x", "input binary file string", "");
    parser.addSwitch("--device_id", "-d", "device index", "0");
    parser.parse(argc, argv);

    std::string binaryFile = parser.value("xclbin_file");
    int device_index = stoi(parser.value("device_id"));

    std::cout << "Open the device" << device_index << std::endl;
    auto device = xrt::device(device_index);
    std::cout << "Load the xclbin " << binaryFile << std::endl;
    auto uuid = device.load_xclbin(binaryFile); 

    xrt::kernel krnl_input;
    xrt::kernel krnl_output;
    krnl_input = xrt::kernel(device, uuid, "krnl_input");
    krnl_output = xrt::kernel(device, uuid, "krnl_output");

    const int size = 16; // In this example, we send 16 instructions
    xrt::bo buffer_in_instr = xrt::bo(device, size, krnl_input.group_id(0));  // 8 bits = 1 bytes. then we have 16 ap_uint<8> data in.
    xrt::bo buffer_in_data = xrt::bo(device, size * 64, krnl_input.group_id(1)); // 512 bits = 64 bytes. then we have 16 ap_uint<512> data in.
    xrt::bo buffer_out = xrt::bo(device, size * 4, krnl_output.group_id(0)); // output 6 search results

    std::cout << "Mapping input buffer" << std::endl;
    auto buffer_in_instr_map = buffer_in_instr.map<ap_uint<8>*>();
    auto buffer_in_data_map = buffer_in_data.map<ap_uint<512>*>();
    std::cout << "Mapping output buffer" << std::endl;
    auto buffer_out_map = buffer_out.map<ap_uint<32>*>();

    std::fill(buffer_in_instr_map, buffer_in_instr_map + size, 0);
    std::fill(buffer_in_data_map, buffer_in_data_map + size, 0);
    std::fill(buffer_out_map, buffer_out_map + size, 0);

    // RESET_CAM + UPDATE_ALL * 8 + SEARCH_ONE * 3 + RESET_CAM + SEARCH_ONE * 3
    // This example targets to first update 128 data into CAM, then do search, after that we reset the CAM and then do search again.
    buffer_in_instr_map[0].range(6, 3) = RESET_CAM; //reset cam
    buffer_in_instr_map[1].range(6, 3) = UPDATE_ALL; //update all
    buffer_in_instr_map[2].range(6, 3) = UPDATE_ALL; //update all
    buffer_in_instr_map[3].range(6, 3) = UPDATE_ALL; //update all
    buffer_in_instr_map[4].range(6, 3) = UPDATE_ALL; //update all
    buffer_in_instr_map[5].range(6, 3) = UPDATE_ALL; //update all
    buffer_in_instr_map[6].range(6, 3) = UPDATE_ALL; //update all
    buffer_in_instr_map[7].range(6, 3) = UPDATE_ALL; //update all
    buffer_in_instr_map[8].range(6, 3) = UPDATE_ALL; //update all
    buffer_in_instr_map[9].range(6, 3) = SEARCH_MQ; //search one
    buffer_in_instr_map[10].range(6, 3) = SEARCH_MQ; //search one
    buffer_in_instr_map[11].range(6, 3) = SEARCH_MQ; //search one
    buffer_in_instr_map[12].range(6, 3) = RESET_CAM; //reset cam
    buffer_in_instr_map[13].range(6, 3) = SEARCH_MQ; //search one
    buffer_in_instr_map[14].range(6, 3) = SEARCH_MQ; //search one
    buffer_in_instr_map[15].range(6, 3) = END_OF_STREAM; //search one

    // set the update number in CAM;
    for (int i = 1; i < 9; i++) {
        for (int j = 0; j < 16; j++) {
            buffer_in_data_map[i].range(32*j+31, 32*j) = 16*(i-1)+j; // set 0 - 127 in CAM
        }
    }

    // set the search number in CAM;
    buffer_in_data_map[9].range(31, 0) = 16;
    buffer_in_data_map[10].range(31, 0) = 32;
    buffer_in_data_map[11].range(31, 0) = 48;

    buffer_in_data_map[13].range(31, 0) = 80;
    buffer_in_data_map[14].range(31, 0) = 96;
    buffer_in_data_map[15].range(31, 0) = 112;

    // set the software result;
    ap_uint<32> buffer_out_map_sw[6];
    buffer_out_map_sw[0].range(0,0) = 1; // have the number
    buffer_out_map_sw[1].range(0,0) = 1; // have the number
    buffer_out_map_sw[2].range(0,0) = 1; // have the number
    buffer_out_map_sw[3].range(0,0) = 0; // do not have the number
    buffer_out_map_sw[4].range(0,0) = 0; // do not have the number
    buffer_out_map_sw[5].range(0,0) = 0; // do not have the number

    std::cout << "Copying input data to device global memory" << std::endl;
    buffer_in_instr.sync(XCL_BO_SYNC_BO_TO_DEVICE);
    buffer_in_data.sync(XCL_BO_SYNC_BO_TO_DEVICE);

    xrt::run read_run;
    xrt::run write_run;
    read_run = xrt::run(krnl_input);
    write_run = xrt::run(krnl_output);

    write_run.set_arg(0, buffer_out);
    write_run.set_arg(1, 6);
    write_run.start();
    read_run.set_arg(0, buffer_in_instr);
    read_run.set_arg(1, buffer_in_data);
    read_run.set_arg(2, 16);
    read_run.start();

    std::cout << "Launching RTL kernel" << std::endl;
    write_run.wait();
    read_run.wait();

    std::cout << "Copying output data to device global memory" << std::endl;
    buffer_out.sync(XCL_BO_SYNC_BO_FROM_DEVICE);


    std::cout << "Comparing results" << std::endl;
    int match = 0;
    for (int i = 0; i < 6; i++) {
        if (buffer_out_map[i].range(0,0) != buffer_out_map_sw[i].range(0,0)) {
            std::cout << "Error: Result mismatch" << std::endl;
            std::cout << "i = " << i << " Software result = " << buffer_out_map_sw[i].range(0,0)
                      << " Device result = " << buffer_out_map[i].range(0,0) << std::endl;
            match = 1;
        }
        else 
        {
            std::cout << "i = " << i << " Software result = " << buffer_out_map_sw[i].range(0,0)
                      << " Device result = " << buffer_out_map[i].range(0,0) << std::endl;
        }
    }

    std::cout << "TEST " << (match ? "FAILED" : "PASSED") << std::endl;
    return (match ? EXIT_FAILURE : EXIT_SUCCESS); 
}
