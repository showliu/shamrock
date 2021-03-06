/*
 * Copyright (c) 2011, Denis Steckelmacher <steckdenis@yahoo.fr>
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
 * \file cpu/device.cpp
 * \brief CPU Device
 */

#include "../platform.h"
#include "device.h"
#include "buffer.h"
#include "kernel.h"
#include "program.h"
#include "worker.h"
#include "builtins.h"

#include <core/config.h>
#include "../propertylist.h"
#include "../commandqueue.h"
#include "../events.h"
#include "../memobject.h"
#include "../kernel.h"
#include "../program.h"
#include "../util.h"

#include <cstring>
#include <cstdlib>
#include <unistd.h>

#include <iostream>
#include <fstream>
#include <sstream>

using namespace Coal;

#if !(defined(DSPC868X) || defined(SHAMROCK_BUILD))
#include "../dsp/shmem.h"
// unsigned arm_speed();
#endif

#define ONE_MEGABYTE (1024 * 1024)
#define ONE_GIGABYTE (1024 * ONE_MEGABYTE)
#define HALF_GIGABYTE (512 * ONE_MEGABYTE)

CPUDevice::CPUDevice(DeviceInterface *parent_device, unsigned int cores)
: DeviceInterface(), p_num_events(0), p_workers(0), p_stop(false),
  p_initialized(false)
{
    // If this is a root device, then the number of cores is that of the system...
    p_parent_device = parent_device;
    if (p_parent_device == NULL) {
        p_cores = sysconf(_SC_NPROCESSORS_ONLN);
    }
    else {
        // Otherwise, it was computed by createSubDevices and passed in:
        p_cores = cores;
        p_partition_properties[0] = 0;
    }

    // Determine frequency:
    p_cpu_mhz = 0.0f;

    std::filebuf fb;
    fb.open("/proc/cpuinfo", std::ios::in);
    std::istream is(&fb);

    while (!is.eof())
    {
        std::string key, value;

        std::getline(is, key, ':');
        is.ignore(1);
        std::getline(is, value);

        if (key.compare(0, 7, "cpu MHz") == 0)
        {
            std::istringstream ss(value);
            ss >> p_cpu_mhz;
        }

        if (key.compare(0, 10, "model name") == 0)
            p_device_name = value;

        if (key.compare(0, 9, "Processor") == 0)
            p_device_name = value;
    }

    if (p_cpu_mhz == 0.0f)
    {
      std::string file("/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq");
      std::ifstream fs(file.c_str());
      if (fs) { fs >> p_cpu_mhz; p_cpu_mhz /= 1000; }
    }

    if (p_cpu_mhz == 0.0f) p_cpu_mhz = 1000.0;

#if !defined(DSPC868X)
    // p_cpu_mhz = arm_speed();
#endif
}


void CPUDevice::init()
{
    if (p_initialized) return;

    // Initialize the locking machinery
    pthread_cond_init(&p_events_cond, 0);
    pthread_mutex_init(&p_events_mutex, 0);

    // Create worker threads
    p_workers = (pthread_t *)std::malloc(numCPUs() * sizeof(pthread_t));

    for (unsigned int i=0; i<numCPUs(); ++i)
    {
        pthread_create(&p_workers[i], 0, &worker, this);
    }

    p_initialized = true;
}

CPUDevice::~CPUDevice()
{
    if (!p_initialized)
        return;

    // Terminate the workers and wait for them
    pthread_mutex_lock(&p_events_mutex);

    p_stop = true;

    pthread_cond_broadcast(&p_events_cond);
    pthread_mutex_unlock(&p_events_mutex);

    for (unsigned int i=0; i<numCPUs(); ++i)
    {
        pthread_join(p_workers[i], 0);
    }

    // Free allocated memory
    std::free((void *)p_workers);
    pthread_mutex_destroy(&p_events_mutex);
    pthread_cond_destroy(&p_events_cond);
}

