/**************************************************************/
/* CS/COE 1541				 			
   compile with gcc -o pipeline eight_stage.c			
   and execute using							
   ./pipeline  /afs/cs.pitt.edu/courses/1541/short_traces/sample.tr	0  
***************************************************************/

#include <stdio.h>
#include<stdlib.h>
#include <stdbool.h>
#include <inttypes.h>
#include <arpa/inet.h>
#include "CPU.h" 

void print_MEM2_WB(struct instruction MEM2_WB, int cycle_number);

int main(int argc, char **argv)
{
  struct instruction *tr_entry;
  struct instruction PCregister, IF1_IF2, IF2_ID, ID_EX1, EX1_EX2, EX2_MEM1, MEM1_MEM2, MEM2_WB; 
  size_t size;
  char *trace_file_name;
  int trace_view_on = 0;
  int prediction_method = 0;
  struct branch_prediction prediction_table[64];
  int flush_counter = 7; //8 stage pipeline, so we have to execute 7 instructions once trace is done
  
  unsigned int cycle_number = 0;

  if (argc == 1) {
    fprintf(stdout, "\nUSAGE: tv <trace_file> <switch - any character>\n");
    fprintf(stdout, "\n(switch) to turn on or off individual item view.\n\n");
    exit(0);
  }
    
  trace_file_name = argv[1];
  if (argc == 3) trace_view_on = atoi(argv[2]) ;
  
  if (argc == 4) { 
	prediction_method = atoi(argv[2]);
	trace_view_on = atoi(argv[3]);
  }
  fprintf(stdout, "\n ** opening file %s\n", trace_file_name);

  trace_fd = fopen(trace_file_name, "rb");

  if (!trace_fd) {
    fprintf(stdout, "\ntrace file %s not opened.\n\n", trace_file_name);
    exit(0);
  }

  trace_init();

  while(1) {
    size = trace_get_item(&tr_entry); /* put the instruction into a buffer */
   
    if (!size && flush_counter==0) {       /* no more instructions (instructions) to simulate */
      printf("+ Simulation terminates at cycle : %u\n", cycle_number);
      break;
    }
    else{              /* move the pipeline forward */
      cycle_number++;

      /* move instructions one stage ahead */
      MEM2_WB = MEM1_MEM2;
      MEM1_MEM2 = EX2_MEM1;
      EX2_MEM1 = EX1_EX2;
      EX1_EX2 = ID_EX1;
      ID_EX1 = IF2_ID;
      IF2_ID = IF1_IF2;
      IF1_IF2 = PCregister;
	  
	  /*STRUCTURAL HAZARDS
		if the instruction at WB is trying to
		write into the register file while the instruction at ID is trying to read from the register file, priority is given
		to the instruction at WB. The instructions at IF1, IF2 and ID are stalled for one cycle while the instruction
		at WB is using the register file.*/
		
		if (MEM2_WB.type == ti_RTYPE	//if instruction at WB will write to register file
			|| MEM2_WB.type == ti_ITYPE
			|| MEM2_WB.type == ti_LOAD) {
		
			
			if (IF2_ID.type != ti_SPECIAL	//if instruction at ID will read from register file
				&& IF2_ID.type != ti_JTYPE
				&& IF2_ID.type != ti_NOP
				&& IF2_ID.type != ti_FLUSHED) {
				
				//print WB
				trace(trace_view_on, cycle_number, MEM2_WB, 8);
				
				//stall IF1, IF2, and ID, creates bubble at EX
				//printf("structural hazard encountered and handled\n");
				
				MEM2_WB = MEM1_MEM2;
				MEM1_MEM2 = EX2_MEM1;
				EX2_MEM1 = EX1_EX2;
				EX1_EX2 = ID_EX1;
				ID_EX1.type = ti_NOP;
				cycle_number++;
			}
		}
      
      /*DATA HAZARD
        1. EX1_EX2 write into register, and ID_EX1 reads from that register. insert no-op into EX1_EX2.
        2. EX2_MEM1 or MEM1_MEM2 is LOAD into register, and ID_EX1 reads from that register. insert no-op into EX1_EX2.
      */
      
      //1st hazard
      if ((EX1_EX2.dReg == ID_EX1.sReg_a) || (EX1_EX2.dReg == ID_EX1.sReg_b)) {
        //print out wb
        trace(trace_view_on, cycle_number, MEM2_WB, 8);
        //move up and change ex1ex2 to no op
        MEM2_WB = MEM1_MEM2;
        MEM1_MEM2 = EX2_MEM1;
        EX2_MEM1 = EX1_EX2;
        EX1_EX2.type = ti_NOP;
        cycle_number++;
      }
      
      //second hazard
      if (EX2_MEM1.type = ti_LOAD) {
        if ((EX2_MEM1.dReg == ID_EX1.sReg_a) || (EX2_MEM1.dReg == ID_EX1.sReg_b)) {
          trace(trace_view_on, cycle_number, MEM2_WB, 8);
          //move up and change ex1ex2 to no op
          MEM2_WB = MEM1_MEM2;
          MEM1_MEM2 = EX2_MEM1;
          EX2_MEM1 = EX1_EX2;
          EX1_EX2.type = ti_NOP;
          cycle_number++;
        }
      }
      
      if (MEM1_MEM2.type = ti_LOAD) {
        if ((MEM1_MEM2.dReg == ID_EX1.sReg_a) || (MEM1_MEM2.dReg == ID_EX1.sReg_b)) {
          trace(trace_view_on, cycle_number, MEM2_WB, 8);
          //move up and change ex1ex2 to no op
          MEM2_WB = MEM1_MEM2;
          MEM1_MEM2 = EX2_MEM1;
          EX2_MEM1 = EX1_EX2;
          EX1_EX2.type = ti_NOP;
          cycle_number++;
        }
      }
	  
	  /*assuming branch condition resolved in ID stage
	  /*if instruction is branch*/
	  if (ID_EX1.type == ti_BRANCH) {
		/*if the target address field is equal to PC of next instruction*/
		
		if(prediction_method == 1) {//apply branch prediction
			int index = (ID_EX1.PC & 0x3F) >> 4;//indexing with bits 9-4 for prediction_table
			struct branch_prediction curr = prediction_table[index];//indexing with bits 9-4
			if (ID_EX1.Addr == IF2_ID.PC && (curr.PC != ID_EX1.PC || (curr.PC == ID_EX1.PC && curr.prediction == false))) {//false prediction
				
				//update branch prediction table
				struct branch_prediction b;
				b.prediction = true;
				b.PC = ID_EX1.PC;
				b.target = ID_EX1.Addr;
				prediction_table[index] = b;
				
				trace(trace_view_on, cycle_number, MEM2_WB, 8);
				
				/*flush incorrect instruction in IF2_ID and advance others
			
				Because of the way this program works - it doesn't actually take
				an incorrect instruction since it's just tracing a real execution.
				This being said: instead of 'flushing an incorrect instruction', I'm
				simulating it by advancing the branch and all instructions before it,
				then inserting a flushed instruction into the ID_EX1 buffer (where the
				branch was previously) */
				
				MEM2_WB = MEM1_MEM2;
				MEM1_MEM2 = EX2_MEM1;
				EX2_MEM1 = EX1_EX2;
				EX1_EX2 = ID_EX1;
				ID_EX1.type = ti_FLUSHED;
				cycle_number++;
				
				trace(trace_view_on, cycle_number, MEM2_WB, 8);
				
				MEM2_WB = MEM1_MEM2;
				MEM1_MEM2 = EX2_MEM1;
				EX2_MEM1 = EX1_EX2;
				EX1_EX2 = ID_EX1;
				ID_EX1 = IF2_ID;
				IF2_ID.type = ti_FLUSHED;
				cycle_number++;
			} else if(ID_EX1.Addr != IF2_ID.PC && curr.PC != ID_EX1.PC) {//no prediction, but correct path
			
			//update branch prediction table
				struct branch_prediction b;
				b.prediction = false;
				b.PC = ID_EX1.PC;
				b.target = ID_EX1.PC + 4;
				prediction_table[index] = b;
			} else if(ID_EX1.Addr != IF2_ID.PC && (curr.PC == ID_EX1.PC && curr.prediction == true)){//false prediction
				//update branch prediction table
				struct branch_prediction b;
				b.prediction = false;
				b.PC = ID_EX1.PC;
				b.target = ID_EX1.PC + 4;
				prediction_table[index] = b;
				
				trace(trace_view_on, cycle_number, MEM2_WB, 8);
				
				/*flush incorrect instruction in IF2_ID and advance others
			
				Because of the way this program works - it doesn't actually take
				an incorrect instruction since it's just tracing a real execution.
				This being said: instead of 'flushing an incorrect instruction', I'm
				simulating it by advancing the branch and all instructions before it,
				then inserting a flushed instruction into the ID_EX1 buffer (where the
				branch was previously) */
				MEM2_WB = MEM1_MEM2;
				MEM1_MEM2 = EX2_MEM1;
				EX2_MEM1 = EX1_EX2;
				EX1_EX2 = ID_EX1;
				ID_EX1.type = ti_FLUSHED;
				cycle_number++;
				
				trace(trace_view_on, cycle_number, MEM2_WB, 8);
				
				MEM2_WB = MEM1_MEM2;
				MEM1_MEM2 = EX2_MEM1;
				EX2_MEM1 = EX1_EX2;
				EX1_EX2 = ID_EX1;
				ID_EX1 = IF2_ID;
				IF2_ID.type = ti_FLUSHED;
				cycle_number++;
				
			} //do not have to update branch table for other cases because the prediction is already correct
		} else {//no branch predictions
			if (ID_EX1.Addr == IF2_ID.PC) {//branch taken
				trace(trace_view_on, cycle_number, MEM2_WB, 8);
					
				/*flush incorrect instruction in IF2_ID and advance others
				
				Because of the way this program works - it doesn't actually take
				an incorrect instruction since it's just tracing a real execution.
				This being said: instead of 'flushing an incorrect instruction', I'm
				simulating it by advancing the branch and all instructions before it,
				then inserting a flushed instruction into the ID_EX1 buffer (where the
				branch was previously) */
				MEM2_WB = MEM1_MEM2;
				MEM1_MEM2 = EX2_MEM1;
				EX2_MEM1 = EX1_EX2;
				EX1_EX2 = ID_EX1;
				ID_EX1.type = ti_FLUSHED;
				cycle_number++;
				
				trace(trace_view_on, cycle_number, MEM2_WB, 8);
				
				MEM2_WB = MEM1_MEM2;
				MEM1_MEM2 = EX2_MEM1;
				EX2_MEM1 = EX1_EX2;
				EX1_EX2 = ID_EX1;
				ID_EX1 = IF2_ID;
				IF2_ID.type = ti_FLUSHED;
				cycle_number++;
			}
		}
	  }
	  
	  if(ID_EX1.type == ti_JTYPE) {//add jump instruction to prediction_table
			int index = (ID_EX1.PC & 0x3F) >> 4;
			struct branch_prediction b;
			b.PC = ID_EX1.PC;
			b.target = ID_EX1.Addr;
			prediction_table[index] = b;
	  }
	  
      
      if(!size){    /* if no more instructions in trace, reduce flush_counter */
        flush_counter--;   
      }
      else{   /* copy trace entry into IF stage */
        memcpy(&PCregister, tr_entry , sizeof(PCregister));
      }

      //printf("==============================================================================\n");
    }  


    trace(trace_view_on, cycle_number, MEM2_WB, 8);
  }

  trace_uninit();  
  
  exit(0);
}

 
