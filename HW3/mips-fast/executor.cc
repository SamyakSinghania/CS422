#include "executor.h"

Exe::Exe (Mipc *mc)
{
   _mc = mc;
}

Exe::~Exe (void) {}

void
Exe::MainLoop (void)
{
   unsigned int ins;
   Bool isSyscall, isIllegalOp;

   while (1) {
      AWAIT_P_PHI0;	// @posedge
	_mc->EX = _mc->ID_EX;
         ins = _mc->EX._ins;
         isSyscall = _mc->EX._isSyscall;
         isIllegalOp = _mc->EX._isIllegalOp;
	 _mc->EX._btaken = 0;
         if (!isSyscall && !isIllegalOp) {
            _mc->EX._opControl(_mc,ins);
#ifdef MIPC_DEBUG
            fprintf(_mc->_debugLog, "<%llu> Executed ins %#x\n", SIM_TIME, ins);
#endif
         }
         else if (isSyscall) {
#ifdef MIPC_DEBUG
            fprintf(_mc->_debugLog, "<%llu> Deferring execution of syscall ins %#x\n", SIM_TIME, ins);
#endif
         }
         else {
#ifdef MIPC_DEBUG
            fprintf(_mc->_debugLog, "<%llu> Illegal ins %#x in execution stage at PC %#x\n", SIM_TIME, ins, _mc->_pc);
#endif
         }

         if (!isIllegalOp && !isSyscall) {
            if (_mc->EX._btaken)
            {
               _mc->_pc = _mc->EX._btgt;
            }
         }
	for(int i=0;i<32;i++)	_mc->_flags[i] = 0;
	 AWAIT_P_PHI1;  // @negedge
	if(_mc->EX._memControl == 0){
	
            if (_mc->EX._writeREG) {
               _mc->_gpr_b[_mc->EX._decodedDST] = _mc->EX._opResultLo;
		_mc->_flags[_mc->EX._decodedDST]	= 1;
            }
		_mc->_gpr_b[0] = 0;
	
	}
	_mc->EX_MEM = _mc->EX;
   }
}
