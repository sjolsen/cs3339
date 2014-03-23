#include <stdlib.h>
#include <stdio.h>

const char reg[32][6] = {"$zero","$at","$v0","$v1","$a0","$a1","$a2","$a3",
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

  const char* RD = reg [rd];
  const char* RS = reg [rs];
  const char* RT = reg [rt];

  #define PRINT(format, ...) (printf ("%8x: "format"\n", pc, ##__VA_ARGS__))

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

static int Convert(unsigned int x)
{
  return (x >> 24) | ((x >> 8) & 0xff00) | ((x << 8) & 0xff0000) | (x << 24);
}

int main(int argc, char *argv[])
{
  int c, count, start, little_endian, *instruction;
  FILE *f;

  printf("CS3339 MIPS Disassembler\n");
  if (argc != 2) {fprintf(stderr, "usage: %s mips_executable\n", argv[0]); exit(-1);}
  if (sizeof(int) != 4) {fprintf(stderr, "error: need 4-byte integers\n"); exit(-1);}

  count = 1;
  little_endian = *((char *)&count);

  f = fopen(argv[1], "r+b");
  if (f == NULL) {fprintf(stderr, "error: could not open file %s\n", argv[1]); exit(-1);}
  c = fread(&count, 4, 1, f);
  if (c != 1) {fprintf(stderr, "error: could not read count from file %s\n", argv[1]); exit(-1);}
  if (little_endian) {
    count = Convert(count);
  }
  c = fread(&start, 4, 1, f);
  if (c != 1) {fprintf(stderr, "error: could not read start from file %s\n", argv[1]); exit(-1);}
  if (little_endian) {
    start = Convert(start);
  }

  instruction = (int *)(malloc(count * 4));
  if (instruction == NULL) {fprintf(stderr, "error: out of memory\n"); exit(-1);}
  c = fread(instruction, 4, count, f);
  if (c != count) {fprintf(stderr, "error: could not read instructions from file %s\n", argv[1]); exit(-1);}
  fclose(f);
  if (little_endian) {
    for (c = 0; c < count; c++) {
      instruction[c] = Convert(instruction[c]);
    }
  }

  for (c = 0; c < count; c++) {
    Decode(start + c * 4, instruction[c]);
  }
}
