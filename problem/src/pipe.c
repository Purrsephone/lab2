/*
 * CMSC 22200
 *
 * ARM pipeline timing simulator
 */

#include "pipe.h"
#include "shell.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <inttypes.h>


// Bhakti Shah, cnet: bhaktishah
// Sophie Veys, cnet: sophiev

uint64_t pc;
uint64_t hlt_pc;

int cycles = 0;
int stall = 0;

Pipe_Reg_IFtoDE IF_to_DE = {
    .pc = 0
};

Pipe_Reg_DEtoEX DE_to_EX = {
	.pc = 0,
	.instr_data = {
        .rm = 0,
    	.shamt = 0,
    	.rn = 0,
    	.rd = 0,
    	.immediate = 0,
    	.address = 0,
    	.op2 = 0,
    	.rt = 0
    },
	.decoded_instr = {
        .opcode = 0,
    	.type = 0
    },
    .res = 0,
    .dep = 0
};

Pipe_Reg_EXtoMEM EX_to_MEM = {
	.pc = 0,
	.instr_data = {
        .rm = 0,
    	.shamt = 0,
    	.rn = 0,
    	.rd = 0,
    	.immediate = 0,
    	.address = 0,
    	.op2 = 0,
    	.rt = 0
    },
	.decoded_instr = {
        .opcode = 0,
    	.type = 0
    },
    .res = 0
};

Pipe_Reg_MEMtoWB MEM_to_WB = {
	.pc = 0,
	.instr_data = {
        .rm = 0,
    	.shamt = 0,
    	.rn = 0,
    	.rd = 0,
    	.immediate = 0,
    	.address = 0,
    	.op2 = 0,
    	.rt = 0
    },
	.decoded_instr = {
        .opcode = 0,
    	.type = 0
    }
};

/* global pipeline state */
CPU_State CURRENT_STATE;

void pipe_init()
{
    memset(&CURRENT_STATE, 0, sizeof(CPU_State));
    CURRENT_STATE.PC = 0x00400000;
}

void pipe_cycle()
{
	pipe_stage_wb();
	pipe_stage_mem();
    pipe_stage_execute();
    //only do these stages if we don't need to stall
    if(stall == 0) {
    	pipe_stage_decode();
    	pipe_stage_fetch();
    	//will need to change later
    	CURRENT_STATE.PC = CURRENT_STATE.PC + 4;
    }
    if(stall == 1)
    {
        stall = 0;
    }
    cycles++;
//	if(MEM_to_WB.decoded_instr.opcode == HLT) {
//		stat_cycles++;
//		stat_inst_retire++;
//		RUN_BIT = 0;
//	}
}

//DE_to_EX.res will be the value that we grabbed
//DE_to_EX.dep will tell us which value (if any), to replace
// .dep = 0 (no dependency), 1 (rm), 2 (rn)
void check_dependency()
{
    if(cycles >= 3)
    {
        if(EX_to_MEM.instr_data.rm == MEM_to_WB.instr_data.rd) {
            DE_to_EX.res = MEM_to_WB.res;
            DE_to_EX.dep = 1;
        }
        //check if rn dependent
        else if(EX_to_MEM.instr_data.rn == MEM_to_WB.instr_data.rd) {
            DE_to_EX.res = MEM_to_WB.res;
            DE_to_EX.dep = 2;
        }
        else if((MEM_to_WB.decoded_instr.opcode == LDUR64) ||
        (MEM_to_WB.decoded_instr.opcode == LDUR32) ||
        (MEM_to_WB.decoded_instr.opcode == LDURB) ||
        (MEM_to_WB.decoded_instr.opcode == LDURH)) {
            if(EX_to_MEM.instr_data.rm == MEM_to_WB.instr_data.rt) {
                stall = 1;
            }
            if(EX_to_MEM.instr_data.rn == MEM_to_WB.instr_data.rt) {
                stall = 1;
            }
        }
        else {
            DE_to_EX.dep = 0;
        }
        printf("DEBUG check depen\n");
        printf("MEM_to_WB.res %li\n", MEM_to_WB.res);
        printf("DE_to_EX.dep %i\n", DE_to_EX.dep);
        //check  if instr dependent on another instr in memory stage
    }
}

