#define _GNU_SOURCE
#include "fuzz_fuzzer.h"
#include "fuzz_random.h"
#include <dirent.h>
#include <sim_avr.h>
#include <stdio.h>

void initialize_fuzzer(avr_t *avr, char *path_to_seeds, size_t max_input_len) {
  Fuzzer *fuzzer = malloc(sizeof(Fuzzer));
  fuzzer->max_input_length = max_input_len;

  Input *current_input = malloc(sizeof(Input));
  void *current_input_buffer = malloc(max_input_len);
  current_input->buf = current_input_buffer;
  current_input->buf_len = max_input_len;
  fuzzer->current_input = current_input;

  CC_ArrayConf array_conf;
  cc_array_conf_init(&array_conf);
  array_conf.capacity = 1000;
  CC_Array *previous_interesting_inputs;
  if (cc_array_new_conf(&array_conf, &previous_interesting_inputs) != CC_OK) {
    perror("ERROR: Failed to initialize previous_interesting_input array\n");
    exit(1);
  }
  fuzzer->previous_interesting_inputs = previous_interesting_inputs;

  if (path_to_seeds != NULL) {
    initialize_seeds(previous_interesting_inputs, path_to_seeds, max_input_len);
  }

  generate_input(avr, fuzzer);
  // printf("%.*s\n", (int)fuzzer->current_input->buf_len,
  //       (char *)fuzzer->current_input->buf);
}

void initialize_seeds(CC_Array *previous_interesting_inputs,
                      char *path_to_seeds, size_t max_len) {
  // for each file in the seeds folder, add the contents of this file to the
  // previous_interesting_inputs array
  DIR *seeds_dir = opendir(path_to_seeds);
  if (seeds_dir == NULL) {
    fprintf(stderr, "Could not open the seeds directory %s", path_to_seeds);
    exit(1);
  }

  struct dirent *seeds_dir_entry;
  char path_to_seed_file[512];
  strcat(path_to_seed_file, path_to_seeds);
  size_t path_to_seeds_len = strlen(path_to_seed_file);
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
    add_seed_from_file(previous_interesting_inputs, path_to_seed_file, max_len);
  }

  closedir(seeds_dir);
  // printf("X seed files added to the list of previous interesting inputs");

  // TODO: also skip ones that exceed max input size
}

void add_seed_from_file(CC_Array *previous_interesting_inputs, char *file_path,
                        size_t max_len) {
  FILE *f = fopen(file_path, "r");
  if (f == NULL) {
    printf("Failed to open seed file %s\n", file_path);
    return;
  }

  int pos = 0;
  int cur;
  char *buffer = malloc(max_len);
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
    perror("Failed to add seed to previous_interesting_inputs array. \
            Likely out of memory.");
    exit(1);
  }
}

Input *get_random_previous_interesting_input(CC_Array *inputs) {

  if (cc_array_size(inputs) == 0) {
    perror("No previous interesting input.\n");
    exit(1);
  }
  size_t array_index = fast_random() % cc_array_size(inputs);

  void *entry;
  if (cc_array_get_at(inputs, array_index, &entry) != CC_OK) {
    perror("Failed to retrieve element from previous_interesting_inputs array");
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

  mutate(fuzzer->current_input->buf, fuzzer->current_input->buf_len,
         fuzzer->max_input_length);

  avr->input_has_reached_new_coverage = 0;
}

// TODOE might use max_len later
void mutate(void *buffer, size_t buf_len, size_t max_len) {
  int num_mutations_1 = fast_random() % 5 + 1;
  int num_mutations_2 = fast_random() % 5 + 1;
  for (int i = 0; i < num_mutations_1; i++) {
    for (int j = 0; j < num_mutations_2; j++) {
      // TODOE here i can have another random that decides what to do e.g. flip,
      // add, remove
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
  add_previous_interesting_input(avr->fuzzer->previous_interesting_inputs,
                                 avr->fuzzer->current_input->buf,
                                 avr->fuzzer->current_input->buf_len);
}