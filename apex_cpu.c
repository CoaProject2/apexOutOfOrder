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
    printf("%-15s: pc(%d) ", name, stage->pc);
    print_instruction(stage);
    printf("\n");
}

/* Debug function which prints the register file
 *
 * Note: You are not supposed to edit this function
 */
static void
print_reg_file(const APEX_CPU *cpu)
{
    int i;
    printf("----------\n%s\n----------\n", "Registers:");

    for (int i = 0; i < REG_FILE_SIZE / 2; ++i)
    {
        printf("R%-3d[%-3d] ", i, cpu->regs[i]);
    }
    printf("\n");
    for (i = (REG_FILE_SIZE / 2); i < REG_FILE_SIZE; ++i)
    {
        printf("R%-3d[%-3d] ", i, cpu->regs[i]);
    }
    printf("\n");
}

/*
 * Fetch Stage of APEX Pipeline
 *
 * Note: You are free to edit this function according to your implementation
 */
static void
APEX_fetch(APEX_CPU *cpu)
{
    APEX_Instruction *current_ins;
    if (cpu->fetch.has_insn)
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
            if (cpu->fetch.opcode == OPCODE_HALT)
            {
                cpu->fetch.has_insn = FALSE;
            }

            if (ENABLE_DEBUG_MESSAGES)
            {
                print_stage_content("Instruction at Fetch____________Stage--->", &cpu->fetch);
                printf("\n");
            }
        }
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

            int first_free_phy_reg = -1;

            int rs1_physical = cpu->decode.rs1 > -1 ? cpu->rename_table[cpu->decode.rs1] : -1;
            int rs2_physical = cpu->decode.rs2 > -1 ? cpu->rename_table[cpu->decode.rs2] : -1;

            //  printf("rs1_physical= %d rs2_physical %d", rs1_physical, rs2_physical);

            for (int i = 0; i < 48; i++)
            {
                if (cpu->free_PR_list[i] == 0)
                {
                    first_free_phy_reg = i;
                    cpu->free_PR_list[i] = 1;
                    cpu->phys_regs_valid[i] = 1;
                    break;
                }
            }

            if (first_free_phy_reg > -1)
            {
                cpu->rename_table[cpu->decode.rd] = first_free_phy_reg;
                //add a rename table entry made rename table contents as invalid
                cpu->rename_table_valid[cpu->decode.rd] = 0;
                cpu->phys_regs_valid[cpu->rename_table[cpu->decode.rd]] = 0;
            }
            if (!(first_free_phy_reg > -1))
            {
                stagestalled = 1;
            }

            for (int i = 0; i < 24; i++)
            {
                /*iq means iq entry occupied*/
                if (cpu->freeiq[i] >= 1)
                {
                    break;
                }
                if (i == 24)
                {
                    stagestalled = 1;
                }
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
                strcpy(iq_entry->opcode_str , cpu->decode.opcode_str);
                iq_entry->src1 = rs1_physical; 
                //cpu->phys_regs[rs1_physical]
                iq_entry->src2 = rs2_physical;
                //cpu->phys_regs[rs2_physical]
                // iq_entry->src2_tag = rs2_physical;
                //  iq_entry->src1_tag = rs1_physical;
                // iq_entry->src2_ready = cpu->phys_regs_valid[rs1_physical];
                // iq_entry->src1_ready = cpu->phys_regs_valid[rs2_physical];
                iq_entry->imm = cpu->decode.imm;
                iq_entry->pc = cpu->decode.pc;
                iq_entry->des_phy_reg = first_free_phy_reg;
                iq_entry->des_rd = cpu->decode.rd;
                switch (cpu->decode.opcode)
                {

                case OPCODE_ADD:
                case OPCODE_MOVC:
                {

                    /*3 for ifu*/
                    iq_entry->fu_type = 3;
                    break;
                }
                case OPCODE_MUL:
                {
                    iq_entry->fu_type = 4;
                    break;
                }
                case OPCODE_JUMP:
                case OPCODE_JAL:
                {
                    iq_entry->fu_type = 5;
                    break;
                }
                }
            }
        }
        if (ENABLE_DEBUG_MESSAGES)
        {
            print_stage_content("Instruction at decode____________Stage--->", &cpu->decode);
        }
        // printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
        // printf("Details of RENAME TABLE State --\n");
        // for (int i = 0; i < 16; i++)
        // {
        //     if (cpu->rename_table[i] != -1)
        //     {
        //         printf("R[%d] -> P[%d]\n", i, cpu->rename_table[i]);
        //     }
        // }
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
    // int selectedinstpc;

    int intfuissued = -1;
    int mulfuissued = -1;
    int branchfuissued = -1;

    IQ_ENTRY selectedintfuiqentry;
    IQ_ENTRY selectedmulfuiqentry;
    IQ_ENTRY selectedbranchfuiqentry;

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
        {
            if (cpu->phys_regs_valid[iqe.src1] == 1 && cpu->phys_regs_valid[iqe.src2] == 1)
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
                //   selectedinstpc = iqe.pc;
                mulfuissued = issuequequeindex;
                break;
            }
        }
        case OPCODE_JUMP:
        case OPCODE_JAL:
        {
            if (iqe.src1_ready == 1)
            {
                selectedbranchfuiqentry = iqe;
                //   selectedinstpc = iqe.pc;
                branchfuissued = issuequequeindex;
                break;
            }
        }
        }
    }

    if (ENABLE_DEBUG_MESSAGES)
    {
        printf("Instruction at issuequeue____________Stage--->");

        IQ_ENTRY *iq_entry1;
        for (int i = 0; i < 24; i++)
        {
            if (cpu->freeiq[i] == 1)
            {
                printf("IQ[0%d] --> ", i);
                iq_entry1 = &cpu->IssueQueue[i];

                printf("%-15s: pc(%d) %s", "Issuequeue ", iq_entry1->pc,iq_entry1->opcode_str);
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
}

static void
APEX_intfu(APEX_CPU *cpu)
{
    IQ_ENTRY iq_entry = cpu->intfu.iq_entry;

    if (!cpu->intfu.stalled && iq_entry.finishedstage < INTFU && cpu->intfu.has_insn)
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
            if (cpu->r_rename_table[iq_entry.des_rd] == 0)
            {

                //instruction retriement process
                //iq_entry.des_phy_reg ==freed entry

                cpu->phys_regs[iq_entry.des_phy_reg] = cpu->intfu.result_buffer;
                //the phy is valid now
                cpu->phys_regs_valid[iq_entry.des_phy_reg] = 1;

                //rat update content is valid
                cpu->rename_table_valid[iq_entry.des_rd] = 1;

                //the r-rat entry is of rd is pointing to most recent phy_reg
                cpu->r_rename_table[iq_entry.des_rd] = iq_entry.des_phy_reg;
                cpu->r_rename_table_valid[iq_entry.des_rd] = 1;
            }
            else
            {
                int previous_rat_index = cpu->r_rename_table[iq_entry.des_rd];
                //the phy is invalid now
                cpu->phys_regs_valid[previous_rat_index] = 0;
                //free physical register
                //mark contents has  free
                cpu->free_PR_list[previous_rat_index] = 0;

                cpu->phys_regs[iq_entry.des_phy_reg] = cpu->intfu.result_buffer;
                //the phy is valid now
                cpu->phys_regs_valid[iq_entry.des_phy_reg] = 1;
                //rat update content is valid
                cpu->rename_table_valid[iq_entry.des_rd] = 1;
                //the r-rat entry is of rd is pointing to most recent phy_reg
                cpu->r_rename_table[iq_entry.des_rd] = iq_entry.des_phy_reg;
                cpu->r_rename_table_valid[iq_entry.des_rd] = 1;
            }
            break;
        }
        case OPCODE_SUB:
        {
            if (iq_entry.src1_ready == 1 && iq_entry.src2_ready == 1)
            {
                cpu->intfu.result_buffer = iq_entry.src1 - iq_entry.src2;
                //instruction retriement process
                cpu->phys_regs[iq_entry.des_phy_reg] = cpu->intfu.result_buffer;
                //freethelist
                cpu->free_PR_list[iq_entry.des_phy_reg] = 0;

                printf("Add %d    %d iq_entry.des_phy_reg]", cpu->phys_regs[iq_entry.des_phy_reg], iq_entry.des_phy_reg);

                if (cpu->intfu.result_buffer == 0)
                {
                    cpu->zero_flag = TRUE;
                }
                else
                {
                    cpu->zero_flag = FALSE;
                }
            }

            break;
        }
        case OPCODE_ADDL:
        {
            if (iq_entry.src1_ready == 1)
            {
                cpu->intfu.result_buffer = iq_entry.src1 + iq_entry.imm;
                //instruction retriement process
                cpu->phys_regs[iq_entry.des_phy_reg] = cpu->intfu.result_buffer;
                //freethelist
                cpu->free_PR_list[iq_entry.des_phy_reg] = 0;

                printf("Add %d    %d iq_entry.des_phy_reg]", cpu->phys_regs[iq_entry.des_phy_reg], iq_entry.des_phy_reg);

                if (cpu->intfu.result_buffer == 0)
                {
                    cpu->zero_flag = TRUE;
                }
                else
                {
                    cpu->zero_flag = FALSE;
                }
            }

            break;
        }

        case OPCODE_SUBL:
        {
            if (iq_entry.src1_ready == 1)
            {
                cpu->intfu.result_buffer = iq_entry.src1 - iq_entry.imm;
                //instruction retriement process
                cpu->phys_regs[iq_entry.des_phy_reg] = cpu->intfu.result_buffer;
                //freethelist
                cpu->free_PR_list[iq_entry.des_phy_reg] = 0;

                printf("Add %d    %d iq_entry.des_phy_reg]", cpu->phys_regs[iq_entry.des_phy_reg], iq_entry.des_phy_reg);

                if (cpu->intfu.result_buffer == 0)
                {
                    cpu->zero_flag = TRUE;
                }
                else
                {
                    cpu->zero_flag = FALSE;
                }
            }

            break;
        }
        case OPCODE_AND:
        {
            if (iq_entry.src1_ready == 1 && iq_entry.src2_ready == 1)
            {
                cpu->intfu.result_buffer = iq_entry.src1 & iq_entry.src2;
                //instruction retriement process
                cpu->phys_regs[iq_entry.des_phy_reg] = cpu->intfu.result_buffer;
                //freethelist
                cpu->free_PR_list[iq_entry.des_phy_reg] = 0;

                printf("Add %d    %d iq_entry.des_phy_reg]", cpu->phys_regs[iq_entry.des_phy_reg], iq_entry.des_phy_reg);

                if (cpu->intfu.result_buffer == 0)
                {
                    cpu->zero_flag = TRUE;
                }
                else
                {
                    cpu->zero_flag = FALSE;
                }
            }

            break;
        }
        case OPCODE_OR:
        {
            if (iq_entry.src1_ready == 1 && iq_entry.src2_ready == 1)
            {
                cpu->intfu.result_buffer = iq_entry.src1 | iq_entry.src2;
                //instruction retriement process
                cpu->phys_regs[iq_entry.des_phy_reg] = cpu->intfu.result_buffer;
                //freethelist
                cpu->free_PR_list[iq_entry.des_phy_reg] = 0;

                printf("Add %d    %d iq_entry.des_phy_reg]", cpu->phys_regs[iq_entry.des_phy_reg], iq_entry.des_phy_reg);

                if (cpu->intfu.result_buffer == 0)
                {
                    cpu->zero_flag = TRUE;
                }
                else
                {
                    cpu->zero_flag = FALSE;
                }
            }

            break;
        }
        case OPCODE_XOR:
        {
            if (iq_entry.src1_ready == 1 && iq_entry.src2_ready == 1)
            {
                cpu->intfu.result_buffer = iq_entry.src1 ^ iq_entry.src2;
                //instruction retriement process
                cpu->phys_regs[iq_entry.des_phy_reg] = cpu->intfu.result_buffer;
                //freethelist
                cpu->free_PR_list[iq_entry.des_phy_reg] = 0;

                printf("Add %d    %d iq_entry.des_phy_reg]", cpu->phys_regs[iq_entry.des_phy_reg], iq_entry.des_phy_reg);

                if (cpu->intfu.result_buffer == 0)
                {
                    cpu->zero_flag = TRUE;
                }
                else
                {
                    cpu->zero_flag = FALSE;
                }
            }

            break;
        }
        case OPCODE_CMP:
        {
            if (iq_entry.src1_ready == 1 && iq_entry.src2_ready == 1)
            {
                cpu->intfu.result_buffer = iq_entry.src1 - iq_entry.src2;

                if (cpu->intfu.result_buffer == 0)
                {
                    cpu->zero_flag = TRUE;
                }
                else
                {
                    cpu->zero_flag = FALSE;
                }
            }

            break;
        }
        case OPCODE_MOVC:
        {
            cpu->intfu.result_buffer = iq_entry.imm;
            if (cpu->r_rename_table[iq_entry.des_rd] == 0 || cpu->r_rename_table[iq_entry.des_rd] == -1)
            {

                //instruction retriement process
                //iq_entry.des_phy_reg ==freed entry

                cpu->phys_regs[iq_entry.des_phy_reg] = cpu->intfu.result_buffer;
                //the phy is valid now
                cpu->phys_regs_valid[iq_entry.des_phy_reg] = 1;

                //rat update content is valid
                cpu->rename_table_valid[iq_entry.des_rd] = 1;

                //the r-rat entry is of rd is pointing to most recent phy_reg
                cpu->r_rename_table[iq_entry.des_rd] = iq_entry.des_phy_reg;
                cpu->r_rename_table_valid[iq_entry.des_rd] = 1;
            }
            else
            {
                int previous_rat_index = cpu->r_rename_table[iq_entry.des_rd];
                //the phy is invalid now
                cpu->phys_regs_valid[previous_rat_index] = 0;
                //free physical register
                //mark contents has  free
                cpu->free_PR_list[previous_rat_index] = 0;

                cpu->phys_regs[iq_entry.des_phy_reg] = cpu->intfu.result_buffer;
                //the phy is valid now
                cpu->phys_regs_valid[iq_entry.des_phy_reg] = 1;
                //rat update content is valid
                cpu->rename_table_valid[iq_entry.des_rd] = 1;
                //the r-rat entry is of rd is pointing to most recent phy_reg
                cpu->r_rename_table[iq_entry.des_rd] = iq_entry.des_phy_reg;
                cpu->r_rename_table_valid[iq_entry.des_rd] = 1;
            }
            break;
        }
        }
        iq_entry.finishedstage = INTFU;
        cpu->intfu.has_insn = FALSE;
        if (ENABLE_DEBUG_MESSAGES)
        {
            printf("Instruction at intfu____________Stage--->");
            printf("\n");

            printf("%-15s: pc(%d) %s", "intfu ", iq_entry.pc,iq_entry.opcode_str);
            printf("\n");
        }
    }
}