void pipe_stage_wb()
{
    printf("%s\n", "writeback");
    printf("%u\n", MEM_to_WB.decoded_instr.opcode);
    printf("mem to wb: %li \n", MEM_to_WB.res);


    // CBNZ, CBZ, STUR64, STUR32, STURB, STURH, HLT
    // BR, B, BEQ, BNE, BGT, BLT, BGE, BLE skip writeback

    if ((MEM_to_WB.decoded_instr.opcode == ADD) ||
        (MEM_to_WB.decoded_instr.opcode == ADDI) ||
        (MEM_to_WB.decoded_instr.opcode == SUB) ||
        (MEM_to_WB.decoded_instr.opcode == SUBI) ||
        (MEM_to_WB.decoded_instr.opcode == MUL) ||
        (MEM_to_WB.decoded_instr.opcode == AND) ||
        (MEM_to_WB.decoded_instr.opcode == EOR) ||
        (MEM_to_WB.decoded_instr.opcode == ORR) ||
        (MEM_to_WB.decoded_instr.opcode == LSLI) ||
        (MEM_to_WB.decoded_instr.opcode == LSRI) ||
        (MEM_to_WB.decoded_instr.opcode == MOVZ) ||
        (MEM_to_WB.decoded_instr.opcode == BUB)) {
        // write to reg
        CURRENT_STATE.REGS[MEM_to_WB.instr_data.rd] = MEM_to_WB.res;
    }
    if (MEM_to_WB.decoded_instr.opcode == ADDS) {
        // write to reg
        CURRENT_STATE.REGS[MEM_to_WB.instr_data.rd] = MEM_to_WB.res;
        // update flags
    	if(MEM_to_WB.res == 0) {
    		CURRENT_STATE.FLAG_Z = 1;
    	}
    	if(MEM_to_WB.res < 0) {
    		CURRENT_STATE.FLAG_N = 1;
    	}

    }
    if (MEM_to_WB.decoded_instr.opcode == ADDSI) {
        // write to reg
	    CURRENT_STATE.REGS[MEM_to_WB.instr_data.rd] = MEM_to_WB.res;
        // update flags
    	if (MEM_to_WB.res == 0) {
    	    CURRENT_STATE.FLAG_Z = 1;
    		CURRENT_STATE.FLAG_N = 0;
    	}
    	if (MEM_to_WB.res < 0) {
        	CURRENT_STATE.FLAG_N = 1;
        	CURRENT_STATE.FLAG_Z = 0;
    	}
    	if (MEM_to_WB.res > 0) {
            CURRENT_STATE.FLAG_N = 0;
            CURRENT_STATE.FLAG_Z = 0;
    	}

    }
    if (MEM_to_WB.decoded_instr.opcode == ANDS) {
        // write to reg
        CURRENT_STATE.REGS[MEM_to_WB.instr_data.rd] = MEM_to_WB.res;
        // update flags
        if (MEM_to_WB.res == 0) {
            CURRENT_STATE.FLAG_Z = 1;
        }
        if (MEM_to_WB.res < 0) {
            CURRENT_STATE.FLAG_N = 1;
        }

    }
    // if a load instruction
    if ((MEM_to_WB.decoded_instr.opcode == LDUR64) ||
        (MEM_to_WB.decoded_instr.opcode == LDUR32) ||
        (MEM_to_WB.decoded_instr.opcode == LDURB) ||
        (MEM_to_WB.decoded_instr.opcode == LDURH)) {
        // write result from memory to register
	CURRENT_STATE.REGS[MEM_to_WB.instr_data.rt] = MEM_to_WB.res;

    }
    if (MEM_to_WB.decoded_instr.opcode == SUBS) {
        if (MEM_to_WB.instr_data.rd != 31) {
            CURRENT_STATE.REGS[MEM_to_WB.instr_data.rd] = MEM_to_WB.res;
        }
        if (MEM_to_WB.res == 0) {
            CURRENT_STATE.FLAG_Z = 1;
            CURRENT_STATE.FLAG_N = 0;
        }
        if (MEM_to_WB.res < 0) {
            CURRENT_STATE.FLAG_N = 1;
            CURRENT_STATE.FLAG_Z = 0;
        }
        if (MEM_to_WB.res > 0) {
            CURRENT_STATE.FLAG_N = 0;
            CURRENT_STATE.FLAG_Z = 0;
        }
    }
    if (MEM_to_WB.decoded_instr.opcode == SUBSI) {
        CURRENT_STATE.REGS[MEM_to_WB.instr_data.rd] = MEM_to_WB.res;
        if (MEM_to_WB.res == 0) {
            CURRENT_STATE.FLAG_Z = 1;
            CURRENT_STATE.FLAG_N = 0;
        }
        if (MEM_to_WB.res < 0) {
            CURRENT_STATE.FLAG_N = 1;
            CURRENT_STATE.FLAG_Z = 0;
        }
        if (MEM_to_WB.res > 0) {
            CURRENT_STATE.FLAG_N = 0;
            CURRENT_STATE.FLAG_Z = 0;
        }


    }
    if (MEM_to_WB.decoded_instr.opcode == HLT) {
	//     printf("meow!!");
	     CURRENT_STATE.PC = hlt_pc;
	     //stat_cycles++;
	     stat_inst_retire++;
             RUN_BIT = 0;
	     return;
      }
	if((cycles > 3) && (MEM_to_WB.decoded_instr.opcode != BUB))
	{
		stat_inst_retire++;
	}
}

void pipe_stage_mem()
{
    printf("%u\n", EX_to_MEM.decoded_instr.opcode);
    printf("%s\n", "memory");
    printf("stat cycle: %d\n", stat_cycles);

    if(stall == 0) {

    //pass along info from one stage to next
	MEM_to_WB.decoded_instr = EX_to_MEM.decoded_instr;
    MEM_to_WB.instr_data = EX_to_MEM.instr_data;
    MEM_to_WB.res = EX_to_MEM.res;
    printf("MEM TO WB res %li\n", MEM_to_WB.res);
    // ADD, ADDI, ADDS, ADDSI, CBNZ, CBZ, AND, ANDS, EOR, ORR
    // LSLI, LSRI, MOVZ, SUB, SUBI, SUBS, SUBSI, MUL, CMP, CMPI
    // BR, B, BEQ, BNE, BGT, BLT, BGE, BLE can SKIP mem stage

    // LDUR64, LDUR32, LDURB, LDURH, STUR64, STUR32, STURB, STURH

    // when HLT is in mem stage, we want to stop running the program
     if (EX_to_MEM.decoded_instr.opcode == LDUR64) {
         // go into memory
	     uint32_t first_32 = mem_read_32(EX_to_MEM.res);
         uint32_t second_32 = mem_read_32(EX_to_MEM.res + 4);
         int64_t doubleword = ((uint64_t)second_32 << 32) | first_32;
         // pass result into writeback
	     MEM_to_WB.res = doubleword;
     }
     if (EX_to_MEM.decoded_instr.opcode == LDUR32) {
        // go into memory
        // pass result into writeback
        MEM_to_WB.res = mem_read_32(EX_to_MEM.res);
     }
     if (EX_to_MEM.decoded_instr.opcode == LDURB) {
         // go into memory
         // pass result into writeback
         MEM_to_WB.res = (mem_read_32(EX_to_MEM.res) & 0xFF);
     }
     if (EX_to_MEM.decoded_instr.opcode == LDURH) {
         // go into memory
         // pass result into writeback
         MEM_to_WB.res = (mem_read_32(EX_to_MEM.res) & 0xFFFF);
     }
     if (EX_to_MEM.decoded_instr.opcode == STUR64) {
	     printf("stur64");
         uint32_t first_32 = (uint32_t)((CURRENT_STATE.REGS[EX_to_MEM.instr_data.rt] & 0xFFFFFFFF00000000LL) >> 32);
         uint32_t second_32 = (uint32_t)(CURRENT_STATE.REGS[EX_to_MEM.instr_data.rt] & 0xFFFFFFFFLL);
         // write result into memory
         mem_write_32(EX_to_MEM.res, second_32);
         mem_write_32((EX_to_MEM.res + 4), first_32);
     }
     if (EX_to_MEM.decoded_instr.opcode == STUR32) {
         // write result into memory
	 printf("stur32");
         mem_write_32(EX_to_MEM.res, CURRENT_STATE.REGS[EX_to_MEM.instr_data.rt]);
     }
     if (EX_to_MEM.decoded_instr.opcode == STURB) {
         // write result into memory
	 // printf("sturb\n");
	 //printf("rt %li\n", CURRENT_STATE.REGS[EX_to_MEM.instr_data.rt] & 0xFF);
         mem_write_32(EX_to_MEM.res, (CURRENT_STATE.REGS[EX_to_MEM.instr_data.rt] & 0xFF));
     }
     if (EX_to_MEM.decoded_instr.opcode == STURH) {
         // write result into memory
	 printf("sturh");
         mem_write_32(EX_to_MEM.res, (CURRENT_STATE.REGS[EX_to_MEM.instr_data.rt] & 0xFFFF));
     }
     if (EX_to_MEM.decoded_instr.opcode == HLT) {
	     CURRENT_STATE.PC = hlt_pc;
     }
    }
}

