/*
 *  Created by: John Skubic
 *
 *  Header for graph implementation
 */

#ifndef GRAPH_H
#define GRAPH_H

typedef struct edge_t {
  int vertex_conn;
  int bw;
  int delay;
  int active;
  struct edge_t *next;
} edge_t;

typedef struct graph_t {
  edge_t *adj_list;
  int size;
} graph_t;

typedef struct routing_table_t {

} routing_table_t;


graph_t *create_graph_from_file(char *filename);
void print_graph(graph_t *graph);
void delete_graph(edge_t *graph);
void activate_link(graph_t *graph, int vertex_a, int vertex_b);
void deactivate_link(graph_t *graph, int vertex_a, int vertex_b);
void activate_switch(graph_t *graph, int vertex);
void deactivate_switch(graph_t *graph, int vertex);
edge_t *find_link(graph_t *graph, int vertex_a, int vertex_b);
void calc_routing_table(graph_t *graph, int s_vertex, routing_table_t *routing_table);
void calc_widest_path_tree(graph_t *graph, int s_vertex, int **widths_arr, int **prev_arr);

int *build_routing_table(graph_t *graph, int id);

#endif //GRAPH_H
