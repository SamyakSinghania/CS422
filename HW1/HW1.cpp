/*
 * Copyright (C) 2007-2023 Intel Corporation.
 * SPDX-License-Identifier: MIT
 */

/*! @file
 *  This is an example of the PIN tool that demonstrates some basic PIN APIs 
 *  and could serve as the starting point for developing your first PIN tool
 */

#include "pin.H"
#include <iostream>
#include <fstream>
#include <unordered_set>
#include <map>
#include <algorithm>
#include <limits>
using std::cerr;
using std::endl;
using std::string;

/* ================================================================== */
// Global variables
/* ================================================================== */

UINT64 insCount    = 0; //number of dynamically executed instructions
UINT64 bblCount    = 0; //number of dynamically executed basic blocks
UINT64 threadCount = 0; //total number of threads, including main thread

static UINT64 icount = 0;
static UINT64 fast_forward_count = 0;
static UINT64 g_loads = 0;
static UINT64 g_stores = 0;
static UINT64 g_nops = 0;
static UINT64 g_direct_calls = 0;
static UINT64 g_indirect_calls = 0;
static UINT64 g_returns = 0;
static UINT64 g_unconditional_branches = 0;
static UINT64 g_conditional_branches = 0;
static UINT64 g_logical_operations = 0;
static UINT64 g_rotate_shift = 0;
static UINT64 g_flag_operations = 0;
static UINT64 g_vector_instructions = 0;
static UINT64 g_conditional_moves = 0;
static UINT64 g_mmx_sse = 0;
static UINT64 g_system_calls = 0;
static UINT64 g_floating_point = 0;
static UINT64 g_others = 0;
static UINT64 cycle_latency = 0;

// Part D statistics
static std::map<UINT32, UINT64> insLengthDist;        // 1. Instruction length distribution
static std::map<UINT32, UINT64> operandCountDist;     // 2. Operand count distribution
static std::map<UINT32, UINT64> regReadDist;          // 3. Register read distribution
static std::map<UINT32, UINT64> regWriteDist;         // 4. Register write distribution
static std::map<UINT32, UINT64> memOpDist;            // 5. Memory operand distribution
static std::map<UINT32, UINT64> memReadDist;          // 6. Memory read distribution
static std::map<UINT32, UINT64> memWriteDist;         // 7. Memory write distribution

// 8. Memory bytes statistics
static UINT64 maxMemBytes = 0;
static UINT64 totalMemBytes = 0;
static UINT64 memInstCount = 0;

// 9. Immediate value statistics
static INT32 maxImm = INT32_MIN;
static INT32 minImm = INT32_MAX;

// 10. Displacement statistics
static ADDRDELTA maxDisp = std::numeric_limits<ADDRDELTA>::min();
static ADDRDELTA minDisp = std::numeric_limits<ADDRDELTA>::max();

std::ostream* out = &cerr;

// Global unordered sets to store unique 32-byte chunks for footprint measurement
std::unordered_set<ADDRINT> insChunks;
std::unordered_set<ADDRINT> dataChunks;

/* ===================================================================== */
// Command line switches
/* ===================================================================== */
KNOB< string > KnobOutputFile(KNOB_MODE_WRITEONCE, "pintool", "o", "", "specify file name for MyPinTool output");

KNOB< BOOL > KnobCount(KNOB_MODE_WRITEONCE, "pintool", "count", "1",
                       "count instructions, basic blocks and threads in the application");

KNOB<UINT64> KnobFastForward(KNOB_MODE_WRITEONCE, "pintool", 
    "f","0", "FastForward Instructions");

/* ===================================================================== */
// Utilities
/* ===================================================================== */

/*!
 *  Print out help message.
 */
INT32 Usage()
{
    cerr << "This tool prints out the number of dynamically executed " << endl
         << "instructions, basic blocks and threads in the application." << endl
         << endl;

    cerr << KNOB_BASE::StringKnobSummary() << endl;

    return -1;
}

/* ===================================================================== */
// Analysis routines
/* ===================================================================== */

/*!
 * Increase counter of the executed basic blocks and instructions.
 * This function is called for every basic block when it is about to be executed.
 * @param[in]   numInstInBbl    number of instructions in the basic block
 * @note use atomic operations for multi-threaded applications
 */
