#include "graph.h"
#include "priority_queue.h"
#include <stdio.h>
#include <stdlib.h>

int main (int argc, char *argv[]) {

  graph_t *graph;
  int *widths;
  int *prev;
  int *rtable;
  int i;

  if((graph = create_graph_from_file(argv[1])) == NULL) {
    printf("Error: Couldn't create graph from file: %s\n", argv[1]);
    return EXIT_FAILURE;
  }

  calc_widest_path_tree(graph, 0, &widths, &prev);

  printf("Widest Path:\n");

  for(i=0; i < graph->size; i++) {
    if(widths[i] == INF)
      printf("Node %d: Prev = %d BW = INF\n", i, prev[i]);
    else
      printf("Node %d: Prev = %d BW = %d\n", i, prev[i], widths[i]);
  }

  rtable = build_routing_table(graph, 0);

  printf("Printing Routing Table:\n");
  for(i=0; i < graph->size; i++) {
    printf("Dest = %d Nhop = %d\n", i, rtable[i]);
  }

  free(rtable);

  return EXIT_SUCCESS;
}
