/*******************************************************************************
Description:
    Host code for simplified kernel operation: triangle_count.
*******************************************************************************/

#include "cmdlineparser.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstring>
#include <ap_int.h>
#include <iomanip>
#include <chrono>
// XRT includes
#include "experimental/xrt_bo.h"
#include "experimental/xrt_device.h"
#include "experimental/xrt_kernel.h"

#define UPDATE_ALL 0xffffff01
#define SEARCH_MQ 0xffffff04

#define CUSTOMIZED_BLOCK_NUM 16
#define CUSTOMIZED_BLOCK_SIZE 128


void readCSRAndEdgeList(const std::string &csr_col_file, const std::string &csr_row_file, 
                        const std::string &edge_list_file,
                        std::vector<int> &col_idx, std::vector<int> &row_ptr, 
                        std::vector<int> &edge_list) {
    // Read CSR col_idx
    std::ifstream col_file(csr_col_file);
    int val;
    while (col_file >> val) {
        col_idx.push_back(val);
    }
    col_file.close();

    // Read CSR row_ptr
    std::ifstream row_file(csr_row_file);
    while (row_file >> val) {
        row_ptr.push_back(val);
    }
    row_file.close();

    // Read edge list (as flat vector: src, dst pairs)
    std::ifstream edge_file(edge_list_file);
    int src, dst;
    while (edge_file >> src >> dst) {
        edge_list.push_back(src);
        edge_list.push_back(dst);
    }
    edge_file.close();
}


int main(int argc, char** argv) {
    // Command Line Parser
    sda::utils::CmdLineParser parser;
    parser.addSwitch("--xclbin_file", "-x", "input binary file string", "");
    parser.addSwitch("--device_id", "-d", "device index", "0");
    parser.addSwitch("--dataset", "-s", "Dataset name (default: test)", "./dataset/test");
    parser.parse(argc, argv);

    std::string binaryFile = parser.value("xclbin_file");
    int device_index = stoi(parser.value("device_id"));
    std::string dataset = parser.value("dataset");

    if (argc < 3) {
        parser.printHelp();
        return EXIT_FAILURE;
    }

    std::cout << "Open the device" << device_index << std::endl;
    auto device = xrt::device(device_index);
    std::cout << "Load the xclbin " << binaryFile << std::endl;
    auto uuid = device.load_xclbin(binaryFile);
    std::cout << "Kernel UUID: " << uuid << std::endl;

    // File paths
    std::string csr_col_file = dataset + "_csr_col.txt";
    std::string csr_row_file = dataset + "_csr_row_2.txt";
    std::string edge_list_file = dataset + "_edgelist.txt";

    std::vector<int> col_idx, row_ptr, edge_list;
    readCSRAndEdgeList(csr_col_file, csr_row_file, edge_list_file, col_idx, row_ptr, edge_list);
    // Print lengths
    std::cout << "col_idx length: " << col_idx.size() << std::endl;
    std::cout << "row_ptr length: " << row_ptr.size() << std::endl;
    std::cout << "edge_list length: " << edge_list.size() / 2 << " edges (" << edge_list.size() << " elements)" << std::endl;

    // Load kernels
    xrt::kernel tc_kernel = xrt::kernel(device, uuid, "triangle_count");
    xrt::kernel count_kernel = xrt::kernel(device, uuid, "sum_count");
    
    // Allocate XRT buffers
    auto bo_col = xrt::bo(device, col_idx.size() * sizeof(int), tc_kernel.group_id(3));
    auto bo_row = xrt::bo(device, row_ptr.size() * sizeof(int), tc_kernel.group_id(1));
    auto bo_edge = xrt::bo(device, edge_list.size() * sizeof(int), tc_kernel.group_id(0));
    auto bo_count = xrt::bo(device, sizeof(int), count_kernel.group_id(0));

    auto col_idx_ptr = bo_col.map<int*>();
    auto row_ptr_ptr = bo_row.map<int*>();
    auto edge_list_ptr = bo_edge.map<int*>();
    auto count_ptr = bo_count.map<int*>();

    // Copy data to device
    std::cout << "Copying data to device..." << std::endl;
    std::memcpy(col_idx_ptr, col_idx.data(), col_idx.size() * sizeof(int));
    std::memcpy(row_ptr_ptr, row_ptr.data(), row_ptr.size() * sizeof(int));
    std::memcpy(edge_list_ptr, edge_list.data(), edge_list.size() * sizeof(int));
    std::memset(count_ptr, 0, sizeof(int));

    bo_col.sync(XCL_BO_SYNC_BO_TO_DEVICE);
    bo_row.sync(XCL_BO_SYNC_BO_TO_DEVICE);
    bo_edge.sync(XCL_BO_SYNC_BO_TO_DEVICE);
    bo_count.sync(XCL_BO_SYNC_BO_TO_DEVICE);

    std::cout << "Launching kernels..." << std::endl;
    xrt::run count_run = xrt::run(count_kernel);
    count_run.set_arg(0, bo_count);

    xrt::run tc_run = xrt::run(tc_kernel);
    tc_run.set_arg(0, bo_edge);
    tc_run.set_arg(1, bo_row);
    tc_run.set_arg(2, bo_row);
    tc_run.set_arg(3, bo_col);
    tc_run.set_arg(4, bo_col);
    tc_run.set_arg(5, (edge_list.size() / 2));


    // add time counter
    auto start_time = std::chrono::high_resolution_clock::now();
    count_run.start();
    tc_run.start();

    // Wait for completion
    tc_run.wait();
    count_run.wait();
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();
    std::cout << "TriangleCount completed, execution time: " << duration << " us" << std::endl;

    // Copy results from device
    std::cout << "Copying results from device..." << std::endl;
    bo_count.sync(XCL_BO_SYNC_BO_FROM_DEVICE);

    // Print results
    uint32_t tc_count = 0;
    tc_count = count_ptr[0];

    std::cout << "Execution completed successfully!" << std::endl;
    std::cout << "Triangle count: " << tc_count << std::endl;

    return EXIT_SUCCESS;
}
