/* Jungle Monkey Reliable Multicast Receiver
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
#include <errno.h>
#include <fcntl.h>
#include <math.h>

#include <glib.h>
#include <gnet/gnet.h>
#include "jmtransfer.h"


#define BUFFER_SIZE 		1472
#define PACKETS_BEFORE_REQUEST	1.2		/* Number of packets that can go by
						   before retransmit request is sent */
#define MAX_RETRIES		8
#define EXP_NEXT_REQUEST	1.5		/* Exponential scale by which to 
						   modify time for next request 
						   (wait for EXP ^ retry * usec_between) */
#define DROP_PACKETS		0		/* Use PROB_DROP to drop packets? */
#define PROB_DROP		0.1		/* Change a packet will be dropped */
#define DEBUG			0

#define PAYLOAD_SIZE  	(BUFFER_SIZE - sizeof(jmtransfer_header))
#define LOG_FILE		"jmreceive.log"

typedef struct _output_buffer
{
  int seq_number;
  int length;
  int offset;
  char buffer[BUFFER_SIZE];

} output_buffer;
GList* output_queue = NULL;


GList* event_queue = NULL; 

/*  static gint verbose = 1; */
static void usage (FILE* file, int exit_status);
static gint jmtransfer_header_compare (gconstpointer a, gconstpointer b);


static GList* early_queue = NULL;
#define EARLY_QUEUE_TOP	((early_queue != NULL)				\
			 ?((jmtransfer_header*) early_queue->data)	\
			 :(NULL))

static gchar* program_name = NULL;
static FILE* log_file = NULL;

