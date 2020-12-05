/*
 * apex_cpu.h
 * Contains APEX cpu pipeline declarations
 *
 * Author:
 * Copyright (c) 2020, Gaurav Kothari (gkothar1@binghamton.edu)
 * State University of New York at Binghamton
 */
#ifndef _APEX_CPU_H_
#define _APEX_CPU_H_

#include "apex_macros.h"
enum
{
	F,
	DRF,
	IQ,
	INTFU,
	MUL1,
	MUL2,
	MUL3,
	BRH1,
    BRH2
};

/* Format of an APEX instruction  */
typedef struct APEX_Instruction
{
    char opcode_str[128];
    int opcode;
    int rd;
    int rs1;
    int rs2;
    int rs3; // used for store
    int imm;
} APEX_Instruction;

typedef struct IQ_ENTRY
{
	int pc;
	char opcode_str[128];
    int opcode;
	int src1;
	int src1_tag;
	int src1_ready;

	int src2;
	int src2_tag;
	int src2_ready;
	int imm;
    int finishedstage;
    int des_phy_reg;

	int fu_type;
	
    int des_rd;
} IQ_ENTRY;
/* Model of CPU stage latch */
typedef struct CPU_Stage
{
    int pc;
    char opcode_str[128];
    int opcode;
    int rs1;
    int rs2;
    int rs3; // Source-3 Register Address for STR instructions
    int rd;
    int imm;
    int rs1_value;
    int rs2_value;
    int rs3_value; // Source-3 Register Value for STR instructions
    int result_buffer;
    int memory_address;
    int has_insn;
    int stalled;
    IQ_ENTRY iq_entry;
} CPU_Stage;


/* Model of APEX CPU */
typedef struct APEX_CPU
{
    int pc;                        /* Current program counter */
    int clock;                     /* Clock cycles elapsed */
    int insn_completed;            /* Instructions retired */
   //R1-15
    int regs[REG_FILE_SIZE];       /* Integer register file */
    /* Integer register file */
	int regs_valid[16];

    int code_memory_size;          /* Number of instruction in the input file */
    APEX_Instruction *code_memory; /* Code Memory */
    int data_memory[DATA_MEMORY_SIZE]; /* Data Memory */
    int single_step;               /* Wait for user input after every cycle */
    int zero_flag;                 /* {TRUE, FALSE} Used by BZ and BNZ to branch */
    int fetch_from_next_cycle;


	

    int phys_regs[48];
	int phys_regs_valid[48];


	// Index represents the PR and values 1- represents free,0 - represents occupied.
	int free_PR_list[48];
	//Number of consumers for every physical register.
	int consumers[48];




	//Rename table to contain info with Index represents the  Physical Register.
	int rename_table[16];
    int rename_table_valid[16];
	//retired Rename table to contain info with Index represents the  Physical Register.
	int r_rename_table[16];
    int r_rename_table_valid[16];
    IQ_ENTRY IssueQueue[24];
  
  
    int freeiq[24];
    /* Pipeline stages */
    CPU_Stage fetch;
    CPU_Stage decode;
    CPU_Stage issuequeue ;
    CPU_Stage intfu ; 
    CPU_Stage mul1 ;
    CPU_Stage mul2 ;
    CPU_Stage mul3 ;
    CPU_Stage jbu1 ;
    CPU_Stage jbu2 ; 
    IQ_ENTRY iq_entry;
} APEX_CPU;

APEX_Instruction *create_code_memory(const char *filename, int *size);
APEX_CPU *APEX_cpu_init(const char *filename);
void APEX_cpu_run(APEX_CPU *cpu);
void APEX_cpu_stop(APEX_CPU *cpu);
void instruction_retirement(APEX_CPU *cpu,IQ_ENTRY iq_entry);
#endif
