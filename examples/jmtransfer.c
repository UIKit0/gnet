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

#include "jmtransfer.h"
#include <stdio.h>

static gint
timeval_compare (gconstpointer a, gconstpointer b)
{
  if (timercmp(&((jmtransfer_event*) a)->time, 
	       &((jmtransfer_event*) b)->time, >))
    return 1;
  else if (timercmp(&((jmtransfer_event*) a)->time, 
	       &((jmtransfer_event*) b)->time, >))
    return 0;

  return -1;
}

static gint
seq_number_compare (gconstpointer a, gconstpointer b)
{
  if (((jmtransfer_event*) a)->seq_number == *((guint32*)b))
    return 0;

  return 1;
}


static gint
type_compare (gconstpointer a, gconstpointer b)
{
  if (((jmtransfer_event*) a)->type == *((gint*)b))
    return 0;

  return 1;
}



jmtransfer_event* 
schedule_event (gint type, guint32 seq_number, long usec)
{
  jmtransfer_event* event;

  /* Create new event */
  event = g_new0 (jmtransfer_event, 1);
  event->type = type;
  event->seq_number = seq_number;
  g_assert(gettimeofday(&event->time, NULL) == 0);
  mytimeraddusec(&event->time, usec);

  /* Insert into event queue */
  event_queue = g_list_insert_sorted (event_queue, event, timeval_compare);

  return event;
}


gint
has_event_number (guint32 seq_number)
{
  return (g_list_find_custom (event_queue, &seq_number, seq_number_compare) != NULL);
}



void
delete_event_number (guint32 seq_number)
{
  GList* found;

  found = g_list_find_custom (event_queue, &seq_number, seq_number_compare);
  if (found != NULL)
    {
      event_queue = g_list_remove_link (event_queue, found);
      g_free (found->data);
      g_list_free_1 (found);
    }
}



gint
has_event_type (gint type)
{
  return (g_list_find_custom (event_queue, &type, type_compare) != NULL);
}

void	
delete_event_type (gint type)
{
  GList* i;

 start_over:
  for (i = event_queue; i != NULL; i = g_list_next(i))
    {
      if ( ((jmtransfer_event*) i->data)->type == type)
	{
	  event_queue = g_list_remove_link (event_queue, i);
	  g_free (i->data);
	  g_list_free_1 (i);
	  goto start_over;
	}
    }
}
