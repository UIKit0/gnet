/* GNet - Networking library
 * Copyright (C) 2000  David Helder
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the 
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA  02111-1307, USA.
 */


#include "scheduler.h"
#include <sys/unistd.h>
#include <math.h>


static gboolean can_dispatch (Scheduler* scheduler);
static void 	set_timer (Scheduler* scheduler);
static gboolean scheduler_cb (gpointer data);


typedef struct _Schedulee
{
  gpointer user_data;
  SchedulerFunc func;

} Schedulee;



Scheduler* 
scheduler_new (void)
{
  Scheduler* scheduler;

  scheduler = g_new0 (Scheduler, 1);
  scheduler->policy = SCHEDULER_POLICY_ROUND_ROBIN;
  scheduler->max_ups = SCHEDULER_UNLIMITED_UNITS;

  return scheduler;
}


void
scheduler_delete (Scheduler* scheduler)
{
  if (scheduler)
    {
      GList* i;

      for (i = scheduler->queue; i != NULL; i = i->next)
	g_free (i->data);
      g_list_free (scheduler->queue);

      if (scheduler->timer)
	g_source_remove (scheduler->timer);

      g_free (scheduler);
    }
}


void
scheduler_set_policy (Scheduler* scheduler, SchedulerPolicy policy)
{
  g_return_if_fail (scheduler);

  scheduler->policy = policy;
}


void
scheduler_set_max_ups (Scheduler* scheduler, gint max_units_per_second)
{
  g_return_if_fail (scheduler);

  scheduler->max_ups = max_units_per_second;

  /* Dispatch all now if it's now below 0 */
  if (max_units_per_second < 0)
    {
      GList* i;

      for (i = scheduler->queue; i != NULL; i = i->next)
	{
	  Schedulee* schedulee = (Schedulee*) i->data;
	  gint units;
	  /* FIX: What if I try to remove myself? */

	  units = (schedulee->func)(schedulee->user_data);
	  if (units)
	    scheduler->last_units = units;

	  g_free (schedulee);
	}

      if (scheduler->queue)
	{
	  g_assert (gettimeofday(&scheduler->last_dispatch, NULL) == 0);

	  g_list_free (scheduler->queue);
	  scheduler->queue = NULL;
	}
    }

  /* Otherwise, reset the timer if there is one and we're waiting for
     something */
  else if (scheduler->timer)
    {
      g_return_if_fail (scheduler->queue);

      g_source_remove (scheduler->timer);
      set_timer (scheduler);
    }

}




void
scheduler_add (Scheduler* scheduler, gpointer user_data, SchedulerFunc func, gint units)
{
  g_return_if_fail (scheduler);
  g_return_if_fail (func);

  /* Dispatch now if queue is empty and we can dispatch */
  if (!scheduler->queue && can_dispatch (scheduler))
    {
      scheduler->last_units = (func)(user_data);
      g_assert (gettimeofday(&scheduler->last_dispatch, NULL) == 0);
    }

  /* Otherwise, add to the queue.  Make sure the timer is set. */
  else
    {
      Schedulee* schedulee;

      schedulee = g_new (Schedulee, 1);
      schedulee->user_data = user_data;
      schedulee->func = func;

      scheduler->queue = g_list_append (scheduler->queue, schedulee);

      g_return_if_fail (scheduler->timer);
    }
}



void
scheduler_remove (Scheduler* scheduler, gpointer user_data)
{
  GList* i;

  g_return_if_fail (scheduler);

  /* Remove all schedulees */
  for (i = scheduler->queue; i != NULL; )
    {
      Schedulee* s = (Schedulee*) i->data;
      GList* old_i = i;
      i = i->next;

      if (s->user_data == user_data)
	{
	  g_free (s);
	  scheduler->queue = g_list_remove_link (scheduler->queue, old_i);
	}
    }

  /* Cancel timer if there's nothing left */
  if (!scheduler->queue && scheduler->timer)
    {
      g_source_remove (scheduler->timer);
      scheduler->timer = 0;
    }
}


static gboolean
can_dispatch (Scheduler* scheduler)
{
  struct timeval timeofday, diff;
  gdouble secs, ups;

  /* If we just dispatched something that was free, it can be dispatched */
  if (scheduler->last_units == 0)
    return TRUE;

  /* If bandwidth is unlimited, it can be dispatched. */
  if (scheduler->max_ups < 0)
    return TRUE;

  g_assert (gettimeofday(&timeofday, NULL) == 0);

  timersub (&timeofday, &scheduler->last_dispatch, &diff);
  secs = (gdouble) diff.tv_sec + (((gdouble) diff.tv_usec) / 1000000.0);
  ups = secs / (gdouble) scheduler->last_units;

  if (ups > scheduler->max_ups)
    return FALSE;

  return TRUE;
}



static void
set_timer (Scheduler* scheduler)
{
  gdouble offset;
  struct timeval timeofday, diff;
  gint ms;

  g_return_if_fail (scheduler);

  if (scheduler->max_ups <= 0 || scheduler->timer)
    return;

  offset = ((gdouble) scheduler->last_units) / ((gdouble) scheduler->max_ups);
  diff.tv_sec = (gint) offset;
  diff.tv_usec = (offset - floor(offset)) * 1000000.0;

  g_assert (gettimeofday(&timeofday, NULL) == 0);

  timeradd (&scheduler->last_dispatch, &diff, &diff);
  timersub (&diff, &timeofday, &diff);

  ms = (diff.tv_sec * 1000) + (diff.tv_usec / 1000);
  g_return_if_fail (ms >= 0);

  scheduler->timer = g_timeout_add (ms, scheduler_cb, scheduler);
}



static gboolean
scheduler_cb (gpointer data)
{
  Scheduler* scheduler = (Scheduler*) data;

  g_return_val_if_fail (scheduler, FALSE);

  scheduler->timer = 0;

  if (scheduler->queue)
    {
      Schedulee* schedulee;

    again:
      /* Pop off next schedulee */
      schedulee = (Schedulee*) scheduler->queue->data;
      scheduler->queue = g_list_remove_link (scheduler->queue, scheduler->queue);

      /* Dispatch */
      scheduler->last_units = (schedulee->func)(schedulee->user_data);
      g_free (schedulee);

      /* Do again if it didn't cost anything */
      if (!scheduler->last_units && scheduler->queue)
	goto again;

      g_assert (gettimeofday(&scheduler->last_dispatch, NULL) == 0);

      /* Schedule next dispatch */
      if (scheduler->queue)
	{
	  g_return_val_if_fail (scheduler->last_units, FALSE);
	  set_timer (scheduler);
	}
    }

  return FALSE;
}

