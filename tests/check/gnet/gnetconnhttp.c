/* GNet GConnHttp unit test
 * Copyright (C) 2004, 2007 Tim-Philipp Müller <tim centricular net>
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

#define GNET_EXPERIMENTAL 1

#include "config.h"
#include "gnetcheck.h"

#include <string.h>       

static gboolean verbose; /* FALSE */

/* Print some information along the way (only for debugging purposes) */
static void
http_dbg_callback (GConnHttp *http, GConnHttpEvent *event, gpointer baz)
{
  if (!verbose)
    return;

  switch (event->type)
  {
    case GNET_CONN_HTTP_RESOLVED:
    {
      GConnHttpEventResolved *ev_resolved = (GConnHttpEventResolved*)event;
      if (ev_resolved->ia) {
        gchar *cname;

        cname = gnet_inetaddr_get_canonical_name (ev_resolved->ia);
        g_printerr ("RESOLVED: %s\n\n", cname);
        g_free (cname);
      } else {
        g_printerr ("RESOLVED: FAILED TO RESOLVE HOSTNAME.\n");
      }
    }
    break;

    case GNET_CONN_HTTP_RESPONSE:
    {
      GConnHttpEventResponse *ev_response = (GConnHttpEventResponse*)event;
      guint                   n;
      
      g_printerr ("RESPONSE: %u\n", ev_response->response_code); 
      for (n = 0;  ev_response->header_fields[n] != NULL;  ++n)
      {
        g_printerr ("HEADER: %s: %s\n", 
                ev_response->header_fields[n], 
                ev_response->header_values[n]);
      }
      g_printerr ("\n");
    }
    break;
    
    case GNET_CONN_HTTP_DATA_PARTIAL:
    {
      GConnHttpEventData *ev_data = (GConnHttpEventData*)event;
      if (ev_data->content_length > 0)
        g_printerr ("PARTIAL DATA: %" G_GUINT64_FORMAT " bytes (%.1f%%)\n", ev_data->data_received,
                ((gfloat)ev_data->data_received/(gfloat)ev_data->content_length) * 100.0);
      else
        g_printerr ("PARTIAL DATA: %" G_GUINT64_FORMAT " bytes (content length unknown)\n", 
                ev_data->data_received);
    }
    break;
    
    case GNET_CONN_HTTP_DATA_COMPLETE:
    {
      GConnHttpEventData *ev_data = (GConnHttpEventData*)event;
      g_printerr ("\n");
      g_printerr ("COMPLETE:  %" G_GUINT64_FORMAT " bytes received.\n", ev_data->data_received);
    }
    break;

    case GNET_CONN_HTTP_REDIRECT:
    {
      GConnHttpEventRedirect *ev_redir = (GConnHttpEventRedirect*)event;
      if (!ev_redir->auto_redirect)
        g_printerr ("REDIRECT: New location => '%s' (not automatic)\n", ev_redir->new_location);
      else
        g_printerr ("REDIRECT: New location => '%s' (automatic)\n", ev_redir->new_location);
    }
    break;
    
    case GNET_CONN_HTTP_CONNECTED:
      g_printerr ("CONNECTED\n");
      break;

    /* Internal GConnHttp error */
    case GNET_CONN_HTTP_ERROR:
    {
      GConnHttpEventError *ev_error = (GConnHttpEventError*)event;
      g_printerr ("ERROR #%u: %s.\n", ev_error->code, ev_error->message);
    }
    break;
    
    case GNET_CONN_HTTP_TIMEOUT:
    {
      g_printerr ("GNET_CONN_HTTP_TIMEOUT.\n");
    }
    break;
                
    default:
      g_assert_not_reached();
  }
}

GNET_START_TEST (test_conn_http_run)
{
  /* at least some of these should lead to a redirect; sf.net is a redirect
   * to the same host */
  const gchar *uris[] = { "http://www.google.com", "http://www.google.co.uk",
      "http://www.google.de", "http://www.google.fr", "http://www.google.es",
      "http://www.gnome.org", "http://www.amazon.com", "http://sf.net" };
  guint i;

  for (i = 0; i < G_N_ELEMENTS (uris); ++i) {
    const gchar *uri = uris[i];
    GConnHttp *httpconn;
    gchar *buf;
    gsize len;
  
    httpconn = gnet_conn_http_new ();
    fail_unless (httpconn != NULL);
    
    fail_unless (gnet_conn_http_set_uri (httpconn, uri),
        "Couldn't set URI '%s' on GConnHttp", uri);

    g_print ("Getting %s ... ", uri);

    fail_unless (gnet_conn_http_run (httpconn, http_dbg_callback, NULL));
    
    gnet_conn_http_steal_buffer (httpconn, &buf, &len);
    g_print ("%s, received %d bytes.\n", (len > 0) ? "ok" : "FAILED", (int) len);
    fail_unless (len > 0);
    fail_unless (buf != NULL);

    gnet_conn_http_delete (httpconn);
    g_free (buf);
  }
}
GNET_END_TEST;

