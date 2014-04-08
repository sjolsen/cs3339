/* -*- c-basic-offset: 2; tab-width: 2; indent-tabs-mode: nil; smart-tabs-mode: nil; eval: (c-set-offset 'case-label '+) -*- */

#if __STDC_VERSION__ < 199901L
# warning "This program should be compiled as C99 or better"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <limits.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>

typedef intmax_t integer;
#define PR_INTEGER PRIiMAX

// Extracts an the unsigned value on [low, high)
static inline
uint32_t bitrange (uint32_t value,
                   uint_fast8_t low,
                   uint_fast8_t high)
{
  return (value >> low) & ((1 << (high - low)) - 1);
}

// Extends the highest bit on [0, bits)
static inline
uint32_t sign_extend (uint32_t value,
                      uint_fast8_t bits)
{
  if (value & (1 << (bits - 1))) // Highest bit 1
    return value | ((uint32_t)-1 << bits);
  else
    return value;
}

static inline
uint_fast8_t log2ceil (uint32_t value)
{
  uint32_t mask = 0x80000000;
  uint_fast8_t result = 32;
  value = value - 1;

  while (mask != 0) {
    if (value & mask)
      return result;
    mask >>= 1;
    --result;
  }
}



enum {
  MEMSIZE = 1048576
};

static uint32_t icount, *instruction;
static uint32_t mem [MEMSIZE / 4];

static integer count        = 0;
static integer loads        = 0;
static integer load_misses  = 0;
static integer stores       = 0;
static integer store_misses = 0;
static integer write_backs  = 0;



enum {
  // Parameterized constants
  ASSOCIATIVITY = 4,
  BLOCK_SIZE    = 16,
  SETS          = 8,
  // Computed constants
  BLOCKS        = SETS * ASSOCIATIVITY,
  CACHE_SIZE    = BLOCKS * BLOCK_SIZE
};

enum {
  VALID = 0b01,
  DIRTY = 0b10
};

struct cacheline {
  uint32_t flags;
  uint32_t tag;
};

// Compute and initialize these "constants" as global non-const variables at
// runtime because C is so utterly braindead you can't even do log2 of a
// constant expression at compile time
static uint32_t offset_bits;
static uint32_t index_bits;

static uint32_t rand_bits;
static uint32_t rand_mask;

static inline
uint32_t offset_of (uint32_t address)
{
  return bitrange (address, 0, offset_bits);
}

static inline
uint32_t index_of (uint32_t address)
{
  return bitrange (address, offset_bits, offset_bits + index_bits);
}

static inline
uint32_t tag_of (uint32_t address)
{
  return bitrange (address, offset_bits + index_bits, 32);
}

static inline
struct cacheline* random_block (struct cacheline* begin,
                                struct cacheline* end)
{
  struct cacheline* block = begin + (count & rand_mask);
  assert (block < end);
  return block;
}

static
void INITIALIZE_GLOBAL_DATA ()
{
  offset_bits = log2ceil (BLOCK_SIZE);
  index_bits  = log2ceil (BLOCKS);

  rand_bits = log2ceil (ASSOCIATIVITY);
  rand_mask = (uint32_t)-1 >> (32 - rand_bits);
}



static
struct cacheline dcache_meta [BLOCKS] = {};

static
bool get_block (uint32_t address, struct cacheline* (*ret_block))
/* Returns whether the access hit; and a pointer to the cacheline in
   `ret_block'. Cache contents are not affected. */
{
  (*ret_block) = NULL;
  struct cacheline* set_begin = &(dcache_meta [index_of (address)]);
  struct cacheline* set_end   = set_begin + ASSOCIATIVITY;

  // Look for a block that has the right data
  for (struct cacheline* block = set_begin; block != set_end; ++block)
    if ((block->flags & VALID) && (block->tag == tag_of (address))) {
      (*ret_block) = block;
      return true;
    }

  // Look for an invalid block
  for (struct cacheline* block = set_begin; block != set_end; ++block)
    if (!(block->flags & VALID)) {
      (*ret_block) = block;
      return false;
    }

  // Get a random block
  (*ret_block) = random_block (set_begin, set_end);
  return false;
}

static
void write_back (struct cacheline* block)
{
  assert (block->flags & VALID);
  ++write_backs;
  block->flags &= ~DIRTY;
}

static
void write_allocate (uint32_t address, struct cacheline* block)
{
  assert (!(block->flags & VALID) || !(block->flags & DIRTY));
  block->flags = VALID & ~DIRTY;
  block->tag = tag_of (address);
}

static
bool prepare_block (uint32_t address, struct cacheline* (*ret_block))
/* Prepares a block for use, handling the processes of write-back and
   write-allocation, then returns whether the access was a hit. */
{
  bool hit = get_block (address, ret_block);
  struct cacheline* block = (*ret_block);

  if (!hit) {
    if ((block->flags & VALID) && (block->flags & DIRTY))
      write_back (block);
    write_allocate (address, block);
  }

  return hit;
}

static
void CLOAD (uint32_t address)
{
  struct cacheline* block = NULL;
  if (!prepare_block (address, &block)) // Miss
    ++load_misses;
  ++loads;
}

