#include "decode.h"

Decode::Decode (Mipc *mc)
{
   _mc = mc;
}

Decode::~Decode (void) {}

void
Decode::MainLoop (void)
{
   unsigned int ins;
   while (1) {
	unsigned int ss = 0, bs = 0;
      AWAIT_P_PHI0;	// @posedge
	ss = _mc->syscall_stall;
	bs = _mc->branch_stall;
	if(_mc->decoder_stall==0){
		_mc->ID = _mc->IF_ID;
         	ins = _mc->ID._ins;
	}
         AWAIT_P_PHI1;	// @negedge
	if(_mc->syscall_stall){
		_mc->ID = _mc->NOP;
		ins = _mc->ID._ins;
	}
	if(_mc->decoder_stall <= 1){
         _mc->Dec(ins);
#ifdef MIPC_DEBUG
         fprintf(_mc->_debugLog, "<%llu> Decoded ins %#x\n", SIM_TIME, ins);
#endif
	}
	if(_mc->decoder_stall)	_mc->decoder_stall -= 1;
	_mc->ID_EX = _mc->ID;
	if(_mc->decoder_stall)	_mc->ID_EX = _mc->NOP;
	if(ss)	_mc->ID_EX = _mc->NOP;
   }
}