VOID CountBbl(UINT32 numInstInBbl)
{
    bblCount++;
    insCount += numInstInBbl;
}

// Analysis routine to track number of instructions executed, and check the exit condition
VOID InsCount()	{
	icount++;
}

ADDRINT Terminate(void)
{
        return (icount >= fast_forward_count + 1000000000);
}

// Analysis routine to check fast-forward condition
ADDRINT FastForward(void) {
	return (icount >= fast_forward_count && icount);
}

/*!
 * Record instruction footprint.
 * This analysis routine is called for every instruction (all instructions are counted regardless of predicate).
 * It calculates which 32-byte chunks this instruction touches and adds them to a global set.
 */
inline VOID RecordInsFootprint(ADDRINT addr, UINT32 size)
{
    ADDRINT start = addr;
    ADDRINT end = addr + size - 1;
    // Compute the starting chunk (aligned to 32 bytes)
    ADDRINT chunk = start - (start % 32);
    // If the instruction spans more than one chunk then record all touched chunks.
    while (chunk <= end)
    {
        insChunks.insert(chunk);
        chunk += 32;
    }
}

/*!
 * Record data footprint.
 * This analysis routine is called for each memory access (load or store) with true predicate.
 * It calculates which 32-byte chunks the memory access touches.
 */
inline VOID RecordDataFootprint(ADDRINT ea, UINT32 size)
{
    ADDRINT start = ea;
    ADDRINT end = ea + size - 1;
    // Compute the starting chunk (aligned to 32 bytes)
    ADDRINT chunk = start - (start % 32);
    while (chunk <= end)
    {
        dataChunks.insert(chunk);
        chunk += 32;
    }
}

