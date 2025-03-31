#include <ap_axi_sdata.h>
#include <ap_int.h>
#include <hls_stream.h>

#define DATA_WIDTH CUSTOMIZED_BUS_WIDTH

extern "C" {
void krnl_input(ap_uint<8>* in_instr, ap_uint<(DATA_WIDTH-8)>* in_data, int size, hls::stream<ap_uint<DATA_WIDTH>>& inStream1) {
    ap_uint<DATA_WIDTH> data;
    mem_rd:
        for (int i = 0; i < size; i++)
        {
            data.range((DATA_WIDTH-1), (DATA_WIDTH-8)) = in_instr[i];
            data.range((DATA_WIDTH-8)-1, 0) = in_data[i];
            inStream1 << data;
        }
}
}
