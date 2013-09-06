#include "JPEGDecoder.h"
#include "JPEGBlitter.h"
#include "JPEGCommon_Gen.h"
#include <utils/threads.h>
#include <utils/Timers.h>
#include <stdio.h>
#undef NDEBUG
#include <assert.h>
#include <hardware/gralloc.h>

#define JPGFILE "/sdcard/1280x720xYUV422H.jpg"

RenderTarget& init_render_target(RenderTarget &target, int width, int height, int pixel_format)
{
    hw_module_t const* module = NULL;
    alloc_device_t *allocdev = NULL;
    struct gralloc_module_t *gralloc_module = NULL;
    buffer_handle_t handle;
    uint32_t fourcc;
    int stride, bpp, err;
    fourcc = pixelFormat2Fourcc(pixel_format);
    bpp = fourcc2LumaBitsPerPixel(fourcc);
    err = hw_get_module(GRALLOC_HARDWARE_MODULE_ID, &module);
    if (err || !module) {
        printf("%s failed to get gralloc module\n", __PRETTY_FUNCTION__);
        assert(false);
    }
    gralloc_module = (struct gralloc_module_t *)module;
    err = gralloc_open(module, &allocdev);
    if (err || !allocdev) {
        printf("%s failed to open alloc device\n", __PRETTY_FUNCTION__);
        assert(false);
    }
    err = allocdev->alloc(allocdev,
            width,
            height,
            pixel_format,
            GRALLOC_USAGE_HW_RENDER,
            &handle,
            &stride);
    if (err) {
        gralloc_close(allocdev);
        printf("%s failed to allocate surface %d, %dx%d, pixelformat %x\n", __PRETTY_FUNCTION__, err,
            width, height, pixel_format);
        assert(false);
    }
    target.type = RenderTarget::ANDROID_GRALLOC;
    target.handle = (int)handle;
    target.width = width;
    target.height = height;
    target.pixel_format = pixel_format;
    target.rect.x = target.rect.y = 0;
    target.rect.width = target.width;
    target.rect.height = target.height;
    target.stride = stride * bpp;
    return target;
}

void deinit_render_target(RenderTarget &target)
{
    buffer_handle_t handle = (buffer_handle_t)target.handle;
    hw_module_t const* module = NULL;
    alloc_device_t *allocdev = NULL;
    struct gralloc_module_t *gralloc_module = NULL;
    int err = hw_get_module(GRALLOC_HARDWARE_MODULE_ID, &module);
    if (err || !module) {
        printf("%s failed to get gralloc module\n", __PRETTY_FUNCTION__);
        return;
    }
    gralloc_module = (struct gralloc_module_t *)module;
    err = gralloc_open(module, &allocdev);
    if (err || !allocdev) {
        printf("%s failed to get gralloc module\n", __PRETTY_FUNCTION__);
        return;
    }
    allocdev->free(allocdev, handle);
    gralloc_close(allocdev);
}

