#ifndef _HAVE_MIKEMAP_
#define _HAVE_MIKEMAP_

#include <stdlib.h>
#include <stdint.h>

namespace mikemap
{

// how long could possibly be a single value in terms of digits
#define MAX_SINGLE_DATA_LEN 10
#define MAX_MAP_LEN 50
#define MAP_KEY_TYPE uint8_t
#define MAP_VALUE_TYPE int32_t
#define MM_OK 0
#define MM_ERR_ZERO_MAX_LEN_KEY 1
#define MM_ERR_ZERO_MAX_LEN_VAL 2
#define MM_ERR_NO_DIGIT 3
#define MM_ERR_MISSING_VAL 4
#define MM_ERR_WRONG_START 5

  class MikeMap
  {

  public:
    MikeMap();
    void set(MAP_KEY_TYPE key, MAP_VALUE_TYPE value);
    bool has(MAP_KEY_TYPE key) const;
    MAP_VALUE_TYPE get(MAP_KEY_TYPE key) const;
    void del(MAP_KEY_TYPE key);
    unsigned int get_len() const;
    void get_at(unsigned int at, MAP_KEY_TYPE *key, MAP_VALUE_TYPE *value) const;
    void clear();
    void to_string(char *ptr);
    int from_string(const char *data_start, int *converted);
    static int to_int(const char *data, int len);

  private:
    MAP_KEY_TYPE mikemap_keys[MAX_MAP_LEN];
    MAP_VALUE_TYPE mikemap_values[MAX_MAP_LEN];
    size_t mikemap_len;
  };

}

#endif
