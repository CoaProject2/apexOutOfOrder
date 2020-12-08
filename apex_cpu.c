/*
 * apex_cpu.c
 * Contains APEX cpu pipeline implementation
 *
 * Author:
 * Copyright (c) 2020, Gaurav Kothari (gkothar1@binghamton.edu)
 * State University of New York at Binghamton
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "apex_cpu.h"
#include "apex_macros.h"
int funct = 0; // 0 for simulate 1 for display and single step for 2
int numOfCycles = 0;
int ENABLE_DEBUG_MESSAGES = TRUE;
/* Converts the PC(4000 series) into array index for code memory
 *
 * Note: You are not supposed to edit this function
 */
static int
get_code_memory_index_from_pc(const int pc)
{
    return (pc - 4000) / 4;
}

static void
print_instruction(const CPU_Stage *stage)
{
    switch (stage->opcode)
    {
    case OPCODE_ADD:
    case OPCODE_SUB:
    case OPCODE_MUL:
    case OPCODE_DIV:
    case OPCODE_AND:
    case OPCODE_OR:
    case OPCODE_XOR:
    {
        printf("%s,R%d,R%d,R%d ", stage->opcode_str, stage->rd, stage->rs1,
               stage->rs2);
        break;
    }
    case OPCODE_LDR:
    {
        {
            printf("%s,R%d,R%d,R%d ", stage->opcode_str, stage->rd, stage->rs1,
                   stage->rs2);
            break;
        }
    }

    case OPCODE_STR:
    {
        printf("%s,R%d,R%d,R%d ", stage->opcode_str, stage->rs1, stage->rs2,
               stage->rs3);
        break;
    }

    case OPCODE_MOVC:
    {
        printf("%s,R%d,#%d ", stage->opcode_str, stage->rd, stage->imm);
        break;
    }

    case OPCODE_ADDL:
    case OPCODE_SUBL:

    {
        printf("%s,R%d,R%d,#%d ", stage->opcode_str, stage->rd, stage->rs1, stage->imm);
        break;
    }
    case OPCODE_CMP:
    {
        printf("%s,R%d,R%d", stage->opcode_str, stage->rs1, stage->rs2);
        break;
    }
    case OPCODE_LOAD:
    {
        printf("%s,R%d,R%d,#%d ", stage->opcode_str, stage->rd, stage->rs1,
               stage->imm);
        break;
    }

    case OPCODE_STORE:
    {
        printf("%s,R%d,R%d,#%d ", stage->opcode_str, stage->rs1, stage->rs2,
               stage->imm);
        break;
    }

    case OPCODE_BZ:
    case OPCODE_BNZ:

    {
        printf("%s,#%d ", stage->opcode_str, stage->imm);
        break;
    }

    case OPCODE_HALT:
    {
        printf("%s", stage->opcode_str);
        break;
    }
    case OPCODE_NOP:
    {
        printf("%s", stage->opcode_str);
        break;
    }
    case OPCODE_JUMP:
    {
        printf("%s,R%d,#%d ", stage->opcode_str, stage->rs1, stage->imm);
        break;
    }
    case OPCODE_JAL:
    {
        printf("%s,R%d,R%d,#%d ", stage->opcode_str, stage->rd, stage->rs1,
               stage->imm);
        break;
    }
    }
}
/* Debug function which prints the CPU stage content
 *
 * Note: You can edit this function to print in more detail
 */
static void
print_stage_content(const char *name, const CPU_Stage *stage)
{
    if (funct != 0)
    {
        printf("%-15s: pc(%d) ", name, stage->pc);
        print_instruction(stage);
        printf("\n");
    }
}
static void printdatamemory(APEX_CPU *cpu)
{
    printf("============== STATE OF DATA MEMORY =============\n");

    for (int count = 1000; count <= 1005; count++)
    {
        printf("|           MEM[%d]       |     Data  Value=%d        |\n", count, cpu->data_memory[count]);
    }
}

/* Debug function which prints the register file
 *
 * Note: You are not supposed to edit this function
 */
// static void
// print_reg_file(const APEX_CPU *cpu)
// {
//     int i;
//     printf("----------\n%s\n----------\n", "Registers:");

//     for (int i = 0; i < REG_FILE_SIZE / 2; ++i)
//     {
//         printf("R%-3d[%-3d] ", i, cpu->regs[i]);
//     }
//     printf("\n");
//     for (i = (REG_FILE_SIZE / 2); i < REG_FILE_SIZE; ++i)
//     {
//         printf("R%-3d[%-3d] ", i, cpu->regs[i]);
//     }
//     printf("\n");
// }

/*
 * Fetch Stage of APEX Pipeline
 *
 * Note: You are free to edit this function according to your implementation
 */

static void print_rename_table(APEX_CPU *cpu)
{
    printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
    printf("Details of RENAME TABLE State --\n");
    for (int i = 0; i < 16; i++)
    {
        if (cpu->rename_table[i] != -1)
        {
            printf("RAT[%d] -> P[%d] == %d  \n ", i, cpu->rename_table[i], cpu->phys_regs[cpu->rename_table[i]]);
        }
    }
}
static void print_r_rename_table(APEX_CPU *cpu)
{
    printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
    printf("Details of R-RENAME TABLE State --\n");
    for (int i = 0; i < 16; i++)
    {
        if (cpu->r_rename_table[i] != -1)
        {
            printf("R-RAT[%d] -> P[%d] == %d  \n ", i, cpu->r_rename_table[i], cpu->phys_regs[cpu->r_rename_table[i]]);
        }
    }
}
static void print_rob(APEX_CPU *cpu)
{
    printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
    printf("Details of R-ROB  State --\n");

    int a = cpu->rob_head;
    while (a < cpu->rob_tail)
    {
        ROB_ENTRY *rob_entry = &cpu->ROB[a];

        printf("%s,R%d __pc[%d] \n  ", rob_entry->opcode_str, rob_entry->des_rd, rob_entry->pc);

        a++;
    }
    printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
}
static void print_physical_register(APEX_CPU *cpu)
{

    printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
    printf("Details of Valid Physical register State --\n");
     printf(" P[#]  ==== [valid] ----- [value]\n  " );
    for (int i = 0; i < 48; i++)
    {
        // if (cpu->free_PR_list[i] == 1)
        {

            printf(" P[%d]  ==== [%d] ----- [%d]\n  ", i, cpu->phys_regs_valid[i],cpu->phys_regs[i]);
        }
    }
}
static void
APEX_fetch(APEX_CPU *cpu)
{
    APEX_Instruction *current_ins;
    if (cpu->fetch.has_insn && !cpu->is_stalled)
    {
        if (!cpu->fetch.stalled)
        {

            /* This fetches new branch target instruction from next cycle */
            if (cpu->fetch_from_next_cycle == TRUE)
            {
                cpu->fetch_from_next_cycle = FALSE;

                /* Skip this cycle*/
                return;
            }
            /* Store current PC in fetch latch */
            cpu->fetch.pc = cpu->pc;
            /* Index into code memory using this pc and copy all instruction fields
         * into fetch latch  */
            current_ins = &cpu->code_memory[get_code_memory_index_from_pc(cpu->pc)];
            strcpy(cpu->fetch.opcode_str, current_ins->opcode_str);
            cpu->fetch.opcode = current_ins->opcode;
            cpu->fetch.rd = current_ins->rd;
            cpu->fetch.rs1 = current_ins->rs1;
            cpu->fetch.rs2 = current_ins->rs2;
            cpu->fetch.rs3 = current_ins->rs3;
            cpu->fetch.imm = current_ins->imm;

            if (!cpu->decode.stalled)
            {
                /* Update PC for next instruction */
                cpu->pc += 4;

                /* Copy data from fetch latch to decode latch*/
                cpu->decode = cpu->fetch;
            }
            else
            {
                cpu->fetch.stalled = 1;
            }
            /* Stop fetching new instructions if HALT is fetched */
            // if (cpu->fetch.opcode == OPCODE_HALT)
            // {
            //     cpu->fetch.has_insn = FALSE;
            // }

            if (ENABLE_DEBUG_MESSAGES)
            {
                print_stage_content("Instruction at Fetch____________Stage--->", &cpu->fetch);
                printf("\n");
            }
        }
    }
    else if (ENABLE_DEBUG_MESSAGES)
    {
        printf("Instruction at fetch____________Stage---empty>");
        printf("\n");
    }
}

