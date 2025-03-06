#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define FEATURE_FPU (1 << 0)      // Floating-Point Unit On-Chip
#define FEATURE_VME (1 << 1)      // Virtual 8086 Mode Extensions
#define FEATURE_DE (1 << 2)       // Debugging Extensions
#define FEATURE_PSE (1 << 3)      // Page Size Extension
#define FEATURE_TSC (1 << 4)      // Time Stamp Counter
#define FEATURE_MSR (1 << 5)      // Model Specific Registers
#define FEATURE_PAE (1 << 6)      // Physical Address Extension
#define FEATURE_MCE (1 << 7)      // Machine-Check Exception
#define FEATURE_CX8 (1 << 8)      // CMPXCHG8 Instruction
#define FEATURE_APIC (1 << 9)     // APIC On-Chip
#define FEATURE_SEP (1 << 11)     // SYSENTER/SYSEXIT instructions
#define FEATURE_MTRR (1 << 12)    // Memory Type Range Registers
#define FEATURE_PGE (1 << 13)     // Page Global Bit
#define FEATURE_MCA (1 << 14)     // Machine-Check Architecture
#define FEATURE_CMOV (1 << 15)    // Conditional Move Instruction
#define FEATURE_PAT (1 << 16)     // Page Attribute Table
#define FEATURE_PSE36 (1 << 17)   // 36-bit Page Size Extension
#define FEATURE_PSN (1 << 18)     // Processor Serial Number
#define FEATURE_CLFLUSH (1 << 19) // CLFLUSH Instruction
#define FEATURE_DS (1 << 21)      // Debug Store
#define FEATURE_ACPI (1 << 22) // Thermal Monitor and Software Clock Facilities
#define FEATURE_MMX (1 << 23)  // MMX Technology
#define FEATURE_FXSR (1 << 24) // FXSAVE and FXSTOR Instructions
#define FEATURE_SSE (1 << 25)  // Streaming SIMD Extensions
#define FEATURE_SSE2 (1 << 26) // Streaming SIMD Extensions 2
#define FEATURE_SS (1 << 27)   // Self Snoop
#define FEATURE_HTT (1 << 28)  // Multi-Threading
#define FEATURE_TM (1 << 29)   // Thermal Monitor
#define FEATURE_PBE (1 << 31)  // Pending Break Enable

#define INSTR_SSE3 (1 << 0)      // Streaming SIMD Extensions 3
#define INSTR_PCLMULQDQ (1 << 1) // PCLMULQDQ Instruction
#define INSTR_DTES64 (1 << 2)    // 64-Bit Debug Store Area
#define INSTR_MONITOR (1 << 3)   // MONITOR/MWAIT
#define INSTR_DS_CPL (1 << 4)    // CPL Qualified Debug Store
#define INSTR_VMX (1 << 5)       // Virtual Machine Extensions
#define INSTR_SMX (1 << 6)       // Safer Mode Extensions
#define INSTR_EST (1 << 7)       // Enhanced SpeedStep Technology
#define INSTR_TM2 (1 << 8)       // Thermal Monitor 2
#define INSTR_SSSE3 (1 << 9)     // Supplemental Streaming SIMD Extensions 3
#define INSTR_CNXT_ID (1 << 10)  // L1 Context ID
#define INSTR_FMA (1 << 12)      // Fused Multiply Add
#define INSTR_CX16 (1 << 13)     // CMPXCHG16B Instruction
#define INSTR_XTPR (1 << 14)     // xTPR Update Control
#define INSTR_PDCM (1 << 15)     // Perf/Debug Capability MSR
#define INSTR_PCID (1 << 17)     // Process-context Identifiers
#define INSTR_DCA (1 << 18)      // Direct Cache Access
#define INSTR_SSE41 (1 << 19)    // Streaming SIMD Extensions 4.1
#define INSTR_SSE42 (1 << 20)    // Streaming SIMD Extensions 4.2
#define INSTR_X2APIC (1 << 21)   // Extended xAPIC Support
#define INSTR_MOVBE (1 << 22)    // MOVBE Instruction
#define INSTR_POPCNT (1 << 23)   // POPCNT Instruction
#define INSTR_TSC (1 << 24)      // Local APIC supports TSC Deadline
#define INSTR_AESNI (1 << 25)    // AESNI Instruction
#define INSTR_XSAVE (1 << 26)    // XSAVE/XSTOR States
#define INSTR_OSXSAVE (1 << 27)  // OS Enabled Extended State Management
#define INSTR_AVX (1 << 28)      // AVX Instructions
#define INSTR_F16C (1 << 29)     // 16-bit Floating Point Instructions
#define INSTR_RDRAND (1 << 30)   // RDRAND Instruction

