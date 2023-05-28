//    Copyright (C) 2011, 2013, 2014, 2015, 2016, 2019, 2021, 2023  John Langner, WB2OSZ
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 2 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program.  If not, see <http://www.gnu.org/licenses/>.
//


/*------------------------------------------------------------------
 *
 * Name:	gen_ardop_frame.c
 *
 * Purpose:	Test program for generating Ardop frames.
 *
 * Description:	Given messages are converted to audio and written 
 *		to a .WAV type audio file.
 *
 * Examples:	Different speeds:
 *
 *			gen_ardop_frame -c code -p payload -o output.wav
 *
 *			gen_packets -c 0x00 -p -0x0112 -o datanak.wav
 *
 *------------------------------------------------------------------*/


#include "direwolf.h"

#include <stdio.h>     
#include <stdlib.h>    
#include <getopt.h>
#include <string.h>
#include <assert.h>
#include <math.h>

#include "audio.h"
#include "textcolor.h"
#include "ardop_frames.h"

static void usage (char **argv);
static int audio_file_open (char *fname, struct audio_s *pa);
static int audio_file_close (void);
static struct audio_s modem;

int main(int argc, char **argv)
{
  int c;
  int err;
	int chan;

  int code = 0;
//  int payload;

  char output_file[256];		/* -o option */
  strlcpy (output_file, "", sizeof(output_file)); 

  char payload[256]; /* -p option */
  strlcpy (payload, "", sizeof(payload));

  /*
   * Set up default values for the modem.
   */

  memset (&modem, 0, sizeof(modem));

  modem.adev[0].defined = 1;
  modem.adev[0].num_channels = DEFAULT_NUM_CHANNELS;              /* -2 stereo */
  modem.adev[0].samples_per_sec = DEFAULT_SAMPLES_PER_SEC;        /* -r option */
  modem.adev[0].bits_per_sample = DEFAULT_BITS_PER_SAMPLE;        /* -8 for 8 instead of 16 bits */

  for (chan = 0; chan < MAX_CHANS; chan++) {
    modem.achan[chan].modem_type = MODEM_AFSK;			/* change with -g */
    modem.achan[chan].mark_freq = DEFAULT_MARK_FREQ;              /* -m option */
    modem.achan[chan].space_freq = DEFAULT_SPACE_FREQ;            /* -s option */
    modem.achan[chan].baud = DEFAULT_BAUD;                        /* -b option */
  }

  modem.chan_medium[0] = MEDIUM_RADIO;

  /*
   * Parse the command line options.
   */

  while (1) {
    c = getopt(argc, argv, "c:p:o:");

    if (c == -1)
      break;

    switch (c) {
      case 'c':				/* -c code */
        code = strtol(optarg, 0, 16);

        dw_printf("code %d\n", code);

        break;

      case 'p':				/* -b payload */
        strlcpy(payload, optarg, sizeof(payload));

        dw_printf("payload %s\n", payload);

        break;

      case 'o':				/* -o for Output file */

        strlcpy (output_file, optarg, sizeof(output_file));

        dw_printf ("Output file set to %s\n", output_file);
        break;

      default:

        /* Should not be here. */
        dw_printf("?? getopt returned character code 0%o ??\n", (unsigned)c);
        usage (argv);
    }
  }

  if (strlen(output_file) == 0) {
    dw_printf ("ERROR: The -o output file option must be specified.\n");
    usage (argv);
    exit (1);
  }

  err = audio_file_open (output_file, &modem);

  ardop_frame frame;

  get_ardop_frame(code, &frame);

  dw_printf ("frame.frame_type %s\n", frame.frame_type);
  dw_printf ("frame.num_carrier %i\n", frame.num_carrier);
  dw_printf ("frame.modulation %i\n", frame.modulation);
  dw_printf ("frame.baud %i\n", frame.baud);
  dw_printf ("frame.bandwidth %i\n", frame.bandwidth);

  if (err < 0) {
    dw_printf ("ERROR - Can't open output file.\n");
    exit (1);
  }

  audio_file_close();

  return EXIT_SUCCESS;
}


static void usage (char **argv)
{
	dw_printf ("\n");
	dw_printf ("Usage: gen_ardop_frame [options] [file]\n");
	dw_printf ("Options:\n");
	dw_printf ("  -c <number>   Frame Code.\n");
	dw_printf ("  -p <payload>  Payload.\n");
	
  exit (EXIT_FAILURE);
}


/*------------------------------------------------------------------
 *
 * Name:        audio_file_open
 *
 * Purpose:     Open a .WAV format file for output.
 *
 * Inputs:      fname		- Name of .WAV file to create.
 *
 *		pa		- Address of structure of type audio_s.
 *				
 *				The fields that we care about are:
 *					num_channels
 *					samples_per_sec
 *					bits_per_sample
 *				If zero, reasonable defaults will be provided.
 *         
 * Returns:     0 for success, -1 for failure.
 *		
 *----------------------------------------------------------------*/

