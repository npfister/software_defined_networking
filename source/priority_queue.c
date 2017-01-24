#include "priority_queue.h"
#include <stdio.h>
#include <stdlib.h>


priority_q_t *init_priority_q(int type, int max_size) {
  priority_q_t *pq;

  if ((pq = (priority_q_t*)malloc(sizeof(priority_q_t))) == NULL) {
    return NULL;
  }

  if(type != PQ_TYPE_MIN && type != PQ_TYPE_MAX) {
    printf("Error: Invalid priority queue type when trying to init priority queue\n");
    free(pq);
    return NULL;
  }

  pq->type = type;
  pq->max_size = max_size;
  pq->size = 0;

  if ((pq->A = (pq_node_t*)malloc(sizeof(pq_node_t*) * max_size)) == NULL) {
    printf("Error: Couldn't allocate space for the heap of size %d\n", max_size);
    free(pq);
    return NULL;
  }

  return pq;
}

void delete_priority_q(priority_q_t *pq) {
  if(pq != NULL) {
    if(pq->A != NULL) {
      free(pq->A);
    }
    free(pq);
  }
}

inline int get_right_leaf(int idx) {return 2*idx+2;}
inline int get_left_leaf(int idx) {return (2*idx)+1;}
inline int get_parent(int idx) {return (idx-1)/2;}

void up_heapify(priority_q_t *pq, int idx) {
  int p; 

  while (idx > 0) {
    p = get_parent(idx);

    if (pq->type == PQ_TYPE_MIN) {

      if (pq->A[p].value > pq->A[idx].value) {
        swap(pq, p, idx);
        idx = p;
      }
      else
        break;

    } else { // PQ_TYPE_MAX

      if (pq->A[p].value < pq->A[idx].value) {
        swap(pq, p, idx);
        idx = p;
      }
      else
        break;
    }
  }
}

void down_heapify(priority_q_t *pq,int idx) {

  int l;
  int r; 
  int target;

  while (idx*2 < pq->size) {
    l = get_left_leaf(idx);
    r = get_right_leaf(idx);

    if(pq->type == PQ_TYPE_MIN) {
      if (r < pq->size && (pq->A[r].value < pq->A[l].value))
        target = r;
      else if (l < pq->size) 
        target = l;
      else
        break;


      if (pq->A[target].value < pq->A[idx].value) {
        swap(pq, target, idx);
        idx = target;
      }
      else
        break;

    } else { //PQ_TYPE_MAX 

      if (r < pq->size && (pq->A[r].value > pq->A[l].value))
        target = r;
      else if (l < pq->size)
        target = l;  
      else
        break;
 
      if (pq->A[target].value > pq->A[idx].value) {
        swap(pq, target, idx);
        idx = target;
      }
      else
        break; 
    }
  }
}

void swap (priority_q_t *pq, int a, int b) {
  
  int id_temp;
  int value_temp;

  id_temp = pq->A[a].switch_id;
  value_temp = pq->A[a].value;

  pq->A[a].switch_id = pq->A[b].switch_id;
  pq->A[a].value = pq->A[b].value;

  pq->A[b].switch_id = id_temp;
  pq->A[b].value = value_temp;

}

int find_idx_by_id (priority_q_t *pq, int id) {
  int i;
  int idx = -1;

  for(i = 0; i < pq->size; i++) {
    if(pq->A[i].switch_id == id) {
      idx = i;
      break;
    }
  }

  return idx;
}

void insert_node (priority_q_t *pq, int id, int value) {

  if(pq->size == pq->max_size) {
    printf("Error: Priority queue size exceeded.  New node will not be added.\n");
    return;
  }

  pq->A[pq->size].switch_id = id;
  pq->A[pq->size].value = value;
  pq->size++;
  up_heapify(pq, pq->size-1);
}

pq_node_t pop_node (priority_q_t *pq) {
  pq_node_t temp;
  temp.switch_id = -1;
  temp.value = -1;

  if(pq->size == 0) {
    printf("Error: Tried popping from an empty priority queue\n");
    return temp;
  }

  temp = pq->A[0];
  swap(pq, 0, pq->size-1);
  pq->size--;
  down_heapify(pq, 0);

  return temp;
}

int get_priority(priority_q_t *pq, int id) {
  int idx;
  int p = 0;

  if((idx = find_idx_by_id(pq, id)) == -1) {
    printf("Error: Invalid id %d sent to get_priority\n", id);
  } else {
    p = pq->A[idx].value;
  }

  return p;
}

void update_priority(priority_q_t *pq, int id, int value) {
  int idx;

  if((idx = find_idx_by_id(pq, id)) == -1) {
    printf("Error: Invalid id %d sent to update_priority\n", id);
    return;
  } 

  if (value > pq->A[idx].value) {
    pq->A[idx].value = value;

    if (pq->type == PQ_TYPE_MIN) {
      down_heapify(pq, idx);
    } else { //PQ_TYPE_MAX
      up_heapify(pq, idx);
    }
  }
  else if (value < pq->A[idx].value) {
    pq->A[idx].value = value;

    if (pq->type == PQ_TYPE_MIN) {
      up_heapify(pq, idx);
    } else { //PQ_TYPE_MAX
      down_heapify(pq, idx);
    }
  }
}

int is_queued(priority_q_t *pq, int id) {
  int i;

  for(i = 0; i < pq->size; i++) {
    if(pq->A[i].switch_id == id)
      return 1;
  }
  return 0;
}

void print_pq(priority_q_t *pq) {
  int i;

  printf("\n----- Printing Priority Queue -----\nSize:%d\n", pq->size);

  for(i = 0; i < pq->size; i++) {
    printf("Location: %d  id = %d value = %d\n", i, pq->A[i].switch_id, pq->A[i].value);
  }
}
