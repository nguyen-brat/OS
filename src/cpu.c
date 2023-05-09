
#include "cpu.h"
#include "mem.h"
#include "mm.h"
#include "flag.h"
#include <stdio.h>

int calc(struct pcb_t * proc) {
	return ((unsigned long)proc & 0UL);
}

int alloc(struct pcb_t * proc, uint32_t size, uint32_t reg_index) {
	addr_t addr = alloc_mem(size, proc);
	if (addr == 0) {
		return 1;
	}else{
		proc->regs[reg_index] = addr;
		return 0;
	}
}

int free_data(struct pcb_t * proc, uint32_t reg_index) {
	return free_mem(proc->regs[reg_index], proc);
}

int read(
		struct pcb_t * proc, // Process executing the instruction
		uint32_t source, // Index of source register
		uint32_t offset, // Source address = [source] + [offset]
		uint32_t destination) { // Index of destination register
	
	BYTE data;
	if (read_mem(proc->regs[source] + offset, proc,	&data)) {
		proc->regs[destination] = data;
		return 0;		
	}else{
		return 1;
	}
}

int write(
		struct pcb_t * proc, // Process executing the instruction
		BYTE data, // Data to be wrttien into memory
		uint32_t destination, // Index of destination register
		uint32_t offset) { 	// Destination address =
					// [destination] + [offset]
	return write_mem(proc->regs[destination] + offset, proc, data);
} 

int run(struct pcb_t * proc) {
	/* Check if Program Counter point to the proper instruction */
	if (FLAG) printf("Flag 150\n");
	if (proc == NULL) printf("proc is empty stupid!\n");
	else {
		printf("It not NULL you are more fucking stupid\n");
		if (proc->code == NULL) printf("proc code is NULL\n");
		else printf("Proc code is not null\n");
		printf("Value of programme counter is: %u\n", proc->pc);
		printf("The size is %u\n", proc->code->size);
	}
	if (proc->pc >= proc->code->size) {
		if (FLAG) printf("Special\n");
		return 1;
	}
	if (FLAG) printf("Flag 149\n");
	struct inst_t ins = proc->code->text[proc->pc];
	if (FLAG) printf("Flag 148\n");
	proc->pc++;
	int stat = 1;
	switch (ins.opcode) {
	case CALC:
		if (FLAG) printf("Flag 147\n");
		stat = calc(proc);
		if (FLAG) printf("Flag 146\n");
		break;
	case ALLOC:
#ifdef MM_PAGING
		if (FLAG) printf("Flag 145\n");
		stat = pgalloc(proc, ins.arg_0, ins.arg_1);
		if (FLAG) printf("Flag 144\n");

#else
		stat = alloc(proc, ins.arg_0, ins.arg_1);
#endif
		break;
	case FREE:
#ifdef MM_PAGING
		if (FLAG) printf("Flag 143\n");
		stat = pgfree_data(proc, ins.arg_0);
		if (FLAG) printf("Flag 142\n");
#else
		stat = free_data(proc, ins.arg_0);
#endif
		break;
	case READ:
#ifdef MM_PAGING
		if (FLAG) printf("Flag 141\n");
		stat = pgread(proc, ins.arg_0, ins.arg_1, ins.arg_2);
		if (FLAG) printf("Flag 140\n");
#else
		stat = read(proc, ins.arg_0, ins.arg_1, ins.arg_2);
#endif
		break;
	case WRITE:
#ifdef MM_PAGING
		if (FLAG) printf("Flag 139\n");
		stat = pgwrite(proc, ins.arg_0, ins.arg_1, ins.arg_2);
		if (FLAG) printf("Flag 138\n");
#else
		stat = write(proc, ins.arg_0, ins.arg_1, ins.arg_2);
#endif
		break;
	default:
		stat = 1;
	}
	return stat;

}