// Analysis routine to exit the application
VOID MyExitRoutine()
{
    UINT64 total_executed = g_loads + g_stores + g_nops + g_direct_calls + 
                           g_indirect_calls + g_returns + g_unconditional_branches + 
                           g_conditional_branches + g_logical_operations + 
                           g_rotate_shift + g_flag_operations + g_vector_instructions + 
                           g_conditional_moves + g_mmx_sse + g_system_calls + 
                           g_floating_point + g_others;

    *out << "===============================================\n";
    *out << "Instruction Type Results: \n";
    *out << "Loads: " << g_loads << " (" << (float)g_loads/total_executed << ")\n";
    *out << "Stores: " << g_stores << " (" << (float)g_stores/total_executed << ")\n";
    *out << "NOPs: " << g_nops << " (" << (float)g_nops/total_executed << ")\n";
    *out << "Direct calls: " << g_direct_calls << " (" << (float)g_direct_calls/total_executed << ")\n";
    *out << "Indirect calls: " << g_indirect_calls << " (" << (float)g_indirect_calls/total_executed << ")\n";
    *out << "Returns: " << g_returns << " (" << (float)g_returns/total_executed << ")\n";
    *out << "Unconditional branches: " << g_unconditional_branches << " (" << (float)g_unconditional_branches/total_executed << ")\n";
    *out << "Conditional branches: " << g_conditional_branches << " (" << (float)g_conditional_branches/total_executed << ")\n";
    *out << "Logical operations: " << g_logical_operations << " (" << (float)g_logical_operations/total_executed << ")\n";
    *out << "Rotate and Shift: " << g_rotate_shift << " (" << (float)g_rotate_shift/total_executed << ")\n";
    *out << "Flag operations: " << g_flag_operations << " (" << (float)g_flag_operations/total_executed << ")\n";
    *out << "Vector instructions: " << g_vector_instructions << " (" << (float)g_vector_instructions/total_executed << ")\n";
    *out << "Conditional moves: " << g_conditional_moves << " (" << (float)g_conditional_moves/total_executed << ")\n";
    *out << "MMX and SSE instructions: " << g_mmx_sse << " (" << (float)g_mmx_sse/total_executed << ")\n";
    *out << "System calls: " << g_system_calls << " (" << (float)g_system_calls/total_executed << ")\n";
    *out << "Floating point instructions: " << g_floating_point << " (" << (float)g_floating_point/total_executed << ")\n";
    *out << "The rest: " << g_others << " (" << (float)g_others/total_executed << ")\n";
    *out << "CPI: " << (float)cycle_latency/total_executed << "\n\n";

    *out << "Instruction Size Results: \n";
    for(int i = 0; i <= 19; i++) {
        *out << i << " : " << (insLengthDist.count(i) ? insLengthDist[i] : 0) << "\n";
    }
    *out << "\n";

    *out << "Memory Instruction Operand Results: \n";
    for(int i = 0; i <= 4; i++) {
        *out << i << " : " << (memOpDist.count(i) ? memOpDist[i] : 0) << "\n";
    }
    *out << "\n";

    *out << "Memory Instruction Read Operand Results: \n";
    for(int i = 0; i <= 4; i++) {
        *out << i << " : " << (memReadDist.count(i) ? memReadDist[i] : 0) << "\n";
    }
    *out << "\n";

    *out << "Memory Instruction Write Operand Results: \n";
    for(int i = 0; i <= 4; i++) {
        *out << i << " : " << (memWriteDist.count(i) ? memWriteDist[i] : 0) << "\n";
    }
    *out << "\n";

    *out << "Instruction Operand Results: \n";
    for(int i = 0; i <= 9; i++) {
        *out << i << " : " << (operandCountDist.count(i) ? operandCountDist[i] : 0) << "\n";
    }
    *out << "\n";

    *out << "Instruction Register Read Operand Results: \n";
    for(int i = 0; i <= 9; i++) {
        *out << i << " : " << (regReadDist.count(i) ? regReadDist[i] : 0) << "\n";
    }
    *out << "\n";

    *out << "Instruction Register Write Operand Results: \n";
    for(int i = 0; i <= 9; i++) {
        *out << i << " : " << (regWriteDist.count(i) ? regWriteDist[i] : 0) << "\n";
    }
    *out << "\n";

    *out << "Instruction Blocks Accesses : " << insChunks.size() << "\n";
    *out << "Memory Blocks Accesses : " << dataChunks.size() << "\n";
    *out << "Maximum number of bytes touched by an instruction : " << maxMemBytes << "\n";
    *out << "Average number of bytes touched by an instruction : " << (memInstCount ? (double)totalMemBytes/memInstCount : 0) << "\n";
    *out << "Maximum value of immediate : " << maxImm << "\n";
    *out << "Minimum value of immediate : " << minImm << "\n";
    *out << "Maximum value of displacement used in memory addressing : " << maxDisp << "\n";
    *out << "Minimum value of displacement used in memory addressing : " << minDisp << "\n";
    *out << "===============================================\n";

    // Final flush before closing
    if (out != &cerr)
    {
        *out << std::flush;  // One final flush before closing
        static_cast<std::ofstream*>(out)->close();
    }
    exit(0);
}

// Predicated analysis routine
void MyPredicatedAnalysis() {
	// analysis code
}

// Make all increment functions inline
inline void inc_load(UINT32 val){
    g_loads += val;
    cycle_latency += val*70;
}
inline void inc_store(UINT32 val){
    g_stores += val;
    cycle_latency += val*70;
}
inline void inc_nop(){
    g_nops++;
    cycle_latency++;
}
inline void inc_direct_call(){
    g_direct_calls++;
    cycle_latency++;
}
inline void inc_indirect_call(){
    g_indirect_calls++;
    cycle_latency++;
}
inline void inc_return(){
    g_returns++;
    cycle_latency++;
}
inline void inc_unconditional_branch(){
    g_unconditional_branches++;
    cycle_latency++;
}
inline void inc_conditional_branch(){
    g_conditional_branches++;
    cycle_latency++;
}
inline void inc_logical_operation(){
    g_logical_operations++;
    cycle_latency++;
}
inline void inc_rotate_shift(){
    g_rotate_shift++;
    cycle_latency++;
}
inline void inc_flag_operation(){
    g_flag_operations++;
    cycle_latency++;
}
inline void inc_vector_instruction(){
    g_vector_instructions++;
    cycle_latency++;
}
inline void inc_conditional_move(){
    g_conditional_moves++;
    cycle_latency++;
}
inline void inc_mmx_sse(){
    g_mmx_sse++;
    cycle_latency++;
}
inline void inc_system_call(){
    g_system_calls++;
    cycle_latency++;
}
inline void inc_floating_point(){
    g_floating_point++;
    cycle_latency++;
}
inline void inc_other(){
    g_others++;
    cycle_latency++;
}

