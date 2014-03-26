/* INTEL CONFIDENTIAL
* Copyright (c) 2013 Intel Corporation.  All rights reserved.
*
* The source code contained or described herein and all documents
* related to the source code ("Material") are owned by Intel
* Corporation or its suppliers or licensors.  Title to the
* Material remains with Intel Corporation or its suppliers and
* licensors.  The Material contains trade secrets and proprietary
* and confidential information of Intel or its suppliers and
* licensors. The Material is protected by worldwide copyright and
* trade secret laws and treaty provisions.  No part of the Material
* may be used, copied, reproduced, modified, published, uploaded,
* posted, transmitted, distributed, or disclosed in any way without
* Intel's prior express written permission.
*
* No license under any patent, copyright, trade secret or other
* intellectual property right is granted to or conferred upon you
* by disclosure or delivery of the Materials, either expressly, by
* implication, inducement, estoppel or otherwise. Any license
* under such intellectual property rights must be express and
* approved by Intel in writing.
*
*/
#include <unistd.h>
#include <stdio.h>
#include <assert.h>

#include "vbp_thread.h"
#include "vbp_loader.h"

/* consider a qual core with hyper thread */
#define MAX_AUTO_THREADS 8

#define THREADING_SCHEME_BUNDLE

typedef long long int nsecs_t;

static nsecs_t systemTime()
{
    struct timeval t;
    gettimeofday(&t, NULL);
    return 1000000 * t.tv_sec + t.tv_usec;
}


typedef struct PerThreadContext {
    pthread_t thread;

    int32_t index;                  // thread index referenced by thread itself when needed.
    int32_t thread_init;
    struct ThreadContext* parent;

    pthread_cond_t input_cond;      // Used to wait for a new packet from the main thread.
    pthread_cond_t progress_cond;   // Used by child threads to wait for progress to change.
    pthread_cond_t output_cond;     // Used by the main thread to wait for frames to finish.

    pthread_mutex_t mutex;          // Mutex used to protect the contents of the PerThreadContext.
    pthread_mutex_t progress_mutex; // Mutex used to protect frame progress values and progress_cond.

    vbp_context* vbpctx;
    viddec_pm_cxt_t* pmctx;         // Working parser context
    viddec_pm_cxt_t* input_pmctx;   // Input parser context
    void* codec_data;               // Points to specific codec data that holds output, all threads share
                                    // one instance
    uint32_t start_item;            // start of parsing item num for bundle parsing

    enum {
        STATE_INPUT_WAIT,
        STATE_WORKING,
        STATE_EXIT
    } state;

} PerThreadContext;

typedef struct ThreadContext {
    PerThreadContext* threads[MAX_AUTO_THREADS]; // The contexts for each thread.
    PerThreadContext* prev_thread;               // The last thread submit_packet() was called on.
    int delaying;                       // Set for the first N packets, where N is the number of threads.
                                        // While it is set, vbp_thread_parse_syntax won't return any results

    uint32_t next_finished;             // The next thread count to return output from.
    uint32_t next_parsing;              // The next thread count to submit input packet to.

    uint32_t active_thread_count;       // num of thread need to be warmed up

    sem_t finish_sem;                   // semaphore of finish work to synchronize working thread and main thread
    uint32_t start_item_to_parse;
    uint32_t last_item_to_parse;

} ThreadContext;


int32_t get_cpu_count()
{
    int32_t cpu_num;
#if defined(_SC_NPROC_ONLN)
    cpu_num = sysconf(_SC_NPROC_ONLN);
#elif defined(_SC_NPROCESSORS_ONLN)
    cpu_num = sysconf(_SC_NPROCESSORS_ONLN);
#endif
    return cpu_num;
}


void set_thread_affinity_mask(cpu_set_t mask)
{
    int err, syscallres;
    pid_t pid = gettid();
    syscallres = syscall(__NR_sched_setaffinity, pid, sizeof(mask), &mask);
    if (syscallres)
    {
        ETRACE("Error in the syscall setaffinity.");
    }
}


