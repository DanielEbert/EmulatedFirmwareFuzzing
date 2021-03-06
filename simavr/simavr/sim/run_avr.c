/*
        run_avr.c

        Copyright 2008, 2010 Michel Pollet <buserror@gmail.com>

        This file is part of simavr.

        simavr is free software: you can redistribute it and/or modify
        it under the terms of the GNU General Public License as published by
        the Free Software Foundation, either version 3 of the License, or
        (at your option) any later version.

        simavr is distributed in the hope that it will be useful,
        but WITHOUT ANY WARRANTY; without even the implied warranty of
        MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
        GNU General Public License for more details.

        You should have received a copy of the GNU General Public License
        along with simavr.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "fuzz_coverage.h"
#include "fuzz_crash_handler.h"
#include "fuzz_fuzzer.h"
#include "fuzz_patch_instructions.h"
#include "sim_avr.h"
#include "sim_core.h"
#include "sim_elf.h"
#include "sim_gdb.h"
#include "sim_hex.h"
#include "sim_vcd_file.h"
#include <collectc/cc_hashtable.h>
#include <errno.h>
#include <libgen.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sim_core_decl.h"

static void display_usage(const char *app) {
  printf("Usage: %s [...] <firmware>\n", app);
  printf(
      "       [--freq|-f <freq>]  Sets the frequency for an .hex firmware\n"
      "       [--mcu|-m <device>] Sets the MCU type for an .hex firmware\n"
      "       [--list-cores]      List all supported AVR cores and exit\n"
      "       [--help|-h]         Display this usage message and exit\n"
      "       [--trace, -t]       Run full scale decoder trace\n"
      "       [-ti <vector>]      Add traces for IRQ vector <vector>\n"
      "       [--gdb|-g [<port>]] Listen for gdb connection on <port> (default "
      "1234)\n"
      "       [-ff <.hex file>]   Load next .hex file as flash\n"
      "       [-ee <.hex file>]   Load next .hex file as eeprom\n"
      "       [--input|-i <file>] A vcd file to use as input signals\n"
      "       [--output|-o <file>] A vcd file to save the traced signals\n"
      "       [--add-trace|-at <name=kind@addr/mask>] Add signal to be traced\n"
      "       [-v]                Raise verbosity level\n"
      "                           (can be passed more than once)\n"
      "       <firmware>          A .hex or an ELF file. ELF files are\n"
      "                           prefered, and can include debugging syms\n");
  exit(1);
}

static void list_cores() {
  printf("Supported AVR cores:\n");
  for (int i = 0; avr_kind[i]; i++) {
    printf("       ");
    for (int ti = 0; ti < 4 && avr_kind[i]->names[ti]; ti++)
      printf("%s ", avr_kind[i]->names[ti]);
    printf("\n");
  }
  exit(1);
}

static avr_t *avr = NULL;

static void sig_int(int sign) {
  printf("signal caught, simavr terminating\n");
  if (avr)
    avr_terminate(avr);
  exit(0);
}

static void sig_crash(int sign) {
  printf("ERROR: emulator caught signal %d. Storing current input to "
         "emulator_crash_input file on disk and terminating\n",
         sign);
  if (avr) {
    if (avr->fuzzer != NULL && avr->fuzzer->current_input != NULL &&
        avr->fuzzer->current_input->buf != NULL) {
      FILE *input_file = fopen("emulator_crash_input", "wb");
      if (input_file == NULL) {
        fprintf(stderr, "(1) Failed to write current input to file.\n");
      } else {
        size_t bytes_written =
            fwrite(avr->fuzzer->current_input->buf, 1,
                   avr->fuzzer->current_input->buf_len, input_file);
        if (bytes_written != avr->fuzzer->current_input->buf_len) {
          fprintf(stderr, "(2) Failed to write current input to file.\n");
        }
        fclose(input_file);
      }
    } else {
      fprintf(stderr, "(3) Failed to write current input to file.\n");
    }
    avr_terminate(avr);
  }
  exit(1);
}

int main(int argc, char *argv[]) {
  elf_firmware_t f = {{0}};
  uint32_t f_cpu = 0;
  int trace = 0;
  int gdb = 0;
  int log = 1;
  int port = 1234;
  char name[24] = "";
  uint32_t loadBase = AVR_SEGMENT_OFFSET_FLASH;
  int trace_vectors[8] = {0};
  int trace_vectors_count = 0;
  const char *vcd_input = NULL;
  char *run_once_file = NULL;
  char *filename = NULL;
  // symbol lookup
  CC_HashTable *symbols = NULL;
  // Default timeout is a very large value, which in practice means that there
  // is no timeout.
  uint64_t timeout = 1;
  timeout <<= 62;
  char *path_to_seeds_dir = NULL;
  char *mutator_so_path = NULL;
  int report_timeouts = 1;
  // Input size in bytes
  size_t max_input_length = 128;

  if (argc == 1)
    display_usage(basename(argv[0]));

  if (cc_hashtable_new(&symbols) != CC_OK) {
    fprintf(stderr, "ERROR: Allocation failed for symbols hash table.\n");
    exit(1);
  }

  for (int pi = 1; pi < argc; pi++) {
    if (!strcmp(argv[pi], "--list-cores")) {
      list_cores();
    } else if (!strcmp(argv[pi], "-h") || !strcmp(argv[pi], "--help")) {
      display_usage(basename(argv[0]));
    } else if (!strcmp(argv[pi], "-m") || !strcmp(argv[pi], "--mcu")) {
      if (pi < argc - 1)
        snprintf(name, sizeof(name), "%s", argv[++pi]);
      else
        display_usage(basename(argv[0]));
    } else if (!strcmp(argv[pi], "-f") || !strcmp(argv[pi], "--freq")) {
      if (pi < argc - 1)
        f_cpu = atoi(argv[++pi]);
      else
        display_usage(basename(argv[0]));
    } else if (!strcmp(argv[pi], "-i") || !strcmp(argv[pi], "--input")) {
      if (pi < argc - 1)
        vcd_input = argv[++pi];
      else
        display_usage(basename(argv[0]));
    } else if (!strcmp(argv[pi], "-t") || !strcmp(argv[pi], "--trace")) {
      trace++;
    } else if (!strcmp(argv[pi], "-o") || !strcmp(argv[pi], "--output")) {
      if (pi + 1 >= argc) {
        fprintf(stderr, "%s: missing mandatory argument for %s.\n", argv[0],
                argv[pi]);
        exit(1);
      }
      snprintf(f.tracename, sizeof(f.tracename), "%s", argv[++pi]);
    } else if (!strcmp(argv[pi], "-at") || !strcmp(argv[pi], "--add-trace")) {
      if (pi + 1 >= argc) {
        fprintf(stderr, "%s: missing mandatory argument for %s.\n", argv[0],
                argv[pi]);
        exit(1);
      }
      ++pi;
      struct {
        char kind[64];
        uint8_t mask;
        uint16_t addr;
        char name[64];
      } trace;
      const int n_args =
          sscanf(argv[pi], "%63[^=]=%63[^@]@0x%hx/0x%hhx", &trace.name[0],
                 &trace.kind[0], &trace.addr, &trace.mask);
      if (n_args != 4) {
        --pi;
        fprintf(stderr, "%s: format for %s is name=kind@addr/mask.\n", argv[0],
                argv[pi]);
        exit(1);
      }

      /****/ if (!strcmp(trace.kind, "portpin")) {
        f.trace[f.tracecount].kind = AVR_MMCU_TAG_VCD_PORTPIN;
      } else if (!strcmp(trace.kind, "irq")) {
        f.trace[f.tracecount].kind = AVR_MMCU_TAG_VCD_IRQ;
      } else if (!strcmp(trace.kind, "trace")) {
        f.trace[f.tracecount].kind = AVR_MMCU_TAG_VCD_TRACE;
      } else {
        fprintf(stderr,
                "%s: unknown trace kind '%s', not one of 'portpin', 'irq', or "
                "'trace'.\n",
                argv[0], trace.kind);
        exit(1);
      }
      f.trace[f.tracecount].mask = trace.mask;
      f.trace[f.tracecount].addr = trace.addr;
      strncpy(f.trace[f.tracecount].name, trace.name,
              sizeof(f.trace[f.tracecount].name));

      printf("Adding %s trace on address 0x%04x, mask 0x%02x ('%s')\n",
             f.trace[f.tracecount].kind == AVR_MMCU_TAG_VCD_PORTPIN
                 ? "portpin"
                 : f.trace[f.tracecount].kind == AVR_MMCU_TAG_VCD_IRQ
                       ? "irq"
                       : f.trace[f.tracecount].kind == AVR_MMCU_TAG_VCD_TRACE
                             ? "trace"
                             : "unknown",
             f.trace[f.tracecount].addr, f.trace[f.tracecount].mask,
             f.trace[f.tracecount].name);

      ++f.tracecount;
    } else if (!strcmp(argv[pi], "-ti")) {
      if (pi < argc - 1)
        trace_vectors[trace_vectors_count++] = atoi(argv[++pi]);
    } else if (!strcmp(argv[pi], "-g") || !strcmp(argv[pi], "--gdb")) {
      gdb++;
      if (pi < (argc - 2) && argv[pi + 1][0] != '-')
        port = atoi(argv[++pi]);
    } else if (!strcmp(argv[pi], "-v")) {
      log++;
    } else if (!strcmp(argv[pi], "-ee")) {
      loadBase = AVR_SEGMENT_OFFSET_EEPROM;
    } else if (!strcmp(argv[pi], "-ff")) {
      loadBase = AVR_SEGMENT_OFFSET_FLASH;
    } else if (!strcmp(argv[pi], "--seeds")) {
      if (pi + 1 >= argc) {
        fprintf(stderr, "%s: missing path argument for %s.\n", argv[0],
                argv[pi]);
        exit(1);
      }
      path_to_seeds_dir = argv[++pi];
    } else if (!strcmp(argv[pi], "--max_input_length")) {
      if (pi + 1 >= argc) {
        fprintf(stderr,
                "%s: missing mandatory argument for --max_input_length %s.\n",
                argv[0], argv[pi]);
        exit(1);
      }
      errno = 0;
      char *i;
      // The timeout value must be an unsigned integer.
      max_input_length = strtoull(argv[++pi], &i, 10);
      if (errno == ERANGE) {
        perror("max_input_length too large: ");
        exit(1);
      }
      if (max_input_length > 2048) {
        printf("Warning: --max_input_length is set to %ld. This is probably "
               "too much. In most cases you are good with values < 256.",
               max_input_length);
      }
    } else if (!strcmp(argv[pi], "--mutator_so_path")) {
      if (pi + 1 >= argc) {
        fprintf(stderr,
                "%s: missing mandatory path to mutator.so file for %s.\n",
                argv[0], argv[pi]);
        exit(1);
      }
      mutator_so_path = argv[++pi];
    } else if (!strcmp(argv[pi], "--run_once_with")) {
      if (pi + 1 >= argc) {
        fprintf(stderr, "%s: missing mandatory path to input file for %s.\n",
                argv[0], argv[pi]);
        exit(1);
      }
      run_once_file = argv[++pi];
    } else if (!strcmp(argv[pi], "--timeout")) {
      if (pi + 1 >= argc) {
        fprintf(stderr, "%s: missing mandatory value argument for %s.\n",
                argv[0], argv[pi]);
        exit(1);
      }
      errno = 0;
      char *i;
      // The timeout value must be an unsigned integer.
      timeout = strtoull(argv[++pi], &i, 10);
      if (errno == ERANGE) {
        perror("Timeout too large: ");
        exit(1);
      }
    } else if (!strcmp(argv[pi], "--dont_report_timeouts")) {
      report_timeouts = 0;
    } else if (argv[pi][0] != '-') {
      filename = argv[pi];
      char *suffix = strrchr(filename, '.');
      if (suffix && !strcasecmp(suffix, ".hex")) {
        if (!name[0] || !f_cpu) {
          fprintf(stderr,
                  "%s: -mcu and -freq are mandatory to load .hex files\n",
                  argv[0]);
          exit(1);
        }
        ihex_chunk_p chunk = NULL;
        int cnt = read_ihex_chunks(filename, &chunk);
        if (cnt <= 0) {
          fprintf(stderr, "%s: Unable to load IHEX file %s\n", argv[0],
                  argv[pi]);
          exit(1);
        }
        printf("Loaded %d section of ihex\n", cnt);
        for (int ci = 0; ci < cnt; ci++) {
          if (chunk[ci].baseaddr < (1 * 1024 * 1024)) {
            f.flash = chunk[ci].data;
            f.flashsize = chunk[ci].size;
            f.flashbase = chunk[ci].baseaddr;
            printf("Load HEX flash %08x, %d\n", f.flashbase, f.flashsize);
          } else if (chunk[ci].baseaddr >= AVR_SEGMENT_OFFSET_EEPROM ||
                     chunk[ci].baseaddr + loadBase >=
                         AVR_SEGMENT_OFFSET_EEPROM) {
            // eeprom!
            f.eeprom = chunk[ci].data;
            f.eesize = chunk[ci].size;
            printf("Load HEX eeprom %08x, %d\n", chunk[ci].baseaddr, f.eesize);
          }
        }
      } else {
        if (elf_read_firmware(filename, &f, symbols) == -1) {
          fprintf(stderr, "%s: Unable to load firmware from file %s\n", argv[0],
                  filename);
          exit(1);
        }
      }
    }
  }

  if (filename == NULL) {
    fprintf(stderr, "ERROR - No file to emulate specified.\n");
    exit(1);
  }

  if (strlen(name))
    strcpy(f.mmcu, name);
  if (f_cpu)
    f.frequency = f_cpu;

  avr = avr_make_mcu_by_name(f.mmcu);
  if (!avr) {
    fprintf(stderr, "%s: AVR '%s' not known\n", argv[0], f.mmcu);
    exit(1);
  }
  avr->timeout = timeout;
  avr->report_timeouts = report_timeouts;
  avr->max_input_length = max_input_length;
  avr_init(avr);
  avr->log = (log > LOG_TRACE ? LOG_TRACE : log);
  avr->trace = trace;
  avr_load_firmware(avr, &f);
  if (f.flashbase) {
    printf("Attempted to load a bootloader at %04x\n", f.flashbase);
    avr->pc = f.flashbase;
  }
  for (int ti = 0; ti < trace_vectors_count; ti++) {
    for (int vi = 0; vi < avr->interrupts.vector_count; vi++)
      if (avr->interrupts.vector[vi]->vector == trace_vectors[ti])
        avr->interrupts.vector[vi]->trace = 1;
  }
  if (vcd_input) {
    static avr_vcd_t input;
    if (avr_vcd_init_input(avr, vcd_input, &input)) {
      fprintf(stderr, "%s: Warning: VCD input file %s failed\n", argv[0],
              vcd_input);
    }
  }

  // even if not setup at startup, activate gdb if crashing
  avr->gdb_port = port;
  if (gdb) {
    avr->state = cpu_Stopped;
    avr_gdb_init(avr);
  }

  // make sure that the required symbols are set
  char *required_symbols[] = {"__bss_end"};
  size_t required_symbols_size =
      sizeof(required_symbols) / sizeof(required_symbols[0]);

  for (int i = 0; i < required_symbols_size; i++) {
    if (!cc_hashtable_contains_key(symbols, required_symbols[i])) {
      fprintf(stderr,
              "Error: Trying to emulate an elf file that does not have the "
              "required symbol %s\n",
              required_symbols[i]);
      exit(1);
    }
  }
  avr->symbols = symbols;

  signal(SIGINT, sig_int);
  signal(SIGTERM, sig_int);

  signal(SIGSEGV, sig_crash);
  signal(SIGPIPE, sig_crash);
  signal(SIGFPE, sig_crash);
  signal(SIGXFSZ, sig_crash);

  initialize_fuzzer(avr, path_to_seeds_dir, run_once_file, mutator_so_path);
  initialize_server_notify(avr, /*do_connect=*/run_once_file != NULL, filename);
  initialize_patch_instructions(avr);
  initialize_coverage(avr);
  initialize_crash_handler(avr);

  for (;;) {
    int state = avr_run(avr);
    if (state == cpu_Done || state == cpu_Crashed)
      break;
  }

  avr_terminate(avr);
}