// Make analysis functions inline
inline VOID UpdateImmediateStats(ADDRINT imm) {
    INT32 signedImm = (INT32)imm;
    maxImm = std::max(maxImm, signedImm);
    minImm = std::min(minImm, signedImm);
}

inline VOID UpdateDisplacementStats(ADDRINT disp) {
    ADDRDELTA signedDisp = (ADDRDELTA)disp;
    maxDisp = std::max(maxDisp, signedDisp);
    minDisp = std::min(minDisp, signedDisp);
}

inline VOID UpdateMemOpDist(UINT32 reads, UINT32 writes) {
    memOpDist[reads + writes]++;
}

inline VOID UpdateInstructionStats(UINT32 size, UINT32 opCount, UINT32 readRegs, UINT32 writeRegs) {
    insLengthDist[size]++;
    operandCountDist[opCount]++;
    regReadDist[readRegs]++;
    regWriteDist[writeRegs]++;
}

inline VOID UpdateMemoryAnalysis(UINT32 memReads, UINT32 memWrites, UINT32 totalBytes) {
    memReadDist[memReads]++;
    memWriteDist[memWrites]++;
    maxMemBytes = std::max(maxMemBytes, (UINT64)totalBytes);
    totalMemBytes += totalBytes;
    memInstCount++;
}

