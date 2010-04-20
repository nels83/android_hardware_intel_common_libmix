/* 
 INTEL CONFIDENTIAL
 Copyright 2009 Intel Corporation All Rights Reserved. 
 The source code contained or described herein and all documents related to the source code ("Material") are owned by Intel Corporation or its suppliers or licensors. Title to the Material remains with Intel Corporation or its suppliers and licensors. The Material contains trade secrets and proprietary and confidential information of Intel or its suppliers and licensors. The Material is protected by worldwide copyright and trade secret laws and treaty provisions. No part of the Material may be used, copied, reproduced, modified, published, uploaded, posted, transmitted, distributed, or disclosed in any way without Intel’s prior express written permission.

 No license under any patent, copyright, trade secret or other intellectual property right is granted to or conferred upon you by disclosure or delivery of the Materials, either expressly, by implication, inducement, estoppel or otherwise. Any license under such intellectual property rights must be express and approved by Intel in writing.
*/

/**
 * SECTION:mixaudio
 * @short_description: Object to support a single stream playback using hardware accelerated decoder.
 * @include: mixaudio.h
 * 
 * #MixAudio object provide thread-safe API for application and/or multimedia framework to take advantage of Intel Smart Sound Technology(TM) driver for hardware audio decode and render.
 * 
 * Each #MixAudio object represents one streaming session with the Intel Smart Sound driver and provides configuration and control of the decoding and playback options.
 * 
 * The #MixAudio object also support integration with Intel Audio Manager service.
 * 
 * An application can utilize the #MixAudio object by calling the following sequence:
 * <orderedlist numeration="arabic">
 * <listitem>mix_audio_new() to create a #MixAudio instance.</listitem>
 * <listitem>mix_audio_initialize() to allocate Intel Smart Sound Technology resource.</listitem>
 * <listitem>mix_audio_configure() to configure stream parameters.</listitem>
 * <listitem>mix_audio_decode() can be called repeatedly for decoding and, optionally, rendering.</listitem>
 * <listitem>mix_audio_start() is called after the 1st mix_audio_decode() method to start rendering.</listitem>
 * <listitem>mix_audio_stop_drain() is called after the last buffer is passed for decoding in with mix_audio_decode(). </listitem>
 * <listitem>mix_audio_deinitialize() to free resource once playback is completed.</listitem>
 * </orderedlist>
 * 
 * Since mix_audio_decode() is a blocking call during playback, the following methods are called in a seperate thread to control progress:
 * <itemizedlist>
 * <listitem>mix_audio_start()</listitem>
 * <listitem>mix_audio_pause()</listitem>
 * <listitem>mix_audio_resume()</listitem>
 * <listitem>mix_audio_stop_drop()</listitem>
 * </itemizedlist>
 */

/**
 * SECTION:mixaudiotypes
 * @title: Mix Audio Types
 * @short_description: Miscellanous types used by #MixAudio API.
 * @include: mixaudiotypes.h
 * 
 * Miscellanous types used by #MixAudio API.
*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <unistd.h>
#include <sys/uio.h>
#include <string.h>

#include <glib.h>
#include <glib/gprintf.h>
#include <mixlog.h>
#include "mixaudio.h"
#include "mixacpaac.h"

#ifdef AUDIO_MANAGER
#include "amhelper.h"
#endif

#ifndef MIXAUDIO_CURRENT
#define MIXAUDIO_CURRENT 0
#endif
#ifndef MIXAUDIO_AGE
#define MIXAUDIO_AGE 0
#endif

/* Include this now but it will change when driver updates.
   We would want to build against a kernel dev package if that
   is available.
*/
#include <linux/types.h>
#include "intel_sst_ioctl.h"
#include "sst_proxy.h"

#ifdef G_LOG_DOMAIN
#undef G_LOG_DOMAIN
#define G_LOG_DOMAIN    ((gchar*)"mixaudio")
#endif

/**
 * LPE_DEVICE:
 * 
 * LPE Device location.
 */
static const char* LPE_DEVICE="/dev/sst";
/* #define LPE_DEVICE "/dev/lpe" */

#define _LOCK(obj) g_static_rec_mutex_lock(obj);
#define _UNLOCK(obj) g_static_rec_mutex_unlock(obj);

#define _UNLOCK_RETURN(obj, res) { _UNLOCK(obj); return res; }

typedef enum {
  MIX_STREAM_PAUSED_DRAINING = MIX_STREAM_LAST,
  MIX_STREAM_INTERNAL_LAST
} MixStreamStateInternal;


MIX_RESULT mix_audio_initialize_default(MixAudio *mix, MixCodecMode mode, MixAudioInitParams *aip, MixDrmParams *drminitparams);
MIX_RESULT mix_audio_configure_default(MixAudio *mix, MixAudioConfigParams *audioconfigparams, MixDrmParams *drmparams);
MIX_RESULT mix_audio_decode_default(MixAudio *mix, const MixIOVec *iovin, gint iovincnt, guint64 *insize, MixIOVec *iovout, gint iovoutcnt);
MIX_RESULT mix_audio_capture_encode_default(MixAudio *mix, MixIOVec *iovout, gint iovoutcnt);
MIX_RESULT mix_audio_start_default(MixAudio *mix);
MIX_RESULT mix_audio_stop_drop_default(MixAudio *mix);
MIX_RESULT mix_audio_stop_drain_default(MixAudio *mix);
MIX_RESULT mix_audio_pause_default(MixAudio *mix);
MIX_RESULT mix_audio_resume_default(MixAudio *mix);
MIX_RESULT mix_audio_get_timestamp_default(MixAudio *mix, guint64 *msecs);
MIX_RESULT mix_audio_set_mute_default(MixAudio *mix, gboolean mute);
MIX_RESULT mix_audio_get_mute_default(MixAudio *mix, gboolean* muted);
MIX_RESULT mix_audio_get_max_vol_default(MixAudio *mix, gint *maxvol);
MIX_RESULT mix_audio_get_min_vol_default(MixAudio *mix, gint *minvol);
MIX_RESULT mix_audio_get_volume_default(MixAudio *mix, gint *currvol, MixVolType type);
MIX_RESULT mix_audio_set_volume_default(MixAudio *mix, gint currvol, MixVolType type, gulong msecs, MixVolRamp ramptype);
MIX_RESULT mix_audio_deinitialize_default(MixAudio *mix);
MIX_RESULT mix_audio_get_stream_state_default(MixAudio *mix, MixStreamState *streamState);
MIX_RESULT mix_audio_get_state_default(MixAudio *mix, MixState *state);
MIX_RESULT mix_audio_get_config_default(MixAudio *mix, MixAudioConfigParams **audioconfigparams);

static gboolean g_IAM_available = FALSE;
MIX_RESULT mix_audio_am_unregister(MixAudio *mix, MixAudioConfigParams *audioconfigparams);
MIX_RESULT mix_audio_am_register(MixAudio *mix, MixAudioConfigParams *audioconfigparams);
MIX_RESULT mix_audio_AM_Change(MixAudioConfigParams *oldparams, MixAudioConfigParams *newparams);
static MIX_RESULT mix_audio_verify_params(MixAudio *mix, const MixAudioConfigParams *audioconfigparams);

static void mix_audio_finalize(GObject *obj);
G_DEFINE_TYPE (MixAudio, mix_audio, G_TYPE_OBJECT);

static gboolean has_FW_INFO = FALSE;
static struct snd_sst_fw_info cur_FW_INFO = {{0}};

static MIX_RESULT mix_audio_FW_INFO(MixAudio *mix);
static MIX_RESULT mix_audio_SST_SET_PARAMS(MixAudio *mix, MixAudioConfigParams *params);
static MIX_RESULT mix_audio_SST_writev(MixAudio *mix, const MixIOVec *iovin, gint iovincnt, guint64 *insize);
static MIX_RESULT mix_audio_SST_STREAM_DECODE(MixAudio *mix, const MixIOVec *iovin, gint iovincnt, guint64 *insize, MixIOVec *iovout, gint iovoutcnt);
static void mix_audio_debug_dump(MixAudio *mix);
static MIX_RESULT mix_audio_is_stream_changed(MixAudio *mix, MixAudioConfigParams *new_params, gboolean *pChanged);

static guint g_log_handler=0;
static void mix_audio_log(const gchar *log_domain, GLogLevelFlags log_level, const gchar *message, gpointer user_data);

/**
 * mix_acp_print_params:
 * @obj: TBD
 * 
 * This method is to print acp param. It is a hidden implementation within MixAudioConfigParams.
*/
void mix_acp_print_params(MixAudioConfigParams *obj);

static void mix_audio_init (MixAudio *self)
{
  self->streamID = 0; // TODO: Find out the invalid value for stream ID when integrates with IAM.
  self->amStreamID = 0;         // TODO: as above
  self->streamState = MIX_STREAM_NULL;
  self->encoding = NULL;
  self->fileDescriptor = -1;
  self->state = MIX_STATE_UNINITIALIZED;
  self->codecMode = MIX_CODING_INVALID;
  self->am_registered = FALSE;
  self->stream_muted = FALSE;

  /* private member initialization */
  g_static_rec_mutex_init (&self->streamlock);
  g_static_rec_mutex_init (&self->controllock);

  self->audioconfigparams = NULL;
  self->deviceState = MIX_AUDIO_DEV_CLOSED;

#ifdef LPESTUB
  g_message("MixAudio running in stub mode!");
  self->ts_last = 0;
  self->ts_elapsed = 0;
#endif

  self->bytes_written=0;

}

void _mix_aip_initialize (void);

static void mix_audio_class_init (MixAudioClass *klass)
{
  GObjectClass *gobject_class = (GObjectClass*)klass;

  gobject_class->finalize = mix_audio_finalize;

  // Init thread before any threads/sync object are used.
  if (!g_thread_supported ()) g_thread_init (NULL);

  /* Init some global vars */
  g_IAM_available = FALSE;

  // base implementations
  klass->initialize = mix_audio_initialize_default;
  klass->configure = mix_audio_configure_default;
  klass->decode = mix_audio_decode_default;
  klass->capture_encode = mix_audio_capture_encode_default;
  klass->start = mix_audio_start_default;
  klass->stop_drop = mix_audio_stop_drop_default;
  klass->stop_drain = mix_audio_stop_drain_default;
  klass->pause = mix_audio_pause_default;
  klass->resume = mix_audio_resume_default;
  klass->get_timestamp = mix_audio_get_timestamp_default;
  klass->set_mute = mix_audio_set_mute_default;
  klass->get_mute = mix_audio_get_mute_default;
  klass->get_max_vol = mix_audio_get_max_vol_default;
  klass->get_min_vol = mix_audio_get_min_vol_default;
  klass->get_volume = mix_audio_get_volume_default;
  klass->set_volume = mix_audio_set_volume_default;
  klass->deinitialize = mix_audio_deinitialize_default;
  klass->get_stream_state = mix_audio_get_stream_state_default;
  klass->get_state = mix_audio_get_state_default;
  klass->get_config = mix_audio_get_config_default;

  // Set log handler...
  if (!g_log_handler)
  {
    // Get Environment variable 
    // See mix_audio_log for details
    const gchar* loglevel = g_getenv("MIX_AUDIO_DEBUG");
    guint64 ll = 0;
    if (loglevel)
    {
      if (g_strstr_len(loglevel,-1, "0x") == loglevel)
      {
        // Hex string
        ll = g_ascii_strtoull(loglevel+2, NULL, 16);
      }
      else
      {
        // Decimal string
        ll = g_ascii_strtoull(loglevel, NULL, 10);
      }
    }
    guint32 mask = (guint32)ll;
    g_log_handler = g_log_set_handler(G_LOG_DOMAIN, 0xffffffff, mix_audio_log, (gpointer)mask);
/*
    g_debug("DEBUG Enabled");
    g_log(G_LOG_DOMAIN, G_LOG_LEVEL_INFO, "%s", "LOG Enabled");
    g_message("MESSAGE Enabled");
    g_warning("WARNING Enabled");
    g_critical("CRITICAL Enabled");
    g_error("ERROR Enabled");
*/
  }
}

