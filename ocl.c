#define CL_TARGET_OPENCL_VERSION 220
#include <CL/cl.h>
#include <stdio.h>
#include <stdint.h>
#include "cubiomes/generator.h"

#define BUFFER_COUNT 3
#define LAYER_COUNT 2

enum Kernels {
    MAP_ISLAND_KERNEL,
    MAP_ZOOM_ISLAND_KERNEL,

    KERNEL_COUNT
};

struct CLLayer {
    int64_t startSeed;
    int64_t startSalt;
};

struct CLLayerStack {
    struct CLLayer layers[LAYER_COUNT];
};

struct OCLGeneratorContext {
    cl_context context;
    cl_command_queue queue;
    cl_mem layersBuffer;
    cl_mem buffers[BUFFER_COUNT];
    cl_program program;

    cl_kernel kernels[KERNEL_COUNT];

    struct CLLayerStack stack;
};

void set_layer_seed(struct CLLayer* layer, int64_t salt, int64_t worldSeed) {
    salt = getLayerSeed(salt);
    int64_t st = worldSeed;
    st = mcStepSeed(st, salt);
    st = mcStepSeed(st, salt);
    st = mcStepSeed(st, salt);

    layer->startSalt = st;
    layer->startSeed = mcStepSeed(st, 0);
}

cl_int set_world_seed(struct OCLGeneratorContext* context, int64_t seed, cl_event* event) {
    set_layer_seed(&context->stack.layers[L_ISLAND_4096], 1, seed);
    set_layer_seed(&context->stack.layers[L_ZOOM_2048], 2000, seed);

    return clEnqueueWriteBuffer(
        context->queue, context->layersBuffer, CL_FALSE, 
        0, sizeof(struct CLLayerStack), context->stack.layers, 
        0, NULL, event
    );
}

const static cl_int ZERO = 0;
cl_int generate_layer(struct OCLGeneratorContext* context, int layer, size_t width, size_t height, const cl_event* wait_for, void* res, cl_event* event) {
    size_t dims[2] = {width, height};
    cl_int2 dims1 = {{(cl_int) width, (cl_int) height}};
    cl_int2 pos = {{0, 0}};
    cl_event fill_event;
    cl_int err;

    size_t buffer_size = width*height*sizeof(cl_int);

    err = clSetKernelArg(context->kernels[MAP_ISLAND_KERNEL], 0, sizeof(cl_mem), &context->layersBuffer);
    if (err < 0) {
        return err;
    }
    err = clSetKernelArg(context->kernels[MAP_ISLAND_KERNEL], 1, sizeof(cl_int2), &pos);
    if (err < 0) {
        return err;
    }
    err = clSetKernelArg(context->kernels[MAP_ISLAND_KERNEL], 2, sizeof(cl_int2), &dims1);
    if (err < 0) {
        return err;
    }
    err = clSetKernelArg(context->kernels[MAP_ISLAND_KERNEL], 3, sizeof(cl_mem), &context->buffers[0]);
    if (err < 0) {
        return err;
    }
    err = clSetKernelArg(context->kernels[MAP_ISLAND_KERNEL], 4, sizeof(cl_mem), &context->buffers[1]);
    if (err < 0) {
        return err;
    }

    cl_event kernel_finished;
    err = clEnqueueNDRangeKernel(
        context->queue, context->kernels[MAP_ISLAND_KERNEL], 
        2, NULL, dims, NULL, 
        1, &fill_event, &kernel_finished
    );
    if (err < 0) {
        return err;
    }

    err = clEnqueueReadBuffer(
        context->queue, context->buffers[1], CL_FALSE, 
        0, buffer_size, res,
        1, &kernel_finished, event
    );
    if (err < 0) {
        return err;
    }
    return CL_SUCCESS;
}