/* ===================================================================== */
// Instrumentation callbacks
/* ===================================================================== */
VOID Instruction(INS ins, VOID *v)
{
    // Instrumentation routine
    INS_InsertIfCall(ins, IPOINT_BEFORE, (AFUNPTR) Terminate, IARG_END);

    // MyExitRoutine() is called only when the last call returns a non-zero value.
    INS_InsertThenCall(ins, IPOINT_BEFORE, (AFUNPTR) MyExitRoutine, IARG_END);

    // FastForward() is called for every instruction executed
    // INS_InsertIfCall(ins, IPOINT_BEFORE, (AFUNPTR) FastForward, IARG_END);

    // // MyPredicatedAnalysis() is called only when the last FastForward() returns a non-zero value.
    // INS_InsertThenPredicatedCall(ins, IPOINT_BEFORE, (AFUNPTR) MyPredicatedAnalysis, IARG_END);

    // Instrumentation routine
    INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR) InsCount, IARG_END);

    INS_InsertIfCall(ins, IPOINT_BEFORE, (AFUNPTR) FastForward, IARG_END);
    INS_InsertThenCall(ins, IPOINT_BEFORE, (AFUNPTR)RecordInsFootprint,IARG_INST_PTR,IARG_UINT32, INS_Size(ins),IARG_END);

    UINT32 memOperands = INS_MemoryOperandCount(ins);
    UINT32 memReads = 0;
    UINT32 memWrites = 0;
    if(memOperands > 0){
        // type B instructions

        UINT32 totalMemBytes = 0;
        for (UINT32 memOp = 0; memOp < memOperands; memOp++) {
            UINT32  op_size = INS_MemoryOperandSize(ins,memOp);
            totalMemBytes += op_size;
            UINT32 val = (op_size+3)/4;
            if (INS_MemoryOperandIsRead(ins, memOp)) {
                INS_InsertIfCall(ins, IPOINT_BEFORE, (AFUNPTR) FastForward, IARG_END);
                INS_InsertThenPredicatedCall(ins,IPOINT_BEFORE,(AFUNPTR) inc_load,IARG_UINT32, val, IARG_END);
                INS_InsertIfCall(ins, IPOINT_BEFORE, (AFUNPTR)FastForward, IARG_END);
                INS_InsertThenPredicatedCall(ins, IPOINT_BEFORE, (AFUNPTR)RecordDataFootprint,
                                        IARG_MEMORYOP_EA, memOp,
                                        IARG_MEMORYREAD_SIZE,
                                        IARG_END);
                memReads++;
            }
            if (INS_MemoryOperandIsWritten(ins, memOp)) {
                INS_InsertIfCall(ins, IPOINT_BEFORE, (AFUNPTR) FastForward, IARG_END);
                INS_InsertThenPredicatedCall(ins,IPOINT_BEFORE,(AFUNPTR) inc_store,IARG_UINT32, val, IARG_END);
                INS_InsertIfCall(ins, IPOINT_BEFORE, (AFUNPTR)FastForward, IARG_END);
                INS_InsertThenPredicatedCall(ins, IPOINT_BEFORE, (AFUNPTR)RecordDataFootprint,
                                        IARG_MEMORYOP_EA, memOp,
                                        IARG_MEMORYWRITE_SIZE,
                                        IARG_END);
                memWrites++;
            }
            // 10
            INS_InsertIfCall(ins, IPOINT_BEFORE, (AFUNPTR)FastForward, IARG_END);
            INS_InsertThenPredicatedCall(ins, IPOINT_BEFORE, (AFUNPTR)UpdateDisplacementStats,
                                   IARG_ADDRINT, INS_OperandMemoryDisplacement(ins, memOp),
                                   IARG_END);
        }
        INS_InsertIfCall(ins, IPOINT_BEFORE, (AFUNPTR)FastForward, IARG_END);
        INS_InsertThenPredicatedCall(ins, IPOINT_BEFORE, (AFUNPTR)UpdateMemoryAnalysis,
                            IARG_UINT32, memReads,
                            IARG_UINT32, memWrites,
                            IARG_UINT32, totalMemBytes,
                            IARG_END);
    }


    INS_InsertIfCall(ins, IPOINT_BEFORE, (AFUNPTR)FastForward, IARG_END);
    INS_InsertThenPredicatedCall(ins, IPOINT_BEFORE, (AFUNPTR)UpdateMemOpDist,
                                IARG_UINT32, memReads,
                                IARG_UINT32, memWrites,
                                IARG_END);
    // NOPs
    if (INS_Category(ins) == XED_CATEGORY_NOP){
        INS_InsertIfCall(ins, IPOINT_BEFORE, (AFUNPTR) FastForward, IARG_END);
        INS_InsertThenPredicatedCall(ins, IPOINT_BEFORE, (AFUNPTR) inc_nop, IARG_END);
    }
    // Direct calls
    else if (INS_Category(ins) == XED_CATEGORY_CALL) {
        INS_InsertIfCall(ins, IPOINT_BEFORE, (AFUNPTR) FastForward, IARG_END);
        if (INS_IsDirectCall(ins)) {
            INS_InsertThenPredicatedCall(ins, IPOINT_BEFORE, (AFUNPTR) inc_direct_call, IARG_END);
        } else {
            INS_InsertThenPredicatedCall(ins, IPOINT_BEFORE, (AFUNPTR) inc_indirect_call, IARG_END);
        }
    }
    // Returns
    else if (INS_Category(ins) == XED_CATEGORY_RET){
        INS_InsertIfCall(ins, IPOINT_BEFORE, (AFUNPTR) FastForward, IARG_END);
        INS_InsertThenPredicatedCall(ins, IPOINT_BEFORE, (AFUNPTR) inc_return, IARG_END);
    }
    // Unconditional branches
    else if (INS_Category(ins) == XED_CATEGORY_UNCOND_BR){
        INS_InsertIfCall(ins, IPOINT_BEFORE, (AFUNPTR) FastForward, IARG_END);
        INS_InsertThenPredicatedCall(ins, IPOINT_BEFORE, (AFUNPTR) inc_unconditional_branch, IARG_END);
    }
    // Conditional branches
    else if (INS_Category(ins) == XED_CATEGORY_COND_BR){
        INS_InsertIfCall(ins, IPOINT_BEFORE, (AFUNPTR) FastForward, IARG_END);
        INS_InsertThenPredicatedCall(ins, IPOINT_BEFORE, (AFUNPTR) inc_conditional_branch, IARG_END);
    }
    // Logical operations
    else if (INS_Category(ins) == XED_CATEGORY_LOGICAL){
        INS_InsertIfCall(ins, IPOINT_BEFORE, (AFUNPTR) FastForward, IARG_END);
        INS_InsertThenPredicatedCall(ins, IPOINT_BEFORE, (AFUNPTR) inc_logical_operation, IARG_END);
    }
    // Rotate and shift
    else if (INS_Category(ins) == XED_CATEGORY_ROTATE || INS_Category(ins) == XED_CATEGORY_SHIFT){
        INS_InsertIfCall(ins, IPOINT_BEFORE, (AFUNPTR) FastForward, IARG_END);
        INS_InsertThenPredicatedCall(ins, IPOINT_BEFORE, (AFUNPTR) inc_rotate_shift, IARG_END);
    }
    // Flag operations
    else if (INS_Category(ins) == XED_CATEGORY_FLAGOP){
        INS_InsertIfCall(ins, IPOINT_BEFORE, (AFUNPTR) FastForward, IARG_END);
        INS_InsertThenPredicatedCall(ins, IPOINT_BEFORE, (AFUNPTR) inc_flag_operation, IARG_END);
    }
    // Vector instructions
    else if (INS_Category(ins) == XED_CATEGORY_AVX || 
             INS_Category(ins) == XED_CATEGORY_AVX2 || 
             INS_Category(ins) == XED_CATEGORY_AVX2GATHER || 
             INS_Category(ins) == XED_CATEGORY_AVX512){
        INS_InsertIfCall(ins, IPOINT_BEFORE, (AFUNPTR) FastForward, IARG_END);
        INS_InsertThenPredicatedCall(ins, IPOINT_BEFORE, (AFUNPTR) inc_vector_instruction, IARG_END);
    }
    // Conditional moves
    else if (INS_Category(ins) == XED_CATEGORY_CMOV){
        INS_InsertIfCall(ins, IPOINT_BEFORE, (AFUNPTR) FastForward, IARG_END);
        INS_InsertThenPredicatedCall(ins, IPOINT_BEFORE, (AFUNPTR) inc_conditional_move, IARG_END);
    }
    // MMX and SSE instructions
    else if (INS_Category(ins) == XED_CATEGORY_MMX || INS_Category(ins) == XED_CATEGORY_SSE){
        INS_InsertIfCall(ins, IPOINT_BEFORE, (AFUNPTR) FastForward, IARG_END);
        INS_InsertThenPredicatedCall(ins, IPOINT_BEFORE, (AFUNPTR) inc_mmx_sse, IARG_END);
    }
    // System calls
    else if (INS_Category(ins) == XED_CATEGORY_SYSCALL){
        INS_InsertIfCall(ins, IPOINT_BEFORE, (AFUNPTR) FastForward, IARG_END);
        INS_InsertThenPredicatedCall(ins, IPOINT_BEFORE, (AFUNPTR) inc_system_call, IARG_END);
    }
    // Floating-point
    else if (INS_Category(ins) == XED_CATEGORY_X87_ALU){
        INS_InsertIfCall(ins, IPOINT_BEFORE, (AFUNPTR) FastForward, IARG_END);
        INS_InsertThenPredicatedCall(ins, IPOINT_BEFORE, (AFUNPTR) inc_floating_point, IARG_END);
    }
    // Others (whatever is left)
    else {
        INS_InsertIfCall(ins, IPOINT_BEFORE, (AFUNPTR) FastForward, IARG_END);
        INS_InsertThenPredicatedCall(ins, IPOINT_BEFORE, (AFUNPTR) inc_other, IARG_END);
    }

    // 1. Instruction length distribution (all instructions)
    INS_InsertIfCall(ins, IPOINT_BEFORE, (AFUNPTR)FastForward, IARG_END);
    INS_InsertThenCall(ins, IPOINT_BEFORE, (AFUNPTR)UpdateInstructionStats,
                   IARG_UINT32, INS_Size(ins),
                   IARG_UINT32, INS_OperandCount(ins),
                   IARG_UINT32, INS_MaxNumRRegs(ins),
                   IARG_UINT32, INS_MaxNumWRegs(ins),
                   IARG_END);

    // 9. Immediate value statistics
    for (UINT32 op = 0; op < INS_OperandCount(ins); op++) {
        if (INS_OperandIsImmediate(ins, op)) {
            INS_InsertIfCall(ins, IPOINT_BEFORE, (AFUNPTR)FastForward, IARG_END);
            INS_InsertThenCall(ins, IPOINT_BEFORE, (AFUNPTR)UpdateImmediateStats,
                              IARG_ADDRINT, static_cast<ADDRINT>(INS_OperandImmediate(ins, op)),
                              IARG_END);
        }
    }
}
/*!
 * Insert call to the CountBbl() analysis routine before every basic block 
 * of the trace.
 * This function is called every time a new trace is encountered.
 * @param[in]   trace    trace to be instrumented
 * @param[in]   v        value specified by the tool in the TRACE_AddInstrumentFunction
 *                       function call
 */
