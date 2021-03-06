/*
 * Copyright (c) 2011, Denis Steckelmacher <steckdenis@yahoo.fr>
 * Copyright (c) 2012-2014, Texas Instruments Incorporated - http://www.ti.com/
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the copyright holder nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * \file api_device.cpp
 * \brief Devices
 */

#include "CL/cl.h"
#include <core/platform.h>
#include <core/deviceinterface.h>

cl_int
clGetDeviceIDs(cl_platform_id   platform,
               cl_device_type   device_type,
               cl_uint          num_entries,
               cl_device_id *   devices,
               cl_uint *        num_devices)
{
    /*-------------------------------------------------------------------------
    * We currently implement only one platform
    *------------------------------------------------------------------------*/
    if (!platform) platform = &the_platform;

    if (platform != &the_platform)     return CL_INVALID_PLATFORM;
    if (num_entries == 0 && devices != 0) return CL_INVALID_VALUE;
    if (num_devices == 0 && devices == 0) return CL_INVALID_VALUE;

    int device_number = platform->getDevices(device_type,
                                         num_entries, devices);

    if (num_devices) *num_devices = device_number;

    if (device_number == 0)
        return CL_DEVICE_NOT_FOUND;

    return CL_SUCCESS;
}

cl_int
clGetDeviceInfo(cl_device_id    d_device,
                cl_device_info  param_name,
                size_t          param_value_size,
                void *          param_value,
                size_t *        param_value_size_ret)
{
    auto device = pobj(d_device);

    if (!device->isA(Coal::Object::T_Device))
        return CL_INVALID_DEVICE;

    return device->info(param_name, param_value_size, param_value,
                       param_value_size_ret);
}

cl_int
clCreateSubDevices(cl_device_id                         d_in_device,
                   const cl_device_partition_property * properties,
                   cl_uint                              num_devices,
                   cl_device_id *                       out_devices,
                   cl_uint *                            num_devices_ret)
{
    auto in_device = pobj(d_in_device);

    if (!in_device->isA(Coal::Object::T_Device))
        return CL_INVALID_DEVICE;

    return in_device->createSubDevices(properties, num_devices, out_devices, num_devices_ret);
}

cl_int
clRetainDevice(cl_device_id d_device)
{
    auto device = pobj(d_device);

    if (!device->isA(Coal::Object::T_Device))
        return CL_INVALID_DEVICE;

    device->reference();

    return CL_SUCCESS;
}

cl_int
clReleaseDevice(cl_device_id d_device)
{
    auto device = pobj(d_device);

    if (!device->isA(Coal::Object::T_Device))
        return CL_INVALID_DEVICE;

    if (device->dereference())
        delete device;

    return CL_SUCCESS;
}
