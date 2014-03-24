/* -*- c-basic-offset: 2; tab-width: 2; indent-tabs-mode: nil; smart-tabs-mode: nil; eval: (c-set-offset 'case-label '+) -*- */

#if __STDC_VERSION__ < 199901L
# warning "This program should be compiled as C99 or better"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <limits.h>

typedef intmax_t integer;
#define PR_INTEGER PRIiMAX

// Extracts an the unsigned value on [low, high)
static inline
uint32_t BITRANGE (uint32_t value,
                   uint_fast8_t low,
                   uint_fast8_t high)
{
  return (value >> low) & ((1 << (high - low)) - 1);
}

// Extends the highest bit on [0, bits)
static inline
uint32_t SIGN_EXTEND (uint32_t value,
                      uint_fast8_t bits)
{
  if (value & (1 << (bits - 1))) // Highest bit 1
    return value | (-1 << bits);
  else
    return value;
}



enum {
  MEMSIZE = 1048576
};

static uint32_t icount, *instruction;
static uint32_t mem [MEMSIZE / 4];



static uint32_t Convert(uint32_t x)
{
  return (x >> 24) | ((x >> 8) & 0xff00) | ((x << 8) & 0xff0000) | (x << 24);
}

static uint32_t Fetch(uint32_t pc)
{
  pc = (pc - 0x00400000) >> 2;
  if (pc >= icount) {
    fprintf(stderr, "instruction fetch out of range\n");
    exit(-1);
  }
  return instruction[pc];
}

static uint32_t LoadWord(uint32_t addr)
{
  if ((addr & 3) != 0) {
    fprintf(stderr, "unaligned data access\n");
    exit(-1);
  }
  addr -= 0x10000000;
  if (addr >= MEMSIZE) {
    fprintf(stderr, "data access out of range\n");
    exit(-1);
  }
  return mem[addr / 4];
}

static void StoreWord(uint32_t data, uint32_t addr)
{
  if ((addr & 3) != 0) {
    fprintf(stderr, "unaligned data access\n");
    exit(-1);
  }
  addr -= 0x10000000;
  if (addr >= MEMSIZE) {
    fprintf(stderr, "data access out of range\n");
    exit(-1);
  }
  mem[addr / 4] = data;
}



enum opcode {
  FUNCTION = 0x00,
  J        = 0x02,
  JAL      = 0x03,
  BEQ      = 0x04,
  BNE      = 0x05,
  ADDIU    = 0x09,
  ANDI     = 0x0C,
  LUI      = 0x0F,
  TRAP     = 0x1A,
  LW       = 0x23,
  SW       = 0x2B
};

enum funcode {
  SLL  = 0x00,
  SRA  = 0x03,
  JR   = 0x08,
  MFHI = 0x10,
  MFLO = 0x12,
  MULT = 0x18,
  DIV  = 0x1A,
  ADDU = 0x21,
  SUBU = 0x23,
  SLT  = 0x2a
};

enum trapcode {
  NEWLINE = 0x00,
  PRINT   = 0x01,
  PROMPT  = 0x05,
  STOP    = 0x0a
};

enum regid {
  ZERO = 0,
  AT   = 1,
  V0   = 2,
  V1   = 3,
  A0   = 4,
  A1   = 5,
  A2   = 6,
  A3   = 7,
  T0   = 8,
  T1   = 9,
  T2   = 10,
  T3   = 11,
  T4   = 12,
  T5   = 13,
  T6   = 14,
  T7   = 15,
  S0   = 16,
  S1   = 17,
  S2   = 18,
  S3   = 19,
  S4   = 20,
  S5   = 21,
  S6   = 22,
  S7   = 23,
  T8   = 24,
  T9   = 25,
  K0   = 26,
  K1   = 27,
  GP   = 28,
  SP   = 29,
  FP   = 30,
  RA   = 31,
  LO   = 32,
  HI   = 33,
  REGS = 1 + HI - ZERO
};

enum pipestage {
  IF1    = 0,
  IF2    = 1,
  ID     = 2,
  EXE1   = 3,
  EXE2   = 4,
  MEM1   = 5,
  MEM2   = 6,
  MEM3   = 7,
  WB     = 8,
  STAGES = 1 + WB - IF1
};

