/* LJoust - Multicast Llama Joust
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


#include <stdlib.h>
#include <strings.h>

#include <sys/time.h>
#include <unistd.h>

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <gnet/gnet.h>



#define LLAMA_WIDTH   30
#define LLAMA_HEIGHT  30


/* **************************************** */

/* XPM */
static char * llama_xpm[] = {
"30 30 70 1",
" 	c None",
".	c #FEFEFE",
"+	c #FFFFFF",
"@	c #827F00",
"#	c #817E00",
"$	c #D4D4D4",
"%	c #000000",
"&	c #2A2A2A",
"*	c #545454",
"=	c #636363",
"-	c #AAAAAA",
";	c #383838",
">	c #0E0E0E",
",	c #827E00",
"'	c #FDFDFD",
")	c #070707",
"!	c #4B4B4B",
"~	c #F2F2F2",
"{	c #EFEFEF",
"]	c #F0F0F0",
"^	c #717171",
"/	c #232323",
"(	c #B1B1B1",
"_	c #A1A1A1",
":	c #FEFFFE",
"<	c #8D8D8D",
"[	c #B7B8B7",
"}	c #E8E8E8",
"|	c #D3D3D3",
"1	c #A6A6A6",
"2	c #AFAFAF",
"3	c #CBCBCB",
"4	c #EDEDED",
"5	c #FCFDFC",
"6	c #F6F6F6",
"7	c #BDBDBD",
"8	c #9D9D9D",
"9	c #ADADAD",
"0	c #B7B7B7",
"a	c #BEBEBE",
"b	c #CDCDCD",
"c	c #B8B8B8",
"d	c #A9A9A9",
"e	c #C3C3C3",
"f	c #1C1C1C",
"g	c #DFDFDF",
"h	c #4C4C4C",
"i	c #BBBBBB",
"j	c #4D4D4D",
"k	c #2B2A00",
"l	c #626262",
"m	c #393800",
"n	c #353523",
"o	c #A2A2A2",
"p	c #7F7F7F",
"q	c #979797",
"r	c #C6C6C6",
"s	c #B4B4B1",
"t	c #151500",
"u	c #393823",
"v	c #EAE9D4",
"w	c #FFFEFE",
"x	c #FEFDFD",
"y	c #6C6900",
"z	c #7A7700",
"A	c #565400",
"B	c #5D5B00",
"C	c #858207",
"D	c #96942A",
"E	c #88860E",
".+.+..@@++++.............+++++",
"...+++@@++++.....+++++++++++++",
"..@@@##@++++.......$%%%%%&++++",
"@@@@%%@@.......++**=----;>*+++",
"@,,@%%@@.....''+)&&!~{].^/%-++",
"@@,@##@@++++.'+*&'.~(_-..$%-++",
"@@,@##@@++::.+*<[.}|_1234$%-++",
"++@@##@@...:+[&[56$7890abc&c++",
".+@@###@+..+*&..63d8(e$.*&++++",
".+@@###@@++*f&.}$dd9eg|*f&++++",
"+++@####@@0&&h6$789i$]|%fj++++",
".+.@####@k&+.63d8(e$..|%-+++++",
".+.@####@k&+}$dd9eg$**l-+.++++",
"...@####@mn$cop<qdrsttuv+.++++",
".wx@####@@y%%%%%%%%t####++++++",
"..x+####@@zAAAAAAAAB###@@+++++",
"+w++@###@@@@##########C@@+++++",
".+.+@###@@@@##########@@@+++++",
"...++###@@@@#########@@@++++++",
".+.++@#C@DDD@@@@@####@@..+++++",
".+.++@#@@w++....@##@#@...+++++",
".+.++@#@@.++....+##@#@...+++++",
"+..++##@..++....++E@@@...+++++",
"..'+@##@++++......@@@@...+++++",
"..'+@@@@++++......@@@@...+++++",
"+.'+#@@@++++......@@@@...+++++",
"..'+@#@@++++......@@@@...+++++",
"...+@@@@++++.....+@@@@...+++++",
"+++@@@@@++++....@@@@@@...+++++",
".+@@@@@@++++....@@@@@@...+++++"};


/* **************************************** */





