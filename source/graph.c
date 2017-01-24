/*
 * Created by: John Skubic
 * 
 * Graph implementation using adjacency list.
 * Includes widest path algorithm 
 */

#include "graph.h"
#include <stdio.h>
#include <stdlib.h>
#include "priority_queue.h"

graph_t *create_graph_from_file(char *filename) {
  FILE *fptr;
  int vertex_a, vertex_b, bw, delay;
  int num_vertices;
  int i;
  edge_t *adj_list = NULL;
  edge_t *temp = NULL;
  graph_t *graph = NULL;

  if((fptr = fopen(filename, "r")) == NULL) {
    return NULL;
  }

  fscanf(fptr, "%d", &num_vertices);

  // Allocate Space for Adjacency List
  if((adj_list = (edge_t*)malloc(num_vertices * sizeof(edge_t))) == NULL) {
    return NULL;
  }

  // Initialize Adjacency List
  for(i = 0; i < num_vertices; i++) {
    adj_list[i].vertex_conn = 0;
    adj_list[i].bw = 0; 
    adj_list[i].delay = 0;
    adj_list[i].active = 1; //TODO: ONLY FOR DEBUG
    adj_list[i].next = NULL; 
  }
  
  while (fscanf(fptr, "%d %d %d %d", &vertex_a, &vertex_b, &bw, &delay) != EOF) {
    
    if(vertex_b < vertex_a) {
      //swap so vertex a is always smaller 
      vertex_a = vertex_a ^ vertex_b;
      vertex_b = vertex_a ^ vertex_b;
      vertex_a = vertex_a ^ vertex_b;
    }
  
    vertex_a -= 1;
    vertex_b -= 1;

    if((temp = (edge_t*)malloc(sizeof(edge_t))) == NULL) {
      return NULL;
    }

    temp->vertex_conn = vertex_b;
    temp->bw = bw;
    temp->delay = delay;
    temp->active = 1; //TODO: ONLY FOR DEBUG
    temp->next = adj_list[vertex_a].next;
    adj_list[vertex_a].next = temp;
    
  } 

  if((graph = (graph_t*)malloc(sizeof(graph_t))) == NULL) {
    return NULL;
  }

  graph->adj_list = adj_list;
  graph->size = num_vertices;

  return graph; 
}

edge_t *find_link(graph_t *graph, int vertex_a, int vertex_b){
  edge_t *curr_ptr = NULL;

  if(vertex_b < vertex_a) {
    //swap so vertex a is always smaller 
    vertex_a = vertex_a ^ vertex_b;
    vertex_b = vertex_a ^ vertex_b;
    vertex_a = vertex_a ^ vertex_b;
  }

  curr_ptr = graph->adj_list[vertex_a].next;
  
  while(curr_ptr != NULL && curr_ptr->vertex_conn != vertex_b) {
    curr_ptr = curr_ptr->next;
  } 

  return curr_ptr;
}

void activate_link(graph_t *graph, int vertex_a, int vertex_b){
  edge_t *link;

  link = find_link(graph, vertex_a, vertex_b);

  if(link == NULL) {
    printf("Error: Trying to activate invalid link between %0d - %0d\n", vertex_a, vertex_b);
  } else {
    link->active = 1;
  }
}


void deactivate_link(graph_t *graph, int vertex_a, int vertex_b){
  edge_t *link;

  link = find_link(graph, vertex_a, vertex_b);

  if(link == NULL) {
    printf("Error: Trying to deactivate invalid link between %0d - %0d\n", vertex_a, vertex_b);
  } else {
    link->active = 0;
  }
}

void activate_switch(graph_t *graph, int vertex) {
  if(vertex >= graph->size) {
    printf("Error: Trying to activate invalid vertex %0d\n", vertex);
  } else {
    graph->adj_list[vertex].active = 1;
  }
}

void deactivate_switch(graph_t *graph, int vertex) {
  if(vertex >= graph->size) {
    printf("Error: Trying to activate invalid vertex %0d\n", vertex);
  } else {
    graph->adj_list[vertex].active = 0;
  }
}

void print_graph(graph_t *graph) {
  int i;
  edge_t *curr_ptr;
 
  printf("---------- GRAPH PRINTOUT ----------\n");
  printf("Number of Vertices: %d\n", graph->size);

  for(i = 0; i < graph->size; i++) {
    printf("Switch %d active: %d\n", i, graph->adj_list[i].active);
  }
 
  for(i = 0; i < graph->size; i++) {
    curr_ptr = graph->adj_list[i].next;

    while(curr_ptr != NULL) {
      printf("Link between %d - %d bw: %d delay %d active: %d\n", i, curr_ptr->vertex_conn, curr_ptr->bw, curr_ptr->delay, curr_ptr->active);
      curr_ptr = curr_ptr->next;
    }
  }  

  printf("---------- END GRAPH PRINTOUT ----------\n");
}


