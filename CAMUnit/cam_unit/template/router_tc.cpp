#include <ap_axi_sdata.h>
#include <ap_int.h>
#include <hls_stream.h>
#include <iostream>
#include <iomanip>
#include <string>


#define IDLE                0x0
#define UPDATE_ALL          0x1
#define UPDATE_GROUP        0x2
#define UPDATE_ONE          0x3
#define SEARCH_ONE          0x4
#define SEARCH_MQ           0x5
#define SET_ROUTING_TABLE   0x6
#define RESET_CAM           0x7
#define UPDATE_DUPLICATE    0x8

#define END_OF_STREAM       0xf 

#define BLOCK_NUM CUSTOMIZED_BLOCK_NUM
#define decode_instruction(x)   (ap_int<520>(x).range(518, 515))
#define get_parallelism(x)      (ap_int<520>(x).range(514, 512))

void PrintHexOutput(const std::string& label, const ap_uint<16>& dest, const ap_uint<512>& data) {
    // Print label, dest, and data in one line
    std::cout << label << " dest: 0x" << std::hex << std::setw(4) << std::setfill('0') << dest.to_uint() << " data: ";
    for (int i = 0; i < 16; ++i) {
        if (i > 0) std::cout << " "; // Add space between values
        std::cout << "0x" << std::hex << std::setw(8) << std::setfill('0') << data.range(32 * i + 31, 32 * i).to_uint();
    }
    std::cout << std::endl; // End the line
}

ap_uint<16> block_lookup_table(ap_uint<3> encoded_parallelism, ap_uint<32> index) {
    #pragma HLS INLINE
    const ap_uint<16> dest_lut[5][16] = {
        {0x0001, 0x0002, 0x0004, 0x0008, 0x0010, 0x0020, 0x0040, 0x0080, 0x0100, 0x0200, 0x0400, 0x0800, 0x1000, 0x2000, 0x4000, 0x8000}, // Parallelism = 1
        {0x0101, 0x0202, 0x0404, 0x0808, 0x1010, 0x2020, 0x4040, 0x8080, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000}, // Parallelism = 2
        {0x1111, 0x2222, 0x4444, 0x8888, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000}, // Parallelism = 4
        {0x5555, 0xAAAA, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000}, // Parallelism = 8
        {0xFFFF, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000}  // Parallelism = 16
    };
    // ap_uint<4> group_id = (index / 8) % 16; // 8 means 8 512bits for 128 int data;
    ap_uint<4> group_id = (index >> 3) & 0xF; // 8 means 8 512bits for 128 int data;   
    return dest_lut[encoded_parallelism - 1][group_id]; 
    // -1 because the encoded_parallelism is 1-5, but the index is 0-4.
}

extern "C" {
void router_tc(hls::stream<ap_axiu<520, 0, 0, 0>>& tc_stream_in,
            hls::stream<ap_axiu<520, 0, 0, BLOCK_NUM>>& router_out) {
#pragma HLS interface ap_ctrl_none port = return
#pragma HLS interface axis port = tc_stream_in
#pragma HLS interface axis port = router_out

    ap_axiu<520, 0, 0, 0> in_packet;
    ap_axiu<520, 0, 0, BLOCK_NUM> out_packet;
    static int routing_table[BLOCK_NUM];
    #pragma HLS ARRAY_PARTITION variable=routing_table complete dim=1

    int index = 0; // accumulate the number of packets.

    router_tc_loop: while (true) {
        #pragma HLS PIPELINE II = 1
        in_packet = tc_stream_in.read();
        if (in_packet.last) break;

        if (decode_instruction(in_packet.data) == SET_ROUTING_TABLE) {
            for (int i = 0; i < BLOCK_NUM; i++) {
            #pragma HLS unroll  
                routing_table[i] = in_packet.data.range(32 * i + 31, 32 * i);
            }
            out_packet.dest = (1 << (BLOCK_NUM + 1)) - 1; // set all bits to 1
            out_packet.last = 0;
            out_packet.data.range(518,515) = RESET_CAM;
            out_packet.data.range(511,0) = 0;
            router_out << out_packet;
            
        } else if (decode_instruction(in_packet.data) == UPDATE_DUPLICATE) {
            out_packet.dest = block_lookup_table(get_parallelism(in_packet.data), index);
            // Perform UPDATE_DUPLICATE
            out_packet.last = 0;
            out_packet.data = in_packet.data;
            router_out << out_packet;
            index++;
            // PrintHexOutput("UPDATE_DUPLICATE", out_packet.dest, out_packet.data);

        } else if (decode_instruction(in_packet.data) == SEARCH_MQ) {
            // Perform SEARCH_MQ
            out_packet.dest = (1 << (BLOCK_NUM + 1)) - 1; // set all bits to 1
            for (int i = 0; i < BLOCK_NUM; i++) {
            #pragma HLS unroll
                int idx = routing_table[i];
                out_packet.data.range(32 * (i + 1) - 1, 32 * i) = in_packet.data.range(32 * (idx + 1) - 1, 32 * idx);
            }
            out_packet.last = 0;
            out_packet.data.range(519,512) = in_packet.data.range(519,512); // update the instruction.
            router_out << out_packet;
            // PrintHexOutput("SEARCH_MQ", out_packet.dest, out_packet.data);
        } else if (decode_instruction(in_packet.data) == RESET_CAM) {
            // Perform RESET_CAM
            out_packet.dest = (1 << (BLOCK_NUM + 1)) - 1; // set all bits to 1
            out_packet.last = 0;
            out_packet.data = in_packet.data;
            router_out << out_packet;
            // PrintHexOutput("RESET_CAM", out_packet.dest, out_packet.data);
        }

        if (!(decode_instruction(in_packet.data) == UPDATE_DUPLICATE)) {
            index = 0;
        }
    }
    
    out_packet.last = 1;
    out_packet.data = 0x0;
    router_out << out_packet;
}
}
