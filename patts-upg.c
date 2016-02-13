/*
 *  patts-upg - Upgrade a PATTS database
 *  Copyright (C) 2016 Delwink, LLC
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License as published by
 *  the Free Software Foundation, version 3 only.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <sqon.h>
#include <stdio.h>
#include <string.h>

#define __USE_MISC
#include <unistd.h>
#include <getopt.h>

#include <limits.h>
#ifndef PASS_MAX
# define PASS_MAX 256
#endif

#include <patts/patts.h>
#include <patts/setup.h>

#ifdef HAVE_CONFIG_H
# include "config.h"
# define VERSION_STR VERSION
#else
# define VERSION_STR "Unknown version"
#endif

#define USAGE_INFO "USAGE: patts-upg [options]\n\n\
patts-upg checks the PATTS database's version, informs the user of the\n\
situation, and upgrades the database if possible.\n\n\
OPTIONS:\n\
\t-d, --database=DB\tUses the DB database.\n\
\t    --help\t\tPrints this help message and exits\n\
\t-h, --host=HOST\t\tConnects to the HOST server.\n\
\t-p, --port=PORT\t\tUses PORT for connection (overrides default)\n\
\t-v, --version\t\tPrints version info and exits\n"

#define VERSION_INFO "patts-upg " VERSION_STR "\n\
Copyright (C) 2016 Delwink, LLC\n\
License AGPLv3: GNU AGPL version 3 only <http://gnu.org/licenses/agpl.html>.\n\
This is libre software: you are free to change and redistribute it.\n\
There is NO WARRANTY, to the extent permitted by law.\n\n\
Written by David McMackins II."

static void
trim (char *s)
{
  while ('\0' != *s)
    {
      if ('\n' == *s)
	{
	  *s = '\0';
	  break;
	}

      ++s;
    }
}

int
main (int argc, char *argv[])
{
  int rc = 0;
  uint8_t db_type = PATTS_DBCONN_MYSQL;
  char *host = NULL, *user = NULL, *passwd = NULL, *database = NULL;
  char *port = "0";

  struct option longopts[] =
    {
      {"database", required_argument, 0, 'd'},
      {"help",     no_argument,       0, 'e'},
      {"host",     required_argument, 0, 'h'},
      {"port",     required_argument, 0, 'p'},
      {"user",     required_argument, 0, 'u'},
      {"version",  no_argument,       0, 'v'},
      {0, 0, 0, 0}
    };

  if (argc > 1)
    {
      int c;
      int longindex;
      long temp_port;
      while ((c = getopt_long (argc, argv, "d:h:p:u:v", longopts, &longindex))
	     != -1)
	{
	  switch (c)
	    {
	    case 'd':
	      database = optarg;
	      break;

	    case 'e':
	      puts (USAGE_INFO);
	      return 0;

	    case 'h':
	      host = optarg;
	      break;

	    case 'p':
	      c = sscanf (optarg, "%ld", &temp_port);
	      if (c != 1)
		{
		  fprintf (stderr, "patts-upg: %s is not a valid number\n",
			   optarg);
		  return 2;
		}

	      if (temp_port < 0 || temp_port > 65535)
		{
		  fprintf (stderr, "patts-upg: %s is not a valid port\n",
			   optarg);
		  return 2;
		}

	      port = optarg;
	      break;

	    case 'u':
	      user = optarg;
	      break;

	    case 'v':
	      puts (VERSION_INFO);
	      return 0;

	    case '?':
	      return 1;
	    };
	}
    }

  bool free_host = false, free_user = false, free_pw = false, free_db = false;
  char *check;

  if (!host)
    {
      host = malloc (128 * sizeof (char));
      if (!host)
	{
	  fprintf (stderr, "patts-upg: Out of memory\n");
	  rc = PATTS_MEMORYERROR;
	  goto end;
	}

      free_host = true;
      printf ("Server (default: localhost): ");
      check = fgets (host, 128, stdin);
      if (!check)
	{
	  rc = PATTS_OVERFLOW;
	  goto end;
	}

      trim (host);
      if (strlen (host) == 0)
	strcpy (host, "localhost");
    }

  if (!user)
    {
      user = malloc (17 * sizeof (char));
      if (!user)
	{
	  fprintf (stderr, "patts-upg: Out of memory\n");
	  rc = PATTS_MEMORYERROR;
	  goto end;
	}

      free_user = true;
      printf ("User (default: root): ");
      check = fgets (user, 17, stdin);
      if (!check)
	{
	  rc = PATTS_OVERFLOW;
	  goto end;
	}

      trim (user);
      if (strlen (user) == 0)
	strcpy (user, "root");
    }

  passwd = patts_malloc (PASS_MAX * sizeof (char));
  if (!passwd)
    {
      fprintf (stderr, "patts-upg: Out of memory\n");
      rc = PATTS_MEMORYERROR;
      goto end;
    }

  free_pw = true;
  check = getpass ("Password (will not echo): ");
  if (!check)
    {
      rc = PATTS_OVERFLOW;
      goto end;
    }

  size_t plen = strlen (check);
  if (plen >= PASS_MAX)
    {
      rc = PATTS_OVERFLOW;
      goto end;
    }

  for (size_t i = 0; i <= plen; ++i)
    {
      passwd[i] = check[i];
      check[i] = 0xDF;
    }

  if (!database)
    {
      database = malloc (128 * sizeof (char));
      if (!database)
	{
	  fprintf (stderr, "patts-upg: Out of memory\n");
	  rc = PATTS_MEMORYERROR;
	  goto end;
	}

      free_db = true;
      printf ("Database (default: patts): ");
      check = fgets (database, 128, stdin);
      if (!check)
	{
	  rc = PATTS_OVERFLOW;
	  goto end;
	}

      trim (database);
      if (strlen (check) == 0)
	strcpy (database, "patts");
    }

  sqon_init ();

  rc = patts_upgrade_db (db_type, host, user, passwd, database, port);
  if (rc)
    {
      if (PATTS_UNEXPECTED == rc)
	{
	  fprintf (stderr, "patts-upg: An unexpected error occurred. This\n"
		   "can happen if the database has a later version\n"
		   "than is supported by your version of libpatts.\n\n");
	}
      else
	{
	  fprintf (stderr, "patts-upg: Error %d occurred.\n\n", rc);
	}

      fprintf (stderr, "patts-upg: Your libpatts version is \"%s\".\n"
	       "Contact Delwink support for assistance.\n", PATTS_VERSION);
      goto end;
    }

  puts ("patts-upg: Database upgrade completed successfully!");

 end:
  if (free_host)
    free (host);
  if (free_user)
    free (user);
  if (free_pw)
    patts_free (passwd);
  if (free_db)
    free (database);

  return rc;
}