void calc_widest_path_tree(graph_t *graph, int s_vertex, int **widths_arr, int **prev_arr) {
  
  int i;
  priority_q_t *pq;
  pq_node_t temp;
  edge_t *curr_ptr;
  int curr_w;
  int new_w;
  int *widths;
  int *prev;

  i = 0;

  if((pq = init_priority_q(PQ_TYPE_MAX, graph->size)) == NULL) {
    printf("Error: Couldn't allocate space for a priority queue\n");
    return;
  }

  if((widths = (int*)malloc(sizeof(int)*graph->size)) == NULL) {
    printf("Error: Couldn't allocate space for widths\n");
    free(pq);
    return;
  }

  if((prev = (int*)malloc(sizeof(int)*graph->size)) == NULL) {
    printf("Error: Couldn't allocate space for widths\n");
    free(pq);
    free(widths);
    return;
  }

  for (i = 0; i < graph->size; i++) {
    widths[i] = 0;
    prev[i] = -1;
    if(graph->adj_list[i].active)
      insert_node(pq, i, 0);
  }

  update_priority(pq, s_vertex, INF);

  while(pq->size > 0) {
    temp = pop_node(pq);
    curr_ptr = graph->adj_list[temp.switch_id].next;
  
    //printf("\n--- New Iteration --- \nPicked Node:%d BW:%d\n", temp.switch_id+1, temp.value);
  
    while(curr_ptr != NULL) {
      if((graph->adj_list[curr_ptr->vertex_conn].active) && curr_ptr->active && is_queued(pq, curr_ptr->vertex_conn)) {
        curr_w = get_priority(pq, curr_ptr->vertex_conn);
        
        //printf("Found Edge With: %d\n", curr_ptr->vertex_conn+1);

        if(curr_ptr->bw < temp.value) 
          new_w = curr_ptr->bw;
        else
          new_w = temp.value;

        if(curr_w < new_w) {
          //printf("Updating with new val %d\n", new_w);
          update_priority(pq, curr_ptr->vertex_conn, new_w);
          prev[curr_ptr->vertex_conn] = temp.switch_id;
        }
      }
      curr_ptr = curr_ptr->next;
    }

    for(i = 0; i < temp.switch_id; i++) {
      curr_ptr = graph->adj_list[i].next;
      if(graph->adj_list[i].active && is_queued(pq, i)) {
        while(curr_ptr != NULL) {
          if((curr_ptr->vertex_conn == temp.switch_id) && curr_ptr->active) {
           
            //printf("Found Edge With: %d\n", i+1);
 
            curr_w = get_priority(pq, i);

            if(curr_ptr->bw < temp.value) 
              new_w = curr_ptr->bw;
            else
              new_w = temp.value;

            if(curr_w < new_w) {
              //printf("Updating with new val %d\n", new_w);
              update_priority(pq, i, new_w);
              prev[i] = temp.switch_id;
            }

          }
          curr_ptr = curr_ptr->next;
        }
      }
    }

    // Update Information for Popped Node
    widths[temp.switch_id] = temp.value;
   
  }

  //return width and prev
  *widths_arr = widths;
  *prev_arr = prev;
}

int *build_routing_table(graph_t *graph, int id) {
  int i;
  int curr;
  int *width;
  int *prev;
  int *table;

  if((table = (int*)malloc(sizeof(int)*graph->size)) == NULL) {
    printf("Error: couldn't allocate space for routing table for id %d\n", id);
    return NULL;
  }

  calc_widest_path_tree(graph, id, &width, &prev);

  if(width == NULL || prev == NULL) {
    printf("Error: Couldn't perform widest path algorithm when building routing table\n");
    return NULL;
  }

  for(i=0; i < graph->size; i++) {
    if(i == id)
      table[i] = id;
    else {
      curr = i;
      while(prev[curr] != id && prev[curr] != -1) {
        curr = prev[curr];
      }
      if(prev[curr] == -1) // Node is unreachable
        table[i] = -1;
      else
        table[i] = curr;
    }  
  }
 
  free(width);
  free(prev);
  
  return table;
}

