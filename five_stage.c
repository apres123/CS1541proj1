/**************************************************************/
/* CS/COE 1541				 			
   compile with gcc -o pipeline five_stage.c			
   and execute using							
   ./pipeline  /afs/cs.pitt.edu/courses/1541/short_traces/sample.tr	0  
***************************************************************/

#include <stdio.h>
#include<stdlib.h>
#include <inttypes.h>
#include <arpa/inet.h>
#include "CPU.h" 

int main(int argc, char **argv)
{
  struct instruction *tr_entry;
  struct instruction PCregister, IF_ID, ID_EX, EX_MEM, MEM_WB; 
  size_t size;
  char *trace_file_name;
  int trace_view_on = 0;
  int flush_counter = 4; //5 stage pipeline, so we have to execute 4 instructions once trace is done
  
  unsigned int cycle_number = 0;

  if (argc == 1) {
    fprintf(stdout, "\nUSAGE: tv <trace_file> <switch - any character>\n");
    fprintf(stdout, "\n(switch) to turn on or off individual item view.\n\n");
    exit(0);
  }
    
  trace_file_name = argv[1];
  if (argc == 3) trace_view_on = atoi(argv[2]) ;

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
          if (trace_view_on && cycle_number>=5) {/* print the instruction exiting the pipeline if trace_view_on=1 */
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
		if (ID_EX.Addr == IF_ID.PC) {
			
			if (trace_view_on && cycle_number>=5) {/* print the instruction exiting the pipeline if trace_view_on=1 */
          switch(MEM_WB.type) {
            case ti_NOP:
              printf("[cycle %d] NOP:\n",cycle_number) ;
              break;
			case ti_FLUSHED:
				printf("[cycle %d] FLUSHED:\n", cycle_number) ;
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

      if(!size){    /* if no more instructions in trace, reduce flush_counter */
        flush_counter--;   
      }
      else{   /* copy trace entry into IF stage */
        memcpy(&PCregister, tr_entry , sizeof(PCregister));          
      }

      //printf("==============================================================================\n");
    }  


    if (trace_view_on && cycle_number>=5) {/* print the instruction exiting the pipeline if trace_view_on=1 */
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

  trace_uninit();

  exit(0);
}