cl_int init_generator_context(struct OCLGeneratorContext* context, size_t width, size_t height) {
    cl_int err;
    cl_platform_id platform;
    if ((err = clGetPlatformIDs(1, &platform, NULL)) < 0) {
        return err;
    }
    cl_device_id device;
    if ((err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, NULL)) < 0) {
        return err;
    }
    context->context = clCreateContext(NULL, 1, &device, NULL, NULL, &err);
    if (err < 0) {
        return err;
    }
    context->queue = clCreateCommandQueueWithProperties(context->context, device, (cl_queue_properties*) NULL, &err);
    if (err < 0) {
        clReleaseContext(context->context);
        return err;
    }
    context->layersBuffer = clCreateBuffer(context->context, CL_MEM_READ_ONLY, sizeof(struct CLLayerStack), NULL, &err);
    if (err < 0) {
        clReleaseContext(context->context);
        return err;
    }
    for (int i = 0; i < BUFFER_COUNT; ++i) {
        context->buffers[i] = clCreateBuffer(context->context, CL_MEM_READ_WRITE, width * height * sizeof(cl_int), NULL, &err);
        if (err < 0) {
            clReleaseContext(context->context);
            return err;
        }
    }
    FILE* program = fopen("genArea.cl", "r");
    if (!program) {
        clReleaseContext(context->context);
        return -1;
    }
    // 256 KiB should be enough?
    // #TODO: handle file loading properly.
    const int SIZE = 1024 * 256;
    char* buffer = (char*) malloc(SIZE); 
    if (!buffer) {
        fclose(program);
        clReleaseContext(context->context);
        return -1;
    }
    int length = fread(buffer, 1, SIZE, program);
    if (length < 0) {
        fclose(program);
        free(buffer);
        clReleaseContext(context->context);
        return -1;
    }
    fclose(program);
    context->program = clCreateProgramWithSource(context->context, 1, (const char**)&buffer, (size_t*)NULL, &err);
    free(buffer);
    if (err < 0) {
        clReleaseContext(context->context);
        return err;
    }
    err = clBuildProgram(context->program, 1, &device, NULL, NULL, NULL);
    if (err < 0) {
        size_t size = 256*1024*sizeof(char);
        char *buildlog = malloc(size); 
        int err2 = clGetProgramBuildInfo(context->program, device, CL_PROGRAM_BUILD_LOG, size, buildlog, NULL); 
        printf("%d\n%s\n", err2, buildlog);
        clReleaseContext(context->context);
        return err;
    }

    const char* kernel_names[KERNEL_COUNT] = {
        "mapIsland", 
        "mapZoomIsland"
    };
    for (int i=0; i<KERNEL_COUNT; ++i) {
        context->kernels[i] = clCreateKernel(context->program, kernel_names[i], &err);
        if (err < 0) {
            clReleaseContext(context->context);
            return err;
        }
    }

    return CL_SUCCESS;
}

void release_generator_context(struct OCLGeneratorContext* context) {
    clReleaseContext(context->context);
}

int main() {
    struct OCLGeneratorContext context;
    cl_int err;
    if ((err = init_generator_context(&context, 256, 256)) < 0) {
        printf("Error initializing context: %d\n", err);
        return -1;
    }
    cl_int* bufferA = (cl_int*) malloc(256 * 256 * sizeof(cl_int));
    int* bufferB = (int*) malloc(256 * 256 * sizeof(int));

    struct LayerStack stack;
    initBiomes();
    setupGenerator(&stack, MC_1_16);
    int acc = 0;
    for (int i=0; i<10000; ++i) {
        cl_event set_seed;
        if ((err = set_world_seed(&context, i, &set_seed)) < 0) {
            printf("Error setting world seed: %d\n", err);
            return -1;
        }
        cl_event event;
        err = generate_layer(&context, L_ISLAND_4096, 256, 256, &set_seed, bufferA, &event);
        clWaitForEvents(1, &event);
        acc += bufferA[0];
        if (err < 0) {
            printf("Error generating world: %d\n", err);
            return -1;
        }

        /*
        applySeed(&stack, i);
        genArea(&stack.layers[L_ISLAND_4096], bufferB, 0, 0, 256, 256);
        */
        /*
        int error_count = 0;
        for (int j=0; j<256*256; ++j) {
            if (bufferA[j] != bufferB[j]) {
                printf("seed=%d: NOT EQUAL at %d: %d != %d\n", i, j, bufferA[j], bufferB[j]);
                error_count += 1;
                if (error_count >= 10) {
                    return -1;
                }
            }
        }
        */
    }
    release_generator_context(&context);
}