DeviceBuffer *CPUDevice::createDeviceBuffer(MemObject *buffer, cl_int *rs)
{
    return (DeviceBuffer *)new CPUBuffer(this, buffer, rs);
}

DeviceProgram *CPUDevice::createDeviceProgram(Program *program)
{
    return (DeviceProgram *)new CPUProgram(this, program);
}

DeviceKernel *CPUDevice::createDeviceKernel(Kernel *kernel,
                                            llvm::Function *function)
{
    return (DeviceKernel *)new CPUKernel(this, kernel, function);
}

cl_int CPUDevice::initEventDeviceData(Event *event)
{
    switch (event->type())
    {
        case Event::MapBuffer:
        {
            MapBufferEvent *e = (MapBufferEvent *)event;
            CPUBuffer *buf = (CPUBuffer *)e->buffer()->deviceBuffer(this);
            unsigned char *data = (unsigned char *)buf->data();

            data += e->offset();

            e->setPtr((void *)data);
            break;
        }
        case Event::MapImage:
        {
            MapImageEvent *e = (MapImageEvent *)event;
            Image2D *image = (Image2D *)e->buffer();
            CPUBuffer *buf = (CPUBuffer *)image->deviceBuffer(this);
            unsigned char *data = (unsigned char *)buf->data();

            data = imageData(data,
                             e->origin(0),
                             e->origin(1),
                             e->origin(2),
                             image->row_pitch(),
                             image->slice_pitch(),
                             image->pixel_size());

            e->setPtr((void *)data);
            e->setRowPitch(image->row_pitch());
            e->setSlicePitch(image->slice_pitch());
            break;
        }
        case Event::UnmapMemObject:
            // Nothing do to
            break;

        case Event::NDRangeKernel:
        case Event::TaskKernel:
        {
            // Instantiate the JIT for the CPU program
            KernelEvent *e = (KernelEvent *)event;
            Program *p = (Program *)e->kernel()->parent();
            CPUProgram *prog = (CPUProgram *)p->deviceDependentProgram(this);

            if (!prog->initJIT())
                return CL_INVALID_PROGRAM_EXECUTABLE;

            // Set device-specific data
            CPUKernelEvent *cpu_e = new CPUKernelEvent(this, e);
            e->setDeviceData((void *)cpu_e);

            break;
        }
        default:
            break;
    }

    return CL_SUCCESS;
}

void CPUDevice::freeEventDeviceData(Event *event)
{
    switch (event->type())
    {
        case Event::NDRangeKernel:
        case Event::TaskKernel:
        {
            CPUKernelEvent *cpu_e = (CPUKernelEvent *)event->deviceData();

            if (cpu_e)
                delete cpu_e;
        }
        default:
            break;
    }
}

void CPUDevice::pushEvent(Event *event)
{
    // Add an event in the list
    pthread_mutex_lock(&p_events_mutex);

    p_events.push_back(event);
    p_num_events++;                 // Way faster than STL list::size() !

    pthread_cond_broadcast(&p_events_cond);
    pthread_mutex_unlock(&p_events_mutex);
}

Event *CPUDevice::getEvent(bool &stop)
{
    // Return the first event in the list, if any. Remove it if it is a
    // single-shot event.
    pthread_mutex_lock(&p_events_mutex);

    while (p_num_events == 0 && !p_stop)
        pthread_cond_wait(&p_events_cond, &p_events_mutex);

    if (p_stop)
    {
        pthread_mutex_unlock(&p_events_mutex);
        stop = true;
        return 0;
    }

    Event *event = p_events.front();

    // If the run of this event will finish it, remove it from the list
    bool last_slot = true;

    if (event->type() == Event::NDRangeKernel ||
        event->type() == Event::TaskKernel)
    {
        CPUKernelEvent *ke = (CPUKernelEvent *)event->deviceData();
        last_slot = ke->reserve();
    }

    if (last_slot)
    {
        p_num_events--;
        p_events.pop_front();
    }

    pthread_mutex_unlock(&p_events_mutex);

    return event;
}

