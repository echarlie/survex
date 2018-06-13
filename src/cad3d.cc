/* cad3d.cc
 * Converts a .3d file to CAD-like formats (DXF, Skencil, SVG) and also Compass
 * PLT.
 */

/* Copyright (C) 1994-2004,2008,2010,2011,2013,2014,2018 Olly Betts
 * Copyright (C) 2004 John Pybus (SVG Output code)
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "export.h"
#include "mainfrm.h"

#include "cmdline.h"
#include "filename.h"
#include "img_hosted.h"
#include "message.h"
#include "str.h"
#include "useful.h"

/* default to DXF */
#define FMT_DEFAULT FMT_DXF

int
main(int argc, char **argv)
{
   double pan = 0;
   double tilt = -90.0;
   export_format format;
   bool format_set = false;
   int show_mask = 0;
   const char *survey = NULL;
   double grid = 0.0; /* grid spacing (or 0 for no grid) */
   double text_height = DEFAULT_TEXT_HEIGHT; /* for station labels */
   double marker_size = DEFAULT_MARKER_SIZE; /* for station markers */
   double scale = 500.0;

   /* Defaults */
   show_mask |= STNS;
   show_mask |= LABELS;
   show_mask |= LEGS;
   show_mask |= SURF;

   /* TRANSLATE */
   static const struct option long_opts[] = {
	/* const char *name; int has_arg (0 no_argument, 1 required, 2 options_*); int *flag; int val */
	{"survey", required_argument, 0, 's'},
	{"no-crosses", no_argument, 0, 'c'},
	{"no-station-names", no_argument, 0, 'n'},
	{"no-legs", no_argument, 0, 'l'},
	{"grid", optional_argument, 0, 'g'},
	{"text-height", required_argument, 0, 't'},
	{"marker-size", required_argument, 0, 'm'},
	{"elevation", required_argument, 0, 'e'},
	{"reduction", required_argument, 0, 'r'},
	{"dxf", no_argument, 0, 'D'},
	{"skencil", no_argument, 0, 'S'},
	{"plt", no_argument, 0, 'P'},
	{"svg", no_argument, 0, 'V'},
	{"help", no_argument, 0, HLP_HELP},
	{"version", no_argument, 0, HLP_VERSION},
	/* Old name for --skencil: */
	{"sketch", no_argument, 0, 'S'},
	{0,0,0,0}
   };

#define short_opts "s:cnlg::t:m:e:r:DSPV"

   /* TRANSLATE */
   static struct help_msg help[] = {
	/*			<-- */
	{HLP_ENCODELONG(0),   /*only load the sub-survey with this prefix*/199, 0},
	{HLP_ENCODELONG(1),   /*do not generate station markers*/100, 0},
	{HLP_ENCODELONG(2),   /*do not generate station labels*/101, 0},
	{HLP_ENCODELONG(3),   /*do not generate survey legs*/102, 0},
	{HLP_ENCODELONG(4),   /*generate grid (default %sm)*/148, STRING(DEFAULT_GRID_SPACING)},
	{HLP_ENCODELONG(5),   /*station labels text height (default %s)*/149, STRING(DEFAULT_TEXT_HEIGHT)},
	{HLP_ENCODELONG(6),   /*station marker size (default %s)*/152, STRING(DEFAULT_MARKER_SIZE)},
	{HLP_ENCODELONG(7),   /*produce an elevation view*/103, 0},
	{HLP_ENCODELONG(8),   /*factor to scale down by (default %s)*/155, "500"},
	{HLP_ENCODELONG(9),   /*produce DXF output*/156, 0},
	/* TRANSLATORS: "Skencil" is the name of a software package, so should not be
	 * translated. */
	{HLP_ENCODELONG(10),  /*produce Skencil output*/158, 0},
	/* TRANSLATORS: "Compass" and "Carto" are the names of software packages,
	 * so should not be translated. */
	{HLP_ENCODELONG(11),  /*produce Compass PLT output for Carto*/159, 0},
	{HLP_ENCODELONG(12),  /*produce SVG output*/160, 0},
	{0, 0, 0}
   };

   msg_init(argv);

   cmdline_init(argc, argv, short_opts, long_opts, NULL, help, 1, 2);
   while (1) {
      int opt = cmdline_getopt();
      if (opt == EOF) break;
      switch (opt) {
       case 'e': /* Elevation */
	 tilt = cmdline_double_arg();
	 break;
       case 'c': /* Crosses */
	 show_mask &= ~STNS;
	 break;
       case 'n': /* Labels */
	 show_mask &= ~LABELS;
	 break;
       case 'l': /* Legs */
	 show_mask &= ~LEGS;
	 break;
       case 'g': /* Grid */
	 if (optarg) {
	    grid = cmdline_double_arg();
	 } else {
	    grid = (double)DEFAULT_GRID_SPACING;
	 }
	 break;
       case 'r': /* Reduction factor */
	 scale = cmdline_double_arg();
	 break;
       case 't': /* Text height */
	 text_height = cmdline_double_arg();
	 break;
       case 'm': /* Marker size */
	 marker_size = cmdline_double_arg();
	 break;
       case 'D':
	 format = FMT_DXF;
	 format_set = true;
	 break;
       case 'S':
	 format = FMT_SK;
	 format_set = true;
	 break;
       case 'P':
	 format = FMT_PLT;
	 format_set = true;
	 break;
       case 'V':
	 format = FMT_SVG;
	 format_set = true;
	 break;
       case 's':
	 survey = optarg;
	 break;
      }
   }

   const char* fnm_in = argv[optind++];
   const char* fnm_out = argv[optind];
   if (fnm_out) {
      if (!format_set) {
	 size_t i;
	 size_t len = strlen(fnm_out);
	 format = FMT_DEFAULT;
	 for (i = 0; i < FMT_MAX_PLUS_ONE_; ++i) {
	    size_t l = strlen(extension[i]);
	    if (len > l + 1 &&
		strcasecmp(fnm_out + len - l, extension[i]) == 0) {
	       format = export_format(i);
	       break;
	    }
	 }
      }
   } else {
      char *baseleaf = baseleaf_from_fnm(fnm_in);
      if (!format_set) format = FMT_DEFAULT;
      /* note : memory allocated by fnm_out gets leaked in this case... */
      fnm_out = add_ext(baseleaf, extension[format]);
      osfree(baseleaf);
   }

   Model model;
   int err = model.Load(fnm_in, survey);
   if (err) fatalerror(err, fnm_in);

   if (!Export(fnm_out, model.GetSurveyTitle(),
	       model.GetDateString(),
	       model,
	       pan, tilt, show_mask, format,
	       model.GetCSProj(),
	       grid, text_height, marker_size,
	       scale)) {
      fatalerror(/*Couldn’t write file “%s”*/402, fnm_out);
   }
   return 0;
}