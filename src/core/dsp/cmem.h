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
#ifndef _CMEM_H
#define _CMEM_H
#include "u_lockable.h"

extern "C"
{
    #include "pciedrv.h"
    #include "cmem_drv.h"
    #include "bufmgr.h"
}

#define HOST_CMEM_BUFFER_SIZE       0x400000 // 4M
#define MAX_NUM_HOST_DSP_BUFFERS    128

class Cmem : public Lockable_off
{
  public:
    ~Cmem() { close(); }
    static Cmem* instance ();

    void open();
    void close();
    void dma_write(int32_t dsp_id, uint32_t addr, uint8_t *buf, uint32_t size);
    void dma_read (int32_t dsp_id, uint32_t addr, uint8_t *buf, uint32_t size);

  private:
    static Cmem* pInstance;

    cmem_host_buf_desc_t buf_desc[MAX_NUM_HOST_DSP_BUFFERS];
    void *               DmaBufPool;

    Cmem() : DmaBufPool(NULL) { open(); }
    Cmem(const Cmem&);              // copy ctor disallowed
    Cmem& operator=(const Cmem&);   // assignment disallowed
};

#endif // _CMEM_H
