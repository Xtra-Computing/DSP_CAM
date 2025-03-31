// Add a constraint: the block number is less than 16.
#include <ap_axi_sdata.h>
#include <ap_int.h>
#include <hls_stream.h>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <string>

#define STREAM_LENGTH 520 // 512 + 8, which 512 is the data width and 8 is the control flag
#define get_valid_flag(x)       (ap_int<STREAM_LENGTH>(x).range(519, 519)) // valid flag, reserved, 1 bit
#define decode_instruction(x)   (ap_int<STREAM_LENGTH>(x).range(518, 515)) // decode the instruction, 4 bits
#define get_parallelism(x)      (ap_int<STREAM_LENGTH>(x).range(514, 512)) // get the parallelism, 3 bits
#define get_data(x)             (ap_int<STREAM_LENGTH>(x).range(511, 0)) // get the data, 512 bits

///// CAM PARAMETERS /////
#define CUSTOMIZED_BLOCK_NUM  16
#define CUSTOMIZED_BLOCK_SIZE 128

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

///// DATA /////
#define DUMMY_DATA_UD          0x80000000  // dummay data for update_duplicate
#define DUMMY_DATA_MQ          0x80000001  // dummay data for search_mq
// Define DEBUG_MODE to enable debug outputs; comment it to disable.
// #define DEBUG_MODE

#ifdef DEBUG_MODE
    #define DEBUG(...) DebugHelper(__VA_ARGS__)
    #define DEBUG_HEX(...) DebugHelperHex(__VA_ARGS__)
#else
    #define DEBUG(...)
    #define DEBUG_HEX(...)
#endif

// Debug helper function for normal output
template<typename... Args>
void DebugHelper(Args&&... args) {
    std::ostringstream oss;
    (oss << ... << std::forward<Args>(args)); // Fold expression for variadic arguments
    std::cout << oss.str() << std::endl;
}


