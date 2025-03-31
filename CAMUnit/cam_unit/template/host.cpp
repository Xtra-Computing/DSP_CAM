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

    //setting routing table + reset cam (the reset is included in the SET_ROUTING_TABLE)
    buffer_in_instr_map[0].range(6, 3) = SET_ROUTING_TABLE;
    buffer_in_instr_map[1].range(6, 3) = UPDATE_DUPLICATE; //UPDATE_DUPLICATE
    buffer_in_instr_map[1].range(2, 0) = 5; // set parallelism to 16
    buffer_in_instr_map[2].range(6, 3) = UPDATE_DUPLICATE; //update all
    buffer_in_instr_map[2].range(2, 0) = 5; // set parallelism to 16
    buffer_in_instr_map[3].range(6, 3) = UPDATE_DUPLICATE; //update all
    buffer_in_instr_map[3].range(2, 0) = 5; // set parallelism to 16
    buffer_in_instr_map[4].range(6, 3) = UPDATE_DUPLICATE; //update all
    buffer_in_instr_map[4].range(2, 0) = 5; // set parallelism to 16
    buffer_in_instr_map[5].range(6, 3) = UPDATE_DUPLICATE; //update all
    buffer_in_instr_map[5].range(2, 0) = 5; // set parallelism to 16
    buffer_in_instr_map[6].range(6, 3) = UPDATE_DUPLICATE; //update all
    buffer_in_instr_map[6].range(2, 0) = 5; // set parallelism to 16
    buffer_in_instr_map[7].range(6, 3) = UPDATE_DUPLICATE; //update all
    buffer_in_instr_map[7].range(2, 0) = 5; // set parallelism to 16
    buffer_in_instr_map[8].range(6, 3) = UPDATE_DUPLICATE; //update all
    buffer_in_instr_map[8].range(2, 0) = 5; // set parallelism to 16
    buffer_in_instr_map[9].range(6, 3) = SEARCH_MQ; //search one
    buffer_in_instr_map[10].range(6, 3) = SEARCH_MQ; //search one
    buffer_in_instr_map[11].range(6, 3) = SEARCH_MQ; //search one
    buffer_in_instr_map[12].range(6, 3) = RESET_CAM; //reset cam
    buffer_in_instr_map[13].range(6, 3) = SEARCH_MQ; //search one
    buffer_in_instr_map[14].range(6, 3) = SEARCH_MQ; //search one
    buffer_in_instr_map[15].range(6, 3) = SEARCH_MQ; //search one

    // set the routing table;
    const int base[16] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
    for (int i = 0; i < 16; i++) {
        buffer_in_data_map[0].range(32*i+31, 32*i) = base[i];
    }
    
    // set the update number in CAM;
    for (int i = 1; i < 9; i++) {
        for (int j = 0; j < 16; j++) {
            buffer_in_data_map[i].range(32*j+31, 32*j) = 16*(i-1)+j; // set 0 - 127 in CAM
        }
    }

    // set the search number in CAM;
    for (int i = 9; i <= 11; i++) {
        for (int j = 0; j < 16; j++) {
            buffer_in_data_map[i].range(32*j+31, 32*j) = 16*(i-9)+j;
        }
    }

    for (int i = 13; i <= 15; i++) {
        for (int j = 0; j < 16; j++) {
            buffer_in_data_map[i].range(32*j+31, 32*j) = 16*(i-13)+j + 1;
        }
    }

    std::cout << "Copying input data to device global memory" << std::endl;
    buffer_in_instr.sync(XCL_BO_SYNC_BO_TO_DEVICE);
    buffer_in_data.sync(XCL_BO_SYNC_BO_TO_DEVICE);

    xrt::run read_run;
    xrt::run write_run;
    read_run = xrt::run(krnl_input);
    write_run = xrt::run(krnl_output);

    write_run.set_arg(0, buffer_out);
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
    std::cout << "match results = " << buffer_out_map[0] << std::endl;
    int match = (buffer_out_map[0] == 48) ? 0 : 1;
    std::cout << "TEST " << (match ? "FAILED" : "PASSED") << std::endl;

    std::cout << "\nResult Analysis (48):\n"
          << "---------------------\n"
          << "Breakdown:\n"
          << "- First 3 multi-query operations: 16 results each (16 CAM blocks active)\n"
          << "- Next 3 operations: 0 results each (CAM was reset)\n"
          << "- Total = (3 × 16) + (3 × 0) = 48\n\n"
          << "Key Points:\n"
          << "1. 16-way parallelism from CAM blocks\n"
          << "2. First batch shows full capacity output\n"
          << "3. Second batch shows reset state (0s)\n"
          << "4. Final sum aggregates all operation outputs"
          << std::endl;
          
    return (match ? EXIT_FAILURE : EXIT_SUCCESS); 
}