static void vbp_update_parser_for_item(viddec_pm_cxt_t *cxt,
                                      viddec_pm_cxt_t *src_cxt,
                                      uint32 item)
{

    /* set up bitstream buffer */
    cxt->getbits.list = src_cxt->getbits.list;

    /* setup buffer pointer */
    cxt->getbits.bstrm_buf.buf = src_cxt->getbits.bstrm_buf.buf;


    /* setup bitstream parser */
    cxt->getbits.bstrm_buf.buf_index = src_cxt->list.data[item].stpos;
    cxt->getbits.bstrm_buf.buf_st = src_cxt->list.data[item].stpos;
    cxt->getbits.bstrm_buf.buf_end = src_cxt->list.data[item].edpos;

    /* It is possible to end up with buf_offset not equal zero. */
    cxt->getbits.bstrm_buf.buf_bitoff = 0;
    cxt->getbits.au_pos = 0;
    cxt->getbits.list_off = 0;
    cxt->getbits.phase = 0;
    cxt->getbits.emulation_byte_counter = 0;

    cxt->list.start_offset = src_cxt->list.data[item].stpos;
    cxt->list.end_offset = src_cxt->list.data[item].edpos;
    cxt->list.total_bytes = src_cxt->list.data[item].edpos - src_cxt->list.data[item].stpos;

}



static void* parser_worker_thread(void* arg)
{
    PerThreadContext* p = arg;
    ThreadContext* t_cxt = p->parent;
    vbp_context* vbpctx = p->vbpctx;
    viddec_pm_cxt_t* pm_cxt = p->pmctx;
    viddec_parser_ops_t* ops = vbpctx->parser_ops;


// probably not to make each parsing thread have affinity to a cpu core
// having cpus fully occupied will even lead to low performance
// current experimental solution: just make main thread have affinity
#if 0
    cpu_set_t mask;
    CPU_ZERO(&mask);
    CPU_SET(p->index, &mask); // cpu affinity is set to same num as thread index
    set_thread_affinity_mask(mask);
#endif

    pthread_mutex_lock(&p->mutex);

    nsecs_t t0;
    while (1) {
        while (p->state == STATE_INPUT_WAIT) {
            pthread_cond_wait(&p->input_cond, &p->mutex);
        }

        if (p->state == STATE_WORKING) {
            //now we get input data, call actual parse.
            //t0 = systemTime();
            sleep(0);
            ops->parse_syntax_threading((void *)p->pmctx, p->codec_data, p->index);

            pthread_mutex_lock(&p->progress_mutex);
            p->state = STATE_INPUT_WAIT;

            pthread_cond_broadcast(&p->progress_cond);
            pthread_cond_signal(&p->output_cond);
            pthread_mutex_unlock(&p->progress_mutex);
        } else if (p->state == STATE_EXIT) {
            break;
        }
    }
    pthread_mutex_unlock(&p->mutex);
    pthread_exit(NULL);
    return NULL;
}

static void* parser_worker_thread_bundle(void* arg)
{
    PerThreadContext* p = arg;
    ThreadContext* t_cxt = p->parent;
    vbp_context* vbpctx = p->vbpctx;
    viddec_parser_ops_t* ops = vbpctx->parser_ops;

// probably not to make each parsing thread have affinity to a cpu core
// having cpus fully occupied will even lead to low performance
// current experimental solution: just make main thread have affinity
#if 1
    cpu_set_t mask;
    CPU_ZERO(&mask);
    CPU_SET(p->index, &mask); // cpu affinity is set to same num as thread index
    set_thread_affinity_mask(mask);
#endif

    pthread_mutex_lock(&p->mutex);

    nsecs_t t0;
    while (1) {
        while (p->state == STATE_INPUT_WAIT) {
            pthread_cond_wait(&p->input_cond, &p->mutex);
        }

        if (p->state == STATE_WORKING) {
            uint32_t working_item = p->start_item;  // start point
            uint32_t slice_index = 0 + p->index;     // start point

            while (working_item <= t_cxt->last_item_to_parse) {
                vbp_update_parser_for_item(p->pmctx, p->input_pmctx, working_item);
                ops->parse_syntax_threading((void *)p->pmctx, p->codec_data, slice_index);

                working_item += t_cxt->active_thread_count;
                slice_index += t_cxt->active_thread_count;
            }

            pthread_mutex_lock(&p->progress_mutex);
            p->state = STATE_INPUT_WAIT;

            pthread_cond_broadcast(&p->progress_cond);
            pthread_mutex_unlock(&p->progress_mutex);
        } else if (p->state == STATE_EXIT) {
            break;
        }
    }
    pthread_mutex_unlock(&p->mutex);
    pthread_exit(NULL);
    return NULL;
}


