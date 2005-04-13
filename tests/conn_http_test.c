/***************************************************************************
 *
 * GConnHttp test
 *
 * Copyright (C) 2004  Tim-Philipp Müller
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
 *
 ***************************************************************************/

#define GNET_EXPERIMENTAL 1
#include "conn-http.h"

#include <stdlib.h>
#include <string.h>       

#undef CONN_HTTP_GTK_DEMO

#ifdef CONN_HTTP_GTK_DEMO
# include <errno.h>
# include <gtk/gtk.h>
# include <unistd.h>
#endif

static gboolean   verbose; /* FALSE */

/***************************************************************************
 *
 *   http_callback
 *
 *   Print some information along the way (not required)
 *
 ***************************************************************************/

static void
http_callback (GConnHttp *http, GConnHttpEvent *event, gpointer baz)
{
	if (!verbose)
		return;

	switch (event->type)
	{
		case GNET_CONN_HTTP_RESOLVED:
		{
			GConnHttpEventResolved *ev_resolved = (GConnHttpEventResolved*)event;
			if (ev_resolved->ia)
				g_print ("RESOLVED: %s\n\n", gnet_inetaddr_get_canonical_name(ev_resolved->ia));
			else
				g_print ("RESOLVED: FAILED TO RESOLVE HOSTNAME.\n");
		}
		break;

		case GNET_CONN_HTTP_RESPONSE:
		{
			GConnHttpEventResponse *ev_response = (GConnHttpEventResponse*)event;
			guint                   n;
			
			g_print("RESPONSE: %u\n", ev_response->response_code); 
			for (n = 0;  ev_response->header_fields[n] != NULL;  ++n)
			{
				g_print("HEADER: %s: %s\n", 
				        ev_response->header_fields[n], 
				        ev_response->header_values[n]);
			}
			g_print("\n");
		}
		break;
		
		case GNET_CONN_HTTP_DATA_PARTIAL:
		{
			GConnHttpEventData *ev_data = (GConnHttpEventData*)event;
			if (ev_data->content_length > 0)
				g_print("PARTIAL DATA: %" G_GUINT64_FORMAT " bytes (%.1f%%)\n", ev_data->data_received,
				        ((gfloat)ev_data->data_received/(gfloat)ev_data->content_length) * 100.0);
			else
				g_print("PARTIAL DATA: %" G_GUINT64_FORMAT " bytes (content length unknown)\n", 
				        ev_data->data_received);
		}
		break;
		
		case GNET_CONN_HTTP_DATA_COMPLETE:
		{
			GConnHttpEventData *ev_data = (GConnHttpEventData*)event;
			g_print("\n");
			g_print("COMPLETE:  %" G_GUINT64_FORMAT " bytes received.\n", ev_data->data_received);
		}
		break;

		case GNET_CONN_HTTP_REDIRECT:
		{
			GConnHttpEventRedirect *ev_redir = (GConnHttpEventRedirect*)event;
			if (!ev_redir->auto_redirect)
				g_print("REDIRECT: New location => '%s' (not automatic)\n", ev_redir->new_location);
			else
				g_print("REDIRECT: New location => '%s' (automatic)\n", ev_redir->new_location);
		}
		break;
		
		case GNET_CONN_HTTP_CONNECTED:
			g_print("CONNECTED\n");
			break;

		/* Internal GConnHttp error */
		case GNET_CONN_HTTP_ERROR:
		{
			GConnHttpEventError *ev_error = (GConnHttpEventError*)event;
			g_print("ERROR #%u: %s.\n", ev_error->code, ev_error->message);
		}
		break;
		
		case GNET_CONN_HTTP_TIMEOUT:
		{
			g_print("GNET_CONN_HTTP_TIMEOUT.\n");
		}
		break;
                
		default:
			g_assert_not_reached();
	}
}

/***************************************************************************
 *
 *   test_get
 *
 ***************************************************************************/

static gboolean
test_get (const gchar *uri)
{
	GConnHttp   *httpconn;
	gchar       *buf;
	gsize        len;
	
	httpconn = gnet_conn_http_new();

	g_print ("\n=====> Testing GET \n");
		
	if (!gnet_conn_http_set_uri (httpconn, uri))
		return FALSE;

	if (!gnet_conn_http_run (httpconn, http_callback, NULL))
	{
		g_print("\t * GET operation failed.\n");
		return FALSE;
	}
		
	gnet_conn_http_steal_buffer(httpconn, &buf, &len);
	
	g_print("\t * GET operation ok: received %u bytes.\n", (guint)len);

	gnet_conn_http_delete(httpconn);
	g_free(buf);
	
	return (len > 0);
}

/***************************************************************************
 *
 *   test_get_binary
 *
 ***************************************************************************/

