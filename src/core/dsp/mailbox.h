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
#ifndef _MAILBOX_H_
#define _MAILBOX_H_
#include "u_locks_pthread.h"
#include "driver.h"

extern "C"
{
    #include "mpm_mailbox.h"
}

class Mailbox
{
  public:

    int32_t create(void* mbox_handle, char *slave_node_name, 
                   uint32_t mem_location, uint32_t direction, 
                   mpm_mailbox_config_t *mbox_config)
    { 
       int32_t result = mpm_mailbox_create(mbox_handle, slave_node_name,
                                        mem_location, direction, mbox_config);
        return result;
    }

    int32_t open(void* mbox_handle)
    { 
        int32_t result = mpm_mailbox_open(mbox_handle); 
        return result;
    }

    int32_t write (void* mbox_handle, uint8_t *buf, uint32_t size, 
                   uint32_t trans_id)
    { 
        int result;

        do result = mpm_mailbox_write (mbox_handle, buf, size, trans_id); 
        while (result == MPM_MAILBOX_ERR_MAIL_BOX_FULL);

        return true;
    }

    int32_t read (void* mbox_handle, uint8_t *buf, uint32_t *size, 
                  uint32_t *trans_id)
    { 
        int32_t result = mpm_mailbox_read (mbox_handle, buf, size, trans_id); 
        return result;
    }

    int32_t query (void* mbox_handle)
    { 
        int32_t result = mpm_mailbox_query (mbox_handle); 
        return result;
    }

    /*-------------------------------------------------------------------------
    * Thread safe instance function for singleton behavior
    *------------------------------------------------------------------------*/
    static Mailbox* instance () 
    {
        static Mutex Mailbox_instance_mutex;
        Mailbox* tmp = pInstance;

        __sync_synchronize();

        if (tmp == 0) 
        {
            ScopedLock lck(Mailbox_instance_mutex);

            tmp = pInstance;
            if (tmp == 0) 
            {
                tmp = new Mailbox;
                __sync_synchronize();
                pInstance = tmp;
            }
        }
        return tmp;
    }

  private:
    static Mailbox* pInstance;

    Mailbox() { }                         // ctor private
    Mailbox(const Mailbox&);              // copy ctor disallowed
    Mailbox& operator=(const Mailbox&);   // assignment disallowed
};

#endif // _MAILBOX_H_
