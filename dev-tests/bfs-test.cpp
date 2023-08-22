#include "blocks/c_code_generator.h"
#include "blocks/extract_cuda.h"
#include "vitis/extract_vitis.h"
#include "vitis/vitis_metadata.h"
#include "vitis/vitis_code_generator.h"
#include "builder/builder.h"
#include "builder/builder_context.h"
#include "builder/static_var.h"
#include "builder/dyn_var.h"
#include "blocks/rce.h"
#include <iostream>
#include <queue>

using builder::dyn_var;
using builder::static_var;

#define MAXDEGREE (32 * 4096)
#define MAXOUTDEGREE (10 * 4096)

#define LO4_MASK 15

static bool que_valid;
static int que_reg; // 512
static int que_addr;

static bool column_valid;
static int column_reg; // 512
static int column_addr; 

static bool result_valid;
static int result_reg; // 512
static int result_addr;

inline void loadQueue(const int vertexNum,
                      int queStatus[8],
                      int& tableStatus,
                      int* ddrQue, // 512

                      std::queue<int>& fromIDStrm, // stream
                      std::queue<bool>& ctrlStrm) { // stream
    int fromID;
    long int que_rd_cnt;
    long int que_wr_cnt;
    int que_rd_ptr;

    que_rd_cnt = (queStatus[1] << 32) + queStatus[0];
    que_wr_cnt = (queStatus[3] << 32) + queStatus[2];
    que_rd_ptr = queStatus[4];

    if (que_rd_cnt != que_wr_cnt) {
        int que_rd_ptrH = que_rd_ptr;
        // Eliminated if
        que_reg = ddrQue[que_rd_ptrH];
        que_valid = 1;
        que_addr = que_rd_ptrH;
        
        fromID = que_reg; 
        tableStatus = fromID;

        que_rd_cnt++;
        if (que_rd_ptr != vertexNum - 1)
            que_rd_ptr++;
        else
            que_rd_ptr = 0;

        fromIDStrm.push(fromID);
        ctrlStrm.push(0);
    } else {
        fromIDStrm.push(0);
        ctrlStrm.push(1);
    }

    queStatus[0] = que_rd_cnt & 0xFFFFFFFF;
    queStatus[1] = que_rd_cnt >> 32;
    queStatus[4] = que_rd_ptr;
}

inline void loadOffset(int idxoffset_s,
                       int offset_reg, // 512
                       std::queue<int>& fromIDStrm,
                       std::queue<int>& ctrlInStrm,

                       int* offset, // 512

                       std::queue<int>& offsetLowStrm,
                       std::queue<int>& offsetHighStrm,
                       std::queue<bool>& ctrlOutStrm) {

	int fromID;
    bool ctrl;

    fromID = fromIDStrm.front();
	fromIDStrm.pop();
    ctrl = ctrlInStrm.front();
	ctrlInStrm.pop();

    if (ctrl == 0) {
        int idxHcur = fromID;
        int idxHnxt = (fromID + 1);

        int offsetLow;
        int offsetHigh;
        if (idxHcur == idxoffset_s && idxHcur == idxHnxt) { // hit, and offsets in one line
            ap_uint<64> t0 = offset_reg.range(32 * (idxLcur + 2) - 1, 32 * idxLcur);
            offsetLow = t0(31, 0);
            offsetHigh = t0(63, 32);
        } else if (idxHcur == idxHnxt) { // no hit, but offsets in one line
            offset_reg = offset[idxHcur];
            idxoffset_s = idxHcur;
            ap_uint<64> t0 = offset_reg.range(32 * (idxLcur + 2) - 1, 32 * idxLcur);
            offsetLow = t0(31, 0);
            offsetHigh = t0(63, 32);
        } else { // no hit, and offsets in different line
            ap_uint<512> offset_t[2];
#pragma HLS array_partition variable = offset_t complete dim = 0
            for (int i = 0; i < 2; i++) {
#pragma HLS PIPELINE II = 1
                offset_t[i] = offset[idxHcur + i];
            }

            offset_reg = offset_t[1];
            idxoffset_s = idxHnxt;
            offsetLow = offset_t[0](511, 480);
            offsetHigh = offset_t[1](31, 0);
        }

        offsetLowStrm.write(offsetLow);
        offsetHighStrm.write(offsetHigh);
        ctrlOutStrm.write(0);
    } else {
        offsetLowStrm.write(0);
        offsetHighStrm.write(0);
        ctrlOutStrm.write(1);
    }
}

