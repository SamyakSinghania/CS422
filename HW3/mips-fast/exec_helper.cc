#include <math.h>
#include "mips.h"
#include "opcodes.h"
#include <assert.h>
#include "app_syscall.h"

#define MAGIC_EXEC 0xdeadbeef

/*------------------------------------------------------------------------
 *
 *  Instruction exec 
 *
 *------------------------------------------------------------------------
 */
void
Mipc::Dec (unsigned int ins)
{
   MipsInsn i;
   signed int a1, a2;
   unsigned int ar1, ar2, s1, s2, r1, r2, t1, t2;
   LL addr;
   unsigned int val;
   LL value, mask;
   int sa,j;
   Word dummy;

   ID._isIllegalOp = FALSE;
   ID._isSyscall = FALSE;

   i.data = ins;

   //Interlock logic
   unsigned prev1, prev2, prev3;
  prev1 = EX._decodedDST;
  prev2 = MEM._decodedDST;
   prev3 = WB._decodedDST;

#define SIGN_EXTEND_BYTE(x)  do { x <<= 24; x >>= 24; } while (0)
#define SIGN_EXTEND_IMM(x)   do { x <<= 16; x >>= 16; } while (0)

   switch (i.reg.op) {
   case 0:
      // SPECIAL (ALU format)
      ID._decodedSRC1 = _gpr[i.reg.rs];
      ID._decodedSRC2 = _gpr[i.reg.rt];
      ID._decodedDST = i.reg.rd;
      ID._writeREG = TRUE;
      ID._writeFREG = FALSE;
      ID._hiWPort = FALSE;
      ID._loWPort = FALSE;
      ID._memControl = FALSE;
      
      if(EX.load_delay && (EX._decodedDST == i.reg.rs || EX._decodedDST == i.reg.rt))	decoder_stall = 2;
      if(EX.load_delay && (EX._decodedDST == i.reg.rs || EX._decodedDST == i.reg.rt))   num_load_delays += 1;

      switch (i.reg.func) {
      case 0x20:			// add
      case 0x21:			// addu
         ID._opControl = func_add_addu;
	 break;

      case 0x24:			// and
         ID._opControl = func_and;
	 break;

      case 0x27:			// nor
         ID._opControl = func_nor;
	 break;

      case 0x25:			// or
         ID._opControl = func_or;
	 break;

      case 0:			// sll
         ID._opControl = func_sll;
         ID._decodedShiftAmt = i.reg.sa;
	 break;

      case 4:			// sllv
         ID._opControl = func_sllv;
	 break;

      case 0x2a:			// slt
         ID._opControl = func_slt;
	 break;

      case 0x2b:			// sltu
         ID._opControl = func_sltu;
	 break;

      case 0x3:			// sra
         ID._opControl = func_sra;
         ID._decodedShiftAmt = i.reg.sa;
	 break;

      case 0x7:			// srav
         ID._opControl = func_srav;
	 break;

      case 0x2:			// srl
         ID._opControl = func_srl;
         ID._decodedShiftAmt = i.reg.sa;
	 break;

      case 0x6:			// srlv
         ID._opControl = func_srlv;
	 break;

      case 0x22:			// sub
      case 0x23:			// subu
	 // no overflow check
         ID._opControl = func_sub_subu;
	 break;

      case 0x26:			// xor
         ID._opControl = func_xor;
	 break;

      case 0x1a:			// div
         ID._opControl = func_div;
         ID._hiWPort = TRUE;
         ID._loWPort = TRUE;
         ID._writeREG = FALSE;
         ID._writeFREG = FALSE;
	 break;

      case 0x1b:			// divu
         ID._opControl = func_divu;
         ID._hiWPort = TRUE;
         ID._loWPort = TRUE;
         ID._writeREG = FALSE;
         ID._writeFREG = FALSE;
	 break;

      case 0x10:			// mfhi
         ID._opControl = func_mfhi;
	if(EX._hiWPort)	decoder_stall = 4;
	else if(MEM._hiWPort)	decoder_stall = 3;
	else if(WB._hiWPort)	decoder_stall = 2;
	 break;

      case 0x12:			// mflo
         ID._opControl = func_mflo;
	if(EX._loWPort) decoder_stall = 4;
        else if(MEM._loWPort)   decoder_stall = 3;
        else if(WB._loWPort)    decoder_stall = 2;
	 break;

      case 0x11:			// mthi
         ID._opControl = func_mthi;
         ID._hiWPort = TRUE;
         ID._writeREG = FALSE;
         ID._writeFREG = FALSE;
	 break;

      case 0x13:			// mtlo
         ID._opControl = func_mtlo;
         ID._loWPort = TRUE;
         ID._writeREG = FALSE;
         ID._writeFREG = FALSE;
	 break;

      case 0x18:			// mult
         ID._opControl = func_mult;
         ID._hiWPort = TRUE;
         ID._loWPort = TRUE;
         ID._writeREG = FALSE;
         ID._writeFREG = FALSE;
	 break;

      case 0x19:			// multu
         ID._opControl = func_multu;
         ID._hiWPort = TRUE;
         ID._loWPort = TRUE;
         ID._writeREG = FALSE;
          ID._writeFREG = FALSE;
	 break;

      case 9:			// jalr
         ID._opControl = func_jalr;
         ID._btgt = ID._decodedSRC1;
         ID._bdslot = 1;
	branch_stall = 1;
         break;

      case 8:			// jr
         ID._opControl = func_jr;
         ID._writeREG = FALSE;
         ID._writeFREG = FALSE;
         ID._btgt = ID._decodedSRC1;
         ID._bdslot = 1;
	 branch_stall = 1;
	 break;

      case 0xd:			// await/break
         ID._opControl = func_await_break;
         ID._writeREG = FALSE;
         ID._writeFREG = FALSE;
	 break;

      case 0xc:			// syscall
         ID._opControl = func_syscall;
         ID._writeREG = FALSE;
         ID._writeFREG = FALSE;
         ID._isSyscall = TRUE;
	syscall_stall = 1;
	 break;

      default:
	 ID._isIllegalOp = TRUE;
         ID._writeREG = FALSE;
         ID._writeFREG = FALSE;
	 break;
      }
      break;	// ALU format

   case 8:			// addi
   case 9:			// addiu
      // ignore overflow: no exceptions
      ID._opControl = func_addi_addiu;
      ID._decodedSRC1 = _gpr[i.imm.rs];
      ID._decodedSRC2 = i.imm.imm;
      ID._decodedDST = i.imm.rt;
      ID._writeREG = TRUE;
      ID._writeFREG = FALSE;
      ID._hiWPort = FALSE;
      ID._loWPort = FALSE;
      ID._memControl = FALSE;

	if(EX.load_delay && (EX._decodedDST == i.reg.rs || EX._decodedDST == i.reg.rs))   decoder_stall = 2;
        if(EX.load_delay && (EX._decodedDST == i.reg.rs || EX._decodedDST == i.reg.rs))   num_load_delays += 1;

      break;

   case 0xc:			// andi
      ID._opControl = func_andi;
      ID._decodedSRC1 = _gpr[i.imm.rs];
      ID._decodedSRC2 = i.imm.imm;
      ID._decodedDST = i.imm.rt;
      ID._writeREG = TRUE;
      ID._writeFREG = FALSE;
      ID._hiWPort = FALSE;
      ID._loWPort = FALSE;
      ID._memControl = FALSE;

	if(EX.load_delay && (EX._decodedDST == i.reg.rs || EX._decodedDST == i.reg.rs))   decoder_stall = 2;
        if(EX.load_delay && (EX._decodedDST == i.reg.rs || EX._decodedDST == i.reg.rs))   num_load_delays += 1;

      break;

   case 0xf:			// lui
      ID._opControl = func_lui;
      ID._decodedSRC2 = i.imm.imm;
      ID._decodedDST = i.imm.rt;
      ID._writeREG = TRUE;
      ID._writeFREG = FALSE;
      ID._hiWPort = FALSE;
      ID._loWPort = FALSE;
      ID._memControl = FALSE;
      break;

   case 0xd:			// ori
      ID._opControl = func_ori;
      ID._decodedSRC1 = _gpr[i.imm.rs];
      ID._decodedSRC2 = i.imm.imm;
      ID._decodedDST = i.imm.rt;
      ID._writeREG = TRUE;
      ID._writeFREG = FALSE;
      ID._hiWPort = FALSE;
      ID._loWPort = FALSE;
      ID._memControl = FALSE;
	if(EX.load_delay && (EX._decodedDST == i.reg.rs || EX._decodedDST == i.reg.rs))   decoder_stall = 2;
        if(EX.load_delay && (EX._decodedDST == i.reg.rs || EX._decodedDST == i.reg.rt))   num_load_delays += 1;

      break;

   case 0xa:			// slti
      ID._opControl = func_slti;
      ID._decodedSRC1 = _gpr[i.imm.rs];
      ID._decodedSRC2 = i.imm.imm;
      ID._decodedDST = i.imm.rt;
      ID._writeREG = TRUE;
      ID._writeFREG = FALSE;
      ID._hiWPort = FALSE;
      ID._loWPort = FALSE;
      ID._memControl = FALSE;
	if(EX.load_delay && (EX._decodedDST == i.reg.rs || EX._decodedDST == i.reg.rt))   decoder_stall = 2;
        if(EX.load_delay && (EX._decodedDST == i.reg.rs || EX._decodedDST == i.reg.rt))   num_load_delays += 1;

      break;

   case 0xb:			// sltiu
      ID._opControl = func_sltiu;
      ID._decodedSRC1 = _gpr[i.imm.rs];
      ID._decodedSRC2 = i.imm.imm;
      ID._decodedDST = i.imm.rt;
      ID._writeREG = TRUE;
      ID._writeFREG = FALSE;
      ID._hiWPort = FALSE;
      ID._loWPort = FALSE;
      ID._memControl = FALSE;

	if(EX.load_delay && (EX._decodedDST == i.reg.rs || EX._decodedDST == i.reg.rt))   decoder_stall = 2;
        if(EX.load_delay && (EX._decodedDST == i.reg.rs || EX._decodedDST == i.reg.rt))   num_load_delays += 1;

      break;

   case 0xe:			// xori
      ID._opControl = func_xori;
      ID._decodedSRC1 = _gpr[i.imm.rs];
      ID._decodedSRC2 = i.imm.imm;
      ID._decodedDST = i.imm.rt;
      ID._writeREG = TRUE;
      ID._writeFREG = FALSE;
      ID._hiWPort = FALSE;
      ID._loWPort = FALSE;
      ID._memControl = FALSE;

	if(EX.load_delay && (EX._decodedDST == i.reg.rs || EX._decodedDST == i.reg.rt))   decoder_stall = 2;
        if(EX.load_delay && (EX._decodedDST == i.reg.rs || EX._decodedDST == i.reg.rt))   num_load_delays += 1;

      break;

   case 4:			// beq
      ID._opControl = func_beq;
      ID._decodedSRC1 = _gpr[i.imm.rs];
      ID._decodedSRC2 = _gpr[i.imm.rt];
      ID._branchOffset = i.imm.imm;
      ID._writeREG = FALSE;
      ID._writeFREG = FALSE;
      ID._hiWPort = FALSE;
      ID._loWPort = FALSE;
      ID._memControl = FALSE;
      ID._branchOffset <<= 16; ID._branchOffset >>= 14; ID._bdslot = 1; ID._btgt = (unsigned)((signed)ID.pc+ID._branchOffset+4);
      branch_stall = 1;

	if(EX.load_delay && (EX._decodedDST == i.reg.rs || EX._decodedDST == i.reg.rt))   decoder_stall = 2;
        if(EX.load_delay && (EX._decodedDST == i.reg.rs || EX._decodedDST == i.reg.rt))   num_load_delays += 1;
	break;

   case 1:
      // REGIMM
      ID._decodedSRC1 = _gpr[i.reg.rs];
      ID._branchOffset = i.imm.imm;
      ID._writeREG = FALSE;
      ID._writeFREG = FALSE;
      ID._hiWPort = FALSE;
      ID._loWPort = FALSE;
      ID._memControl = FALSE;

	if(EX.load_delay && (EX._decodedDST == i.reg.rs || EX._decodedDST == i.reg.rs))   decoder_stall = 2;
 branch_stall = 1;

       if(EX.load_delay && (EX._decodedDST == i.reg.rs || EX._decodedDST == i.reg.rs))   num_load_delays += 1;

      switch (i.reg.rt) {
      case 1:			// bgez
         ID._opControl = func_bgez;
         ID._branchOffset <<= 16; ID._branchOffset >>= 14; ID._bdslot = 1; ID._btgt = (unsigned)((signed)ID.pc+ID._branchOffset+4);
	 break;

      case 0x11:			// bgezal
         ID._opControl = func_bgezal;
         ID._decodedDST = 31;
         ID._writeREG = TRUE;
         ID._branchOffset <<= 16; ID._branchOffset >>= 14; ID._bdslot = 1; ID._btgt = (unsigned)((signed)ID.pc+ID._branchOffset+4);
	 break;

      case 0x10:			// bltzal
         ID._opControl = func_bltzal;
         ID._decodedDST = 31;
         ID._writeREG = TRUE;
         ID._branchOffset <<= 16; ID._branchOffset >>= 14; ID._bdslot = 1; ID._btgt = (unsigned)((signed)ID.pc+ID._branchOffset+4);
	 break;

      case 0x0:			// bltz
         ID._opControl = func_bltz;
         ID._branchOffset <<= 16; ID._branchOffset >>= 14; ID._bdslot = 1; ID._btgt = (unsigned)((signed)ID.pc+ID._branchOffset+4);
	 break;

      default:
	 ID._isIllegalOp = TRUE;
	 break;
      }
      break;

   case 7:			// bgtz
      ID._opControl = func_bgtz;
      ID._decodedSRC1 = _gpr[i.reg.rs];
      ID._branchOffset = i.imm.imm;
      ID._writeREG = FALSE;
      ID._writeFREG = FALSE;
      ID._hiWPort = FALSE;
      ID._loWPort = FALSE;
      ID._memControl = FALSE;
      ID._branchOffset <<= 16; ID._branchOffset >>= 14; ID._bdslot = 1; ID._btgt = (unsigned)((signed)ID.pc+ID._branchOffset+4);
	if(EX.load_delay && (EX._decodedDST == i.reg.rs || EX._decodedDST == i.reg.rs))   decoder_stall = 2;
	 branch_stall = 1;
        if(EX.load_delay && (EX._decodedDST == i.reg.rs || EX._decodedDST == i.reg.rs))   num_load_delays += 1;

      break;

   case 6:			// blez
      ID._opControl = func_blez;
      ID._decodedSRC1 = _gpr[i.reg.rs];
      ID._branchOffset = i.imm.imm;
      ID._writeREG = FALSE;
      ID._writeFREG = FALSE;
      ID._hiWPort = FALSE;
      ID._loWPort = FALSE;
      ID._memControl = FALSE;
      ID._branchOffset <<= 16; ID._branchOffset >>= 14; ID._bdslot = 1; ID._btgt = (unsigned)((signed)ID.pc+ID._branchOffset+4);

	if(EX.load_delay && (EX._decodedDST == i.reg.rs || EX._decodedDST == i.reg.rs))   decoder_stall = 2;
	 branch_stall = 1;

        if(EX.load_delay && (EX._decodedDST == i.reg.rs || EX._decodedDST == i.reg.rs))   num_load_delays += 1;
      break;

   case 5:			// bne
      ID._opControl = func_bne;
      ID._decodedSRC1 = _gpr[i.reg.rs];
      ID._decodedSRC2 = _gpr[i.reg.rt];
      ID._branchOffset = i.imm.imm;
      ID._writeREG = FALSE;
      ID._writeFREG = FALSE;
      ID._hiWPort = FALSE;
      ID._loWPort = FALSE;
      ID._memControl = FALSE;
      ID._branchOffset <<= 16; ID._branchOffset >>= 14; ID._bdslot = 1; ID._btgt = (unsigned)((signed)ID.pc+ID._branchOffset+4);
	
	if(EX.load_delay && (EX._decodedDST == i.reg.rs || EX._decodedDST == i.reg.rt))   decoder_stall = 2;
	 branch_stall = 1;

        if(EX.load_delay && (EX._decodedDST == i.reg.rs || EX._decodedDST == i.reg.rt))   num_load_delays += 1;
      break;


   case 2:			// j
      ID._opControl = func_j;
      ID._branchOffset = i.tgt.tgt;
      ID._writeREG = FALSE;
      ID._writeFREG = FALSE;
      ID._hiWPort = FALSE;
      ID._loWPort = FALSE;
      ID._memControl = FALSE;
      ID._btgt = ((ID.pc+4) & 0xf0000000) | (ID._branchOffset<<2); ID._bdslot = 1;

	 branch_stall = 1;
      break;

   case 3:			// jal
      ID._opControl = func_jal;
      ID._branchOffset = i.tgt.tgt;
      ID._decodedDST = 31;
      ID._writeREG = TRUE;
      ID._writeFREG = FALSE;
      ID._hiWPort = FALSE;
      ID._loWPort = FALSE;
      ID._memControl = FALSE;
      ID._btgt = ((ID.pc+4) & 0xf0000000) | (ID._branchOffset<<2); ID._bdslot = 1;

	 branch_stall = 1;
      break;

   case 0x20:			// lb  
      ID._opControl = func_lb;
      ID._memOp = mem_lb;
      ID._decodedSRC1 = _gpr[i.reg.rs];
      ID._decodedSRC2 = i.imm.imm;
      ID._decodedDST = i.reg.rt;
      ID._writeREG = TRUE;
      ID._writeFREG = FALSE;
      ID._hiWPort = FALSE;
      ID._loWPort = FALSE;
      ID._memControl = TRUE;
	ID.load_delay = 1;
	if(EX.load_delay && (EX._decodedDST == i.reg.rs || EX._decodedDST == i.reg.rs))   decoder_stall = 2;

        if(EX.load_delay && (EX._decodedDST == i.reg.rs || EX._decodedDST == i.reg.rs))   num_load_delays += 1;
	 break;

   case 0x24:			// lbu
      ID._opControl = func_lbu;
      ID._memOp = mem_lbu;
      ID._decodedSRC1 = _gpr[i.reg.rs];
      ID._decodedSRC2 = i.imm.imm;
      ID._decodedDST = i.reg.rt;
      ID._writeREG = TRUE;
      ID._writeFREG = FALSE;
      ID._hiWPort = FALSE;
      ID._loWPort = FALSE;
      ID._memControl = TRUE;
	ID.load_delay = 1;
	if(EX.load_delay && (EX._decodedDST == i.reg.rs || EX._decodedDST == i.reg.rs))   decoder_stall = 2;

        if(EX.load_delay && (EX._decodedDST == i.reg.rs || EX._decodedDST == i.reg.rs))   num_load_delays += 1;
	break;

   case 0x21:			// lh
      ID._opControl = func_lh;
      ID._memOp = mem_lh;
      ID._decodedSRC1 = _gpr[i.reg.rs];
      ID._decodedSRC2 = i.imm.imm;
      ID._decodedDST = i.reg.rt;
      ID._writeREG = TRUE;
      ID._writeFREG = FALSE;
      ID._hiWPort = FALSE;
      ID._loWPort = FALSE;
      ID._memControl = TRUE;
	ID.load_delay = 1;
	if(EX.load_delay && (EX._decodedDST == i.reg.rs || EX._decodedDST == i.reg.rs))   decoder_stall = 2;

        if(EX.load_delay && (EX._decodedDST == i.reg.rs || EX._decodedDST == i.reg.rs))   num_load_delays += 1;
      break;

   case 0x25:			// lhu
      ID._opControl = func_lhu;
      ID._memOp = mem_lhu;
      ID._decodedSRC1 = _gpr[i.reg.rs];
      ID._decodedSRC2 = i.imm.imm;
      ID._decodedDST = i.reg.rt;
      ID._writeREG = TRUE;
      ID._writeFREG = FALSE;
      ID._hiWPort = FALSE;
      ID._loWPort = FALSE;
      ID._memControl = TRUE;
	ID.load_delay = 1;
	if(EX.load_delay && (EX._decodedDST == i.reg.rs || EX._decodedDST == i.reg.rs))   decoder_stall = 2;

        if(EX.load_delay && (EX._decodedDST == i.reg.rs || EX._decodedDST == i.reg.rs))   num_load_delays += 1;
      break;

   case 0x22:			// lwl
      ID._opControl = func_lwl;
      ID._memOp = mem_lwl;
      ID._decodedSRC1 = _gpr[i.reg.rs];
      ID._decodedSRC2 = i.imm.imm;
      ID._subregOperand = _gpr[i.reg.rt];
      ID._decodedDST = i.reg.rt;
      ID._writeREG = TRUE;
      ID._writeFREG = FALSE;
      ID._hiWPort = FALSE;
      ID._loWPort = FALSE;
      ID._memControl = TRUE;
	ID.load_delay = 1;
	if(EX.load_delay && (EX._decodedDST == i.reg.rs || EX._decodedDST == i.reg.rt))   decoder_stall = 2;

        if(EX.load_delay && (EX._decodedDST == i.reg.rs || EX._decodedDST == i.reg.rt))   num_load_delays += 1;
break;

   case 0x23:			// lw
      ID._opControl = func_lw;
      ID._memOp = mem_lw;
      ID._decodedSRC1 = _gpr[i.reg.rs];
      ID._decodedSRC2 = i.imm.imm;
      ID._decodedDST = i.reg.rt;
      ID._writeREG = TRUE;
      ID._writeFREG = FALSE;
      ID._hiWPort = FALSE;
      ID._loWPort = FALSE;
      ID._memControl = TRUE;
	ID.load_delay = 1;
	if(EX.load_delay && (EX._decodedDST == i.reg.rs || EX._decodedDST == i.reg.rs))   decoder_stall = 2;

        if(EX.load_delay && (EX._decodedDST == i.reg.rs || EX._decodedDST == i.reg.rs))   num_load_delays += 1;
      break;

   case 0x26:			// lwr
      ID._opControl = func_lwr;

      ID._memOp = mem_lwr;
      ID._decodedSRC1 = _gpr[i.reg.rs];
      ID._decodedSRC2 = i.imm.imm;
      ID._subregOperand = _gpr[i.reg.rt];
      ID._decodedDST = i.reg.rt;
      ID._writeREG = TRUE;
      ID._writeFREG = FALSE;
      ID._hiWPort = FALSE;
      ID._loWPort = FALSE;
      ID._memControl = TRUE;
	ID.load_delay = 1;
	if(EX.load_delay && (EX._decodedDST == i.reg.rs || EX._decodedDST == i.reg.rt))   decoder_stall = 2;
        if(EX.load_delay && (EX._decodedDST == i.reg.rs || EX._decodedDST == i.reg.rt))   num_load_delays += 1;
      break;

   case 0x31:			// lwc1
      ID._opControl = func_lwc1;
      ID._memOp = mem_lwc1;
      ID._decodedSRC1 = _gpr[i.reg.rs];
      ID._decodedSRC2 = i.imm.imm;
      ID._decodedDST = i.reg.rt;
      ID._writeREG = FALSE;
      ID._writeFREG = TRUE;
      ID._hiWPort = FALSE;
      ID._loWPort = FALSE;
      ID._memControl = TRUE;
break;

   case 0x39:			// swc1
      ID._opControl = func_swc1;
      ID._memOp = mem_swc1;
      ID._decodedSRC1 = _gpr[i.reg.rs];
      ID._decodedSRC2 = i.imm.imm;
      ID._decodedDST = i.reg.rt;
      ID._writeREG = FALSE;
      ID._writeFREG = FALSE;
      ID._hiWPort = FALSE;
      ID._loWPort = FALSE;
      ID._memControl = TRUE;

if(prev1 !=0 && (prev1 == i.reg.rs || prev1 == i.reg.rt))      decoder_stall = 4;
      else if(prev2 !=0 && (prev2 == i.reg.rs || prev2 == i.reg.rt))      decoder_stall = 3;
      else if(prev3 !=0 && (prev3 == i.reg.rs || prev3 == i.reg.rt))      decoder_stall = 2;

      break;

   case 0x28:			// sb
      ID._opControl = func_sb;
      ID._memOp = mem_sb;
      ID._decodedSRC1 = _gpr[i.reg.rs];
      ID._decodedSRC2 = i.imm.imm;
      ID._decodedDST = i.reg.rt;
      ID._writeREG = FALSE;
      ID._writeFREG = FALSE;
      ID._hiWPort = FALSE;
      ID._loWPort = FALSE;
      ID._memControl = TRUE;
	
	if(EX.load_delay && (EX._decodedDST == i.reg.rs || EX._decodedDST == i.reg.rt))   decoder_stall = 2;
        if(EX.load_delay && (EX._decodedDST == i.reg.rs || EX._decodedDST == i.reg.rt))   num_load_delays += 1;
break;

   case 0x29:			// sh  store half word
      ID._opControl = func_sh;
      ID._memOp = mem_sh;
      ID._decodedSRC1 = _gpr[i.reg.rs];
      ID._decodedSRC2 = i.imm.imm;
      ID._decodedDST = i.reg.rt;
      ID._writeREG = FALSE;
      ID._writeFREG = FALSE;
      ID._hiWPort = FALSE;
      ID._loWPort = FALSE;
      ID._memControl = TRUE;

	if(EX.load_delay && (EX._decodedDST == i.reg.rs || EX._decodedDST == i.reg.rt))   decoder_stall = 2;
        if(EX.load_delay && (EX._decodedDST == i.reg.rs || EX._decodedDST == i.reg.rt))   num_load_delays += 1;
      break;

   case 0x2a:			// swl
      ID._opControl = func_swl;
      ID._memOp = mem_swl;
      ID._decodedSRC1 = _gpr[i.reg.rs];
      ID._decodedSRC2 = i.imm.imm;
      ID._decodedDST = i.reg.rt;
      ID._writeREG = FALSE;
      ID._writeFREG = FALSE;
      ID._hiWPort = FALSE;
      ID._loWPort = FALSE;
      ID._memControl = TRUE;

	if(EX.load_delay && (EX._decodedDST == i.reg.rs || EX._decodedDST == i.reg.rt))   decoder_stall = 2;
        if(EX.load_delay && (EX._decodedDST == i.reg.rs || EX._decodedDST == i.reg.rt))   num_load_delays += 1;

break;

   case 0x2b:			// sw
      ID._opControl = func_sw;
      ID._memOp = mem_sw;
      ID._decodedSRC1 = _gpr[i.reg.rs];
      ID._decodedSRC2 = i.imm.imm;
      ID._decodedDST = i.reg.rt;
      ID._writeREG = FALSE;
      ID._writeFREG = FALSE;
      ID._hiWPort = FALSE;
      ID._loWPort = FALSE;
      ID._memControl = TRUE;

	if(EX.load_delay && (EX._decodedDST == i.reg.rs || EX._decodedDST == i.reg.rt))   decoder_stall = 2;
        if(EX.load_delay && (EX._decodedDST == i.reg.rs || EX._decodedDST == i.reg.rt))   num_load_delays += 1;
break;

   case 0x2e:			// swr
      ID._opControl = func_swr;
      ID._memOp = mem_swr;
      ID._decodedSRC1 = _gpr[i.reg.rs];
      ID._decodedSRC2 = i.imm.imm;
      ID._decodedDST = i.reg.rt;
      ID._writeREG = FALSE;
      ID._writeFREG = FALSE;
      ID._hiWPort = FALSE;
      ID._loWPort = FALSE;
      ID._memControl = TRUE;

	if(EX.load_delay && (EX._decodedDST == i.reg.rs || EX._decodedDST == i.reg.rt))   decoder_stall = 2;
        if(EX.load_delay && (EX._decodedDST == i.reg.rs || EX._decodedDST == i.reg.rt))   num_load_delays += 1;
break;

   case 0x11:			// floating-point
      _fpinst++;
      switch (i.freg.fmt) {
      case 4:			// mtc1
         ID._opControl = func_mtc1;
         ID._decodedSRC1 = _gpr[i.freg.ft];
         ID._decodedDST = i.freg.fs;
         ID._writeREG = FALSE;
         ID._writeFREG = TRUE;
         ID._hiWPort = FALSE;
         ID._loWPort = FALSE;
         ID._memControl = FALSE;
	 break;

      case 0:			// mfc1
         ID._opControl = func_mfc1;
         ID._decodedSRC1 = _fpr[(i.freg.fs)>>1].l[FP_TWIDDLE^((i.freg.fs)&1)];
         ID._decodedDST = i.freg.ft;
         ID._writeREG = TRUE;
         ID._writeFREG = FALSE;
         ID._hiWPort = FALSE;
         ID._loWPort = FALSE;
         ID._memControl = FALSE;
	 break;
      default:
         ID._isIllegalOp = TRUE;
         ID._writeREG = FALSE;
         ID._writeFREG = FALSE;
         ID._hiWPort = FALSE;
         ID._loWPort = FALSE;
         ID._memControl = FALSE;
	 break;
      }
      break;
   default:
      ID._isIllegalOp = TRUE;
      ID._writeREG = FALSE;
      ID._writeFREG = FALSE;
      ID._hiWPort = FALSE;
      ID._loWPort = FALSE;
      ID._memControl = FALSE;
      break;
   }
}


