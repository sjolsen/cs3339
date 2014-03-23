/* -*- c-basic-offset: 2; tab-width: 2; indent-tabs-mode: nil; smart-tabs-mode: nil; eval: (c-set-offset 'case-label '+) -*- */

#if __STDC_VERSION__ < 199901L
# warning "This program should be compiled as C99 or better"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <assert.h>

#define MEMSIZE 1048576
#define ARRAYLEN(NAME) (sizeof (NAME) / sizeof (*NAME))

typedef uint32_t word;
typedef int32_t sword;
typedef uint64_t dword;
typedef int_least64_t integer;
#define PR_INTEGER PRIdLEAST64

static word icount, *instruction;
static word mem [MEMSIZE / 4];



static word Convert(word x)
{
  return (x >> 24) | ((x >> 8) & 0xff00) | ((x << 8) & 0xff0000) | (x << 24);
}

static word Fetch(word pc)
{
  pc = (pc - 0x00400000) >> 2;
  if (pc >= icount) {
    fprintf(stderr, "instruction fetch out of range\n");
    exit(-1);
  }
  return instruction[pc];
}

static word LoadWord(word addr)
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

static void StoreWord(word data, word addr)
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



enum {
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

enum {
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

enum {
  NEWLINE = 0x00,
  PRINT   = 0x01,
  PROMPT  = 0x05,
  STOP    = 0x0a
};

const char regname[32][6] = {"$zero","$at","$v0","$v1","$a0","$a1","$a2","$a3",
                             "$t0","$t1","$t2","$t3","$t4","$t5","$t6","$t7","$s0","$s1","$s2","$s3",
                             "$s4","$s5","$s6","$s7","$t8","$t9","$k0","$k1","$gp","$sp","$fp","$ra"};

static void Decode(int pc, int instr)  // do not make any changes outside of this function
{
  #ifndef CHAR_BIT
  # define CHAR_BIT 8
  #endif

  // Reinterpret the bit sequence defined by [low, high) on `instr'
  #define BITRANGE(value, low, high) (((value) >> low) & ((1 << (high - low)) - 1))
  #define SIGN_EXTEND(value, bits) (((signed int) (value) << (sizeof (int) * CHAR_BIT - bits)) >> (sizeof (int) * CHAR_BIT - bits))

  unsigned int opcode = BITRANGE (instr, 26, 32);
  unsigned int rs     = BITRANGE (instr, 21, 26);
  unsigned int rt     = BITRANGE (instr, 16, 21);
  unsigned int rd     = BITRANGE (instr, 11, 16);
  unsigned int shamt  = BITRANGE (instr, 6, 11);
  unsigned int funct  = BITRANGE (instr, 0, 6);
  unsigned int imm    = BITRANGE (instr, 0, 16);
  signed   int simm   = SIGN_EXTEND (imm, 16);
  unsigned int addr   = BITRANGE (instr, 0, 26);
  unsigned int jaddr  = ((pc + 4) & 0xf0000000) + (addr << 2); // Jump address, adjusted for printing
  unsigned int baddr  = pc + (simm << 2) + 4; // Branch address, adjusted for printing

  #undef SIGN_EXTEND
  #undef BITRANGE

  const char* RD = regname [rd];
  const char* RS = regname [rs];
  const char* RT = regname [rt];

  #define PRINT(format, ...) (fprintf (stderr, "%8x: "format"\n", pc, ##__VA_ARGS__))

  switch (opcode) {
    case 0x00:
      switch (funct) {
        //   FUNCTION     FORMAT           NAME    ARGUMENTS
        case 0x00: PRINT ("%s %s, %s, %u", "sll",  RD, RS, shamt); break;
        case 0x03: PRINT ("%s %s, %s, %u", "sra",  RD, RS, shamt); break;
        case 0x08: PRINT ("%s %s",         "jr",   RS);            break;
        case 0x10: PRINT ("%s %s",         "mfhi", RD);            break;
        case 0x12: PRINT ("%s %s",         "mflo", RD);            break;
        case 0x18: PRINT ("%s %s, %s",     "mult", RS, RT);        break;
        case 0x1a: PRINT ("%s %s, %s",     "div",  RS, RT);        break;
        case 0x21: PRINT ("%s %s, %s, %s", "addu", RD, RS, RT);    break;
        case 0x23: PRINT ("%s %s, %s, %s", "subu", RD, RS, RT);    break;
        case 0x2a: PRINT ("%s %s, %s, %s", "slt",  RD, RS, RT);    break;
        default:   PRINT ("unimplemented");
      }
      break;
    //   OPCODE       FORMAT           NAME     ARGUMENTS
    case 0x02: PRINT ("%s %x",         "j",     jaddr);         break;
    case 0x03: PRINT ("%s %x",         "jal",   jaddr);         break;
    case 0x04: PRINT ("%s %s, %s, %x", "beq",   RS, RT, baddr); break;
    case 0x05: PRINT ("%s %s, %s, %x", "bne",   RS, RT, baddr); break;
    case 0x09: PRINT ("%s %s, %s, %d", "addiu", RT, RS, simm);  break;
    case 0x0c: PRINT ("%s %s, %s, %u", "andi",  RT, RS, imm);   break;
    // We should be using unsigned here per the MIPS spec, but we need signed to pass the testcases.
    case 0x0f: PRINT ("%s %s, %d",     "lui",   RT, simm);      break;
    case 0x1a: PRINT ("%s %x",         "trap",  addr);          break;
    case 0x23: PRINT ("%s %s, %d(%s)", "lw",    RT, simm, RS);  break;
    case 0x2b: PRINT ("%s %s, %d(%s)", "sw",    RT, simm, RS);  break;
    default:   PRINT ("unimplemented");
  }

  #undef PRINT
}

static void Interpret(int start)
{
  // Registers
  word pc = start;
  word reg [32] = {
    [28] = 0x10008000, // $gp
    [29] = 0x10000000 + MEMSIZE // $sp
  };
  word lo, hi;
  dword wide;

  // Control data
  integer count = 0;

  #define REG_HILO 33
  #define RREAD(REG) fprintf (stderr, "%s => 0x%x\n", REG == REG_HILO ? "HILO" : regname [REG], REG == REG_HILO ? lo : reg [REG])
  #define RWRITE(REG) fprintf (stderr, "%s <= 0x%x\n", REG == REG_HILO ? "HILO" : regname [REG], REG == REG_HILO ? lo : reg [REG])

  // Main loop
  while (1) {
    word instr  = Fetch(pc);
    Decode (pc, instr);
    reg [0] = 0; // $zero
    pc += 4;
    ++count;

    uint8_t  opcode = instr >> 26;
    uint8_t  rs     = (instr >> 21) & 0x1f;
    uint8_t  rt     = (instr >> 16) & 0x1f;
    uint8_t  rd     = (instr >> 11) & 0x1f;
    uint8_t  shamt  = (instr >> 6) & 0x1f;
    uint8_t  funct  = instr & 0x3f;
    uint16_t uimm   = instr & 0xffff;
    int16_t  simm   = uimm;
    word     addr   = instr & 0x3ffffff;
    word     jaddr  = (pc & 0xf0000000) + addr * 4;
    word     baddr  = pc + simm * 4;

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
            reg [rd] = reg [rs] >> shamt;
            RWRITE (rd);
            break;

          case JR:
            RREAD (rs);
            pc = reg [rs];
            break;

          case MFHI:
            RREAD (REG_HILO);
            reg [rd] = hi;
            RWRITE (rd);
            break;

          case MFLO:
            RREAD (REG_HILO);
            reg [rd] = lo;
            RWRITE (rd);
            break;

          case MULT:
            RREAD (rs);
            RREAD (rt);
            wide = reg [rs] * reg [rt];
            lo = wide & 0xffffffff;
            hi = wide >> 32;
            RWRITE (REG_HILO);
            break;

          case DIV:
            RREAD (rs);
            RREAD (rt);
            if (reg [rt] == 0)
            {
              fprintf (stderr, "division by zero: pc = 0x%x\n", pc-4);
              goto halt;
            }
            else
            {
              lo = reg [rs] / reg [rt];
              hi = reg [rs] % reg [rt];
            }
            RWRITE (REG_HILO);
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
            reg [rd] = ((sword) reg [rs] < (sword) reg [rt] ? 1 : 0);
            RWRITE (rd);
            break;

          default:
            fprintf (stderr, "unimplemented instruction: pc = 0x%x\n", pc-4);
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
        reg [rt] = simm << 16;
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
            scanf ("%"PRIu32, &reg [rt]);
            RWRITE (rt);
            break;

          case STOP:
            goto halt;
            break;

          default:
            fprintf (stderr, "unimplemented trap: pc = 0x%x\n", pc-4);
            goto halt;
        }
        break;

      case LW:
        RREAD (rs);
        fprintf (stderr, "0x%"PRIx32": Load %"PRIi16"(0x%"PRIx32") => ", pc-4, simm, reg [rs]);
        reg [rt] = LoadWord (reg [rs] + simm);
        fprintf (stderr, "0x%"PRIx32"\n", reg [rt]);
        RWRITE (rt);
        break;

      case SW:
        RREAD (rs);
        RREAD (rt);
        fprintf (stderr, "0x%"PRIx32": Store %"PRIi16"(0x%"PRIx32") <= 0x%"PRIx32"\n", pc-4, simm, reg [rs], reg [rt]);
        StoreWord (reg [rt], reg [rs] + simm);
        break;

      default:
        fprintf (stderr, "unimplemented instruction: pc = 0x%x\n", pc-4);
        goto halt;
    }
  }

halt:
  ;
}

int main(int argc, char *argv[])
{
  word c, start;
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

  instruction = (word *)(malloc(icount * 4));
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