/*** GET ASYNC ***/

typedef struct {
  GMainLoop *loop;
  gboolean got_error;
  guint64 len;
} AsyncHelper;

static void
http_async_callback (GConnHttp *http, GConnHttpEvent *event, gpointer userdata)
{
  AsyncHelper *helper = (AsyncHelper *) userdata;

  if (verbose) {
    g_printerr ("%s: event->type = %u\n", __FUNCTION__, event->type);
    http_dbg_callback (http, event, NULL);
  }

  switch (event->type)
  {
    case GNET_CONN_HTTP_RESOLVED:
    case GNET_CONN_HTTP_RESPONSE:
    case GNET_CONN_HTTP_REDIRECT:
    case GNET_CONN_HTTP_CONNECTED:
      break;
    
    case GNET_CONN_HTTP_DATA_PARTIAL:
    {
      GConnHttpEventData *data_event = (GConnHttpEventData*) event;

      /* we received data, make sure these values are set */
      g_assert (data_event->buffer != NULL);
      g_assert (data_event->buffer_length > 0);
      g_assert (data_event->data_received > 0);
      helper->len = data_event->data_received;
      break;
    }

    case GNET_CONN_HTTP_ERROR:
    {
      GConnHttpEventError *err_event = (GConnHttpEventError*) event;
      if (verbose) {
        g_printerr ("Error: %s (code=%u)\n", err_event->message,
            err_event->code);
      }
      helper->got_error = TRUE;
    }
    /* fallthrough */

    case GNET_CONN_HTTP_DATA_COMPLETE:
    case GNET_CONN_HTTP_TIMEOUT:
      if (verbose)
        g_printerr ("%s: done. Deleting http object.\n", __FUNCTION__);
      /* make sure we can call _delete() from the callback */
      gnet_conn_http_delete (http);
      g_main_loop_quit (helper->loop);
      break;
                
    default:
      g_assert_not_reached();
  }
}

/* This mainly tests whether we can do gnet_conn_http_delete()
 * from within the async callback when we have the data.
*/
static gboolean
test_get_async (const gchar *uri)
{
  AsyncHelper helper;
  GConnHttp *httpconn;

  httpconn = gnet_conn_http_new ();

  if (!gnet_conn_http_set_uri (httpconn, uri)) {
    g_printerr ("Couldn't set URI '%s' on GConnHttp", uri);
    return FALSE;
  }

  g_print ("Getting (async) '%s' ... ", uri);

  helper.loop = g_main_loop_new (NULL, FALSE);

  helper.got_error = FALSE;
  helper.len = 0;

  gnet_conn_http_run_async (httpconn, http_async_callback, &helper);

  g_main_loop_run (helper.loop);

  g_print ("done, received %u bytes.\n", (guint) helper.len);

  g_main_loop_unref (helper.loop);
  helper.loop = NULL;

  /* http conn object has been deleted in callback */

  return (!helper.got_error);
}

GNET_START_TEST (test_conn_http_get_async)
{
  /* If this test returns, it went ok (check that an error event gets sent).
   * It should return FALSE though, since we should have gotten an error */
  fail_if (test_get_async ("http://non-exist.ant"));

  /* If this test returns and doesn't crash, it went ok
   * (the httpconn is deleted from within callback, which should work) */
  fail_unless (test_get_async ("http://www.google.com"));
  fail_unless (test_get_async ("http://www.google.co.uk"));
  fail_unless (test_get_async ("http://www.google.de"));
  fail_unless (test_get_async ("http://www.google.fr"));
  fail_unless (test_get_async ("http://www.google.es"));
}
GNET_END_TEST;

/*** POST ***/

/* PLEASE do not use this developer ID in 
 *  your own code, get your own one */
#define AWS_DEV_ID "1BXDDWFYXTVWC8WQXF02"
#define POST_ARTIST "Massive Attack"
#define POST_ALBUM "Blue lines"

