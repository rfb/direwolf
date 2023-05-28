#include "ardop_frames.h"

#include <string.h>

int get_ardop_frame(int code, ardop_frame *frame) {
  frame->code = code;

  switch(code) {
    case 0 ... 0x0f:
      // DATANAK
      strcpy(frame->frame_type, "DATANAK");
      frame->num_carrier = 1;
      frame->modulation = M_4FSK;
      frame->baud = 50;
      break;

    case 0x20:
      // BREAK
      strcpy(frame->frame_type, "BREAK");
      frame->num_carrier = 1;
      frame->modulation = M_4FSK;
      frame->baud = 50;
      break;

    case 0x21:
      // END
      strcpy(frame->frame_type, "END");
      frame->num_carrier = 1;
      frame->modulation = M_4FSK;
      frame->baud = 50;
      break;

    case 0x22:
      // DISC
      strcpy(frame->frame_type, "DISC");
      frame->num_carrier = 1;
      frame->modulation = M_4FSK;
      frame->baud = 50;
      break;

    case 0x23:
      // IDLE
      strcpy(frame->frame_type, "IDLE");
      frame->num_carrier = 1;
      frame->modulation = M_4FSK;
      frame->baud = 50;
      break;

    case 0x24:
      strcpy(frame->frame_type, "CONACK200");
      frame->num_carrier = 1;
      frame->modulation = M_4PSK;
      frame->baud = 100;
      break;

    case 0x25:
      strcpy(frame->frame_type, "CONACK500");
      frame->num_carrier = 1;
      frame->modulation = M_4PSK;
      frame->baud = 100;
      break;

    case 0x26:
      strcpy(frame->frame_type, "CONACK1000");
      frame->num_carrier = 1;
      frame->modulation = M_4PSK;
      frame->baud = 100;
      break;

    case 0x27:
      strcpy(frame->frame_type, "CONACK2000");
      frame->num_carrier = 1;
      frame->modulation = M_4PSK;
      frame->baud = 100;
      break;

    case 0x28:
      strcpy(frame->frame_type, "CONREJBUSY");
      frame->num_carrier = 1;
      frame->modulation = M_4FSK;
      frame->baud = 50;
      break;

    default:
      break;
  }

  return 1;
}
