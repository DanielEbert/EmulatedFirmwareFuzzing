#include "fuzz_coverage.h"
#include "fuzz_server_notify.h"
#include "sim_avr.h"
#include "sim_avr_types.h"
#include <collectc/cc_hashset.h>
#include <stdio.h>

void initialize_coverage(avr_t *avr) {
  CC_HashSetConf config;
  cc_hashset_conf_init(&config);
  config.key_length = sizeof(Edge);
  config.hash = GENERAL_HASH;
  config.key_compare = edge_compare;

  CC_HashSet *coverage;
  if (cc_hashset_new_conf(&config, &coverage) != CC_OK) {
    printf("ERROR: coverage set allocation failed. Exiting...\n");
    exit(1);
  }
  avr->coverage = coverage;
}

void edge_triggered(avr_t *avr, avr_flashaddr_t from, avr_flashaddr_t to) {
  Edge *edge = malloc(sizeof(Edge));
  edge->from = from;
  edge->to = to;
  if (!cc_hashset_contains(avr->coverage, edge)) {
    cc_hashset_add(avr->coverage, edge);
    avr->input_has_reached_new_coverage = 1;
    send_coverage(avr, edge);
  } else {
    free(edge);
  }
}

// Returns zero if keys are equal
int edge_compare(const void *key1, const void *key2) {
  Edge e1 = *((Edge *)key1);
  Edge e2 = *((Edge *)key2);
  return !(e1.from == e2.from && e1.to == e2.to);
}