/******************************************************************************
* Device's decision about whether CommandQueue should push more events over
* This number could be tuned (e.g. using ooo example).  Note that p_num_events
* are in device's queue, but not yet executed.
******************************************************************************/
bool CPUDevice::gotEnoughToWorkOn()
{
    return p_num_events > 0;
}

cl_int CPUDevice::createSubDevices(
                   const cl_device_partition_property * properties,
                   cl_uint                              num_devices,
                   cl_device_id *                       out_devices,
                   cl_uint *                            num_devices_ret)
{
    cl_int retval = CL_SUCCESS;
    unsigned int partition_size, num_new_devices = 0;
    unsigned int *cores_per_device = NULL;

    // CL_DEVICE_PARTITION_MAX_SUB_DEVICES

    // Determine if properties and property values are valid:
    if (properties) {
        // We support CL_DEVICE_PARTITION_EQUALLY and CL_DEVICE_PARTITION_BY_COUNTS
        if (properties[0] == CL_DEVICE_PARTITION_EQUALLY) {
            partition_size = properties[1];
            if (properties[2] != 0) {
                retval = CL_INVALID_VALUE;
            }
            else if (numCPUs() == 1) {
                retval = CL_DEVICE_PARTITION_FAILED; // cannot partition further.
            }
            else if (partition_size > 0 && partition_size <= numCPUs()) {
                num_new_devices = numCPUs() / partition_size;  // discards fraction.
            }
            else {
                retval = CL_INVALID_VALUE;
            }
        }
	else if (properties[0] == CL_DEVICE_PARTITION_BY_COUNTS) {
	    // TODO
	    retval = CL_INVALID_VALUE;
	}
	else {
            retval = CL_INVALID_VALUE;
	}
    }
    else {
        retval = CL_INVALID_VALUE;
    }

    if (retval == CL_SUCCESS && out_devices) {
        if (num_devices < num_new_devices) retval = CL_INVALID_VALUE;
    }

    assert(retval != CL_SUCCESS || partition_size);
    if (partition_size) {
        // Create num_new_devices SubDevices:
        Coal::CPUDevice * new_device;
        if (out_devices) {
            for (int i = 0; i < num_new_devices; i++) {
                new_device = new CPUDevice(this, partition_size);
                out_devices[i] = desc(new_device);
		new_device->setProperties(properties);
            }
        }
        if (num_devices_ret) *num_devices_ret = num_new_devices;
    }

    return (retval);
}


unsigned int CPUDevice::numCPUs() const
{
    return p_cores;
}

float CPUDevice::cpuMhz() const
{
    return p_cpu_mhz;
}

// From inner parentheses to outher ones :
//
// sizeof * 8 => 8
// -1         => 7
// 1 << $     => 10000000
// -1         => 01111111
// *2         => 11111110
// +1         => 11111111
//
// A simple way to do this is (1 << (sizeof(type) * 8)) - 1, but it overflows
// the type (for int8, 1 << $ = 100000000 = 256 > 255)
#define TYPE_MAX(type) ((((type)1 << ((sizeof(type) * 8) - 1)) - 1) * 2 + 1)