void decode_blit_functionality_test()
{
    JpegDecodeStatus st;
    JpegInfo jpginfo;
    hw_module_t const* module = NULL;
    alloc_device_t *allocdev = NULL;
    struct gralloc_module_t *gralloc_module = NULL;
    buffer_handle_t handle;
    JpegDecoder decoder;
    JpegBlitter blitter;
    blitter.setDecoder(decoder);
    RenderTarget targets[5];
    RenderTarget *dec_target, *blit_nv12_target, *blit_rgba_target, *blit_yuy2_target, *blit_yv12_target;
    FILE* fp = fopen(JPGFILE, "rb");
    assert(fp);
    fseek(fp, 0, SEEK_END);
    jpginfo.bufsize = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    jpginfo.buf = new uint8_t[jpginfo.bufsize];
    fread(jpginfo.buf, 1, jpginfo.bufsize, fp);
    fclose(fp);

    printf("finished loading src file: size %u\n", jpginfo.bufsize);
    st = decoder.parse(jpginfo);
    assert(st == JD_SUCCESS);

    init_render_target(targets[0], jpginfo.image_width, jpginfo.image_height, jpginfo.image_pixel_format);
    init_render_target(targets[1], jpginfo.image_width, jpginfo.image_height, HAL_PIXEL_FORMAT_NV12_TILED_INTEL);
    init_render_target(targets[2], jpginfo.image_width, jpginfo.image_height, HAL_PIXEL_FORMAT_RGBA_8888);
    init_render_target(targets[3], jpginfo.image_width, jpginfo.image_height, HAL_PIXEL_FORMAT_YCbCr_422_I);
    init_render_target(targets[4], jpginfo.image_width, jpginfo.image_height, HAL_PIXEL_FORMAT_YV12);
    dec_target = &targets[0];
    blit_nv12_target = &targets[1];
    blit_rgba_target = &targets[2];
    blit_yuy2_target = &targets[3];
    blit_yv12_target = &targets[4];
    dec_target->rect.x = blit_nv12_target->rect.x = blit_yuy2_target->rect.x = blit_rgba_target->rect.x = blit_yv12_target->rect.x = 0;
    dec_target->rect.y = blit_nv12_target->rect.y = blit_yuy2_target->rect.y = blit_rgba_target->rect.y = blit_yv12_target->rect.y = 0;
    dec_target->rect.width = blit_nv12_target->rect.width = blit_yuy2_target->rect.width = blit_rgba_target->rect.width = blit_yv12_target->rect.width = jpginfo.image_width;
    dec_target->rect.height = blit_nv12_target->rect.height = blit_yuy2_target->rect.height = blit_rgba_target->rect.height = blit_yv12_target->rect.height = jpginfo.image_height;
    RenderTarget* targetlist[5] = {dec_target, blit_nv12_target, blit_rgba_target, blit_yuy2_target, blit_yv12_target };
    //st = decoder.init(jpginfo.image_width, jpginfo.image_height, targetlist, 5);
    st = decoder.init(jpginfo.image_width, jpginfo.image_height, &dec_target, 1);
    assert(st == JD_SUCCESS);

    //jpginfo.render_target = dec_target;
    st = decoder.decode(jpginfo, *dec_target);
    printf("decode returns %d\n", st);
    assert(st == JD_SUCCESS);

    uint8_t *data;
    uint32_t offsets[3];
    uint32_t pitches[3];
    JpegDecoder::MapHandle maphandle = decoder.mapData(*dec_target, (void**) &data, offsets, pitches);
    assert (maphandle.valid);
    FILE* fpdump = fopen("/sdcard/dec_dump.yuv", "wb");
    assert(fpdump);
    // Y
    for (int i = 0; i < dec_target->height; ++i) {
        fwrite(data + offsets[0] + i * pitches[0], 1, dec_target->width, fpdump);
    }
    // U
    for (int i = 0; i < dec_target->height; ++i) {
        fwrite(data + offsets[1] + i * pitches[1], 1, dec_target->width/2, fpdump);
    }
    // V
    for (int i = 0; i < dec_target->height; ++i) {
        fwrite(data + offsets[2] + i * pitches[2], 1, dec_target->width/2, fpdump);
    }
    fclose(fpdump);
    printf("Dumped decoded YUV to /sdcard/dec_dump.yuv\n");
    decoder.unmapData(*dec_target, maphandle);

    st = decoder.blit(*dec_target, *blit_nv12_target);
    assert(st == JD_SUCCESS);

    maphandle = decoder.mapData(*blit_nv12_target, (void**) &data, offsets, pitches);
    assert (maphandle.valid);
    fpdump = fopen("/sdcard/nv12_dump.yuv", "wb");
    assert(fpdump);
    // Y
    for (int i = 0; i < blit_nv12_target->height; ++i) {
        fwrite(data + offsets[0] + i * pitches[0], 1, blit_nv12_target->width, fpdump);
    }
    // UV
    for (int i = 0; i < blit_nv12_target->height/2; ++i) {
        fwrite(data + offsets[1] + i * pitches[1], 1, blit_nv12_target->width, fpdump);
    }
    fclose(fpdump);
    printf("Dumped converted NV12 to /sdcard/nv12_dump.yuv\n");
    decoder.unmapData(*blit_nv12_target, maphandle);

    st = decoder.blit(*dec_target, *blit_yuy2_target);
    assert(st == JD_SUCCESS);
    maphandle = decoder.mapData(*blit_yuy2_target, (void**) &data, offsets, pitches);
    assert (maphandle.valid);
    fpdump = fopen("/sdcard/yuy2_dump.yuv", "wb");
    assert(fpdump);
    // YUYV
    for (int i = 0; i < blit_yuy2_target->height; ++i) {
        fwrite(data + offsets[0] + i * pitches[0], 2, blit_yuy2_target->width, fpdump);
    }
    fclose(fpdump);
    printf("Dumped converted YUY2 to /sdcard/yuy2_dump.yuv\n");
    decoder.unmapData(*blit_yuy2_target, maphandle);

    st = decoder.blit(*dec_target, *blit_rgba_target);
    assert(st == JD_SUCCESS);
    maphandle = decoder.mapData(*blit_rgba_target, (void**) &data, offsets, pitches);
    assert (maphandle.valid);
    fpdump = fopen("/sdcard/rgba_dump.yuv", "wb");
    assert(fpdump);
    // RGBA
    for (int i = 0; i < blit_rgba_target->height; ++i) {
        fwrite(data + offsets[0] + i * pitches[0], 4, blit_rgba_target->width, fpdump);
    }
    fclose(fpdump);
    printf("Dumped converted RGBA to /sdcard/rgba_dump.yuv\n");
    decoder.unmapData(*blit_rgba_target, maphandle);

    st = decoder.blit(*dec_target, *blit_yv12_target);
    assert(st == JD_SUCCESS);
    maphandle = decoder.mapData(*blit_yv12_target, (void**) &data, offsets, pitches);
    assert (maphandle.valid);
    fpdump = fopen("/sdcard/yv12_dump.yuv", "wb");
    assert(fpdump);
    // YV12
    for (int i = 0; i < blit_yv12_target->height; ++i) {
        fwrite(data + offsets[0] + i * pitches[0], 1, blit_yv12_target->width, fpdump);
    }
    for (int i = 0; i < blit_yv12_target->height/2; ++i) {
        fwrite(data + offsets[1] + i * pitches[1], 1, blit_yv12_target->width/2, fpdump);
    }
    for (int i = 0; i < blit_yv12_target->height/2; ++i) {
        fwrite(data + offsets[2] + i * pitches[2], 1, blit_yv12_target->width/2, fpdump);
    }
    fclose(fpdump);
    printf("Dumped converted YV12 to /sdcard/yv12_dump.yuv\n");
    decoder.unmapData(*blit_yv12_target, maphandle);


    decoder.deinit();

    deinit_render_target(*dec_target);
    deinit_render_target(*blit_nv12_target);
    deinit_render_target(*blit_yuy2_target);
    deinit_render_target(*blit_rgba_target);
    deinit_render_target(*blit_yv12_target);
    delete[] jpginfo.buf;

}