uint32_t update_context_from_input(viddec_pm_cxt_t* dest,
                                   viddec_pm_cxt_t* source)
{
    if ((dest == NULL) || (source == NULL) || (dest == source)) {
        ETRACE("%s error", __func__);
        return 1;
    }
    /* set up bitstream buffer */
    dest->getbits.list = source->getbits.list;

    /* buffer pointer */
    dest->getbits.bstrm_buf.buf = source->getbits.bstrm_buf.buf;

    /* bitstream parser */
    dest->getbits.bstrm_buf.buf_index = source->getbits.bstrm_buf.buf_index;
    dest->getbits.bstrm_buf.buf_st = source->getbits.bstrm_buf.buf_st;
    dest->getbits.bstrm_buf.buf_end = source->getbits.bstrm_buf.buf_end;

    /* It is possible to end up with buf_offset not equal zero. */
    dest->getbits.bstrm_buf.buf_bitoff = 0;
    dest->getbits.au_pos = 0;
    dest->getbits.list_off = 0;
    dest->getbits.phase = 0;
    dest->getbits.emulation_byte_counter = 0;

    dest->list.start_offset = source->list.start_offset;
    dest->list.end_offset = source->list.end_offset;
    dest->list.total_bytes = source->list.total_bytes;
    return 0;
}

uint32_t update_context_to_output(viddec_pm_cxt_t* dest,
                                   viddec_pm_cxt_t* source)
{
    if ((dest == NULL) || (source == NULL) || (dest == source)) {
        ETRACE("%s error", __func__);
        return 1;
    }

    /* bitstream parser */
    dest->getbits.bstrm_buf.buf_index = source->getbits.bstrm_buf.buf_index;
    dest->getbits.bstrm_buf.buf_st = source->getbits.bstrm_buf.buf_st;
    dest->getbits.bstrm_buf.buf_end = source->getbits.bstrm_buf.buf_end;

    /* It is possible to end up with buf_offset not equal zero. */
    dest->getbits.bstrm_buf.buf_bitoff = source->getbits.bstrm_buf.buf_bitoff;
    dest->getbits.au_pos = source->getbits.au_pos;
    dest->getbits.list_off = source->getbits.list_off;
    dest->getbits.phase = source->getbits.phase;
    dest->getbits.emulation_byte_counter = source->getbits.emulation_byte_counter;
    dest->getbits.is_emul_reqd = source->getbits.is_emul_reqd;

    dest->list.start_offset = source->list.start_offset;
    dest->list.end_offset = source->list.end_offset;
    dest->list.total_bytes = source->list.total_bytes;

    return 0;
}



uint32_t feed_thread_input(PerThreadContext* p, void* parent)
{
    ThreadContext* t_context = p->parent;
    viddec_pm_cxt_t* pm_cxt = (viddec_pm_cxt_t*) parent;

    //nsecs_t t0 = systemTime();
    if (pm_cxt->getbits.bstrm_buf.buf == NULL) {
        return 1;
    }

    pthread_mutex_lock(&p->mutex);

    if (p->state == STATE_WORKING) {
        pthread_mutex_lock(&p->progress_mutex);
        while (p->state == STATE_WORKING) {
            pthread_cond_wait(&p->progress_cond, &p->progress_mutex);
        }
        pthread_mutex_unlock(&p->progress_mutex);
    }

    /* Now update the input to the working thread*/
    update_context_from_input(p->pmctx, pm_cxt);
    p->codec_data = (void*)&(pm_cxt->codec_data[0]);

    p->state = STATE_WORKING;
    t_context->next_parsing++;

    //t0 = systemTime();
    pthread_cond_signal(&p->input_cond);
    pthread_mutex_unlock(&p->mutex);

    return 0;
}