int
main(int argc, char** argv)
{
  gchar* addr_name;
  gint port;
  gint kbs;
  gchar* file_name;
  gint file_length;

  GInetAddr* ia;
  GMcastSocket* msocket;
  GUdpPacket* packet_in;
  GUdpPacket* packet_out;
  gchar buffer[BUFFER_SIZE];
  jmtransfer_header* header = (jmtransfer_header*) buffer;

  FILE* fout;

  gint expected_seq_number = 0;
  gint total_bytes_read = 0;
  gint total_bytes_written = 0;
  gint usec_between_packets;

#if DROP_PACKETS > 0
  fprintf (stderr, "dropping packets\n");
  srand(time(NULL));
#endif

  program_name = argv[0];

  /* Parse arguments */
  if (argc != 6)
    usage (stderr, EXIT_FAILURE);

  addr_name = 	argv[1];
  port = 	atoi(argv[2]);
  kbs =  	atoi(argv[3]);
  file_name = 	argv[4];
  file_length = atoi(argv[5]);


  /* Open the log file */
#if DEBUG
  log_file = fopen(LOG_FILE, "w");
  if (log_file == NULL) log_file = stderr;
#endif

#if DEBUG
  fprintf (log_file, "arguments = %s, %d, %d, %s, %d\n", 
	   addr_name, port, kbs, file_name, file_length);
#endif

  /* Create the address */
  ia = gnet_inetaddr_new (addr_name, port);
  if (ia == NULL)
    {
      perror ("Could not find internet address");
      exit (EXIT_FAILURE);
    }

  /* Create the multicast socket */
  msocket = gnet_mcast_socket_inetaddr_new(ia);
  g_assert (msocket != NULL);

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

  /* Open the file to write into.  If the file name is "-", use stdout */
  if (strncmp(file_name, "-", 1) != 0)
    {
      if ((fout = fopen (file_name, "w")) == NULL)
	{
	  perror ("Could not open file");
	  exit (EXIT_FAILURE);
	}
    }
  else
    fout = stdout;


  /* Calculate number of usec between sends */
  usec_between_packets = (1000000.0 * (double) BUFFER_SIZE) / 
    (((double) kbs / 8.0) * 1024.0);

#if DEBUG
  fprintf (log_file, "usec between packets is %d\n", usec_between_packets);
#endif

  /* Loop reading and writing.  We quit once we've written everything */
  for (total_bytes_written = 0; total_bytes_written < file_length; )
    {
      fd_set read_fdset, write_fdset;
      struct timeval timeout, timeofday;
      struct timeval* timeoutp = NULL;
      int max_fdp1;
      int num_fd_ready;



      /* *****************************************/
      /* Wait until either a packet or an event */
      
      /* Set up the fd set */
      FD_ZERO (&read_fdset);
      FD_ZERO (&write_fdset);
      max_fdp1 = 0;

      /* Read only if there's more to read */
      if (total_bytes_read < file_length)
	{
#if DEBUG
	  fprintf (log_file, "read select %d\n", msocket->sockfd);
#endif
	  FD_SET (msocket->sockfd, &read_fdset);
	  max_fdp1 = MAX (msocket->sockfd, max_fdp1);
	}
#if DEBUG
      else
	fprintf (log_file, "no read select\n");
#endif

      /* Write to fout if there is data */
      if (output_queue != NULL)
	{
#if DEBUG
	  fprintf (log_file, "write select %d\n", fileno(fout));
#endif
	  FD_SET (fileno(fout), &write_fdset);
	  max_fdp1 = MAX (fileno(fout), max_fdp1);
	}
#if DEBUG
      else
	fprintf (log_file, "no write select\n");
#endif


      /* Get next event */
      if (EVENT_QUEUE_TOP != NULL)
	{
	  timeout = EVENT_QUEUE_TOP->time;
	  gettimeofday(&timeofday, NULL);

	  timersub (&timeout, &timeofday, &timeout);
	  mytimermakepos(&timeout);

	  timeoutp = &timeout;

#if DEBUG
	  fprintf (log_file, "next event in %ld,%ld\n", 
		   timeout.tv_sec, timeout.tv_usec);
#endif
	}
      else
	{
	  timeoutp = NULL;
#if DEBUG
	  fprintf (log_file, "no event select\n");
#endif
	}

      if (max_fdp1 != 0) ++max_fdp1;

#if DEBUG
      fprintf (log_file, "select (%d, %p)...", max_fdp1, timeoutp);
#endif

      /* Wait for something to happen */
      if ((num_fd_ready = select (max_fdp1, &read_fdset, &write_fdset, NULL, timeoutp)) == -1)
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


      /* 
	 if we have a packet
	 if this is the next packet in our sequence
	 write the data out					x
	 if there any packets in the early queue		x
	 attempt to write them				x
	 else
	 add the packet to our early queue			x
	 create an event to request the missing packets	x
	 cancel any events involving the packet		x
	     
	 if we have an event
	 if the event is a request
	 if this is the max request				x
	 quit
	 send the request					x
	 schedule another request * 2			x
      */

      /* We have a packet to read */
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

	      /* Read a packet */
	      bytes_read = gnet_mcast_socket_receive (msocket, packet_in);
	      if (bytes_read < 0)
		{
		  perror ("Reading from multicast socket");
		  continue;
		}

	      /* Make sure the packet is long enough */
	      if (bytes_read < sizeof(jmtransfer_header))
		continue;

	      /* Make sure the packet has the right magic name */
	      if (strncmp (header->magic, JRMP_MAGIC, sizeof(JRMP_MAGIC) - 1) != 0)
		continue;

	      /* Read the fields */
	      seq_number = g_ntohl (header->seq_number);
	      packet_type = g_ntohs (header->packet_type);
	      length = g_ntohs (header->length);

	      /* Maybe drop the packet (for debuging) */
#if DROP_PACKETS > 0
	      if ( ((double) rand()) / ((double) RAND_MAX) < PROB_DROP)
		{
#if DEBUG
		  fprintf (log_file, "drop packet %d of type %d\n", 
			   seq_number, packet_type);
#endif
		  continue;
		}
#endif

	      /* TODO: If its a request for something we don't have AND we have
		 the request queued, delay the request */

	      /* If it's not a send, ignore it */
	      if (packet_type != JRMP_SEND)
		continue;

#if DEBUG
	      fprintf (log_file, "receive %d, expecting %d\n", 
		       seq_number, expected_seq_number); 
#endif

	      /* Make sure the length is sane */
	      if (bytes_read != length + sizeof(jmtransfer_header))
		continue;

	      /* Check if this is the next packet in our sequence */
	      if (seq_number == expected_seq_number)
		{
		  output_buffer* ob;

#if DEBUG
		  fprintf (log_file, "COMMIT %d\n", seq_number);
#endif

		  /* Write the packet to the output buffer */
		  ob = g_new0(output_buffer, 1);
		  ob->seq_number = expected_seq_number;
		  ob->length = length;
		  ob->offset = 0;
		  memcpy (ob->buffer, header->data, length);
		  output_queue = g_list_append (output_queue, ob);

		  total_bytes_read += length;		  /* Count it as bytes read */
		  ++expected_seq_number;		  /* Expect next packet */

		  /* Write any packets from our early queue */
		  while (EARLY_QUEUE_TOP != NULL && 
			 g_ntohl(EARLY_QUEUE_TOP->seq_number) == expected_seq_number)
		    {
		      guint16 early_length;
		      jmtransfer_header* early_header;

		      /* Pop top packet */
		      early_header = EARLY_QUEUE_TOP;
		      early_queue = g_list_remove (early_queue, early_header);
		      early_length = g_ntohs(early_header->length);

#if DEBUG
		      fprintf (log_file, "commit %d, length %d (was early)\n", 
			       expected_seq_number, early_length);
#endif

		      /* Write the packet to the file */
		      ob = g_new0(output_buffer, 1);
		      ob->seq_number = expected_seq_number;
		      ob->length = early_length;
		      ob->offset = 0;
		      memcpy (ob->buffer, early_header->data, early_length);
		      output_queue = g_list_append (output_queue, ob);

		      total_bytes_read += early_length;   /* Count it as bytes read */
		      ++expected_seq_number;		  /* Expect next packet */
		    }
		}

	      else if (seq_number > expected_seq_number)
		{
		  /* Add the packet to our early queue */
		  jmtransfer_header* early_packet = 
		    (jmtransfer_header*) g_new0(gchar, sizeof(buffer));
		  memcpy(early_packet, buffer, sizeof(buffer));
		  early_queue = g_list_insert_sorted(early_queue, early_packet,
						     jmtransfer_header_compare);

#if DEBUG
		  fprintf (log_file, "early queue %d\n", seq_number);
#endif

		  /* Create an event to request the missing packets,
		     if there isn't one already */
#if 0
		  if (!has_event(expected_seq_number))
		    {
#if DEBUG
		      fprintf (log_file, "schedule first request %d\n", 
			       expected_seq_number);
#endif
		      schedule_event (JRMP_EVENT_REQUEST, expected_seq_number, 
				      PACKETS_BEFORE_REQUEST * usec_between_packets);
		    }
#endif
		}
	      /* else, it's a packet we already have */

	      /* Cancel any event involving the packet we just received */
	      delete_event_number (seq_number);

	      /* Schedule a request for the expected packet, if there isn't one
		 already.  If we receive the expected packet (which is to be expected,
		 everything is ok.  Otherwise, we automatically request it.
	      */
	      if (total_bytes_read != file_length &&
		  !has_event_number(expected_seq_number))
		{
		  schedule_event (JRMP_EVENT_REQUEST, expected_seq_number, 
				  PACKETS_BEFORE_REQUEST * usec_between_packets);
		}

	    } while (gnet_udp_socket_has_packet((GUdpSocket*) msocket));
	}


      /* Process event until there are none left */
      while (EVENT_QUEUE_TOP != NULL &&
	     !timercmp (&timeofday, &EVENT_QUEUE_TOP->time, <))
	{
	  /* Get the event */
	  jmtransfer_event* event = EVENT_QUEUE_TOP;
	  event_queue = g_list_remove (event_queue, event);

#if DEBUG
 	  fprintf (log_file, "have event\n");
#endif

	  /* Check if this is a request */
	  if (event->type == JRMP_EVENT_REQUEST)
	    {
	      jmtransfer_event* new_event;

	      /* If this is the max request, quit */
	      /* TODO - this is kinda weird ... */
	      if (MAX_RETRIES <= event->retry_number)
		{
		  fprintf (stderr, "*** Warning: not receiving anything\n");
		  continue;
		}
#if DEBUG
	      fprintf (log_file, "request %d\n", event->seq_number);
#endif

	      /* Create the request packet */
	      strncpy (header->magic, JRMP_MAGIC, sizeof(header->magic));
	      header->seq_number = g_htonl(event->seq_number);
	      header->packet_type = g_htons(JRMP_REQUEST);
	      header->length = g_htons(0);
	      
	      /* Send the request packet */
	      if (gnet_mcast_socket_send (msocket, packet_out) != 0)
		perror ("Sending retransmit request packet");

	      /* Schedule another send event */
	      /* This will get canceled if we receive the packet */
	      new_event = schedule_event (JRMP_EVENT_REQUEST, event->seq_number, 
					  (event->retry_number == 0)? (0) :
					  (usec_between_packets *
					   pow((double) EXP_NEXT_REQUEST, 
					       (double) (event->retry_number))));

	      new_event->retry_number = event->retry_number + 1;
	    }



	  /* Delete the event */
	  g_free(event);
	}


      /* We can write out */
      if (FD_ISSET(fileno(fout), &write_fdset))
	{
	  output_buffer* ob;
	  int bytes_written;

#if DEBUG
	  fprintf (log_file, "have write\n");
#endif

	  g_assert (output_queue != NULL);

	  /* Get the top output buffer */
	  ob = (output_buffer*) output_queue->data;

#if DEBUG
	  fprintf (log_file, "about to write %d bytes at offset %d\n", 
		   ob->length, ob->offset);
#endif

	  /* Write out */
	  bytes_written = fwrite (&ob->buffer[ob->offset], sizeof(gchar), ob->length, fout);
	  if (ferror(fout) && errno != EWOULDBLOCK)
	    {
	      perror ("Fwrite out");
	      break;
	    }

#if DEBUG
	  fprintf (log_file, "write %d, %d bytes\n", ob->seq_number, bytes_written);
#endif

	  ob->length -= bytes_written;
	  ob->offset += bytes_written;
	  total_bytes_written += bytes_written;

	  if (ob->length == 0)
	    {
	      /* We've written all - remove */
	      output_queue = g_list_remove (output_queue, ob);
	      g_free(ob);
	    }
	}
    }

  if (log_file != NULL)	fclose (log_file);
  fclose (fout);

  /* Check if we failed */
  if (total_bytes_written < file_length)
    exit (EXIT_FAILURE);

  exit (EXIT_SUCCESS);
}


static void
usage (FILE* file, int exit_status)
{
  fprintf (file, "%s <address> <port> <kbs> <filename> <length>\n", program_name);
  exit(exit_status);
}


static gint
jmtransfer_header_compare (gconstpointer a, gconstpointer b)
{
  if (g_ntohl(((jmtransfer_header*) a)->seq_number) >
      g_ntohl(((jmtransfer_header*) b)->seq_number))
    return 1;
  else
    return 0;
}
