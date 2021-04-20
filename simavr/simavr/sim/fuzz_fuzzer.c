#define _GNU_SOURCE
#include "fuzz_fuzzer.h"
#include "fuzz_config.h"
#include "fuzz_random.h"
#include <dirent.h>
#include <sim_avr.h>
#include <stdio.h>

void initialize_fuzzer(avr_t *avr, char *path_to_seeds, char *run_once_file) {
  Fuzzer *fuzzer = malloc(sizeof(Fuzzer));
  avr->fuzzer = fuzzer;

  Input *current_input = malloc(sizeof(Input));
  void *current_input_buffer = malloc(MAX_INPUT_LENGTH);
  current_input->buf = current_input_buffer;
  current_input->buf_len = 0;
  fuzzer->current_input = current_input;

  CC_ArrayConf array_conf;
  cc_array_conf_init(&array_conf);
  array_conf.capacity = 1000;
  CC_Array *previous_interesting_inputs;
  if (cc_array_new_conf(&array_conf, &previous_interesting_inputs) != CC_OK) {
    fprintf(stderr,
            "ERROR: Failed to initialize previous_interesting_input array\n");
    exit(1);
  }
  fuzzer->previous_interesting_inputs = previous_interesting_inputs;

  if (run_once_file != NULL) {
    avr->run_once = 1;
    add_seed_from_file(previous_interesting_inputs, run_once_file);
  } else if (path_to_seeds != NULL) {
    initialize_seeds(previous_interesting_inputs, path_to_seeds);
  } else {
    fprintf(stderr,
            "ERROR: Either run_once_file or path_to_seeds must not be NULL.\n");
  }

  generate_input(avr, fuzzer);

  // TODOE: refactor; malloc error checks
  avr->shadow = calloc(1, 1 << 16); // TODO: set the init ones to 1; change size
  avr->shadow_propagation = calloc(sizeof(avr_flashaddr_t), 1 << 16);
  // stack is towards the end.. lets just set the first addr to 1. note that
  // the registers are located there aswell, but since the standard lib code
  // is run at the start this is probably fine anyway
  for (int i = 0; i < 3000; i++) {
    avr->shadow[i] = 1;
  }
}

void initialize_seeds(CC_Array *previous_interesting_inputs,
                      char *path_to_seeds) {
  // for each file in the seeds folder, add the contents of this file to the
  // previous_interesting_inputs array
  DIR *seeds_dir = opendir(path_to_seeds);
  if (seeds_dir == NULL) {
    fprintf(stderr, "Could not open the seeds directory %s", path_to_seeds);
    exit(1);
  }

  struct dirent *seeds_dir_entry;
  char path_to_seed_file[512];
  size_t path_to_seeds_len = strlen(path_to_seeds);
  if (path_to_seeds_len >= 512) {
    fprintf(stderr,
            "path_to_seeds path length must be less than 512 characters long.");
    exit(1);
  }
  strcpy(path_to_seed_file, path_to_seeds);
  path_to_seed_file[path_to_seeds_len] = '/';
  path_to_seeds_len++;

  while ((seeds_dir_entry = readdir(seeds_dir)) != NULL) {
    if (seeds_dir_entry->d_type != DT_REG) {
      continue; // skip directories and symlinks
    }
    if (strlen(seeds_dir_entry->d_name) > 512 - path_to_seeds_len - 1) {
      continue;
    } // we can now safely call strcpy
    strcpy(path_to_seed_file + path_to_seeds_len, seeds_dir_entry->d_name);
    // printf("Adding file %s to the previous interesting inputs\n",
    //       path_to_seed_file);
    add_seed_from_file(previous_interesting_inputs, path_to_seed_file);
  }

  closedir(seeds_dir);
  // printf("X seed files added to the list of previous interesting inputs");

  // TODO: also skip ones that exceed max input size
}

void add_seed_from_file(CC_Array *previous_interesting_inputs,
                        char *file_path) {
  FILE *f = fopen(file_path, "r");
  if (f == NULL) {
    fprintf(stderr, "Failed to open seed file %s filelen: %ld\n", file_path,
            strlen(file_path));
    perror("");
    return;
  }

  int pos = 0;
  int cur;
  char *buffer = malloc(MAX_INPUT_LENGTH);
  do {
    cur = fgetc(f);
    buffer[pos] = cur;
    pos++;
  } while (cur != EOF);

  fclose(f);

  // -1 to pos due to the additional loop interation where cur is EOF
  add_previous_interesting_input(previous_interesting_inputs, buffer, pos - 1);
}

void add_previous_interesting_input(CC_Array *previous_interesting_inputs,
                                    char *buf, size_t buf_len) {
  Input *entry = malloc(sizeof(Input));
  entry->buf = buf;
  entry->buf_len = buf_len;

  if (cc_array_add(previous_interesting_inputs, entry) != CC_OK) {
    fprintf(stderr, "Failed to add seed to previous_interesting_inputs array. \
            Likely out of memory.");
    exit(1);
  }
}

Input *get_random_previous_interesting_input(CC_Array *inputs) {

  if (cc_array_size(inputs) == 0) {
    fprintf(stderr, "No previous interesting input.\n");
    exit(1);
  }
  size_t array_index = fast_random() % cc_array_size(inputs);

  void *entry;
  if (cc_array_get_at(inputs, array_index, &entry) != CC_OK) {
    fprintf(
        stderr,
        "Failed to retrieve element from previous_interesting_inputs array");
    exit(1);
  }
  return (Input *)entry;
}

void generate_input(avr_t *avr, Fuzzer *fuzzer) {
  Input *input = get_random_previous_interesting_input(
      fuzzer->previous_interesting_inputs);

  // Override old current input with a randomly selected previous input.
  memcpy(fuzzer->current_input->buf, input->buf, input->buf_len);
  fuzzer->current_input->buf_len = input->buf_len;

  mutate(fuzzer->current_input->buf, fuzzer->current_input->buf_len);

  avr->input_has_reached_new_coverage = 0;
}

void mutate(void *buffer, size_t buf_len) {
  // TODOE later i need to change this so that i can increase the buf_len.
  // maybe just pass fuzzer->current_input here
  int num_mutations_1 = fast_random() % (NUM_MUTATIONS + 1);
  int num_mutations_2 = fast_random() % (NUM_MUTATIONS + 1);
  for (int i = 0; i < num_mutations_1; i++) {
    for (int j = 0; j < num_mutations_2; j++) {
      // TODOE here i can have another random that decides what to do e.g.
      // flip, add, remove
      int index = fast_random() % buf_len;
      char new_value = fast_random();
      *(char *)(buffer + index) = new_value;
    }
  }
}

void evaluate_input(avr_t *avr) {
  // also hash and save to disk
  if (!avr->input_has_reached_new_coverage) {
    return;
  }
  printf("Input with new coverage added.\n");

  // Copy current input to a new buffer, buffer is saved as
  // previous_interesting_input, and the current input (buffer) can be reused
  char *buffer = malloc(MAX_INPUT_LENGTH);
  memcpy(buffer, avr->fuzzer->current_input->buf, MAX_INPUT_LENGTH);

  add_previous_interesting_input(avr->fuzzer->previous_interesting_inputs,
                                 buffer, avr->fuzzer->current_input->buf_len);
}