static gboolean
test_get_binary (const gchar *uri)
{
	GConnHttp   *httpconn;
	gchar       *buf;
	gsize        len;
	
	httpconn = gnet_conn_http_new();

	g_print ("\n=====> Testing binary GET \n");
		
	if (!gnet_conn_http_set_uri (httpconn, uri))
		return FALSE;

	if (!gnet_conn_http_run (httpconn, NULL, NULL))
	{
		g_print("\t * GET operation failed.\n");
		return FALSE;
	}
	
	gnet_conn_http_steal_buffer(httpconn, &buf, &len);
	
	g_print("\t * GET operation ok: received %u bytes.\n", (guint)len);

#ifdef CONN_HTTP_GTK_DEMO
	if (1)
	{
		GtkWidget   *win, *img;
		gchar       *fn;
		gint         fd;
	
		/* write image data to temp file */
		fd = g_file_open_tmp(NULL, &fn, NULL);
		if (!fd)
		{
			g_print("Could not create temp file: %s\n", g_strerror(errno));
			return FALSE;
		}
		
		if (write(fd, buf, len) != len)
		{
			g_print("Could not write to temp file: %s\n", g_strerror(errno));
			return FALSE;
		}
		
		close(fd);
	
		gtk_init(0, NULL);
	
		win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
		img = gtk_image_new_from_file(fn);
		gtk_container_add(GTK_CONTAINER(win), img);
		gtk_widget_show_all(win);
		g_signal_connect(win, "delete-event", G_CALLBACK(gtk_main_quit), NULL);
	
		gtk_main();
		unlink(fn);
		g_free(fn);
		return TRUE;
	}

#else

	if (!buf || len < 8 || strncmp(buf, "\211PNG\r\n\032\n", 8) != 0)
	{
		g_print ("\t * missing PNG signature?!\n");
		return FALSE;
	}
	
	g_print ("\t * PNG signature ok\n");

	if (len < (8+(4+4)) || strncmp(buf+8+4, "IHDR", 4) != 0)
	{
		g_print ("\t * missing IHDR chunk?!\n");
		return FALSE;
	}
	
	g_print ("\t * image width x height = %u x %u\n",
	         GUINT32_FROM_BE(*((guint32*)(buf+8+4+4))),
	         GUINT32_FROM_BE(*((guint32*)(buf+8+4+4+4))));
	
#endif
	
	gnet_conn_http_delete(httpconn);
	g_free(buf);
	return TRUE;
}

/***************************************************************************
 *
 *   test_redirect
 *
 ***************************************************************************/

static gboolean
test_redirect (const gchar *uri)
{
	GConnHttp   *httpconn;
	gchar       *buf;
	gsize        len;
	
	httpconn = gnet_conn_http_new();

	g_print ("\n=====> Testing GET with redirect \n");
		
	if (!gnet_conn_http_set_uri (httpconn, uri))
		return FALSE;

	if (!gnet_conn_http_run (httpconn, http_callback, NULL))
	{
		g_print("\t * GET operation with redirect failed.\n");
		return FALSE;
	}
		
	gnet_conn_http_steal_buffer(httpconn, &buf, &len);
	
	g_print("\t * GET operation with redirect ok: received %u bytes.\n", (guint)len);
	
	gnet_conn_http_delete(httpconn);
	g_free(buf);
	
	return TRUE;
}

/***************************************************************************
 *
 *   test_simple_get
 *
 *   Tests gnet_http_get()
 *
 ***************************************************************************/

static gboolean
test_simple_get (const gchar *url)
{
	gboolean  ret;
	gchar    *buf = NULL;
	gsize     buflen = 0;
	guint     code = 0;                

	g_print ("\n=====> Testing gnet_conn_http_run(): \n");

	ret = gnet_http_get (url, &buf, &buflen, &code);
	
	if (ret == FALSE)
	{
		g_print("\t * gnet_http_get() failed (buflen = %u, code = %u).\n", buflen, code);
		return FALSE;                
	}
        
	g_print("\t * gnet_http_get() succeded (buflen = %u, code = %u).\n", buflen, code);

	g_free (buf);
	
	return TRUE;
}

/***************************************************************************
 *
 *   test_post
 *
 ***************************************************************************/