void pipe_stage_execute()
{
    printf("%u\n", DE_to_EX.decoded_instr.opcode);
    printf("%s\n", "execute");

    // check dependency

    // pass along info from one stage to next
	EX_to_MEM.decoded_instr = DE_to_EX.decoded_instr;
	EX_to_MEM.instr_data = DE_to_EX.instr_data;


	check_dependency();

    //do a nonsense instr, like set 0 reg = 0
    if(stall == 1) {
        EX_to_MEM.decoded_instr.opcode = BUB;
        EX_to_MEM.res = 0;
        EX_to_MEM.instr_data.rd = 31;
    }

    if(stall == 0) {
        // do addition
        if ((DE_to_EX.decoded_instr.opcode == ADDS) ||
            (DE_to_EX.decoded_instr.opcode == ADD)) {
                // no dependency
                if(DE_to_EX.dep == 0) {
                    EX_to_MEM.res = CURRENT_STATE.REGS[DE_to_EX.instr_data.rm]
                    + CURRENT_STATE.REGS[DE_to_EX.instr_data.rn];
                }
                // replace rm with forwarded value
                if(DE_to_EX.dep == 1) {
                    EX_to_MEM.res = DE_to_EX.res + CURRENT_STATE.REGS[DE_to_EX.instr_data.rn];
                }
                // replace rn with forwarded value
                if(DE_to_EX.dep == 2) {
                    EX_to_MEM.res = DE_to_EX.res + CURRENT_STATE.REGS[DE_to_EX.instr_data.rm];
                }
        }
        if ((DE_to_EX.decoded_instr.opcode == ADDSI) ||
            (DE_to_EX.decoded_instr.opcode == ADDI)) {
                // no dependency
                if((DE_to_EX.dep == 0) || (DE_to_EX.dep == 1)) {
                     EX_to_MEM.res = CURRENT_STATE.REGS[DE_to_EX.instr_data.rn] + DE_to_EX.instr_data.immediate;
                }
                // replace rn with forwarded value
                if(DE_to_EX.dep == 2) {
                    EX_to_MEM.res = DE_to_EX.res + DE_to_EX.instr_data.immediate;
                }
        }
        // do branching
        // TODO
        if (DE_to_EX.decoded_instr.opcode == CBNZ) {
            if (CURRENT_STATE.REGS[DE_to_EX.instr_data.rt]) {
                CURRENT_STATE.PC = ((int64_t) CURRENT_STATE.PC)
                    + DE_to_EX.instr_data.address;
            }
            else {
                CURRENT_STATE.PC = CURRENT_STATE.PC + 4;
            }
        }
        // TODO
        if (DE_to_EX.decoded_instr.opcode == CBZ) {
            if (!CURRENT_STATE.REGS[DE_to_EX.instr_data.rt]) {
                CURRENT_STATE.PC = ((int64_t) CURRENT_STATE.PC)
                    + DE_to_EX.instr_data.address;
            }
            else {
                CURRENT_STATE.PC = CURRENT_STATE.PC + 4;
            }
        }
        // TODO
        if (DE_to_EX.decoded_instr.opcode == BR) {
            CURRENT_STATE.PC = CURRENT_STATE.REGS[DE_to_EX.instr_data.rn];
        }
        // TODO
        if (DE_to_EX.decoded_instr.opcode == B) {
            CURRENT_STATE.PC = CURRENT_STATE.PC + DE_to_EX.instr_data.address;
        }
        // TODO
        if (DE_to_EX.decoded_instr.opcode == BEQ) {
            if (CURRENT_STATE.FLAG_Z) {
                CURRENT_STATE.PC = (int64_t) (CURRENT_STATE.PC) + (DE_to_EX.instr_data.address);
            }
            else {
                CURRENT_STATE.PC = CURRENT_STATE.PC + 4;
            }
        }
        // TODO
        if (DE_to_EX.decoded_instr.opcode == BNE) {
            if (!CURRENT_STATE.FLAG_Z) {
                CURRENT_STATE.PC = (int64_t) (CURRENT_STATE.PC) + (DE_to_EX.instr_data.address);
            }
            else {
                CURRENT_STATE.PC = CURRENT_STATE.PC + 4;
            }
        }
        // TODO
        if (DE_to_EX.decoded_instr.opcode == BGT) {
            if (!CURRENT_STATE.FLAG_N && (!CURRENT_STATE.FLAG_Z)) {
                CURRENT_STATE.PC = ((int64_t) CURRENT_STATE.PC)
                    + DE_to_EX.instr_data.address;
            }
            else {
                CURRENT_STATE.PC = CURRENT_STATE.PC + 4;
            }
        }
        // TODO
        if (DE_to_EX.decoded_instr.opcode == BLT) {
            if (CURRENT_STATE.FLAG_N && (!CURRENT_STATE.FLAG_Z)) {
                CURRENT_STATE.PC = CURRENT_STATE.PC + DE_to_EX.instr_data.address;
            }
            else {
                CURRENT_STATE.PC = CURRENT_STATE.PC + 4;
            }
        }
        // TODO
        if (DE_to_EX.decoded_instr.opcode == BGE) {
            if ((!CURRENT_STATE.FLAG_N) || (CURRENT_STATE.FLAG_Z)) {
                CURRENT_STATE.PC = CURRENT_STATE.PC + DE_to_EX.instr_data.address;
            }
            else {
                CURRENT_STATE.PC = CURRENT_STATE.PC + 4;
            }
        }
        // TODO
        if (DE_to_EX.decoded_instr.opcode == BLE) {
            if ((CURRENT_STATE.FLAG_N) || (CURRENT_STATE.FLAG_Z)) {
                CURRENT_STATE.PC = CURRENT_STATE.PC + DE_to_EX.instr_data.address;
            }
            else {
                CURRENT_STATE.PC = CURRENT_STATE.PC + 4;
            }
        }
        if ((DE_to_EX.decoded_instr.opcode == AND) ||
            (DE_to_EX.decoded_instr.opcode == ANDS)) {
                if(DE_to_EX.dep == 0) {
                    EX_to_MEM.res = CURRENT_STATE.REGS[DE_to_EX.instr_data.rm]
                    & CURRENT_STATE.REGS[DE_to_EX.instr_data.rn];
                }
                if(DE_to_EX.dep == 1) {
                    EX_to_MEM.res = DE_to_EX.res & CURRENT_STATE.REGS[DE_to_EX.instr_data.rn];
                }
                if(DE_to_EX.dep == 2) {
                    EX_to_MEM.res = DE_to_EX.res & CURRENT_STATE.REGS[DE_to_EX.instr_data.rm];
                }
        }
        if (DE_to_EX.decoded_instr.opcode == EOR) {
            if(DE_to_EX.dep == 0) {
                EX_to_MEM.res = CURRENT_STATE.REGS[DE_to_EX.instr_data.rm]
                ^ CURRENT_STATE.REGS[DE_to_EX.instr_data.rn];
            }
            if(DE_to_EX.dep == 1) {
                EX_to_MEM.res = DE_to_EX.res ^ CURRENT_STATE.REGS[DE_to_EX.instr_data.rn];
            }
            if(DE_to_EX.dep == 2) {
                EX_to_MEM.res = DE_to_EX.res ^ CURRENT_STATE.REGS[DE_to_EX.instr_data.rm];
            }
        }
        if (DE_to_EX.decoded_instr.opcode == ORR) {
                if(DE_to_EX.dep == 0) {
                    EX_to_MEM.res = CURRENT_STATE.REGS[DE_to_EX.instr_data.rm]
                    | CURRENT_STATE.REGS[DE_to_EX.instr_data.rn];
                }
                if(DE_to_EX.dep == 1) {
                    EX_to_MEM.res = DE_to_EX.res | CURRENT_STATE.REGS[DE_to_EX.instr_data.rn];
                }
                if(DE_to_EX.dep == 2) {
                    EX_to_MEM.res = DE_to_EX.res | CURRENT_STATE.REGS[DE_to_EX.instr_data.rm];
                }
        }

    	if ((DE_to_EX.decoded_instr.opcode == LDUR64) ||
            (DE_to_EX.decoded_instr.opcode == LDUR32) ||
            (DE_to_EX.decoded_instr.opcode == LDURH)) {
                uint64_t final_addr;
                if ((DE_to_EX.dep == 0) || (DE_to_EX.dep == 1)) {
                     final_addr = (DE_to_EX.instr_data.address + CURRENT_STATE.REGS[DE_to_EX.instr_data.rn]);
                }
                if (DE_to_EX.dep == 2) {
                    final_addr = DE_to_EX.instr_data.address + DE_to_EX.res;
                }
    			while ((final_addr & 0x3) != 0) {
    				final_addr = final_addr + 1;
    			}
    			EX_to_MEM.res = final_addr;
    	}
    	if (DE_to_EX.decoded_instr.opcode == LDURB)
    	{
            uint64_t final_addr;
            if ((DE_to_EX.dep == 0) || (DE_to_EX.dep == 1)) {
                 final_addr = (DE_to_EX.instr_data.address + CURRENT_STATE.REGS[DE_to_EX.instr_data.rn]);
            }
            if (DE_to_EX.dep == 2) {
                final_addr = DE_to_EX.instr_data.address + DE_to_EX.res;
            }
    		 EX_to_MEM.res = final_addr;
    	}
        if (DE_to_EX.decoded_instr.opcode == LSLI) {
            printf("LSLI cond \n");
            if ((DE_to_EX.dep == 0) || (DE_to_EX.dep == 1)) {
                printf("ex to mem res %li\n", EX_to_MEM.res);
                 EX_to_MEM.res = CURRENT_STATE.REGS[DE_to_EX.instr_data.rn] << (64 - DE_to_EX.instr_data.immediate);
            }
            if (DE_to_EX.dep == 2) {
                EX_to_MEM.res = DE_to_EX.res << (64 - DE_to_EX.instr_data.immediate);
                printf("ex to mem res %li\n", EX_to_MEM.res);
                printf("de to ex res %li\n", DE_to_EX.res);
            }
        }
        if (DE_to_EX.decoded_instr.opcode == LSRI) {
            if ((DE_to_EX.dep == 0) || (DE_to_EX.dep == 1)) {
                EX_to_MEM.res = CURRENT_STATE.REGS[DE_to_EX.instr_data.rn] >> DE_to_EX.instr_data.immediate;
            }
            if (DE_to_EX.dep == 2) {
                EX_to_MEM.res = DE_to_EX.res >> DE_to_EX.instr_data.immediate;
            }
        }
        if (DE_to_EX.decoded_instr.opcode == MOVZ) {
            EX_to_MEM.res = DE_to_EX.instr_data.immediate;
        }
        // could probably combine LDUR & STUR ?
        if ((DE_to_EX.decoded_instr.opcode == STUR64) ||
            (DE_to_EX.decoded_instr.opcode == STUR32) ||
            (DE_to_EX.decoded_instr.opcode == STURH)) {
                uint64_t final_addr;
                if ((DE_to_EX.dep == 0) || (DE_to_EX.dep == 1)) {
                     final_addr = (DE_to_EX.instr_data.address + CURRENT_STATE.REGS[DE_to_EX.instr_data.rn]);
                }
                if (DE_to_EX.dep == 2) {
                    final_addr = DE_to_EX.instr_data.address + DE_to_EX.res;
                }
    	     //printf("final address execute %lu\n", final_addr);
            	while((final_addr & 0x3) != 0) {
                    final_addr = final_addr + 1;
                }
    		//	printf("final address execute %lu\n", final_addr);
    			EX_to_MEM.res = final_addr;
    	}
        if (DE_to_EX.decoded_instr.opcode == STURB) {
            uint64_t final_addr;
            if ((DE_to_EX.dep == 0) || (DE_to_EX.dep == 1)) {
                 final_addr = (DE_to_EX.instr_data.address + CURRENT_STATE.REGS[DE_to_EX.instr_data.rn]);
            }
            if (DE_to_EX.dep == 2) {
                final_addr = DE_to_EX.instr_data.address + DE_to_EX.res;
            }
    	    EX_to_MEM.res = final_addr;
    	}
        if ((DE_to_EX.decoded_instr.opcode == SUB) ||
            (DE_to_EX.decoded_instr.opcode == SUBS)) {
                if(DE_to_EX.dep == 0) {
                    EX_to_MEM.res = CURRENT_STATE.REGS[DE_to_EX.instr_data.rn] - CURRENT_STATE.REGS[DE_to_EX.instr_data.rm];
                }
                if(DE_to_EX.dep == 1) {
                    EX_to_MEM.res = CURRENT_STATE.REGS[DE_to_EX.instr_data.rn] - DE_to_EX.res;
                }
                if(DE_to_EX.dep == 2) {
                    EX_to_MEM.res = DE_to_EX.res - CURRENT_STATE.REGS[DE_to_EX.instr_data.rm];
                }
        }
        if ((DE_to_EX.decoded_instr.opcode == SUBI) ||
            (DE_to_EX.decoded_instr.opcode == SUBSI)) {
                if ((DE_to_EX.dep == 0) || (DE_to_EX.dep == 1)) {
                    EX_to_MEM.res = CURRENT_STATE.REGS[DE_to_EX.instr_data.rn] - DE_to_EX.instr_data.immediate;
                }
                if (DE_to_EX.dep == 2) {
                    EX_to_MEM.res = DE_to_EX.res - DE_to_EX.instr_data.immediate;
                }
        }
        if (DE_to_EX.decoded_instr.opcode == MUL) {
            if(DE_to_EX.dep == 0) {
                EX_to_MEM.res = CURRENT_STATE.REGS[DE_to_EX.instr_data.rn] * CURRENT_STATE.REGS[DE_to_EX.instr_data.rm];
            }
            if(DE_to_EX.dep == 1) {
                EX_to_MEM.res = CURRENT_STATE.REGS[DE_to_EX.instr_data.rn] * DE_to_EX.res;
            }
            if(DE_to_EX.dep == 2) {
                EX_to_MEM.res = DE_to_EX.res * CURRENT_STATE.REGS[DE_to_EX.instr_data.rm];
            }
        }
        if (DE_to_EX.decoded_instr.opcode == HLT) {
    	    CURRENT_STATE.PC = hlt_pc;
    	}
    }
}

