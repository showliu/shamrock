/******************************************************************************
 * Copyright (c) 2011-2013, Peter Collingbourne <peter@pcc.me.uk>
 * Copyright (c) 2013, Texas Instruments Incorporated - http://www.ti.com/
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
#include "clc.h"

_CLC_OVERLOAD _CLC_DEF float3 cross(float3 p0, float3 p1) 
{
   return (float3)(p0.y*p1.z - p0.z*p1.y, 
		   p0.z*p1.x - p0.x*p1.z,
		   p0.x*p1.y - p0.y*p1.x);
}

_CLC_OVERLOAD _CLC_DEF float4 cross(float4 p0, float4 p1) 
{
   return (float4)(p0.y*p1.z - p0.z*p1.y, 
	           p0.z*p1.x - p0.x*p1.z,
		   p0.x*p1.y - p0.y*p1.x, 
		   0.f);
}

_CLC_OVERLOAD _CLC_DEF double3 cross(double3 p0, double3 p1) 
{
   return (double3)(p0.y*p1.z - p0.z*p1.y, 
		   p0.z*p1.x  - p0.x*p1.z,
		   p0.x*p1.y  - p0.y*p1.x);
}

_CLC_OVERLOAD _CLC_DEF double4 cross(double4 p0, double4 p1) 
{
   return (double4)(p0.y*p1.z - p0.z*p1.y, 
	           p0.z*p1.x  - p0.x*p1.z,
		   p0.x*p1.y  - p0.y*p1.x, 
		   0.);
}
