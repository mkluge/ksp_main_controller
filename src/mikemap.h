#ifndef _HAVE_MIKEMAP_
#define _HAVE_MIKEMAP_

#define MAX_MAP_LEN 30

#include "stdint.h"

class MikeMap {

public:
  MikeMap();
  void set( int key, int value);
  bool has( int key) const;
  int get( int key) const;
  void del( int key);
  int get_len() const;
  void get_at( int at, int *key, int *value) const;
  void clear();

private:
  uint8_t keys[MAX_MAP_LEN];
  int16_t values[MAX_MAP_LEN];
  int len;
};

#endif