cl_int CPUDevice::info(cl_device_info param_name,
                       size_t param_value_size,
                       void *param_value,
                       size_t *param_value_size_ret) const
{
    void *value = 0;
    size_t value_length = 0;

    union {
        cl_device_type cl_device_type_var;
        cl_uint cl_uint_var;
        size_t size_t_var;
        cl_ulong cl_ulong_var;
        cl_bool cl_bool_var;
        cl_device_fp_config cl_device_fp_config_var;
        cl_device_mem_cache_type cl_device_mem_cache_type_var;
        cl_device_local_mem_type cl_device_local_mem_type_var;
        cl_device_exec_capabilities cl_device_exec_capabilities_var;
        cl_command_queue_properties cl_command_queue_properties_var;
        cl_platform_id cl_platform_id_var;
        size_t work_dims[MAX_WORK_DIMS];
        cl_device_id cl_device_id_var;
        cl_device_partition_property cl_device_partition_property_var[MAX_PARTITION_PROPS];
        cl_device_affinity_domain cl_device_affinity_domain_var;
    };

    switch (param_name)
    {
        case CL_DEVICE_TYPE:
            SIMPLE_ASSIGN(cl_device_type, CL_DEVICE_TYPE_CPU);
            break;

        case CL_DEVICE_VENDOR_ID:
            SIMPLE_ASSIGN(cl_uint, 0);
            break;

        case CL_DEVICE_MAX_COMPUTE_UNITS:
            SIMPLE_ASSIGN(cl_uint, numCPUs());
            break;

        case CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS:
            SIMPLE_ASSIGN(cl_uint, MAX_WORK_DIMS);
            break;

        /*-----------------------------------------------------------------
        * Set to a small enough size so that Khronos basic/local_kernel_def
        * test can allocate and pass.
        *----------------------------------------------------------------*/
        case CL_DEVICE_MAX_WORK_GROUP_SIZE:
            SIMPLE_ASSIGN(size_t, 46*1024);
            break;

        case CL_DEVICE_MAX_WORK_ITEM_SIZES:
            for (int i=0; i<MAX_WORK_DIMS; ++i)
            {
                work_dims[i] = 46*1024;
            }
            value_length = MAX_WORK_DIMS * sizeof(size_t);
            value = &work_dims;
            break;

        case CL_DEVICE_PREFERRED_VECTOR_WIDTH_CHAR:
            SIMPLE_ASSIGN(cl_uint, 16);
            break;

        case CL_DEVICE_PREFERRED_VECTOR_WIDTH_SHORT:
            SIMPLE_ASSIGN(cl_uint, 8);
            break;

        case CL_DEVICE_PREFERRED_VECTOR_WIDTH_INT:
            SIMPLE_ASSIGN(cl_uint, 4);
            break;

        case CL_DEVICE_PREFERRED_VECTOR_WIDTH_LONG:
            SIMPLE_ASSIGN(cl_uint, 2);
            break;

        case CL_DEVICE_PREFERRED_VECTOR_WIDTH_FLOAT:
            SIMPLE_ASSIGN(cl_uint, 4);
            break;

        case CL_DEVICE_PREFERRED_VECTOR_WIDTH_DOUBLE:
            SIMPLE_ASSIGN(cl_uint, 2);
            break;

        case CL_DEVICE_MAX_CLOCK_FREQUENCY:
            SIMPLE_ASSIGN(cl_uint, cpuMhz());
            break;

        case CL_DEVICE_ADDRESS_BITS:
            SIMPLE_ASSIGN(cl_uint, 8*sizeof(void *));
            break;

        case CL_DEVICE_MAX_READ_IMAGE_ARGS:
            SIMPLE_ASSIGN(cl_uint, 0);          //images not supported
            break;

        case CL_DEVICE_MAX_WRITE_IMAGE_ARGS:
            SIMPLE_ASSIGN(cl_uint, 0);          // images not supported
            break;

        case CL_DEVICE_IMAGE2D_MAX_WIDTH:
            SIMPLE_ASSIGN(size_t, 0);           // images not supported
            break;

        case CL_DEVICE_IMAGE2D_MAX_HEIGHT:
            SIMPLE_ASSIGN(size_t, 0);           //images not supported
            break;

        case CL_DEVICE_IMAGE3D_MAX_WIDTH:
            SIMPLE_ASSIGN(size_t, 0);           //images not supported
            break;

        case CL_DEVICE_IMAGE3D_MAX_HEIGHT:
            SIMPLE_ASSIGN(size_t, 0);           //images not supported
            break;

        case CL_DEVICE_IMAGE3D_MAX_DEPTH:
            SIMPLE_ASSIGN(size_t, 0);           //images not supported
            break;

        case CL_DEVICE_IMAGE_MAX_BUFFER_SIZE:
            SIMPLE_ASSIGN(size_t, 0);           //images not supported
            break;

        case CL_DEVICE_IMAGE_MAX_ARRAY_SIZE:
            SIMPLE_ASSIGN(size_t, 0);           //images not supported
            break;

        case CL_DEVICE_IMAGE_SUPPORT:
            SIMPLE_ASSIGN(cl_bool, CL_FALSE);   //images not supported
            break;

        case CL_DEVICE_MAX_PARAMETER_SIZE:
            SIMPLE_ASSIGN(size_t, 1024);        // This is the minimum
            break;

        case CL_DEVICE_MAX_SAMPLERS:
            SIMPLE_ASSIGN(cl_uint, 0);          //images not supported
            break;

        case CL_DEVICE_MEM_BASE_ADDR_ALIGN:     // in bits!
            SIMPLE_ASSIGN(cl_uint, 1024 /* sizeof(double16)*8) */);  // 128 byte
            break;

        case CL_DEVICE_MIN_DATA_TYPE_ALIGN_SIZE:  // in bytes!
	    SIMPLE_ASSIGN(cl_uint, 128  /* sizeof(double16) */);
            break;

        case CL_DEVICE_SINGLE_FP_CONFIG:
            // TODO: Check what an x86 SSE engine can support.
	    // Currently not supporting CL_FP_DENORM
            SIMPLE_ASSIGN(cl_device_fp_config,
                          CL_FP_INF_NAN | CL_FP_ROUND_TO_NEAREST);
            break;

        case CL_DEVICE_DOUBLE_FP_CONFIG:
            // These are minimally required to be supported by the OCL spec:
            SIMPLE_ASSIGN(cl_device_fp_config,
                   CL_FP_FMA | CL_FP_ROUND_TO_NEAREST | CL_FP_ROUND_TO_ZERO |
                   CL_FP_ROUND_TO_INF | CL_FP_INF_NAN | CL_FP_DENORM);
            break;

        case CL_DEVICE_GLOBAL_MEM_CACHE_TYPE:
            SIMPLE_ASSIGN(cl_device_mem_cache_type,
                          CL_READ_WRITE_CACHE);
            break;

        case CL_DEVICE_GLOBAL_MEM_CACHELINE_SIZE:
            // TODO: Get this information from the processor
            SIMPLE_ASSIGN(cl_uint, 16);
            break;

        case CL_DEVICE_GLOBAL_MEM_CACHE_SIZE:
            // TODO: Get this information from the processor
            SIMPLE_ASSIGN(cl_ulong, 512*1024*1024);
            break;

        case CL_DEVICE_GLOBAL_MEM_SIZE:
            // parse /proc/meminfo to get the value
            SIMPLE_ASSIGN(cl_ulong, parse_file_line_value("/proc/meminfo",
                                               "MemTotal:", 512*1024) * 1024);
            break;

        case CL_DEVICE_LOCAL_MEM_SIZE:
            SIMPLE_ASSIGN(cl_ulong, 128 * 1024);
	    break;

        case CL_DEVICE_MAX_MEM_ALLOC_SIZE:
	    {
            // Minimum size must be 1/4 the size of CL_DEVICE_GLOBAL_MEM_SIZE:
            unsigned long global_size_bytes = parse_file_line_value("/proc/meminfo",
                                              "MemTotal:", 512*1024) * 1024;
            SIMPLE_ASSIGN(cl_ulong, global_size_bytes >> 2);
	    }
            break;

        case CL_DEVICE_MAX_CONSTANT_BUFFER_SIZE:
            SIMPLE_ASSIGN(cl_ulong, 64<<10);
            break;

        case CL_DEVICE_MAX_CONSTANT_ARGS:
            SIMPLE_ASSIGN(cl_uint, 8);
            break;

        case CL_DEVICE_LOCAL_MEM_TYPE:
            SIMPLE_ASSIGN(cl_device_local_mem_type, CL_GLOBAL);
            break;


        case CL_DEVICE_ERROR_CORRECTION_SUPPORT:
            SIMPLE_ASSIGN(cl_bool, CL_FALSE);
            break;

        case CL_DEVICE_PROFILING_TIMER_RESOLUTION:
            // TODO
            SIMPLE_ASSIGN(size_t, 1000);        // 1000 nanoseconds = 1 ms
            break;

        case CL_DEVICE_ENDIAN_LITTLE:
            SIMPLE_ASSIGN(cl_bool, CL_TRUE);
            break;

        case CL_DEVICE_AVAILABLE:
            SIMPLE_ASSIGN(cl_bool, CL_TRUE);
            break;

        case CL_DEVICE_COMPILER_AVAILABLE:
            SIMPLE_ASSIGN(cl_bool, CL_TRUE);
            break;

        case CL_DEVICE_LINKER_AVAILABLE:
            SIMPLE_ASSIGN(cl_bool, CL_TRUE);
            break;

        case CL_DEVICE_EXECUTION_CAPABILITIES:
            SIMPLE_ASSIGN(cl_device_exec_capabilities, CL_EXEC_KERNEL |
                          CL_EXEC_NATIVE_KERNEL);
            break;

        case CL_DEVICE_QUEUE_PROPERTIES:
            SIMPLE_ASSIGN(cl_command_queue_properties,
                          CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE |
                          CL_QUEUE_PROFILING_ENABLE);
            break;

        case CL_DEVICE_BUILT_IN_KERNELS:
            STRING_ASSIGN("");
            break;

        case CL_DEVICE_PRINTF_BUFFER_SIZE:
            SIMPLE_ASSIGN(size_t, ONE_MEGABYTE);
	    break;

        case CL_DEVICE_PREFERRED_INTEROP_USER_SYNC:
            SIMPLE_ASSIGN(cl_bool, CL_TRUE);
	    break;

        case CL_DEVICE_NAME:
            value_length = p_device_name.size() + 1;
            value        = const_cast<char*>(p_device_name.c_str());
            break;

        case CL_DEVICE_VENDOR:
            STRING_ASSIGN("Generic");
            break;

        case CL_DRIVER_VERSION:
            STRING_ASSIGN("" COAL_VERSION);
            break;

        case CL_DEVICE_PROFILE:
            STRING_ASSIGN("FULL_PROFILE");
            break;

        case CL_DEVICE_VERSION:
            STRING_ASSIGN("OpenCL 1.2 " COAL_VERSION);
            break;

        case CL_DEVICE_EXTENSIONS:
            STRING_ASSIGN("cl_khr_global_int32_base_atomics"
                          " cl_khr_global_int32_extended_atomics"
                          " cl_khr_local_int32_base_atomics"
                          " cl_khr_local_int32_extended_atomics"
                          " cl_khr_byte_addressable_store"
                          " cl_khr_fp64");
            break;

        case CL_DEVICE_PLATFORM:
            SIMPLE_ASSIGN(cl_platform_id, &the_platform);
            break;

        case CL_DEVICE_PREFERRED_VECTOR_WIDTH_HALF:
            SIMPLE_ASSIGN(cl_uint, 0);
            break;

        case CL_DEVICE_HOST_UNIFIED_MEMORY:
            SIMPLE_ASSIGN(cl_bool, CL_TRUE);
            break;

        case CL_DEVICE_NATIVE_VECTOR_WIDTH_CHAR:
            SIMPLE_ASSIGN(cl_uint, 16);
            break;

        case CL_DEVICE_NATIVE_VECTOR_WIDTH_SHORT:
            SIMPLE_ASSIGN(cl_uint, 8);
            break;

        case CL_DEVICE_NATIVE_VECTOR_WIDTH_INT:
            SIMPLE_ASSIGN(cl_uint, 4);
            break;

        case CL_DEVICE_NATIVE_VECTOR_WIDTH_LONG:
            SIMPLE_ASSIGN(cl_uint, 2);
            break;

        case CL_DEVICE_NATIVE_VECTOR_WIDTH_FLOAT:
            SIMPLE_ASSIGN(cl_uint, 4);
            break;

        case CL_DEVICE_NATIVE_VECTOR_WIDTH_DOUBLE:
            SIMPLE_ASSIGN(cl_uint, 2);
            break;

        case CL_DEVICE_NATIVE_VECTOR_WIDTH_HALF:
            SIMPLE_ASSIGN(cl_uint, 0);
            break;

        case CL_DEVICE_OPENCL_C_VERSION:
            STRING_ASSIGN("OpenCL C 1.2 LLVM " LLVM_VERSION);
            break;

        case CL_DEVICE_PARENT_DEVICE:
            {
            auto d_device = desc(p_parent_device);
            SIMPLE_ASSIGN(cl_device_id, d_device);
            }
            break;
        case CL_DEVICE_PARTITION_MAX_SUB_DEVICES:
	    if (numCPUs() == 1) {
	        // cannot subdivide further:
                SIMPLE_ASSIGN(cl_uint, 0);
            }
	    else {
	        // Can have each sub-device have one compute unit:
                SIMPLE_ASSIGN(cl_uint, numCPUs());
	    }
            break;
        case CL_DEVICE_PARTITION_PROPERTIES:
            value_length = MAX_PARTITION_PROPS * sizeof(cl_device_partition_property);
            cl_device_partition_property_var[0] = CL_DEVICE_PARTITION_EQUALLY;
	    //TODO: cl_device_partition_property_var[1] = CL_DEVICE_PARTITION_BY_COUNTS;
            value = &cl_device_partition_property_var;
            break;
        case CL_DEVICE_PARTITION_AFFINITY_DOMAIN:
            SIMPLE_ASSIGN(cl_device_affinity_domain, 0);
            break;
        case CL_DEVICE_PARTITION_TYPE:
            if (!p_parent_device) {
                value_length = 0;  // this is a root device.
            }
            else {  // otherwise, return properties argument of clCreateSubDevices():
	        value = (void *)&p_partition_properties[0];
                value_length = sizeof(cl_device_partition_property) * MAX_PARTITION_PROPS;
            }
            break;
        case CL_DEVICE_REFERENCE_COUNT:
            SIMPLE_ASSIGN(cl_uint, references());
            break;

        default:
            return CL_INVALID_VALUE;
    }

    if (param_value && param_value_size < value_length)
        return CL_INVALID_VALUE;

    if (param_value_size_ret)
        *param_value_size_ret = value_length;

    if (param_value)
        std::memcpy(param_value, value, value_length);

    return CL_SUCCESS;
}

void CPUDevice::setProperties(const cl_device_partition_property *properties)
{
    // store for retrieval by clGetDeviceInfo(...,CL_DEVICE_PARTITION_TYPE,..)

    assert(properties != NULL);
    for (int i = 0; i < MAX_PARTITION_PROPS; i++) {
        p_partition_properties[i] = properties[i];
    }
}


#if !defined(DSPC868X)
#if 0 // /dev/mem is no longer available
unsigned arm_speed()
{
    //return 1000.0;
    const unsigned TETRIS_PLL = 125000000;
    const unsigned pagesize = 0x1000;

    shmem_persistent page;
    page.configure(0x02620000, pagesize);
    char *host_msmc = (char*)page.map(0x02620000, pagesize);
    unsigned SECPLLCTL0 = *(unsigned*)(host_msmc + 0x370);
    unsigned prediv = 1 + (SECPLLCTL0 & 0x3F);
    unsigned mult   = 1 + ((SECPLLCTL0 >> 6) & 0x1FFF);
    unsigned output_div = 1 + ((SECPLLCTL0 >> 19) & 0xF);
    unsigned speed = TETRIS_PLL * mult / prediv / output_div;
    page.unmap(host_msmc, pagesize);

    return speed / 1000000;
}
#endif
#endif

