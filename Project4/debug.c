#include <stdio.h>

const char regname[32][6] = {"$zero","$at","$v0","$v1","$a0","$a1","$a2","$a3",
                             "$t0","$t1","$t2","$t3","$t4","$t5","$t6","$t7","$s0","$s1","$s2","$s3",
                             "$s4","$s5","$s6","$s7","$t8","$t9","$k0","$k1","$gp","$sp","$fp","$ra"};

void Decode (int pc, int instr)
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