enum target_state
{
    TARGET_FREE,
    TARGET_DECODE,
    TARGET_BLIT,
};

struct thread_param
{
    JpegDecoder *decoder;
    RenderTarget *targets;
    RenderTarget *nv12_targets;
    RenderTarget *yuy2_targets;
    RenderTarget *imc3_targets;
    size_t target_count;
    target_state *states;
};

static Mutex state_lock;

void read_new_frame(JpegInfo &jpginfo)
{
    memset(&jpginfo, 0, sizeof(JpegInfo));
    FILE* fp = fopen(JPGFILE, "rb");
    assert(fp);
    fseek(fp, 0, SEEK_END);
    jpginfo.bufsize = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    jpginfo.buf = new uint8_t[jpginfo.bufsize];
    fread(jpginfo.buf, 1, jpginfo.bufsize, fp);
    fclose(fp);
}

static bool exit_thread = false;

#define VPP_DECODE_BATCH

void* decode_frame_threadproc(void* data)
{
    thread_param *param = (thread_param*) data;
    JpegInfo *jpginfos = new JpegInfo[param->target_count];
    int surface_id = 0;
    int blit_surface_id = (surface_id + param->target_count - 1) % param->target_count;
    while(!exit_thread) {
        printf("%s blit %d and decode %d\n", __FUNCTION__, blit_surface_id, surface_id);
        RenderTarget& cur_target = param->targets[surface_id];
#ifdef VPP_DECODE_BATCH
        RenderTarget& blit_target = param->targets[blit_surface_id];
        RenderTarget& blit_nv12_target = param->nv12_targets[blit_surface_id];
        RenderTarget& blit_yuy2_target = param->yuy2_targets[blit_surface_id];
        if (param->states[blit_surface_id] == TARGET_BLIT) {
            printf("%s blit with surface %d\n", __FUNCTION__, blit_surface_id);
            nsecs_t t1 = systemTime();
            if (param->decoder->busy(blit_target)) {
                param->decoder->sync(blit_target);
                nsecs_t t2 = systemTime();
                printf("%s wait surface %d decode took %f ms\n", __FUNCTION__, blit_surface_id, ns2us(t2 - t1)/1000.0);
                param->states[blit_surface_id] = TARGET_FREE;
            }
            t1 = systemTime();
            param->decoder->blit(blit_target, blit_nv12_target);
            nsecs_t t2 = systemTime();
            param->decoder->blit(blit_target, blit_yuy2_target);
            nsecs_t t3 = systemTime();
            printf("%s blit %d NV12 took %f ms, YUY2 took %f ms\n",
                __FUNCTION__,
                blit_surface_id, ns2us(t2 - t1)/1000.0,
                ns2us(t3 - t2)/1000.0);
            param->states[blit_surface_id] = TARGET_FREE;
        }
#endif
        if (param->states[surface_id] != TARGET_FREE) {
            printf("%s wait surface %d blit finish\n", __FUNCTION__, surface_id);
            nsecs_t t1 = systemTime();
            while (param->states[surface_id] != TARGET_FREE) {
                usleep(1000);
            }
            nsecs_t t2 = systemTime();
            printf("%s wait surface %d for decode/blit finish took %f ms\n", __FUNCTION__, surface_id, ns2us(t2 - t1)/1000.0);
        }
        JpegInfo &jpginfo = jpginfos[surface_id];
        read_new_frame(jpginfo);
        nsecs_t t3 = systemTime();
        param->decoder->parse(jpginfo);
        nsecs_t t4 = systemTime();
        printf("%s parse surface %d took %f ms\n", __FUNCTION__, surface_id, ns2us(t4 - t3)/1000.0);
        param->states[surface_id] = TARGET_DECODE;
        param->decoder->decode(jpginfo, cur_target);
        nsecs_t t5 = systemTime();
        printf("%s decode surface %d took %f ms\n", __FUNCTION__, surface_id, ns2us(t5 - t4)/1000.0);
        param->states[surface_id] = TARGET_BLIT;
        surface_id  = (surface_id + 1) % param->target_count;
        blit_surface_id  = (blit_surface_id + 1) % param->target_count;
    }
    delete[] jpginfos;
    return NULL;
}

