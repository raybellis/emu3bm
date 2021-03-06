/*
 *	main.c
 *	Copyright (C) 2018 David García Goñi <dagargo at gmail dot com>
 *
 *   This file is part of emu3bm.
 *
 *   emu3bm is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   emu3bm is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with emu3bm.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "emu3bm.h"

int verbosity = 0;

int
get_positive_int (char *str)
{
  char *endstr;

  int value = (int) strtol (str, &endstr, 10);

  if (errno || endstr == str || *endstr != '\0')
    {
      fprintf (stderr, "Value %s not valid\n", str);
      value = -1;
    }
  return value;
}

int
main (int argc, char *argv[])
{
  int c;
  int xflg = 0, sflg = 0, nflg = 0, errflg = 0, modflg = 0, pflg = 0, zflg =
    0;
  char *bank_filename;
  char *sample_filename;
  char *preset_name;
  char *rt_controls = NULL;
  char *zone_params = NULL;
  int level = -1;
  int cutoff = -1;
  int q = -1;
  int filter = -1;
  int pbr = -1;
  int preset = -1;
  extern char *optarg;
  extern int optind, optopt;
  int result = 0;
  int loop;

  while ((c = getopt (argc, argv, "vns:S:xr:l:c:q:f:b:e:p:z:")) != -1)
    {
      switch (c)
	{
	case 'v':
	  verbosity++;
	  break;
	case 's':
	  sflg++;
	  sample_filename = optarg;
	  loop = 0;
	  break;
	case 'S':
	  sflg++;
	  sample_filename = optarg;
	  loop = 1;
	  break;
	case 'x':
	  xflg++;
	  break;
	case 'n':
	  nflg++;
	  break;
	case 'r':
	  rt_controls = optarg;
	  modflg++;
	  break;
	case 'l':
	  level = get_positive_int (optarg);
	  modflg++;
	  break;
	case 'c':
	  cutoff = get_positive_int (optarg);
	  modflg++;
	  break;
	case 'q':
	  q = get_positive_int (optarg);
	  modflg++;
	  break;
	case 'f':
	  filter = get_positive_int (optarg);
	  modflg++;
	  break;
	case 'b':
	  pbr = get_positive_int (optarg);
	  modflg++;
	  break;
	case 'e':
	  preset = get_positive_int (optarg);
	  break;
	case 'p':
	  preset_name = optarg;
	  pflg++;
	  break;
	case 'z':
	  zone_params = optarg;
	  zflg++;
	  break;
	case '?':
	  fprintf (stderr, "Unrecognized option: -%c\n", optopt);
	  errflg++;
	}
    }

  if (optind + 1 == argc)
    bank_filename = argv[optind];
  else
    errflg++;

  if (nflg > 1)
    errflg++;

  if (sflg > 1)
    errflg++;

  if (pflg > 1)
    errflg++;

  if (zflg > 1)
    errflg++;

  if (nflg + sflg + pflg + zflg > 1)
    errflg++;

  if ((nflg || sflg || pflg || zflg) && modflg)
    errflg++;

  if (errflg > 0)
    {
      fprintf (stderr, "%s\n", PACKAGE_STRING);
      fprintf (stderr, "Usage: %s [OPTIONS] bank_file.\n",
	       basename (argv[0]));
      exit (EXIT_FAILURE);
    }

  if (nflg)
    {
      result = emu3_create_bank (bank_filename);
      exit (result);
    }

  struct emu3_file *file = emu3_open_file (bank_filename);

  if (!file)
    exit (result);

  if (sflg)
    {
      result = emu3_add_sample (file, sample_filename, loop);

      if (result)
	{
	  emu3_close_file (file);
	  exit (result);
	}
    }

  if (pflg)
    {
      result = emu3_add_preset (file, preset_name);

      if (result)
	{
	  emu3_close_file (file);
	  exit (result);
	}
    }

  if (zflg)
    {
      result = emu3_add_preset_zone (file, zone_params);

      if (result)
	{
	  emu3_close_file (file);
	  exit (result);
	}
    }

  if (!nflg && !sflg && !pflg && !zflg)
    {
      result =
	emu3_process_bank (file, preset, xflg, rt_controls, level,
			   cutoff, q, filter, pbr);

      if (result)
	{
	  emu3_close_file (file);
	  exit (result);
	}
    }

  if (sflg || pflg || zflg || modflg)
    emu3_write_file (file);

  emu3_close_file (file);

  exit (result);
}