VOID Trace(TRACE trace, VOID* v)
{
    // Visit every basic block in the trace
    for (BBL bbl = TRACE_BblHead(trace); BBL_Valid(bbl); bbl = BBL_Next(bbl))
    {
        // Insert a call to CountBbl() before every basic bloc, passing the number of instructions
        BBL_InsertCall(bbl, IPOINT_BEFORE, (AFUNPTR)CountBbl, IARG_UINT32, BBL_NumIns(bbl), IARG_END);
    }
}

/*!
 * Increase counter of threads in the application.
 * This function is called for every thread created by the application when it is
 * about to start running (including the root thread).
 * @param[in]   threadIndex     ID assigned by PIN to the new thread
 * @param[in]   ctxt            initial register state for the new thread
 * @param[in]   flags           thread creation flags (OS specific)
 * @param[in]   v               value specified by the tool in the 
 *                              PIN_AddThreadStartFunction function call
 */
VOID ThreadStart(THREADID threadIndex, CONTEXT* ctxt, INT32 flags, VOID* v) { threadCount++; }

/*!
 * Print out analysis results.
 * This function is called when the application exits.
 * @param[in]   code            exit code of the application
 * @param[in]   v               value specified by the tool in the 
 *                              PIN_AddFiniFunction function call
 */
