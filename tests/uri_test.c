/* Parse various URIs
 * Copyright (C) 2001  David Helder
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

#include <glib.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gnet.h>

static int failed = 0;

#define TEST(N, S, C) do {                             \
if (C) { /*g_print ("%d %s: PASS\n", (N), (S)); */        } \
else   { g_print ("%d %s: FAIL\n", (N), (S)); failed = 1; } \
} while (0)

struct URITest
{
  gchar* str;
  gchar* pretty;
  struct
  {
    gchar* scheme;
    gchar* userinfo;	
    gchar* hostname;
    gint   port;
    gchar* path;
    gchar* query;
    gchar* fragment;
  } uri;
};

struct URITest tests[] = 
{
  /* VALID URIS.  PARSING AND PRINTING OF THESE SHOULD NOT CHANGE */

  /* scheme/path */
  { "scheme:", NULL, 
    {"scheme", NULL, NULL, 0, NULL, NULL, NULL}},

  { "scheme:path", NULL, 
    {"scheme", NULL, NULL, 0, "path", NULL, NULL}},

  { "path", NULL, 
    {NULL, NULL, NULL, 0, "path", NULL, NULL}},

  { "/path", NULL, 
    {NULL, NULL, NULL, 0, "/path", NULL, NULL}},

  /* hostname/port */
  { "scheme://hostname/path", NULL, 
    {"scheme", NULL, "hostname", 0, "/path", NULL, NULL}},

  { "scheme://hostname:123/path", NULL, 
    {"scheme", NULL, "hostname", 123, "/path", NULL, NULL}},

  /* ipv6 hostname/port */
  { "scheme://[01:23:45:67:89:ab:cd:ef]/path", NULL, 
    {"scheme", NULL, "01:23:45:67:89:ab:cd:ef", 0, "/path", NULL, NULL}},

  { "scheme://[01:23:45:67:89:ab:cd:ef]:123/path", NULL, 
    {"scheme", NULL, "01:23:45:67:89:ab:cd:ef", 123, "/path", NULL, NULL}},

  /* query/fragment */
  { "path?query", NULL, 
    {NULL, NULL, NULL, 0, "path", "query", NULL}},

  { "path?query#fragment", NULL, 
    {NULL, NULL, NULL, 0, "path", "query", "fragment"}},

  { "scheme:path?query#fragment", NULL, 
    {"scheme", NULL, NULL, 0, "path", "query", "fragment"}},

  /* full */
  { "scheme://hostname:123/path?query#fragment", NULL, 
    {"scheme", NULL, "hostname", 123, "/path", "query", "fragment"}},

  /* user/pass */
  { "scheme://userinfo@hostname", NULL, 
    {"scheme", "userinfo", "hostname", 0, NULL, NULL, NULL }},

  { "scheme://userinfo@hostname:123/path?query#fragment", NULL, 
    {"scheme", "userinfo", "hostname", 123, "/path", "query", "fragment"}},

  { "scheme://user:pass@hostname", NULL, 
    {"scheme", "user:pass", "hostname", 0, NULL, NULL, NULL}},

  { "scheme://user:pass@hostname:123/path?query#fragment", NULL, 
    {"scheme", "user:pass", "hostname", 123, "/path", "query", "fragment"}},

  /* FUNNY URIS.  PARSING AND PRINTING OF THESE MAY CHANGE */

  { "scheme://hostname:123path?query#fragment", 
    "scheme://hostname:123/path?query#fragment",  /* PRETTY */
    {"scheme", NULL, "hostname", 123, "path", "query", "fragment"}},

  { "scheme:hostname:123/path?query#fragment", NULL, 
    {"scheme", NULL, NULL, 0, "hostname:123/path", "query", "fragment"}},

  { "scheme://:pass@hostname:123/path?query#fragment", NULL, 
    {"scheme", ":pass", "hostname", 123, "/path", "query", "fragment"}},

  /* IPv6 hostname without brackets */
  { "scheme://01:23:45:67:89:ab:cd:ef:123/path", 
    "scheme://01:23/:45:67:89:ab:cd:ef:123/path",  /* PRETTY */
    {"scheme", NULL, "01", 23, ":45:67:89:ab:cd:ef:123/path", NULL, NULL}},

  /* Brackets that don't close - hostname will be everything */
  { "scheme://[01:23:45:67:89:ab:cd:ef:123/path", 
    "scheme://[01:23:45:67:89:ab:cd:ef:123/path]",
    {"scheme", NULL, "01:23:45:67:89:ab:cd:ef:123/path", 0, NULL, NULL, NULL}},

  /* Skip initial white space */
  { " \f\n\r\t\vscheme:", "scheme:", 
    {"scheme", NULL, NULL, 0, NULL, NULL, NULL}},

  { " \f\n\r\t\vpath", "path",
    {NULL, NULL, NULL, 0, "path", NULL, NULL}},


  { NULL, NULL, {NULL, NULL, NULL, 0, NULL, NULL, NULL} }

};


struct EscapeTest
{
  gchar* escaped;
  gchar* unescaped;
  gchar* escaped2;
};

