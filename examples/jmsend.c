/* Jungle Monkey Reliable Multicast Sender
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <sys/stat.h>

#include <glib.h>
#include <gnet/gnet.h>
#include "jmtransfer.h"


#define BUFFER_SIZE 	1472
#define PAYLOAD_SIZE  	(BUFFER_SIZE - sizeof(jmtransfer_header))
#define LINGER_SCALE 	50
#define DEBUG		0
#define LOG_FILE	"jmsend.log"

static void usage (FILE* file, int exit_status);
static int file_size(FILE* f);

static void send_packet(guint32 seq_number);



GList* event_queue = NULL; 

static gchar* program_name = NULL;
#if DEBUG
static FILE* log_file = NULL;
#endif

static GInetAddr* ia = NULL;
static GMcastSocket* msocket = NULL;
static GUdpPacket* packet_in = NULL;
static GUdpPacket* packet_out = NULL;
static gchar buffer[BUFFER_SIZE] = JRMP_MAGIC;
static jmtransfer_header* header = (jmtransfer_header*) buffer;

static gint file_length;
static gchar* memin;



int
main(int argc, char** argv)
{
  gchar* addr_name;
  gint port;
  gint kbs;
  gchar* file_name;

  FILE* fin;
  struct timeval send_time;
  gint usec_between_packets;
  guint32 current_seq_number = 0;
  guint32 last_seq_number;
  gint mtu;

  program_name = argv[0];

  /* Parse arguments */
  if (argc != 5)
    usage (stderr, EXIT_FAILURE);

  addr_name = argv[1];
  port = atoi(argv[2]);
  kbs = atoi(argv[3]);
  file_name = argv[4];

  /* Open the log file */
#if DEBUG
  log_file = fopen(LOG_FILE, "w");
  if (log_file == NULL) log_file = stderr;
#endif

#if DEBUG
  fprintf (log_file, "arguments = %s, %d, %d, %s\n", 
	   addr_name, port, kbs, file_name);
#endif

  /* Create the address */
  ia = gnet_inetaddr_new (addr_name, port);
  if (ia == NULL)
    {
      perror ("Could not find internet address");
      exit (EXIT_FAILURE);
    }

  /* Create the multicast socket for receiving */
  msocket = gnet_mcast_socket_inetaddr_new(ia);
  g_assert (msocket != NULL);

  /* Get the MTU */
  mtu = gnet_udp_socket_get_MTU((GUdpSocket*) msocket);
  if (mtu == -1)
    perror ("can't get MTU");
#if DEBUG
  else
    fprintf (log_file, "MTU is %d, sizeof(udphdr) is %d, sizeof(ip) is %d\n", 
	     mtu, sizeof(struct udphdr), sizeof(struct iphdr));
#endif

  /* Join the address */
  if (gnet_mcast_socket_join_group (msocket, ia) != 0)
    {
      perror ("Could not join multicast group");
      exit (EXIT_FAILURE);
    }

  /* Set TTL to unrestricted  */
  if (gnet_udp_socket_set_mcast_ttl((GUdpSocket*) msocket, 255) != 0)
    {
      perror ("Could not set TTL");
      exit (EXIT_FAILURE);
    }

  /* Create packet for sending */
  packet_out = gnet_udp_packet_send_new (buffer, sizeof(buffer), ia);
  g_assert (packet_out != NULL);

  /* Create packet for receiving */
  packet_in = gnet_udp_packet_receive_new (buffer, sizeof(buffer));
  g_assert (packet_in != NULL);

  /* Open the file to read from.  We don't allow "-" because we want
     to mmap the file. */
  if ((fin = fopen (file_name, "r")) == NULL)
    {
      perror ("Could not open file");
      exit (EXIT_FAILURE);
    }

  /* Get the length of the file */
  file_length = file_size (fin);
  g_assert (file_length >= 0);

  /* Map the file to memory */
  memin = mmap(NULL, file_length, PROT_READ, MAP_PRIVATE, fileno(fin), 0);
  if ((memin == NULL) || (int) memin == -1)
    {
      perror ("Memory map failed");
      exit (EXIT_FAILURE);
    }

  /* Calculate number of usec between sends */
  usec_between_packets = (1000000.0 * (double) BUFFER_SIZE) / 
    (((double) kbs / 8.0) * 1024.0);

  /* Calculate last sequence number */
  last_seq_number = ceil (file_length / (double) PAYLOAD_SIZE);