typedef struct _jouster
{
  GdkRectangle position;
  GdkPoint velocity;
  gchar* name;
} Jouster;

Jouster* myJouster = NULL;
gboolean did_move = TRUE;

/* Backing pixmap for drawing area */
static GdkPixmap *back = NULL;
static GdkPixmap *picture = NULL;
static GtkWidget *window = NULL;

GdkPoint fieldSize;

GList* jousters;

GList* platforms;
GdkRectangle bottom;
GdkRectangle top;

gint tick = 0;

gchar out_buffer[256];
gchar in_buffer[256];
GUdpPacket* in_packet;
GUdpPacket* out_packet;

GMcastSocket* msocket_in;
GUdpSocket* usocket_out;

struct timeval start_time;


static void print_usage_and_exit();
static void init_gui();
static void init_network();
static void init_game();


static void do_collisions();
static gint do_tick(gpointer data);
static void draw (GtkWidget *widget);
static void update_jouster(gchar* name, Jouster* jouster);

static void delete_event (GtkWidget *widget, GdkEvent *event, gpointer data); 
static gint configure_event (GtkWidget *widget, GdkEventConfigure *event);
static gint expose_event (GtkWidget *widget, GdkEventExpose *event);
static gint key_press_event (GtkWidget* widget, GdkEventKey* event);

static gboolean rectangle_above(GdkRectangle* a, GdkRectangle* b);
static gboolean rectangle_left(GdkRectangle* a, GdkRectangle* b);

#define rectangle_below(a, b) (rectangle_above(b, a))
#define rectangle_right(a, b) (rectangle_left(b, a))


/* **************************************** */

int
main (int argc, char *argv[])
{
  int option;
  gchar* name = NULL;

  while ((option = getopt(argc, argv, "n:")) != -1)
    {
      switch (option)
	{
	case 'n':
	  {
	    name = optarg;
	    break;
	  }
	case '?':
	default:
	  {
	    print_usage_and_exit();
	  }
	}
    }

  if (name == NULL)
    print_usage_and_exit();

  myJouster = g_new0(Jouster, 1);
  myJouster->position.width = LLAMA_WIDTH;
  myJouster->position.height = LLAMA_HEIGHT;
  myJouster->name = name;
  jousters = g_list_prepend(jousters, myJouster);

  gtk_init (&argc, &argv);

  init_gui();
  init_network();
  init_game();


  /* Test timeout */
/*   gtk_idle_add(timeout_callback, NULL); */
  gtk_timeout_add(20, do_tick, NULL);

  gtk_main ();

  return 0;
}



static
void update_jouster(gchar* name, Jouster* new_jouster)
{
  GList* iterator;
  Jouster* jouster;

  int delta_x, delta_y;

  /* Try to find the jouster */
  for (iterator = jousters; 
       iterator != NULL; 
       iterator = g_list_next(iterator))
    {
      jouster = (Jouster*) iterator->data;
      
      if (strcmp(name, jouster->name) == 0)
	{
	  /* Found it */

	  /* Update velocity */
	  jouster->velocity = new_jouster->velocity;

	  /* Update position if off a lot */
	  delta_x = jouster->position.x - new_jouster->position.x;
	  delta_y = jouster->position.y - new_jouster->position.y;

	  if ((ABS(delta_x) > (3 * LLAMA_WIDTH)) || 
	      (ABS(delta_y) > (3 * LLAMA_WIDTH)) )
	    {
	      jouster->position = new_jouster->position;
	    }
	  else
	    {
	      if (delta_x > LLAMA_WIDTH)
		--jouster->velocity.x;
	      else if (delta_x < -LLAMA_WIDTH)
		++jouster->velocity.x;

	      if (delta_y > LLAMA_HEIGHT)
		--jouster->velocity.y;
	      else if (delta_y < -LLAMA_HEIGHT)
		++jouster->velocity.y;
	    }

	  return;

	}
    }

  /* Add a new jouster */
  jouster = g_new0(Jouster, 1);
  *jouster = *new_jouster;
  jouster->name = g_strdup(name);
  jousters = g_list_prepend(jousters, jouster);
}