/*
 * Decode Stage of APEX Pipeline
 *
 * Note: You are free to edit this function according to your implementation
 */
static void
APEX_decode(APEX_CPU *cpu)
{
    if (cpu->decode.has_insn)
    {
        int stagestalled = 0;
        /*create a iq entruy*/
        if (stagestalled == 0)
        {
            IQ_ENTRY *iq_entry = NULL;

            if (cpu->decode.opcode == OPCODE_HALT)
            {
                // halt should got to ROB, not IQ
                //rob
                ROB_ENTRY *rob_entry = &cpu->ROB[cpu->rob_tail];
                rob_entry->pc = cpu->decode.pc;
                rob_entry->src1 = -1;
                rob_entry->src2 = -1;
                rob_entry->des_rd = cpu->decode.rd;
                rob_entry->exception_codes = 0;
                rob_entry->result_valid = 1;
                rob_entry->result = 0;
                rob_entry->mready = 0;
                rob_entry->instruction_type = cpu->decode.opcode;
                strcpy(rob_entry->opcode_str, cpu->decode.opcode_str);
                rob_entry->des_phy_reg = -1;
                cpu->rob_tail = (cpu->rob_tail + 1) % 64;
                //rob end

            }
            else if (cpu->decode.opcode == OPCODE_BZ || cpu->decode.opcode == OPCODE_BNZ)
            {
                int i = 0;
                for (i = 0; i < 24; i++)
                {
                    /*iq means iq entry occupied o means free 1 means occupied*/
                    if (cpu->freeiq[i] < 1)
                    {
                        break;
                    }
                }
                if (i == 24)
                {
                    stagestalled = 1;
                }
                if (stagestalled == 0)
                {
                    /*fill the iq entry*/
                    for (int i = 0; i < 24; i++)
                    {
                        if (cpu->freeiq[i] == 0)
                        {
                            iq_entry = &cpu->IssueQueue[i];
                            cpu->freeiq[i] = 1;
                            break;
                        }
                    }
                    iq_entry->opcode = cpu->decode.opcode;
                    strcpy(iq_entry->opcode_str, cpu->decode.opcode_str);
                    iq_entry->imm = cpu->decode.imm;
                    iq_entry->pc = cpu->decode.pc;

                    //rob
                    ROB_ENTRY *rob_entry = &cpu->ROB[cpu->rob_tail];
                    rob_entry->pc = cpu->decode.pc;
                    rob_entry->imm = cpu->decode.imm;
                    rob_entry->exception_codes = 0;
                    rob_entry->result_valid = 0;
                    rob_entry->result = 0;
                    rob_entry->mready = 0;
                    rob_entry->instruction_type = cpu->decode.opcode;
                    strcpy(rob_entry->opcode_str, cpu->decode.opcode_str);
                    iq_entry->rob_tail = cpu->rob_tail; // rob index assigned
                    cpu->rob_tail = (cpu->rob_tail + 1) % 64;
                    //rob end
                    if (cpu->decode.opcode == OPCODE_BZ)
                    {
                        if (cpu->zero_flag == TRUE)
                        {
                            //  cpu->is_stalled = 1;
                        }
                    }
                    if (cpu->decode.opcode == OPCODE_BNZ)
                    {
                        if (cpu->zero_flag == FALSE)
                        {
                            //cpu->is_stalled = 1;
                        }
                    }

                    iq_entry->fu_type = 5;
                    cpu->decode.has_insn = FALSE;
                }
            }
            else
            {

                int rs1_physical = cpu->decode.rs1 > -1 ? cpu->rename_table[cpu->decode.rs1] : -1;
                int rs2_physical = cpu->decode.rs2 > -1 ? cpu->rename_table[cpu->decode.rs2] : -1;
                int rs3_physical = -1;
                int first_free_phy_reg;
                if (cpu->decode.opcode == OPCODE_STR)
                    rs3_physical = cpu->decode.rs3 > -1 ? cpu->rename_table[cpu->decode.rs3] : -1;

                if ((cpu->decode.opcode == OPCODE_ADD) || (cpu->decode.opcode == OPCODE_ADDL) || (cpu->decode.opcode == OPCODE_AND) || (cpu->decode.opcode == OPCODE_MUL) || (cpu->decode.opcode == OPCODE_DIV) || (cpu->decode.opcode == OPCODE_OR) || (cpu->decode.opcode == OPCODE_JAL) || (cpu->decode.opcode == OPCODE_SUB) || (cpu->decode.opcode == OPCODE_JUMP) || (cpu->decode.opcode == OPCODE_MOVC) || (cpu->decode.opcode == OPCODE_SUBL) || (cpu->decode.opcode == OPCODE_LOAD) || (cpu->decode.opcode == OPCODE_XOR))
                {
                    first_free_phy_reg = -1;

                    for (int i = 0; i < 48; i++)
                    {
                        if (cpu->free_PR_list[i] == 0)
                        {
                            first_free_phy_reg = i;
                            cpu->free_PR_list[i] = 1;
                            break;
                        }
                    }

                    if (first_free_phy_reg > -1)
                    {
                        cpu->rename_table[cpu->decode.rd] = first_free_phy_reg;
                        //add a rename table entry made rename table contents as invalid
                        cpu->rename_table_valid[cpu->decode.rd] = 1;

                        cpu->phys_regs_valid[first_free_phy_reg] = 0;
                    }

                    if (!(first_free_phy_reg > -1))
                    {
                        stagestalled = 1;
                    }
                }
                int i = 0;
                for (i = 0; i < 24; i++)
                {
                    /*iq means iq entry occupied o means free 1 means occupied*/
                    if (cpu->freeiq[i] < 1)
                    {
                        break;
                    }
                }
                if (i == 24)
                {
                    stagestalled = 1;
                }

                if (cpu->decode.opcode != OPCODE_LOAD && cpu->decode.opcode != OPCODE_LDR && cpu->decode.opcode != OPCODE_STORE && cpu->decode.opcode != OPCODE_STR)
                {
                    if (stagestalled == 0)
                    {
                        /*fill the iq entry*/
                        for (int i = 0; i < 24; i++)
                        {
                            if (cpu->freeiq[i] == 0)
                            {
                                iq_entry = &cpu->IssueQueue[i];
                                cpu->freeiq[i] = 1;
                                break;
                            }
                        }
                        iq_entry->opcode = cpu->decode.opcode;
                        strcpy(iq_entry->opcode_str, cpu->decode.opcode_str);
                        iq_entry->src1 = rs1_physical;
                        iq_entry->src2 = rs2_physical;
                        iq_entry->imm = cpu->decode.imm;
                        iq_entry->pc = cpu->decode.pc;
                        iq_entry->des_phy_reg = first_free_phy_reg;
                        iq_entry->des_rd = cpu->decode.rd;
                        switch (cpu->decode.opcode)
                        {

                        case OPCODE_ADD:
                        case OPCODE_SUB:
                        case OPCODE_MOVC:
                        case OPCODE_CMP:
                        {
                            //rob
                            ROB_ENTRY *rob_entry = &cpu->ROB[cpu->rob_tail];
                            rob_entry->pc = cpu->decode.pc;
                            rob_entry->src1 = rs1_physical;
                            rob_entry->src2 = rs2_physical;
                            rob_entry->des_rd = cpu->decode.rd;
                            rob_entry->exception_codes = 0;
                            rob_entry->result_valid = 0;
                            rob_entry->result = 0;
                            rob_entry->mready = 0;
                            rob_entry->instruction_type = cpu->decode.opcode;
                            strcpy(rob_entry->opcode_str, cpu->decode.opcode_str);
                            rob_entry->des_phy_reg = first_free_phy_reg;
                            iq_entry->rob_tail = cpu->rob_tail; // rob index assigned
                            cpu->rob_tail = (cpu->rob_tail + 1) % 64;
                            //rob end

                            /*3 for ifu*/
                            iq_entry->fu_type = 3;
                            cpu->decode.has_insn = FALSE;
                            break;
                        }
                        case OPCODE_MUL:
                        {
                            //rob
                            ROB_ENTRY *rob_entry = &cpu->ROB[cpu->rob_tail];
                            rob_entry->pc = cpu->decode.pc;
                            rob_entry->src1 = rs1_physical;
                            rob_entry->src2 = rs2_physical;
                            rob_entry->des_rd = cpu->decode.rd;
                            rob_entry->exception_codes = 0;
                            rob_entry->result_valid = 0;
                            rob_entry->result = 0;
                            rob_entry->mready = 0;
                            rob_entry->instruction_type = cpu->decode.opcode;
                            strcpy(rob_entry->opcode_str, cpu->decode.opcode_str);
                            rob_entry->des_phy_reg = first_free_phy_reg;
                            iq_entry->rob_tail = cpu->rob_tail; // rob index assigned
                            cpu->rob_tail = (cpu->rob_tail + 1) % 64;
                            //rob end
                            iq_entry->fu_type = 4;
                            cpu->decode.has_insn = FALSE;
                            break;
                        }
                        case OPCODE_JUMP:
                        case OPCODE_JAL:
                        {
                            //rob
                            ROB_ENTRY *rob_entry = &cpu->ROB[cpu->rob_tail];
                            rob_entry->pc = cpu->decode.pc;
                            rob_entry->src1 = rs1_physical;
                            rob_entry->des_rd = cpu->decode.rd;
                            rob_entry->exception_codes = 0;
                            rob_entry->result_valid = 0;
                            rob_entry->result = 0;
                            rob_entry->mready = 0;
                            rob_entry->instruction_type = cpu->decode.opcode;
                            strcpy(rob_entry->opcode_str, cpu->decode.opcode_str);
                            rob_entry->des_phy_reg = first_free_phy_reg;
                            iq_entry->rob_tail = cpu->rob_tail; // rob index assigned
                            cpu->rob_tail = (cpu->rob_tail + 1) % 64;
                            //rob end

                            // cpu->is_stalled = 1;
                            iq_entry->fu_type = 5;
                            cpu->decode.has_insn = FALSE;
                            break;
                        }
                        }
                    }
                }
                else
                {
                    switch (cpu->decode.opcode)
                    {
                    case OPCODE_LDR:
                    {

                        ROB_ENTRY *rob_entry = &cpu->ROB[cpu->rob_tail];
                        rob_entry->pc = cpu->decode.pc;
                        rob_entry->src1 = rs1_physical;
                        rob_entry->src2 = rs2_physical;
                        rob_entry->des_rd = cpu->decode.rd;
                        rob_entry->exception_codes = 0;
                        rob_entry->result_valid = 0;
                        rob_entry->result = 0;
                        rob_entry->instruction_type = cpu->decode.opcode;
                        strcpy(rob_entry->opcode_str, cpu->decode.opcode_str);
                        rob_entry->des_phy_reg = first_free_phy_reg;

                        if (cpu->phys_regs_valid[rs1_physical] == 1 && cpu->phys_regs_valid[rs2_physical] == 1)
                        {
                            rob_entry->mready = 1;

                            cpu->rob_current_instruction = cpu->rob_tail;
                            cpu->rob_tail = (cpu->rob_tail + 1) % 64;
                            cpu->memory1.rob_entry = rob_entry;

                            cpu->memory1.has_insn = TRUE;
                        }

                        else
                        {
                            //create entry in iq and wait for The ready bit

                            rob_entry->mready = 0;

                            cpu->rob_current_instruction = cpu->rob_tail;
                            cpu->rob_tail = (cpu->rob_tail + 1) % 64;

                            /*fill the iq entry*/
                            for (int i = 0; i < 24; i++)
                            {
                                if (cpu->freeiq[i] == 0)
                                {
                                    iq_entry = &cpu->IssueQueue[i];
                                    cpu->freeiq[i] = 1;
                                    break;
                                }
                            }
                            iq_entry->opcode = cpu->decode.opcode;
                            strcpy(iq_entry->opcode_str, cpu->decode.opcode_str);
                            iq_entry->src1 = rs1_physical;
                            iq_entry->src2 = rs2_physical;
                            iq_entry->pc = cpu->decode.pc;
                            iq_entry->des_phy_reg = first_free_phy_reg;
                            iq_entry->des_rd = cpu->decode.rd;
                            iq_entry->rob_tail = cpu->rob_current_instruction;
                            cpu->decode.has_insn = FALSE;
                        }
                        break;
                    }
                    case OPCODE_LOAD:
                    {
                        ROB_ENTRY *rob_entry = &cpu->ROB[cpu->rob_tail];
                        rob_entry->pc = cpu->decode.pc;
                        rob_entry->src1 = rs1_physical;

                        rob_entry->imm = cpu->decode.imm;
                        rob_entry->des_rd = cpu->decode.rd;
                        rob_entry->exception_codes = 0;
                        rob_entry->result_valid = 0;
                        rob_entry->result = 0;
                        rob_entry->instruction_type = cpu->decode.opcode;
                        strcpy(rob_entry->opcode_str, cpu->decode.opcode_str);
                        rob_entry->des_phy_reg = first_free_phy_reg;

                        if (cpu->phys_regs_valid[rs1_physical] == 1)
                        {
                            rob_entry->mready = 1;
                            cpu->rob_current_instruction = cpu->rob_tail;
                            cpu->rob_tail = (cpu->rob_tail + 1) % 64;
                            cpu->memory1.rob_entry = rob_entry;

                            cpu->memory1.has_insn = TRUE;
                        }
                        else
                        {
                            //create entry in iq and wait for The ready bit

                            //create entry in iq and wait for The ready bit

                            rob_entry->mready = 0;

                            cpu->rob_current_instruction = cpu->rob_tail;
                            cpu->rob_tail = (cpu->rob_tail + 1) % 64;

                            /*fill the iq entry*/
                            for (int i = 0; i < 24; i++)
                            {
                                if (cpu->freeiq[i] == 0)
                                {
                                    iq_entry = &cpu->IssueQueue[i];
                                    cpu->freeiq[i] = 1;
                                    break;
                                }
                            }
                            iq_entry->opcode = cpu->decode.opcode;
                            strcpy(iq_entry->opcode_str, cpu->decode.opcode_str);
                            iq_entry->src1 = rs1_physical;
                            iq_entry->src2 = rs2_physical;
                            iq_entry->pc = cpu->decode.pc;
                            iq_entry->des_phy_reg = first_free_phy_reg;
                            iq_entry->des_rd = cpu->decode.rd;
                            iq_entry->rob_tail = cpu->rob_current_instruction;
                            cpu->decode.has_insn = FALSE;
                        }
                        break;
                    }
                    case OPCODE_STORE:
                    {

                        ROB_ENTRY *rob_entry = &cpu->ROB[cpu->rob_tail];
                        rob_entry->pc = cpu->decode.pc;
                        rob_entry->src1 = rs1_physical;
                        rob_entry->src2 = rs2_physical;
                        rob_entry->imm = cpu->decode.imm;
                        rob_entry->exception_codes = 0;
                        rob_entry->result_valid = 0;
                        rob_entry->result = 0;
                        rob_entry->instruction_type = cpu->decode.opcode;
                        strcpy(rob_entry->opcode_str, cpu->decode.opcode_str);
                        rob_entry->des_phy_reg = first_free_phy_reg;
                        if (cpu->phys_regs_valid[rs1_physical] == 1 && cpu->phys_regs_valid[rs2_physical] == 1)
                        {
                            rob_entry->mready = 1;
                            cpu->rob_current_instruction = cpu->rob_tail;
                            cpu->rob_tail = (cpu->rob_tail + 1) % 64;
                            cpu->memory1.rob_entry = rob_entry;

                            cpu->memory1.has_insn = TRUE;
                        }
                        else
                        {
                            //create entry in iq and wait for The mready bit

                            rob_entry->mready = 0;

                            cpu->rob_current_instruction = cpu->rob_tail;
                            cpu->rob_tail = (cpu->rob_tail + 1) % 64;

                            /*fill the iq entry*/
                            for (int i = 0; i < 24; i++)
                            {
                                if (cpu->freeiq[i] == 0)
                                {
                                    iq_entry = &cpu->IssueQueue[i];
                                    cpu->freeiq[i] = 1;
                                    break;
                                }
                            }
                            iq_entry->opcode = cpu->decode.opcode;
                            strcpy(iq_entry->opcode_str, cpu->decode.opcode_str);
                            iq_entry->src1 = rs1_physical;
                            iq_entry->src2 = rs2_physical;
                            iq_entry->imm = cpu->decode.imm;
                            iq_entry->pc = cpu->decode.pc;
                            iq_entry->des_phy_reg = first_free_phy_reg;
                            iq_entry->des_rd = cpu->decode.rd;
                            iq_entry->rob_tail = cpu->rob_current_instruction;
                            cpu->decode.has_insn = FALSE;
                        }
                        break;
                    }
                    case OPCODE_STR:
                    {

                        ROB_ENTRY *rob_entry = &cpu->ROB[cpu->rob_tail];
                        rob_entry->pc = cpu->decode.pc;
                        rob_entry->src1 = rs1_physical;
                        rob_entry->src2 = rs2_physical;
                        rob_entry->src3 = rs3_physical;
                        rob_entry->exception_codes = 0;
                        rob_entry->result_valid = 0;
                        rob_entry->result = 0;
                        rob_entry->instruction_type = cpu->decode.opcode;
                        strcpy(rob_entry->opcode_str, cpu->decode.opcode_str);
                        rob_entry->des_phy_reg = first_free_phy_reg;
                        rob_entry->mready = 1;
                        if (cpu->phys_regs_valid[rs1_physical] == 1 && cpu->phys_regs_valid[rs2_physical] == 1 && cpu->phys_regs_valid[rs3_physical] == 1)
                        {
                            cpu->rob_current_instruction = cpu->rob_tail;
                            cpu->rob_tail = (cpu->rob_tail + 1) % 64;
                            cpu->memory1.rob_entry = rob_entry;
                            cpu->memory1.has_insn = TRUE;
                        }
                        else
                        {
                            //create entry in iq and wait for The ready bit

                            rob_entry->mready = 0;

                            cpu->rob_current_instruction = cpu->rob_tail;
                            cpu->rob_tail = (cpu->rob_tail + 1) % 64;

                            /*fill the iq entry*/
                            for (int i = 0; i < 24; i++)
                            {
                                if (cpu->freeiq[i] == 0)
                                {
                                    iq_entry = &cpu->IssueQueue[i];
                                    cpu->freeiq[i] = 1;
                                    break;
                                }
                            }
                            iq_entry->opcode = cpu->decode.opcode;
                            strcpy(iq_entry->opcode_str, cpu->decode.opcode_str);
                            iq_entry->src1 = rs1_physical;
                            iq_entry->src2 = rs2_physical;
                            iq_entry->src3 = rs3_physical;
                            iq_entry->pc = cpu->decode.pc;
                            iq_entry->des_phy_reg = first_free_phy_reg;
                            iq_entry->des_rd = cpu->decode.rd;
                            iq_entry->rob_tail = cpu->rob_current_instruction;
                            cpu->decode.has_insn = FALSE;
                        }
                        break;
                    }
                    }
                }
            }
        }

        if (ENABLE_DEBUG_MESSAGES)
        {
            print_stage_content("Instruction at decode____________Stage--->", &cpu->decode);
        }
    }
    else if (ENABLE_DEBUG_MESSAGES)
    {
        printf("Instruction at decode____________Stage---empty>");
        printf("\n");
    }
}

