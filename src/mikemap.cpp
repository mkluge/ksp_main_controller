#include "mikemap.h"

#ifdef TEST_MIKEMAP
#include <assert.h>

#define DEFAULT_TEST_VALUE 99

int main(int argc, char* argv[])
{
  MikeMap mm;
  MAP_KEY_TYPE key=DEFAULT_TEST_VALUE;
  MAP_VALUE_TYPE value=DEFAULT_TEST_VALUE;
  
  assert(mm.get_len()==0);
  for( int i=0; i<MAX_MAP_LEN-4; i++)
    mm.set(i,i);
  assert(mm.get_len()==MAX_MAP_LEN-4);
  mm.del(1);
  assert(mm.get_len()==MAX_MAP_LEN-5);
  assert(mm.get(4)==4);
  assert(mm.has(1)==false);
  assert(mm.has(2)==true);
  mm.get_at( MAX_MAP_LEN+4, &key, &value);
  assert( key==DEFAULT_TEST_VALUE);
  assert( value==DEFAULT_TEST_VALUE);
  mm.get_at( 3, &key, &value);
  assert( key==4);
  assert( value==4);
  mm.clear();
  key=DEFAULT_TEST_VALUE;
  value=DEFAULT_TEST_VALUE;
  assert(mm.get_len()==0);
  for( int i=0; i<MAX_MAP_LEN-4; i++)
    mm.set(i,i);
  assert(mm.get_len()==MAX_MAP_LEN-4);
  mm.del(1);
  assert(mm.get_len()==MAX_MAP_LEN-5);
  assert(mm.get(4)==4);
  assert(mm.get(10)==10);
  assert(mm.has(1)==false);
  assert(mm.has(2)==true);
  mm.get_at( MAX_MAP_LEN+4, &key, &value);
  assert( key==DEFAULT_TEST_VALUE);
  assert( value==DEFAULT_TEST_VALUE);
  mm.get_at( 3, &key, &value);
  assert( key==4);
  assert( value==4);
  mm.del(2);
  mm.del(5);
  mm.set(2,20);
  assert(mm.has(2)==true);
  assert(mm.get_len()==MAX_MAP_LEN-6);
  mm.get_at( MAX_MAP_LEN-7, &key, &value);
  assert( key==2);
  assert( value==20);
  assert(mm.has(5)==false);
 }
#endif

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
