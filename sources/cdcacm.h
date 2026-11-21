// *** cdcacm.h *******************************************************

extern bool force_winusb;

extern bool cdcacm_active;

// EXPERIMENT: bulk IN packets per usb frame
#define USBSTAT_MAX  24
extern volatile uint32 usbstat_hist[USBSTAT_MAX];
extern volatile uint32 usbstat_ready;
extern volatile uint32 usbstat_empty;
extern volatile int usbstat_frame;
extern volatile int usbstat_count;

typedef void (*cdcacm_reset_cbfn)(void);

typedef bool (*cdcacm_receive_cbfn)(byte *buffer, int length);

void
cdcacm_print(const byte *line, int length);

void
cdcacm_command_ack(void);

void
cdcacm_register(cdcacm_reset_cbfn reset, cdcacm_receive_cbfn receive);

