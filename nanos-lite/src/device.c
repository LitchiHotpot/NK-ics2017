#include "common.h"

#define NAME(key) \
  [_KEY_##key] = #key,

static const char *keyname[256] __attribute__((used)) = {
  [_KEY_NONE] = "NONE",
  _KEYS(NAME)
};

extern int current_game;
size_t events_read(void *buf, size_t len) {

  int key = _read_key();
  char keydown_char;  
  if(key & 0x8000)
    keydown_char='d';
  else
    keydown_char='u';

  key &= ~0x8000;

  if (key != _KEY_NONE) {
  	if (key==13){
  		current_game=!current_game;
  	}
    return snprintf(buf, len, "k%c %s\n", keydown_char, keyname[key]) - 1;
  }
  else {
    unsigned long time_ms = _uptime();
    return snprintf(buf, len, "t %d\n", time_ms) - 1;
  }
  return 0;
}

static char dispinfo[128] __attribute__((used));

void dispinfo_read(void *buf, off_t offset, size_t len) {
  memcpy(buf, dispinfo + offset, len);
}

void fb_write(const void *buf, off_t offset, size_t len) {
  off_t offset_len = offset / sizeof(uint32_t);
  _draw_rect(buf, offset_len % _screen.width, offset_len / _screen.width, len / sizeof(uint32_t), 1);
}

void init_device() {
  _ioe_init();

  // TODO: print the string to array `dispinfo` with the format
  // described in the Navy-apps convention
  sprintf(dispinfo, "WIDTH: %d\nHEIGHT: %d", _screen.width, _screen.height);
}