static void
do_collisions()
{
  GList* jiterator;
  GList* piterator;
  Jouster* jouster;
  GdkRectangle* platform;

  GdkRectangle intersection;


  /* For each jouster */
  for (jiterator = jousters; 
       jiterator != NULL; 
       jiterator = g_list_next(jiterator))
    {
      jouster = (Jouster*) jiterator->data;

      /* For each platform */
      /* Check collisions with platforms */
      for (piterator = platforms; 
	   piterator != NULL; 
	   piterator = g_list_next(piterator))
	{
	  platform = (GdkRectangle*) piterator->data;

	  /* Check if there's an intersection */
	  if (gdk_rectangle_intersect(&jouster->position, platform, &intersection))
	    {
	      /* Push up if necessary */
	      if (jouster->position.y < platform->y && 
		  !rectangle_above(&jouster->position, platform) )
		{
		  jouster->position.y = platform->y - jouster->position.height;
		  jouster->velocity.y = -jouster->velocity.y + 1;
		}

	      /* Push down if necessary */
	      if (jouster->position.y > platform->y && 
		  !rectangle_above(platform, &jouster->position) )
		{
		  jouster->position.y = platform->y + platform->height;
		  jouster->velocity.y = -jouster->velocity.y;
		}
	  
	      /* TODO: Push left, right??? */
	    }
	}

      /* TODO: Check for collisions with the rest of the jousters */
    }
}



static gint
do_tick(gpointer data)
{
  GList* jiterator;
  Jouster* jouster;

/*   struct timeval time; */
/*   gettimeofday(&time, NULL); */
/*   g_print ("time = %ld, %ld\n", time.tv_sec, time.tv_usec / 1000); */

  ++tick;

/*   g_print ("do_tick\n"); */

  /* Check if I have a packet ready */
  while (gnet_mcast_socket_has_packet(msocket_in))
    {
      gint length;
      gint rv;
      Jouster jouster;
      gchar name_buffer[32];

      length = gnet_mcast_socket_receive(msocket_in, in_packet);
      in_buffer[length] = '\0';

      rv = sscanf(in_buffer, "JOUST %30s %d %d %d %d %d %d\n",
		  name_buffer, 
		  &jouster.position.x, &jouster.position.y,
		  &jouster.position.width, &jouster.position.height,
		  &jouster.velocity.x, &jouster.velocity.y);

      if (rv != 7)
	{
	  g_print("warning, bad packet: %s", in_buffer);
	  break;
	}

      if (strcmp(name_buffer, myJouster->name) == 0)
	break;

      update_jouster(name_buffer, &jouster);
    }


  /* Send an update packet if I moved */
  if ((did_move == TRUE) || (tick % 10) == 0)
    {
      snprintf(out_buffer, sizeof(out_buffer), "JOUST %s %d %d %d %d %d %d\n",
	       myJouster->name, 
	       myJouster->position.x, myJouster->position.y, 
	       myJouster->position.width, myJouster->position.height, 
	       myJouster->velocity.x, myJouster->velocity.y);
      out_packet->length = strlen(out_buffer);

      gnet_udp_socket_send(usocket_out, out_packet);

      did_move = FALSE;
    }


  /* Update each jouster */
  for (jiterator = jousters;
       jiterator != NULL;
       jiterator = g_list_next(jiterator))
    {
      jouster = (Jouster*) jiterator->data;

      /* Update position */
      jouster->position.x += jouster->velocity.x;
      jouster->position.y += jouster->velocity.y;

      /* Adjust x for wrap around */
      jouster->position.x %= fieldSize.x;
      if (jouster->position.x < 0) 
	jouster->position.x += fieldSize.x;

      /* Do gravity */
      if (tick % 10 == 0)
	jouster->velocity.y++;

      /* Adjust velocities */
      jouster->velocity.x = MIN(jouster->velocity.x, 10);
      jouster->velocity.x = MAX(jouster->velocity.x, -10);

      jouster->velocity.y = MIN(jouster->velocity.y, 10);
      jouster->velocity.y = MAX(jouster->velocity.y, -10);
    }

  do_collisions();



  draw (window);

  return TRUE;
}