/*
 *
 * Debugging: print registers
 *
 */
void 
Mipc::dumpregs (void)
{
   int i;

   printf ("\n--- PC = %08x ---\n", _pc);
   for (i=0; i < 32; i++) {
      if (i < 10)
	 printf (" r%d: %08x (%ld)\n", i, _gpr[i], _gpr[i]);
      else
	 printf ("r%d: %08x (%ld)\n", i, _gpr[i], _gpr[i]);
   }
   printf ("taken: %d, bd: %d\n", WB._btaken, WB._bdslot);
   printf ("target: %08x\n", WB._btgt);
}

void
Mipc::func_add_addu (Mipc *mc, unsigned ins)
{
    MipsInsn i;
    i.data = ins;
    mc->EX._decodedSRC1 = mc->_gpr_b[i.reg.rs];
    mc->EX._decodedSRC2 = mc->_gpr_b[i.reg.rt];
    mc->EX._opResultLo = mc->EX._decodedSRC1 + mc->EX._decodedSRC2;
   //printf("Encountered unimplemented instruction: add or addu.\n");
   //printf("You need to fill in func_add_addu in exec_helper.cc to proceed forward.\n");
   //exit(0);
}

void
Mipc::func_and (Mipc *mc, unsigned ins)
{	
	MipsInsn i;
    i.data = ins;
    mc->EX._decodedSRC1 = mc->_gpr_b[i.reg.rs];
    mc->EX._decodedSRC2 = mc->_gpr_b[i.reg.rt];
   mc->EX._opResultLo = mc->EX._decodedSRC1 & mc->EX._decodedSRC2;
}