struct wav_header {             /* .WAV file header. */
        char riff[4];           /* "RIFF" */
        int filesize;          /* file length - 8 */
        char wave[4];           /* "WAVE" */
        char fmt[4];            /* "fmt " */
        int fmtsize;           /* 16. */
        short wformattag;       /* 1 for PCM. */
        short nchannels;        /* 1 for mono, 2 for stereo. */
        int nsamplespersec;    /* sampling freq, Hz. */
        int navgbytespersec;   /* = nblockalign * nsamplespersec. */
        short nblockalign;      /* = wbitspersample / 8 * nchannels. */
        short wbitspersample;   /* 16 or 8. */
        char data[4];           /* "data" */
        int datasize;          /* number of bytes following. */
} ;

				/* 8 bit samples are unsigned bytes */
				/* in range of 0 .. 255. */
 
				/* 16 bit samples are signed short */
				/* in range of -32768 .. +32767. */

static FILE *out_fp = NULL;

static struct wav_header header;

static int byte_count;			/* Number of data bytes written to file. */
					/* Will be written to header when file is closed. */


static int audio_file_open (char *fname, struct audio_s *pa)
{
	int n;

/*
 * Fill in defaults for any missing values.
 */
	if (pa -> adev[0].num_channels == 0)
	  pa -> adev[0].num_channels = DEFAULT_NUM_CHANNELS;

	if (pa -> adev[0].samples_per_sec == 0)
	  pa -> adev[0].samples_per_sec = DEFAULT_SAMPLES_PER_SEC;

	if (pa -> adev[0].bits_per_sample == 0)
	  pa -> adev[0].bits_per_sample = DEFAULT_BITS_PER_SAMPLE;


/*
 * Write the file header.  Don't know length yet.
 */
        out_fp = fopen (fname, "wb");	
	
        if (out_fp == NULL) {
           dw_printf ("Couldn't open file for write: %s\n", fname);
	   perror ("");
           return (-1);
        }

	memset (&header, 0, sizeof(header));

        memcpy (header.riff, "RIFF", (size_t)4);
        header.filesize = 0; 
        memcpy (header.wave, "WAVE", (size_t)4);
        memcpy (header.fmt, "fmt ", (size_t)4);
        header.fmtsize = 16;			// Always 16.
        header.wformattag = 1;     		// 1 for PCM.

        header.nchannels = pa -> adev[0].num_channels;   		
        header.nsamplespersec = pa -> adev[0].samples_per_sec;    
        header.wbitspersample = pa -> adev[0].bits_per_sample;  
		
        header.nblockalign = header.wbitspersample / 8 * header.nchannels;     
        header.navgbytespersec = header.nblockalign * header.nsamplespersec;   
        memcpy (header.data, "data", (size_t)4);
        header.datasize = 0;        

	assert (header.nchannels == 1 || header.nchannels == 2);

        n = fwrite (&header, sizeof(header), (size_t)1, out_fp);

	if (n != 1) {
	  dw_printf ("Couldn't write header to: %s\n", fname);
	  perror ("");
	  fclose (out_fp);
	  out_fp = NULL;
          return (-1);
        }


/*
 * Number of bytes written will be filled in later.
 */
        byte_count = 0;
	
	return (0);

} /* end audio_open */


/*------------------------------------------------------------------
 *
 * Name:        audio_put
 *
 * Purpose:     Send one byte to the audio output file.
 *
 * Inputs:	c	- One byte in range of 0 - 255.
 *
 * Returns:     Normally non-negative.
 *              -1 for any type of error.
 *
 * Description:	The caller must deal with the details of mono/stereo
 *		and number of bytes per sample.
 *
 *----------------------------------------------------------------*/


int audio_put (int a, int c)
{
  byte_count++;
  return putc(c, out_fp);

} /* end audio_put */


int audio_flush (int a)
{
	return 0;
}

/*------------------------------------------------------------------
 *
 * Name:        audio_file_close
 *
 * Purpose:     Close the audio output file.
 *
 * Returns:     Normally non-negative.
 *              -1 for any type of error.
 *
 *
 * Description:	Must go back to beginning of file and fill in the
 *		size of the data.
 *
 *----------------------------------------------------------------*/

static int audio_file_close (void)
{
	int n;

        //text_color_set(DW_COLOR_DEBUG); 
	//dw_printf ("audio_close()\n");

/*
 * Go back and fix up lengths in header.
 */
        header.filesize = byte_count + sizeof(header) - 8;           
        header.datasize = byte_count;        

	if (out_fp == NULL) {
	  return (-1);
 	}

        fflush (out_fp);

        fseek (out_fp, 0L, SEEK_SET);         
        n = fwrite (&header, sizeof(header), (size_t)1, out_fp);

	if (n != 1) {
	  dw_printf ("Couldn't write header to audio file.\n");
	  perror ("");		// TODO: remove perror.
	  fclose (out_fp);
	  out_fp = NULL;
          return (-1);
        }

        fclose (out_fp);
        out_fp = NULL;

	return (0);

} /* end audio_close */


