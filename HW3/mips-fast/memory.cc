#include "memory.h"

Memory::Memory (Mipc *mc)
{
   _mc = mc;
}

Memory::~Memory (void) {}

void
Memory::MainLoop (void)
{
   Bool memControl;

   while (1) {
      AWAIT_P_PHI0;	// @posedge
	_mc->MEM = _mc->EX_MEM;
         memControl = _mc->MEM._memControl;
         if (memControl) {
            _mc->MEM._memOp (_mc);
#ifdef MIPC_DEBUG
            fprintf(_mc->_debugLog, "<%llu> Accessing memory at address %#x for ins %#x %x %x\n", SIM_TIME, _mc->MEM._memory_addr_reg, _mc->MEM._ins,_mc->_gpr_b[_mc->MEM._decodedDST],_mc->MEM._opResultLo);
#endif
         }
         else {
#ifdef MIPC_DEBUG
            fprintf(_mc->_debugLog, "<%llu> Memory has nothing to do for ins %#x\n", SIM_TIME, _mc->MEM._ins);
#endif
         }

	
	AWAIT_P_PHI1;       // @negedge
	if(_mc->MEM.load_delay){
	if(_mc->MEM._writeREG && _mc->_flags[_mc->MEM._decodedDST] == 0){
		_mc->_gpr_b[_mc->MEM._decodedDST] = _mc->MEM._opResultLo;
	     _mc->_gpr_b[0] = 0;
	}
	}
	 _mc->MEM_WB = _mc->MEM;
   }
}
