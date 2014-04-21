/* -*- c-basic-offset: 2; tab-width: 2; eval: (c-set-offset 'case-label '+) -*- */

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


/// Utility definitions

typedef intmax_t integer;
#define PR_INTEGER PRIiMAX

static inline
uint32_t bitrange (uint32_t value,
                   uint_fast8_t low,
                   uint_fast8_t high)
/* Extracts an the unsigned value on [low, high) */
{
	return (value >> low) & ((1 << (high - low)) - 1);
}

static inline
uint32_t sign_extend (uint32_t value,
                      uint_fast8_t bits)
/* Extends the highest bit on [0, bits) */
{
	if (value & (1 << (bits - 1))) // Highest bit 1
		return value | ((uint32_t)-1 << bits);
	else
		return value;
}


/// Global system state

enum {
	MEMSIZE = 1048576
};

static uint32_t icount, *instruction;
static uint32_t mem [MEMSIZE / 4];


/// Branch target predictor definitions

enum {
	BTB_SIZE = 16
};

static uint32_t btb_table [BTB_SIZE] = {};

static integer btb_accesses = 0;
static integer btb_hits     = 0;

static inline
int btb_index (uint32_t address)
{
	return (address >> 2) % BTB_SIZE;
}

static
void btb_predict (uint32_t instruction_address,
                  uint32_t actual_target)
{
	++btb_accesses;
	if (btb_table [btb_index (instruction_address)] == actual_target)
		++btb_hits;
	btb_table [btb_index (instruction_address)] = actual_target;
}


/// Load address predictor definitions

enum {
	LAP_SIZE = 16
};

struct lap_entry {
	uint32_t last_load_address;
	uint32_t second_last_load_address;
};

static struct lap_entry lap_table [LAP_SIZE] = {};

static integer lap_accesses = 0;
static integer lap_hits     = 0;

static inline
int lap_index (uint32_t address)
{
	return (address >> 2) % LAP_SIZE;
}

static
void lap_predict (uint32_t instruction_address,
                  uint32_t actual_load_address)
{
	++lap_accesses;

	struct lap_entry (*entry) = &lap_table [lap_index (instruction_address)];
	uint32_t x = entry->last_load_address;
	uint32_t y = entry->second_last_load_address;
	uint32_t prediction = x + (x - y);

	if (prediction == actual_load_address)
		++lap_hits;

	entry->second_last_load_address = x;
	entry->last_load_address = actual_load_address;
}


/// Load value frequency definitions

enum {
	LVF_MAX_FREQS = 5200,
	LVF_PRINT_FREQS = 10
};

struct lvf_freq {
	uint32_t value;
	integer freq;
};

static integer lvf_nvalues = 0;

static struct lvf_freq lvf_freqs [LVF_MAX_FREQS] = {};
static struct lvf_freq* lvf_next = lvf_freqs;

static
struct lvf_freq* lvf_lookup (uint32_t value)
{
	struct lvf_freq* left = lvf_freqs;
	struct lvf_freq* right = lvf_next;
	struct lvf_freq* loc = NULL;

	// Binary search in descending order
	while (left < right) {
		struct lvf_freq* mid = left + (right - left)/2;
		if (mid->value == value) {
			loc = mid;
			break;
		}
		else if (mid->value > value) {
			left = mid + 1;
			continue;
		}
		else /* (mid->value < value) */ {
			right = mid;
			continue;
		}
	}

	// Not found, so insert at left
	if (loc == NULL) {
		if (lvf_next >= lvf_freqs + LVF_MAX_FREQS) {
			fprintf (stderr, "loaded too many unique values for the profiler\n");
			exit (EXIT_FAILURE);
		}

		memmove (left + 1, left, (lvf_next - left) * sizeof (*left));
		++lvf_next;
		loc = left;
		*loc = (struct lvf_freq) {value, 0};
	}

	return loc;
}

static
void lvf_load (uint32_t value)
{
	++lvf_lookup (value)->freq;
	++lvf_nvalues;
}

static inline
int freq_greater (const void* a, const void* b) {
	integer freqa = (*(const struct lvf_freq*)a).freq;
	integer freqb = (*(const struct lvf_freq*)b).freq;
	return freqb - freqa;
}

static
int lvf_freqreduce ()
/* Returns the number of frequencies available in `lvf_freqs' */
{
	int nfreqs = lvf_next - lvf_freqs;
	qsort (lvf_freqs, nfreqs, sizeof (lvf_freqs [0]), freq_greater);
	return nfreqs;
}


/// Memory access routines

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


/// The processor proper

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
   simulates the predictor behaviour of a MIPS program and reports certain
   statistics about that behaviour. */
{
	/// Registers
	uint32_t pc = start;
	uint32_t reg [REGS] = {
		[GP] = 0x10008000,
		[SP] = 0x10000000 + MEMSIZE
	};
	uint32_t lo = 0xDEADBEEF;
	uint32_t hi = 0xDEADBEEF;

	integer count = 0;

	/// Begin program execution
	while (1) {
		uint32_t instr = Fetch (pc);
		reg [ZERO] = 0;
		pc += 4;
		++count;

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
				btb_predict (pc - 4, reg [rs]);
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
			reg [rt] = (uint32_t)uimm << 16;
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
			lap_predict (pc - 4, reg [rs] + simm);
			reg [rt] = LoadWord (reg [rs] + simm);
			lvf_load (reg [rt]);
			break;

		case SW:
			StoreWord (reg [rt], reg [rs] + simm);
			break;

		default:
			fprintf (stderr, "unimplemented instruction: pc = 0x%"PRIx32"\n", pc - 4);
			goto halt;
		}
	}

halt:
	printf ("\nprogram finished at pc = 0x%"PRIx32"  (%"PR_INTEGER" instructions executed)\n", pc, count);

	if (btb_accesses > 0)
		printf ("indirect jumps: %"PR_INTEGER"\n"
		        "BTB hits: %"PR_INTEGER" (%.1f%%)\n",
		        btb_accesses,
		        btb_hits,
		        (100.0 * btb_hits) / btb_accesses);

	printf ("load instructions: %"PR_INTEGER"\n"
	        "load address hits: %"PR_INTEGER" (%.1f%%)\n",
	        lap_accesses,
	        lap_hits,
	        (100.0 * lap_hits) / lap_accesses);

	int total_freqs = lvf_freqreduce ();
	int print_freqs = total_freqs > LVF_PRINT_FREQS ? LVF_PRINT_FREQS : total_freqs;
	printf ("%d unique values loaded\n"
	        "top %d most frequently loaded values\n",
	        total_freqs,
	        print_freqs);
	for (const struct lvf_freq* freq = lvf_freqs; freq < lvf_freqs + print_freqs; ++freq)
		printf ("val = %"PRIu32" freq = %"PR_INTEGER" (%.1f%%)\n",
		        freq->value,
		        freq->freq,
		        (100.0 * freq->freq) / lvf_nvalues);
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
