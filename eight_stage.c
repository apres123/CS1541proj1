/**************************************************************/
/* CS/COE 1541				 			
   compile with gcc -o pipeline eight_stage.c			
   and execute using							
   ./pipeline  /afs/cs.pitt.edu/courses/1541/short_traces/sample.tr	0  
***************************************************************/

#include <stdio.h>
#include<stdlib.h>
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
  int flush_counter = 7; //8 stage pipeline, so we have to execute 7 instructions once trace is done
  
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
      MEM2_WB = MEM1_MEM2;
      MEM1_MEM2 = EX2_MEM1;
      EX2_MEM1 = EX1_EX2;
      EX1_EX2 = ID_EX1;
      ID_EX1 = IF2_ID;
      IF2_ID = IF1_IF2;
      IF1_IF2 = PCregister;
      
      /*DATA HAZARD
        1. EX1_EX2 write into register, and ID_EX1 reads from that register. insert no-op into EX1_EX2.
        2. EX2_MEM1 or MEM1_MEM2 is LOAD into register, and ID_EX1 reads from that register. insert no-op into EX1_EX2.
      */
      
      //1st hazard
      if ((EX1_EX2.dReg == ID_EX1.sReg_a) || (EX1_EX2.dReg == ID_EX1.sReg_b)) {
        //print out wb
        if (trace_view_on && cycle_number>=8) {
          print_MEM2_WB(MEM2_WB, cycle_number);
        }
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
          if (trace_view_on && cycle_number>=8) {
            print_MEM2_WB(MEM2_WB, cycle_number);
          }
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
          if (trace_view_on && cycle_number>=8) {
            print_MEM2_WB(MEM2_WB, cycle_number);
          }
          //move up and change ex1ex2 to no op
          MEM2_WB = MEM1_MEM2;
          MEM1_MEM2 = EX2_MEM1;
          EX2_MEM1 = EX1_EX2;
          EX1_EX2.type = ti_NOP;
          cycle_number++;
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


    if (trace_view_on && cycle_number>=8) {/* print the instruction exiting the pipeline if trace_view_on=1 */
      switch(MEM2_WB.type) {
        case ti_NOP:
          printf("[cycle %d] NOP:\n",cycle_number) ;
          break;
        case ti_FLUSHED:
          printf("[cycle %d] FLUSHED:\n", cycle_number) ;
          break;
        case ti_RTYPE: /* registers are translated for printing by subtracting offset  */
          printf("[cycle %d] RTYPE:",cycle_number) ;
		  printf(" (PC: %d)(sReg_a: %d)(sReg_b: %d)(dReg: %d) \n", MEM2_WB.PC, MEM2_WB.sReg_a, MEM2_WB.sReg_b, MEM2_WB.dReg);
          break;
        case ti_ITYPE:
          printf("[cycle %d] ITYPE:",cycle_number) ;
		  printf(" (PC: %d)(sReg_a: %d)(dReg: %d)(addr: %d)\n", MEM2_WB.PC, MEM2_WB.sReg_a, MEM2_WB.dReg, MEM2_WB.Addr);
          break;
        case ti_LOAD:
          printf("[cycle %d] LOAD:",cycle_number) ;      
		  printf(" (PC: %d)(sReg_a: %d)(dReg: %d)(addr: %d)\n", MEM2_WB.PC, MEM2_WB.sReg_a, MEM2_WB.dReg, MEM2_WB.Addr);
          break;
        case ti_STORE:
          printf("[cycle %d] STORE:",cycle_number) ;      
		  printf(" (PC: %d)(sReg_a: %d)(sReg_b: %d)(addr: %d)\n", MEM2_WB.PC, MEM2_WB.sReg_a, MEM2_WB.sReg_b, MEM2_WB.Addr);
          break;
        case ti_BRANCH:
          printf("[cycle %d] BRANCH:",cycle_number) ;
		  printf(" (PC: %d)(sReg_a: %d)(sReg_b: %d)(addr: %d)\n", MEM2_WB.PC, MEM2_WB.sReg_a, MEM2_WB.sReg_b, MEM2_WB.Addr);
          break;
        case ti_JTYPE:
          printf("[cycle %d] JTYPE:",cycle_number) ;
		  printf(" (PC: %d)(addr: %d)\n", MEM2_WB.PC, MEM2_WB.Addr);
          break;
        case ti_SPECIAL:
          printf("[cycle %d] SPECIAL:\n",cycle_number) ;      	
          break;
        case ti_JRTYPE:
          printf("[cycle %d] JRTYPE:",cycle_number) ;
		  printf(" (PC: %d) (sReg_a: %d)(addr: %d)\n", MEM2_WB.PC, MEM2_WB.dReg, MEM2_WB.Addr);
          break;
      }
    }
  }

  trace_uninit();  
  
  exit(0);
}

  /*When doing data hazards, I have to print an extra time
    so im making this function instead of copy-pasting the print stuff twice lol
    if you guys think of a better way lmk
  */
void print_MEM2_WB(struct instruction MEM2_WB, int cycle_number) {
  switch(MEM2_WB.type) {
      case ti_NOP:
        printf("[cycle %d] NOP:\n",cycle_number) ;
        break;
      case ti_FLUSHED:
        printf("[cycle %d] FLUSHED:\n", cycle_number) ;
        break;
      case ti_RTYPE: /* registers are translated for printing by subtracting offset  */
        printf("[cycle %d] RTYPE:",cycle_number) ;
    printf(" (PC: %d)(sReg_a: %d)(sReg_b: %d)(dReg: %d) \n", MEM2_WB.PC, MEM2_WB.sReg_a, MEM2_WB.sReg_b, MEM2_WB.dReg);
        break;
      case ti_ITYPE:
        printf("[cycle %d] ITYPE:",cycle_number) ;
    printf(" (PC: %d)(sReg_a: %d)(dReg: %d)(addr: %d)\n", MEM2_WB.PC, MEM2_WB.sReg_a, MEM2_WB.dReg, MEM2_WB.Addr);
        break;
      case ti_LOAD:
        printf("[cycle %d] LOAD:",cycle_number) ;      
    printf(" (PC: %d)(sReg_a: %d)(dReg: %d)(addr: %d)\n", MEM2_WB.PC, MEM2_WB.sReg_a, MEM2_WB.dReg, MEM2_WB.Addr);
        break;
      case ti_STORE:
        printf("[cycle %d] STORE:",cycle_number) ;      
    printf(" (PC: %d)(sReg_a: %d)(sReg_b: %d)(addr: %d)\n", MEM2_WB.PC, MEM2_WB.sReg_a, MEM2_WB.sReg_b, MEM2_WB.Addr);
        break;
      case ti_BRANCH:
        printf("[cycle %d] BRANCH:",cycle_number) ;
    printf(" (PC: %d)(sReg_a: %d)(sReg_b: %d)(addr: %d)\n", MEM2_WB.PC, MEM2_WB.sReg_a, MEM2_WB.sReg_b, MEM2_WB.Addr);
        break;
      case ti_JTYPE:
        printf("[cycle %d] JTYPE:",cycle_number) ;
    printf(" (PC: %d)(addr: %d)\n", MEM2_WB.PC, MEM2_WB.Addr);
        break;
      case ti_SPECIAL:
        printf("[cycle %d] SPECIAL:\n",cycle_number) ;      	
        break;
      case ti_JRTYPE:
        printf("[cycle %d] JRTYPE:",cycle_number) ;
    printf(" (PC: %d) (sReg_a: %d)(addr: %d)\n", MEM2_WB.PC, MEM2_WB.dReg, MEM2_WB.Addr);
        break;
        
    }
}
