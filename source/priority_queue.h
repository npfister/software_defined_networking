#ifndef PRIORITY_QUEUE_H
#define PRIORITY_QUEUE_H

#ifndef PQ_TYPE_MIN
#define PQ_TYPE_MIN 1
#endif

#ifndef PQ_TYPE_MAX
#define PQ_TYPE_MAX 0
#endif

#define INF 0x7fffffff

typedef struct pq_node_t {
  int switch_id; 
  int value;
} pq_node_t;

typedef struct priority_q_t {
  int type; 
  int size;
  int max_size;
  pq_node_t *A; 
} priority_q_t;

priority_q_t *init_priority_q(int type, int size);
void insert_node (priority_q_t *pq, int id, int value);
void update_priority(priority_q_t *pq, int id, int value);
int get_priority(priority_q_t *pq, int id);
int is_queued(priority_q_t *pq, int id);
pq_node_t pop_node (priority_q_t *pq);

void delete_priority_q(priority_q_t *pq);
int get_right_leaf(int idx);
int get_left_leaf(int idx);
int get_parent(int idx);
void up_heapify(priority_q_t *pq, int idx);
void down_heapify(priority_q_t *pq, int idx);
void swap (priority_q_t *pq, int a, int b);
int find_idx_by_id (priority_q_t *pq, int id);
void print_pq(priority_q_t *pq);

#endif //PRIORITY_QUEUE_H