static void
APEX_memory1(APEX_CPU *cpu)
{
    ROB_ENTRY *selectedrobentry = cpu->memory1.rob_entry;
    if (cpu->memory1.has_insn && selectedrobentry->mready == 1)
    {
        switch (selectedrobentry->instruction_type)
        {
        case OPCODE_LDR:
        {
            // LDR dest ,SRC2, SRC3
            //dest <- src2+src3
            cpu->memory1.memory_address = cpu->phys_regs[selectedrobentry->src2] + cpu->phys_regs[selectedrobentry->src1];
            // dest reg <- mem addr[memory_address]
            break;
        }
        case OPCODE_LOAD:
        { // load r1,r2,#10
            cpu->memory1.memory_address = cpu->phys_regs[selectedrobentry->src1] + selectedrobentry->imm;
            break;
        }
        case OPCODE_STR:
        {
            // rs1,rs2,r3
            // mem addr[memory_address] <- src1
            cpu->memory1.memory_address = cpu->phys_regs[selectedrobentry->src2] + cpu->phys_regs[selectedrobentry->src3];
            break;
        }
        case OPCODE_STORE:
        {
            // mem addr[memory_address] <- src1
            cpu->memory1.memory_address = cpu->phys_regs[selectedrobentry->src2] + selectedrobentry->imm;
            break;
        }
        }
        cpu->memory2 = cpu->memory1;
        cpu->memory1.has_insn = FALSE;

        if (ENABLE_DEBUG_MESSAGES)
        {
            printf("Instruction at memory1--->");
            printf("\n");
            printf("\n");
        }
    }
    else if (ENABLE_DEBUG_MESSAGES)
    {
        printf("Instruction at memory1____________Stage---empty>");
        printf("\n");
    }
}
static void
APEX_memory2(APEX_CPU *cpu)
{
    ROB_ENTRY *selectedrobentry = cpu->memory2.rob_entry;
    if (cpu->memory2.has_insn && selectedrobentry->mready == 1)
    {

        switch (selectedrobentry->instruction_type)
        {
        case OPCODE_LDR:
        case OPCODE_LOAD:
        {
            // LDR dest ,SRC1, SRC2
            //dest <- src1+src2
            // dest reg <- mem addr[memory_address]
            cpu->memory2.result_buffer = cpu->data_memory[cpu->memory2.memory_address];
            selectedrobentry->exception_codes = 0;
            selectedrobentry->result_valid = 1;
            selectedrobentry->result = cpu->memory2.result_buffer;
            //end

            break;
        }
        case OPCODE_STR:
        case OPCODE_STORE:

        {
            // mem addr[memory_address] <- src1
            //  cpu->data_memory[cpu->memory2.memory_address] = cpu->phys_regs[selectedrobentry->src1];
            cpu->memory2.result_buffer = cpu->phys_regs[selectedrobentry->src1];
            //start
            // ROB_ENTRY *rob_entry = &cpu->ROB[cpu->rob_tail];
            selectedrobentry->exception_codes = 0;
            selectedrobentry->result_valid = 1;
            selectedrobentry->result = cpu->memory2.result_buffer;
            selectedrobentry->des_phy_reg = cpu->memory2.memory_address;
            //end
            break;
        }
        }

        cpu->memory2.has_insn = FALSE;
        if (ENABLE_DEBUG_MESSAGES)
        {
            printf("Instruction at memory2--->");
            printf("\n");
            printf("\n");
        }
    }
    else if (ENABLE_DEBUG_MESSAGES)
    {
        printf("Instruction at memory2____________Stage---empty>");
        printf("\n");
    }
}