static void Interpret (uint32_t start)
{
  // Registers
  uint32_t pc = start;
  uint32_t reg [REGS] = {
    [GP] = 0x10008000,
    [SP] = 0x10000000 + MEMSIZE
  };

  // Control data
  integer count = 0;

  #define RREAD(REG)
  #define RWRITE(REG)

  // Main loop
  while (1) {
    uint32_t instr  = Fetch(pc);
    reg [ZERO] = 0; // $zero
    pc += 4;
    ++count;

    uint8_t  opcode = BITRANGE (instr, 26, 32);
    uint8_t  rs     = BITRANGE (instr, 21, 26);
    uint8_t  rt     = BITRANGE (instr, 16, 21);
    uint8_t  rd     = BITRANGE (instr, 11, 16);
    uint8_t  shamt  = BITRANGE (instr, 6, 11);
    uint8_t  funct  = BITRANGE (instr, 0, 6);
    uint16_t uimm   = BITRANGE (instr, 0, 16);
    int16_t  simm   = SIGN_EXTEND (uimm, 16);
    uint32_t addr   = BITRANGE (instr, 0, 26);
    uint32_t jaddr  = (pc & 0xf0000000) + addr * 4;
    uint32_t baddr  = pc + simm * 4;

    switch (opcode)
    {
      case FUNCTION:
        switch (funct)
        {
          case SLL:
            RREAD (rs);
            reg [rd] = reg [rs] << shamt;
            RWRITE (rd);
            break;

          case SRA:
            RREAD (rs);
            reg [rd] = SIGN_EXTEND (reg [rs] >> shamt, 32 - shamt);
            RWRITE (rd);
            break;

          case JR:
            RREAD (rs);
            pc = reg [rs];
            break;

          case MFHI:
            RREAD (HI);
            reg [rd] = reg [HI];
            RWRITE (rd);
            break;

          case MFLO:
            RREAD (LO);
            reg [rd] = reg [LO];
            RWRITE (rd);
            break;

          case MULT:
            RREAD (rs);
            RREAD (rt);
            uint64_t wide = reg [rs] * reg [rt];
            reg [LO] = wide & 0xffffffff;
            reg [HI] = wide >> 32;
            RWRITE (LO);
            RWRITE (HI);
            break;

          case DIV:
            RREAD (rs);
            RREAD (rt);
            if (reg [rt] == 0) {
              fprintf (stderr, "division by zero: pc = 0x%"PRIx32"\n", pc - 4);
              goto halt;
            }
            else {
              reg [LO] = reg [rs] / reg [rt];
              reg [HI] = reg [rs] % reg [rt];
            }
            RWRITE (LO);
            RWRITE (HI);
            break;

          case ADDU:
            RREAD (rs);
            RREAD (rt);
            reg [rd] = reg [rs] + reg [rt];
            RWRITE (rd);
            break;

          case SUBU:
            RREAD (rs);
            RREAD (rt);
            reg [rd] = reg [rs] - reg [rt];
            RWRITE (rd);
            break;

          case SLT:
            RREAD (rs);
            RREAD (rt);
            reg [rd] = ((int32_t) reg [rs] < (int32_t) reg [rt] ? 1 : 0);
            RWRITE (rd);
            break;

          default:
            fprintf (stderr, "unimplemented instruction: pc = 0x%"PRIx32"\n", pc - 4);
            goto halt;
        }
        break;

      case J:
        pc = jaddr;
        break;

      case JAL:
        reg [31] = pc;
        pc = jaddr;
        RWRITE (31);
        break;

      case BEQ:
        RREAD (rs);
        RREAD (rt);
        if (reg [rs] == reg [rt])
          pc = baddr;
        break;

      case BNE:
        RREAD (rs);
        RREAD (rt);
        if (reg [rs] != reg [rt])
          pc = baddr;
        break;

      case ADDIU:
        RREAD (rs);
        reg [rt] = reg [rs] + simm;
        RWRITE (rt);
        break;

      case ANDI:
        RREAD (rs);
        reg [rt] = reg [rs] & uimm;
        RWRITE (rt);
        break;

      case LUI:
        reg [rt] = uimm << 16;
        RWRITE (rt);
        break;

      case TRAP:
        switch (addr & 0xf)
        {
          case NEWLINE:
            printf ("\n");
            break;

          case PRINT:
            RREAD (rs);
            printf (" %d", reg [rs]);
            break;

          case PROMPT:
            printf ("\n? ");
            fflush (stdout);
            int32_t input;
            scanf ("%"PRIi32, &input);
            reg [rt] = input;
            RWRITE (rt);
            break;

          case STOP:
            goto halt;

          default:
            fprintf (stderr, "unimplemented trap: pc = 0x%"PRIx32"\n", pc - 4);
            goto halt;
        }
        break;

      case LW:
        RREAD (rs);
        reg [rt] = LoadWord (reg [rs] + simm);
        RWRITE (rt);
        break;

      case SW:
        RREAD (rs);
        RREAD (rt);
        StoreWord (reg [rt], reg [rs] + simm);
        break;

      default:
        fprintf (stderr, "unimplemented instruction: pc = 0x%"PRIx32"\n", pc - 4);
        goto halt;
    }
  }

halt:
  ;
}



int main(int argc, char *argv[])
{
  uint32_t c, start;
  int little_endian;
  FILE *f;

  printf("CS3339 MIPS Interpreter\n");
  if (argc != 2) {fprintf(stderr, "usage: %s executable\n", argv[0]); exit(-1);}
  if (sizeof(int) != 4) {fprintf(stderr, "error: need 4-byte integers\n"); exit(-1);}
  if (sizeof(long long) != 8) {fprintf(stderr, "error: need 8-byte long longs\n"); exit(-1);}

  c = 1;
  little_endian = *((char *)&c);
  f = fopen(argv[1], "r+b");
  if (f == NULL) {fprintf(stderr, "error: could not open file %s\n", argv[1]); exit(-1);}
  c = fread(&icount, 4, 1, f);
  if (c != 1) {fprintf(stderr, "error: could not read count from file %s\n", argv[1]); exit(-1);}
  if (little_endian) {
    icount = Convert(icount);
  }
  c = fread(&start, 4, 1, f);
  if (c != 1) {fprintf(stderr, "error: could not read start from file %s\n", argv[1]); exit(-1);}
  if (little_endian) {
    start = Convert(start);
  }

  instruction = (uint32_t *)(malloc(icount * 4));
  if (instruction == NULL) {fprintf(stderr, "error: out of memory\n"); exit(-1);}
  c = fread(instruction, 4, icount, f);
  if (c != icount) {fprintf(stderr, "error: could not read (all) instructions from file %s\n", argv[1]); exit(-1);}
  fclose(f);
  if (little_endian) {
    for (c = 0; c < icount; c++) {
      instruction[c] = Convert(instruction[c]);
    }
  }

  printf("running %s\n\n", argv[1]);
  Interpret(start);

  free (instruction);
  return 0;
}
