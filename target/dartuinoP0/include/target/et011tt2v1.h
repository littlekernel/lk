


#define FBSIZE  240*240>>2


int eink_refresh(void);
status_t eink_init(void);

uint8_t * get_eink_framebuffer(void);        // returns pointer to eink framebuffer