/*
 * issuequeue Stage of APEX Pipeline
 *
 * Note: You are free to edit this function according to your implementation
 */
static void
APEX_issuequeue(APEX_CPU *cpu)
{
    int issuequequeindex;
    int intfuissued = -1;
    int mulfuissued = -1;
    int branchfuissued = -1;
    int robissued = -1;

    IQ_ENTRY selectedintfuiqentry;
    IQ_ENTRY selectedmulfuiqentry;
    IQ_ENTRY selectedbranchfuiqentry;
    IQ_ENTRY selectedrobqentry;

    for (int i = 0; i < 24; i++)
    {
        if (cpu->freeiq[i] != 1)
        {
            continue;
        }
        //  printf("iq free %d %d\n", i, cpu->freeiq[i]);
        issuequequeindex = i;
        IQ_ENTRY iqe = cpu->IssueQueue[issuequequeindex];

        switch (iqe.opcode)
        {
        case OPCODE_ADD:
        case OPCODE_SUB:
        case OPCODE_AND:
        case OPCODE_OR:
        case OPCODE_XOR:
        {
            if (cpu->phys_regs_valid[iqe.src1] == 1 && cpu->phys_regs_valid[iqe.src2] == 1)
            {
                selectedintfuiqentry = iqe;
                intfuissued = issuequequeindex;
            }
            break;
        }
        case OPCODE_ADDL:
        case OPCODE_SUBL:
        {
            if (cpu->phys_regs_valid[iqe.src1] == 1)
            {
                selectedintfuiqentry = iqe;
                intfuissued = issuequequeindex;
            }
            break;
        }
        case OPCODE_MOVC:
        {

            selectedintfuiqentry = iqe;
            intfuissued = issuequequeindex;

            break;
        }
        case OPCODE_MUL:
        {
            if (cpu->phys_regs_valid[iqe.src1] == 1 && cpu->phys_regs_valid[iqe.src2] == 1)

            {
                selectedmulfuiqentry = iqe;
                mulfuissued = issuequequeindex;
            }
            break;
        }
        case OPCODE_JUMP:
        case OPCODE_JAL:
        {
            if (cpu->phys_regs_valid[iqe.src1] == 1)
            {
                selectedbranchfuiqentry = iqe;
                branchfuissued = issuequequeindex;
                // cpu->is_stalled = 1;
            }
            break;
        }
        case OPCODE_BZ:
        case OPCODE_BNZ:
        {
            selectedbranchfuiqentry = iqe;
            branchfuissued = issuequequeindex;
            break;
        }
        case OPCODE_HALT:
        {
            selectedintfuiqentry = iqe;
            intfuissued = issuequequeindex;
            break;
        }
        case OPCODE_LDR:
        {
            if (cpu->phys_regs_valid[iqe.src1] == 1 && cpu->phys_regs_valid[iqe.src2] == 1)

            {
                selectedrobqentry = iqe;
                robissued = issuequequeindex;
                cpu->memory1.rob_entry = &cpu->ROB[iqe.rob_tail];
                cpu->ROB[iqe.rob_tail].mready = 1;
            }
            break;
        }
        case OPCODE_LOAD:
        {
            if (cpu->phys_regs_valid[iqe.src1] == 1)

            {
                selectedrobqentry = iqe;
                robissued = issuequequeindex;
                cpu->memory1.rob_entry = &cpu->ROB[iqe.rob_tail];
                cpu->ROB[iqe.rob_tail].mready = 1;
            }
            break;
        }
        case OPCODE_STR:
        {
            if (cpu->phys_regs_valid[iqe.src1] == 1 && cpu->phys_regs_valid[iqe.src2] == 1 && cpu->phys_regs_valid[iqe.src3] == 1)

            {
                selectedrobqentry = iqe;
                robissued = issuequequeindex;
                cpu->memory1.rob_entry = &cpu->ROB[selectedrobqentry.rob_tail];
                cpu->ROB[iqe.rob_tail].mready = 1;
            }
            break;
        }
        case OPCODE_STORE:
        {
            if (cpu->phys_regs_valid[iqe.src1] == 1 && cpu->phys_regs_valid[iqe.src2] == 1)

            {
                selectedrobqentry = iqe;
                robissued = issuequequeindex;
                cpu->memory1.rob_entry = &cpu->ROB[selectedrobqentry.rob_tail];
                cpu->ROB[iqe.rob_tail].mready = 1;
            }
            break;
        }
        }
    }

    if (ENABLE_DEBUG_MESSAGES)
    {
        printf("Instruction at issuequeue____________Stage--->");
        printf("\n");

        IQ_ENTRY *iq_entry1;
        for (int i = 0; i < 24; i++)
        {
            if (cpu->freeiq[i] == 1)
            {
                printf("IQ[0%d] --> ", i);
                iq_entry1 = &cpu->IssueQueue[i];

                printf("%-15s: pc(%d) %s", "Issuequeue ", iq_entry1->pc, iq_entry1->opcode_str);
            }
        }
        printf("\n");
    }

    if (intfuissued > -1)
    {
        IQ_ENTRY entry = selectedintfuiqentry;
        entry.finishedstage = IQ;
        cpu->intfu.iq_entry = entry;
        cpu->intfu.stalled = 0;
        cpu->freeiq[intfuissued] = 0;
        cpu->intfu.has_insn = TRUE;
    }
    if (mulfuissued > -1)
    {
        IQ_ENTRY entry = selectedmulfuiqentry;
        entry.finishedstage = IQ;
        cpu->mul1.iq_entry = entry;
        cpu->mul1.stalled = 0;
        cpu->mul1.has_insn = TRUE;
        cpu->freeiq[mulfuissued] = 0;
    }
    if (branchfuissued > -1)
    {
        IQ_ENTRY entry = selectedbranchfuiqentry;
        entry.finishedstage = IQ;
        cpu->jbu1.iq_entry = entry;
        cpu->jbu1.stalled = 0;

        cpu->jbu1.has_insn = TRUE;
        cpu->freeiq[branchfuissued] = 0;
    }
    if (robissued > -1)
    {
        // IQ_ENTRY entry = selectedrobqentry;
        //     ROB_ENTRY *rob_entry = &cpu->ROB[entry.rob_tail];
        //     rob_entry->mready = 1;
        cpu->memory1.has_insn = TRUE;
        selectedrobqentry.finishedstage = IQ;
        cpu->freeiq[robissued] = 0;
    }
}