VOID Fini(INT32 code, VOID* v)
{
    *out << "===============================================" << endl;
    *out << "MyPinTool analysis results: " << endl;
    *out << "Number of instructions: " << insCount << endl;
    *out << "Number of basic blocks: " << bblCount << endl;
    *out << "Number of threads: " << threadCount << endl;
    *out << "===============================================" << endl;
}

/*!
 * The main procedure of the tool.
 * This function is called when the application image is loaded but not yet started.
 * @param[in]   argc            total number of elements in the argv array
 * @param[in]   argv            array of command line arguments, 
 *                              including pin -t <toolname> -- ...
 */
int main(int argc, char* argv[])
{
    // Initialize PIN library. Print help message if -h(elp) is specified
    // in the command line or the command line is invalid
    if (PIN_Init(argc, argv))
    {
        return Usage();
    }

    string fileName = KnobOutputFile.Value();
    fast_forward_count = KnobFastForward.Value() * 1e9;
    if (!fileName.empty())
    {
        out = new std::ofstream(fileName.c_str());
    }

    // Register Instruction to be called to instrument instructions


    if (KnobCount)
    {
            INS_AddInstrumentFunction(Instruction,0);
        // Register function to be called to instrument traces
        TRACE_AddInstrumentFunction(Trace, 0);

        // Register function to be called for every thread before it starts running
        PIN_AddThreadStartFunction(ThreadStart, 0);

        // Register function to be called when the application exits
        PIN_AddFiniFunction(Fini, 0);
    }

    cerr << "===============================================" << endl;
    cerr << "This application is instrumented by MyPinTool" << endl;
    if (!KnobOutputFile.Value().empty())
    {
        cerr << "See file " << KnobOutputFile.Value() << " for analysis results" << endl;
    }
    cerr << "===============================================" << endl;

    // Start the program, never returns
    PIN_StartProgram();

    return 0;
}

/* ===================================================================== */
/* eof */
/* ===================================================================== */

