#include "contiki.h"
#include "net/rime.h"
#include "lib/list.h"
#include "lib/memb.h"
#include "lib/random.h"
#include "dev/button-sensor.h"
#include "dev/leds.h"
#include "contiki-conf.h"
#include "dev/adc-sensor.h"

#include <stdio.h>

#define CHANNEL 135


struct example_neighbor {
  struct example_neighbor *next;
  rimeaddr_t addr;
  uint16_t rank; 
  struct ctimer ctimer;
};
static rimeaddr_t to;
static uint16_t myrank=2;
#define NEIGHBOR_TIMEOUT 60 * CLOCK_SECOND
#define MAX_NEIGHBORS 16
LIST(neighbor_table);
MEMB(neighbor_mem, struct example_neighbor, MAX_NEIGHBORS);
/*---------------------------------------------------------------------------*/
PROCESS(broadcast_process, "Broadcast example");
PROCESS(multihop_process, "multihop example");
AUTOSTART_PROCESSES(&broadcast_process,&multihop_process);
/*---------------------------------------------------------------------------*/
static void
remove_neighbor(void *n)
{
  struct example_neighbor *e = n;
  list_remove(neighbor_table, e);
  memb_free(&neighbor_mem, e);
}
/*---------------------------------------------------------------------------*/
static void
received_announcement(struct announcement *a,
                      const rimeaddr_t *from,
		      uint16_t id, uint16_t value)
{
  struct example_neighbor *e;

     /* printf("Got announcement from %d.%d, id %d, value %d\n",
      from->u8[0], from->u8[1], id, value);*/

  for(e = list_head(neighbor_table); e != NULL; e = e->next) {
    if(rimeaddr_cmp(from, &e->addr)) {
      /* Our neighbor was found, so we update the timeout. */
      ctimer_set(&e->ctimer, NEIGHBOR_TIMEOUT, remove_neighbor, e);
      return;
    }
  }

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
static int8_t rssi;
static char s1[40];
static char s2[40];
static int rv;
  static struct sensors_sensor *sensor;
  static float sane = 0;
  static int dec;
  static float frac;
  static int dec1;
  int i;
   char *voltage;
static struct multihop_conn multihop;
static void
broadcast_recv1(struct broadcast_conn *c, const rimeaddr_t *from)
{  for(i=0;i<40;i++) s1[i]='\0';
  /*printf("broadcast message received from %d.%d\n",
         from->u8[0], from->u8[1]);
  printf(" message received '%s'\n", (char *)packetbuf_dataptr());*/
  voltage=(char *)packetbuf_dataptr();
  rssi=-packetbuf_attr(PACKETBUF_ATTR_RSSI);
//printf("rssi=%u\n",rssi);
sensor = sensors_find(ADC_SENSOR);
    if(sensor) {
    rv = sensor->value(ADC_SENSOR_TYPE_VDD);

      if(rv != -1) {
        sane = (rv * 3.75)/ 2047;
        dec = sane;
        frac = sane - dec;
        //printf("Supply=%d.%02u V (%d)\n", dec, (unsigned int)(frac*100), rv);
        /* Store rv temporarily in dec so we can use it for the battery */
        dec1=dec;
        dec = rv;
      }}
  sprintf(s1,"FE%u,%u,%u,%u,%d,%s,%uEE",from->u8[0],from->u8[1],rimeaddr_node_addr.u8[0],rimeaddr_node_addr.u8[1],rssi,voltage,dec1*100+(unsigned int)(frac*100));
   printf("%s\n",s1);
packetbuf_copyfrom(s1, sizeof(s1));
    
    multihop_send(&multihop, &to);
    //printf("send\n");
}
static void
broadcast_recv2(struct broadcast_conn *c, const rimeaddr_t *from)
{  for(i=0;i<40;i++) s2[i]='\0';
  /*printf("broadcast message received from %d.%d\n",
         from->u8[0], from->u8[1]);
  printf(" message received '%s'\n", (char *)packetbuf_dataptr());*/
  voltage=(char *)packetbuf_dataptr();
  rssi=-packetbuf_attr(PACKETBUF_ATTR_RSSI);
sensor = sensors_find(ADC_SENSOR);
    if(sensor) {
    rv = sensor->value(ADC_SENSOR_TYPE_VDD);

      if(rv != -1) {
        sane = (rv * 3.75)/ 2047;
        dec = sane;
        frac = sane - dec;
        //printf("Supply=%d.%02u V (%d)\n", dec, (unsigned int)(frac*100), rv);
        /* Store rv temporarily in dec so we can use it for the battery */
        dec1=dec;
        dec = rv;
      }}
  sprintf(s2,"FE%u,%u,%u,%u,%d,%s,%uEE",from->u8[0],from->u8[1],rimeaddr_node_addr.u8[0],rimeaddr_node_addr.u8[1],rssi,voltage,dec1*100+(unsigned int)(frac*100));
  printf("%s\n",s2);
packetbuf_copyfrom(s2, sizeof(s2));
    
    multihop_send(&multihop, &to);
}
static const struct broadcast_callbacks broadcast_call1 = {broadcast_recv1};
static const struct broadcast_callbacks broadcast_call2 = {broadcast_recv2};
static struct broadcast_conn broadcast1;
static struct broadcast_conn broadcast2;
/*---------------------------------------------------------------------------*/
/*
 * This function is called at the final recepient of the message.
 */
static void
recv(struct multihop_conn *c, const rimeaddr_t *sender,
     const rimeaddr_t *prevhop,
     uint8_t hops)
{
  //printf("multihop message received is '%s'\n", (char *)packetbuf_dataptr());
  //printf("prevhop is  %d.%d   ",prevhop->u8[0],prevhop->u8[1]);
  //printf("sender is  %d.%d\n",sender->u8[0],sender->u8[1]);
}

static rimeaddr_t *
forward(struct multihop_conn *c,
	const rimeaddr_t *originator, const rimeaddr_t *dest,
	const rimeaddr_t *prevhop, uint8_t hops)
{
  /* Find a random neighbor to send to. */
  int num, i;
  struct example_neighbor *n;

  while(list_length(neighbor_table) > 0) {
    num = random_rand() % list_length(neighbor_table);
    i = 0;
    for(n = list_head(neighbor_table); n != NULL && i != num; n = n->next) {
      ++i;
    }
    if(n != NULL&&n->rank<=myrank) {
	  //printf("nexthop's rank is %u,my rank is %u\n",n->rank,myrank); 
     // printf("%d.%d: Forwarding packet to %d.%d (%d in list), hops %d\n",
	     //rimeaddr_node_addr.u8[0], rimeaddr_node_addr.u8[1],
	     //n->addr.u8[0], n->addr.u8[1], num,
	    // packetbuf_attr(PACKETBUF_ATTR_HOPS));
      return &n->addr;
    }
  }
  //printf("%d.%d: did not find a neighbor to foward to\n",
	// rimeaddr_node_addr.u8[0], rimeaddr_node_addr.u8[1]);
  return NULL;
}
static const struct multihop_callbacks multihop_call = {recv, forward};

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(broadcast_process, ev, data)
{
  static struct etimer et;

  PROCESS_EXITHANDLER(broadcast_close(&broadcast1);)
  PROCESS_EXITHANDLER(broadcast_close(&broadcast2);)
  PROCESS_BEGIN();

  broadcast_open(&broadcast1, 129, &broadcast_call1);
  broadcast_open(&broadcast2, 127, &broadcast_call2);
  while(1) {

    PROCESS_YIELD();
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(multihop_process, ev, data)
{
  PROCESS_EXITHANDLER(multihop_close(&multihop);)
    
  PROCESS_BEGIN();

  memb_init(&neighbor_mem);
  list_init(neighbor_table);

  multihop_open(&multihop, CHANNEL, &multihop_call);

  announcement_register(&example_announcement,CHANNEL,received_announcement);

  announcement_set_value(&example_announcement, myrank);
    to.u8[0] = 143;
    to.u8[1] = 159;
  while(1) {//static struct etimer et;
    
    leds_on(LEDS_GREEN);
    PROCESS_YIELD();
    //etimer_set(&et, 0.5*CLOCK_SECOND);
    //PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
    

  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