static int
APEX_intfu(APEX_CPU *cpu)
{
    IQ_ENTRY iq_entry = cpu->intfu.iq_entry;

    if (!cpu->intfu.stalled && cpu->intfu.has_insn)
    {
        switch (iq_entry.opcode)
        {
        case OPCODE_ADD:
        {
            cpu->intfu.result_buffer = cpu->phys_regs[iq_entry.src1] + cpu->phys_regs[iq_entry.src2];
            if (cpu->intfu.result_buffer == 0)
            {
                cpu->zero_flag = TRUE;
            }
            else
            {
                cpu->zero_flag = FALSE;
            }
            //start
            ROB_ENTRY *rob_entry = &cpu->ROB[iq_entry.rob_tail];
            rob_entry->exception_codes = 0;
            rob_entry->result_valid = 1;
            rob_entry->result = cpu->intfu.result_buffer;
            rob_entry->des_phy_reg = iq_entry.des_phy_reg;
            rob_entry->des_rd = iq_entry.des_rd;
            //end
            break;
        }
        case OPCODE_SUB:
        {
            cpu->intfu.result_buffer = cpu->phys_regs[iq_entry.src1] - cpu->phys_regs[iq_entry.src2];
            if (cpu->intfu.result_buffer == 0)
            {
                cpu->zero_flag = TRUE;
            }
            else
            {
                cpu->zero_flag = FALSE;
            }
            //start
            ROB_ENTRY *rob_entry = &cpu->ROB[iq_entry.rob_tail];
            rob_entry->exception_codes = 0;
            rob_entry->result_valid = 1;
            rob_entry->result = cpu->intfu.result_buffer;
            rob_entry->des_phy_reg = iq_entry.des_phy_reg;
            rob_entry->des_rd = iq_entry.des_rd;
            //end
            break;
        }
        case OPCODE_ADDL:
        {
            cpu->intfu.result_buffer = iq_entry.src1 + iq_entry.imm;

            //start
            ROB_ENTRY *rob_entry = &cpu->ROB[iq_entry.rob_tail];
            rob_entry->exception_codes = 0;
            rob_entry->result_valid = 1;
            rob_entry->result = cpu->intfu.result_buffer;
            rob_entry->des_phy_reg = iq_entry.des_phy_reg;
            rob_entry->des_rd = iq_entry.des_rd;
            //end

            break;
        }

        case OPCODE_SUBL:
        {
            cpu->intfu.result_buffer = iq_entry.src1 - iq_entry.imm;

            if (cpu->intfu.result_buffer == 0)
            {
                cpu->zero_flag = TRUE;
            }
            else
            {
                cpu->zero_flag = FALSE;
            }
            //start
            ROB_ENTRY *rob_entry = &cpu->ROB[iq_entry.rob_tail];
            rob_entry->exception_codes = 0;
            rob_entry->result_valid = 1;
            rob_entry->result = cpu->intfu.result_buffer;
            rob_entry->des_phy_reg = iq_entry.des_phy_reg;
            rob_entry->des_rd = iq_entry.des_rd;
            //end
            break;
        }
        case OPCODE_AND:
        {
            cpu->intfu.result_buffer = cpu->phys_regs[iq_entry.src1] & cpu->phys_regs[iq_entry.src2];

            //start
            ROB_ENTRY *rob_entry = &cpu->ROB[iq_entry.rob_tail];
            rob_entry->exception_codes = 0;
            rob_entry->result_valid = 1;
            rob_entry->result = cpu->intfu.result_buffer;
            rob_entry->des_phy_reg = iq_entry.des_phy_reg;
            rob_entry->des_rd = iq_entry.des_rd;
            //end
            break;
        }
        case OPCODE_OR:
        {
            cpu->intfu.result_buffer = cpu->phys_regs[iq_entry.src1] | cpu->phys_regs[iq_entry.src2];

            //start
            ROB_ENTRY *rob_entry = &cpu->ROB[iq_entry.rob_tail];
            rob_entry->exception_codes = 0;
            rob_entry->result_valid = 1;
            rob_entry->result = cpu->intfu.result_buffer;
            rob_entry->des_phy_reg = iq_entry.des_phy_reg;
            rob_entry->des_rd = iq_entry.des_rd;
            //end
            break;
        }
        case OPCODE_XOR:
        {
            cpu->intfu.result_buffer = cpu->phys_regs[iq_entry.src1] ^ cpu->phys_regs[iq_entry.src2];
            //start
            ROB_ENTRY *rob_entry = &cpu->ROB[iq_entry.rob_tail];
            rob_entry->exception_codes = 0;
            rob_entry->result_valid = 1;
            rob_entry->result = cpu->intfu.result_buffer;
            rob_entry->des_phy_reg = iq_entry.des_phy_reg;
            rob_entry->des_rd = iq_entry.des_rd;
            //end

            break;
        }
        case OPCODE_CMP:
        {
            cpu->intfu.result_buffer = cpu->phys_regs[iq_entry.src1] - cpu->phys_regs[iq_entry.src2];
            if (cpu->intfu.result_buffer == 0)
            {
                cpu->zero_flag = TRUE;
            }
            else
            {
                cpu->zero_flag = FALSE;
            }
            //start
            ROB_ENTRY *rob_entry = &cpu->ROB[iq_entry.rob_tail];
            rob_entry->exception_codes = 0;
            rob_entry->result_valid = 1;
            rob_entry->result = cpu->intfu.result_buffer;
            rob_entry->des_phy_reg = iq_entry.des_phy_reg;
            rob_entry->des_rd = iq_entry.des_rd;
            //end
            break;
        }
        case OPCODE_MOVC:
        {

            cpu->intfu.result_buffer = iq_entry.imm;

            //start
            ROB_ENTRY *rob_entry = &cpu->ROB[iq_entry.rob_tail];
            rob_entry->exception_codes = 0;
            rob_entry->result_valid = 1;
            rob_entry->result = cpu->intfu.result_buffer;
            rob_entry->des_phy_reg = iq_entry.des_phy_reg;
            rob_entry->des_rd = iq_entry.des_rd;
            //end
            break;
        }
        case OPCODE_HALT:
        {
            /* Stop fetching new instructions if HALT is fetched */

            /* Stop the APEX simulator */
            return TRUE;
        }
        }
        iq_entry.finishedstage = INTFU;
        cpu->intfu.has_insn = FALSE;
        if (ENABLE_DEBUG_MESSAGES)
        {
            printf("Instruction at intfu____________Stage--->");
            printf("\n");

            printf("%-15s: pc(%d) %s", "intfu ", iq_entry.pc, iq_entry.opcode_str);
            printf("\n");
        }
    }
    else if (ENABLE_DEBUG_MESSAGES)
    {
        printf("Instruction at intfu____________Stage---empty>");
        printf("\n");
    }
    return 0;
}