void* blit_frame_threadproc(void* data)
{
    thread_param *param = (thread_param*) data;
    int surface_id = 0;
    while(!exit_thread) {
        printf("%s blit %d->%d\n", __FUNCTION__, surface_id, surface_id);
        RenderTarget& dec_target = param->targets[surface_id];
        RenderTarget& blit_target = param->nv12_targets[surface_id];
        if (param->states[surface_id] != TARGET_BLIT) {
            printf("%s wait surface %d decoding finish\n", __FUNCTION__, surface_id);
            nsecs_t t1 = systemTime();
            while (param->states[surface_id] != TARGET_BLIT) {
                usleep(100);
            }
            nsecs_t t2 = systemTime();
            printf("%s wait surface %d for decode finish took %f ms\n", __FUNCTION__, surface_id, ns2us(t2 - t1)/1000.0);
        }
        nsecs_t t3 = systemTime();
        param->decoder->blit(dec_target, blit_target);
        nsecs_t t4 = systemTime();
        printf("%s blit surface %d took %f ms\n", __FUNCTION__, surface_id, ns2us(t4 - t3)/1000.0);
        param->states[surface_id] = TARGET_FREE;
        surface_id  = (surface_id + 1) % param->target_count;
    }
    return NULL;
}

void parallel_decode_blit_test()
{
    RenderTarget **all_targets = new RenderTarget*[12];
    RenderTarget dec_targets[12];
    RenderTarget nv12_targets[12];
    RenderTarget yuy2_targets[12];
    RenderTarget imc3_targets[12];
    JpegInfo jpginfos[12];
    target_state states[12];
    for (int i = 0; i < 12; ++i) {
        init_render_target(dec_targets[i], 1280, 720, fourcc2PixelFormat(VA_FOURCC_422H)); // 422H
        init_render_target(nv12_targets[i], 1280, 720, fourcc2PixelFormat(VA_FOURCC_NV12)); // NV12 for video encode
        init_render_target(yuy2_targets[i], 1280, 720, fourcc2PixelFormat(VA_FOURCC_YUY2)); // YUY2 for overlay
        //init_render_target(imc3_targets[i], 1280, 720, HAL_PIXEL_FORMAT_IMC3); // IMC3 for libjpeg encode
        jpginfos[i].buf = new uint8_t[2 * 1024 * 1024];
        all_targets[i] = &dec_targets[i];
        //all_targets[i + 12] = &nv12_targets[i];
        //all_targets[i + 24] = &yuy2_targets[i];
        //all_targets[i + 36] = &imc3_targets[i];
        states[i] = TARGET_FREE;
    }

    exit_thread = false;

    pthread_attr_t dec_attr, blit_attr;
    pthread_attr_init(&dec_attr);
    pthread_attr_init(&blit_attr);
    pthread_attr_setdetachstate(&dec_attr, PTHREAD_CREATE_JOINABLE);
    pthread_attr_setdetachstate(&blit_attr, PTHREAD_CREATE_JOINABLE);
    pthread_t dec_thread, blit_thread;
    thread_param param;
    param.nv12_targets = nv12_targets;
    param.yuy2_targets = yuy2_targets;
    param.imc3_targets = imc3_targets;
    param.targets = dec_targets;
    param.target_count = 12;
    param.decoder = new JpegDecoder();
    //param.decoder->init(1280, 720, all_targets, 36);
    param.decoder->init(1280, 720, all_targets, 12);
    param.states = states;
    pthread_create(&dec_thread, &dec_attr, decode_frame_threadproc, (void*)&param);
#ifndef VPP_DECODE_BATCH
    pthread_create(&blit_thread, &blit_attr, blit_frame_threadproc, (void*)&param);
#endif
    pthread_attr_destroy(&blit_attr);
    pthread_attr_destroy(&dec_attr);

    // test for 1 minute
    usleep(60 * 1000 * 1000);
    exit_thread = true;
    void *dummy;
    pthread_join(dec_thread, &dummy);
#ifndef VPP_DECODE_BATCH
    pthread_join(blit_thread, &dummy);
#endif

    for (int i = 0; i < 12; ++i) {
        delete[] jpginfos[i].buf;
        deinit_render_target(dec_targets[i]);
        deinit_render_target(nv12_targets[i]);
        deinit_render_target(yuy2_targets[i]);
        //deinit_render_target(imc3_targets[i]);
    }
    delete[] all_targets;
}

int main(int argc, char ** argv)
{
    //decode_blit_functionality_test();
    parallel_decode_blit_test();
    return 0;
}