void pipe_stage_decode()
{
	printf("%s\n", "decode");
    DE_to_EX.pc = IF_to_DE.pc;
    unsigned long opcode;

	// 6 bit opcodes
    opcode = pc >> 26;
	uint64_t b_addr = pc & 0x3FFFFFF;
	bit_26 b_addr_26;
	b_addr_26.bits = b_addr * 4;

	// B
	if (opcode == 0x5) {
    	enum instr_type type =  BI;
   		enum op opcode = B;
		DE_to_EX.instr_data.address = b_addr_26.bits;
		DE_to_EX.decoded_instr.opcode = opcode;
		DE_to_EX.decoded_instr.type = type;
		DE_to_EX.decoded_instr.data = DE_to_EX.instr_data;
	}

	// 8 bit opcodes
	opcode = pc >> 24;
	int64_t cb_address = (int64_t) pc;
	cb_address = (cb_address & 0xFFFFE0) >> 5;
	bit_19 cb_address_bits;
	cb_address_bits.bits = cb_address * 4;
	uint64_t cb_rt = pc & 0x1F;

  	// CBNZ
	if (opcode == 0xB5) {
		enum instr_type type =  CB;
		enum op opcode = CBNZ;
		DE_to_EX.instr_data.address = cb_address_bits.bits;
		DE_to_EX.instr_data.rt = cb_rt;
		DE_to_EX.decoded_instr.opcode = opcode;
		DE_to_EX.decoded_instr.type = type;
		DE_to_EX.decoded_instr.data = DE_to_EX.instr_data;
	}
  	// CBZ
	if (opcode == 0xB4) {
		enum instr_type type =  CB;
		enum op opcode = CBZ;
		DE_to_EX.instr_data.address = cb_address_bits.bits;
		DE_to_EX.instr_data.rt = cb_rt;
		DE_to_EX.decoded_instr.opcode = opcode;
		DE_to_EX.decoded_instr.type = type;
		DE_to_EX.decoded_instr.data = DE_to_EX.instr_data;
	}
	// B.cond
	if (opcode == 0x54) {
		unsigned long condition_code = cb_rt;
		// BEQ
		if (condition_code == 0x0) {
			enum instr_type type =  CB;
			enum op opcode = BEQ;
            DE_to_EX.instr_data.address = cb_address_bits.bits;
    		DE_to_EX.instr_data.rt = cb_rt;
    		DE_to_EX.decoded_instr.opcode = opcode;
    		DE_to_EX.decoded_instr.type = type;
    		DE_to_EX.decoded_instr.data = DE_to_EX.instr_data;
		}
		// BNE
		if (condition_code == 0x1) {
			enum instr_type type =  CB;
			enum op opcode = BNE;
            DE_to_EX.instr_data.address = cb_address_bits.bits;
    		DE_to_EX.instr_data.rt = cb_rt;
    		DE_to_EX.decoded_instr.opcode = opcode;
    		DE_to_EX.decoded_instr.type = type;
    		DE_to_EX.decoded_instr.data = DE_to_EX.instr_data;
		}
		// BGT
		if (condition_code == 0xC) {
			enum instr_type type =  CB;
			enum op opcode = BGT;
            DE_to_EX.instr_data.address = cb_address_bits.bits;
    		DE_to_EX.instr_data.rt = cb_rt;
    		DE_to_EX.decoded_instr.opcode = opcode;
    		DE_to_EX.decoded_instr.type = type;
    		DE_to_EX.decoded_instr.data = DE_to_EX.instr_data;
		}
		// BLT
		if (condition_code == 0xB) {
			enum instr_type type =  CB;
			enum op opcode = BLT;
            DE_to_EX.instr_data.address = cb_address_bits.bits;
    		DE_to_EX.instr_data.rt = cb_rt;
    		DE_to_EX.decoded_instr.opcode = opcode;
    		DE_to_EX.decoded_instr.type = type;
    		DE_to_EX.decoded_instr.data = DE_to_EX.instr_data;
		}
		// BGE
		if (condition_code == 0xA) {
			enum instr_type type =  CB;
			enum op opcode = BGE;
            DE_to_EX.instr_data.address = cb_address_bits.bits;
    		DE_to_EX.instr_data.rt = cb_rt;
    		DE_to_EX.decoded_instr.opcode = opcode;
    		DE_to_EX.decoded_instr.type = type;
    		DE_to_EX.decoded_instr.data = DE_to_EX.instr_data;
		}
		// BLE
		if (condition_code == 0xD) {
			enum instr_type type =  CB;
			enum op opcode = BLE;
            DE_to_EX.instr_data.address = cb_address_bits.bits;
    		DE_to_EX.instr_data.rt = cb_rt;
    		DE_to_EX.decoded_instr.opcode = opcode;
    		DE_to_EX.decoded_instr.type = type;
    		DE_to_EX.decoded_instr.data = DE_to_EX.instr_data;
		}
	}
	// 9 bit opcodes
	opcode = pc >> 23;
	uint64_t iw_immediate = ((pc >> 5) & 0xFFFF);
	uint64_t iw_op2 = (pc >> 21) & 0x3;
	uint64_t iw_rd = pc & 0x1F;

	// MOVZ
	if (opcode == 0x1A5) {
		enum instr_type type =  IW;
		enum op opcode = MOVZ;
		DE_to_EX.instr_data.immediate = iw_immediate;
		DE_to_EX.instr_data.op2 = iw_op2;
		DE_to_EX.instr_data.rd = iw_rd;
		DE_to_EX.decoded_instr.opcode = opcode;
		DE_to_EX.decoded_instr.type = type;
		DE_to_EX.decoded_instr.data = DE_to_EX.instr_data;
	}

	// 10 bit opcodes
	opcode = pc >> 22;
	uint64_t i_immediate = (pc >> 10) & 0xFFF;
	uint64_t i_rn = (pc >> 5) & 0x1F;
	uint64_t i_rd = pc & 0x1F;

	// ADDI
	if (opcode == 0x244) {
		enum instr_type type =  I;
		enum op opcode = ADDI;
		DE_to_EX.instr_data.immediate = i_immediate;
		DE_to_EX.instr_data.rn = i_rn;
		DE_to_EX.instr_data.rd = i_rd;
		DE_to_EX.decoded_instr.opcode = opcode;
		DE_to_EX.decoded_instr.type = type;
		DE_to_EX.decoded_instr.data = DE_to_EX.instr_data;
	}
	// ADDIS
	if (opcode == 0x2C4) {
		enum instr_type type =  I;
		enum op opcode = ADDSI;
        DE_to_EX.instr_data.immediate = i_immediate;
		DE_to_EX.instr_data.rn = i_rn;
		DE_to_EX.instr_data.rd = i_rd;
		DE_to_EX.decoded_instr.opcode = opcode;
		DE_to_EX.decoded_instr.type = type;
		DE_to_EX.decoded_instr.data = DE_to_EX.instr_data;
	}
	// SUBI
	if (opcode == 0x344) {
		enum instr_type type =  I;
		enum op opcode = SUBI;
        DE_to_EX.instr_data.immediate = i_immediate;
        DE_to_EX.instr_data.rn = i_rn;
        DE_to_EX.instr_data.rd = i_rd;
        DE_to_EX.decoded_instr.opcode = opcode;
        DE_to_EX.decoded_instr.type = type;
        DE_to_EX.decoded_instr.data = DE_to_EX.instr_data;
	}
	// SUBSI / CMPI
	if (opcode == 0x3C4) {
		enum instr_type type =  I;
		enum op opcode = SUBSI;
        DE_to_EX.instr_data.immediate = i_immediate;
        DE_to_EX.instr_data.rn = i_rn;
        DE_to_EX.instr_data.rd = i_rd;
        DE_to_EX.decoded_instr.opcode = opcode;
        DE_to_EX.decoded_instr.type = type;
        DE_to_EX.decoded_instr.data = DE_to_EX.instr_data;
	}
	if (opcode == 0x34D) {
		// LSRI
		if ((i_immediate & 0x3F) == 0x3F) {
			enum instr_type type =  I;
			enum op opcode = LSRI;
			DE_to_EX.instr_data.immediate = (i_immediate >> 6) & 0x3F;
            DE_to_EX.instr_data.rn = i_rn;
            DE_to_EX.instr_data.rd = i_rd;
            DE_to_EX.decoded_instr.opcode = opcode;
            DE_to_EX.decoded_instr.type = type;
            DE_to_EX.decoded_instr.data = DE_to_EX.instr_data;
		}
		// LSLI
		else {
			enum instr_type type =  I;
			enum op opcode = LSLI;
            DE_to_EX.instr_data.immediate = (i_immediate >> 6) & 0x3F;
            DE_to_EX.instr_data.rn = i_rn;
            DE_to_EX.instr_data.rd = i_rd;
            DE_to_EX.decoded_instr.opcode = opcode;
            DE_to_EX.decoded_instr.type = type;
            DE_to_EX.decoded_instr.data = DE_to_EX.instr_data;
		}
	}

	// 11 bit opcodes
	opcode = pc >> 21;
	uint64_t r_rm = (pc >> 16) & 0x1F;
	uint64_t r_shamt = (pc >> 10) & 0x3F;
	uint64_t r_rn = (pc >> 5) & 0x1F;
	uint64_t r_rd = pc & 0x1F;
	uint64_t d_addoffset = (pc >> 12) & 0x1FF;
	uint64_t d_op2 = (pc >> 10) & 0x3;
	uint64_t d_rn = (pc >> 5) & 0x1F;
	uint64_t d_rt = pc & 0x1F;

	// ADD
	if (opcode == 0x458) {
		enum instr_type type =  R;
    	enum op opcode = ADD;
        DE_to_EX.decoded_instr.opcode = opcode;
        DE_to_EX.decoded_instr.type = type;
		DE_to_EX.instr_data.rm = r_rm;
		DE_to_EX.instr_data.shamt = r_shamt;
		DE_to_EX.instr_data.rn = r_rn;
		DE_to_EX.instr_data.rd = r_rd;
		DE_to_EX.decoded_instr.data = DE_to_EX.instr_data;
	}
	// ADDS
	if (opcode == 0x558) {
		enum instr_type type =  R;
    	enum op opcode = ADDS;
        DE_to_EX.decoded_instr.opcode = opcode;
        DE_to_EX.decoded_instr.type = type;
		DE_to_EX.instr_data.rm = r_rm;
		DE_to_EX.instr_data.shamt = r_shamt;
		DE_to_EX.instr_data.rn = r_rn;
		DE_to_EX.instr_data.rd = r_rd;
		DE_to_EX.decoded_instr.data = DE_to_EX.instr_data;
	}
	// AND
	if (opcode == 0x450) {
		enum instr_type type =  R;
    	enum op opcode = AND;
        DE_to_EX.decoded_instr.opcode = opcode;
        DE_to_EX.decoded_instr.type = type;
		DE_to_EX.instr_data.rm = r_rm;
		DE_to_EX.instr_data.shamt = r_shamt;
		DE_to_EX.instr_data.rn = r_rn;
		DE_to_EX.instr_data.rd = r_rd;
		DE_to_EX.decoded_instr.data = DE_to_EX.instr_data;
	}
	// ANDS
	if (opcode == 0x750) {
		enum instr_type type =  R;
    	enum op opcode = ANDS;
        DE_to_EX.decoded_instr.opcode = opcode;
        DE_to_EX.decoded_instr.type = type;
		DE_to_EX.instr_data.rm = r_rm;
		DE_to_EX.instr_data.shamt = r_shamt;
		DE_to_EX.instr_data.rn = r_rn;
		DE_to_EX.instr_data.rd = r_rd;
		DE_to_EX.decoded_instr.data = DE_to_EX.instr_data;
	}
	// EOR
	if (opcode == 0x650) {
		enum instr_type type =  R;
    	enum op opcode = EOR;
        DE_to_EX.decoded_instr.opcode = opcode;
        DE_to_EX.decoded_instr.type = type;
		DE_to_EX.instr_data.rm = r_rm;
		DE_to_EX.instr_data.shamt = r_shamt;
		DE_to_EX.instr_data.rn = r_rn;
		DE_to_EX.instr_data.rd = r_rd;
		DE_to_EX.decoded_instr.data = DE_to_EX.instr_data;
	}
	// ORR
	if (opcode == 0x550) {
		enum instr_type type =  R;
	    	enum op opcode = ORR;
            DE_to_EX.decoded_instr.opcode = opcode;
            DE_to_EX.decoded_instr.type = type;
    		DE_to_EX.instr_data.rm = r_rm;
    		DE_to_EX.instr_data.shamt = r_shamt;
    		DE_to_EX.instr_data.rn = r_rn;
    		DE_to_EX.instr_data.rd = r_rd;
    		DE_to_EX.decoded_instr.data = DE_to_EX.instr_data;
	}
	// LDUR64
	if (opcode == 0x7C2) {
		enum instr_type type =  D;
		enum op opcode = LDUR64;
		DE_to_EX.instr_data.address = d_addoffset;
		DE_to_EX.instr_data.op2 = d_op2;
		DE_to_EX.instr_data.rn = d_rn;
		DE_to_EX.instr_data.rt = d_rt;
		DE_to_EX.decoded_instr.opcode = opcode;
		DE_to_EX.decoded_instr.type = type;
		DE_to_EX.decoded_instr.data = DE_to_EX.instr_data;
	}
	// LDUR32
	if (opcode == 0x5C2) {
		enum instr_type type =  D;
		enum op opcode = LDUR32;
        DE_to_EX.instr_data.address = d_addoffset;
		DE_to_EX.instr_data.op2 = d_op2;
		DE_to_EX.instr_data.rn = d_rn;
		DE_to_EX.instr_data.rt = d_rt;
		DE_to_EX.decoded_instr.opcode = opcode;
		DE_to_EX.decoded_instr.type = type;
		DE_to_EX.decoded_instr.data = DE_to_EX.instr_data;
    }
	// LDURB
	if (opcode == 0x1C2) {
		enum instr_type type =  D;
		enum op opcode = LDURB;
        DE_to_EX.instr_data.address = d_addoffset;
		DE_to_EX.instr_data.op2 = d_op2;
		DE_to_EX.instr_data.rn = d_rn;
		DE_to_EX.instr_data.rt = d_rt;
		DE_to_EX.decoded_instr.opcode = opcode;
		DE_to_EX.decoded_instr.type = type;
		DE_to_EX.decoded_instr.data = DE_to_EX.instr_data;
	}
	// LDURH
	if (opcode == 0x3C2) {
		enum instr_type type =  D;
		enum op opcode = LDURH;
        DE_to_EX.instr_data.address = d_addoffset;
		DE_to_EX.instr_data.op2 = d_op2;
		DE_to_EX.instr_data.rn = d_rn;
		DE_to_EX.instr_data.rt = d_rt;
		DE_to_EX.decoded_instr.opcode = opcode;
		DE_to_EX.decoded_instr.type = type;
		DE_to_EX.decoded_instr.data = DE_to_EX.instr_data;
	}
	// STUR64
	if (opcode == 0x7C0) {
		enum instr_type type =  D;
		enum op opcode = STUR64;
        DE_to_EX.instr_data.address = d_addoffset;
		DE_to_EX.instr_data.op2 = d_op2;
		DE_to_EX.instr_data.rn = d_rn;
		DE_to_EX.instr_data.rt = d_rt;
		DE_to_EX.decoded_instr.opcode = opcode;
		DE_to_EX.decoded_instr.type = type;
		DE_to_EX.decoded_instr.data = DE_to_EX.instr_data;
	}
	// STUR32
	 if (opcode == 0x5C0) {
		enum instr_type type =  D;
 		enum op opcode = STUR32;
        DE_to_EX.instr_data.address = d_addoffset;
		DE_to_EX.instr_data.op2 = d_op2;
		DE_to_EX.instr_data.rn = d_rn;
		DE_to_EX.instr_data.rt = d_rt;
		DE_to_EX.decoded_instr.opcode = opcode;
		DE_to_EX.decoded_instr.type = type;
		DE_to_EX.decoded_instr.data = DE_to_EX.instr_data;
    }
	// STURB
	if (opcode == 0x1C0) {
		enum instr_type type =  D;
		enum op opcode = STURB;
        DE_to_EX.instr_data.address = d_addoffset;
		DE_to_EX.instr_data.op2 = d_op2;
		DE_to_EX.instr_data.rn = d_rn;
		DE_to_EX.instr_data.rt = d_rt;
		DE_to_EX.decoded_instr.opcode = opcode;
		DE_to_EX.decoded_instr.type = type;
		DE_to_EX.decoded_instr.data = DE_to_EX.instr_data;
	}
	// STURH
	if (opcode == 0x3C0) {
		enum instr_type type =  D;
		enum op opcode = STURH;
        DE_to_EX.instr_data.address = d_addoffset;
		DE_to_EX.instr_data.op2 = d_op2;
		DE_to_EX.instr_data.rn = d_rn;
		DE_to_EX.instr_data.rt = d_rt;
		DE_to_EX.decoded_instr.opcode = opcode;
		DE_to_EX.decoded_instr.type = type;
		DE_to_EX.decoded_instr.data = DE_to_EX.instr_data;
	}
	// SUB
	if (opcode == 0x658) {
		enum instr_type type =  R;
    	enum op opcode = SUB;
        DE_to_EX.decoded_instr.opcode = opcode;
        DE_to_EX.decoded_instr.type = type;
		DE_to_EX.instr_data.rm = r_rm;
		DE_to_EX.instr_data.shamt = r_shamt;
		DE_to_EX.instr_data.rn = r_rn;
		DE_to_EX.instr_data.rd = r_rd;
		DE_to_EX.decoded_instr.data = DE_to_EX.instr_data;
	}
	// SUBS
	if (opcode == 0x758) {
		enum instr_type type =  R;
    	enum op opcode = SUBS;
        DE_to_EX.decoded_instr.opcode = opcode;
        DE_to_EX.decoded_instr.type = type;
		DE_to_EX.instr_data.rm = r_rm;
		DE_to_EX.instr_data.shamt = r_shamt;
		DE_to_EX.instr_data.rn = r_rn;
		DE_to_EX.instr_data.rd = r_rd;
		DE_to_EX.decoded_instr.data = DE_to_EX.instr_data;
	}
	// MUL
	if (opcode == 0x4D8) {
		enum instr_type type =  R;
    	enum op opcode = MUL;
        DE_to_EX.decoded_instr.opcode = opcode;
        DE_to_EX.decoded_instr.type = type;
		DE_to_EX.instr_data.rm = r_rm;
		DE_to_EX.instr_data.shamt = r_shamt;
		DE_to_EX.instr_data.rn = r_rn;
		DE_to_EX.instr_data.rd = r_rd;
		DE_to_EX.decoded_instr.data = DE_to_EX.instr_data;
	}
	// HLT
	if (opcode == 0x6A2) {
		enum instr_type type =  IW;
		enum op opcode = HLT;
        	DE_to_EX.decoded_instr.opcode = opcode;
        	DE_to_EX.decoded_instr.type = type;
		DE_to_EX.pc = IF_to_DE.pc + 8;
		uint64_t immediate = ((pc >> 5) & 0xFFFF);
		DE_to_EX.instr_data.immediate = immediate;
		DE_to_EX.decoded_instr.data = DE_to_EX.instr_data;
		hlt_pc = CURRENT_STATE.PC;
		CURRENT_STATE.PC = hlt_pc;
	}
	// BR
	if (opcode == 0x6B0) {
		enum instr_type type =  R;
	    enum op opcode = BR;
        DE_to_EX.decoded_instr.opcode = opcode;
        DE_to_EX.decoded_instr.type = type;
		DE_to_EX.instr_data.rm = r_rm;
		DE_to_EX.instr_data.shamt = r_shamt;
		DE_to_EX.instr_data.rn = r_rn;
		DE_to_EX.instr_data.rd = r_rd;
		DE_to_EX.decoded_instr.data = DE_to_EX.instr_data;
	}
	//printf("%u\n", DE_to_EX.decoded_instr.opcode);
	printf("decode de to ex rn  %lu\n", DE_to_EX.instr_data.rn);
}

void pipe_stage_fetch()
{
	printf("%s\n", "fetch");
    pc = mem_read_32(CURRENT_STATE.PC);
    IF_to_DE.pc = pc;
}