void
Mipc::func_nor (Mipc *mc, unsigned ins)
{	
   MipsInsn i;
    i.data = ins;
    mc->EX._decodedSRC1 = mc->_gpr_b[i.reg.rs];
    mc->EX._decodedSRC2 = mc->_gpr_b[i.reg.rt];
   mc->EX._opResultLo = ~(mc->EX._decodedSRC1 | mc->EX._decodedSRC2);
}

void
Mipc::func_or (Mipc *mc, unsigned ins)
{
	MipsInsn i;
    i.data = ins;
    mc->EX._decodedSRC1 = mc->_gpr_b[i.reg.rs];
    mc->EX._decodedSRC2 = mc->_gpr_b[i.reg.rt];
   mc->EX._opResultLo = mc->EX._decodedSRC1 | mc->EX._decodedSRC2;
}

void
Mipc::func_sll (Mipc *mc, unsigned ins)
{
	MipsInsn i;
    i.data = ins;
    mc->EX._decodedSRC1 = mc->_gpr_b[i.reg.rs];
    mc->EX._decodedSRC2 = mc->_gpr_b[i.reg.rt];
   mc->EX._opResultLo = mc->EX._decodedSRC2 << mc->EX._decodedShiftAmt;
}

void
Mipc::func_sllv (Mipc *mc, unsigned ins)
{
	MipsInsn i;
    i.data = ins;
    mc->EX._decodedSRC1 = mc->_gpr_b[i.reg.rs];
    mc->EX._decodedSRC2 = mc->_gpr_b[i.reg.rt];
   mc->EX._opResultLo = (unsigned)mc->EX._decodedSRC2 << (mc->EX._decodedSRC1 & 0x1f);
   //printf("Encountered unimplemented instruction: sllv.\n");
   //printf("You need to fill in func_sllv in exec_helper.cc to proceed forward.\n");
   //exit(0);
}

