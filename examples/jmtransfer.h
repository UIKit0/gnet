/* Jungle Monkey Transfer header
 * Copyright (C) 2000  David Helder
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/*

  jmtransfer is the header file for jmsend and jmreceive, utilites for
  transmiting files reliable via multicast.

  Transfering is done using the Jungle Monkey Reliable Multicast
  Protocol (JRMP).  The server sends out packets at some predetermined
  rate.  Client receive packets.  Client know the rate of transfer, so
  can determine if a packet they receive was dropped.  If it was, it
  sends a retransmitt request to the channel.

  Problems:
  - Not really scalable, but that's okay for now

  TODO:
  - Attempt local repair first
  - Cleanup logic for timeouts

*/


#ifndef _JMTRANSFER_H
#define _JMTRANSFER_H

#include <glib.h>
#include <sys/time.h>

#define JRMP_MAGIC 	"JRMP"
#define JRMP_SEND  	0
#define JRMP_REQUEST	1
/*#define JRMP_DATA_LENGTH(packet_length) \
           (packet_length - &(((jmtransfer_packet*) 0)->data))
*/
/*  #define JRMP_HEADER_SIZE (&(((jmtransfer_packet*) 0)->data)) */


typedef struct _jmtransfer_header
{
  gchar magic[4];		/* Should be "JRMP" */

  guint32 seq_number;		/* Sequence number */

  guint16 packet_type;		/* Packet type (send or transfer) */
  guint16 length;		/* Length of the data in bytes in send, or the
				   number of packets being requested in a request
				*/
  gchar data[0];

} jmtransfer_header;


#define mytimernormalize(tvp)						\
       do {								\
           (tvp)->tv_sec  += (tvp)->tv_usec / 1000000; 			\
	   (tvp)->tv_usec %= 1000000;					\
       } while (0)

#define mytimeraddusec(tvp, usec)					\
       do {								\
           (tvp)->tv_usec += usec;					\
           if ((tvp)->tv_usec < 0)					\
           {								\
             (tvp)->tv_sec -= (-((tvp)->tv_usec / 1000000) + 1);	\
             (tvp)->tv_usec = (1000000 + ((tvp)->tv_usec % 1000000));	\
           }								\
           mytimernormalize(tvp);					\
       } while (0)

#define mytimerusec(tvp) (((tvp)->tv_sec * 1000000) + (tvp)->tv_usec)

/* if negative, round up to zero */
#define mytimermakepos(tvp) ((tvp)->tv_sec < 0)				\
                              ?((tvp)->tv_sec = 0, (tvp)->tv_usec = 0)	\
			      :(0)


#define JRMP_EVENT_SEND_NEXT	0
#define JRMP_EVENT_SEND_REPAIR	1
#define JRMP_EVENT_DONE		2
#define JRMP_EVENT_REQUEST	3


typedef struct _jmtransfer_event
{
  gint type;			/* Type of event		 */
  guint32 seq_number;		/* Packet sequence number	 */
  struct timeval time;  	/* Amount of time from now event occurs */
  gint retry_number;		/* Retry request number */
} jmtransfer_event;


/**

   Event queues are used by both the sender and the receiver.

*/

#define EVENT_QUEUE_TOP	((event_queue != NULL)				\
			 ?((jmtransfer_event*) event_queue->data)	\
			 :(NULL))

extern GList* event_queue;

jmtransfer_event* schedule_event (gint type, guint32 seq_number, long usec);

gint has_event_number (guint32 seq_number);
void delete_event_number (guint32 seq_number);

gint has_event_type (gint type);
void delete_event_type (gint type);


#endif /* _JMTRANSFER_H */