#if DEBUG
  fprintf (log_file, "usec between packets: %d\n", usec_between_packets);
#endif

  /* Add an event for the first packet send */
  schedule_event (JRMP_EVENT_SEND_NEXT, 0, 0);

  /* Initialize send time - the time a packet will be sent */
  g_assert (gettimeofday(&send_time, NULL) == 0);

  
  /* Loop receiving packets and writing them to the output file */
  while (1)
    {
      fd_set read_fdset;
      struct timeval timeout, timeofday;
      struct timeval* timeoutp = NULL;
      int num_fd_ready;


      /* *****************************************/
      /* Wait until either a packet or an event */
      
      /* Set up the fd set */
      FD_ZERO (&read_fdset);
      FD_SET (msocket->sockfd, &read_fdset);

      /* Get next event */
      if (event_queue != NULL)
	{
	  timeout = EVENT_QUEUE_TOP->time;
	  g_assert (gettimeofday(&timeofday, NULL) == 0);

	  timersub (&timeout, &timeofday, &timeout);
	  mytimermakepos(&timeout);

	  timeoutp = &timeout;

#if DEBUG
	  fprintf (log_file, "next event in %ld,%ld\n", 
		   timeout.tv_sec, timeout.tv_usec);
#endif
	}
      else
	timeoutp = NULL;

#if DEBUG
      fprintf (log_file, "select...");
#endif

      /* Wait for something to happen */
      if ((num_fd_ready = select (msocket->sockfd + 1, &read_fdset, NULL, NULL, timeoutp)) 
	  == -1)
	{
	  perror ("Select error");
	  break;
	}

#if DEBUG
      fprintf (log_file, "done\n");
#endif


      /* Get the timeofday - we'll need it later */
      g_assert (gettimeofday(&timeofday, NULL) == 0);
#if DEBUG
      fprintf (log_file, "timeofday = %ld,%ld\n", timeofday.tv_sec, timeofday.tv_usec);
#endif


      /* **************************************** */
      /* 
	 if we have a packet
	 if its a request
	 if its a request for something we didn't just send
	 schedule a send event

	 if we have an event
	 if it is a send
	 send it
      */


      /* We have a packet */
      if (FD_ISSET(msocket->sockfd, &read_fdset))
	{
	  int bytes_read;
	  guint32 seq_number;
	  guint16 packet_type;
	  guint16 length;

	  /* Read as many packets as possible */
	  do
	    {
#if DEBUG
	      fprintf (log_file, "have packet\n");
#endif

	      /* Read the packet */
	      bytes_read = gnet_mcast_socket_receive (msocket, packet_in);
	      if (bytes_read < 0)
		{
		  perror ("Reading from multicast socket");
		}

	      /* Make sure the packet is long enough */
	      if (bytes_read < sizeof(jmtransfer_header))
		continue;

	      /* Make sure the packet has the right magic */ 
	      if (strncmp (header->magic, JRMP_MAGIC, sizeof(JRMP_MAGIC) - 1) != 0)
		continue;

	      /* Read the fields */
	      seq_number = g_ntohl (header->seq_number);
	      packet_type = g_ntohs (header->packet_type);
	      length = g_ntohs (header->length);

	      /* If it's not a request, ignore it */
	      if (packet_type != JRMP_REQUEST)
		continue;

	      /* If a resend isn't scheduled already, schedule one immediately.  Scheduling
		 it in the past ensures it will be scheduled immediately. */
	      if (!has_event_number(seq_number))
		{
#if DEBUG
		  fprintf (log_file, "resend request %d\n", seq_number);
#endif
		  schedule_event (JRMP_EVENT_SEND_REPAIR, seq_number, -100000);
		}

	      /* If we're supposed to quit, reschedule that */
	      if (has_event_type(JRMP_EVENT_DONE))
		{
#if DEBUG
		  fprintf (log_file, "rescheduling DONE\n");
#endif

		  delete_event_type (JRMP_EVENT_DONE);
		  schedule_event (JRMP_EVENT_DONE, 0, 
				  usec_between_packets * LINGER_SCALE);
		}
	    } while (gnet_udp_socket_has_packet((GUdpSocket*) msocket));
	}


      /* Process event until there are none left */
      while (EVENT_QUEUE_TOP != NULL &&
	     !timercmp (&timeofday, &EVENT_QUEUE_TOP->time, <))
	{
	  jmtransfer_event* event = EVENT_QUEUE_TOP;
	  g_assert (event != NULL);
	  event_queue = g_list_remove (event_queue, event);

#if DEBUG
 	  fprintf (log_file, "have event\n");
#endif
	      
	  /* If the event is a send, send it */
	  if (event->type == JRMP_EVENT_SEND_NEXT)
	    {
	      current_seq_number = event->seq_number;

	      /* Send the packet */
#if DEBUG
	      fprintf (log_file, "SEND %d\n", event->seq_number);
#endif
	      send_packet (event->seq_number);

	      /* Check if theres another packet */
	      if ((event->seq_number + 1) != last_seq_number)
		{
		  /* Get the new timeofday */
		  struct timeval new_time;
		  gint time_usec;

		  /* Increment send time */
		  mytimeraddusec (&send_time, usec_between_packets);

		  /* Calculate time for next packet */
		  g_assert (gettimeofday(&new_time, NULL) == 0);
		  timersub (&send_time, &new_time, &new_time);
		  time_usec = mytimerusec (&new_time);

		  /* This is kind of backwards since our scheduler
		     undoes and redoes this. */

#if DEBUG
		  fprintf (log_file, "next send in %d usec\n",
			   time_usec);
#endif

		  /* Schedule the next event */
		  schedule_event (JRMP_EVENT_SEND_NEXT,
				  event->seq_number + 1,
				  time_usec);
		}
	      else
		{
		  jmtransfer_event* new_event;

		  /* Linger for a bit in case clients need more repairs */
		  new_event = schedule_event (JRMP_EVENT_DONE,
					      0, usec_between_packets * LINGER_SCALE);
		}
	    }

	  /* If the event is a repair, send it */
	  else if (event->type == JRMP_EVENT_SEND_REPAIR)
	    {
#if DEBUG
	      fprintf (log_file, "send repair %d?, current seq number = %d\n", 
		       event->seq_number, current_seq_number);
#endif

	      /* Make sure it's a request for something in the past,
		 so clients don't get ahead of us */
	      if (event->seq_number <= current_seq_number)
		{
#if DEBUG
		  fprintf (log_file, "send repair %d\n", event->seq_number);
#endif
		  send_packet (event->seq_number);
		}
	    }

	  /* If we're done, break */
	  else if (event->type == JRMP_EVENT_DONE)
	    {
	      /* If there are still events on the queue,
		 schedule another done.  Otherwise, quit */
	      if (event_queue == NULL)
		goto finish;

#if DEBUG
	      fprintf (log_file, "schedule done\n");
#endif

	      schedule_event (JRMP_EVENT_DONE, 0, 
			      usec_between_packets * LINGER_SCALE);
	    }

	  /* Delete the event */
	  g_free(event);
	}
    }

 finish:

  /* Unmap the memory */
  if (munmap(memin, file_length) != 0)
    {
      perror ("Unmapping memory");
      exit (EXIT_FAILURE);
    }

  /* Close the file */
  fclose (fin);

  exit (EXIT_SUCCESS);
}


void
usage (FILE* file, int exit_status)
{
  fprintf (file, "%s <address> <port> <kbs> <filename>\n", program_name);
  exit(exit_status);
}


int 
file_size(FILE* f)
{
  int rv;
  struct stat filestat;

  rv = fstat(fileno(f), &filestat);

  if (rv == 0)
    return filestat.st_size;

  return rv;
}


void 
send_packet(guint32 seq_number)
{
  gint offset;
  gint data_length;
  gint bytes_written;

  /* Write the magic */
  strncpy(header->magic, JRMP_MAGIC, sizeof(header->magic));

  /* Write the sequence number */
  header->seq_number = g_htonl(seq_number);

  /* Write the type */
  header->packet_type = g_htons(JRMP_SEND);

  /* Write the data length */
  offset = seq_number * PAYLOAD_SIZE;
  data_length = MIN(PAYLOAD_SIZE, file_length - offset);
  header->length = g_htons(data_length);

  /* Read the packet */
  memcpy (header->data, &memin[offset], data_length);
  packet_out->length = sizeof(jmtransfer_header) + data_length;

  /* Write the packet */
  bytes_written = gnet_mcast_socket_send (msocket, packet_out);
  if (bytes_written != 0)
    perror ("Sending packet");
}

