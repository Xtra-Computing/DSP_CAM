#include <ap_axi_sdata.h>
#include <ap_int.h>
#include <hls_stream.h>

#define DATA_WIDTH CUSTOMIZED_BUS_WIDTH

extern "C" {
void krnl_output(ap_uint<32>* out, int size, hls::stream<ap_uint<DATA_WIDTH>>& outStream1) {
    ap_uint<DATA_WIDTH> data;
    mem_wr:
        for (int i = 0; i < size; i++)
        {
            data = outStream1.read();
            out[i] = data.range(31, 0);
        }
}
}