void
Mipc::func_slt (Mipc *mc, unsigned ins)
{	
	MipsInsn i;
    i.data = ins;
    mc->EX._decodedSRC1 = mc->_gpr_b[i.reg.rs];
    mc->EX._decodedSRC2 = mc->_gpr_b[i.reg.rt];
   if (mc->EX._decodedSRC1 < mc->EX._decodedSRC2) {
      mc->EX._opResultLo = 1;
   }
   else {
      mc->EX._opResultLo = 0;
   }
}

void
Mipc::func_sltu (Mipc *mc, unsigned ins)
{
	MipsInsn i;
    i.data = ins;
    mc->EX._decodedSRC1 = mc->_gpr_b[i.reg.rs];
    mc->EX._decodedSRC2 = mc->_gpr_b[i.reg.rt];
   if ((unsigned)mc->EX._decodedSRC1 < (unsigned)mc->EX._decodedSRC2) {
      mc->EX._opResultLo = 1;
   }
   else {
      mc->EX._opResultLo = 0;
   }
}

void
Mipc::func_sra (Mipc *mc, unsigned ins)
{
	MipsInsn i;
    i.data = ins;
    mc->EX._decodedSRC1 = mc->_gpr_b[i.reg.rs];
    mc->EX._decodedSRC2 = mc->_gpr_b[i.reg.rt];
   mc->EX._opResultLo = mc->EX._decodedSRC2 >> mc->EX._decodedShiftAmt;
}

