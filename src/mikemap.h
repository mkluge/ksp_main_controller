#ifndef _HAVE_MIKEMAP_
#define _HAVE_MIKEMAP_

#define MAX_MAP_LEN 100

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
  int keys[MAX_MAP_LEN];
  int values[MAX_MAP_LEN];
  int len;
};

#endif