int APEX_mul1(APEX_CPU *cpu)
{
    IQ_ENTRY iq_entry = cpu->mul1.iq_entry;

    if (!cpu->mul1.stalled && iq_entry.finishedstage < MUL1 && cpu->mul1.has_insn)
    {
        if (iq_entry.opcode == OPCODE_MUL)
        {
            // cpu->mul1.result_buffer = iq_entry.src1 * iq_entry.src2;
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
    return 0;
}

int APEX_mul3(APEX_CPU *cpu)
{
    IQ_ENTRY iq_entry = cpu->mul3.iq_entry;

    if (!cpu->mul3.stalled && iq_entry.finishedstage < MUL3 && cpu->mul3.has_insn)
    {
        if (cpu->r_rename_table[iq_entry.des_rd] == 0 || cpu->r_rename_table[iq_entry.des_rd] == -1)
        {

            //instruction retriement process
            //iq_entry.des_phy_reg ==freed entry

            cpu->phys_regs[iq_entry.des_phy_reg] = cpu->mul3.result_buffer;
            //the phy is valid now
            cpu->phys_regs_valid[iq_entry.des_phy_reg] = 1;

            //rat update content is valid
            cpu->rename_table_valid[iq_entry.des_rd] = 1;

            //the r-rat entry is of rd is pointing to most recent phy_reg
            cpu->r_rename_table[iq_entry.des_rd] = iq_entry.des_phy_reg;
            cpu->r_rename_table_valid[iq_entry.des_rd] = 1;
        }
        else
        {
            int previous_rat_index = cpu->r_rename_table[iq_entry.des_rd];
            //the phy is invalid now
            cpu->phys_regs_valid[previous_rat_index] = 0;
            //free physical register
            //mark contents has  free
            cpu->free_PR_list[previous_rat_index] = 0;

            cpu->phys_regs[iq_entry.des_phy_reg] = cpu->mul3.result_buffer;
            //the phy is valid now
            cpu->phys_regs_valid[iq_entry.des_phy_reg] = 1;
            //rat update content is valid
            cpu->rename_table_valid[iq_entry.des_rd] = 1;
            //the r-rat entry is of rd is pointing to most recent phy_reg
            cpu->r_rename_table[iq_entry.des_rd] = iq_entry.des_phy_reg;
            cpu->r_rename_table_valid[iq_entry.des_rd] = 1;
        }

        iq_entry.finishedstage = MUL3;
        cpu->mul3.has_insn = FALSE;
        if (ENABLE_DEBUG_MESSAGES)
        {
            printf("Instruction at mul3____________Stage--->");
            //   print_stage_content("Instruction at mul3____________Stage--->", &cpu->mul3);
            printf("\n");

            printf("%-15s: pc(%d) ", "mul3", iq_entry.pc);
            printf("\n");
        }
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
            cpu->jbu1.result_buffer = iq_entry.src1 + iq_entry.imm;
            cpu->jbu1.rd = iq_entry.pc + 4;
        }
        case OPCODE_JUMP:
        {
            cpu->jbu1.result_buffer = iq_entry.src1 + iq_entry.imm;
        }
        }
        cpu->jbu2 = cpu->jbu1;
        cpu->jbu1.has_insn = FALSE;

        if (ENABLE_DEBUG_MESSAGES)
        {
            printf("Instruction at jbu1--->");
            printf("\n");

            printf("%-15s: pc(%d) ", "jbu1", iq_entry.pc);
            //  print_stage_content("Instruction at jbu1____________Stage--->", &cpu->jbu1);
            printf("\n");
        }
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
        case OPCODE_JAL:
        {
        }
        case OPCODE_JUMP:
        {
        }
        }
        cpu->jbu2.has_insn = FALSE;

        if (ENABLE_DEBUG_MESSAGES)
        {
            printf("Instruction at jbu2--->");
            printf("\n");

            printf("%-15s: pc(%d) ", "jbu2", iq_entry.pc);
            //  print_stage_content("Instruction at jbu2____________Stage--->", &cpu->jbu2);
            printf("\n");
        }
    }
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
void APEX_cpu_run(APEX_CPU *cpu)
{
    char user_prompt_val;

    while (TRUE)
    {
        if (ENABLE_DEBUG_MESSAGES)
        {
            printf("--------------------------------------------\n");
            printf("Clock Cycle #: %d\n", cpu->clock);
            printf("--------------------------------------------\n");
        }
        APEX_jbu2(cpu);
        APEX_jbu1(cpu);
        APEX_mul3(cpu);
        APEX_mul2(cpu);
        APEX_mul1(cpu);
        APEX_intfu(cpu);
        APEX_issuequeue(cpu);
        APEX_decode(cpu);
        APEX_fetch(cpu);

        //  print_reg_file(cpu);
        printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
        printf("Details of Physical register State --\n");
        for (int i = 0; i < 48; i++)
        {
            ///  if (cpu->free_PR_list[i] == 1)
            // {
            printf(" P[%d]  ==== [%d] \n  ", i, cpu->phys_regs[i]);
            // }
        }
        if (cpu->single_step)
        {
            printf("Press any key to advance CPU Clock or <q> to quit:\n");
            scanf("%c", &user_prompt_val);

            if ((user_prompt_val == 'Q') || (user_prompt_val == 'q'))
            {
                printf("APEX_CPU: Simulation Stopped, cycles = %d instructions = %d\n", cpu->clock, cpu->insn_completed);
                break;
            }
        }

        cpu->clock++;
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