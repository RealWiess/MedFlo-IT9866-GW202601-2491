/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE libopusfile SOFTWARE CODEC SOURCE CODE. *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE libopusfile SOURCE CODE IS (C) COPYRIGHT 1994-2018           *
 * by the Xiph.Org Foundation and contributors https://xiph.org/    *
 *                                                                  *
 ********************************************************************

 function: stdio-based convenience library for opening/seeking/decoding
 last mod: $Id: vorbisfile.c 17573 2010-10-27 14:53:59Z xiphmont $

 ********************************************************************/
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "internal.h"
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>


typedef struct OpusMemStream OpusMemStream;

#define OP_MEM_SIZE_MAX (~(size_t)0>>1)
#define OP_MEM_DIFF_MAX ((ptrdiff_t)OP_MEM_SIZE_MAX)

/*The context information needed to read from a block of memory as if it were a
   file.*/
struct OpusMemStream{
  /*The block of memory to read from.*/
  const unsigned char *data;
  /*The total size of the block.
    This must be at most OP_MEM_SIZE_MAX to prevent signed overflow while
     seeking.*/
  ptrdiff_t            size;
  /*The current file position.
    This is allowed to be set arbitrarily greater than size (i.e., past the end
     of the block, though we will not read data past the end of the block), but
     is not allowed to be negative (i.e., before the beginning of the block).*/
  ptrdiff_t            pos;
};

static int op_mem_read(void *_stream,unsigned char *_ptr,int _buf_size){
  OpusMemStream *stream;
  ptrdiff_t      size;
  ptrdiff_t      pos;
  stream=(OpusMemStream *)_stream;
  /*Check for empty read.*/
  if(_buf_size<=0)return 0;
  size=stream->size;
  pos=stream->pos;
  /*Check for EOF.*/
  if(pos>=size)return 0;
  /*Check for a short read.*/
  _buf_size=(int)OP_MIN(size-pos,_buf_size);
  memcpy(_ptr,stream->data+pos,_buf_size);
  pos+=_buf_size;
  stream->pos=pos;
  return _buf_size;
}

static int op_mem_seek(void *_stream,opus_int64 _offset,int _whence){
  OpusMemStream *stream;
  ptrdiff_t      pos;
  stream=(OpusMemStream *)_stream;
  pos=stream->pos;
  OP_ASSERT(pos>=0);
  switch(_whence){
    case SEEK_SET:{
      /*Check for overflow:*/
      if(_offset<0||_offset>OP_MEM_DIFF_MAX)return -1;
      pos=(ptrdiff_t)_offset;
    }break;
    case SEEK_CUR:{
      /*Check for overflow:*/
      if(_offset<-pos||_offset>OP_MEM_DIFF_MAX-pos)return -1;
      pos=(ptrdiff_t)(pos+_offset);
    }break;
    case SEEK_END:{
      ptrdiff_t size;
      size=stream->size;
      OP_ASSERT(size>=0);
      /*Check for overflow:*/
      if(_offset<-size||_offset>OP_MEM_DIFF_MAX-size)return -1;
      pos=(ptrdiff_t)(size+_offset);
    }break;
    default:return -1;
  }
  stream->pos=pos;
  return 0;
}

static opus_int64 op_mem_tell(void *_stream){
  OpusMemStream *stream;
  stream=(OpusMemStream *)_stream;
  return (ogg_int64_t)stream->pos;
}

static int op_mem_close(void *_stream){
  _ogg_free(_stream);
  return 0;
}

static const OpusFileCallbacks OP_MEM_CALLBACKS={
  op_mem_read,
  op_mem_seek,
  op_mem_tell,
  op_mem_close
};

void *op_mem_stream_create(OpusFileCallbacks *_cb,
 const unsigned char *_data,size_t _size){
  OpusMemStream *stream;
  if(_size>OP_MEM_SIZE_MAX)return NULL;
  stream=(OpusMemStream *)_ogg_malloc(sizeof(*stream));
  if(stream!=NULL){
    *_cb=*&OP_MEM_CALLBACKS;
    stream->data=_data;
    stream->size=_size;
    stream->pos=0;
  }
  return stream;
}

/*ITE add*/
void op_mem_stream_reset(void *_stream,const unsigned char *_data,size_t _size){
  OpusMemStream *stream;
  stream=(OpusMemStream *)_stream;
  if(stream!=NULL){
      stream->data=_data;
      stream->size=_size;
  }
}