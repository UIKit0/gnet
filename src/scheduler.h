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

/* 

   The scheduler manages schedulees.  A schedulee is a function and
   executing that function has some unit cost.  The scheduler assures
   that all schedulees get executed and the units spent doesn't exceed
   some maximum rate.  The intent is to use this to limit bandwidth.
   The schedulee functions are reads or writes and cost is bytes
   writen.  

   This works the last time I checked, but I haven't integrated and
   documented it.  The problem is whether it should be integrated with
   iochannel, etc, which might be tricky.  It might make more sense to
   subclass Iochannel and add a subiochannel_set_scheduler.  But, I'm
   not sure if this is possible.  

   It's tricky to get this working with asynchronous IO functions.  If
   you can write to an IOChannel, then you hand the IOChannel to the
   scheduler which will perform the read at the appropriate time.
   This is fine, but can we integrate it with gnet_iochannel and
   conn/server?

*/


#ifndef _SCHEDULER_H
#define _SCHEDULER_H

#include <glib.h>
#include <sys/types.h>
#include <sys/time.h>


typedef enum
{
  SCHEDULER_POLICY_ROUND_ROBIN

} SchedulerPolicy;


#define SCHEDULER_UNLIMITED_UNITS	-1


typedef struct _Scheduler
{
  SchedulerPolicy	policy;
  gint 			max_ups;

  GList* 		queue;

  guint			timer;

  gint			last_units;
  struct timeval	last_dispatch;

} Scheduler;

typedef gint (*SchedulerFunc)(gpointer user_data);	/* Returns units used */


Scheduler*  scheduler_new (void);
void	    scheduler_delete (Scheduler* scheduler);

void	    scheduler_set_policy (Scheduler* scheduler, SchedulerPolicy policy);
void	    scheduler_set_max_ups (Scheduler* scheduler, gint max_units_per_second);

void	    scheduler_add (Scheduler* scheduler, gpointer user_data, SchedulerFunc func, gint units);
void	    scheduler_remove (Scheduler* scheduler, gpointer user_data);


#endif /* _SCHEDULER_H */
