/******************************************************************************
 * Copyright (c) 2013-2014, Texas Instruments Incorporated - http://www.ti.com/
 *   All rights reserved.
 *
 *   Redistribution and use in source and binary forms, with or without
 *   modification, are permitted provided that the following conditions are met:
 *       * Redistributions of source code must retain the above copyright
 *         notice, this list of conditions and the following disclaimer.
 *       * Redistributions in binary form must reproduce the above copyright
 *         notice, this list of conditions and the following disclaimer in the
 *         documentation and/or other materials provided with the distribution.
 *       * Neither the name of Texas Instruments Incorporated nor the
 *         names of its contributors may be used to endorse or promote products
 *         derived from this software without specific prior written permission.
 *
 *   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *   AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *   IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
 *   ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 *   LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *   CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *   SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *   INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *   CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *   ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 *   THE POSSIBILITY OF SUCH DAMAGE.
 *****************************************************************************/
#ifndef _CPU_CLC_H_
#define _CPU_CLC_H_

#include "clc.h"

#define PREFETCH_VECTORIZE(PRIM_TYPE) \
  _CLC_OVERLOAD _CLC_INLINE void prefetch(const __global PRIM_TYPE    *p, size_t num_gentypes){} \
   _CLC_OVERLOAD _CLC_INLINE void prefetch(const __global PRIM_TYPE##2 *p, size_t num_gentypes){} \
   _CLC_OVERLOAD _CLC_INLINE void prefetch(const __global PRIM_TYPE##3 *p, size_t num_gentypes){} \
   _CLC_OVERLOAD _CLC_INLINE void prefetch(const __global PRIM_TYPE##4 *p, size_t num_gentypes){} \
   _CLC_OVERLOAD _CLC_INLINE void prefetch(const __global PRIM_TYPE##8 *p, size_t num_gentypes){} \
   _CLC_OVERLOAD _CLC_INLINE void prefetch(const __global PRIM_TYPE##16 *p, size_t num_gentypes){} \


#define PREFETCH_TYPES()           \
        PREFETCH_VECTORIZE(char)   \
        PREFETCH_VECTORIZE(uchar)  \
        PREFETCH_VECTORIZE(short)  \
        PREFETCH_VECTORIZE(ushort) \
        PREFETCH_VECTORIZE(int)    \
        PREFETCH_VECTORIZE(uint)   \
        PREFETCH_VECTORIZE(long)   \
        PREFETCH_VECTORIZE(ulong)  \
        PREFETCH_VECTORIZE(float)  \
        PREFETCH_VECTORIZE(double) \

PREFETCH_TYPES()

/*-----------------------------------------------------------------------------
* This can be empty since our copy routines are currently synchronous. When 
* the copy routines are improved to be asynchronous, then this function will
* need a real implementation.
*----------------------------------------------------------------------------*/
#define wait_group_events(num_events, event_list)

#define CROSS_SIZES(type) \
    TEMPLATE(type)              \
    TEMPLATE(_VEC_TYPE(type,2))  \
    TEMPLATE(_VEC_TYPE(type,3))  \
    TEMPLATE(_VEC_TYPE(type,4))  \
    TEMPLATE(_VEC_TYPE(type,8))  \
    TEMPLATE(_VEC_TYPE(type,16)) \

#define TEMPLATE(gentype) \
_CLC_OVERLOAD _CLC_DECL event_t async_work_group_copy(local gentype *dst, const global gentype *src, \
		              size_t num_gentypes, event_t event); \
_CLC_OVERLOAD _CLC_DECL event_t async_work_group_copy(global gentype *dst, const local gentype *src, \
		              size_t num_gentypes, event_t event); \
_CLC_OVERLOAD _CLC_DECL event_t async_work_group_copy(global gentype *dst, const global gentype *src, \
		              size_t num_gentypes, event_t event); \
_CLC_OVERLOAD _CLC_DECL event_t async_work_group_strided_copy(local gentype *dst, const global gentype *src, \
		              size_t num_gentypes, size_t src_stride, event_t event); \
_CLC_OVERLOAD _CLC_DECL event_t async_work_group_strided_copy(global gentype *dst, const local gentype *src, \
		              size_t num_gentypes, size_t dst_stride, event_t event); \

CROSS_SIZES(char)
CROSS_SIZES(uchar)
CROSS_SIZES(short)
CROSS_SIZES(ushort)
CROSS_SIZES(int)
CROSS_SIZES(uint)
CROSS_SIZES(long)
CROSS_SIZES(ulong)
CROSS_SIZES(float)
CROSS_SIZES(double)

#undef CROSS_SIZES
#undef TEMPLATE

_CLC_OVERLOAD _CLC_DECL char  rotate(char v, char i);
_CLC_OVERLOAD _CLC_DECL uchar rotate(uchar v, uchar i);
_CLC_OVERLOAD _CLC_DECL short  rotate(short v, short i);
_CLC_OVERLOAD _CLC_DECL ushort rotate(ushort v, ushort i);
_CLC_OVERLOAD _CLC_DECL long  rotate(long v, long i);
_CLC_OVERLOAD _CLC_DECL ulong rotate(ulong v, ulong i);
_CLC_OVERLOAD _CLC_DECL int  rotate(int v, int i);
_CLC_OVERLOAD _CLC_DECL uint rotate(uint v, uint i);

BINARY_VEC_DECL(char, char, rotate)
BINARY_VEC_DECL(uchar, uchar, rotate)
BINARY_VEC_DECL(short, short, rotate)
BINARY_VEC_DECL(ushort, ushort, rotate)
BINARY_VEC_DECL(int, int, rotate)
BINARY_VEC_DECL(uint, uint, rotate)
BINARY_VEC_DECL(long, long, rotate)
BINARY_VEC_DECL(ulong, ulong, rotate)

_CLC_OVERLOAD _CLC_INLINE char   clz(char   v) { return (v<0 ? 0 : (__builtin_clzs(v)-8)); }
_CLC_OVERLOAD _CLC_INLINE uchar  clz(uchar  v) { return (v==0 ? 8 : __builtin_clzs(v)- 8); }
_CLC_OVERLOAD _CLC_INLINE short  clz(short  v) { return (v==0 ? 16 : __builtin_clzs(v)); }
_CLC_OVERLOAD _CLC_INLINE ushort clz(ushort v) { return (v==0 ? 16 : __builtin_clzs(v)); }
_CLC_OVERLOAD _CLC_INLINE int    clz(int    v) { return (v==0 ? 32 : __builtin_clz(v)); }
_CLC_OVERLOAD _CLC_INLINE uint   clz(uint   v) { return (v==0 ? 32 : __builtin_clz(v)); }
_CLC_OVERLOAD _CLC_INLINE long clz(long   v) { return (v==0 ? 64 : __builtin_clzl(v)); }
_CLC_OVERLOAD _CLC_INLINE ulong clz(ulong  v) { return (v==0 ? 64 : __builtin_clzl(v)); }

UNARY_VEC_DECL(char, char, clz)
UNARY_VEC_DECL(uchar, uchar, clz)
UNARY_VEC_DECL(short, short, clz)
UNARY_VEC_DECL(ushort, ushort, clz)
UNARY_VEC_DECL(int, int, clz)
UNARY_VEC_DECL(uint, uint, clz)
UNARY_VEC_DECL(long, long, clz)
UNARY_VEC_DECL(ulong, ulong, clz)

_CLC_OVERLOAD _CLC_INLINE uchar  abs(char x)    { return __builtin_abs(x); }
_CLC_OVERLOAD _CLC_INLINE ushort abs(short x)   { return __builtin_abs(x); }
_CLC_OVERLOAD _CLC_INLINE uint   abs(int x)     { return __builtin_abs(x); }
_CLC_OVERLOAD _CLC_INLINE ulong  abs(long x)    { if (x < 0) x = -x; return x; }

_CLC_OVERLOAD _CLC_INLINE uchar  abs(uchar x)   { return x; }
_CLC_OVERLOAD _CLC_INLINE ushort abs(ushort x)  { return x; }
_CLC_OVERLOAD _CLC_INLINE uint   abs(uint x)    { return x; }
_CLC_OVERLOAD _CLC_INLINE ulong  abs(ulong x)   { return x; }

UNARY_VEC_DECL(char,  uchar,  abs)
UNARY_VEC_DECL(short, ushort, abs)
UNARY_VEC_DECL(int,   uint,   abs)
UNARY_VEC_DECL(long,  ulong,  abs)

/*-----------------------------------------------------------------------------
* ABS for unsigned types is straightforward
*----------------------------------------------------------------------------*/
#define DEFINE(type, utype) \
    _CLC_OVERLOAD _CLC_INLINE _VEC_TYPE(utype,2)  abs(_VEC_TYPE(utype,2) x)  {return x;}\
    _CLC_OVERLOAD _CLC_INLINE _VEC_TYPE(utype,3)  abs(_VEC_TYPE(utype,3) x)  {return x;}\
    _CLC_OVERLOAD _CLC_INLINE _VEC_TYPE(utype,4)  abs(_VEC_TYPE(utype,4) x)  {return x;}\
    _CLC_OVERLOAD _CLC_INLINE _VEC_TYPE(utype,8)  abs(_VEC_TYPE(utype,8) x)  {return x;}\
    _CLC_OVERLOAD _CLC_INLINE _VEC_TYPE(utype,16) abs(_VEC_TYPE(utype,16) x) {return x;}\

DEFINE(uchar, uchar)
DEFINE(ushort, ushort)
DEFINE(uint, uint)
DEFINE(ulong, ulong)

#undef DEFINE

_CLC_OVERLOAD _CLC_DECL long   mul_hi(long x,   long y);
_CLC_OVERLOAD _CLC_DECL ulong  mul_hi(ulong x,   ulong y);
_CLC_OVERLOAD _CLC_DECL char  mul_hi(char x,  char y);
_CLC_OVERLOAD _CLC_DECL uchar mul_hi(uchar x, uchar y);
_CLC_OVERLOAD _CLC_DECL short  mul_hi(short x,  short y);
_CLC_OVERLOAD _CLC_DECL ushort mul_hi(ushort x, ushort y);
_CLC_OVERLOAD _CLC_DECL int    mul_hi(int x,    int y);
_CLC_OVERLOAD _CLC_DECL uint   mul_hi(uint x,   uint y);

BINARY_VEC_DECL(char, char, mul_hi)
BINARY_VEC_DECL(uchar, uchar, mul_hi)
BINARY_VEC_DECL(short, short, mul_hi)
BINARY_VEC_DECL(ushort, ushort, mul_hi)
BINARY_VEC_DECL(int, int, mul_hi)
BINARY_VEC_DECL(uint, uint, mul_hi)
BINARY_VEC_DECL(long, long, mul_hi)
BINARY_VEC_DECL(ulong, ulong, mul_hi)


_CLC_OVERLOAD _CLC_DECL char   add_sat(char x, char y)   ;
_CLC_OVERLOAD _CLC_DECL uchar  add_sat(uchar x, uchar y) ;
_CLC_OVERLOAD _CLC_DECL short  add_sat(short x, short y) ;
_CLC_OVERLOAD _CLC_DECL ushort add_sat(ushort x, ushort y) ;
_CLC_OVERLOAD _CLC_DECL int    add_sat(int x, int y)     ;
_CLC_OVERLOAD _CLC_DECL uint   add_sat(uint x, uint y)   ;
_CLC_OVERLOAD _CLC_DECL long   add_sat(long x, long y)   ;
_CLC_OVERLOAD _CLC_DECL ulong   add_sat(ulong x, ulong y)   ;

BINARY_VEC_DECL(char, char, add_sat)
BINARY_VEC_DECL(uchar, uchar, add_sat)
BINARY_VEC_DECL(short, short, add_sat)
BINARY_VEC_DECL(ushort, ushort, add_sat)
BINARY_VEC_DECL(int, int, add_sat)
BINARY_VEC_DECL(uint, uint, add_sat)
BINARY_VEC_DECL(long, long, add_sat)
BINARY_VEC_DECL(ulong, ulong, add_sat)


_CLC_OVERLOAD _CLC_DECL char   sub_sat(char x, char y)   ;
_CLC_OVERLOAD _CLC_DECL uchar  sub_sat(uchar x, uchar y) ;
_CLC_OVERLOAD _CLC_DECL short  sub_sat(short x, short y) ;
_CLC_OVERLOAD _CLC_DECL ushort sub_sat(ushort x, ushort y) ;
_CLC_OVERLOAD _CLC_DECL int    sub_sat(int x, int y)     ;
_CLC_OVERLOAD _CLC_DECL uint   sub_sat(uint x, uint y)   ;
_CLC_OVERLOAD _CLC_DECL long   sub_sat(long x, long y)   ;
_CLC_OVERLOAD _CLC_DECL ulong   sub_sat(ulong x, ulong y)   ;

BINARY_VEC_DECL(char, char, sub_sat)
BINARY_VEC_DECL(uchar, uchar, sub_sat)
BINARY_VEC_DECL(short, short, sub_sat)
BINARY_VEC_DECL(ushort, ushort, sub_sat)
BINARY_VEC_DECL(int, int, sub_sat)
BINARY_VEC_DECL(uint, uint, sub_sat)
BINARY_VEC_DECL(long, long, sub_sat)
BINARY_VEC_DECL(ulong, ulong, sub_sat)

_CLC_OVERLOAD _CLC_INLINE short   upsample(char x, uchar y)
{ return (short)x << 8 | y; }

_CLC_OVERLOAD _CLC_INLINE ushort  upsample(uchar x, uchar y)
{ return (ushort)x << 8 | y; }

_CLC_OVERLOAD _CLC_INLINE int     upsample(short x, ushort y)
{ return (int) x << 16 | y; }

_CLC_OVERLOAD _CLC_INLINE uint    upsample(ushort x, ushort y)
{ return (uint)  x << 16 | y; }

_CLC_OVERLOAD _CLC_INLINE long     upsample(int x, uint y)
{ return (long) x << 32 | y; }

_CLC_OVERLOAD _CLC_INLINE ulong    upsample(uint x, uint y)
{ return (ulong) x << 32 | y; }

BINARY_VEC_DECL_ALT(char,  short,  uchar, upsample)
BINARY_VEC_DECL_ALT(uchar, ushort, uchar, upsample)
BINARY_VEC_DECL_ALT(short,  int,  ushort, upsample)
BINARY_VEC_DECL_ALT(ushort, uint, ushort, upsample)
BINARY_VEC_DECL_ALT(int,  long,  uint, upsample)
BINARY_VEC_DECL_ALT(uint, ulong, uint, upsample)

_CLC_OVERLOAD _CLC_DECL char mad_sat(char a, char b, char c);
_CLC_OVERLOAD _CLC_DECL uchar mad_sat(uchar a, uchar b, uchar c);
_CLC_OVERLOAD _CLC_DECL short mad_sat(short a, short b, short c);
_CLC_OVERLOAD _CLC_DECL ushort mad_sat(ushort a, ushort b, ushort c);
_CLC_OVERLOAD _CLC_DECL int mad_sat(int a, int b, int c);
_CLC_OVERLOAD _CLC_DECL uint mad_sat(uint a, uint b, uint c);
_CLC_OVERLOAD _CLC_DECL long mad_sat(long a, long b, long c);
_CLC_OVERLOAD _CLC_DECL ulong mad_sat(ulong a, ulong b, ulong c);

TERNARY_VEC_DECL(char, char, mad_sat)
TERNARY_VEC_DECL(uchar, uchar, mad_sat)
TERNARY_VEC_DECL(short, short, mad_sat)
TERNARY_VEC_DECL(ushort, ushort, mad_sat)
TERNARY_VEC_DECL(int, int, mad_sat)
TERNARY_VEC_DECL(uint, uint, mad_sat)
TERNARY_VEC_DECL(long, long, mad_sat)
TERNARY_VEC_DECL(ulong, ulong, mad_sat)

int printf(__constant char* _format, ...);
void *memcpy(void *dst, const void * src, uint size);

_CLC_DECL  size_t  get_local_id     (uint dim);
_CLC_DECL  uint    get_work_dim     (void)     ;
_CLC_DECL  size_t  get_global_size  (uint dim) ;
_CLC_DECL  size_t  get_local_size   (uint dim) ;
_CLC_DECL  size_t  get_global_offset(uint dim) ;
_CLC_DECL  size_t __get_global_first(uint dim) ;
_CLC_DECL  size_t  get_num_groups   (uint dim)  ;
_CLC_DECL  size_t  get_global_id    (uint dim) ;
_CLC_DECL  size_t  get_group_id     (uint dim) ;
                     
#endif //_CPU_CLC_H_