/* Draw a rectangle on the screen */
static void
draw (GtkWidget *widget)
{
  GdkRectangle rect;
  GList* iterator;


/*   g_print ("widget: private_flags = %d, state = %d, saved_state = %d\n", */
/* 	   widget->private_flags, widget->state, widget->saved_state); */

/*   g_print ("widget: name = 0x%x, style = 0x%x, window = 0x%x, parent = 0x%x\n", */
/* 	   widget->name, widget->style, widget->window, widget->parent); */


  /* Clear back */
  gdk_draw_rectangle (back,
		      widget->style->white_gc,
		      TRUE,
		      0, 0,
		      widget->allocation.width,
		      widget->allocation.height);


  /* Draw platforms */
  for (iterator = platforms; 
       iterator != NULL; 
       iterator = g_list_next(iterator))
    {
      GdkRectangle* platform;

      platform = (GdkRectangle*) iterator->data;

/*       g_print ("platform = {%d, %d, %d, %d}\n",  */
/* 	       platform->x, platform->y, */
/* 	       platform->width, platform->height); */

      gdk_draw_rectangle (back,
			  widget->style->black_gc,
			  TRUE,
			  platform->x, platform->y,
			  platform->width, platform->height);
    }


  /* Draw jousters */
  for (iterator = jousters;
       iterator != NULL;
       iterator = g_list_next(iterator))
    {
      Jouster* jouster;

      jouster = (Jouster*) iterator->data;

/*       g_print ("jouster = {%d, %d} {%d, %d, %d, %d}\n",  */
/* 	       jouster->velocity.x, jouster->velocity.y, */
/* 	       jouster->position.x, jouster->position.y, */
/* 	       jouster->position.width, jouster->position.height); */



      gdk_draw_pixmap(back,
		      widget->style->fg_gc[GTK_WIDGET_STATE (widget)],
		      picture,
		      0, 0,
		      jouster->position.x, jouster->position.y,
		      jouster->position.width, jouster->position.height);
    }

  rect.x = 0;
  rect.y = 0;
  rect.width = widget->allocation.width;
  rect.height = widget->allocation.height;

/*   g_print ("widget.allocation = {%d, %d}\n",  */
/* 	   widget->allocation.width,  */
/* 	   widget->allocation.height); */

  gtk_widget_draw (widget, &rect);
}



static void
print_usage_and_exit()
{
  g_print ("Llama Joust.  Copyright 2000 David A. Helder\n");
  g_print ("ljoust -n <name>\n");
  exit(EXIT_FAILURE);
}


static void 
init_gui()
{
  GtkWidget *drawing_area;
  GtkWidget *vbox;
  GdkBitmap* mask;
  GtkStyle* style;

  /* Create the window */
  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (window), vbox);
  gtk_widget_show (vbox);

  gtk_signal_connect (GTK_OBJECT (window), "delete_event",
		      GTK_SIGNAL_FUNC (delete_event), 0);
  gtk_signal_connect (GTK_OBJECT (window), "key_press_event",
		      GTK_SIGNAL_FUNC (key_press_event), NULL);


  /* Create the drawing area */
  drawing_area = gtk_drawing_area_new ();
  gtk_drawing_area_size (GTK_DRAWING_AREA (drawing_area), 400, 300);
  gtk_box_pack_start (GTK_BOX (vbox), drawing_area, TRUE, TRUE, 0);
  gtk_widget_show (drawing_area);

  /* Signals used to handle backing pixmap */
  gtk_signal_connect (GTK_OBJECT (drawing_area), "expose_event",
		      (GtkSignalFunc) expose_event, NULL);
  gtk_signal_connect (GTK_OBJECT(drawing_area),"configure_event",
		      (GtkSignalFunc) configure_event, NULL);
  gtk_widget_set_events (drawing_area, 
			   GDK_EXPOSURE_MASK
			 | GDK_KEY_PRESS_MASK);


  /* Show the window */
  gtk_widget_show (window);

  /* Load my picture */
  style = gtk_widget_get_style(window);
  picture = gdk_pixmap_create_from_xpm_d(window->window, &mask, 
					 &style->bg[GTK_STATE_NORMAL], 
					 llama_xpm);
}


