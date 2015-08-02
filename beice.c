
#include "contiki.h"
#include "net/rime.h"
#include "random.h"
#include "contiki-conf.h"
#include "dev/adc-sensor.h"
#include "dev/button-sensor.h"
#include "dev/cc2530-rf.h"
#include "dev/leds.h"

#include <stdio.h>
/*---------------------------------------------------------------------------*/
PROCESS(example_broadcast_process, "Broadcast example");
AUTOSTART_PROCESSES(&example_broadcast_process);
/*---------------------------------------------------------------------------*/
static char rssi;
static void
broadcast_recv(struct broadcast_conn *c, const rimeaddr_t *from)
{        leds_on(LEDS_RED);
         printf("broadcast message received from %d.%d: '%s'\n",
         from->u8[0], from->u8[1], (char *)packetbuf_dataptr());
         rssi=-packetbuf_attr(PACKETBUF_ATTR_RSSI);
         printf("rssi=%u\n",rssi);
}
static const struct broadcast_callbacks broadcast_call = {broadcast_recv};
static struct broadcast_conn broadcast;
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(example_broadcast_process, ev, data)
{
  static struct etimer et;
static int rv;
  static struct sensors_sensor *sensor;
  static float sane = 0;
  static int dec;
  static float frac;
  static int dec1;
  static char s[20];
  PROCESS_EXITHANDLER(broadcast_close(&broadcast);)

  PROCESS_BEGIN();

  broadcast_open(&broadcast, 129, &broadcast_call); 

  while(1) {
     leds_on(LEDS_RED);
    /* Delay 2-4 seconds */
    etimer_set(&et, CLOCK_SECOND * 4 + random_rand() % (CLOCK_SECOND * 4));
    //etimer_set(&et, CLOCK_SECOND * 4 );
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
sensor = sensors_find(ADC_SENSOR);
    if(sensor) {
    rv = sensor->value(ADC_SENSOR_TYPE_VDD);

      if(rv != -1) {
        sane =( rv * 3.16 )/ 2047;
        dec = sane;
        frac = sane - dec;
        //printf("Supply=%d.%02u V (%d)\n", dec, (unsigned int)(frac*100), rv);
        /* Store rv temporarily in dec so we can use it for the battery */
        dec1=dec;
        dec = rv;
      }
    sprintf(s,"%u",dec1*100+(unsigned int)(frac*100));
    packetbuf_copyfrom(s, sizeof(s));
    broadcast_send(&broadcast);
    //printf("broadcast message sent\n");
  }
}
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