#define cpu_id(in, a, b, c, d)                                                 \
  __asm__("cpuid" : "=a"(a), "=b"(b), "=c"(c), "=d"(d) : "a"(in));

uint32_t cpu_features = 0;
uint32_t cpu_instructions = 0;

void cpu_detect() {
  uint32_t max_ext_func = 0;
  uint32_t unused = 0;
  uint32_t r[4];

  printf("cpu0 ");
    cpu_id(0x80000000, max_ext_func, unused, unused, unused);
  if (max_ext_func >= 0x80000004) {
    // Get CPU name
    char name[48];
    cpu_id(0x80000002, r[0], r[1], r[2], r[3]);
    memcpy(name, r, 16);
    cpu_id(0x80000003, r[0], r[1], r[2], r[3]);
    memcpy(name + 16, r, 16);
    cpu_id(0x80000004, r[0], r[1], r[2], r[3]);
    memcpy(name + 32, r, 16);

    // Processor name is right justified with leading spaces
    const char *p = name;
    while (*p == ' ') {
      ++p;
    }
    printf("%s, ", p);
  }

  uint32_t max_func = 0;
  // Get max CPUID function and manufacturer string
  cpu_id(0, max_func, r[0], r[2], r[1]);
  char manufacturer[13];
  memcpy(manufacturer, r, sizeof(manufacturer));
  manufacturer[12] = 0;
  printf("%s, ", manufacturer);

  if (max_func >= 1) {
    uint32_t version = 0;
    cpu_id(1, version, unused, cpu_instructions, cpu_features);

    unsigned stepping = version & 0x0F;
    unsigned model = (version >> 4) & 0xF;
    unsigned family = (version >> 8) & 0xF;
    printf("step=%u, model=%u, family=%u\n", stepping, model, family);

  //   printf("  Features: ");
  //   if (cpu_features & FEATURE_FPU)
  //     printf(" FPU");
  //   if (cpu_features & FEATURE_VME)
  //     printf(" VME");
  //   if (cpu_features & FEATURE_DE)
  //     printf(" DE");
  //   if (cpu_features & FEATURE_PSE)
  //     printf(" PSE");
  //   if (cpu_features & FEATURE_TSC)
  //     printf(" TSC");
  //   if (cpu_features & FEATURE_MSR)
  //     printf(" MSR");
  //   if (cpu_features & FEATURE_MCE)
  //     printf(" MCE");
  //   if (cpu_features & FEATURE_CX8)
  //     printf(" CX8");
  //   if (cpu_features & FEATURE_APIC)
  //     printf(" APIC");
  //   if (cpu_features & FEATURE_SEP)
  //     printf(" SEP");
  //   if (cpu_features & FEATURE_MTRR)
  //     printf(" MTRR");
  //   if (cpu_features & FEATURE_PGE)
  //     printf(" PGE");
  //   if (cpu_features & FEATURE_MCA)
  //     printf(" MCA");
  //   if (cpu_features & FEATURE_CMOV)
  //     printf(" CMOV");
  //   if (cpu_features & FEATURE_PAT)
  //     printf(" PAT");
  //   if (cpu_features & FEATURE_PSE36)
  //     printf(" PSE36");
  //   if (cpu_features & FEATURE_PSN)
  //     printf(" PSN");
  //   if (cpu_features & FEATURE_CLFLUSH)
  //     printf(" CLFLUSH");
  //   if (cpu_features & FEATURE_DS)
  //     printf(" DS");
  //   if (cpu_features & FEATURE_ACPI)
  //     printf(" ACPI");
  //   if (cpu_features & FEATURE_MMX)
  //     printf(" MMX");
  //   if (cpu_features & FEATURE_FXSR)
  //     printf(" FXSR");
  //   if (cpu_features & FEATURE_SSE)
  //     printf(" SSE");
  //   if (cpu_features & FEATURE_SSE2)
  //     printf(" SSE2");
  //   if (cpu_features & FEATURE_SS)
  //     printf(" SS");
  //   if (cpu_features & FEATURE_HTT)
  //     printf(" HTT");
  //   if (cpu_features & FEATURE_TM)
  //     printf(" TM");
  //   if (cpu_features & FEATURE_PBE)
  //     printf(" PBE");
  //   printf("\n");

  //   printf("  Instructions:");
  //   if (cpu_instructions & INSTR_SSE3)
  //     printf(" SSE3");
  //   if (cpu_instructions & INSTR_PCLMULQDQ)
  //     printf(" PCLMULQDQ");
  //   if (cpu_instructions & INSTR_DTES64)
  //     printf(" DTES64");
  //   if (cpu_instructions & INSTR_MONITOR)
  //     printf(" MONITOR");
  //   if (cpu_instructions & INSTR_DS_CPL)
  //     printf(" DS_CPL");
  //   if (cpu_instructions & INSTR_VMX)
  //     printf(" VMX");
  //   if (cpu_instructions & INSTR_SMX)
  //     printf(" SMX");
  //   if (cpu_instructions & INSTR_EST)
  //     printf(" EST");
  //   if (cpu_instructions & INSTR_TM2)
  //     printf(" TM2");
  //   if (cpu_instructions & INSTR_CNXT_ID)
  //     printf(" CNXT_ID");
  //   if (cpu_instructions & INSTR_FMA)
  //     printf(" FMA");
  //   if (cpu_instructions & INSTR_CX16)
  //     printf(" CX16");
  //   if (cpu_instructions & INSTR_XTPR)
  //     printf(" XTPR");
  //   if (cpu_instructions & INSTR_PDCM)
  //     printf(" PDCM");
  //   if (cpu_instructions & INSTR_PCID)
  //     printf(" PCID");
  //   if (cpu_instructions & INSTR_DCA)
  //     printf(" DCA");
  //   if (cpu_instructions & INSTR_SSE41)
  //     printf(" SSE41");
  //   if (cpu_instructions & INSTR_SSE42)
  //     printf(" SSE42");
  //   if (cpu_instructions & INSTR_X2APIC)
  //     printf(" X2APIC");
  //   if (cpu_instructions & INSTR_MOVBE)
  //     printf(" MOVBE");
  //   if (cpu_instructions & INSTR_POPCNT)
  //     printf(" POPCNT");
  //   if (cpu_instructions & INSTR_TSC)
  //     printf(" TSC");
  //   if (cpu_instructions & INSTR_AESNI)
  //     printf(" AESNI");
  //   if (cpu_instructions & INSTR_XSAVE)
  //     printf(" XSAVE");
  //   if (cpu_instructions & INSTR_OSXSAVE)
  //     printf(" OSXSAVE");
  //   if (cpu_instructions & INSTR_AVX)
  //     printf(" AVX");
  //   if (cpu_instructions & INSTR_F16C)
  //     printf(" F16C");
  //   if (cpu_instructions & INSTR_RDRAND)
  //     printf(" RDRAND");
  //   printf("\n");
   }
}