#include "mikemap.h"
#include <ArduinoUnit.h>

test(map)
{
  MikeMap mm;
  assertEqual(mm.get_len(), 0);
  for( int i=0; i<200; i++)
    mm.set(i,i);
  assertEqual(mm.get_len(), MAX_MAP_LEN);
  mm.del(1);
  assertEqual(mm.get_len(), MAX_MAP_LEN-1);
  assertEqual(mm.get(4), 4);
  assertEqual(mm.has(1), false);
  assertEqual(mm.has(2), true);
  mm.clear();
  assertEqual(mm.get_len(), 0);
}

MikeMap::MikeMap()
{
  len=0;
}

void MikeMap::set( int key, int value)
{
  for( int i=0; i<len; i++)
  {
    if( keys[i]==key)
    {
      values[i]=value;
      return;
    }
  }
  if( len==MAX_MAP_LEN )
    return;
  keys[len]=key;
  values[len]=value;
  len++;
}

bool MikeMap::has( int key) const
{
  for( int i=0; i<len; i++)
  {
    if( keys[i]==key)
    {
      return true;
    }
  }
  return false;
}

int MikeMap::get( int key) const
{
  for( int i=0; i<len; i++)
  {
    if( keys[i]==key)
    {
      return values[i];
    }
  }
  return 0;
}

void MikeMap::del( int key)
{
  for( int i=0; i<len; i++)
  {
    if( keys[i]==key)
    {
      for( int si=i; si<len-1; si++)
      {
        keys[si]   = keys[si+1];
        values[si] = values[si+1];
      }
      len--;
      return;
    }
  }
}

int MikeMap::get_len() const
{
  return len;
}

void MikeMap::clear()
{
  len=0;
}

void MikeMap::get_at( int at, int *key, int *value) const
{
  if( at>=len )
    return;
  *key=keys[at];
  *value=values[at];
}