void vbp_thread_init(vbp_context* pcontext)
{
    int i;
    ThreadContext* t_context = NULL;
    int32_t thread_count = pcontext->thread_count;
    int32_t err = 0;

#ifdef THREADING_SCHEME_BUNDLE
    ITRACE("%s, threading_parse_scheme set to SCHEME_BUNDLE", __func__);
    pcontext->threading_parse_scheme = SCHEME_BUNDLE;
#else
    ITRACE("%s, threading_parse_scheme set to SCHEME_SEQUENTIAL", __func__);
    pcontext->threading_parse_scheme = SCHEME_SEQUENTIAL;
#endif

    if (thread_count == 0) {
        int32_t cpu_num = get_cpu_count();
        if (cpu_num > 1) {
            if (pcontext->threading_parse_scheme == SCHEME_BUNDLE) {
                thread_count = pcontext->thread_count = cpu_num - 1;
            } else {
                thread_count = pcontext->thread_count = cpu_num - 1;
            }
        }
        else {
            thread_count = pcontext->thread_count = 1;
        }
    }

    pcontext->thread_opaque = t_context =
              (ThreadContext*)malloc(sizeof(ThreadContext));
    if (t_context != NULL) {
        t_context->active_thread_count = thread_count; //default active count

        t_context->delaying = 1;
        t_context->next_parsing = t_context->next_finished = 0;

        ITRACE("%s, creating %d parsing thread.", __func__, thread_count);
        for (i = 0; i < thread_count; i++) {
            t_context->threads[i] = (PerThreadContext*)malloc(sizeof(PerThreadContext));
            assert(t_context->threads[i] != NULL);
            PerThreadContext* p = t_context->threads[i];

            if (p != NULL) {
                p->index = i;
                p->parent = t_context;
                p->vbpctx = pcontext;
                p->pmctx = vbp_malloc(viddec_pm_cxt_t, 1);
                viddec_pm_utils_bstream_init(&(p->pmctx->getbits), NULL, 0);

                pthread_mutex_init(&p->mutex, NULL);
                pthread_mutex_init(&p->progress_mutex, NULL);
                pthread_cond_init(&p->input_cond, NULL);
                pthread_cond_init(&p->progress_cond, NULL);
                pthread_cond_init(&p->output_cond, NULL);

                p->state = STATE_INPUT_WAIT;

                if(pcontext->threading_parse_scheme == SCHEME_SEQUENTIAL) {
                    err = pthread_create(&p->thread, NULL, parser_worker_thread, p);
                } else {
                    err = pthread_create(&p->thread, NULL, parser_worker_thread_bundle, p);
                }

                p->thread_init = !err;
            }
        } 
    }
#if 1
    ITRACE("%s, set_thread_affinity_mask", __func__);
    cpu_set_t mask;
    CPU_ZERO(&mask);
    CPU_SET(3, &mask); // 0~thread_count-1 cpus was set to each sub thread,
                       // last cpu is set to main thread
    set_thread_affinity_mask(mask);
#endif
}


void vbp_thread_free(vbp_context* pcontext)
{
    ITRACE("%s", __func__);
    ThreadContext* t_context = pcontext->thread_opaque;
    int i;
    int thread_count = pcontext->thread_count;

    for (i = 0; i < thread_count; i++) {
        PerThreadContext *p = t_context->threads[i];

        pthread_mutex_lock(&p->mutex);
        p->state = STATE_EXIT;
        pthread_cond_signal(&p->input_cond);
        pthread_mutex_unlock(&p->mutex);

        if (p->thread_init) {
            pthread_join(p->thread, NULL);
        }
        p->thread_init = 0;
    }

    for (i = 0; i < thread_count; i++) {
        PerThreadContext *p = t_context->threads[i];

        pthread_mutex_destroy(&p->mutex);
        pthread_mutex_destroy(&p->progress_mutex);
        pthread_cond_destroy(&p->input_cond);
        pthread_cond_destroy(&p->progress_cond);
        pthread_cond_destroy(&p->output_cond);

        if (p->pmctx != NULL) {
            free(p->pmctx);
        }

        free(p);
        p = NULL;
    }

    free(t_context);
}

/*
 * Entry function of multi-thread parsing
 *
 * parent - A viddec_pm_cxt_t type parser management context,
 *          which contains input stream.
 * ctxt   - Codec specific parser context, actually codec_data[] in
 *          viddec_pm_cxt_t, Used for storing parsed output
 * return - 0 indicates no output is gotten, just warm up the threads
 *          1 indicates there is output
 *
 * see viddec_parser_ops.h
 *     uint32_t (*fn_parse_syntax) (void *parent, void *ctxt);
 */