GNET_START_TEST (test_conn_http_post)
{
  GConnHttp   *http;
  gchar       *postdata, *buf, *tag, *artist_esc, *album_esc;
  gsize        buflen;
  
  /* g_markup_printf_escaped() only exists in Glib-2.4 and later */
  artist_esc = g_markup_escape_text (POST_ARTIST, -1);
  album_esc = g_markup_escape_text (POST_ALBUM, -1);
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
                  "<devtag xsi:type=\"xsd:string\">%s</devtag>"
                  "<keywords xsi:type=\"xsd:string\">%s</keywords>" /* <----------- */
                  "</q2:ArtistRequest>"
                  "</soap:Body>"
                  "</soap:Envelope>",
                  artist_esc, AWS_DEV_ID, album_esc);

  g_free (artist_esc);
  g_free (album_esc);

  http = gnet_conn_http_new();

  fail_unless (gnet_conn_http_set_uri (http,
      "http://soap.amazon.com/onca/soap3"));

  fail_unless (gnet_conn_http_set_method (http, GNET_CONN_HTTP_METHOD_POST,
      postdata, strlen(postdata)));

  fail_unless (gnet_conn_http_set_header (http, "SOAPAction", 
      "\"http://soap.amazon.com\"", GNET_CONN_HTTP_FLAG_SKIP_HEADER_CHECK));
               
  fail_unless (gnet_conn_http_set_header (http, "Content-Type", 
      "text/xml; charset=utf-8", GNET_CONN_HTTP_FLAG_SKIP_HEADER_CHECK));


  fail_unless (gnet_conn_http_run (http, http_dbg_callback, postdata));
  
  gnet_conn_http_steal_buffer(http, &buf, &buflen);
  g_print ("POST operation ok, received %u bytes.\n", (guint) buflen);

  /* now parse result */
  tag = strstr(buf, "</ListPrice>");
  if (tag)
  {
    *tag = 0x00;
    while (tag > buf && *tag != '>')
      --tag;
    ++tag;
  }
    
  if (tag && *tag == '$')
    g_print ("%s, %s - ListPrice = %s\n", POST_ARTIST, POST_ALBUM, tag);
  else
    g_print ("Could not find ListPrice for CD %s, %s in data returned :|\n",
        POST_ARTIST, POST_ALBUM);

  gnet_conn_http_delete(http);
  g_free(postdata);
  g_free(buf);
}
GNET_END_TEST;

GNET_START_TEST (test_gnet_http_get)
{
  const gchar *urls[] = {"http://www.gnetlibrary.org/src/",
      "http://www.heise.de" };
  guint i;

  for (i = 0; i < G_N_ELEMENTS (urls); ++i) {
    gchar *buf = NULL;
    gsize  buflen = 0;
    guint  code = 0;                

    g_print ("gnet_http_get %s ... ", urls[i]);

    fail_unless (gnet_http_get (urls[i], &buf, &buflen, &code),
        "buflen = %u, code = %u", (guint) buflen, code);
  
    g_print ("done - buflen = %u, code = %u\n", (guint) buflen, code);
    fail_unless (code == 200);
    fail_unless (buflen > 0);
    fail_unless (buf != NULL);
    /* make sure we can access it */
    fail_unless (buf[buflen-1] == 0 || buf[buflen-1] != 0);
    g_free (buf);
  }
}
GNET_END_TEST;

GNET_START_TEST (test_get_binary)
{
  gchar *uris[] = { "http://www.gnetlibrary.org/gnet.png" };
  guint i;

  for (i = 0; i < G_N_ELEMENTS (uris); ++i) {
    GConnHttp *httpconn;
    gchar     *buf;
    gsize      len;
  
    httpconn = gnet_conn_http_new ();

    g_print ("Getting binary file %s ... ", uris[i]);
    
    fail_unless (gnet_conn_http_set_uri (httpconn, uris[i]),
        "Can't set URI '%s' on GConnHttp!", uris[i]);

    fail_unless (gnet_conn_http_run (httpconn, NULL, NULL));
  
    gnet_conn_http_steal_buffer (httpconn, &buf, &len);
    fail_unless (len > 0);
    fail_unless (buf != NULL);
    /* check that we can access the data */
    fail_unless (buf[len-1] == 0 || buf[len-1] != 0);
  
    g_print ("ok, received %u bytes.\n", (guint) len);

    /* now check the data we've received, should be PNG */
    if (!buf || len < 8 || strncmp(buf, "\211PNG\r\n\032\n", 8) != 0)
      g_error ("missing PNG signature?!");
   
    if (len < (8+(4+4)) || strncmp(buf+8+4, "IHDR", 4) != 0)
      g_error ("missing IHDR chunk?!");
  
    g_print ("PNG image width x height = %u x %u\n",
             GUINT32_FROM_BE(*((guint32*)(buf+8+4+4))),
             GUINT32_FROM_BE(*((guint32*)(buf+8+4+4+4))));

    gnet_conn_http_delete(httpconn);
    g_free(buf);
  }
}
GNET_END_TEST;

static Suite *
gnetconnhttp_suite (void)
{
  Suite *s = suite_create ("GConnHttp");
  TCase *tc_chain = tcase_create ("connhttp");

  verbose = (g_getenv ("GNET_DEBUG") != NULL);

  tcase_set_timeout (tc_chain, 0);

  suite_add_tcase (s, tc_chain);
  tcase_add_test (tc_chain, test_conn_http_run);
  tcase_add_test (tc_chain, test_conn_http_get_async);
  tcase_add_test (tc_chain, test_conn_http_post);
  tcase_add_test (tc_chain, test_gnet_http_get);
  tcase_add_test (tc_chain, test_get_binary);
  return s;
}

GNET_CHECK_MAIN (gnetconnhttp);