inline void loadColumnSendAddr(hls::stream<ap_uint<32> >& offsetLowStrm,
                               hls::stream<ap_uint<32> >& offsetHighStrm,
                               hls::stream<bool>& ctrlInStrm,

                               hls::stream<ap_uint<32> >& columnAddrStrm,
                               hls::stream<bool>& ctrlOutStrm) {
#pragma HLS inline off
    bool ctrl;
    ap_uint<32> offsetLow;
    ap_uint<32> offsetHigh;

    offsetLow = offsetLowStrm.read();
    offsetHigh = offsetHighStrm.read();
    ctrl = ctrlInStrm.read();

    if (ctrl == 0) {
        for (ap_uint<32> i = offsetLow; i < offsetHigh; i++) {
#pragma HLS PIPELINE II = 1
            columnAddrStrm.write(i);
            ctrlOutStrm.write(0);
        }
    }

    ctrlOutStrm.write(1);
}

inline void loadColumn(hls::stream<ap_uint<32> >& raddrStrm,
                       hls::stream<bool>& ctrlInStrm,

                       ap_uint<512>* column,

                       hls::stream<ap_uint<32> >& toIDStrm,
                       hls::stream<bool>& ctrlOutStrm) {
#pragma HLS inline off

    bool ctrl = ctrlInStrm.read();

    while (!ctrl) {
#pragma HLS PIPELINE II = 1
        ap_uint<32> raddr = raddrStrm.read();
        int idxHc = raddr.range(31, 4);
        int idxLc = raddr.range(3, 0);

        if (column_valid == 0 || idxHc != column_addr) {
            column_valid = 1;
            column_addr = idxHc;
            column_reg = column[idxHc];
        }

        toIDStrm.write(column_reg.range(32 * (idxLc + 1) - 1, 32 * idxLc));
        ctrlOutStrm.write(0);
        ctrl = ctrlInStrm.read();
    }

    ctrlOutStrm.write(1);
}

inline void loadRes(hls::stream<ap_uint<32> >& toIDStrm,
                    hls::stream<bool>& ctrlInStrm,

                    ap_uint<512>* color512,

                    hls::stream<ap_uint<32> >& toIDOutStrm,
                    hls::stream<ap_uint<32> >& colorOutStrm,
                    hls::stream<bool>& ctrlOutStrm) {
#pragma HLS inline off
    bool ctrl;

    ctrl = ctrlInStrm.read();
    while (!ctrl) {
#pragma HLS PIPELINE II = 1
        ctrl = ctrlInStrm.read();
        ap_uint<32> toID = toIDStrm.read();
        int raddrH = toID.range(31, 4);
        int raddrL = toID.range(3, 0);

        if (result_valid == 0 || raddrH != result_addr) {
            result_valid = 1;
            result_addr = raddrH;
            result_reg = color512[raddrH];
        }

        ap_uint<32> color = result_reg.range(32 * (raddrL + 1) - 1, 32 * raddrL);
        colorOutStrm.write(color);
        toIDOutStrm.write(toID);
        ctrlOutStrm.write(0);
    }

    ctrlOutStrm.write(1);
}

inline void storeUpdate(hls::stream<ap_uint<32> >& toIDStrm,
                        hls::stream<ap_uint<32> >& colorInStrm,
                        hls::stream<bool>& ctrlInStrm,

                        ap_uint<32>& tableStatus,
                        ap_uint<32>* toIDTable) {
#pragma HLS inline off
    ap_uint<32> cnt = 0;

    bool ctrl = ctrlInStrm.read();
    while (!ctrl) {
#pragma HLS PIPELINE II = 1
        ctrl = ctrlInStrm.read();
        ap_uint<32> toID = toIDStrm.read();
        ap_uint<32> color = colorInStrm.read();

        if (color == (ap_uint<32>)-1) {
            toIDTable[cnt] = toID;
            cnt++;
        }
    }

    tableStatus = cnt;
}

inline void BFSS1Dataflow(const int vertexNum,
                          int idxoffset_s,
                          int* offset_reg, // 512
                          int queStatus[8],
                          int* ddrQue, // 512

                          int* offsetG1, // 512
                          int* columnG1, // 512

                          int* color512, // 512

                          int tableStatus[6],
                          int* toIDTable) {

    std::queue<int> fromIDStrm;
    std::queue<bool> ctrlStrm0;

    std::queue<int> offsetLowStrm;
    std::queue<int> offsetHighStrm;


    std::queue<bool> ctrlStrm2;
    std::queue<int> toIDStrm1;

    std::queue<int> ctrlStrm3;
    std::queue<int> toIDStrm2;

    std::queue<bool> ctrlStrm4;
    std::queue<int> toIDStrm3;

    std::queue<bool> ctrlStrm5;
    std::queue<int> colorStrm;

    // load one node from queue
    loadQueue(vertexNum, queStatus, tableStatus[0], ddrQue, fromIDStrm, ctrlStrm0);

    // read CSR Graph offset
    loadOffset(idxoffset_s, offset_reg, fromIDStrm, ctrlStrm0, offsetG1, offsetLowStrm, offsetHighStrm, ctrlStrm2);

    // get address for read column
    loadColumnSendAddr(offsetLowStrm, offsetHighStrm, ctrlStrm2, toIDStrm1, ctrlStrm3);

    // read CSR Graph column
    loadColumn(toIDStrm1, ctrlStrm3, columnG1, toIDStrm2, ctrlStrm4);

    // load unvisited node
    loadRes(toIDStrm2, ctrlStrm4, color512, toIDStrm3, colorStrm, ctrlStrm5);

    storeUpdate(toIDStrm3, colorStrm, ctrlStrm5, tableStatus[1], toIDTable);
}