uint32_t vbp_thread_parse_syntax(void* parent,
                                 void* ctxt,
                                 vbp_context* pcontext)
{
    ThreadContext* t_context = pcontext->thread_opaque;
    uint32_t finished = t_context->next_finished;

    if ((parent == NULL) || (ctxt == NULL)) {
        return 0;
    }

    PerThreadContext* p;

    nsecs_t t0,t1;
    //t0 = t1 = systemTime();

    /* Submit an input packet to the next parser thread*/
    p = t_context->threads[t_context->next_parsing];
    feed_thread_input(p, parent);

    //p->state = STATE_WORKING;
    //t_context->next_parsing++;

    //t0 = systemTime();
    //pthread_cond_signal(&p->input_cond);

    //t0 = systemTime();

    if ((t_context->delaying == 1) &&
        (t_context->next_parsing > (t_context->active_thread_count - 1))) {
        t_context->delaying = 0;
    }

    /* If we are still in early stage that warming up each thread, indicate we got no output*/
    if (t_context->delaying == 1) {
        return 0;
    }

    /* return available parsed frame from the oldest thread
     * notice that we start getting output from thread[0] after just submitting input
     * to thread[active_count-1]
     * */
    p = t_context->threads[finished++];

    if (p->state != STATE_INPUT_WAIT) {
        pthread_mutex_lock(&p->progress_mutex);
        while (p->state != STATE_INPUT_WAIT) {
            pthread_cond_wait(&p->output_cond, &p->progress_mutex);
        }
        pthread_mutex_unlock(&p->progress_mutex);
    }


    if (finished > (t_context->active_thread_count - 1)) {
        finished = 0;
    }

    if (t_context->next_parsing >= t_context->active_thread_count) {
        t_context->next_parsing = 0;
    }

    t_context->next_finished = finished;

    update_context_to_output((viddec_pm_cxt_t*) parent, p->pmctx);

    return 1;
}


/*
 * Entry function of multi-thread parsing
 *
 * parent - A viddec_pm_cxt_t type parser management context,
 *          which contains input stream.
 * ctxt   - Codec specific parser context, actually codec_data[] in
 *          viddec_pm_cxt_t, Used for storing parsed output
 * start_item - num of start item passed to trigger multithread parsing
 *
 */
uint32_t vbp_thread_parse_syntax_bundle(void* parent,
                                   void* ctxt,
                                   vbp_context* pcontext,
                                   uint32_t start_item)
{
    ThreadContext* t_context = pcontext->thread_opaque;
    if ((parent == NULL) || (ctxt == NULL)) {
        return 0;
    }

    PerThreadContext* p = NULL;
    viddec_pm_cxt_t* pm_cxt = (viddec_pm_cxt_t*) parent;
    t_context->start_item_to_parse = start_item;
    t_context->last_item_to_parse = pm_cxt->list.num_items - 1;

    sem_init(&(t_context->finish_sem),0,0);

    uint32_t i;
    for (i = 0; i < t_context->active_thread_count; i++) {
        p = t_context->threads[i];
        p->start_item = start_item + i;

        if (p->state == STATE_WORKING) {
            pthread_mutex_lock(&p->progress_mutex);
            while (p->state == STATE_WORKING) {
                pthread_cond_wait(&p->progress_cond, &p->progress_mutex);
            }
            pthread_mutex_unlock(&p->progress_mutex);
        }

        p->codec_data = (void*)&(pm_cxt->codec_data[0]);
        p->input_pmctx = pm_cxt;

        p->state = STATE_WORKING;

        pthread_cond_signal(&p->input_cond);
        pthread_mutex_unlock(&p->mutex);

    }
    return 1;
}


/*
 * set active threads num since not all threads need to be warmed up
 * when a frame has fewer slice num than threads we created.
 *
 * active_count  - threads num to be activated.
 */
uint32_t vbp_thread_set_active(vbp_context* pcontext,
                              uint32_t active_count)
{
    ThreadContext* t_context = pcontext->thread_opaque;

    if (t_context != NULL) {
        if (active_count < pcontext->thread_count) {
            t_context->active_thread_count = active_count;
        } else { //reset to the default
            t_context->active_thread_count = pcontext->thread_count;
        }

        //reset to the default
        t_context->delaying = 1;
        t_context->next_parsing = t_context->next_finished = 0;
    }
    return 0;
}

uint32_t vbp_thread_get_active(vbp_context* pcontext)
{
    ThreadContext* t_context = pcontext->thread_opaque;

    if (t_context != NULL) {
        return t_context->active_thread_count;
    }
    return 0;
}


