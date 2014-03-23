/* -*- c-basic-offset: 2; tab-width: 2; indent-tabs-mode: nil; eval: (c-set-offset 'case-label '+) -*- */

#include <stdlib.h>
#include <stdio.h>

#define MEMSIZE 1048576

static int little_endian, icount, *instruction;
static int mem[MEMSIZE / 4];

static int Convert(unsigned int x)
{
  return (x >> 24) | ((x >> 8) & 0xff00) | ((x << 8) & 0xff0000) | (x << 24);
}

static int Fetch(int pc)
{
  pc = (pc - 0x00400000) >> 2;
  if ((unsigned)pc >= icount) {
    fprintf(stderr, "instruction fetch out of range\n");
    exit(-1);
  }
  return instruction[pc];
}

static int LoadWord(int addr)
{
  if (addr & 3 != 0) {
    fprintf(stderr, "unaligned data access\n");
    exit(-1);
  }
  addr -= 0x10000000;
  if ((unsigned)addr >= MEMSIZE) {
    fprintf(stderr, "data access out of range\n");
    exit(-1);
  }
  return mem[addr / 4];
}

static void StoreWord(int data, int addr)
{
  if (addr & 3 != 0) {
    fprintf(stderr, "unaligned data access\n");
    exit(-1);
  }
  addr -= 0x10000000;
  if ((unsigned)addr >= MEMSIZE) {
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

static void Interpret(int start)
{
  register int instr, opcode, rs, rt, rd, shamt, funct, uimm, simm, addr, jaddr, baddr;
  register int pc, hi, lo;
  int reg[32];
  register int cont = 1, count = 0;
  register long long wide;

  pc = start;
  reg[28] = 0x10008000;  // gp
  reg[29] = 0x10000000 + MEMSIZE;  // sp

  while (cont) {
    count++;
    instr = Fetch(pc);
    pc += 4;
    reg[0] = 0;  // $zero

    opcode = (unsigned)instr >> 26;
    rs = (instr >> 21) & 0x1f;
    rt = (instr >> 16) & 0x1f;
    rd = (instr >> 11) & 0x1f;
    shamt = (instr >> 6) & 0x1f;
    funct = instr & 0x3f;
    uimm = instr & 0xffff;
    simm = ((signed)uimm << 16) >> 16;
    addr = instr & 0x3ffffff;
    jaddr = (pc & 0xf0000000) + addr * 4;
    baddr = pc + simm * 4;

    switch (opcode) {
      case FUNCTION:
        switch (funct) {
          case SLL: reg [rd] = reg [rs] << shamt; break;
          case SRA: reg [rd] = reg [rs] >> shamt; break;
          case JR: pc = reg [rs]; break;
          case MFHI: reg [rd] = hi; break;
          case MFLO: reg [rd] = lo; break;

          case MULT:
            wide = reg [rs] * reg [rt];
            lo = wide & 0xffffffff;
            hi = wide >> 32;
            break;
          case DIV:
            if (reg [rt] == 0) {
              fprintf (stderr, "division by zero: pc = 0x%x\n", pc-4);
              cont = 0;
            } else {
              lo = reg [rs] / reg [rt];
              hi = reg [rs] % reg [rt];
            }
            break;

          case ADDU: reg [rd] = reg [rs] + reg [rt]; break;
          case SUBU: reg [rd] = reg [rs] - reg [rt]; break;
          case SLT: reg [rd] = (reg [rs] < reg [rt] ? 1 : 0); break;

          default:
            fprintf (stderr, "unimplemented instruction: pc = 0x%x\n", pc-4);
            cont = 0;
        }
        break;

      case J:
        pc = jaddr;
        break;
      case JAL:
        reg [31] = pc;
        pc = jaddr;
        break;
      case BEQ:
        if (reg [rs] == reg [rt])
          pc = baddr;
        break;
      case BNE:
        if (reg [rs] != reg [rt])
          pc = baddr;
        break;

      case ADDIU: reg [rt] = reg [rs] + simm; break;
      case ANDI: reg [rt] = reg [rs] & uimm; break;
      case LUI: reg [rt] = simm << 16; break;

      case TRAP:
        switch (addr & 0xf) {
          case NEWLINE: printf ("\n"); break;
          case PRINT: printf (" %d", reg [rs]); break;
          case PROMPT:
            printf ("\n? ");
            fflush (stdout);
            scanf ("%d", &reg [rt]);
            break;
          case STOP: cont = 0; break;
          default:
            fprintf (stderr, "unimplemented trap: pc = 0x%x\n", pc-4);
            cont = 0;
        }
        break;

      case LW: reg [rt] = LoadWord (reg [rs] + simm); break;
      case SW: StoreWord (reg [rt], reg [rs] + simm); break;

      default:
        fprintf (stderr, "unimplemented instruction: pc = 0x%x\n", pc-4);
        cont = 0;
    }
  }

  printf("\nprogram finished at pc = 0x%x  (%d instructions executed)\n", pc, count);
}

int main(int argc, char *argv[])
{
  int c, start;
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

  instruction = (int *)(malloc(icount * 4));
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
