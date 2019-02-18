/**************************************************************/
/* CS/COE 1541				 			
   compile with gcc -o pipeline five_stage.c			
   and execute using							
   ./pipeline  /afs/cs.pitt.edu/courses/1541/short_traces/sample.tr	0  
***************************************************************/

#include <stdio.h>
#include <stdbool.h>
#include<stdlib.h>
#include <inttypes.h>
#include <arpa/inet.h>
#include "CPU.h" 

int get_index(int PC);

int main(int argc, char **argv)
{
  struct instruction *tr_entry;
  struct instruction PCregister, IF_ID, ID_EX, EX_MEM, MEM_WB; 
  size_t size;
  char *trace_file_name;
  int trace_view_on = 0;
  int prediction_method = 0;
  struct branch_prediction prediction_table[64];
  int flush_counter = 4; //5 stage pipeline, so we have to execute 4 instructions once trace is done
  
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
      MEM_WB = EX_MEM;
      EX_MEM = ID_EX;
      ID_EX = IF_ID;
      IF_ID = PCregister;
      //printf(" (PC: %d)(addr: %d)\n", MEM_WB.PC, MEM_WB.Addr);
      
      /*  check if load instruction in EX stage
          check if instruction in ID stage uses loaded data */
      if (ID_EX.type == ti_LOAD) {
        //printf("IS LOAD\n");
        if ((ID_EX.dReg == IF_ID.sReg_a) || (ID_EX.dReg == IF_ID.sReg_b)) {
          //printf("DATA HAZARD\n");
          IF_ID.type = ti_NOP;
          
          //this is gross but i couldnt think of a clean way to do it
          trace(trace_view_on, cycle_number, MEM_WB, 5);
		  
          MEM_WB = EX_MEM;
          EX_MEM = ID_EX;
          ID_EX = IF_ID;
          IF_ID = PCregister;
          cycle_number++;
        }
      }
	  /*IMPORTANT: to simulate a flushed instruction I added a new type field to the enum for instruction in CPU.h
	    Make sure that if you're printing because of trace_view_on that the switch statement includes case ti_FLUSHED: */
		
	  /*assuming branch condition resolved in ID stage
	  /*if instruction is branch*/
	  if (ID_EX.type == ti_BRANCH) {
		/*if the target address field is equal to PC of next instruction*/
		
		if(prediction_method == 1) {//apply branch prediction
			int index = get_index(ID_EX.PC );//indexing with bits 9-4 for prediction_table
			struct branch_prediction curr = prediction_table[index];//indexing with bits 9-4
			if (ID_EX.Addr == IF_ID.PC && (curr.PC != ID_EX.PC || (curr.PC == ID_EX.PC && curr.prediction == false))) {//false prediction
				
				//update branch prediction table
				struct branch_prediction b;
				b.prediction = true;
				b.PC = ID_EX.PC;
				b.target = ID_EX.Addr;
				prediction_table[index] = b;
				
				trace(trace_view_on, cycle_number, MEM_WB, 5);
				
				/*flush incorrect instruction in IF_ID and advance others
			
				Because of the way this program works - it doesn't actually take
				an incorrect instruction since it's just tracing a real execution.
				This being said: instead of 'flushing an incorrect instruction', I'm
				simulating it by advancing the branch and all instructions before it,
				then inserting a flushed instruction into the ID_EX buffer (where the
				branch was previously) */
				MEM_WB = EX_MEM;
				EX_MEM = ID_EX;
				ID_EX.type = ti_FLUSHED;
				cycle_number++;
			} else if(ID_EX.Addr != IF_ID.PC && curr.PC != ID_EX.PC) {//no prediction, but correct path
			
			//update branch prediction table
				struct branch_prediction b;
				b.prediction = false;
				b.PC = ID_EX.PC;
				b.target = ID_EX.PC + 4;
				prediction_table[index] = b;
			} else if(ID_EX.Addr != IF_ID.PC && (curr.PC == ID_EX.PC && curr.prediction == true)){//false prediction
				//update branch prediction table
				struct branch_prediction b;
				b.prediction = false;
				b.PC = ID_EX.PC;
				b.target = ID_EX.PC + 4;
				prediction_table[index] = b;
				
				trace(trace_view_on, cycle_number, MEM_WB, 5);
				
				/*flush incorrect instruction in IF_ID and advance others
			
				Because of the way this program works - it doesn't actually take
				an incorrect instruction since it's just tracing a real execution.
				This being said: instead of 'flushing an incorrect instruction', I'm
				simulating it by advancing the branch and all instructions before it,
				then inserting a flushed instruction into the ID_EX buffer (where the
				branch was previously) */
				MEM_WB = EX_MEM;
				EX_MEM = ID_EX;
				ID_EX.type = ti_FLUSHED;
				cycle_number++;
				
			} //do not have to update branch table for other cases because the prediction is already correct
		} else {//no branch predictions
			if (ID_EX.Addr == IF_ID.PC) {//branch taken
				trace(trace_view_on, cycle_number, MEM_WB, 5);
					
				/*flush incorrect instruction in IF_ID and advance others
				
				Because of the way this program works - it doesn't actually take
				an incorrect instruction since it's just tracing a real execution.
				This being said: instead of 'flushing an incorrect instruction', I'm
				simulating it by advancing the branch and all instructions before it,
				then inserting a flushed instruction into the ID_EX buffer (where the
				branch was previously) */
				MEM_WB = EX_MEM;
				EX_MEM = ID_EX;
				ID_EX.type = ti_FLUSHED;
				cycle_number++;
			}
		}
	  }
	  
	  if(ID_EX.type == ti_JTYPE) {//add jump instruction to prediction_table
			int index = get_index(ID_EX.PC);
			struct branch_prediction b;
			b.PC = ID_EX.PC;
			b.target = ID_EX.Addr;
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

	trace(trace_view_on, cycle_number, MEM_WB, 5);
    
  }

  trace_uninit();

  exit(0);
}

int get_index(int PC) {
	return (PC & 0x3F) >> 4;
}