static void mix_audio_log(const gchar *log_domain, GLogLevelFlags log_level, const gchar *message, gpointer user_data)
{
  // Log message based on a mask.
  // Mask could be read from MIX_AUDIO_DEBUG environment variable
  // mask is a bit mask specifying the message to print. The lsb (0) is "ERROR" and graduating increasing
  // value as describe in GLogLevelFlags structure. Not that lsb in GLogLevelFlags is not "ERROR" and
  // here we shifted the log_level to ignore the first 2 values in GLogLevelFlags, making ERROR align to
  // the lsb.
  static const gchar* lognames[] = {"error", "critical", "warning", "message", "log", "debug"};
  guint32 mask = (guint32)user_data & ((G_LOG_LEVEL_MASK & log_level) >> 2);
  gint index = 0;

  GTimeVal t = {0};

  // convert bit mask back to index.
  index = ffs(mask) - 1;

  if ((index<0) || (index >= (sizeof(lognames)/sizeof(lognames[0])))) return;

  g_get_current_time(&t);
  g_printerr("%" G_GUINT64_FORMAT ":%s-%s: %s\n", 
    ((guint64)1000000 * t.tv_sec + (guint64)t.tv_usec),
    log_domain?log_domain:G_LOG_DOMAIN,
    lognames[index],
    message?message:"NULL");
}

MixAudio *mix_audio_new(void)
{
  MixAudio *ret = g_object_new(MIX_TYPE_AUDIO, NULL);

  return ret;
}

void mix_audio_finalize(GObject *obj)
{
  /* clean up here. */
  MixAudio *mix = MIX_AUDIO(obj);

  if (G_UNLIKELY(!mix)) return;

  /*
    We are not going to check the thread lock anymore in this method.
    If a thread is accessing the object it better still have a ref on this
    object and in that case, this method won't be called.
    
    The application have to risk access violation if it calls the methods in
    a thread without actually holding a reference.
  */

  g_debug("_finalized(). bytes written=%" G_GUINT64_FORMAT, mix->bytes_written);

  g_static_rec_mutex_free (&mix->streamlock);
  g_static_rec_mutex_free (&mix->controllock);

  if (mix->audioconfigparams)
  {
    mix_acp_unref(mix->audioconfigparams);
    mix->audioconfigparams = NULL;
  }
}

MixAudio *mix_audio_ref(MixAudio *mix) 
{ 
  if (G_UNLIKELY(!mix)) return NULL;

  return (MixAudio*)g_object_ref(G_OBJECT(mix)); 
}

MIX_RESULT mix_audio_initialize_default(MixAudio *mix, MixCodecMode mode, MixAudioInitParams *aip, MixDrmParams *drminitparams)
{
    // API version 0.79. 1/19/2009
    MIX_RESULT ret = MIX_RESULT_SUCCESS;

    if (G_UNLIKELY(!mix)) return MIX_RESULT_NULL_PTR;

    if ((mode <= MIX_CODING_INVALID) || (mode >= MIX_CODING_LAST)) return MIX_RESULT_INVALID_PARAM;

    // initialized must be called with both thread-lock held, so no other operation is allowed.
    
    // try lock stream thread. If failed, a pending _decode/_encode/_drain is ongoing.
    if (!g_static_rec_mutex_trylock(&mix->streamlock)) return MIX_RESULT_WRONG_STATE;

    // also lock the control thread lock.
    _LOCK(&mix->controllock);

    if (mix->state != MIX_STATE_UNINITIALIZED)
        ret = MIX_RESULT_ALREADY_INIT;

    if (!MIX_SUCCEEDED(ret))
    {
        _UNLOCK(&mix->controllock);
        _UNLOCK(&mix->streamlock);
        return ret;
    }

          // Open device. Same flags to open for decode and encode?
#ifdef LPESTUB
    {
        gchar *filename = NULL;
        GError *err = NULL;
        const gchar* fn = NULL;
        fn = g_getenv("MIX_AUDIO_OUTPUT");
        if (fn) mix->fileDescriptor = open(fn, O_RDWR|O_CREAT, S_IRUSR|S_IWUSR);
        if (mix->fileDescriptor == -1)
        {
            mix->fileDescriptor = g_file_open_tmp ("mixaudio.XXXXXX", &filename, &err);
            if (err)
            {
                g_warning("Oops, cannot open temp file: Error message: %s", err->message);
            }
            else
            {
                g_debug("Opening %s instead of %s as output data file.\n", filename, LPE_DEVICE);
            }
        }
        else
        {
            g_debug("Opening %s as output data file.\n", fn);
        }
        if (filename) g_free(filename);
    }
#else
    g_debug("Calling open(%s)\n", LPE_DEVICE);
    mix->fileDescriptor = open(LPE_DEVICE, O_RDWR);
    g_debug("open returned %d", mix->fileDescriptor);
#endif

    if (mix->fileDescriptor != -1)
    {
        mix->codecMode = mode;
        mix->state = MIX_STATE_INITIALIZED;
        ret = MIX_RESULT_SUCCESS;
        mix->deviceState = MIX_AUDIO_DEV_OPENED;
    }
    else
    {
        ret = MIX_RESULT_LPE_NOTAVAIL;
        g_warning("open() failed. Error(0x%08x): %s", errno, strerror(errno));
    }

    _UNLOCK(&mix->controllock);
    _UNLOCK(&mix->streamlock);

    return ret;
}

gboolean mix_audio_am_is_available(void)
{
  // return FALSE for now until IAM is available for integration.
  // TODO: Check IAM
  return FALSE;
}

gboolean mix_audio_base_am_is_enabled(MixAudio *mix)
{
  // TODO: Check IAM usage
  return FALSE;
}

/**
 * mix_audio_SST_SET_PARAMS:
 * @mix: #MixAudio object.
 * @params: Audio parameter used to configure SST.
 * @returns: #MIX_RESULT indicating configuration result.
 * 
 * This method setup up a SST stream with the given parameters. Note that even though 
 * this method could succeed and SST stream is setup properly, client may still not be able
 * to use the session if other condition are met, such as a successfully set-up IAM, if used.
 */
MIX_RESULT mix_audio_SST_SET_PARAMS(MixAudio *mix, MixAudioConfigParams *params)
{
  MIX_RESULT ret = MIX_RESULT_SUCCESS;

  if (G_UNLIKELY(!mix)) return MIX_RESULT_NULL_PTR;

  if (mix->state == MIX_STATE_UNINITIALIZED) return MIX_RESULT_NOT_INIT;

  if (!MIX_IS_AUDIOCONFIGPARAMS(params)) return MIX_RESULT_INVALID_PARAM;

  mix_acp_print_params(params);

  struct snd_sst_params sst_params = {0};

  gboolean converted = mix_sst_params_convert(params, &sst_params);

  if (converted)
  {
    // Setup the driver structure
    // We are assuming the configstream will always be called after open so the codec mode
    // should already been setup.
    sst_params.stream_id = mix->streamID;
    // We are not checking the codecMODE here for out-of-range...assuming we check that
    // during init...
    if (mix->codecMode == MIX_CODING_ENCODE)
      sst_params.ops = STREAM_OPS_CAPTURE;
    else sst_params.ops = STREAM_OPS_PLAYBACK;

     // hard-coded to support music only.
    sst_params.stream_type = 0x0; // stream_type 0x00 is STREAM_TYPE_MUSIC per SST doc.

    // SET_PARAMS
    int retVal = 0;

#ifdef LPESTUB
    // Not calling the ioctl
#else
    g_debug("Calling SNDRV_SST_STREAM_SET_PARAMS. fd=%d", mix->fileDescriptor);
    retVal = ioctl(mix->fileDescriptor, SNDRV_SST_STREAM_SET_PARAMS, &sst_params);
    g_debug("_SET_PARAMS returned %d", retVal);
#endif

    if (!retVal)
    {
      // IOCTL success.
      switch (sst_params.result)
      {
        // Please refers to SST API doc for return value definition.
        case 5:
          g_debug("SET_PARAMS succeeded with Stream Parameter Modified.");
        case 0:
          // driver says ok, too.
          ret = MIX_RESULT_SUCCESS;
          mix->deviceState = MIX_AUDIO_DEV_ALLOCATED;
          mix->streamState = MIX_STREAM_STOPPED;
          mix->streamID = sst_params.stream_id;
          // clear old params
          if (MIX_IS_AUDIOCONFIGPARAMS(mix->audioconfigparams))
          {
            mix_acp_unref(mix->audioconfigparams);
            mix->audioconfigparams=NULL;
          }
          // replace with new one.
          mix->audioconfigparams = MIX_AUDIOCONFIGPARAMS(mix_params_dup(MIX_PARAMS(params)));
          // Note: do not set mix->state here because this state may rely op other than SET_PARAMS
          g_debug("SET_PARAMS succeeded streamID=%d.", mix->streamID);
          break;
        case 1:
          ret = MIX_RESULT_STREAM_NOTAVAIL;
          g_debug("SET_PARAMS failed STREAM not available.");
          break;
        case 2:
          ret = MIX_RESULT_CODEC_NOTAVAIL;
          g_debug("SET_PARAMS failed CODEC not available.");
          break;
        case 3:
          ret = MIX_RESULT_CODEC_NOTSUPPORTED;
          g_debug("SET_PARAMS failed CODEC not supported.");
          break;
        case 4:
          ret = MIX_RESULT_INVALID_PARAM;
          g_debug("SET_PARAMS failed Invalid Stream Parameters.");
          break;
        case 6:
          g_debug("SET_PARAMS failed Invalid Stream ID.");
        default:
          ret = MIX_RESULT_FAIL;
          g_critical("SET_PARAMS failed unexpectedly. Result code: %u\n", sst_params.result);
          break;
      }
    }
    else
    {
      // log errors
      ret = MIX_RESULT_SYSTEM_ERRNO; 
      g_debug("Failed to SET_PARAMS. errno:0x%08x. %s\n", errno, strerror(errno));
    }
  }
  else
  {
    ret = MIX_RESULT_INVALID_PARAM;
  }

  return ret;
}

MIX_RESULT mix_audio_get_state_default(MixAudio *mix, MixState *state)
{
    // API version 0.79. 1/22/2009
    MIX_RESULT ret = MIX_RESULT_SUCCESS;

    if (G_UNLIKELY(mix == NULL)) return MIX_RESULT_NULL_PTR;
    if (state == NULL) return MIX_RESULT_NULL_PTR;

    *state = mix->state;

    return ret;
}

MIX_RESULT mix_audio_decode_default(MixAudio *mix, const MixIOVec *iovin, gint iovincnt, guint64 *insize, MixIOVec *iovout, gint iovoutcnt)
{
  MIX_RESULT ret = MIX_RESULT_FAIL;

  if (G_UNLIKELY(!mix)) return MIX_RESULT_NULL_PTR;

  if (!g_static_rec_mutex_trylock(&mix->streamlock)) return MIX_RESULT_WRONG_STATE;

  if (mix->state != MIX_STATE_CONFIGURED) _UNLOCK_RETURN(&mix->streamlock, MIX_RESULT_WRONG_STATE);

  if (MIX_ACP_DECODEMODE(mix->audioconfigparams) == MIX_DECODE_DIRECTRENDER)
    ret = mix_audio_SST_writev(mix, iovin, iovincnt, insize);
  else
    ret = mix_audio_SST_STREAM_DECODE(mix, iovin, iovincnt, insize, iovout, iovoutcnt);

  _UNLOCK(&mix->streamlock);

  return ret;
}