static
void CSTORE (uint32_t address)
{
  struct cacheline* block = NULL;
  if (!prepare_block (address, &block)) // Miss
    ++store_misses;
  block->flags |= DIRTY;
  ++stores;
}



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
  REGS = 1 + RA - ZERO,
  HILO
};

static void Interpret (uint32_t start)
/* This interpreter simulates a non-pipelined MIPS processor. Specifically, it
   simulates the cache behaviour of a MIPS program and reports certain
   statistics about that behaviour. */
{

  /// Registers
  uint32_t pc = start;
  uint32_t reg [REGS] = {
    [GP] = 0x10008000,
    [SP] = 0x10000000 + MEMSIZE
  };
  uint32_t lo, hi;

  /// Begin program execution
  while (1) {
    // IF1/2 operations
    uint32_t instr = Fetch (pc);
    reg [ZERO] = 0;
    pc += 4;
    ++count;

    // ID operations
    uint8_t  opcode = bitrange (instr, 26, 32);
    uint8_t  rs     = bitrange (instr, 21, 26);
    uint8_t  rt     = bitrange (instr, 16, 21);
    uint8_t  rd     = bitrange (instr, 11, 16);
    uint8_t  shamt  = bitrange (instr, 6, 11);
    uint8_t  funct  = bitrange (instr, 0, 6);
    uint16_t uimm   = bitrange (instr, 0, 16);
    int16_t  simm   = sign_extend (uimm, 16);
    uint32_t addr   = bitrange (instr, 0, 26);
    uint32_t jaddr  = (pc & 0xf0000000) + addr * 4;
    uint32_t baddr  = pc + simm * 4;

    // ID (via RREAD) through WB operations
    switch (opcode)
    {
      case FUNCTION:
        switch (funct)
        {
          case SLL:
            reg [rd] = reg [rs] << shamt;
            break;

          case SRA:
            reg [rd] = sign_extend (reg [rs] >> shamt, 32 - shamt);
            break;

          case JR:
            pc = reg [rs];
            break;

          case MFHI:
            reg [rd] = hi;
            break;

          case MFLO:
            reg [rd] = lo;
            break;

          case MULT:
            ; // C requires a statement after a label
            uint64_t wide = reg [rs] * reg [rt];
            lo = wide & 0xffffffff;
            hi = wide >> 32;
            break;

          case DIV:
            if (reg [rt] == 0) {
              fprintf (stderr, "division by zero: pc = 0x%"PRIx32"\n", pc - 4);
              goto halt;
            }
            else {
              lo = reg [rs] / reg [rt];
              hi = reg [rs] % reg [rt];
            }
            break;

          case ADDU:
            reg [rd] = reg [rs] + reg [rt];
            break;

          case SUBU:
            reg [rd] = reg [rs] - reg [rt];
            break;

          case SLT:
            reg [rd] = ((int32_t) reg [rs] < (int32_t) reg [rt] ? 1 : 0);
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
        reg [RA] = pc;
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

      case ADDIU:
        reg [rt] = reg [rs] + simm;
        break;

      case ANDI:
        reg [rt] = reg [rs] & uimm;
        break;

      case LUI:
        reg [rt] = uimm << 16;
        break;

      case TRAP:
        switch (addr & 0xf)
        {
          case NEWLINE:
            printf ("\n");
            break;

          case PRINT:
            printf (" %d", reg [rs]);
            break;

          case PROMPT:
            printf ("\n? ");
            fflush (stdout);
            int32_t input;
            scanf ("%"PRIi32, &input);
            reg [rt] = input;
            break;

          case STOP:
            goto halt;

          default:
            fprintf (stderr, "unimplemented trap: pc = 0x%"PRIx32"\n", pc - 4);
            goto halt;
        }
        break;

      case LW:
        CLOAD (reg [rs] + simm);
        reg [rt] = LoadWord (reg [rs] + simm);
        break;

      case SW:
        CSTORE (reg [rs] + simm);
        StoreWord (reg [rt], reg [rs] + simm);
        break;

      default:
        fprintf (stderr, "unimplemented instruction: pc = 0x%"PRIx32"\n", pc - 4);
        goto halt;
    }
  }

halt:
  printf ("\nprogram finished at pc = 0x%"PRIx32"  (%"PR_INTEGER" instructions executed)\n", pc, count);

  printf ("\n"
          "loads: %"PR_INTEGER"\n"
          "load misses: %"PR_INTEGER"\n"
          "stores: %"PR_INTEGER"\n"
          "store misses: %"PR_INTEGER"\n"
          "write backs: %"PR_INTEGER"\n",
          loads,
          load_misses,
          stores,
          store_misses,
          write_backs);

  printf ("load hit ratio: %.3f%%\n"
          "store hit ratio: %.3f%%\n"
          "overall hit ratio: %.3f%%\n"
          "write backs per store: %.3f%%\n",
          (100.0 * (loads - load_misses)) / loads,
          (100.0 * (stores - store_misses)) / stores,
          (100.0 * ((loads + stores) - (load_misses + store_misses))) / (loads + stores),
          (100.0 * write_backs) / stores);
}



int main(int argc, char *argv[])
{
  uint32_t c, start;
  int little_endian;
  FILE *f;

  INITIALIZE_GLOBAL_DATA ();

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