void
Mipc::func_srav (Mipc *mc, unsigned ins)
{
	MipsInsn i;
    i.data = ins;
    mc->EX._decodedSRC1 = mc->_gpr_b[i.reg.rs];
    mc->EX._decodedSRC2 = mc->_gpr_b[i.reg.rt];
   mc->EX._opResultLo = mc->EX._decodedSRC2 >> (mc->EX._decodedSRC1 & 0x1f);
}

void
Mipc::func_srl (Mipc *mc, unsigned ins)
{
	MipsInsn i;
    i.data = ins;
    mc->EX._decodedSRC1 = mc->_gpr_b[i.reg.rs];
    mc->EX._decodedSRC2 = mc->_gpr_b[i.reg.rt];
   mc->EX._opResultLo = (unsigned)mc->EX._decodedSRC2 >> mc->EX._decodedShiftAmt;
}

void
Mipc::func_srlv (Mipc *mc, unsigned ins)
{
	MipsInsn i;
    i.data = ins;
    mc->EX._decodedSRC1 = mc->_gpr_b[i.reg.rs];
    mc->EX._decodedSRC2 = mc->_gpr_b[i.reg.rt];
   mc->EX._opResultLo = (unsigned)mc->EX._decodedSRC2 >> (mc->EX._decodedSRC1 & 0x1f);
}

void
Mipc::func_sub_subu (Mipc *mc, unsigned ins)
{
	MipsInsn i;
    i.data = ins;
    mc->EX._decodedSRC1 = mc->_gpr_b[i.reg.rs];
    mc->EX._decodedSRC2 = mc->_gpr_b[i.reg.rt];
   mc->EX._opResultLo = (unsigned)mc->EX._decodedSRC1 - (unsigned)mc->EX._decodedSRC2;
}

void
Mipc::func_xor (Mipc *mc, unsigned ins)
{
	MipsInsn i;
    i.data = ins;
    mc->EX._decodedSRC1 = mc->_gpr_b[i.reg.rs];
    mc->EX._decodedSRC2 = mc->_gpr_b[i.reg.rt];
   mc->EX._opResultLo = mc->EX._decodedSRC1 ^ mc->EX._decodedSRC2;
}

void
Mipc::func_div (Mipc *mc, unsigned ins)
{
	MipsInsn i;
    i.data = ins;
    mc->EX._decodedSRC1 = mc->_gpr_b[i.reg.rs];
    mc->EX._decodedSRC2 = mc->_gpr_b[i.reg.rt];
   if (mc->EX._decodedSRC2 != 0) {
      mc->EX._opResultHi = (unsigned)(mc->EX._decodedSRC1 % mc->EX._decodedSRC2);
      mc->EX._opResultLo = (unsigned)(mc->EX._decodedSRC1 / mc->EX._decodedSRC2);
   }
   else {
      mc->EX._opResultHi = 0x7fffffff;
      mc->EX._opResultLo = 0x7fffffff;
   }
}

void
Mipc::func_divu (Mipc *mc, unsigned ins)
{
	MipsInsn i;
    i.data = ins;
    mc->EX._decodedSRC1 = mc->_gpr_b[i.reg.rs];
    mc->EX._decodedSRC2 = mc->_gpr_b[i.reg.rt];
   if ((unsigned)mc->EX._decodedSRC2 != 0) {
      mc->EX._opResultHi = (unsigned)(mc->EX._decodedSRC1) % (unsigned)(mc->EX._decodedSRC2);
      mc->EX._opResultLo = (unsigned)(mc->EX._decodedSRC1) / (unsigned)(mc->EX._decodedSRC2);
   }
   else {
      mc->EX._opResultHi = 0x7fffffff;
      mc->EX._opResultLo = 0x7fffffff;
   }
}

void
Mipc::func_mfhi (Mipc *mc, unsigned ins)
{
	MipsInsn i;
    i.data = ins;
    mc->EX._decodedSRC1 = mc->_gpr_b[i.reg.rs];
    mc->EX._decodedSRC2 = mc->_gpr_b[i.reg.rt];
   mc->EX._opResultLo = mc->_hi;
}

void
Mipc::func_mflo (Mipc *mc, unsigned ins)
{
	MipsInsn i;
    i.data = ins;
    mc->EX._decodedSRC1 = mc->_gpr_b[i.reg.rs];
    mc->EX._decodedSRC2 = mc->_gpr_b[i.reg.rt];
   mc->EX._opResultLo = mc->_lo;
}

void
Mipc::func_mthi (Mipc *mc, unsigned ins)
{
	MipsInsn i;
    i.data = ins;
    mc->EX._decodedSRC1 = mc->_gpr_b[i.reg.rs];
    mc->EX._decodedSRC2 = mc->_gpr_b[i.reg.rt];
   mc->EX._opResultHi = mc->EX._decodedSRC1;
}

void
Mipc::func_mtlo (Mipc *mc, unsigned ins)
{
	MipsInsn i;
    i.data = ins;
    mc->EX._decodedSRC1 = mc->_gpr_b[i.reg.rs];
    mc->EX._decodedSRC2 = mc->_gpr_b[i.reg.rt];
   mc->EX._opResultLo = mc->EX._decodedSRC1;
}

