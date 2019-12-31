#ifndef _HAVE_MIKEMAP_
#define _HAVE_MIKEMAP_

#include <stdlib.h>
#include <stdint.h>

namespace mikemap {

#define MAX_MAP_LEN 50
#define MAP_KEY_TYPE uint8_t
#define MAP_VALUE_TYPE int32_t

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
	void to_string( char *ptr);
	void from_string( char *ptr, const char *data_start);

  private:
    MAP_KEY_TYPE mikemap_keys[MAX_MAP_LEN];
    MAP_VALUE_TYPE mikemap_values[MAX_MAP_LEN];
    uint8_t mikemap_len;
};

}

#endif