static gboolean
test_post (const gchar *artist, const gchar *album)
{
	GConnHttp   *http;
	gchar       *postdata, *buf, *tag, *artist_esc, *album_esc;
	gsize        buflen;
	
	g_print ("\n=====> Testing POST \n");

	/* g_markup_printf_escaped() only exists in Glib-2.4 and later */
	artist_esc = g_markup_escape_text (artist, -1);
	album_esc = g_markup_escape_text (album, -1);
	postdata = g_strdup_printf (
	                "<?xml version=\"1.0\" encoding=\"utf-8\"?>"
	                "<soap:Envelope xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\""
	                " xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\""
	                " xmlns:soap=\"http://schemas.xmlsoap.org/soap/envelope/\">"
	                "<soap:Body>"
	                "<q1:ArtistSearchRequest xmlns:q1=\"http://soap.amazon.com\">"
	                "<ArtistSearchRequest href=\"#id1\" />"
	                "</q1:ArtistSearchRequest>"
	                "<q2:ArtistRequest id=\"id1\" xsi:type=\"q2:ArtistRequest\" xmlns:q2=\"http://soap.amazon.com\">"
	                "<artist xsi:type=\"xsd:string\">%s</artist>" /* <----------- */
	                "<page xsi:type=\"xsd:string\">1</page>"
	                "<mode xsi:type=\"xsd:string\">music</mode>"
	                "<tag xsi:type=\"xsd:string\">webservices-20</tag>"
	                "<type xsi:type=\"xsd:string\">lite</type>"
	                "<devtag xsi:type=\"xsd:string\"></devtag>"
	                "<keywords xsi:type=\"xsd:string\">%s</keywords>" /* <----------- */
	                "</q2:ArtistRequest>"
	                "</soap:Body>"
	                "</soap:Envelope>",
	                artist_esc, album_esc);

	g_free (artist_esc);
	g_free (album_esc);

	http = gnet_conn_http_new();

	if (!gnet_conn_http_set_uri (http, "http://soap.amazon.com/onca/soap3"))
		return FALSE;

	if (!gnet_conn_http_set_method (http, GNET_CONN_HTTP_METHOD_POST, postdata, strlen(postdata)))
		return FALSE;

	if (!gnet_conn_http_set_header (http, "SOAPAction", 
	                                "\"http://soap.amazon.com\"", 
	                                GNET_CONN_HTTP_FLAG_SKIP_HEADER_CHECK))
	{
		g_printerr("gnet_conn_http_set_header(SOAPAction) failed.\n");
		return FALSE;
	}
	             
	if (!gnet_conn_http_set_header (http, "Content-Type", 
	                                "text/xml; charset=utf-8", 
	                                GNET_CONN_HTTP_FLAG_SKIP_HEADER_CHECK))
	{
		g_printerr("gnet_conn_http_set_header(Content-Type) failed.\n");
		return FALSE;
	}


	if (!gnet_conn_http_run (http, http_callback, postdata))
	{
		g_print("\t * POST operation failed.\n");
		return FALSE;
	}
	
	gnet_conn_http_steal_buffer(http, &buf, &buflen);
	g_print("\t * POST operation ok: received %u bytes.\n", (guint)buflen);
		
	tag = strstr(buf, "</ListPrice>");
	if (tag)
	{
		*tag = 0x00;
		while (tag > buf && *tag != '>')
			--tag;
		++tag;
	}
		
	if (tag && *tag == '$')
		g_print ("\t * %s, %s - CD Album: ListPrice = %s\n", artist, album, tag);
	else
		g_print ("\t * oops, could not find ListPrice for CD Album %s, %s in data returned :|\n", artist, album);

	gnet_conn_http_delete(http);
	g_free(postdata);
	g_free(buf);
	return TRUE;
}

/***************************************************************************
 *
 *   foo
 *
 *   Without this, the main loop doesn't want to get going ?!
 *
 ***************************************************************************/

static gboolean
foo (gpointer baz)
{
	return TRUE;
}


/***************************************************************************
 *
 *   main
 *
 ***************************************************************************/

int
main (int argc, char **argv)
{
	verbose = FALSE;

	/* DIRTY HACK. Kids, don't do this at home */
	if (argc > 1  &&  (strcmp(argv[1], "--verbose") == 0 || strcmp(argv[1],"-v") == 0))
	{
		verbose = TRUE;
		argc--;
		argv[1] = argv[2];
	}
        
	/* without this, the GLib mainloop 
	 * doesn't seem to get started?! */
	g_timeout_add(50, foo, NULL); 

	if (argc > 1)
	{
		gboolean ok;
		 
		ok = test_get(argv[1]);
	
		g_print ("------------------------------------------------------------\n");
		g_print ("GET (%s):  %s\n", argv[1], (ok) ? "OK" : "FAILED");
		g_print ("------------------------------------------------------------\n");
		
		g_assert (ok == TRUE);
        }
	else
	{
		gboolean  get_ok, binget_ok, post_ok, redir1_ok, redir2_ok, urlget_ok;
		
		get_ok = test_get("http://www.google.com");

		post_ok = test_post ("Massive Attack", "Blue Lines");
		
		binget_ok = test_get_binary ("http://www.gnetlibrary.org/gnet.png");

		/* Test redirect to different host */
		redir1_ok = test_redirect ("http://www.amazon.com");
	
		/* Test redirect to different URL on same host */
		redir2_ok = test_redirect ("http://sf.net");
                
		/* Test gnet_http_get() */
		urlget_ok = test_simple_get ("http://www.gnetlibrary.org/src/"); 

		g_print ("------------------------------------------------------------\n");
		g_print ("GET (html)                 %s\n", (get_ok)    ? "OK" : "FAILED");
		g_print ("GET (binary)               %s\n", (binget_ok) ? "OK" : "FAILED");
		g_print ("POST                       %s\n", (post_ok)   ? "OK" : "FAILED");
		g_print ("Redirect (same host)       %s\n", (redir1_ok) ? "OK" : "FAILED");
		g_print ("Redirect (different host)  %s\n", (redir2_ok) ? "OK" : "FAILED");
		g_print ("gnet_http_get()            %s\n", (urlget_ok) ? "OK" : "FAILED");
		g_print ("------------------------------------------------------------\n");
		
                g_assert (get_ok && binget_ok && post_ok && redir1_ok && redir2_ok && urlget_ok);
	}
		
	return EXIT_SUCCESS;
}

