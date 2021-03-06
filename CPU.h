
#ifndef TRACE_ITEM_H
#define TRACE_ITEM_H

enum opcode {
	ti_NOP = 0,
	ti_RTYPE,
	ti_ITYPE,
	ti_LOAD,
	ti_STORE,
	ti_BRANCH,
	ti_JTYPE,
	ti_SPECIAL,
	ti_JRTYPE,
	ti_FLUSHED	/*added to 'flush' instruction in case of taken branch*/
};

struct instruction {
	unsigned char type;			// see above
	unsigned char sReg_a;		// 1st operand
	unsigned char sReg_b;		// 2nd operand
	unsigned char dReg;			// dest. operand
	unsigned int PC;			// program counter
	unsigned int Addr;			// mem. address
};

//entries in branch prediction hash table
struct branch_prediction {
	bool prediction;//true if predicted to take branchPrediction
	int PC;//PC of instruction
	int target;//location jumped or branched to
};

#endif

#define TRACE_BUFSIZE 1024*1024

static FILE *trace_fd;
static int trace_buf_ptr;
static int trace_buf_end;
static struct instruction *trace_buf;
static FILE *out_fd;

int is_big_endian(void)
{
	union {
		uint32_t i;
		char c[4];
	} bint = { 0x01020304 };

	return bint.c[0] == 1;
}

uint32_t my_ntohl(uint32_t x)
{
	u_char *s = (u_char *)&x;
	return (uint32_t)(s[3] << 24 | s[2] << 16 | s[1] << 8 | s[0]);
}

void trace_init()
{
	trace_buf = malloc(sizeof(struct instruction) * TRACE_BUFSIZE);

	if (!trace_buf) {
		fprintf(stdout, "** trace_buf not allocated\n");
		exit(-1);
	}

	trace_buf_ptr = 0;
	trace_buf_end = 0;
}

void trace_uninit()
{
	free(trace_buf);
	fclose(trace_fd);
}

int trace_get_item(struct instruction **item)
{
	int n_items;

	if (trace_buf_ptr == trace_buf_end) {	/* if no more unprocessed items in the trace buffer, get new data  */
		n_items = fread(trace_buf, sizeof(struct instruction), TRACE_BUFSIZE, trace_fd);
		if (!n_items) return 0;				/* if no more items in the file, we are done */

		trace_buf_ptr = 0;
		trace_buf_end = n_items;			/* n_items were read and placed in trace buffer */
	}

	*item = &trace_buf[trace_buf_ptr];	/* read a new trace item for processing */
	trace_buf_ptr++;

	if (is_big_endian()) {
		(*item)->PC = my_ntohl((*item)->PC);
		(*item)->Addr = my_ntohl((*item)->Addr);
	}

	return 1;
}

int write_trace(struct instruction item, char *fname)
{
	out_fd = fopen(fname, "a");
	int n_items;
	if (is_big_endian()) {
		(&item)->PC = my_ntohl((&item)->PC);
		(&item)->Addr = my_ntohl((&item)->Addr);
	}

	n_items = fwrite(&item, sizeof(struct instruction), 1, out_fd);
	fclose(out_fd);
	if (!n_items) return 0;				/* if no more items in the file, we are done */

		
	return 1;
}

void trace(int trace_view_on, int cycle_number, struct instruction MEM_WB, int pipeline_num) {
	
	if (trace_view_on && cycle_number>=pipeline_num) {/* print the instruction exiting the pipeline if trace_view_on=1 */
      switch(MEM_WB.type) {
        case ti_NOP:
          printf("[cycle %d] NOP:\n",cycle_number) ;
          break;
		case ti_FLUSHED:
		  printf("[cycle %d] FLUSHED:\n",cycle_number) ;
		  break;
        case ti_RTYPE: /* registers are translated for printing by subtracting offset  */
          printf("[cycle %d] RTYPE:",cycle_number) ;
		  printf(" (PC: %d)(sReg_a: %d)(sReg_b: %d)(dReg: %d) \n", MEM_WB.PC, MEM_WB.sReg_a, MEM_WB.sReg_b, MEM_WB.dReg);
          break;
        case ti_ITYPE:
          printf("[cycle %d] ITYPE:",cycle_number) ;
		  printf(" (PC: %d)(sReg_a: %d)(dReg: %d)(addr: %d)\n", MEM_WB.PC, MEM_WB.sReg_a, MEM_WB.dReg, MEM_WB.Addr);
          break;
        case ti_LOAD:
          printf("[cycle %d] LOAD:",cycle_number) ;      
		  printf(" (PC: %d)(sReg_a: %d)(dReg: %d)(addr: %d)\n", MEM_WB.PC, MEM_WB.sReg_a, MEM_WB.dReg, MEM_WB.Addr);
          break;
        case ti_STORE:
          printf("[cycle %d] STORE:",cycle_number) ;      
		  printf(" (PC: %d)(sReg_a: %d)(sReg_b: %d)(addr: %d)\n", MEM_WB.PC, MEM_WB.sReg_a, MEM_WB.sReg_b, MEM_WB.Addr);
          break;
        case ti_BRANCH:
          printf("[cycle %d] BRANCH:",cycle_number) ;
		  printf(" (PC: %d)(sReg_a: %d)(sReg_b: %d)(addr: %d)\n", MEM_WB.PC, MEM_WB.sReg_a, MEM_WB.sReg_b, MEM_WB.Addr);
          break;
        case ti_JTYPE:
          printf("[cycle %d] JTYPE:",cycle_number) ;
		  printf(" (PC: %d)(addr: %d)\n", MEM_WB.PC, MEM_WB.Addr);
          break;
        case ti_SPECIAL:
          printf("[cycle %d] SPECIAL:\n",cycle_number) ;      	
          break;
        case ti_JRTYPE:
          printf("[cycle %d] JRTYPE:",cycle_number) ;
		  printf(" (PC: %d) (sReg_a: %d)(addr: %d)\n", MEM_WB.PC, MEM_WB.dReg, MEM_WB.Addr);
          break;
      }
    }
	
}