int APEX_mul1(APEX_CPU *cpu)
{
    IQ_ENTRY iq_entry = cpu->mul1.iq_entry;

    if (!cpu->mul1.stalled && iq_entry.finishedstage < MUL1 && cpu->mul1.has_insn)
    {
        if (iq_entry.opcode == OPCODE_MUL)
        {
            cpu->mul1.result_buffer = cpu->phys_regs[iq_entry.src1] * cpu->phys_regs[iq_entry.src2];
        }
        iq_entry.finishedstage = MUL1;

        cpu->mul2 = cpu->mul1;
        cpu->mul1.has_insn = FALSE;
        if (ENABLE_DEBUG_MESSAGES)
        {
            printf("Instruction at mul1____________Stage--->");
            printf("\n");

            printf("%-15s: pc(%d) ", "mul1", iq_entry.pc);
            printf("\n");
        }
    }
    else if (ENABLE_DEBUG_MESSAGES)
    {
        printf("Instruction at mul1____________Stage---empty>");
        printf("\n");
    }
    return 0;
}

int APEX_mul2(APEX_CPU *cpu)
{
    IQ_ENTRY iq_entry = cpu->mul2.iq_entry;

    if (!cpu->mul2.stalled && iq_entry.finishedstage < MUL2 && cpu->mul2.has_insn)
    {
        iq_entry.finishedstage = MUL2;

        cpu->mul3 = cpu->mul2;
        cpu->mul2.has_insn = FALSE;

        if (ENABLE_DEBUG_MESSAGES)
        {
            printf("Instruction at mul2____________Stage--->");
            printf("\n");

            printf("%-15s: pc(%d) ", "mul2", iq_entry.pc);
            printf("\n");
        }
    }
    else if (ENABLE_DEBUG_MESSAGES)
    {
        printf("Instruction at mul2____________Stage---empty>");
        printf("\n");
    }
    return 0;
}

