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
    gchar* user;	
    gchar* password;
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
    {"scheme", NULL, NULL, NULL, 0, NULL, NULL, NULL}},

  { "scheme:path", NULL, 
    {"scheme", NULL, NULL, NULL, 0, "path", NULL, NULL}},

  { "path", NULL, 
    {NULL, NULL, NULL, NULL, 0, "path", NULL, NULL}},

  { "/path", NULL, 
    {NULL, NULL, NULL, NULL, 0, "/path", NULL, NULL}},

  /* hostname/port */
  { "scheme://hostname/path", NULL, 
    {"scheme", NULL, NULL, "hostname", 0, "/path", NULL, NULL}},

  { "scheme://hostname:123/path", NULL, 
    {"scheme", NULL, NULL, "hostname", 123, "/path", NULL, NULL}},

  /* query/fragment */
  { "path?query", NULL, 
    {NULL, NULL, NULL, NULL, 0, "path", "query", NULL}},

  { "path?query#fragment", NULL, 
    {NULL, NULL, NULL, NULL, 0, "path", "query", "fragment"}},

  { "scheme:path?query#fragment", NULL, 
    {"scheme", NULL, NULL, NULL, 0, "path", "query", "fragment"}},

  /* full */
  { "scheme://hostname:123/path?query#fragment", NULL, 
    {"scheme", NULL, NULL, "hostname", 123, "/path", "query", "fragment"}},

  /* user/pass */
  { "scheme://user@hostname", NULL, 
    {"scheme", "user", NULL, "hostname", 0, NULL, NULL, NULL }},

  { "scheme://user@hostname:123/path?query#fragment", NULL, 
    {"scheme", "user", NULL, "hostname", 123, "/path", "query", "fragment"}},

  { "scheme://user:pass@hostname", NULL, 
    {"scheme", "user", "pass", "hostname", 0, NULL, NULL, NULL}},

  { "scheme://user:pass@hostname:123/path?query#fragment", NULL, 
    {"scheme", "user", "pass", "hostname", 123, "/path", "query", "fragment"}},

  /* FUNNY URIS.  PARSING AND PRINTING OF THESE MAY CHANGE */

  /* empty */
  { "", NULL, 
    {NULL, NULL, NULL, NULL, 0, NULL, NULL, NULL} },

  { "scheme://hostname:123path?query#fragment", 
    "scheme://hostname:123/path?query#fragment",  /* PRETTY */
    {"scheme", NULL, NULL, "hostname", 123, "path", "query", "fragment"}},

  { "scheme:hostname:123/path?query#fragment", NULL, 
    {"scheme", NULL, NULL, NULL, 0, "hostname:123/path", "query", "fragment"}},

  { "scheme://:pass@hostname:123/path?query#fragment", NULL, 
    {"scheme", "", "pass", "hostname", 123, "/path", "query", "fragment"}},

  { NULL, NULL, {NULL, NULL, NULL, NULL, 0, NULL, NULL, NULL} }
};

#define SAFESTRCMP(A,B) (((A)&&(B))?(strcmp((A),(B))):((A)||(B)))

int
main (int argc, char* argv[])
{
  int i;

  gnet_init ();

  for (i = 0; tests[i].str; ++i)
    {
      GURI* uri;
      gchar* pretty;

      uri = gnet_uri_new (tests[i].str);
      TEST (i, "gnet_uri_new", uri != NULL);
      if (!uri) continue;

      pretty = gnet_uri_get_nice_string (uri);
      TEST (i, "gnet_uri_get_nice_string", pretty);
      if (!pretty) continue;
      TEST (i, pretty, TRUE);

      if (tests[i].pretty)
	TEST (i, "pretty1", !strcmp (pretty, tests[i].pretty));
      else
	TEST (i, "pretty2", !strcmp (pretty, tests[i].str));
      g_free (pretty);

      TEST (i, "scheme",   !SAFESTRCMP(uri->scheme,   tests[i].uri.scheme));
      TEST (i, "user",     !SAFESTRCMP(uri->user,     tests[i].uri.user));
      TEST (i, "password", !SAFESTRCMP(uri->password, tests[i].uri.password));
      TEST (i, "hostname", !SAFESTRCMP(uri->hostname, tests[i].uri.hostname));
      TEST (i, "port",     uri->port == tests[i].uri.port);
      TEST (i, "path",     !SAFESTRCMP(uri->path,     tests[i].uri.path));
      TEST (i, "query",    !SAFESTRCMP(uri->query,    tests[i].uri.query));
      TEST (i, "fragment", !SAFESTRCMP(uri->fragment, tests[i].uri.fragment));

      gnet_uri_delete (uri);
    }

  if (failed)
    exit (1);

  exit (0);

}
