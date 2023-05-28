typedef enum { M_4FSK, M_4PSK, M_8PSK } ARDOP_MODULATION;

typedef struct {
  char frame_type[16];
  int num_carrier;
  ARDOP_MODULATION modulation;
  int baud;
  int bandwidth;
  int code;
} ardop_frame;

int get_ardop_frame(int, ardop_frame*);