static void
init_network()
{
  GInetAddr* iaddr;
  gint rv;

  /* Connect to network */
  iaddr = gnet_inetaddr_new("234.234.234.234", 4567);
  g_assert(iaddr != NULL);

  /* Set up multicast */
  msocket_in = gnet_mcast_socket_port_new(4567);
  g_assert (msocket_in != NULL);
  rv = gnet_mcast_socket_join_group(msocket_in, iaddr);
  g_assert (rv == 0);
/*   rv = mcast_socket_set_loopback(msocket_in, 0);  */
/*   g_assert (rv == 0); */

  /* Set up udp */
/*   usocket_out = (udp_socket*) msocket_in; */
  usocket_out = gnet_udp_socket_new();
  g_assert (usocket_out != NULL);

  /* Set up packets */
  in_packet =  gnet_udp_packet_receive_new(in_buffer, sizeof(in_buffer));
  g_assert (in_packet != NULL);
  out_packet = gnet_udp_packet_send_new(out_buffer, sizeof(out_buffer), iaddr);
  g_assert (out_packet != NULL);


}


static void
init_game()
{
  GdkRectangle* middlePlatform;

  /* Add the bottom to the list of platforms */
  platforms = g_list_prepend(platforms, &bottom);

  /* Add the top to the list of platforms */
  platforms = g_list_prepend(platforms, &top);

  /* Add another platform to the list */
  middlePlatform = g_new(GdkRectangle, 1);
  middlePlatform->x = 50;
  middlePlatform->y = 100;
  middlePlatform->height = 10;
  middlePlatform->width = 200;
  platforms = g_list_prepend(platforms, middlePlatform);

  /* Note the time of day */
  gettimeofday(&start_time, NULL);

}


/* another callback */
void
delete_event (GtkWidget *widget, GdkEvent *event, gpointer data)
{
  struct timeval end_time;
  struct timeval diff;

  /* Print out frame rate */
  gettimeofday(&end_time, NULL);
  timersub(&end_time, &start_time, &diff);

  g_print ("ticks = %d, time = %d, frame rate = %d\n", tick, 
	   diff.tv_sec, tick / diff.tv_sec );

  gtk_main_quit ();
}



/* Create a new backing pixmap of the appropriate size */
static gint
configure_event (GtkWidget *widget, GdkEventConfigure *event)
{
/*   g_print ("configure_event\n"); */

  if (back)
    gdk_pixmap_unref(back);

  bottom.x = 0;
  bottom.y = widget->allocation.height - 5;
  bottom.width = widget->allocation.width;
  bottom.height = 100;

  top.x = 0;
  top.y = 1.2 * widget->allocation.height + 100;
  top.width = widget->allocation.width;
  top.height = 100;

  fieldSize.x = widget->allocation.width;
  fieldSize.y = widget->allocation.height;

  back = gdk_pixmap_new(widget->window,
			widget->allocation.width,
			widget->allocation.height,
			  -1);
  gdk_draw_rectangle (back,
		      widget->style->white_gc,
		      TRUE,
		      0, 0,
		      widget->allocation.width,
		      widget->allocation.height);

  return TRUE;
}


/* Redraw the screen from the backing pixmap */
static gint
expose_event (GtkWidget *widget, GdkEventExpose *event)
{
/*   g_print ("expose_event\n"); */

  gdk_draw_pixmap(widget->window,
		  widget->style->fg_gc[GTK_WIDGET_STATE (widget)],
		  back,
		  event->area.x, event->area.y,
		  event->area.x, event->area.y,
		  event->area.width, event->area.height);

  return FALSE;
}


static gint
key_press_event (GtkWidget* widget, GdkEventKey* event)
{
/*   g_print ("key_press_event\n"); */

  did_move = TRUE;
  switch (event->keyval)
    {
    case GDK_Up:      {myJouster->velocity.y -= 1;	break; }
    case GDK_Down:    {myJouster->velocity.y += 1;	break; }
    case GDK_Left:    {myJouster->velocity.x -= 1;	break; }
    case GDK_Right:   {myJouster->velocity.x += 1;	break; }
    }

  return TRUE;
}




static gboolean
rectangle_above(GdkRectangle* a, GdkRectangle* b)
{
  if ((a->y + a->height) < b->y)
    return TRUE;

  return FALSE;
}

static gboolean
rectangle_left(GdkRectangle* a, GdkRectangle* b)
{
  if ((a->x + a->width) < b->x)
    return TRUE;

  return FALSE;
}
