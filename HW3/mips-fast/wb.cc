#include "wb.h"

Writeback::Writeback (Mipc *mc)
{
   _mc = mc;
}

Writeback::~Writeback (void) {}

void
Writeback::MainLoop (void)
{
   unsigned int ins;
   Bool writeReg;
   Bool writeFReg;
   Bool loWPort;
   Bool hiWPort;
   Bool isSyscall;
   Bool isIllegalOp;
   unsigned decodedDST;
   unsigned opResultLo, opResultHi;

   while (1) {
      AWAIT_P_PHI0;	// @posedge
	_mc->WB = _mc->MEM_WB;
         writeReg = _mc->WB._writeREG;
         writeFReg = _mc->WB._writeFREG;
         loWPort = _mc->WB._loWPort;
         hiWPort = _mc->WB._hiWPort;
         decodedDST = _mc->WB._decodedDST;
         opResultLo = _mc->WB._opResultLo;
         opResultHi = _mc->WB._opResultHi;
         isSyscall = _mc->WB._isSyscall;
         isIllegalOp = _mc->WB._isIllegalOp;
         ins = _mc->WB._ins;
         if (isSyscall) {
            _mc->num_syscalls += 1;
            _mc->_nfetched -= 1;
#ifdef MIPC_DEBUG
            fprintf(_mc->_debugLog, "<%llu> SYSCALL! Trapping to emulation layer at PC %#x\n", SIM_TIME, _mc->_pc);
#endif      
            _mc->WB._opControl(_mc, ins);
            _mc->_pc -= 4;
	    _mc->syscall_stall = 0;
	    for(int i=0;i<32;i++)   _mc->_flags[i] = 0; 
            for(int i=0;i<32;i++)      _mc->_gpr_b[i] = _mc->_gpr[i];
         }
         else if (isIllegalOp) {
            printf("Illegal ins %#x at PC %#x. Terminating simulation!\n", ins, _mc->_pc);
#ifdef MIPC_DEBUG
            fclose(_mc->_debugLog);
#endif
            printf("Register state on termination:\n\n");
            _mc->dumpregs();
            exit(0);
         }
         else {
            if (writeReg) {
               _mc->_gpr[decodedDST] = opResultLo;
#ifdef MIPC_DEBUG
               fprintf(_mc->_debugLog, "<%llu> Writing to reg %u, value: %#x\n", SIM_TIME, decodedDST, opResultLo);
#endif
            }
            else if (writeFReg) {
               _mc->_fpr[(decodedDST)>>1].l[FP_TWIDDLE^((decodedDST)&1)] = opResultLo;
#ifdef MIPC_DEBUG
               fprintf(_mc->_debugLog, "<%llu> Writing to freg %u, value: %#x\n", SIM_TIME, decodedDST>>1, opResultLo);
#endif
            }
            else if (loWPort || hiWPort) {
               if (loWPort) {
                  _mc->_lo = opResultLo;
#ifdef MIPC_DEBUG
                  fprintf(_mc->_debugLog, "<%llu> Writing to Lo, value: %#x\n", SIM_TIME, opResultLo);
#endif
               }
               if (hiWPort) {
                  _mc->_hi = opResultHi;
#ifdef MIPC_DEBUG
                  fprintf(_mc->_debugLog, "<%llu> Writing to Hi, value: %#x\n", SIM_TIME, opResultHi);
#endif
               }
            }
         }
         _mc->_gpr[0] = 0;
         _mc->_gpr_b[0] = 0;	
	 AWAIT_P_PHI1;       // @negedge
   }
}