void
Mipc::func_mult (Mipc *mc, unsigned ins)
{
	MipsInsn i;
    i.data = ins;
    mc->EX._decodedSRC1 = mc->_gpr_b[i.reg.rs];
    mc->EX._decodedSRC2 = mc->_gpr_b[i.reg.rt];
   unsigned int ar1, ar2, s1, s2, r1, r2, t1, t2;
                                                                                
   ar1 = mc->EX._decodedSRC1;
   ar2 = mc->EX._decodedSRC2;
   s1 = ar1 >> 31; if (s1) ar1 = 0x7fffffff & (~ar1 + 1);
   s2 = ar2 >> 31; if (s2) ar2 = 0x7fffffff & (~ar2 + 1);
                                                                                
   t1 = (ar1 & 0xffff) * (ar2 & 0xffff);
   r1 = t1 & 0xffff;              // bottom 16 bits
                                                                                
   // compute next set of 16 bits
   t1 = (ar1 & 0xffff) * (ar2 >> 16) + (t1 >> 16);
   t2 = (ar2 & 0xffff) * (ar1 >> 16);
                                                                                
   r1 = r1 | (((t1+t2) & 0xffff) << 16); // bottom 32 bits
   r2 = (ar1 >> 16) * (ar2 >> 16) + (t1 >> 16) + (t2 >> 16) +
            (((t1 & 0xffff) + (t2 & 0xffff)) >> 16);
                                                                                
   if (s1 ^ s2) {
      r1 = ~r1;
      r2 = ~r2;
      r1++;
      if (r1 == 0)
         r2++;
   }
   mc->EX._opResultHi = r2;
   mc->EX._opResultLo = r1;
}

void
Mipc::func_multu (Mipc *mc, unsigned ins)
{
	MipsInsn i;
    i.data = ins;
    mc->EX._decodedSRC1 = mc->_gpr_b[i.reg.rs];
    mc->EX._decodedSRC2 = mc->_gpr_b[i.reg.rt];

   unsigned int ar1, ar2, s1, s2, r1, r2, t1, t2;
                                                                                
   ar1 = mc->EX._decodedSRC1;
   ar2 = mc->EX._decodedSRC2;
                                                                                
   t1 = (ar1 & 0xffff) * (ar2 & 0xffff);
   r1 = t1 & 0xffff;              // bottom 16 bits
                                                                                
   // compute next set of 16 bits
   t1 = (ar1 & 0xffff) * (ar2 >> 16) + (t1 >> 16);
   t2 = (ar2 & 0xffff) * (ar1 >> 16);
                                                                                
   r1 = r1 | (((t1+t2) & 0xffff) << 16); // bottom 32 bits
   r2 = (ar1 >> 16) * (ar2 >> 16) + (t1 >> 16) + (t2 >> 16) +
            (((t1 & 0xffff) + (t2 & 0xffff)) >> 16);
                            
   mc->EX._opResultHi = r2;
   mc->EX._opResultLo = r1;                                                    
}

void
Mipc::func_jalr (Mipc *mc, unsigned ins)
{
	MipsInsn i;
    i.data = ins;
    mc->EX._decodedSRC1 = mc->_gpr_b[i.reg.rs];
    mc->EX._decodedSRC2 = mc->_gpr_b[i.reg.rt];
    mc->EX._btgt = mc->EX._decodedSRC1;
   mc->EX._btaken = 1;
   mc->_num_jal++;
   mc->EX._opResultLo = mc->_pc ;
}

void
Mipc::func_jr (Mipc *mc, unsigned ins)
{
	 MipsInsn i;
    i.data = ins;
    mc->EX._decodedSRC1 = mc->_gpr_b[i.reg.rs];
    mc->EX._decodedSRC2 = mc->_gpr_b[i.reg.rt];
    mc->EX._btgt = mc->EX._decodedSRC1;
   mc->EX._btaken = 1;
   mc->_num_jr++;
}

void
Mipc::func_await_break (Mipc *mc, unsigned ins)
{
}

void
Mipc::func_syscall (Mipc *mc, unsigned ins)
{
   mc->fake_syscall (ins);
}

void
Mipc::func_addi_addiu (Mipc *mc, unsigned ins)
{
	 MipsInsn i;
    i.data = ins;
    mc->EX._decodedSRC1 = mc->_gpr_b[i.reg.rs];

  SIGN_EXTEND_IMM(mc->EX._decodedSRC2);
  mc->EX._opResultLo = mc->EX._decodedSRC1 + mc->EX._decodedSRC2;
  // printf("Encountered unimplemented instruction: addi or addiu.\n");
  // printf("You need to fill in func_addi_addiu in exec_helper.cc to proceed forward.\n");
  // exit(0);
}

void
Mipc::func_andi (Mipc *mc, unsigned ins)
{
	 MipsInsn i;
    i.data = ins;
    mc->EX._decodedSRC1 = mc->_gpr_b[i.reg.rs];

   mc->EX._opResultLo = mc->EX._decodedSRC1 & mc->EX._decodedSRC2;
}

void
Mipc::func_lui (Mipc *mc, unsigned ins)
{
  // printf("Encountered unimplemented instruction: lui.\n");
  // printf("You need to fill in func_lui in exec_helper.cc to proceed forward.\n");
  // exit(0);
  mc->EX._opResultLo = (mc->EX._decodedSRC2 << 16);
}

void
Mipc::func_ori (Mipc *mc, unsigned ins)
{
	 MipsInsn i;
    i.data = ins;
    mc->EX._decodedSRC1 = mc->_gpr_b[i.reg.rs];

  mc->EX._opResultLo = mc->EX._decodedSRC1 | mc->EX._decodedSRC2;
  // printf("Encountered unimplemented instruction: ori.\n");
  // printf("You need to fill in func_ori in exec_helper.cc to proceed forward.\n");
  // exit(0);
}

void
Mipc::func_slti (Mipc *mc, unsigned ins)
{
	 MipsInsn i;
    i.data = ins;
    mc->EX._decodedSRC1 = mc->_gpr_b[i.reg.rs];

   SIGN_EXTEND_IMM(mc->EX._decodedSRC2);
   if (mc->EX._decodedSRC1 < mc->EX._decodedSRC2) {
      mc->EX._opResultLo = 1;
   }
   else {
      mc->EX._opResultLo = 0;
   }
}

void
Mipc::func_sltiu (Mipc *mc, unsigned ins)
{
	 MipsInsn i;
    i.data = ins;
    mc->EX._decodedSRC1 = mc->_gpr_b[i.reg.rs];

   SIGN_EXTEND_IMM(mc->EX._decodedSRC2);
   if ((unsigned)mc->EX._decodedSRC1 < (unsigned)mc->EX._decodedSRC2) {
      mc->EX._opResultLo = 1;
   }
   else {
      mc->EX._opResultLo = 0;
   }
}

void
Mipc::func_xori (Mipc *mc, unsigned ins)
{
	 MipsInsn i;
    i.data = ins;
    mc->EX._decodedSRC1 = mc->_gpr_b[i.reg.rs];

   mc->EX._opResultLo = mc->EX._decodedSRC1 ^ mc->EX._decodedSRC2;
}

void
Mipc::func_beq (Mipc *mc, unsigned ins)
{
	 MipsInsn i;
    i.data = ins;
    mc->_num_cond_br++;
    mc->EX._decodedSRC1 = mc->_gpr_b[i.reg.rs];
    mc->EX._decodedSRC2 = mc->_gpr_b[i.reg.rt];
   mc->EX._btaken = (mc->EX._decodedSRC1 == mc->EX._decodedSRC2);
   //printf("Encountered unimplemented instruction: beq.\n");
   //printf("You need to fill in func_beq in exec_helper.cc to proceed forward.\n");
   //exit(0);
}

void
Mipc::func_bgez (Mipc *mc, unsigned ins)
{
	MipsInsn i;
    i.data = ins;
    mc->EX._decodedSRC1 = mc->_gpr_b[i.reg.rs];

   mc->_num_cond_br++;
   mc->EX._btaken = !(mc->EX._decodedSRC1 >> 31);
}