MIX_RESULT mix_audio_deinitialize_default(MixAudio *mix)
{
  MIX_RESULT ret = MIX_RESULT_SUCCESS;
  if (G_UNLIKELY(!mix)) return MIX_RESULT_NULL_PTR;

  if (!g_static_rec_mutex_trylock(&mix->streamlock)) return MIX_RESULT_WRONG_STATE;

#ifdef AUDIO_MANAGER
  if (mix->amStreamID && (lpe_stream_unregister(mix->amStreamID) < 0)) {
    g_debug("lpe_stream_unregister failed\n");
    //return MIX_RESULT_FAIL;   // TODO: not sure what to do here
  }
#endif

  _LOCK(&mix->controllock);

  if (mix->state == MIX_STATE_UNINITIALIZED)
    ret = MIX_RESULT_SUCCESS;
  else if ((mix->streamState != MIX_STREAM_STOPPED) && (mix->streamState != MIX_STREAM_NULL))
    ret = MIX_RESULT_WRONG_STATE;
  else
  {
    if (mix->fileDescriptor != -1)
    {
      g_debug("Closing fd=%d\n", mix->fileDescriptor);
      close(mix->fileDescriptor);
      mix->fileDescriptor = -1;
      mix->deviceState = MIX_AUDIO_DEV_CLOSED;
    }
    mix->state = MIX_STATE_UNINITIALIZED;
    mix->stream_muted = FALSE;
  }

  mix->bytes_written = 0;

  _UNLOCK(&mix->controllock);
  _UNLOCK(&mix->streamlock);

  return ret;
}


MIX_RESULT mix_audio_stop_drop_default(MixAudio *mix)
{
    // API version 0.79. 1/22/2009
    MIX_RESULT ret = MIX_RESULT_SUCCESS;
    int retVal = 0;

    if (G_UNLIKELY(!mix)) return MIX_RESULT_NULL_PTR;

    _LOCK(&mix->controllock);

    // ret should be SUCCESS when this sequence starts...
    if (mix->state != MIX_STATE_CONFIGURED) 
    {
        // Not allowing control operation if it is not configured.
        ret = MIX_RESULT_NOT_CONFIGURED;
    }
    else if (MIX_ACP_DECODEMODE(mix->audioconfigparams) == MIX_DECODE_DECODERETURN)
    {
        // Not allowing control operation if it is configured as DNR.
        ret = MIX_RESULT_WRONGMODE;
    }

    if (!MIX_SUCCEEDED(ret)) _UNLOCK_RETURN(&mix->controllock, ret);

  // Will call DROP even if we are already stopped. It is needed to unblock any pending readv()/write() call that is made after the last STOP_x and before the next START.
#ifdef LPESTUB
    // Not calling ioctl.
#else
    g_debug("Calling SNDRV_SST_STREAM_DROP. fd=%d", mix->fileDescriptor);
    retVal = ioctl(mix->fileDescriptor, SNDRV_SST_STREAM_DROP);
    g_debug("_DROP returned %d", retVal);
#endif

    if (retVal != 0)
    {
        ret = MIX_RESULT_SYSTEM_ERRNO; 
        g_warning("Failed to stop stream. Error(0x%08x): %s", errno, strerror(errno));
    }
    else
    {
        mix->streamState = MIX_STREAM_STOPPED;
        ret = MIX_RESULT_SUCCESS;
    }

    _UNLOCK(&mix->controllock);

    return ret;
}

MIX_RESULT mix_audio_stop_drain_default(MixAudio *mix)
{
    // API version 0.79. 1/22/2009
    MIX_RESULT ret = MIX_RESULT_SUCCESS;
    int retVal = 0;
    gboolean doDrain = FALSE;
    
    if (G_UNLIKELY(!mix)) return MIX_RESULT_NULL_PTR;

    // No need to lock to check vars that won't be changed in this function

    // If cannot obtain stream lock meaning there's a pending _decode/_encode and will not proceed.
    if (!g_static_rec_mutex_trylock(&mix->streamlock)) return MIX_RESULT_WRONG_STATE;

    _LOCK(&mix->controllock);

    // Check unallowed condition

    // ret should be SUCCESS when this sequence starts...
    if (mix->codecMode != MIX_CODING_DECODE) 
    {
        // Not allowing control operation if it is configure for decode.
        ret = MIX_RESULT_WRONGMODE;
    }
    else if (mix->state != MIX_STATE_CONFIGURED) 
    {
        // Not allowing control operation if it is not configured.
        ret = MIX_RESULT_NOT_CONFIGURED;
    }
    else if (MIX_ACP_DECODEMODE(mix->audioconfigparams) == MIX_DECODE_DECODERETURN)
    {
        // Not allowing control operation if it is configured as DNR.
        ret = MIX_RESULT_WRONGMODE;
    }

    // Now check if we need to exit.
    if (!MIX_SUCCEEDED(ret))
    {
        _UNLOCK(&mix->controllock);
        _UNLOCK(&mix->streamlock);
        return ret;
    }

    if (mix->streamState == MIX_STREAM_STOPPED)
        // no need to drain and we already stopped.
        ret = MIX_RESULT_SUCCESS;
    else if ((mix->streamState == MIX_STREAM_DRAINING) || mix->streamState == MIX_STREAM_PAUSED_DRAINING)
        // Not allowed if we are already draining or PAUSED in draining state.
        ret = MIX_RESULT_WRONG_STATE;
    else
    {
        doDrain = TRUE;
        g_debug("MIX stream needs DRAINING");
        mix->streamState = MIX_STREAM_DRAINING;
        // Set state to MIX_STREAM_DRAINING and other operations that may change teh streamState has to
        // be careful when handling stream with this state.
    }

    // release the control lock. we only hold the stream lock during this blocking DRAIN call.
    _UNLOCK(&mix->controllock);


    if (doDrain)
    {
      // Calling the blocking DRAIN holding just the stream lock, without the control lock

#ifdef LPESTUB
      
#else
      g_debug("Calling SNDRV_SST_STREAM_DRAIN fd=%d", mix->fileDescriptor);
      retVal = ioctl(mix->fileDescriptor, SNDRV_SST_STREAM_DRAIN);
      g_debug("_DRAIN returned %d", retVal);
#endif

        // obtain control lock and update state.
        _LOCK(&mix->controllock);

        if (retVal != 0)
        {
            if (mix->streamState != MIX_STREAM_STOPPED)
            {
                // DRAIN could return failed if DROP is called during DRAIN.
                // Any state resulting as a failed DRAIN would be error, except STOPPED.
                ret = MIX_RESULT_SYSTEM_ERRNO;
                g_debug("Failed to drain stream. Error(0x%08x): %s", errno, strerror(errno));
            }
        }
        else
        {
            if ((mix->streamState != MIX_STREAM_DRAINING) &&
                (mix->streamState != MIX_STREAM_STOPPED))
            {
                // State is changed while in DRAINING. This should not be allowed and is a bug.
                g_warning("MIX Internal state error! DRAIN state(%u) changed!",mix->streamState);
                ret = MIX_RESULT_FAIL;
            }
            else
            {
                mix->streamState = MIX_STREAM_STOPPED;
                ret = MIX_RESULT_SUCCESS;
            }
        }
    
        _UNLOCK(&mix->controllock);
    }

    _UNLOCK(&mix->streamlock);

  return ret;
}

MIX_RESULT mix_audio_start_default(MixAudio *mix)
{
  // API version 0.79. 1/22/2009
  MIX_RESULT ret = MIX_RESULT_FAIL;

  if (G_UNLIKELY(mix==NULL)) return MIX_RESULT_NULL_PTR;

  _LOCK(&mix->controllock);

  if (mix->state != MIX_STATE_CONFIGURED)
    _UNLOCK_RETURN(&mix->controllock, MIX_RESULT_NOT_CONFIGURED);

  // Not allowed if in DNR mode.
  if (MIX_ACP_DECODEMODE(mix->audioconfigparams) == MIX_DECODE_DECODERETURN)
    _UNLOCK_RETURN(&mix->controllock, MIX_RESULT_WRONGMODE);

  // Note this impl return success even if stream is already started.
  switch (mix->streamState)
  {
    case MIX_STREAM_PLAYING:
    case MIX_STREAM_PAUSED:
    case MIX_STREAM_PAUSED_DRAINING:
      ret = MIX_RESULT_SUCCESS;
      break;
    case MIX_STREAM_STOPPED:
      {
        int retVal = 0;
#ifdef LPESTUB
        // Not calling ioctl.
#else
        g_debug("Calling SNDRV_SST_STREAM_START. fd=%d", mix->fileDescriptor);
        retVal = ioctl(mix->fileDescriptor, SNDRV_SST_STREAM_START);
        g_debug("_START returned %d", retVal);
#endif

        if (retVal != 0)
        {
          ret = MIX_RESULT_SYSTEM_ERRNO; 
          g_debug("Fail to START. Error:0x%08x. Stream state unchanged.", errno);
          mix_audio_debug_dump(mix);
        }
        else
        {
          mix->streamState = MIX_STREAM_PLAYING;
          ret = MIX_RESULT_SUCCESS;
        }
      }
      break;
    case MIX_STREAM_DRAINING:
    default:
      ret = MIX_RESULT_WRONG_STATE;
      break;
  }

  _UNLOCK(&mix->controllock);

#ifdef LPESTUB
  if (MIX_SUCCEEDED(ret))
  {
    if (mix->ts_last == 0)
    {
      GTimeVal tval = {0};
      g_get_current_time(&tval);
      mix->ts_last = 1000ll * tval.tv_sec + tval.tv_usec / 1000;
    }
  }
#endif
  return ret;
}

