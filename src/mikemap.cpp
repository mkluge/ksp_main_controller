#include "mikemap.h"
#include <stdio.h>
//#include <string.h>
//#include <stdlib.h>

//#define TEST_MIKEMAP

#ifdef TEST_MIKEMAP
#include <assert.h>

#define DEFAULT_TEST_VALUE 99
using namespace mikemap;

int main(int argc, char *argv[])
{
  MikeMap mm;
  char input[] = "[100,1,101,0,10,0,9,0,14,0,11,0,15,0]";
  MAP_KEY_TYPE key = DEFAULT_TEST_VALUE;
  MAP_VALUE_TYPE value = DEFAULT_TEST_VALUE;
  int converted = 0;

  assert(mm.get_len() == 0);
  for (int i = 0; i < MAX_MAP_LEN - 4; i++)
    mm.set(i, i);
  assert(mm.get_len() == MAX_MAP_LEN - 4);
  mm.del(1);
  assert(mm.get_len() == MAX_MAP_LEN - 5);
  assert(mm.get(4) == 4);
  assert(mm.has(1) == false);
  assert(mm.has(2) == true);
  mm.get_at(MAX_MAP_LEN + 4, &key, &value);
  assert(key == DEFAULT_TEST_VALUE);
  assert(value == DEFAULT_TEST_VALUE);
  mm.get_at(3, &key, &value);
  assert(key == 4);
  assert(value == 4);
  mm.clear();
  key = DEFAULT_TEST_VALUE;
  value = DEFAULT_TEST_VALUE;
  assert(mm.get_len() == 0);
  for (int i = 0; i < MAX_MAP_LEN - 4; i++)
    mm.set(i, i);
  assert(mm.get_len() == MAX_MAP_LEN - 4);
  mm.del(1);
  assert(mm.get_len() == MAX_MAP_LEN - 5);
  assert(mm.get(4) == 4);
  assert(mm.get(10) == 10);
  assert(mm.has(1) == false);
  assert(mm.has(2) == true);
  mm.get_at(MAX_MAP_LEN + 4, &key, &value);
  assert(key == DEFAULT_TEST_VALUE);
  assert(value == DEFAULT_TEST_VALUE);
  mm.get_at(3, &key, &value);
  assert(key == 4);
  assert(value == 4);
  mm.del(2);
  mm.del(5);
  mm.set(2, 20);
  assert(mm.has(2) == true);
  assert(mm.get_len() == MAX_MAP_LEN - 6);
  mm.get_at(MAX_MAP_LEN - 7, &key, &value);
  assert(key == 2);
  assert(value == 20);
  assert(mm.has(5) == false);
  mm.clear();
  assert(mm.from_string(input, &converted) == 0);
  assert(converted == 14);
  mm.get_at(0, &key, &value);
  printf("k0: %d  v0: %d\n", key, value);
  assert(key == 100);
  assert(value == 1);
  mm.get_at(4, &key, &value);
  assert(key == 14);
  assert(value == 0);
}
#endif

namespace mikemap
{

  MikeMap::MikeMap()
  {
    this->mikemap_len = 0;
  }

  void MikeMap::set(MAP_KEY_TYPE key, MAP_VALUE_TYPE value)
  {
    for (unsigned int i = 0; i < mikemap_len; i++)
    {
      if (mikemap_keys[i] == key)
      {
        mikemap_values[i] = value;
        return;
      }
    }
    if (mikemap_len == MAX_MAP_LEN)
      return;
    mikemap_keys[mikemap_len] = key;
    mikemap_values[mikemap_len] = value;
    mikemap_len++;
  }

  bool MikeMap::has(MAP_KEY_TYPE key) const
  {
    for (unsigned int i = 0; i < mikemap_len; i++)
    {
      if (mikemap_keys[i] == key)
      {
        return true;
      }
    }
    return false;
  }

  MAP_VALUE_TYPE MikeMap::get(MAP_KEY_TYPE key) const
  {
    for (unsigned int i = 0; i < mikemap_len; i++)
    {
      if (mikemap_keys[i] == key)
      {
        return mikemap_values[i];
      }
    }
    return 0;
  }

  void MikeMap::to_string(char *ptr)
  {
    int offset = 0;
    for (unsigned int i = 0; i < mikemap_len; i++)
    {
      if (i > 0)
      {
        ptr[offset] = ',';
        offset++;
      }
      offset += sprintf(&ptr[offset], "%d", mikemap_keys[i]);
      ptr[offset] = ',';
      offset++;
      offset += sprintf(&ptr[offset], "%d", mikemap_values[i]);
    }
  }

  int MikeMap::to_int(const char *data, int len)
  {
    long int val = 0;
    while (len)
    {
      long int new_val = ((int)*data - (int)'0');
      val = val * 10 + new_val;
      len--;
      data++;
    }
#ifdef TEST_MIKEMAP
    printf("val: %d\n", val);
#endif
    return val;
  }

  int MikeMap::from_string(const char *data_start, int *converted)
  {
    const char *data_ptr = data_start;

    this->clear();
    // the map is supposed to start with "["
    if (*data_ptr != '[')
      return MM_ERR_WRONG_START;
    data_ptr++;

    while (1)
    {
      int len_key = 0;
      const char *key_start = data_ptr;
      {
        while (len_key < MAX_SINGLE_DATA_LEN && *data_ptr != ',' && *data_ptr != ']')
        {
          if (*data_ptr < '0' || *data_ptr > '9')
            return MM_ERR_NO_DIGIT;
          len_key++;
          data_ptr++;
        }
        if (len_key == 0 || len_key == MAX_SINGLE_DATA_LEN)
          return MM_ERR_ZERO_MAX_LEN_KEY;
        if (*data_ptr == ']')
          return MM_ERR_MISSING_VAL;
      }
      int k = this->to_int(key_start, len_key);
      (*converted)++;
      data_ptr++;
      int len_val = 0;
      const char *val_start = data_ptr;
      {
        while (len_val < MAX_SINGLE_DATA_LEN && *data_ptr != ',' && *data_ptr != ']')
        {
          if (*data_ptr < '0' || *data_ptr > '9')
            return MM_ERR_NO_DIGIT;
          len_val++;
          data_ptr++;
        }
        if (len_val == 0 || len_val == MAX_SINGLE_DATA_LEN)
          return MM_ERR_ZERO_MAX_LEN_VAL;
      }
      int v = this->to_int(val_start, len_val);
      (*converted)++;
      this->set(k, v);
      if (*data_ptr == ']')
        break;
      else
        data_ptr++;
    }

    return MM_OK;
  }

  void MikeMap::del(MAP_KEY_TYPE key)
  {
    for (unsigned int i = 0; i < mikemap_len; i++)
    {
      if (mikemap_keys[i] == key)
      {
        for (unsigned int si = i; si < (mikemap_len - 1); si++)
        {
          mikemap_keys[si] = mikemap_keys[si + 1];
          mikemap_values[si] = mikemap_values[si + 1];
        }
        mikemap_keys[mikemap_len] = 0;
        mikemap_values[mikemap_len] = 0;
        mikemap_len--;
        return;
      }
    }
  }

  unsigned int MikeMap::get_len() const
  {
    return mikemap_len;
  }

  void MikeMap::clear()
  {
    mikemap_len = 0;
  }

  void MikeMap::get_at(unsigned int at, MAP_KEY_TYPE *key, MAP_VALUE_TYPE *value) const
  {
    if (at < mikemap_len)
    {
      *key = mikemap_keys[at];
      *value = mikemap_values[at];
    }
  }
}