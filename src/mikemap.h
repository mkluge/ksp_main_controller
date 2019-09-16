#ifndef _HAVE_MIKEMAP_
#define _HAVE_MIKEMAP_

#define MAX_MAP_LEN 50

#include "stdint.h"

#define MAP_KEY_TYPE uint8_t
#define MAP_VALUE_TYPE int16_t

class MikeMap {

  public:
    MikeMap();
    void set( MAP_KEY_TYPE key, MAP_VALUE_TYPE value);
    bool has( MAP_KEY_TYPE key) const;
    MAP_VALUE_TYPE get( MAP_KEY_TYPE key) const;
    void del( MAP_KEY_TYPE key);
    unsigned int get_len() const;
    void get_at( unsigned int at, MAP_KEY_TYPE *key, MAP_VALUE_TYPE *value) const;
    void clear();

  private:
    uint8_t keys[MAX_MAP_LEN];
    int16_t values[MAX_MAP_LEN];
    unsigned int len;

};

#endif