MIX_RESULT mix_audio_get_version(guint* major, guint *minor)
{
  // simulate the way libtool generate version so the number synchronize with the filename.
  if (major)
    *major = MIXAUDIO_CURRENT-MIXAUDIO_AGE;

  if (minor)
    *minor = MIXAUDIO_AGE;

  return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_audio_configure_default(MixAudio *mix, MixAudioConfigParams *audioconfigparams, MixDrmParams *drmparams)
{
  // API version 0.79. 1/19/2009

  MIX_RESULT ret = MIX_RESULT_FAIL;
  gboolean changed = FALSE;
  MixParams *new_params = NULL;

  if (G_UNLIKELY(!mix)) return MIX_RESULT_NULL_PTR;

  g_message("_configure() starts.");

  // input param checks
  if (!MIX_IS_AUDIOCONFIGPARAMS(audioconfigparams)) return MIX_RESULT_NOT_ACP;
  if (MIX_ACP_DECODEMODE(audioconfigparams) >= MIX_DECODE_LAST) return MIX_RESULT_INVALID_DECODE_MODE;
  if (!mix_acp_is_streamname_valid(audioconfigparams)) return MIX_RESULT_INVALID_STREAM_NAME;

  // dup a copy. we will need it when configure is successful.
  // don't want to find out we can't dup it only after everything is set.
  new_params = mix_params_dup(MIX_PARAMS(audioconfigparams));
  if (!MIX_IS_AUDIOCONFIGPARAMS(new_params))
  {
    if (MIX_IS_PARAMS(new_params)) mix_params_unref(new_params);
    g_error("Failed to duplicate input MixAudioConfigParams!");
    return MIX_RESULT_FAIL;
  }

  // If we cannot lock stream thread, data is flowing and we can't configure.
  if (!g_static_rec_mutex_trylock(&mix->streamlock)) return MIX_RESULT_WRONG_STATE;

  _LOCK(&mix->controllock);

  // Check all unallowed conditions
  if (mix->state == MIX_STATE_UNINITIALIZED) 
  {
    ret = MIX_RESULT_NOT_INIT; // Will not allowed if the state is still UNINITIALIZED
    goto _configure_done;
  }

  if ((mix->streamState != MIX_STREAM_STOPPED) && (mix->streamState != MIX_STREAM_NULL))
  {
    ret = MIX_RESULT_WRONG_STATE;
    goto _configure_done;
  }

  ret = mix_audio_verify_params(mix, audioconfigparams);
  if (!MIX_SUCCEEDED(ret)) goto _configure_done;

  // now configure stream.

#ifdef AUDIO_MANAGER
  ret = mix_audio_am_unregister(mix, audioconfigparams);
#else
  ret = MIX_RESULT_SUCCESS;
#endif
  if (!MIX_SUCCEEDED(ret)) goto _configure_failed;

  ret = mix_audio_is_stream_changed(mix, audioconfigparams, &changed);
  if (!MIX_SUCCEEDED(ret)) goto _configure_failed;

  if (changed)
  {
    ret = mix_audio_SST_SET_PARAMS(mix, audioconfigparams);
    if (!MIX_SUCCEEDED(ret)) goto _configure_failed;
  }
  else
  {
    g_message("No stream change is detected. Not calling SET_PARAMS.");
  }

#ifdef AUDIO_MANAGER
  ret = mix_audio_am_register(mix, audioconfigparams);
#else
  ret = MIX_RESULT_SUCCESS;
#endif
  if (!MIX_SUCCEEDED(ret)) goto _configure_failed;

  ret = MIX_RESULT_SUCCESS;

_configure_failed:

    if (mix->audioconfigparams)
    {
        mix_acp_unref(mix->audioconfigparams);
        mix->audioconfigparams=NULL;
    }

    if (MIX_SUCCEEDED(ret))
    {
        mix->state = MIX_STATE_CONFIGURED;
        mix->audioconfigparams = MIX_AUDIOCONFIGPARAMS(new_params);
        new_params = NULL;
    }
    else
    {
        // attempt to configure has failed. Revert state back to INITIALIZED only.
        mix->state = MIX_STATE_INITIALIZED;
        mix->streamState = MIX_STREAM_STOPPED;
        // No need to un-set SST PARAM. not supported really.
    }

_configure_done:

    _UNLOCK(&mix->controllock);
    _UNLOCK(&mix->streamlock);

    if (new_params)
    {
        mix_params_unref(new_params);
    }

    g_message("_configure() done returning 0x%08x.", ret);

    return ret;
}

MIX_RESULT mix_audio_get_timestamp_default(MixAudio *mix, guint64 *msecs)
{
    // API version 0.79. 1/22/2009
    MIX_RESULT ret = MIX_RESULT_SUCCESS;
    unsigned long long ts = 0;
    int retVal = 0;
    
    if (G_UNLIKELY(!mix)) return MIX_RESULT_NULL_PTR;    
    if (!msecs) return MIX_RESULT_NULL_PTR;

    _LOCK(&mix->controllock);

    // ret should be SUCCESS when this sequence starts...
    if (mix->codecMode != MIX_CODING_DECODE) 
        ret = MIX_RESULT_WRONGMODE;
    else if (mix->state != MIX_STATE_CONFIGURED) 
        ret = MIX_RESULT_NOT_CONFIGURED;
    else if (MIX_ACP_DECODEMODE(mix->audioconfigparams) == MIX_DECODE_DECODERETURN)
        ret = MIX_RESULT_WRONGMODE;

    // Now check if we need to exit.
    if (!MIX_SUCCEEDED(ret)) _UNLOCK_RETURN(&mix->controllock, ret);


#ifdef LPESTUB
    // For stubbing, just get system clock.
    if (MIX_ACP_BITRATE(mix->audioconfigparams) > 0)
    {
        // use bytes_written and bitrate
        // to get times in msec.
        ts = mix->bytes_written * 8000 / MIX_ACP_BITRATE(mix->audioconfigparams);
    }
    else if (mix->ts_last)
    {
        GTimeVal tval = {0};
        g_get_current_time(&tval);
        ts = 1000ll * tval.tv_sec + tval.tv_usec / 1000;
        ts -= mix->ts_last;
        ts += mix->ts_elapsed;
    }
    else
    {
        ts = 0;
    }
#else
    g_debug("Calling _GET_TSTAMP. fd=%d", mix->fileDescriptor);
    retVal = ioctl(mix->fileDescriptor, SNDRV_SST_STREAM_GET_TSTAMP, &ts);
    g_debug("_GET_TSTAMP returned %d. timestamp=%" G_GUINT64_FORMAT, retVal, ts);
#endif

    if (retVal != 0)
    {
        ret = MIX_RESULT_SYSTEM_ERRNO; 
        g_debug("_GET_TSTAMP failed. Error(0x%08x): %s", errno, strerror(errno));
        mix_audio_debug_dump(mix);
    }
    else
    {
        *msecs = ts;
    }

    _UNLOCK(&mix->controllock);

    return ret;
}

#ifdef AUDIO_MANAGER
gboolean mix_audio_AM_Change(MixAudioConfigParams *oldparams, MixAudioConfigParams *newparams) 
{
  if (strcmp(oldparams->stream_name, newparams->stream_name) == 0) {
    return FALSE;
  }

  return TRUE;
}

MIX_RESULT mix_audio_am_unregister(MixAudio *mix, MixAudioConfigParams *audioconfigparams)
{
  MIX_RESULT ret = MIX_RESULT_SUCCESS;

  if (G_UNLIKELY(!mix)) return MIX_RESULT_NULL_PTR;

  if (mix->am_registered && MIX_IS_AUDIOCONFIGPARAMS(mix->audioconfigparams) && MIX_IS_AUDIOCONFIGPARAMS(audioconfigparams))
  {
    // we have 2 params. let's check
    if ((MIX_ACP_DECODEMODE(mix->audioconfigparams) != MIX_ACP_DECODEMODE(audioconfigparams)) || 
       mix_audio_AM_Change(mix->audioconfigparams, audioconfigparams)) //TODO: add checking for SST change
    {
      // decode mode change.
      if (mix->amStreamID > 0) {
        if (lpe_stream_unregister(mix->amStreamID) != 0) {
          return MIX_RESULT_AM_UNREGISTER_FAIL;
        }
        mix->amStreamID = 0;
        mix->am_registered = FALSE;
      }
    }
  }

  return ret;
}

MIX_RESULT mix_audio_am_register(MixAudio *mix, MixAudioConfigParams *audioconfigparams)
{
  MIX_RESULT ret = MIX_RESULT_SUCCESS;

  if (G_UNLIKELY(!mix)) return MIX_RESULT_NULL_PTR;

  gint32 codec_mode = -1;

  if (mix->codecMode == MIX_CODING_DECODE)
    codec_mode = 0;
  else if (mix->codecMode == MIX_CODING_ENCODE)
    codec_mode = 1;
  else
    return MIX_RESULT_FAIL;     // TODO: what to do when fail?

#ifdef AUDIO_MANAGER
  if (audioconfigparams->stream_name == NULL)
    return MIX_RESULT_FAIL;

// if AM is enable, and not_registered, then register
  if ((audioconfigparams->audio_manager == MIX_AUDIOMANAGER_INTELAUDIOMANAGER) && !mix->am_registered) {
//  if (!mix->am_registered) {  // TODO: remove this and uncomment line above
    mix_log(MIX_AUDIO_COMP, MIX_LOG_LEVEL_INFO, "audio_manager=MIX_AUDIOMANAGER_INTELAUDIOMANAGER and !am_registered\n");
    gint32 amStreamID = lpe_stream_register(mix->streamID, "music", audioconfigparams->stream_name, codec_mode);

    if (amStreamID == -1){
      mix->amStreamID = 0;
      mix_log(MIX_AUDIO_COMP, MIX_LOG_LEVEL_INFO, "AM register failed: general error\n");
      return MIX_RESULT_AM_REGISTER_FAIL;
    }
    else if (amStreamID == -2) {        // -2: Direct render not avail, see AM spec
      mix->amStreamID = 0;
      mix_log(MIX_AUDIO_COMP, MIX_LOG_LEVEL_INFO, "AM register failed: Direct render not available\n");
      return MIX_RESULT_DIRECT_NOTAVAIL;
    }
    mix->am_registered = TRUE;
    mix->amStreamID = amStreamID;
  }
#endif

  return ret;
}
#endif /* AUDIO_MANAGER */

MIX_RESULT mix_audio_capture_encode_default(MixAudio *mix, MixIOVec *iovout, gint iovoutcnt)
{
  struct iovec *vec;
  gint bytes_read;

  if (G_UNLIKELY(!mix)) return MIX_RESULT_NULL_PTR;

  // TODO: set count limit
  if (iovoutcnt < 1) {
    return MIX_RESULT_INVALID_COUNT;
  }

  if (iovout == NULL)
    return MIX_RESULT_NULL_PTR;

  vec = (struct iovec *) g_alloca(sizeof(struct iovec) * iovoutcnt);
  if (!vec) return MIX_RESULT_NO_MEMORY;

  gint i;
  for (i=0; i < iovoutcnt; i++)
  {
    vec[i].iov_base = iovout[i].data;
    vec[i].iov_len = iovout[i].buffer_size;
    iovout[i].data_size = 0;
  }

  mix_log(MIX_AUDIO_COMP, MIX_LOG_LEVEL_INFO, "begin readv()\n");
  bytes_read = readv(mix->fileDescriptor, vec, iovoutcnt);
  mix_log(MIX_AUDIO_COMP, MIX_LOG_LEVEL_INFO, "end readv(), return: %d\n", bytes_read);
  if (bytes_read < 1) { 
    mix_log(MIX_AUDIO_COMP, MIX_LOG_LEVEL_ERROR, "return: %d\n", bytes_read);
    return MIX_RESULT_FAIL;
  }

  i=0;
  while (bytes_read > 0)
  {
    if (bytes_read > iovout[i].buffer_size) {
      iovout[i].data_size = iovout[i].buffer_size;
      bytes_read = bytes_read - iovout[i].buffer_size;
    }
    else {
      iovout[i].data_size = bytes_read;
      bytes_read = 0;
    }
    i++;
  }

  return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_audio_get_max_vol_default(MixAudio *mix, gint *maxvol)
{
    // API version 0.79. 1/22/2009
  MIX_RESULT ret = MIX_RESULT_SUCCESS;

  if (G_UNLIKELY(!mix)) return MIX_RESULT_NULL_PTR;
  if (!maxvol) return MIX_RESULT_NULL_PTR;

  _LOCK(&mix->controllock);

  if (!has_FW_INFO)
  {
    ret = mix_audio_FW_INFO(mix);
  }

  if (MIX_SUCCEEDED(ret))
  {
    *maxvol = (gint)cur_FW_INFO.pop_info.max_vol;
  }

  _UNLOCK(&mix->controllock);

  return ret;
}


MIX_RESULT mix_audio_get_min_vol_default(MixAudio *mix, gint *minvol)
{
    // API version 0.79. 1/22/2009
  MIX_RESULT ret = MIX_RESULT_SUCCESS;

  if (G_UNLIKELY(!mix)) return MIX_RESULT_NULL_PTR;
  if (!minvol) return MIX_RESULT_NULL_PTR;

  _LOCK(&mix->controllock);

  if (!has_FW_INFO)
  {
    ret = mix_audio_FW_INFO(mix);
  }

  if (MIX_SUCCEEDED(ret))
  {
    *minvol = (gint)cur_FW_INFO.pop_info.min_vol;
  }

  _UNLOCK(&mix->controllock);

  return ret;
}

MIX_RESULT mix_audio_get_stream_state_default(MixAudio *mix, MixStreamState *streamState)
{
    // API version 0.79. 1/22/2009
    MIX_RESULT ret = MIX_RESULT_SUCCESS;

    if (G_UNLIKELY(mix==NULL)) return MIX_RESULT_NULL_PTR;
    if (streamState == NULL) return MIX_RESULT_NULL_PTR;

    _LOCK(&mix->controllock);

    if (mix->state != MIX_STATE_CONFIGURED) 
        ret = MIX_RESULT_NOT_CONFIGURED;
    else if (MIX_ACP_DECODEMODE(mix->audioconfigparams) == MIX_DECODE_DECODERETURN)
        ret = MIX_RESULT_WRONGMODE;

    // Now check if we need to exit.
    if (!MIX_SUCCEEDED(ret)) _UNLOCK_RETURN(&mix->controllock, ret);

    // PAUSED_DRAINING is internal state.
    if (mix->streamState == MIX_STREAM_PAUSED_DRAINING)
        *streamState = MIX_STREAM_PAUSED;
    else
        *streamState = mix->streamState;

    _UNLOCK(&mix->controllock);

    return MIX_RESULT_SUCCESS;
}


MIX_RESULT mix_audio_get_volume_default(MixAudio *mix, gint *currvol, MixVolType type)
{
    // API version 0.79. 1/22/2009
    MIX_RESULT ret = MIX_RESULT_SUCCESS;
    struct snd_sst_vol vol = {0};
    int retVal = 0;
    
    if (G_UNLIKELY(!mix)) return MIX_RESULT_NULL_PTR;
    if (!currvol) return MIX_RESULT_NULL_PTR;
    if ((type != MIX_VOL_PERCENT) && (type != MIX_VOL_DECIBELS)) return MIX_RESULT_INVALID_PARAM;

    _LOCK(&mix->controllock);

    // ret should be SUCCESS when this sequence starts...
    if (mix->codecMode != MIX_CODING_DECODE) 
        ret = MIX_RESULT_WRONGMODE;
    else if (mix->state != MIX_STATE_CONFIGURED) 
        ret = MIX_RESULT_NOT_CONFIGURED;
    else if (MIX_ACP_DECODEMODE(mix->audioconfigparams) == MIX_DECODE_DECODERETURN)
        ret = MIX_RESULT_WRONGMODE;

    // Now check if we need to exit.
    if (!MIX_SUCCEEDED(ret)) _UNLOCK_RETURN(&mix->controllock, ret);

    vol.stream_id = mix->streamID;


#ifdef LPESTUB
  // Not calling.
#else
    g_debug("Calling _GET_VOL. fd=%d", mix->fileDescriptor);
    retVal = ioctl(mix->fileDescriptor, SNDRV_SST_GET_VOL, &vol);
    g_debug("_GET_VOL returned %d. vol=%d", retVal, vol.volume);
#endif

  if (retVal != 0)
  {
    ret = MIX_RESULT_SYSTEM_ERRNO; 
    g_debug("_GET_VOL failed. Error(0x%08x): %s", errno, strerror(errno));
    mix_audio_debug_dump(mix);
  }
  else
  {
    gint maxvol = 0;
    ret = mix_audio_get_max_vol(mix, &maxvol);

    if (MIX_SUCCEEDED(ret))
    {
      if (type == MIX_VOL_PERCENT)
        *currvol = (maxvol!=0)?((vol.volume * 100) / maxvol):0;
      else
        *currvol = vol.volume;
    }
  }

  _UNLOCK(&mix->controllock);

  return ret;
}

MIX_RESULT mix_audio_get_mute_default(MixAudio *mix, gboolean* muted)
{
    // API version 0.79. 1/22/2009
    MIX_RESULT ret = MIX_RESULT_SUCCESS;
    if (G_UNLIKELY(!mix)) return MIX_RESULT_NULL_PTR;
    if (muted == NULL) return MIX_RESULT_NULL_PTR;

    _LOCK(&mix->controllock);

    // ret should be SUCCESS when this sequence starts...
    if (mix->codecMode != MIX_CODING_DECODE) 
        ret = MIX_RESULT_WRONGMODE;
    else if (mix->state != MIX_STATE_CONFIGURED) 
        ret = MIX_RESULT_NOT_CONFIGURED;
    else if (MIX_ACP_DECODEMODE(mix->audioconfigparams) == MIX_DECODE_DECODERETURN)
        ret = MIX_RESULT_WRONGMODE;

    // Now check if we need to exit.
    if (!MIX_SUCCEEDED(ret)) _UNLOCK_RETURN(&mix->controllock, ret);

    *muted = mix->stream_muted;

    _UNLOCK(&mix->controllock);

    return ret;
}

MIX_RESULT mix_audio_set_mute_default(MixAudio *mix, gboolean mute)
{
    // API version 0.79. 1/22/2009
    MIX_RESULT ret = MIX_RESULT_SUCCESS;
    int retVal = 0;

    if (G_UNLIKELY(!mix)) return MIX_RESULT_NULL_PTR;

    struct snd_sst_mute m = { 0 };

    if (mute) m.mute = 1;
    else m.mute = 0;

    _LOCK(&mix->controllock);

    // ret should be SUCCESS when this sequence starts...
    if (mix->codecMode != MIX_CODING_DECODE) 
        ret = MIX_RESULT_WRONGMODE;
    else if (mix->state != MIX_STATE_CONFIGURED) 
        ret = MIX_RESULT_NOT_CONFIGURED;
    else if (MIX_ACP_DECODEMODE(mix->audioconfigparams) == MIX_DECODE_DECODERETURN)
        ret = MIX_RESULT_WRONGMODE;

    // Now check if we need to exit.
    if (!MIX_SUCCEEDED(ret)) _UNLOCK_RETURN(&mix->controllock, ret);

    m.stream_id = mix->streamID;

#ifdef LPESTUB
  // Not calling.
#else
    g_debug("Calling _MUTE. fd=%d", mix->fileDescriptor);
    retVal = ioctl(mix->fileDescriptor, SNDRV_SST_MUTE, &m);
    g_debug("_MUTE returned %d", retVal);
#endif

    if (retVal != 0)
    {
        ret = MIX_RESULT_SYSTEM_ERRNO; 
        g_debug("_MUTE failed. Error(0x%08x): %s", errno, strerror(errno));
        mix_audio_debug_dump(mix);
    }
    else
    {
        mix->stream_muted = mute;
    }

  _UNLOCK(&mix->controllock);

  return ret;
}

MIX_RESULT mix_audio_pause_default(MixAudio *mix)
{
    // API version 0.79. 1/22/2009
    MIX_RESULT ret = MIX_RESULT_SUCCESS;
    int retVal = 0;

    if (G_UNLIKELY(mix==NULL)) return MIX_RESULT_NULL_PTR;

    _LOCK(&mix->controllock);

    // ret should be SUCCESS when this sequence starts...
    if (mix->codecMode != MIX_CODING_DECODE) 
        ret = MIX_RESULT_WRONGMODE;
    else if (mix->state != MIX_STATE_CONFIGURED) 
        ret = MIX_RESULT_NOT_CONFIGURED;
    else if (MIX_ACP_DECODEMODE(mix->audioconfigparams) == MIX_DECODE_DECODERETURN)
        ret = MIX_RESULT_WRONGMODE;
    else if ((mix->streamState != MIX_STREAM_PLAYING) && (mix->streamState != MIX_STREAM_DRAINING))
        ret = MIX_RESULT_WRONG_STATE;

    // Now check if we need to exit.
    if (!MIX_SUCCEEDED(ret)) _UNLOCK_RETURN(&mix->controllock, ret);

    // If stream is paused, return success.
    if ((mix->streamState == MIX_STREAM_PAUSED) ||
        (mix->streamState == MIX_STREAM_PAUSED_DRAINING))
    {
        g_debug("Stream already paused.");
        _UNLOCK_RETURN(&mix->controllock, MIX_RESULT_SUCCESS);
    }

#ifdef LPESTUB
  // Not calling
#else
    g_debug("Calling SNDRV_SST_STREAM_PAUSE. fd=%d", mix->fileDescriptor);
    retVal = ioctl(mix->fileDescriptor, SNDRV_SST_STREAM_PAUSE);
    g_debug("_PAUSE returned %d", retVal);
#endif

  if (retVal != 0)
  {
    if (mix->streamState == MIX_STREAM_DRAINING)
    {
      // if stream state has been DRAINING, DRAIN could become successful during the PAUSE call, but not yet have chance to update streamState since we now hold the control lock. 
      // In this case, the mix_streamState becomes out-of-sync with the actual playback state. PAUSE failed due to stream already STOPPED but mix->streamState remains at "DRAINING"
      // On the other hand, we can't let DRAIN hold the lock the entire time.
      // We would not know if we fail PAUSE due to DRAINING, or a valid reason.
      // DRAINING is not likely problem for resume, as long as the PAUSED state is set when stream is really PAUSED.
      ret = MIX_RESULT_NEED_RETRY;
      g_warning("PAUSE failed while DRAINING. Draining could be just completed. Retry needed.");
    }
    else
    {
      ret = MIX_RESULT_SYSTEM_ERRNO; 
      g_debug("_PAUSE failed. Error:0x%08x", errno);
      mix_audio_debug_dump(mix);
    }
  }
  else
  {
    if (mix->streamState == MIX_STREAM_DRAINING)
      mix->streamState = MIX_STREAM_PAUSED_DRAINING;
    else
      mix->streamState = MIX_STREAM_PAUSED;

#ifdef AUDIO_MANAGER
    if (lpe_stream_notify_pause(mix->amStreamID) != 0) 
    {
        ret = MIX_RESULT_AM_NOTIFY_PAUSE_FAIL;
    }
#endif
  }

  _UNLOCK(&mix->controllock);

#ifdef LPESTUB
  if (MIX_SUCCEEDED(ret))
  {
    GTimeVal tval = {0};
    g_get_current_time(&tval);
    guint64 ts = 1000ll * tval.tv_sec + tval.tv_usec / 1000;
    mix->ts_elapsed += ts - mix->ts_last;
    mix->ts_last = 0;
  }
#endif
  return ret;
}

MIX_RESULT mix_audio_resume_default(MixAudio *mix)
{
    // API version 0.79. 1/22/2009
    MIX_RESULT ret = MIX_RESULT_SUCCESS;
    int retVal = 0;

    if (G_UNLIKELY(!mix)) return MIX_RESULT_NULL_PTR;

    _LOCK(&mix->controllock);

    // ret should be SUCCESS when this sequence starts...
    if (mix->codecMode != MIX_CODING_DECODE) 
        ret = MIX_RESULT_WRONGMODE;
    else if (mix->state != MIX_STATE_CONFIGURED) 
        ret = MIX_RESULT_NOT_CONFIGURED;
    else if (MIX_ACP_DECODEMODE(mix->audioconfigparams) == MIX_DECODE_DECODERETURN)
        ret = MIX_RESULT_WRONGMODE;
    else if ((mix->streamState != MIX_STREAM_PAUSED) && (mix->streamState != MIX_STREAM_PAUSED_DRAINING))
        ret = MIX_RESULT_WRONG_STATE;

    // Now check if we need to exit.
    if (!MIX_SUCCEEDED(ret)) _UNLOCK_RETURN(&mix->controllock, ret);

    // If stream is paused, return success.
    if ((mix->streamState == MIX_STREAM_PLAYING) ||
        (mix->streamState == MIX_STREAM_DRAINING))
    {
        g_debug("Stream already playing.");
        _UNLOCK_RETURN(&mix->controllock, MIX_RESULT_SUCCESS);
    }

#ifdef LPESTUB
  // Not calling
#else
  g_debug("Calling SNDRV_SST_STREAM_RESUME. fd=%d", mix->fileDescriptor);
  retVal = ioctl(mix->fileDescriptor, SNDRV_SST_STREAM_RESUME);
  g_debug("_STREAM_RESUME returned %d", retVal);
#endif

  if (retVal != 0)
  {
    ret = MIX_RESULT_SYSTEM_ERRNO; 
    g_debug("_PAUSE failed. Error:0x%08x", errno);
    mix_audio_debug_dump(mix);
  }
  else 
  {
    if (mix->streamState == MIX_STREAM_PAUSED_DRAINING)
      mix->streamState = MIX_STREAM_DRAINING;
    else
      mix->streamState = MIX_STREAM_PLAYING;
 
#ifdef AUDIO_MANAGER
    if (lpe_stream_notify_resume(mix->amStreamID) != 0) {
      ret = MIX_RESULT_AM_NOTIFY_RESUME_FAIL;
    }
#endif
  }

  _UNLOCK(&mix->controllock);

#ifdef LPESTUB
  if (MIX_SUCCEEDED(ret))
  {
    GTimeVal tval = {0};
    g_get_current_time(&tval);
    guint64 ts = 1000ll * tval.tv_sec + tval.tv_usec / 1000;
    mix->ts_last = ts;
  }
#endif

  return ret;
}

MIX_RESULT mix_audio_set_volume_default(MixAudio *mix, gint currvol, MixVolType type, gulong msecs, MixVolRamp ramptype)
{
    // API version 0.79. 1/22/2009
    MIX_RESULT ret = MIX_RESULT_SUCCESS;
    int retVal = 0;
    struct snd_sst_vol vol = {
        .ramp_duration = msecs,
        .ramp_type = ramptype
    };
    
    if (G_UNLIKELY(mix == NULL)) return MIX_RESULT_NULL_PTR;
    if ((type != MIX_VOL_PERCENT) && (type != MIX_VOL_DECIBELS)) return MIX_RESULT_INVALID_PARAM;
    
    _LOCK(&mix->controllock);

    // ret should be SUCCESS when this sequence starts...
    if (mix->codecMode != MIX_CODING_DECODE) 
        ret = MIX_RESULT_WRONGMODE;
    else if (mix->state != MIX_STATE_CONFIGURED) 
        ret = MIX_RESULT_NOT_CONFIGURED;
    else if (MIX_ACP_DECODEMODE(mix->audioconfigparams) == MIX_DECODE_DECODERETURN)
        ret = MIX_RESULT_WRONGMODE;

    // Now check if we need to exit.
    if (!MIX_SUCCEEDED(ret)) _UNLOCK_RETURN(&mix->controllock, ret);

    vol.stream_id = mix->streamID;

    if (type == MIX_VOL_DECIBELS)
    {
        vol.volume = currvol;
    }
    else
    {
        gint maxvol = 0;
        ret = mix_audio_get_max_vol(mix, &maxvol);
        
        if (!maxvol)
        g_critical("Max Vol is 0!");
    
        if (MIX_SUCCEEDED(ret))
        {
        vol.volume = currvol * maxvol / 100;
        }
    }


#ifdef LPESTUB
  // Not calling
#else
  g_debug("calling _SET_VOL vol=%d", vol.volume);
  retVal = ioctl(mix->fileDescriptor, SNDRV_SST_SET_VOL, &vol);
  g_debug("_SET_VOL returned %d", retVal);
#endif

  if (retVal != 0)
  {
    ret = MIX_RESULT_SYSTEM_ERRNO; 
    g_debug("_SET_VOL failed. Error(0x%08x): %s", errno, strerror(errno));
    mix_audio_debug_dump(mix);
  }

  _UNLOCK(&mix->controllock);

  return ret;
}

MIX_RESULT mix_audio_FW_INFO(MixAudio *mix)
{
  if (G_UNLIKELY(!mix)) return MIX_RESULT_NULL_PTR;

  MIX_RESULT ret = MIX_RESULT_SUCCESS;

  _LOCK(&mix->controllock);

  // This call always get the fw info.
  int retVal = 0;

#ifdef LPESTUB
  // Not calling.
#else
  g_debug("calling SNDRV_SST_FW_INFO fd=%d", mix->fileDescriptor);
  retVal = ioctl(mix->fileDescriptor, SNDRV_SST_FW_INFO, &cur_FW_INFO);
  g_debug("SNDRV_SST_FW_INFO returned %d", retVal);
#endif

  if (!retVal)
  {
    has_FW_INFO = TRUE;
  }
  else
  {
    ret = MIX_RESULT_SYSTEM_ERRNO; 
    g_debug("_FW_INFO failed. Error:0x%08x", errno);
    mix_audio_debug_dump(mix);
  }

  _UNLOCK(&mix->controllock);

  return ret;
}


static MIX_RESULT mix_audio_SST_writev(MixAudio *mix, const MixIOVec *iovin, gint iovincnt, guint64 *insize)
{
  MIX_RESULT ret = MIX_RESULT_SUCCESS;

  if (G_UNLIKELY(!mix)) return MIX_RESULT_NULL_PTR;

/*
  definition of "struct iovec" used by writev:
  struct iovec {
      void  *iov_base;
      size_t iov_len;
  };
*/

  if (!mix) return MIX_RESULT_NULL_PTR;

  size_t total_bytes = 0;
  // NOTE: we may want to find a way to avoid this copy.
  struct iovec *in = (struct iovec*)g_alloca(sizeof(struct iovec) * iovincnt);
  if (!in) return MIX_RESULT_NO_MEMORY;

  int i;
  for (i=0;i<iovincnt;i++)
  {
    in[i].iov_base = (void*)iovin[i].data;
    in[i].iov_len = (size_t)iovin[i].data_size;
    total_bytes += in[i].iov_len;
  }

  ssize_t written = 0;

#ifdef LPESTUB
  gulong wait_time = 0; //wait time in second.
  if (MIX_ACP_BITRATE(mix->audioconfigparams) > 0)
  {
    wait_time = total_bytes*8*1000*1000/MIX_ACP_BITRATE(mix->audioconfigparams);
    // g_debug("To wait %lu usec for writev() to simulate blocking\n", wait_time);
  }
  GTimer *timer = g_timer_new();
  g_timer_start(timer);

  g_debug("calling writev(fd=%d)", mix->fileDescriptor);
  written = writev(mix->fileDescriptor, in, iovincnt);
  if (written >= 0) mix->bytes_written += written;
  g_debug("writev() returned %d. Total %" G_GUINT64_FORMAT, written, mix->bytes_written);
  /* Now since writing to file rarely block, we put timestamp there to block.*/
  g_timer_stop(timer);
  gulong elapsed = 0;
  g_timer_elapsed(timer, &elapsed);
  g_timer_destroy(timer);
  // g_debug("writev() returned in %lu usec\n", elapsed);
  if ((MIX_ACP_BITRATE(mix->audioconfigparams) > 0) && (wait_time > elapsed))
  {
    wait_time -= elapsed;
    g_usleep(wait_time);
  }
#else
  g_debug("calling writev(fd=%d) with %d", mix->fileDescriptor, total_bytes);
  written = writev(mix->fileDescriptor, in, iovincnt);
  if (written > 0) mix->bytes_written += written;
  g_debug("writev() returned %d. Total %" G_GUINT64_FORMAT, written, mix->bytes_written);
#endif

  if (written < 0)
  {
    ret = MIX_RESULT_SYSTEM_ERRNO;
    g_debug("writev() failed. Error:0x%08x", errno);
  }
  else
  {
    // guranttee written is positive value before sign extending it.
    if (insize) *insize = (guint64)written;
    if (written != total_bytes)
    {
        g_warning("writev() wrote only %d out of %d", written, total_bytes);
    }
  }

  return ret;
}

static MIX_RESULT mix_audio_SST_STREAM_DECODE(MixAudio *mix, const MixIOVec *iovin, gint iovincnt, guint64 *insize, MixIOVec *iovout, gint iovoutcnt)
{
  MIX_RESULT ret = MIX_RESULT_SUCCESS;
  int retVal = 0;

  if (G_UNLIKELY(!mix)) return MIX_RESULT_NULL_PTR;

  if ((iovout == NULL) || (iovoutcnt <= 0))
  {
    g_critical("Wrong mode. Please report a bug...");
    return MIX_RESULT_NULL_PTR;
  }
  
  g_message("Input entries=%d. Output entries=%d", iovincnt, iovoutcnt);

  struct snd_sst_buff_entry *ientries = NULL; 
  struct snd_sst_buff_entry *oentries = NULL;

  ientries = (struct snd_sst_buff_entry*)g_alloca(sizeof(struct snd_sst_buff_entry) * iovincnt);
  oentries = (struct snd_sst_buff_entry*)g_alloca(sizeof(struct snd_sst_buff_entry) * iovoutcnt);

  if (!ientries || !oentries) return MIX_RESULT_NO_MEMORY;

  struct snd_sst_dbufs dbufs = {0};

  struct snd_sst_buffs ibuf = {0};
  struct snd_sst_buffs obuf = {0};

  ibuf.entries = iovincnt;
  ibuf.type = SST_BUF_USER;
  ibuf.buff_entry = ientries;

  obuf.entries = iovoutcnt;
  obuf.type = SST_BUF_USER;
  obuf.buff_entry = oentries;

  dbufs.ibufs = &ibuf;
  dbufs.obufs = &obuf;

  int i = 0;
  for (i=0;i<iovincnt;i++)
  {
    ientries[i].size = (unsigned long)iovin[i].data_size;
    ientries[i].buffer = (void *)iovin[i].data;
    g_debug("Creating in entry#%d, size=%u", i, ientries[i].size);
  }

  for (i=0;i<iovoutcnt;i++)
  {
    oentries[i].size = (unsigned long)iovout[i].buffer_size;
    oentries[i].buffer = (void *)iovout[i].data;
    g_debug("Creating out entry#%d, size=%u", i, oentries[i].size);
  }

#ifdef LPESTUB
  size_t total_bytes = 0;
  // NOTE: we may want to find a way to avoid this copy.
  struct iovec *in = (struct iovec*)g_alloca(sizeof(struct iovec) * iovincnt);
  if (iovincnt>1)
  {
    for (i=0;i<iovincnt-1;i++)
    {
      in[i].iov_base = (void*)iovin[i].data;
      in[i].iov_len = (size_t)iovin[i].data_size;
      total_bytes += in[i].iov_len;
    }
    in[i].iov_base = (void*)iovin[i].data;
    in[i].iov_len = (size_t)iovin[i].data_size/2;
    total_bytes += in[i].iov_len;
  }
  else
  {
    for (i=0;i<iovincnt;i++)
    {
      in[i].iov_base = (void*)iovin[i].data;
      in[i].iov_len = (size_t)iovin[i].data_size;
      total_bytes += in[i].iov_len;
    }
  }
  ssize_t written = 0;

  g_debug("calling stub STREAM_DECODE (writev) (fd=%d)", mix->fileDescriptor);
  written = writev(mix->fileDescriptor, in, iovincnt);
  if (written >= 0) 
  { 
    mix->bytes_written += written;
    dbufs.output_bytes_produced = written;
    dbufs.input_bytes_consumed = written;
  }
  g_debug("stub STREAM_DECODE (writev) returned %d. Total %" G_GUINT64_FORMAT, written, mix->bytes_written);
#else
  g_debug("calling SNDRV_SST_STREAM_DECODE fd=%d", mix->fileDescriptor);
  retVal = ioctl(mix->fileDescriptor, SNDRV_SST_STREAM_DECODE, &dbufs);
  g_debug("SNDRV_SST_STREAM_DECODE returned %d", retVal);  
#endif

  if (retVal)
  {
    ret = MIX_RESULT_SYSTEM_ERRNO; 
    g_debug("_STREAM_DECODE failed. Error:0x%08x", errno);
    mix_audio_debug_dump(mix);
  }
  else
  {
    if (insize) *insize = dbufs.input_bytes_consumed;
    //if (outsize) *outsize = dbufs.output_bytes_produced;
    unsigned long long produced = dbufs.output_bytes_produced;
    int i;
    for (i=0;i<iovoutcnt;i++)
    {
        if (produced != 0)
        {
            if (produced > iovout[i].buffer_size)
            {
                iovout[i].data_size = iovout[i].buffer_size;
                produced -= iovout[i].data_size;
            }
            else
            {
                iovout[i].data_size = produced;
                produced = 0;
            }
        }
        else
        {
            iovout[i].data_size = 0;
        }
    }
    produced = 0;
    for (i=0;i<iovoutcnt;i++)
    {
        produced += iovout[i].data_size;
    }
    g_message("consumed=%" G_GUINT64_FORMAT " produced=%" G_GUINT64_FORMAT "(%" G_GUINT64_FORMAT ")", dbufs.input_bytes_consumed, dbufs.output_bytes_produced, produced);

  }

  return ret;
}

// Starting interface
//MIX_RESULT mix_audio_get_version(guint* major, guint *minor);

MIX_RESULT mix_audio_initialize(MixAudio *mix, MixCodecMode mode, MixAudioInitParams *aip, MixDrmParams *drminitparams)
{
  if (G_UNLIKELY(!mix)) return MIX_RESULT_NULL_PTR;

  MixAudioClass *klass = MIX_AUDIO_GET_CLASS(mix);

  mix_log(MIX_AUDIO_COMP, MIX_LOG_LEVEL_VERBOSE, "mix_audio_initialize\n");

  if (!klass->initialize)
    return MIX_RESULT_FAIL;     // TODO: add more descriptive error

#ifdef AUDIO_MANAGER
  if (dbus_init() < 0) {
    mix_log(MIX_AUDIO_COMP, MIX_LOG_LEVEL_ERROR, "Failed to connect to dbus\n");
// commented out, gracefully exit right now
//    return MIX_RESULT_FAIL;     // TODO: add more descriptive error
  }
#endif

  return klass->initialize(mix, mode, aip, drminitparams);
}

MIX_RESULT mix_audio_configure(MixAudio *mix, MixAudioConfigParams *audioconfigparams, MixDrmParams *drmparams)
{
  if (G_UNLIKELY(!mix)) return MIX_RESULT_NULL_PTR;

  MixAudioClass *klass = MIX_AUDIO_GET_CLASS(mix);

  if (!klass->configure)
	return MIX_RESULT_FAIL;
  
  return klass->configure(mix, audioconfigparams, drmparams);
}

MIX_RESULT mix_audio_decode(MixAudio *mix, const MixIOVec *iovin, gint iovincnt, guint64 *insize, MixIOVec *iovout, gint iovoutcnt)
{
  if (G_UNLIKELY(!mix)) return MIX_RESULT_NULL_PTR;

  MixAudioClass *klass = MIX_AUDIO_GET_CLASS(mix);

  if (!klass->decode)
    return MIX_RESULT_FAIL;

  return klass->decode(mix, iovin, iovincnt, insize, iovout, iovoutcnt);
}

MIX_RESULT mix_audio_capture_encode(MixAudio *mix, MixIOVec *iovout, gint iovoutcnt)
{
  if (G_UNLIKELY(!mix)) return MIX_RESULT_NULL_PTR;

  MixAudioClass *klass = MIX_AUDIO_GET_CLASS(mix);

  if (!klass->capture_encode)
    return MIX_RESULT_FAIL;

  return klass->capture_encode(mix, iovout, iovoutcnt);
}

MIX_RESULT mix_audio_start(MixAudio *mix)
{
  if (G_UNLIKELY(!mix)) return MIX_RESULT_NULL_PTR;

  MixAudioClass *klass = MIX_AUDIO_GET_CLASS(mix);

  if (!klass->start)
    return MIX_RESULT_FAIL;

  return klass->start(mix);
}

MIX_RESULT mix_audio_stop_drop(MixAudio *mix)
{  
  if (G_UNLIKELY(!mix)) return MIX_RESULT_NULL_PTR;

  MixAudioClass *klass = MIX_AUDIO_GET_CLASS(mix);

  if (!klass->stop_drop)
    return MIX_RESULT_FAIL;

  return klass->stop_drop(mix);
}

MIX_RESULT mix_audio_stop_drain(MixAudio *mix)
{
  if (G_UNLIKELY(!mix)) return MIX_RESULT_NULL_PTR;

  MixAudioClass *klass = MIX_AUDIO_GET_CLASS(mix);

  if (!klass->stop_drain)
    return MIX_RESULT_FAIL;

  return klass->stop_drain(mix);
}

MIX_RESULT mix_audio_pause(MixAudio *mix)
{
  if (G_UNLIKELY(!mix)) return MIX_RESULT_NULL_PTR;

  MixAudioClass *klass = MIX_AUDIO_GET_CLASS(mix);

  if (!klass->pause)
    return MIX_RESULT_FAIL;

  return klass->pause(mix);
}

MIX_RESULT mix_audio_resume(MixAudio *mix)
{
  if (G_UNLIKELY(!mix)) return MIX_RESULT_NULL_PTR;

  MixAudioClass *klass = MIX_AUDIO_GET_CLASS(mix);

  if (!klass->resume)
    return MIX_RESULT_FAIL;

  return klass->resume(mix);
}

MIX_RESULT mix_audio_get_timestamp(MixAudio *mix, guint64 *msecs)
{
  if (G_UNLIKELY(!mix)) return MIX_RESULT_NULL_PTR;

  MixAudioClass *klass = MIX_AUDIO_GET_CLASS(mix);

  if (!klass->get_timestamp)
    return MIX_RESULT_FAIL;

  return klass->get_timestamp(mix, msecs);
}

MIX_RESULT mix_audio_get_mute(MixAudio *mix, gboolean* muted)
{
  if (G_UNLIKELY(!mix)) return MIX_RESULT_NULL_PTR;

  MixAudioClass *klass = MIX_AUDIO_GET_CLASS(mix);

  if (!klass->get_mute)
    return MIX_RESULT_FAIL;

  return klass->get_mute(mix, muted);
}

MIX_RESULT mix_audio_set_mute(MixAudio *mix, gboolean mute)
{
  if (G_UNLIKELY(!mix)) return MIX_RESULT_NULL_PTR;

  MixAudioClass *klass = MIX_AUDIO_GET_CLASS(mix);

  if (!klass->set_mute)
    return MIX_RESULT_FAIL;

  return klass->set_mute(mix, mute);
}

MIX_RESULT mix_audio_get_max_vol(MixAudio *mix, gint *maxvol)
{
  if (G_UNLIKELY(!mix)) return MIX_RESULT_NULL_PTR;

  MixAudioClass *klass = MIX_AUDIO_GET_CLASS(mix);

  if (!klass->get_max_vol)
    return MIX_RESULT_FAIL;

  return klass->get_max_vol(mix, maxvol);
}

MIX_RESULT mix_audio_get_min_vol(MixAudio *mix, gint *minvol)
{
  if (G_UNLIKELY(!mix)) return MIX_RESULT_NULL_PTR;

  MixAudioClass *klass = MIX_AUDIO_GET_CLASS(mix);

  if (!klass->get_min_vol)
    return MIX_RESULT_FAIL;

  return klass->get_min_vol(mix, minvol);
}

MIX_RESULT mix_audio_get_volume(MixAudio *mix, gint *currvol, MixVolType type)
{
  if (G_UNLIKELY(!mix)) return MIX_RESULT_NULL_PTR;

  MixAudioClass *klass = MIX_AUDIO_GET_CLASS(mix);

  if (!klass->get_volume)
    return MIX_RESULT_FAIL;

  return klass->get_volume(mix, currvol, type);
}

MIX_RESULT mix_audio_set_volume(MixAudio *mix, gint currvol, MixVolType type, gulong msecs, MixVolRamp ramptype)
{
  if (G_UNLIKELY(!mix)) return MIX_RESULT_NULL_PTR;

  MixAudioClass *klass = MIX_AUDIO_GET_CLASS(mix);

  if (!klass->set_volume)
    return MIX_RESULT_FAIL;

  return klass->set_volume(mix, currvol, type, msecs, ramptype);
}

MIX_RESULT mix_audio_deinitialize(MixAudio *mix)
{
  if (G_UNLIKELY(!mix)) return MIX_RESULT_NULL_PTR;

  MixAudioClass *klass = MIX_AUDIO_GET_CLASS(mix);

  if (!klass->deinitialize)
    return MIX_RESULT_FAIL;

  return klass->deinitialize(mix);
}

MIX_RESULT mix_audio_get_stream_state(MixAudio *mix, MixStreamState *streamState)
{
  if (G_UNLIKELY(!mix)) return MIX_RESULT_NULL_PTR;

  MixAudioClass *klass = MIX_AUDIO_GET_CLASS(mix);

  if (!klass->get_stream_state)
    return MIX_RESULT_FAIL;

  return klass->get_stream_state(mix, streamState);
}

MIX_RESULT mix_audio_get_state(MixAudio *mix, MixState *state)
{
  if (G_UNLIKELY(!mix)) return MIX_RESULT_NULL_PTR;

  MixAudioClass *klass = MIX_AUDIO_GET_CLASS(mix);

  if (!klass->get_state)
    return MIX_RESULT_FAIL;

  return klass->get_state(mix, state);
}

MIX_RESULT mix_audio_is_am_available(MixAudioManager am, gboolean *avail)
{
  MIX_RESULT ret = MIX_RESULT_SUCCESS;

  if (avail)
    *avail = FALSE;
  else 
    ret = MIX_RESULT_NULL_PTR;

  return ret;
}

const gchar* dbgstr_UNKNOWN="UNKNOWN";

static const gchar* _mix_stream_state_get_name (MixStreamState s)
{
  static const gchar *MixStreamStateNames[] = {
    "MIX_STREAM_NULL",
    "MIX_STREAM_STOPPED",
    "MIX_STREAM_PLAYING",
    "MIX_STREAM_PAUSED",
    "MIX_STREAM_DRAINING",
    "MIX_STREAM_PAUSED_DRAINING",
    "MIX_STREAM_INTERNAL_LAST"
  };

  const gchar *ret = dbgstr_UNKNOWN;

  if (s < sizeof(MixStreamStateNames)/sizeof(MixStreamStateNames[0]))
  {
    ret = MixStreamStateNames[s];
  }

  return ret;
}

static const gchar* _mix_state_get_name(MixState s)
{
  static const gchar* MixStateNames[] = {
    "MIX_STATE_NULL",
    "MIX_STATE_UNINITIALIZED",
    "MIX_STATE_INITIALIZED",
    "MIX_STATE_CONFIGURED",
    "MIX_STATE_LAST"
  };

  const gchar *ret = dbgstr_UNKNOWN;

  if (s < sizeof(MixStateNames)/sizeof(MixStateNames[0]))
  {
    ret = MixStateNames[s];
  }
  
  return ret;
}

static const gchar* _mix_codec_mode_get_name(MixCodecMode s)
{
  static const gchar* MixCodecModeNames[] = {
    "MIX_CODING_INVALID",
    "MIX_CODING_ENCODE",
    "MIX_CODING_DECODE",
    "MIX_CODING_LAST"
  };

  const gchar *ret = dbgstr_UNKNOWN;

  if (s < sizeof(MixCodecModeNames)/sizeof(MixCodecModeNames[0]))
  {
    ret = MixCodecModeNames[s];
  }
  
  return ret;
}

static const gchar* _mix_device_state_get_name(MixDeviceState s)
{
  static const gchar* MixDeviceStateNames[] = {
    "MIX_AUDIO_DEV_CLOSED",
    "MIX_AUDIO_DEV_OPENED",
    "MIX_AUDIO_DEV_ALLOCATED"
  };

  const gchar *ret = dbgstr_UNKNOWN;

  if (s < sizeof(MixDeviceStateNames)/sizeof(MixDeviceStateNames[0]))
  {
    ret = MixDeviceStateNames[s];
  }
  
  return ret;
}

void mix_audio_debug_dump(MixAudio *mix)
{
  const gchar* prefix="MixAudio:";

  if (!MIX_IS_AUDIO(mix))
  {
    g_debug("%s Not a valid MixAudio object.", prefix);
    return;
  }

  g_debug("%s streamState(%s)", prefix, _mix_stream_state_get_name(mix->streamState));
  g_debug("%s encoding(%s)", prefix, mix->encoding?mix->encoding:dbgstr_UNKNOWN);
  g_debug("%s fileDescriptor(%d)", prefix, mix->fileDescriptor);
  g_debug("%s state(%s)", prefix, _mix_state_get_name(mix->state));
  g_debug("%s codecMode(%s)", prefix, _mix_codec_mode_get_name(mix->codecMode));

  // Private members
  g_debug("%s streamID(%d)", prefix, mix->streamID);
  //GStaticRecMutex streamlock; // lock that must be acquired to invoke stream method.
  //GStaticRecMutex controllock; // lock that must be acquired to call control function.
  if (MIX_IS_AUDIOCONFIGPARAMS(mix->audioconfigparams))
  {
    // TODO: print audioconfigparams
  }
  else
  {
    g_debug("%s audioconfigparams(NULL)", prefix);
  }
  
  g_debug("%s deviceState(%s)", prefix, _mix_device_state_get_name(mix->deviceState));

  g_debug("%s ts_last(%" G_GUINT64_FORMAT ")", prefix, mix->ts_last);
  g_debug("%s ts_elapsed(%" G_GUINT64_FORMAT ")", prefix, mix->ts_elapsed);
  g_debug("%s bytes_written(%" G_GUINT64_FORMAT ")", prefix, mix->bytes_written);

  return;
}

MIX_RESULT mix_audio_get_config(MixAudio *mix, MixAudioConfigParams **audioconfigparams)
{
  if (G_UNLIKELY(!mix)) return MIX_RESULT_NULL_PTR;

  MixAudioClass *klass = MIX_AUDIO_GET_CLASS(mix);

  if (!klass->get_config)
    return MIX_RESULT_FAIL;

  return klass->get_config(mix, audioconfigparams);
}

MIX_RESULT mix_audio_get_config_default(MixAudio *mix, MixAudioConfigParams **audioconfigparams)
{
  // API version 0.79. 1/22/2009
  MIX_RESULT ret = MIX_RESULT_SUCCESS;
  struct snd_sst_get_stream_params stream_params = {{0}};
  MixAudioConfigParams *p = NULL;
  int retVal = 0;

  if (G_UNLIKELY(!mix || !audioconfigparams)) return MIX_RESULT_NULL_PTR;

  _LOCK(&mix->controllock);

  // _get_config is called only it is configured.
  if (mix->state != MIX_STATE_CONFIGURED) _UNLOCK_RETURN(&mix->controllock, MIX_RESULT_NOT_CONFIGURED);

#ifdef LPESTUB
#else
  // Check only if we are initialized.
    g_debug("Calling SNDRV_SST_STREAM_GET_PARAMS. fd=%d", mix->fileDescriptor);
    retVal = ioctl(mix->fileDescriptor, SNDRV_SST_STREAM_GET_PARAMS, &stream_params);
    g_debug("_GET_PARAMS returned %d", retVal);
#endif

  if (retVal != 0)
  {
      ret = MIX_RESULT_SYSTEM_ERRNO; 
      g_debug("Failed to GET_PARAMS. errno:0x%08x. %s\n", errno, strerror(errno));
  }
  else
  {
      p = mix_sst_params_to_acp(&stream_params);
      if (p != NULL)
      {
         if (mix->audioconfigparams != NULL)
         {
            // Complete the return structure with fields that are not returned from the SST.
            MIX_ACP_DECODEMODE(p) = MIX_ACP_DECODEMODE(mix->audioconfigparams);

            gchar *sn = mix_acp_get_streamname(mix->audioconfigparams);
            if (sn != NULL)
            {
                if (!MIX_SUCCEEDED(mix_acp_set_streamname(MIX_AUDIOCONFIGPARAMS(p), sn)))
                {
                    g_error("Failed to set stream name!");
                }
                g_free(sn);
            }

            if (!MIX_SUCCEEDED(mix_acp_set_audio_manager(MIX_AUDIOCONFIGPARAMS(p), mix_acp_get_audio_manager(mix->audioconfigparams))))
            {
                g_error("Failed to set audio manager!");
            }

            // remove existingi copy of audioconfigparams and copy this return struct.
            mix_acp_unref(mix->audioconfigparams);
            mix->audioconfigparams = NULL;
         }

         mix->audioconfigparams = MIX_AUDIOCONFIGPARAMS(mix_params_dup(MIX_PARAMS(p)));
      }
      *audioconfigparams = p;
  }

  _UNLOCK(&mix->controllock);

  return ret;
}

MIX_RESULT mix_audio_get_bytes_decoded(MixAudio *mix, guint64 *byte)
{
  if (G_UNLIKELY(!mix)) return MIX_RESULT_NULL_PTR;

  MixAudioClass *klass = MIX_AUDIO_GET_CLASS(mix);

  if (!klass->get_bytes_decoded)
    return MIX_RESULT_FAIL;

  return klass->get_bytes_decoded(mix, byte);
}

MIX_RESULT mix_audio_get_bytes_decoded_default(MixAudio *mix, guint64 *bytecount)
{
    // API version 0.79. 1/22/2009
    MIX_RESULT ret = MIX_RESULT_SUCCESS;
    unsigned long long bytes_decoded = 0;
    int retVal = 0;

    if ((G_UNLIKELY(!mix)) || (bytecount == NULL)) return MIX_RESULT_NULL_PTR;

    _LOCK(&mix->controllock);

    // ret should be SUCCESS when this sequence starts...
    if (mix->codecMode != MIX_CODING_DECODE) 
        ret = MIX_RESULT_WRONGMODE;
    else if (mix->state != MIX_STATE_CONFIGURED) 
        ret = MIX_RESULT_NOT_CONFIGURED;
    else if (MIX_ACP_DECODEMODE(mix->audioconfigparams) == MIX_DECODE_DECODERETURN)
        ret = MIX_RESULT_WRONGMODE;

    // Now check if we need to exit.
    if (!MIX_SUCCEEDED(ret)) _UNLOCK_RETURN(&mix->controllock, ret);


#ifdef LPESTUB
#else
      g_debug("Calling _STREAM_BYTES_DECODED. fd=%d", mix->fileDescriptor);
      retVal = ioctl(mix->fileDescriptor, SNDRV_SST_STREAM_BYTES_DECODED, &bytes_decoded);
      g_debug("_STREAM_BYTES_DECODED returned %d. Bytes decoded=%" G_GUINT64_FORMAT, retVal, bytes_decoded);
#endif

      if (retVal != 0)
      {
        ret = MIX_RESULT_SYSTEM_ERRNO; 
        g_debug("_STREAM_BYTES_DECODED failed. Error(0x%08x): %s", errno, strerror(errno));
        mix_audio_debug_dump(mix);
      }
      else
      {
        *bytecount = bytes_decoded;
      }

  _UNLOCK(&mix->controllock);

  return ret;
}

static MIX_RESULT mix_audio_verify_params(MixAudio *mix, const MixAudioConfigParams *audioconfigparams)
{
    MIX_RESULT ret = MIX_RESULT_FAIL;

    if (G_UNLIKELY(!mix))
    {
        g_error("Null pointer passed to internal function!");
        return MIX_RESULT_NULL_PTR;
    }

    g_message("_verify_params() starts.");

    switch (mix->codecMode)
    {
        case MIX_CODING_DECODE:
        {
            if ((MIX_ACP_DECODEMODE(audioconfigparams) != MIX_DECODE_DIRECTRENDER) &&
            (MIX_ACP_DECODEMODE(audioconfigparams) != MIX_DECODE_DECODERETURN))
            {
                ret = MIX_RESULT_CONF_MISMATCH;
                goto verify_params_done;
            }
            break;
        }
        case MIX_CODING_ENCODE: 
        {
            if (!MIX_IS_AUDIOCONFIGPARAMSAAC(audioconfigparams))
            {
                ret = MIX_RESULT_CODEC_NOTSUPPORTED;
                goto verify_params_done;
            }
        }
        default:
        break;
    }

    ret = MIX_RESULT_SUCCESS;

verify_params_done:

    g_message("_verify_params() done. Returning 0x%08x.", ret);

    return ret;
}

static MIX_RESULT mix_audio_is_stream_changed(MixAudio *mix, MixAudioConfigParams *new_params, gboolean *pChanged)
{
    MIX_RESULT ret = MIX_RESULT_FAIL;

    gboolean changed = FALSE;
    MixParams *old_mask = NULL;
    MixParams *new_mask = NULL;

    g_message("is_stream_changed() starts");

    if ((mix == NULL) || (new_params == NULL) || (pChanged == NULL))
    {
        g_error("Null pointer passed to internal function!");
        return MIX_RESULT_NULL_PTR;
    }

    changed = (mix->audioconfigparams == NULL);
    if (changed) 
    {
        ret = MIX_RESULT_SUCCESS;
        goto stream_changed_done;
    }

    old_mask = mix_params_dup(MIX_PARAMS(mix->audioconfigparams));
    if (MIX_IS_AUDIOCONFIGPARAMS(old_mask))
    {
         MIX_ACP_DECODEMODE(old_mask) = MIX_DECODE_NULL;
         if (!MIX_SUCCEEDED(mix_acp_set_streamname(MIX_AUDIOCONFIGPARAMS(old_mask), NULL)))
         {
            g_error("Failed to set stream name!");
            goto stream_changed_done;
         }
         if (!MIX_SUCCEEDED(mix_acp_set_audio_manager(MIX_AUDIOCONFIGPARAMS(old_mask), MIX_AUDIOMANAGER_NONE)))
         {
            g_error("Failed to set audio manager!");
            goto stream_changed_done;
         }
    }
    else
    {
        g_error("Failed to duplicate param!");
        goto stream_changed_done;
    }

    new_mask = mix_params_dup(MIX_PARAMS(new_params));
    if (MIX_IS_AUDIOCONFIGPARAMS(new_mask))
    {
         MIX_ACP_DECODEMODE(new_mask) = MIX_DECODE_NULL;
         if (!MIX_SUCCEEDED(mix_acp_set_streamname(MIX_AUDIOCONFIGPARAMS(new_mask), NULL)))
         {
            g_error("Failed to set stream name!");
            goto stream_changed_done;
         }
         if (!MIX_SUCCEEDED(mix_acp_set_audio_manager(MIX_AUDIOCONFIGPARAMS(new_mask), MIX_AUDIOMANAGER_NONE)))
         {
            g_error("Failed to set audio manager!");
            goto stream_changed_done;
         }
    }
    else
    {
        g_error("Failed to duplicate param!");
        goto stream_changed_done;
    }


    changed = !mix_params_equal(old_mask, new_mask);

    ret = MIX_RESULT_SUCCESS;

stream_changed_done:

    if (old_mask)
    {
        mix_params_unref(old_mask);
        old_mask=NULL;
    }
    if (new_mask)
    {
        mix_params_unref(new_mask);
        new_mask=NULL;
    }

    *pChanged = changed;

    g_message("is_stream_changed() done returning 0x%08x, changed=%d", ret, changed);

    return ret;
}
