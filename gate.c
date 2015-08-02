#include "contiki.h"
#include "net/rime.h"
#include "lib/list.h"
#include "lib/memb.h"
#include "lib/random.h"
#include "dev/button-sensor.h"
#include "dev/leds.h"
//#include "dev/cc2530-rf.h"
#include <stdio.h>

#define CHANNEL 135
//#define CHANNEL2 137

struct example_neighbor {
  struct example_neighbor *next;
  rimeaddr_t addr;
  uint16_t rank; 
  struct ctimer ctimer;
};
static uint16_t myrank=0;
#define NEIGHBOR_TIMEOUT 60 * CLOCK_SECOND
#define MAX_NEIGHBORS 16
LIST(neighbor_table);
MEMB(neighbor_mem, struct example_neighbor, MAX_NEIGHBORS);
/*---------------------------------------------------------------------------*/
PROCESS(example_multihop_process, "multihop example");
//PROCESS(example_multihop_process2, "multihop example");
//AUTOSTART_PROCESSES(&example_multihop_process1,&example_multihop_process2);
AUTOSTART_PROCESSES(&example_multihop_process);
/*---------------------------------------------------------------------------*/
/*
 * This function is called by the ctimer present in each neighbor
 * table entry. The function removes the neighbor from the table
 * because it has become too old.
 */
static void
remove_neighbor(void *n)
{
  struct example_neighbor *e = n;
  list_remove(neighbor_table, e);
  memb_free(&neighbor_mem, e);
}
/*---------------------------------------------------------------------------*/
/*
 * This function is called when an incoming announcement arrives. The
 * function checks the neighbor table to see if the neighbor is
 * already present in the list. If the neighbor is not present in the
 * list, a new neighbor table entry is allocated and is added to the
 * neighbor table.
 */
static void
received_announcement(struct announcement *a,
                      const rimeaddr_t *from,
		      uint16_t id, uint16_t value)
{
  struct example_neighbor *e;

  /* We received an announcement from a neighbor so we need to update
     the neighbor list, or add a new entry to the table. */
  for(e = list_head(neighbor_table); e != NULL; e = e->next) {
    if(rimeaddr_cmp(from, &e->addr)) {
      /* Our neighbor was found, so we update the timeout. */
      ctimer_set(&e->ctimer, NEIGHBOR_TIMEOUT, remove_neighbor, e);
      return;
    }
  }

  /* The neighbor was not found in the list, so we add a new entry by
     allocating memory from the neighbor_mem pool, fill in the
     necessary fields, and add it to the list. */
  e = memb_alloc(&neighbor_mem);
  if(e != NULL) {
    rimeaddr_copy(&e->addr, from);
    e->rank=value;        
    list_add(neighbor_table, e);
    ctimer_set(&e->ctimer, NEIGHBOR_TIMEOUT, remove_neighbor, e);
  }
}
static struct announcement example_announcement;
/*---------------------------------------------------------------------------*/
static char *s;
static void
recv(struct multihop_conn *c, const rimeaddr_t *sender,
     const rimeaddr_t *prevhop,
     uint8_t hops)
{
  printf("%s\n", (char *)packetbuf_dataptr());
  //s=(char *)packetbuf_dataptr();
  //printf("prevhop is  %d.%d \n  ",prevhop->u8[0],prevhop->u8[1]);
  //printf("sender is  %d.%d\n",sender->u8[0],sender->u8[1]);
}
/*
 * This function is called to forward a packet. The function picks a
 * random neighbor from the neighbor list and returns its address. The
 * multihop layer sends the packet to this address. If no neighbor is
 * found, the function returns NULL to signal to the multihop layer
 * that the packet should be dropped.
 */
/*static rimeaddr_t *
forward(struct multihop_conn *c,
	const rimeaddr_t *originator, const rimeaddr_t *dest,
	const rimeaddr_t *prevhop, uint8_t hops)
{
 
  /*int num, i;
  struct example_neighbor *n;

  while(list_length(neighbor_table) > 0) {
    num = random_rand() % list_length(neighbor_table);
    i = 0;
    for(n = list_head(neighbor_table); n != NULL && i != num; n = n->next) {
      ++i;
    }
    if(n != NULL&&n->rank<=myrank) {
	  //printf("nexthop's rank is %u,my rank is %u\n",n->rank,myrank); 
      printf("%d.%d: Forwarding packet to %d.%d (%d in list), hops %d\n",
	     rimeaddr_node_addr.u8[0], rimeaddr_node_addr.u8[1],
	     n->addr.u8[0], n->addr.u8[1], num,
	     packetbuf_attr(PACKETBUF_ATTR_HOPS));
      return &n->addr;
    }
  }
  //printf("%d.%d: did not find a neighbor to foward to\n",
	// rimeaddr_node_addr.u8[0], rimeaddr_node_addr.u8[1]);
  return NULL;

} */
static const struct multihop_callbacks multihop_call = {recv, NULL};
//static const struct multihop_callbacks multihop_call2 = {recv, NULL};
static struct multihop_conn multihop;
//static struct multihop_conn multihop2;
static struct etimer et;
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(example_multihop_process, ev, data)
{
  PROCESS_EXITHANDLER(multihop_close(&multihop);)
    
  PROCESS_BEGIN();
  
  memb_init(&neighbor_mem);

  list_init(neighbor_table);

  multihop_open(&multihop, CHANNEL, &multihop_call);

  announcement_register(&example_announcement,CHANNEL,received_announcement);
 
  announcement_set_value(&example_announcement, myrank);
  while(1){
  PROCESS_YIELD();
  printf("multihop start!\n");
  }
  PROCESS_END();
}