struct EscapeTest escape_tests[] = 
{
  { "http://userinfo@www.example.com:80/path?query#fragment", 
    "http://userinfo@www.example.com:80/path?query#fragment" , NULL},
  { "http://userinfo@www.example.com:80/~path?query#fragment", 
    "http://userinfo@www.example.com:80/~path?query#fragment" , NULL},
  { "http://%5euser%5einfo%5e@www.example.com:80/~%5epa%5eth%5e?%5equ%5eery%5e#%5efra%5egment%5e", 
    "http://^user^info^@www.example.com:80/~^pa^th^?^qu^ery^#^fra^gment^" , NULL},
  { "http://%5e%5e%5euser%5e%5e%5einfo%5e%5e%5e@www.example.com:80/~%5e%5e%5epa%5e%5e%5eth%5e%5e%5e?%5e%5e%5equ%5e%5e%5eery%5e%5e%5e#%5e%5e%5efra%5e%5e%5egment%5e%5e%5e", 
    "http://^^^user^^^info^^^@www.example.com:80/~^^^pa^^^th^^^?^^^qu^^^ery^^^#^^^fra^^^gment^^^" , NULL},
  { "http://user%40info@www.example.com:80/path?query#fragment", 
    "http://user@info@www.example.com:80/path?query#fragment" , NULL},
  { "http://user%40info@www.example.com:80/path?query#fragment", 
    "http://user@info@www.example.com:80/path?query#fragment" , NULL},

  { "http://www.example.com/pa%th", "http://www.example.com/pa%th",
    "http://www.example.com/pa%25th"},
  { NULL, NULL , NULL}
};


#define SAFESTRCMP(A,B) (((A)&&(B))?(strcmp((A),(B))):((A)||(B)))

int
main (int argc, char* argv[])
{
  int i;

  gnet_init ();

  /* Empty string is an error */
  if (gnet_uri_new("") != NULL)
    {
      g_print ("empty string is error: FAIL\n");
      failed = 1;
    }

  /* String of whitespace is an error */
  if (gnet_uri_new(" \n\t\r") != NULL)
    {
      g_print ("whitespace string is error: FAIL\n");
      failed = 1;
    }

  for (i = 0; tests[i].str; ++i)
    {
      GURI* uri;
      gchar* pretty;
      gchar* escape;
      gchar* unescape;

      uri = gnet_uri_new (tests[i].str);
      TEST (i, "gnet_uri_new", uri != NULL);
      if (!uri) continue;

      pretty = gnet_uri_get_string (uri);
      TEST (i, "gnet_uri_get_string", pretty);
      if (!pretty) continue;

      if (tests[i].pretty)
	TEST (i, "pretty1", !strcmp (pretty, tests[i].pretty));
      else
	TEST (i, "pretty2", !strcmp (pretty, tests[i].str));

      TEST (i, "scheme",   !SAFESTRCMP(uri->scheme,   tests[i].uri.scheme));
      TEST (i, "userinfo", !SAFESTRCMP(uri->userinfo, tests[i].uri.userinfo));
      TEST (i, "hostname", !SAFESTRCMP(uri->hostname, tests[i].uri.hostname));
      TEST (i, "port",     uri->port == tests[i].uri.port);
      TEST (i, "path",     !SAFESTRCMP(uri->path,     tests[i].uri.path));
      TEST (i, "query",    !SAFESTRCMP(uri->query,    tests[i].uri.query));
      TEST (i, "fragment", !SAFESTRCMP(uri->fragment, tests[i].uri.fragment));

      gnet_uri_escape (uri);
      escape = gnet_uri_get_string (uri);
      TEST (i, "gnet_uri_escape", escape != NULL);
/*        g_print ("%s -e-> %s\n", pretty, escape); */

      gnet_uri_unescape (uri);
      unescape = gnet_uri_get_string (uri);
      TEST (i, "gnet_uri_unescape", unescape != NULL);
/*        g_print ("%s -u-> %s\n", escape, unescape); */
      
      TEST (i, "url = unescape(escape(url))", !strcmp(pretty, unescape));

      g_free (escape);
      g_free (unescape);
      g_free (pretty);

      gnet_uri_delete (uri);
    }

  for (i = 0; escape_tests[i].escaped; ++i)
    {
      GURI* uri;
      gchar* escape;
      gchar* unescape;

      uri = gnet_uri_new (escape_tests[i].escaped);
      TEST (i, "gnet_uri_new", uri != NULL);
      if (!uri) continue;

      gnet_uri_unescape (uri);
      unescape = gnet_uri_get_string (uri);
      TEST (i, "gnet_uri_unescape", unescape != NULL);
/*        g_print ("unescape = %s\n", unescape); */

      TEST (i, "unescape is correct", 
	    !strcmp(escape_tests[i].unescaped, unescape));
      g_free (unescape);

      gnet_uri_escape (uri);
      escape = gnet_uri_get_string (uri);
      TEST (i, "gnet_uri_escape", escape != NULL);
/*        g_print ("escape = %s\n", escape); */

      if (escape_tests[i].escaped2)
	TEST (i, "escape is correct", !strcmp(escape_tests[i].escaped2, escape));
      else
	TEST (i, "escape is correct", !strcmp(escape_tests[i].escaped, escape));

      g_free (escape);
    }

  if (failed)
    exit (1);

  exit (0);

}