template <int MAXDEGREE>
void bfsImpl(const int srcID,
             const int vertexNum,
             int* columnG1,
             int* offsetG1,

             int* queue1,
             int* queue2,
             int* color,

             int* result_dt,
             int* result_ft,
             int* result_pt,
             int* result_lv) {

    int idxoffset_s = -1;
    int offset_reg; // perv: 512

    int visitor[2] = {1, 1};
    int queStatus[8];
    int tableStatus[6];

    int* toIDTable = (int*)malloc(MAXDEGREE * sizeof(int));


    que_valid = 0;
    column_valid = 0;
    result_valid = 0;

    tableStatus[0] = 0;
    tableStatus[1] = 0;
    tableStatus[2] = 0;
    tableStatus[3] = 1;
    tableStatus[4] = 0;
    tableStatus[5] = 0; // overflow info

    queue[0] = srcID;         // push into queue
    result_dt[srcID] = 0;     // color into visited
    result_pt[srcID] = srcID; // parent node
    result_lv[srcID] = 0;     // distance value

    queStatus[0] = 0;
    queStatus[1] = 0;
    queStatus[2] = 1;
    queStatus[3] = 0;
    queStatus[4] = 0;
    queStatus[5] = 1;
    queStatus[6] = 0;

    long int que_rd_cnt;
    long int que_wr_cnt;
    que_rd_cnt = (queStatus[1] << 32) + queStatus[0];
    que_wr_cnt = (queStatus[3] << 32) + queStatus[2];

    while (que_rd_cnt != que_wr_cnt) {
        BFSS1Dataflow(vertexNum, idxoffset_s, offset_reg, queStatus, queue1, offsetG1, columnG1, color,
                      tableStatus, toIDTable);

        BFSS2Dataflow<MAXDEGREE>(vertexNum, visitor, tableStatus, queStatus, toIDTable, queue2, result_dt, result_ft,
                                 result_pt, result_lv);

        que_rd_cnt = (queStatus[1] << 32) + queStatus[0];
        que_wr_cnt = (queStatus[3] << 32) + queStatus[2];
    }
}

inline void initBuffer(const int depth, int* result) {
    for (int i = 0; i < depth; i++) {
        result[i] = -1;
    }
}

/**
 * @brief bfs Implement the directed graph traversal by breath-first search algorithm
 *
 * @tparam MAXOUTDEGREE the maximum outdegree of input graphs. Large value will result in more URAM usage.
 *
 * @param srcID the source vertex ID for this search, starting from 0
 * @param numVertex vertex number of the input graph
 * @param indexCSR column index of CSR format
 * @param offsetCSR row offset of CSR format
 * @param color512 intermediate color map which should be shared with dtime, pred or distance in host.
 * @param queue32 intermediate queue32 used during the BFS
 * @param dtime the result of discovery time stamp for each vertex
 * @param ftime the result of finish time stamp for each vertex
 * @param pred the result of parent index of each vertex
 * @param distance the distance result from given source vertex for each vertex
 *
 */
template <int MAXOUTDEGREE>
void bfs(const int srcID,
         const int numVertex,

         int* offsetCSR,
         int* indexCSR,

         int* queue1,
         int* queue2,
         int* color,

         int* ftime,
         int* dtime,
         int* pred,
         int* distance) {
#pragma HLS inline off

    const int depth = numVertex;

    initBuffer(depth, color);

    bfsImpl<MAXOUTDEGREE>(srcID, numVertex, indexCSR, offsetCSR, queue1, queue2, color, dtime,
                                         ftime, pred, distance);
}

int main(int argc, char *argv[]) {
	builder::builder_context context;
	auto ast = context.extract_function_ast(bfs, "bfs");
	block::eliminate_redundant_vars(ast);
	block::vitis_device_mem_ports = 2;	
	block::extract_vitis_from(block::to<block::func_decl>(ast));
	ast->dump(std::cout, 0);
	std::cout << std::endl;
	vitis::hls_code_generator::generate_code(ast, std::cout, 0);
	return 0;
}