int APEX_mul3(APEX_CPU *cpu)
{

    IQ_ENTRY iq_entry = cpu->mul3.iq_entry;

    if (!cpu->mul3.stalled && iq_entry.finishedstage < MUL3 && cpu->mul3.has_insn)
    {

        //start
        ROB_ENTRY *rob_entry = &cpu->ROB[iq_entry.rob_tail];
        rob_entry->exception_codes = 0;
        rob_entry->result_valid = 1;
        rob_entry->result = cpu->mul3.result_buffer;
        rob_entry->des_phy_reg = iq_entry.des_phy_reg;
        rob_entry->des_rd = iq_entry.des_rd;
        //end
        iq_entry.finishedstage = MUL3;
        cpu->mul3.has_insn = FALSE;
        if (ENABLE_DEBUG_MESSAGES)
        {
            printf("Instruction at mul3____________Stage--->");
            printf("\n");

            printf("%-15s: pc(%d) ", "mul3", iq_entry.pc);
            printf("\n");
        }
    }
    else if (ENABLE_DEBUG_MESSAGES)
    {
        printf("Instruction at mul3____________Stage---empty>");
        printf("\n");
    }
    return 0;
}
int APEX_jbu1(APEX_CPU *cpu)
{
    if (cpu->jbu1.has_insn)
    {
        IQ_ENTRY iq_entry = cpu->jbu1.iq_entry;
        switch (iq_entry.opcode)
        {
        case OPCODE_JAL:
        {
            // cpu->jbu1.result_buffer = iq_entry.src1 + iq_entry.imm;
            cpu->jbu1.result_buffer = cpu->phys_regs[iq_entry.src1] + iq_entry.imm;
            cpu->jbu1.rd = iq_entry.pc + 4;
            cpu->decode.has_insn = FALSE;
            cpu->fetch.has_insn = FALSE;
            for (int i = 0; i < 24; i++)
            {
                cpu->freeiq[i] = 0;
            }
            break;
        }
        case OPCODE_JUMP:
        {
            cpu->jbu1.result_buffer = cpu->phys_regs[iq_entry.src1] + iq_entry.imm;
            cpu->decode.has_insn = FALSE;
            cpu->fetch.has_insn = FALSE;
            for (int i = 0; i < 24; i++)
            {
                cpu->freeiq[i] = 0;
            }
            break;
        }
        case OPCODE_BZ:
        {
            if (cpu->zero_flag == TRUE)
            {
                cpu->jbu1.result_buffer = iq_entry.pc + iq_entry.imm;
                cpu->decode.has_insn = FALSE;
                cpu->fetch.has_insn = FALSE;
                for (int i = 0; i < 24; i++)
                {
                    cpu->freeiq[i] = 0;
                }
            }

            break;
        }
        case OPCODE_BNZ:
        {
            if (cpu->zero_flag == FALSE)
            {
                cpu->jbu1.result_buffer = iq_entry.pc + iq_entry.imm;
                cpu->decode.has_insn = FALSE;
                cpu->fetch.has_insn = FALSE;
                for (int i = 0; i < 24; i++)
                {
                    cpu->freeiq[i] = 0;
                }
            }
            break;
        }
        }
        cpu->jbu2 = cpu->jbu1;
        cpu->jbu1.has_insn = FALSE;

        if (ENABLE_DEBUG_MESSAGES)
        {
            printf("Instruction at jbu1--->");
            printf("\n");

            printf("%-15s: pc(%d) ", "jbu1", iq_entry.pc);
            printf("\n");
        }
    }
    else if (ENABLE_DEBUG_MESSAGES)
    {
        printf("Instruction at jbu1____________Stage---empty>");
        printf("\n");
    }
    return 0;
}
int APEX_jbu2(APEX_CPU *cpu)
{
    if (cpu->jbu2.has_insn)
    {
        IQ_ENTRY iq_entry = cpu->jbu2.iq_entry;
        switch (iq_entry.opcode)
        {
        case OPCODE_BZ:
        {
            //start
            ROB_ENTRY *rob_entry = &cpu->ROB[iq_entry.rob_tail];
            rob_entry->exception_codes = 0;
            rob_entry->result_valid = 1;
            rob_entry->result = cpu->jbu2.result_buffer;
            rob_entry->des_phy_reg = iq_entry.des_phy_reg;
            rob_entry->des_rd = iq_entry.des_rd;
            //end
            if (cpu->zero_flag == TRUE)
            { // DO IN ROB
                //cpu->pc = cpu->jbu2.result_buffer;
                //cpu->is_stalled = 0; // 1 means stalled
                cpu->jbu2.result_buffer = iq_entry.pc + iq_entry.imm;
                cpu->decode.has_insn = FALSE;
                cpu->pc = cpu->jbu2.result_buffer;

                cpu->fetch.has_insn = TRUE;
                for (int i = 0; i < 24; i++)
                {
                    cpu->freeiq[i] = 0;
                }
            }
            break;
        }
        case OPCODE_BNZ:
        {
            //start
            ROB_ENTRY *rob_entry = &cpu->ROB[iq_entry.rob_tail];
            rob_entry->exception_codes = 0;
            rob_entry->result_valid = 1;
            rob_entry->result = cpu->jbu2.result_buffer;
            rob_entry->des_phy_reg = iq_entry.des_phy_reg;
            rob_entry->des_rd = iq_entry.des_rd;
            //end
            if (cpu->zero_flag == FALSE)
            { // DO IN ROB
                //cpu->pc = cpu->jbu2.result_buffer;
                //cpu->is_stalled = 0; // 1 means stalled
                cpu->jbu2.result_buffer = cpu->jbu2.iq_entry.pc + cpu->jbu2.iq_entry.imm;
                cpu->decode.has_insn = FALSE;
                cpu->pc = cpu->jbu2.result_buffer;

                cpu->fetch.has_insn = TRUE;
                for (int i = 0; i < 24; i++)
                {
                    cpu->freeiq[i] = 0;
                }
            }
            break;
        }
        case OPCODE_JAL:
        {
            //start
            ROB_ENTRY *rob_entry = &cpu->ROB[iq_entry.rob_tail];
            cpu->jbu2.result_buffer = cpu->phys_regs[iq_entry.src1] + iq_entry.imm;

            rob_entry->exception_codes = 0;
            rob_entry->result_valid = 1;
            rob_entry->result = cpu->jbu2.result_buffer;
            rob_entry->des_phy_reg = iq_entry.des_phy_reg;
            rob_entry->des_rd = iq_entry.des_rd;

            rob_entry->imm = cpu->jbu2.rd; // one addition to pass commit function

            cpu->decode.has_insn = FALSE;
            cpu->pc = cpu->jbu2.result_buffer;

            cpu->fetch.has_insn = TRUE;
            for (int i = 0; i < 24; i++)
            {
                cpu->freeiq[i] = 0;
            }
            //end

            break;
        }
        case OPCODE_JUMP:
        {

            //start
            ROB_ENTRY *rob_entry = &cpu->ROB[iq_entry.rob_tail];
            rob_entry->exception_codes = 0;
            rob_entry->result_valid = 1;

            cpu->jbu2.result_buffer = cpu->phys_regs[iq_entry.src1] + iq_entry.imm;
            rob_entry->result = cpu->jbu2.result_buffer;
            rob_entry->des_phy_reg = iq_entry.des_phy_reg;
            rob_entry->des_rd = iq_entry.des_rd;

            //end
            //cpu->is_stalled = 0;
            //cpu->pc = cpu->jbu2.result_buffer;
            cpu->pc = cpu->jbu2.result_buffer;

            cpu->decode.has_insn = FALSE;
            cpu->fetch.has_insn = TRUE;
            for (int i = 0; i < 24; i++)
            {
                cpu->freeiq[i] = 0;
            }

            break;
        }
        }
        cpu->jbu2.has_insn = FALSE;

        if (ENABLE_DEBUG_MESSAGES)
        {
            printf("Instruction at jbu2--->");
            printf("\n");

            printf("%-15s: pc(%d) ", "jbu2", iq_entry.pc);
            printf("\n");
        }
    }
    else if (ENABLE_DEBUG_MESSAGES)
    {
        printf("Instruction at jbu2____________Stage---empty>");
        printf("\n");
    }
    return 0;
}
int APEX_instruction_commitment(APEX_CPU *cpu)
{

    ROB_ENTRY *rob_entry = &cpu->ROB[cpu->rob_head];
    ROB_ENTRY *selectedrobentry = rob_entry;
    while (selectedrobentry->result_valid && cpu->rob_tail > cpu->rob_head)
    {
        if (selectedrobentry->result_valid && (selectedrobentry->instruction_type == OPCODE_STR || selectedrobentry->instruction_type == OPCODE_STORE))
        {
            //  cpu->data_memory[cpu->memory2.memory_address]=rob_entry->result;
            cpu->data_memory[rob_entry->des_phy_reg] = rob_entry->result;
            cpu->rob_head = (cpu->rob_head + 1) % 64;
        }
        else if (selectedrobentry->result_valid && (selectedrobentry->instruction_type == OPCODE_ADD || selectedrobentry->instruction_type == OPCODE_LOAD || selectedrobentry->instruction_type == OPCODE_LDR ||
                                                    selectedrobentry->instruction_type == OPCODE_SUB || selectedrobentry->instruction_type == OPCODE_MOVC || selectedrobentry->instruction_type == OPCODE_CMP || selectedrobentry->instruction_type == OPCODE_MUL))
        {

            instruction_retirement_intfu(cpu, selectedrobentry->result, selectedrobentry->des_rd, selectedrobentry->des_phy_reg);
            cpu->rob_head = (cpu->rob_head + 1) % 64;
        }
        else if (selectedrobentry->result_valid && (selectedrobentry->instruction_type == OPCODE_BZ))
        {
            if (cpu->zero_flag == TRUE)
            {
                cpu->rob_head = 0;
                cpu->rob_tail = 0;
                memset(cpu->ROB, 0, sizeof(int) * 64);
            }
            else
            {
                cpu->rob_head = (cpu->rob_head + 1) % 64;
            }
        }
        else if (selectedrobentry->result_valid && (selectedrobentry->instruction_type == OPCODE_BNZ))
        {
            if (cpu->zero_flag == FALSE)
            {
                cpu->rob_head = 0;
                cpu->rob_tail = 0;
                memset(cpu->ROB, 0, sizeof(int) * 64);
            }
            else
            {
                cpu->rob_head = (cpu->rob_head + 1) % 64;
            }
        }
        else if (selectedrobentry->result_valid && selectedrobentry->instruction_type == OPCODE_JAL)
        {
            instruction_retirement_intfu(cpu, selectedrobentry->imm, selectedrobentry->des_rd, selectedrobentry->des_phy_reg);
            cpu->rob_head = 0;
            cpu->rob_tail = 0;
            memset(cpu->ROB, 0, sizeof(int) * 64);
        }
        else if (selectedrobentry->result_valid && selectedrobentry->instruction_type == OPCODE_JUMP)
        {
            cpu->rob_head = 0;
            cpu->rob_tail = 0;
            memset(cpu->ROB, 0, sizeof(int) * 64);

            break;
        }

        if (ENABLE_DEBUG_MESSAGES)
        {
            printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
            printf("Details of ROB Retired Instructions –\n");
            printf("%s----[%d]", selectedrobentry->opcode_str, selectedrobentry->pc);
            printf("\n");
            printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
        }
        if (selectedrobentry->instruction_type == OPCODE_HALT)
        {

            return TRUE;
        }
        selectedrobentry = &cpu->ROB[cpu->rob_head];
    }
    //default
    return 0;
}
/*
 * This function creates and initializes APEX cpu.
 *
 * Note: You are free to edit this function according to your implementation
 */
