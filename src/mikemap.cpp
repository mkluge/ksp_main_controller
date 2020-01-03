#include "mikemap.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

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

namespace mikemap {

MikeMap::MikeMap()
{
  mikemap_len=0;
  memset( mikemap_keys, 0, sizeof(mikemap_keys));
  memset( mikemap_values, 0, sizeof(mikemap_values));
}

void MikeMap::set( MAP_KEY_TYPE key, MAP_VALUE_TYPE value)
{
  for( unsigned int i=0; i<mikemap_len; i++)
  {
    if( mikemap_keys[i]==key)
    {
      mikemap_values[i]=value;
      return;
    }
  }
  if( mikemap_len==MAX_MAP_LEN )
    return;
  mikemap_keys[mikemap_len]=key;
  mikemap_values[mikemap_len]=value;
  mikemap_len++;
}

bool MikeMap::has( MAP_KEY_TYPE key) const
{
  for( unsigned int i=0; i<mikemap_len; i++)
  {
    if( mikemap_keys[i]==key)
    {
      return true;
    }
  }
  return false;
}

MAP_VALUE_TYPE MikeMap::get( MAP_KEY_TYPE key) const
{
  for( unsigned int i=0; i<mikemap_len; i++)
  {
    if( mikemap_keys[i]==key)
    {
      return mikemap_values[i];
    }
  }
  return 0;
}

void MikeMap::to_string( char *ptr)
{
  int offset=0;
  for( unsigned int i=0; i<mikemap_len; i++)
  {
	if( i>0 )
	{
		ptr[offset]=',';
		offset++;
	}
	offset += sprintf( &ptr[offset], "%d", mikemap_keys[i]);
	ptr[offset]=',';
	offset++;
	offset += sprintf( &ptr[offset], "%ld", mikemap_values[i]);
  }
}

void MikeMap::from_string( char *ptr, const char *data_start)
{
	this->clear();
	char *data_ptr;
	char *key;
	char *val;

	if( (data_ptr=strstr( ptr, data_start))==NULL )
	{
		return;
	}
	data_ptr+=8; // 8 == strlen(data_start)

	key = strtok( data_ptr, ",");
	while(1)
	{
		if(key==NULL)
			break;
		val = strtok( NULL, ",");
		if(val==NULL)
			break;
		this->set( atol(key), atol(val));
		if (strchr( val, ']'))
			break;
		key = strtok( NULL, ",");
    }
}

void MikeMap::del( MAP_KEY_TYPE key)
{
  for( unsigned int i=0; i<mikemap_len; i++)
  {
    if( mikemap_keys[i]==key)
    {
      for( unsigned int si=i; si<(mikemap_len-1); si++)
      {
        mikemap_keys[si]   = mikemap_keys[si+1];
        mikemap_values[si] = mikemap_values[si+1];
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
  mikemap_len=0;
}

void MikeMap::get_at( unsigned int at, MAP_KEY_TYPE *key, MAP_VALUE_TYPE *value) const
{
  if( at<mikemap_len )
  {
    *key=mikemap_keys[at];
    *value=mikemap_values[at];
  }
}

}