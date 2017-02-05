#ifndef PACKET_TYPES_H
#define PACKET_TYPES_H


#define K_SEC     5 //how often switches send keep alive messages
#define M_MISSES  3 //number of missed keep alive messages before neighbors report a switch as dead
#define SW_DIED (K_SEC * M_MISSES) //time after which a switch is dead and a topology update should be sent

#define MAX_NEIGHBORS 16
#define MAX_SWITCHES 32

//enum for packet types
typedef enum packet_types pack_t;
enum packet_types{
  REGISTER_REQUEST = 0,
  // s->c 
  // controller learns host/port info
  REGISTER_RESPONSE = 1,
  // c->s 
  // switch learns list of neighbors, its id, 
  // active flag, and host/port info
  KEEP_ALIVE = 2,
  // s->s 
  // mark sender as active
  ROUTE_UPDATE = 3,
  // c->s
  // contains an updated routing table
  TOPOLOGY_UPDATE = 4
  // s->c
  // sends list of alive neighbors
};

typedef struct { 
  unsigned char type;
  unsigned char switch_id;
  unsigned int host; 
  unsigned int port;
}register_req_t;

typedef struct { 
  unsigned char type;
  char neighbor_id[MAX_NEIGHBORS]; // -1 -> invalid entry
  unsigned char active_flag[MAX_NEIGHBORS];
  unsigned char host[MAX_NEIGHBORS];
  unsigned char port[MAX_NEIGHBORS];
}register_resp_t;

typedef struct { 
  unsigned char type;
  unsigned char sender_id;
}keep_alive_t;

typedef struct { 
  unsigned char type;
  char route_table[MAX_SWITCHES];
}route_update_t;

typedef struct { 
  unsigned char type;
  unsigned char sender_id;  
  char neighbor_id[MAX_NEIGHBORS]; // -1 -> invalid entry
  unsigned char active_flag[MAX_NEIGHBORS];
}topology_update_t;

#endif //PACKED_TYPES_H