APEX_CPU *
APEX_cpu_init(const char *filename)
{
    int i;
    APEX_CPU *cpu;

    if (!filename)
    {
        return NULL;
    }

    cpu = calloc(1, sizeof(APEX_CPU));

    if (!cpu)
    {
        return NULL;
    }

    /* Initialize PC, Registers and all pipeline stages */
    cpu->pc = 4000;
    memset(cpu->regs, 0, sizeof(int) * REG_FILE_SIZE);
    memset(cpu->data_memory, 0, sizeof(int) * DATA_MEMORY_SIZE);
    cpu->single_step = ENABLE_SINGLE_STEP;
    cpu->zero_flag = -1;
    /* Parse input file and create code memory */
    cpu->code_memory = create_code_memory(filename, &cpu->code_memory_size);

    memset(cpu->phys_regs, 0, sizeof(int) * 48);
    //invalid contents
    memset(cpu->phys_regs_valid, 0, sizeof(int) * 48);

    //invalid contents
    memset(cpu->rename_table, -1, sizeof(int) * 16);

    memset(cpu->rename_table_valid, 0, sizeof(int) * 16);
    //invalid contents
    memset(cpu->r_rename_table, -1, sizeof(int) * 16);

    memset(cpu->r_rename_table_valid, 0, sizeof(int) * 16);

    if (!cpu->code_memory)
    {
        free(cpu);
        return NULL;
    }

    if (ENABLE_DEBUG_MESSAGES)
    {
        fprintf(stderr,
                "APEX_CPU: Initialized APEX CPU, loaded %d instructions\n",
                cpu->code_memory_size);
        fprintf(stderr, "APEX_CPU: PC initialized to %d\n", cpu->pc);
        fprintf(stderr, "APEX_CPU: Printing Code Memory\n");
        printf("%-9s %-9s %-9s %-9s %-9s\n", "opcode_str", "rd", "rs1", "rs2",
               "imm");

        for (i = 0; i < cpu->code_memory_size; ++i)
        {
            printf("%-9s %-9d %-9d %-9d %-9d\n", cpu->code_memory[i].opcode_str,
                   cpu->code_memory[i].rd, cpu->code_memory[i].rs1,
                   cpu->code_memory[i].rs2, cpu->code_memory[i].imm);
        }
    }

    /* To start fetch stage */
    cpu->fetch.has_insn = TRUE;
    return cpu;
}

/*
 * APEX CPU simulation loop
 *
 * Note: You are free to edit this function according to your implementation
 */
void APEX_cpu_run(APEX_CPU *cpu, const char *fun, const char *steps)
{
    char user_prompt_val;
    //int funct=0; // 0 for simulate 1 for display and single step for 2
    if (strcmp(fun, "simulate") == 0)
    {
        ENABLE_DEBUG_MESSAGES = FALSE;
        funct = 0;
        numOfCycles = atoi(steps);
    }
    else if (strcmp(fun, "display") == 0)
    {
        funct = 1;
        numOfCycles = atoi(steps);
    }
    else if (strcmp(fun, "single_step") == 0)
    {
        funct = 2;
    }

    while (TRUE)
    {
        int breaktrue = 0;
        if (ENABLE_DEBUG_MESSAGES)
        {
            printf("--------------------------------------------\n");
            printf("Clock Cycle #: %d\n", cpu->clock + 1);
            printf("--------------------------------------------\n");
        }

        if (APEX_instruction_commitment(cpu))
        {
            breaktrue = 1;
        }
        APEX_memory2(cpu);
        APEX_memory1(cpu);
        APEX_jbu2(cpu);
        APEX_jbu1(cpu);
        APEX_mul3(cpu);
        APEX_mul2(cpu);
        APEX_mul1(cpu);
        APEX_intfu(cpu);
        APEX_issuequeue(cpu);
        APEX_decode(cpu);
        APEX_fetch(cpu);

        if (breaktrue) {

            for(int i=0;i <16;i++) {
                cpu->rename_table[i]=cpu->r_rename_table[i];
            }
        }
        //  print_reg_file(cpu);
        if (funct != 0)
        {
            print_rob(cpu);
            print_rename_table(cpu);
            print_r_rename_table(cpu);
            //printdatamemory(cpu);
            if (FALSE)
            {
                print_physical_register(cpu);
            }
        }
        if (funct == 2 && cpu->single_step)
        {
            printf("Press any key to advance CPU Clock or <q> to quit:\n");
            scanf("%c", &user_prompt_val);

            if ((user_prompt_val == 'Q') || (user_prompt_val == 'q'))
            {
                printf("APEX_CPU: Simulation Stopped, cycles = %d instructions = %d\n", cpu->clock + 1, cpu->insn_completed);
                break;
            }
        }

        cpu->clock++;
        if (funct != 2 && numOfCycles == cpu->clock)
        {
            if (funct == 0)
            {
                print_rob(cpu);
                print_rename_table(cpu);
                print_r_rename_table(cpu);
            }
            printdatamemory(cpu);
            if (TRUE)
            {
                print_physical_register(cpu);
            }
            break;
        }
        if (breaktrue)
        {


            if (funct != 2)
            {

                if (funct == 0)
                {
                    print_rob(cpu);
                    print_rename_table(cpu);
                    print_r_rename_table(cpu);
                }
                printdatamemory(cpu);
                if (TRUE)
                {
                    print_physical_register(cpu);
                }
            }
            if (funct == 2)
            {
                print_physical_register(cpu);
            }
            break;
        }
    }
}
void instruction_retirement_intfu(APEX_CPU *cpu, int result_buffer, int des_rd, int des_phy_reg)
{

    if (cpu->rename_table[des_rd] == cpu->r_rename_table[des_rd])
    {

        //instruction retriement process
        //iq_entry.des_phy_reg ==freed entry

        cpu->phys_regs[des_phy_reg] = result_buffer;
        //the phy is valid now
        cpu->phys_regs_valid[des_phy_reg] = 1;

        //the r-rat entry is of rd is pointing to most recent phy_reg
        cpu->r_rename_table[des_rd] = des_phy_reg;
        cpu->r_rename_table_valid[des_rd] = 1;
    }
    else
    {
        int previous_rat_index = cpu->r_rename_table[des_rd];
        //the phy is invalid now
        cpu->phys_regs_valid[previous_rat_index] = 0;
        //free physical register
        //mark contents has  free
        cpu->free_PR_list[previous_rat_index] = 0;

        cpu->phys_regs[des_phy_reg] = result_buffer;
        //the phy is valid now
        cpu->phys_regs_valid[des_phy_reg] = 1;
        //rat update content is valid

        //the r-rat entry is of rd is pointing to most recent phy_reg
        cpu->r_rename_table[des_rd] = des_phy_reg;
        cpu->r_rename_table_valid[des_rd] = 1;
    }
}

/*
 * This function deallocates APEX CPU.
 *
 * Note: You are free to edit this function according to your implementation
 */
void APEX_cpu_stop(APEX_CPU *cpu)
{
    free(cpu->code_memory);
    free(cpu);
}
