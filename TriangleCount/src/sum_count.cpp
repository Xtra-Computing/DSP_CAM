#include "tc_header.h"

extern "C" {
void sum_count(ap_uint<32>* mem, 
               hls::stream<ap_uint<520>>& block_in_1,
               hls::stream<ap_uint<520>>& block_in_2,
               hls::stream<ap_uint<520>>& block_in_3,
               hls::stream<ap_uint<520>>& block_in_4,
               hls::stream<ap_uint<520>>& block_in_5,
               hls::stream<ap_uint<520>>& block_in_6,
               hls::stream<ap_uint<520>>& block_in_7,
               hls::stream<ap_uint<520>>& block_in_8,
               hls::stream<ap_uint<520>>& block_in_9,
               hls::stream<ap_uint<520>>& block_in_10,
               hls::stream<ap_uint<520>>& block_in_11,
               hls::stream<ap_uint<520>>& block_in_12,
               hls::stream<ap_uint<520>>& block_in_13,
               hls::stream<ap_uint<520>>& block_in_14,
               hls::stream<ap_uint<520>>& block_in_15,
               hls::stream<ap_uint<520>>& block_in_16) {
#pragma HLS INTERFACE m_axi port=mem offset=slave bundle=gmem
#pragma HLS INTERFACE axis port=block_in_1
#pragma HLS INTERFACE axis port=block_in_2
#pragma HLS INTERFACE axis port=block_in_3
#pragma HLS INTERFACE axis port=block_in_4
#pragma HLS INTERFACE axis port=block_in_5
#pragma HLS INTERFACE axis port=block_in_6
#pragma HLS INTERFACE axis port=block_in_7
#pragma HLS INTERFACE axis port=block_in_8
#pragma HLS INTERFACE axis port=block_in_9
#pragma HLS INTERFACE axis port=block_in_10
#pragma HLS INTERFACE axis port=block_in_11
#pragma HLS INTERFACE axis port=block_in_12
#pragma HLS INTERFACE axis port=block_in_13
#pragma HLS INTERFACE axis port=block_in_14
#pragma HLS INTERFACE axis port=block_in_15
#pragma HLS INTERFACE axis port=block_in_16
#pragma HLS INTERFACE s_axilite port=mem bundle=control
#pragma HLS INTERFACE s_axilite port=return bundle=control

    static ap_uint<32> triangle_count[16] = {0};
#pragma HLS ARRAY_PARTITION variable=triangle_count complete dim=1

    for (int i = 0; i < 16; i++) { // add initialization
#pragma HLS UNROLL
        triangle_count[i] = 0;
    }

    hls::stream<ap_uint<520>>* streams[16] = {
        &block_in_1, &block_in_2, &block_in_3, &block_in_4,
        &block_in_5, &block_in_6, &block_in_7, &block_in_8,
        &block_in_9, &block_in_10, &block_in_11, &block_in_12,
        &block_in_13, &block_in_14, &block_in_15, &block_in_16
    };

    bool done[16] = {false};
#pragma HLS ARRAY_PARTITION variable=done complete dim=1

    sum_count_loop: while (true) {
    #pragma HLS pipeline II=1
        // Process each stream
        for (int i = 0; i < 16; i++) {
#pragma HLS UNROLL
            if ((!done[i]) && (!streams[i]->empty())) {
                ap_uint<520> v = streams[i]->read();

                if (v.range(519, 516) == END_OF_STREAM) {
                    done[i] = true;
                    continue;
                }

                if ((v.range(519, 516) == SEARCH_MQ) && (v.range(0, 0) == 1)) {
                    triangle_count[i]++;
                }
            }
        }

        // Assign the all_done signal
        bool all_done = (done[0] && done[1] && done[2] && done[3] &&
                         done[4] && done[5] && done[6] && done[7] &&
                         done[8] && done[9] && done[10] && done[11] &&
                         done[12] && done[13] && done[14] && done[15]);

        if (all_done) {
            break;
        }
    }

    // Sum up the triangle_count values and write to mem[0]
    ap_uint<32> total_count = 0;
    for (int i = 0; i < 16; i++) {
#pragma HLS UNROLL
        total_count += triangle_count[i];
    }

    mem[0] = total_count;
}
}