void
Mipc::func_bgezal (Mipc *mc, unsigned ins)
{
	MipsInsn i;
    i.data = ins;
    mc->EX._decodedSRC1 = mc->_gpr_b[i.reg.rs];

   mc->_num_cond_br++;
   mc->EX._btaken = !(mc->EX._decodedSRC1 >> 31);
   mc->EX._opResultLo = mc->_pc ;
}

void
Mipc::func_bltzal (Mipc *mc, unsigned ins)
{
	MipsInsn i;
    i.data = ins;
    mc->EX._decodedSRC1 = mc->_gpr_b[i.reg.rs];

   mc->_num_cond_br++;
   mc->EX._btaken = (mc->EX._decodedSRC1 >> 31);
   mc->EX._opResultLo = mc->_pc ;
}

void
Mipc::func_bltz (Mipc *mc, unsigned ins)
{
	MipsInsn i;
    i.data = ins;
    mc->EX._decodedSRC1 = mc->_gpr_b[i.reg.rs];

   mc->_num_cond_br++;
   mc->EX._btaken = (mc->EX._decodedSRC1 >> 31);
}

void
Mipc::func_bgtz (Mipc *mc, unsigned ins)
{
	MipsInsn i;
    i.data = ins;
    mc->EX._decodedSRC1 = mc->_gpr_b[i.reg.rs];

   mc->_num_cond_br++;
   mc->EX._btaken = (mc->EX._decodedSRC1 > 0);
}

void
Mipc::func_blez (Mipc *mc, unsigned ins)
{
	MipsInsn i;
    i.data = ins;
    mc->EX._decodedSRC1 = mc->_gpr_b[i.reg.rs];

   mc->_num_cond_br++;
   mc->EX._btaken = (mc->EX._decodedSRC1 <= 0);
}

void
Mipc::func_bne (Mipc *mc, unsigned ins)
{
	MipsInsn i;
    i.data = ins;
    mc->EX._decodedSRC1 = mc->_gpr_b[i.reg.rs];
    mc->EX._decodedSRC2 = mc->_gpr_b[i.reg.rt];

   mc->_num_cond_br++;
   mc->EX._btaken = (mc->EX._decodedSRC1 != mc->EX._decodedSRC2);
}

void
Mipc::func_j (Mipc *mc, unsigned ins)
{
   mc->EX._btaken = 1;
}

void
Mipc::func_jal (Mipc *mc, unsigned ins)
{
   mc->_num_jal++;
   mc->EX._btaken = 1;
   mc->EX._opResultLo = mc->_pc ;
   //printf("Encountered unimplemented instruction: jal.\n");
   //printf("You need to fill in func_jal in exec_helper.cc to proceed forward.\n");
   //exit(0);
}

void
Mipc::func_lb (Mipc *mc, unsigned ins)
{
   signed int a1;
    MipsInsn i;
    i.data = ins;
    mc->EX._decodedSRC1 = mc->_gpr_b[i.reg.rs];
   mc->_num_load++;
   SIGN_EXTEND_IMM(mc->EX._decodedSRC2);
   mc->EX._memory_addr_reg = (unsigned)(mc->EX._decodedSRC1+mc->EX._decodedSRC2);
}

void
Mipc::func_lbu (Mipc *mc, unsigned ins)
{
	 MipsInsn i;
    i.data = ins;
    mc->EX._decodedSRC1 = mc->_gpr_b[i.reg.rs];

   mc->_num_load++;
   SIGN_EXTEND_IMM(mc->EX._decodedSRC2);
   mc->EX._memory_addr_reg = (unsigned)(mc->EX._decodedSRC1+mc->EX._decodedSRC2);
}

void
Mipc::func_lh (Mipc *mc, unsigned ins)
{
	 MipsInsn i;
    i.data = ins;
    mc->EX._decodedSRC1 = mc->_gpr_b[i.reg.rs];

   signed int a1;
                                                                                
   mc->_num_load++;
   SIGN_EXTEND_IMM(mc->EX._decodedSRC2);
   mc->EX._memory_addr_reg = (unsigned)(mc->EX._decodedSRC1+mc->EX._decodedSRC2);
}

void
Mipc::func_lhu (Mipc *mc, unsigned ins)
{
	 MipsInsn i;
    i.data = ins;
    mc->EX._decodedSRC1 = mc->_gpr_b[i.reg.rs];

   mc->_num_load++;
   SIGN_EXTEND_IMM(mc->EX._decodedSRC2);
   mc->EX._memory_addr_reg = (unsigned)(mc->EX._decodedSRC1+mc->EX._decodedSRC2);
}

void
Mipc::func_lwl (Mipc *mc, unsigned ins)
{
	 MipsInsn i;
    i.data = ins;
    mc->EX._decodedSRC1 = mc->_gpr_b[i.reg.rs];
    mc->EX._subregOperand = mc->_gpr_b[i.reg.rt];

   signed int a1;
   unsigned s1;
                                                                             
   mc->_num_load++;
   SIGN_EXTEND_IMM(mc->EX._decodedSRC2);
   mc->EX._memory_addr_reg = (unsigned)(mc->EX._decodedSRC1+mc->EX._decodedSRC2);
}

void
Mipc::func_lw (Mipc *mc, unsigned ins)
{
	 MipsInsn i;
    i.data = ins;
    mc->EX._decodedSRC1 = mc->_gpr_b[i.reg.rs];

   mc->_num_load++;
   SIGN_EXTEND_IMM(mc->EX._decodedSRC2);
   mc->EX._memory_addr_reg = (unsigned)(mc->EX._decodedSRC1+mc->EX._decodedSRC2);
   //printf("Encountered unimplemented instruction: lw.\n");
   // printf("You need to fill in func_lw in exec_helper.cc to proceed forward.\n");
   //exit(0);
}

void
Mipc::func_lwr (Mipc *mc, unsigned ins)
{
	MipsInsn i;
    i.data = ins;
    mc->EX._decodedSRC1 = mc->_gpr_b[i.reg.rs];
    mc->EX._subregOperand = mc->_gpr_b[i.reg.rt];

   unsigned ar1, s1;
                                                                                
   mc->_num_load++;
   SIGN_EXTEND_IMM(mc->EX._decodedSRC2);
   mc->EX._memory_addr_reg = (unsigned)(mc->EX._decodedSRC1+mc->EX._decodedSRC2);
}

void
Mipc::func_lwc1 (Mipc *mc, unsigned ins)
{
	MipsInsn i;
    i.data = ins;
    mc->EX._decodedSRC1 = mc->_gpr_b[i.reg.rs];

   mc->_num_load++;
   SIGN_EXTEND_IMM(mc->EX._decodedSRC2);
   mc->EX._memory_addr_reg = (unsigned)(mc->EX._decodedSRC1+mc->EX._decodedSRC2);
}

void
Mipc::func_swc1 (Mipc *mc, unsigned ins)
{
	MipsInsn i;
    i.data = ins;
    mc->EX._decodedSRC1 = mc->_gpr_b[i.reg.rs];

   mc->_num_store++;
   SIGN_EXTEND_IMM(mc->EX._decodedSRC2);
   mc->EX._memory_addr_reg = (unsigned)(mc->EX._decodedSRC1+mc->EX._decodedSRC2);
}

void
Mipc::func_sb (Mipc *mc, unsigned ins)
{
	MipsInsn i;
    i.data = ins;
    mc->EX._decodedSRC1 = mc->_gpr_b[i.reg.rs];

   mc->_num_store++;
   SIGN_EXTEND_IMM(mc->EX._decodedSRC2);
   mc->EX._memory_addr_reg = (unsigned)(mc->EX._decodedSRC1+mc->EX._decodedSRC2);
}

