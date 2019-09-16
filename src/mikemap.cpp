#include "mikemap.h"
#include <ArduinoUnit.h>

/*
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
*/

MikeMap::MikeMap()
{
  this->len=0;
}

void MikeMap::set( MAP_KEY_TYPE key, MAP_VALUE_TYPE value)
{
  for( unsigned int i=0; i<this->len; i++)
  {
    if( this->keys[i]==key)
    {
      this->values[i]=value;
      return;
    }
  }
  if( this->len==MAX_MAP_LEN )
    return;
  this->keys[len]=key;
  this->values[len]=value;
  this->len++;
}

bool MikeMap::has( MAP_KEY_TYPE key) const
{
  for( unsigned int i=0; i<this->len; i++)
  {
    if( this->keys[i]==key)
    {
      return true;
    }
  }
  return false;
}

MAP_VALUE_TYPE MikeMap::get( MAP_KEY_TYPE key) const
{
  for( unsigned int i=0; i<this->len; i++)
  {
    if( this->keys[i]==key)
    {
      return this->values[i];
    }
  }
  return 0;
}

void MikeMap::del( MAP_KEY_TYPE key)
{
  for( unsigned int i=0; i<this->len; i++)
  {
    if( this->keys[i]==key)
    {
      for( unsigned int si=i; si<(this->len-1); si++)
      {
        this->keys[si]   = this->keys[si+1];
        this->values[si] = this->values[si+1];
      }
      this->len--;
      return;
    }
  }
}

unsigned int MikeMap::get_len() const
{
  return this->len;
}

void MikeMap::clear()
{
  this->len=0;
}

void MikeMap::get_at( unsigned int at, MAP_KEY_TYPE *key, MAP_VALUE_TYPE *value) const
{
  if( at<this->len )
  {
    *key=this->keys[at];
    *value=this->values[at];
  }
}