void
Mipc::func_sh (Mipc *mc, unsigned ins)
{
	MipsInsn i;
    i.data = ins;
    mc->EX._decodedSRC1 = mc->_gpr_b[i.reg.rs];

   mc->_num_store++;
   SIGN_EXTEND_IMM(mc->EX._decodedSRC2);
   mc->EX._memory_addr_reg = (unsigned)(mc->EX._decodedSRC1+mc->EX._decodedSRC2);
}

void
Mipc::func_swl (Mipc *mc, unsigned ins)
{
	MipsInsn i;
    i.data = ins;
    mc->EX._decodedSRC1 = mc->_gpr_b[i.reg.rs];

   unsigned ar1, s1;
                                                                                
   mc->_num_store++;
   SIGN_EXTEND_IMM(mc->EX._decodedSRC2);
   mc->EX._memory_addr_reg = (unsigned)(mc->EX._decodedSRC1+mc->EX._decodedSRC2);
}

void
Mipc::func_sw (Mipc *mc, unsigned ins)
{
	MipsInsn i;
    i.data = ins;
    mc->EX._decodedSRC1 = mc->_gpr_b[i.reg.rs];

   mc->_num_store++;
   SIGN_EXTEND_IMM(mc->EX._decodedSRC2);
   mc->EX._memory_addr_reg = (unsigned)(mc->EX._decodedSRC1+mc->EX._decodedSRC2);
}

void
Mipc::func_swr (Mipc *mc, unsigned ins)
{
	MipsInsn i;
    i.data = ins;
    mc->EX._decodedSRC1 = mc->_gpr_b[i.reg.rs];

   unsigned ar1, s1;
                                                                            
   mc->_num_store++;
   SIGN_EXTEND_IMM(mc->EX._decodedSRC2);
   mc->EX._memory_addr_reg = (unsigned)(mc->EX._decodedSRC1+mc->EX._decodedSRC2);
}

void
Mipc::func_mtc1 (Mipc *mc, unsigned ins)
{
   mc->EX._opResultLo = mc->EX._decodedSRC1;
}

void
Mipc::func_mfc1 (Mipc *mc, unsigned ins)
{
   mc->EX._opResultLo = mc->EX._decodedSRC1;
}



void
Mipc::mem_lb (Mipc *mc)
{
   signed int a1;

   a1 = mc->_mem->BEGetByte(mc->MEM._memory_addr_reg, mc->_mem->Read(mc->MEM._memory_addr_reg & ~(LL)0x7));
   SIGN_EXTEND_BYTE(a1);
   mc->MEM._opResultLo = a1;
}

void
Mipc::mem_lbu (Mipc *mc)
{
   mc->MEM._opResultLo = mc->_mem->BEGetByte(mc->MEM._memory_addr_reg, mc->_mem->Read(mc->MEM._memory_addr_reg & ~(LL)0x7));
}

void
Mipc::mem_lh (Mipc *mc)
{
   signed int a1;

   a1 = mc->_mem->BEGetHalfWord(mc->MEM._memory_addr_reg, mc->_mem->Read(mc->MEM._memory_addr_reg & ~(LL)0x7));
   SIGN_EXTEND_IMM(a1);
   mc->MEM._opResultLo = a1;
}

void
Mipc::mem_lhu (Mipc *mc)
{
   mc->MEM._opResultLo = mc->_mem->BEGetHalfWord (mc->MEM._memory_addr_reg, mc->_mem->Read(mc->MEM._memory_addr_reg & ~(LL)0x7));
}

void
Mipc::mem_lwl (Mipc *mc)
{
   signed int a1;
   unsigned s1;

   a1 = mc->_mem->BEGetWord (mc->MEM._memory_addr_reg, mc->_mem->Read(mc->MEM._memory_addr_reg & ~(LL)0x7));
   s1 = (mc->MEM._memory_addr_reg & 3) << 3;
   mc->MEM._opResultLo = (a1 << s1) | (mc->MEM._subregOperand & ~(~0UL << s1));
}

void
Mipc::mem_lw (Mipc *mc)
{
   mc->MEM._opResultLo = mc->_mem->BEGetWord (mc->MEM._memory_addr_reg, mc->_mem->Read(mc->MEM._memory_addr_reg & ~(LL)0x7));
}

void
Mipc::mem_lwr (Mipc *mc)
{
   unsigned ar1, s1;

   ar1 = mc->_mem->BEGetWord (mc->MEM._memory_addr_reg, mc->_mem->Read(mc->MEM._memory_addr_reg & ~(LL)0x7));
   s1 = (~mc->MEM._memory_addr_reg & 3) << 3;
   mc->MEM._opResultLo = (ar1 >> s1) | (mc->MEM._subregOperand & ~(~(unsigned)0 >> s1));
}

void
Mipc::mem_lwc1 (Mipc *mc)
{
   mc->MEM._opResultLo = mc->_mem->BEGetWord (mc->MEM._memory_addr_reg, mc->_mem->Read(mc->MEM._memory_addr_reg & ~(LL)0x7));
}

void
Mipc::mem_swc1 (Mipc *mc)
{
   mc->_mem->Write(mc->MEM._memory_addr_reg & ~(LL)0x7, mc->_mem->BESetWord (mc->MEM._memory_addr_reg, mc->_mem->Read(mc->MEM._memory_addr_reg & ~(LL)0x7), mc->_fpr[mc->MEM._decodedDST>>1].l[FP_TWIDDLE^(mc->MEM._decodedDST&1)]));
}

void
Mipc::mem_sb (Mipc *mc)
{
   mc->_mem->Write(mc->MEM._memory_addr_reg & ~(LL)0x7, mc->_mem->BESetByte (mc->MEM._memory_addr_reg, mc->_mem->Read(mc->MEM._memory_addr_reg & ~(LL)0x7), mc->_gpr_b[mc->MEM._decodedDST] & 0xff));
}

void
Mipc::mem_sh (Mipc *mc)
{
   mc->_mem->Write(mc->MEM._memory_addr_reg & ~(LL)0x7, mc->_mem->BESetHalfWord (mc->MEM._memory_addr_reg, mc->_mem->Read(mc->MEM._memory_addr_reg & ~(LL)0x7), mc->_gpr_b[mc->MEM._decodedDST] & 0xffff));
}

void
Mipc::mem_swl (Mipc *mc)
{
   unsigned ar1, s1;

   ar1 = mc->_mem->BEGetWord (mc->MEM._memory_addr_reg, mc->_mem->Read(mc->MEM._memory_addr_reg & ~(LL)0x7));
   s1 = (mc->MEM._memory_addr_reg & 3) << 3;
   ar1 = (mc->_gpr_b[mc->MEM._decodedDST] >> s1) | (ar1 & ~(~(unsigned)0 >> s1));
   mc->_mem->Write(mc->MEM._memory_addr_reg & ~(LL)0x7, mc->_mem->BESetWord (mc->MEM._memory_addr_reg, mc->_mem->Read(mc->MEM._memory_addr_reg & ~(LL)0x7), ar1));
}

void
Mipc::mem_sw (Mipc *mc)
{
   mc->_mem->Write(mc->MEM._memory_addr_reg & ~(LL)0x7, mc->_mem->BESetWord (mc->MEM._memory_addr_reg, mc->_mem->Read(mc->MEM._memory_addr_reg & ~(LL)0x7), mc->_gpr_b[mc->MEM._decodedDST]));
}

void
Mipc::mem_swr (Mipc *mc)
{
   unsigned ar1, s1;

   ar1 = mc->_mem->BEGetWord (mc->MEM._memory_addr_reg, mc->_mem->Read(mc->MEM._memory_addr_reg & ~(LL)0x7));
   s1 = (~mc->MEM._memory_addr_reg & 3) << 3;
   ar1 = (mc->_gpr_b[mc->MEM._decodedDST] << s1) | (ar1 & ~(~0UL << s1));
   mc->_mem->Write(mc->MEM._memory_addr_reg & ~(LL)0x7, mc->_mem->BESetWord (mc->MEM._memory_addr_reg, mc->_mem->Read(mc->MEM._memory_addr_reg & ~(LL)0x7), ar1));
}
