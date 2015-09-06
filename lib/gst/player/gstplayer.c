/* GStreamer
 *
 * Copyright (C) 2014-2015 Sebastian Dröge <sebastian@centricular.com>
 * Copyright (C) 2015 Brijesh Singh <brijesh.ksingh@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

/**
 * SECTION:gstplayer
 * @short_description: GStreamer Player API
 *
 */

/* TODO:
 *
 * - Equalizer
 * - Gapless playback
 * - Frame stepping
 * - Subtitle font, connection speed
 * - Deinterlacing
 * - Buffering control (-> progressive downloading)
 * - Playlist/queue object
 * - Custom video sink (e.g. embed in GL scene)
 *
 */

#include "gstplayer.h"
#include "gstplayer-media-info-private.h"

#include <gst/gst.h>
#include <gst/video/video.h>
#include <gst/video/colorbalance.h>
#include <gst/tag/tag.h>
#include <gst/pbutils/descriptions.h>

#include <string.h>

GST_DEBUG_CATEGORY_STATIC (gst_player_debug);
#define GST_CAT_DEFAULT gst_player_debug

#define DEFAULT_URI NULL
#define DEFAULT_POSITION GST_CLOCK_TIME_NONE
#define DEFAULT_DURATION GST_CLOCK_TIME_NONE
#define DEFAULT_VOLUME 1.0
#define DEFAULT_MUTE FALSE
#define DEFAULT_RATE 1.0
#define DEFAULT_POSITION_UPDATE_INTERVAL_MS 100

GQuark
gst_player_error_quark (void)
{
  static GQuark quark;

  if (!quark)
    quark = g_quark_from_static_string ("gst-player-error-quark");

  return quark;
}

enum
{
  PROP_0,
  PROP_VIDEO_RENDERER,
  PROP_SIGNAL_DISPATCHER,
  PROP_URI,
  PROP_SUBURI,
  PROP_POSITION,
  PROP_DURATION,
  PROP_MEDIA_INFO,
  PROP_CURRENT_AUDIO_TRACK,
  PROP_CURRENT_VIDEO_TRACK,
  PROP_CURRENT_SUBTITLE_TRACK,
  PROP_VOLUME,
  PROP_MUTE,
  PROP_RATE,
  PROP_PIPELINE,
  PROP_POSITION_UPDATE_INTERVAL,
  PROP_LAST
};

enum
{
  SIGNAL_POSITION_UPDATED,
  SIGNAL_DURATION_CHANGED,
  SIGNAL_STATE_CHANGED,
  SIGNAL_BUFFERING,
  SIGNAL_END_OF_STREAM,
  SIGNAL_ERROR,
  SIGNAL_WARNING,
  SIGNAL_VIDEO_DIMENSIONS_CHANGED,
  SIGNAL_MEDIA_INFO_UPDATED,
  SIGNAL_VOLUME_CHANGED,
  SIGNAL_MUTE_CHANGED,
  SIGNAL_SEEK_DONE,
  SIGNAL_LAST
};

enum
{
  GST_PLAY_FLAG_VIDEO = (1 << 0),
  GST_PLAY_FLAG_AUDIO = (1 << 1),
  GST_PLAY_FLAG_SUBTITLE = (1 << 2),
  GST_PLAY_FLAG_VIS = (1 << 3)
};

struct _GstPlayer
{
  GstObject parent;

  GstPlayerVideoRenderer *video_renderer;
  GstPlayerSignalDispatcher *signal_dispatcher;

  gchar *uri;
  gchar *suburi;

  GThread *thread;
  GMutex lock;
  GCond cond;
  GMainContext *context;
  GMainLoop *loop;

  GstElement *playbin;
  GstBus *bus;
  GstState target_state, current_state;
  gboolean is_live, is_eos;
  GSource *tick_source, *ready_timeout_source;

  gdouble rate;
  guint position_update_interval_ms;

  GstPlayerState app_state;
  gint buffering;

  GstTagList *global_tags;
  GstPlayerMediaInfo *media_info;

  GstElement *current_vis_element;

  /* Protected by lock */
  gboolean seek_pending;        /* Only set from main context */
  GstClockTime last_seek_time;  /* Only set from main context */
  GSource *seek_source;
  GstClockTime seek_position;
};

struct _GstPlayerClass
{
  GstObjectClass parent_class;
};

static GstElement
    * gst_player_video_renderer_create_video_sink (GstPlayerVideoRenderer *
    self, GstPlayer * player);

static void gst_player_signal_dispatcher_dispatch (GstPlayerSignalDispatcher *
    self, GstPlayer * player, void (*emitter) (gpointer data), gpointer data,
    GDestroyNotify destroy);

static GMutex vis_lock;
static GQueue vis_list = G_QUEUE_INIT;
static guint32 vis_cookie;

#define parent_class gst_player_parent_class
G_DEFINE_TYPE (GstPlayer, gst_player, GST_TYPE_OBJECT);

static guint signals[SIGNAL_LAST] = { 0, };
static GParamSpec *param_specs[PROP_LAST] = { NULL, };

static void gst_player_dispose (GObject * object);
static void gst_player_finalize (GObject * object);
static void gst_player_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_player_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);
static void gst_player_constructed (GObject * object);

static gpointer gst_player_main (gpointer data);

static void gst_player_seek_internal_locked (GstPlayer * self);
static gboolean gst_player_stop_internal (gpointer user_data);
static gboolean gst_player_pause_internal (gpointer user_data);
static gboolean gst_player_play_internal (gpointer user_data);
static gboolean gst_player_set_rate_internal (gpointer user_data);
static gboolean gst_player_set_position_update_interval_internal (gpointer
    user_data);
static void change_state (GstPlayer * self, GstPlayerState state);

static GstPlayerMediaInfo *gst_player_media_info_create (GstPlayer * self);

static void gst_player_streams_info_create (GstPlayer * self,
    GstPlayerMediaInfo * media_info, const gchar * prop, GType type);
static void gst_player_stream_info_update (GstPlayer * self,
    GstPlayerStreamInfo * s);
static void gst_player_stream_info_update_tags_and_caps (GstPlayer * self,
    GstPlayerStreamInfo * s);
static GstPlayerStreamInfo *gst_player_stream_info_find (GstPlayer * self,
    GstPlayerMediaInfo * media_info, GType type, gint stream_index);
static GstPlayerStreamInfo *gst_player_stream_info_get_current (GstPlayer *
    self, const gchar * prop, GType type);

static void gst_player_video_info_update (GstPlayer * self,
    GstPlayerStreamInfo * stream_info);
static void gst_player_audio_info_update (GstPlayer * self,
    GstPlayerStreamInfo * stream_info);
static void gst_player_subtitle_info_update (GstPlayer * self,
    GstPlayerStreamInfo * stream_info);

static void emit_media_info_updated_signal (GstPlayer * self);

static void *get_title (GstTagList * tags);
static void *get_container_format (GstTagList * tags);
static void *get_from_tags (GstPlayer * self, GstPlayerMediaInfo * media_info,
    void *(*func) (GstTagList *));
static void *get_cover_sample (GstTagList * tags);

static void
gst_player_init (GstPlayer * self)
{
  GST_TRACE_OBJECT (self, "Initializing");

  self = gst_player_get_instance_private (self);

  g_mutex_init (&self->lock);
  g_cond_init (&self->cond);

  self->context = g_main_context_new ();
  self->loop = g_main_loop_new (self->context, FALSE);

  self->position_update_interval_ms = DEFAULT_POSITION_UPDATE_INTERVAL_MS;
  self->seek_pending = FALSE;
  self->seek_position = GST_CLOCK_TIME_NONE;
  self->last_seek_time = GST_CLOCK_TIME_NONE;

  GST_TRACE_OBJECT (self, "Initialized");
}

static void
gst_player_class_init (GstPlayerClass * klass)
{
  GObjectClass *gobject_class = (GObjectClass *) klass;

  gobject_class->set_property = gst_player_set_property;
  gobject_class->get_property = gst_player_get_property;
  gobject_class->dispose = gst_player_dispose;
  gobject_class->finalize = gst_player_finalize;
  gobject_class->constructed = gst_player_constructed;

  param_specs[PROP_VIDEO_RENDERER] =
      g_param_spec_object ("video-renderer",
      "Video Renderer", "Video renderer to use for rendering videos",
      GST_TYPE_PLAYER_VIDEO_RENDERER,
      G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

  param_specs[PROP_SIGNAL_DISPATCHER] =
      g_param_spec_object ("signal-dispatcher",
      "Signal Dispatcher", "Dispatcher for the signals to e.g. event loops",
      GST_TYPE_PLAYER_SIGNAL_DISPATCHER,
      G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

  param_specs[PROP_URI] = g_param_spec_string ("uri", "URI", "Current URI",
      DEFAULT_URI, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  param_specs[PROP_SUBURI] = g_param_spec_string ("suburi", "Subtitle URI",
      "Current Subtitle URI", NULL, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  param_specs[PROP_POSITION] =
      g_param_spec_uint64 ("position", "Position", "Current Position",
      0, G_MAXUINT64, DEFAULT_POSITION,
      G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  param_specs[PROP_MEDIA_INFO] =
      g_param_spec_object ("media-info", "Media Info",
      "Current media information", GST_TYPE_PLAYER_MEDIA_INFO,
      G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  param_specs[PROP_CURRENT_AUDIO_TRACK] =
      g_param_spec_object ("current-audio-track", "Current Audio Track",
      "Current audio track information", GST_TYPE_PLAYER_AUDIO_INFO,
      G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  param_specs[PROP_CURRENT_VIDEO_TRACK] =
      g_param_spec_object ("current-video-track", "Current Video Track",
      "Current video track information", GST_TYPE_PLAYER_VIDEO_INFO,
      G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  param_specs[PROP_CURRENT_SUBTITLE_TRACK] =
      g_param_spec_object ("current-subtitle-track", "Current Subtitle Track",
      "Current audio subtitle information", GST_TYPE_PLAYER_SUBTITLE_INFO,
      G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  param_specs[PROP_DURATION] =
      g_param_spec_uint64 ("duration", "Duration", "Duration",
      0, G_MAXUINT64, DEFAULT_DURATION,
      G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  param_specs[PROP_VOLUME] =
      g_param_spec_double ("volume", "Volume", "Volume",
      0, 10.0, DEFAULT_VOLUME, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  param_specs[PROP_MUTE] =
      g_param_spec_boolean ("mute", "Mute", "Mute",
      DEFAULT_MUTE, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  param_specs[PROP_PIPELINE] =
      g_param_spec_object ("pipeline", "Pipeline",
      "GStreamer pipeline that is used",
      GST_TYPE_ELEMENT, G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  param_specs[PROP_RATE] =
      g_param_spec_double ("rate", "rate", "Playback rate",
      -64.0, 64.0, DEFAULT_RATE, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  param_specs[PROP_POSITION_UPDATE_INTERVAL] =
      g_param_spec_uint ("position-update-interval", "Position update interval",
      "Interval in milliseconds between two position-updated signals."
      "Pass 0 to stop updating the position.",
      0, 10000, DEFAULT_POSITION_UPDATE_INTERVAL_MS,
      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (gobject_class, PROP_LAST, param_specs);

  signals[SIGNAL_POSITION_UPDATED] =
      g_signal_new ("position-updated", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS, 0, NULL,
      NULL, NULL, G_TYPE_NONE, 1, GST_TYPE_CLOCK_TIME);

  signals[SIGNAL_DURATION_CHANGED] =
      g_signal_new ("duration-changed", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS, 0, NULL,
      NULL, NULL, G_TYPE_NONE, 1, GST_TYPE_CLOCK_TIME);

  signals[SIGNAL_STATE_CHANGED] =
      g_signal_new ("state-changed", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS, 0, NULL,
      NULL, NULL, G_TYPE_NONE, 1, GST_TYPE_PLAYER_STATE);

  signals[SIGNAL_BUFFERING] =
      g_signal_new ("buffering", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS, 0, NULL,
      NULL, NULL, G_TYPE_NONE, 1, G_TYPE_INT);

  signals[SIGNAL_END_OF_STREAM] =
      g_signal_new ("end-of-stream", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS, 0, NULL,
      NULL, NULL, G_TYPE_NONE, 0, G_TYPE_INVALID);

  signals[SIGNAL_ERROR] =
      g_signal_new ("error", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS, 0, NULL,
      NULL, NULL, G_TYPE_NONE, 1, G_TYPE_ERROR);

  signals[SIGNAL_VIDEO_DIMENSIONS_CHANGED] =
      g_signal_new ("video-dimensions-changed", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS, 0, NULL,
      NULL, NULL, G_TYPE_NONE, 2, G_TYPE_INT, G_TYPE_INT);

  signals[SIGNAL_MEDIA_INFO_UPDATED] =
      g_signal_new ("media-info-updated", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS, 0, NULL,
      NULL, NULL, G_TYPE_NONE, 1, GST_TYPE_PLAYER_MEDIA_INFO);

  signals[SIGNAL_VOLUME_CHANGED] =
      g_signal_new ("volume-changed", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS, 0, NULL,
      NULL, NULL, G_TYPE_NONE, 0, G_TYPE_INVALID);

  signals[SIGNAL_MUTE_CHANGED] =
      g_signal_new ("mute-changed", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS, 0, NULL,
      NULL, NULL, G_TYPE_NONE, 0, G_TYPE_INVALID);

  signals[SIGNAL_WARNING] =
      g_signal_new ("warning", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS, 0, NULL,
      NULL, NULL, G_TYPE_NONE, 1, G_TYPE_ERROR);

  signals[SIGNAL_SEEK_DONE] =
      g_signal_new ("seek-done", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS, 0, NULL,
      NULL, NULL, G_TYPE_NONE, 1, GST_TYPE_CLOCK_TIME);
}

static void
gst_player_dispose (GObject * object)
{
  GstPlayer *self = GST_PLAYER (object);

  GST_TRACE_OBJECT (self, "Stopping main thread");

  if (self->loop) {
    g_main_loop_quit (self->loop);

    g_thread_join (self->thread);
    self->thread = NULL;

    g_main_loop_unref (self->loop);
    self->loop = NULL;

    g_main_context_unref (self->context);
    self->context = NULL;
  }

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gst_player_finalize (GObject * object)
{
  GstPlayer *self = GST_PLAYER (object);

  GST_TRACE_OBJECT (self, "Finalizing");

  g_free (self->uri);
  if (self->suburi)
    g_free (self->suburi);
  if (self->global_tags)
    gst_tag_list_unref (self->global_tags);
  if (self->video_renderer)
    g_object_unref (self->video_renderer);
  if (self->signal_dispatcher)
    g_object_unref (self->signal_dispatcher);
  if (self->current_vis_element)
    gst_object_unref (self->current_vis_element);
  g_mutex_clear (&self->lock);
  g_cond_clear (&self->cond);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gst_player_constructed (GObject * object)
{
  GstPlayer *self = GST_PLAYER (object);

  GST_TRACE_OBJECT (self, "Constructed");

  g_mutex_lock (&self->lock);
  self->thread = g_thread_new ("GstPlayer", gst_player_main, self);
  while (!self->loop || !g_main_loop_is_running (self->loop))
    g_cond_wait (&self->cond, &self->lock);
  g_mutex_unlock (&self->lock);

  G_OBJECT_CLASS (parent_class)->constructed (object);
}

static gboolean
gst_player_set_uri_internal (gpointer user_data)
{
  GstPlayer *self = user_data;

  gst_player_stop_internal (self);

  g_mutex_lock (&self->lock);

  GST_DEBUG_OBJECT (self, "Changing URI to '%s'", GST_STR_NULL (self->uri));

  g_object_set (self->playbin, "uri", self->uri, NULL);

  /* if have suburi from previous playback then free it */
  if (self->suburi) {
    g_free (self->suburi);
    self->suburi = NULL;
    g_object_set (self->playbin, "suburi", NULL, NULL);
  }

  g_mutex_unlock (&self->lock);

  return G_SOURCE_REMOVE;
}

static gboolean
gst_player_set_suburi_internal (gpointer user_data)
{
  GstPlayer *self = user_data;
  GstClockTime position;
  GstState target_state;

  /* save the state and position */
  target_state = self->target_state;
  position = gst_player_get_position (self);

  gst_player_stop_internal (self);
  g_mutex_lock (&self->lock);

  GST_DEBUG_OBJECT (self, "Changing SUBURI to '%s'",
      GST_STR_NULL (self->suburi));

  g_object_set (self->playbin, "suburi", self->suburi, NULL);
  g_object_set (self->playbin, "uri", self->uri, NULL);

  g_mutex_unlock (&self->lock);

  /* restore state and position */
  if (position != GST_CLOCK_TIME_NONE)
    gst_player_seek (self, position);
  if (target_state == GST_STATE_PAUSED)
    gst_player_pause_internal (self);
  else if (target_state == GST_STATE_PLAYING)
    gst_player_play_internal (self);

  return G_SOURCE_REMOVE;
}

static void
gst_player_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstPlayer *self = GST_PLAYER (object);

  switch (prop_id) {
    case PROP_VIDEO_RENDERER:
      self->video_renderer = g_value_dup_object (value);
      break;
    case PROP_SIGNAL_DISPATCHER:
      self->signal_dispatcher = g_value_dup_object (value);
      break;
    case PROP_URI:{
      g_mutex_lock (&self->lock);
      if (self->uri)
        g_free (self->uri);

      self->uri = g_value_dup_string (value);
      GST_DEBUG_OBJECT (self, "Set uri=%s", self->uri);
      g_mutex_unlock (&self->lock);

      g_main_context_invoke_full (self->context, G_PRIORITY_DEFAULT,
          gst_player_set_uri_internal, self, NULL);
      break;
    }
    case PROP_SUBURI:{
      g_mutex_lock (&self->lock);
      if (self->suburi)
        g_free (self->suburi);

      self->suburi = g_value_dup_string (value);
      GST_DEBUG_OBJECT (self, "Set suburi=%s", self->suburi);
      g_mutex_unlock (&self->lock);

      g_main_context_invoke_full (self->context, G_PRIORITY_DEFAULT,
          gst_player_set_suburi_internal, self, NULL);
      break;
    }
    case PROP_VOLUME:
      GST_DEBUG_OBJECT (self, "Set volume=%lf", g_value_get_double (value));
      g_object_set_property (G_OBJECT (self->playbin), "volume", value);
      break;
    case PROP_RATE:
      g_mutex_lock (&self->lock);
      self->rate = g_value_get_double (value);
      GST_DEBUG_OBJECT (self, "Set rate=%lf", g_value_get_double (value));
      g_mutex_unlock (&self->lock);

      gst_player_set_rate_internal (self);
      break;
    case PROP_MUTE:
      GST_DEBUG_OBJECT (self, "Set mute=%d", g_value_get_boolean (value));
      g_object_set_property (G_OBJECT (self->playbin), "mute", value);
      break;
    case PROP_POSITION_UPDATE_INTERVAL:
      g_mutex_lock (&self->lock);
      self->position_update_interval_ms = g_value_get_uint (value);
      GST_DEBUG_OBJECT (self, "Set position update interval=%u ms",
          g_value_get_uint (value));
      g_mutex_unlock (&self->lock);

      gst_player_set_position_update_interval_internal (self);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_player_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstPlayer *self = GST_PLAYER (object);

  switch (prop_id) {
    case PROP_URI:
      g_mutex_lock (&self->lock);
      g_value_set_string (value, self->uri);
      g_mutex_unlock (&self->lock);
      break;
    case PROP_SUBURI:
      g_mutex_lock (&self->lock);
      g_value_set_string (value, self->suburi);
      g_mutex_unlock (&self->lock);
      GST_DEBUG_OBJECT (self, "Returning has-suburi=%d",
          g_value_get_boolean (value));
      break;
    case PROP_POSITION:{
      gint64 position = 0;

      gst_element_query_position (self->playbin, GST_FORMAT_TIME, &position);
      g_value_set_uint64 (value, position);
      GST_TRACE_OBJECT (self, "Returning position=%" GST_TIME_FORMAT,
          GST_TIME_ARGS (g_value_get_uint64 (value)));
      break;
    }
    case PROP_DURATION:{
      gint64 duration = 0;

      gst_element_query_duration (self->playbin, GST_FORMAT_TIME, &duration);
      g_value_set_uint64 (value, duration);
      GST_TRACE_OBJECT (self, "Returning duration=%" GST_TIME_FORMAT,
          GST_TIME_ARGS (g_value_get_uint64 (value)));
      break;
    }
    case PROP_MEDIA_INFO:{
      GstPlayerMediaInfo *media_info = gst_player_get_media_info (self);
      g_value_set_object (value, media_info);
      g_object_unref (media_info);
      break;
    }
    case PROP_CURRENT_AUDIO_TRACK:{
      GstPlayerAudioInfo *audio_info =
          gst_player_get_current_audio_track (self);
      g_value_set_object (value, audio_info);
      g_object_unref (audio_info);
      break;
    }
    case PROP_CURRENT_VIDEO_TRACK:{
      GstPlayerVideoInfo *video_info =
          gst_player_get_current_video_track (self);
      g_value_set_object (value, video_info);
      g_object_unref (video_info);
      break;
    }
    case PROP_CURRENT_SUBTITLE_TRACK:{
      GstPlayerSubtitleInfo *subtitle_info =
          gst_player_get_current_subtitle_track (self);
      g_value_set_object (value, subtitle_info);
      g_object_unref (subtitle_info);
      break;
    }
    case PROP_VOLUME:
      g_object_get_property (G_OBJECT (self->playbin), "volume", value);
      GST_TRACE_OBJECT (self, "Returning volume=%lf",
          g_value_get_double (value));
      break;
    case PROP_RATE:
      g_mutex_lock (&self->lock);
      g_value_set_double (value, gst_player_get_rate (self));
      g_mutex_unlock (&self->lock);
      break;
    case PROP_MUTE:
      g_object_get_property (G_OBJECT (self->playbin), "mute", value);
      GST_TRACE_OBJECT (self, "Returning mute=%d", g_value_get_boolean (value));
      break;
    case PROP_PIPELINE:
      g_value_set_object (value, self->playbin);
      break;
    case PROP_POSITION_UPDATE_INTERVAL:
      g_mutex_lock (&self->lock);
      g_value_set_uint (value, gst_player_get_position_update_interval (self));
      g_mutex_unlock (&self->lock);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static gboolean
main_loop_running_cb (gpointer user_data)
{
  GstPlayer *self = GST_PLAYER (user_data);

  GST_TRACE_OBJECT (self, "Main loop running now");

  g_mutex_lock (&self->lock);
  g_cond_signal (&self->cond);
  g_mutex_unlock (&self->lock);

  return G_SOURCE_REMOVE;
}

typedef struct
{
  GstPlayer *player;
  GstPlayerState state;
} StateChangedSignalData;

static void
state_changed_dispatch (gpointer user_data)
{
  StateChangedSignalData *data = user_data;

  g_signal_emit (data->player, signals[SIGNAL_STATE_CHANGED], 0, data->state);
}

static void
state_changed_signal_data_free (StateChangedSignalData * data)
{
  g_object_unref (data->player);
  g_free (data);
}

static void
change_state (GstPlayer * self, GstPlayerState state)
{
  if (state == self->app_state)
    return;

  GST_DEBUG_OBJECT (self, "Changing app state from %s to %s",
      gst_player_state_get_name (self->app_state),
      gst_player_state_get_name (state));
  self->app_state = state;

  if (g_signal_handler_find (self, G_SIGNAL_MATCH_ID,
          signals[SIGNAL_STATE_CHANGED], 0, NULL, NULL, NULL) != 0) {
    StateChangedSignalData *data = g_new (StateChangedSignalData, 1);

    data->player = g_object_ref (self);
    data->state = state;
    gst_player_signal_dispatcher_dispatch (self->signal_dispatcher, self,
        state_changed_dispatch, data,
        (GDestroyNotify) state_changed_signal_data_free);
  }
}

typedef struct
{
  GstPlayer *player;
  GstClockTime position;
} PositionUpdatedSignalData;

static void
position_updated_dispatch (gpointer user_data)
{
  PositionUpdatedSignalData *data = user_data;

  if (data->player->target_state >= GST_STATE_PAUSED) {
    g_signal_emit (data->player, signals[SIGNAL_POSITION_UPDATED], 0,
        data->position);
    g_object_notify_by_pspec (G_OBJECT (data->player),
        param_specs[PROP_POSITION]);
  }
}

static void
position_updated_signal_data_free (PositionUpdatedSignalData * data)
{
  g_object_unref (data->player);
  g_free (data);
}

static gboolean
tick_cb (gpointer user_data)
{
  GstPlayer *self = GST_PLAYER (user_data);
  gint64 position;

  if (self->target_state >= GST_STATE_PAUSED
      && gst_element_query_position (self->playbin, GST_FORMAT_TIME,
          &position)) {
    GST_LOG_OBJECT (self, "Position %" GST_TIME_FORMAT,
        GST_TIME_ARGS (position));

    if (g_signal_handler_find (self, G_SIGNAL_MATCH_ID,
            signals[SIGNAL_POSITION_UPDATED], 0, NULL, NULL, NULL) != 0) {
      PositionUpdatedSignalData *data = g_new (PositionUpdatedSignalData, 1);

      data->player = g_object_ref (self);
      data->position = position;
      gst_player_signal_dispatcher_dispatch (self->signal_dispatcher, self,
          position_updated_dispatch, data,
          (GDestroyNotify) position_updated_signal_data_free);
    }
  }

  return G_SOURCE_CONTINUE;
}

static void
add_tick_source (GstPlayer * self)
{
  if (self->tick_source)
    return;

  if (!self->position_update_interval_ms)
    return;

  self->tick_source = g_timeout_source_new (self->position_update_interval_ms);
  g_source_set_callback (self->tick_source, (GSourceFunc) tick_cb, self, NULL);
  g_source_attach (self->tick_source, self->context);
}

static void
remove_tick_source (GstPlayer * self)
{
  if (!self->tick_source)
    return;

  g_source_destroy (self->tick_source);
  g_source_unref (self->tick_source);
  self->tick_source = NULL;
}

static gboolean
ready_timeout_cb (gpointer user_data)
{
  GstPlayer *self = user_data;

  if (self->target_state <= GST_STATE_READY) {
    GST_DEBUG_OBJECT (self, "Setting pipeline to NULL state");
    self->target_state = GST_STATE_NULL;
    self->current_state = GST_STATE_NULL;
    gst_element_set_state (self->playbin, GST_STATE_NULL);
  }

  return G_SOURCE_REMOVE;
}

static void
add_ready_timeout_source (GstPlayer * self)
{
  if (self->ready_timeout_source)
    return;

  self->ready_timeout_source = g_timeout_source_new_seconds (60);
  g_source_set_callback (self->ready_timeout_source,
      (GSourceFunc) ready_timeout_cb, self, NULL);
  g_source_attach (self->ready_timeout_source, self->context);
}

static void
remove_ready_timeout_source (GstPlayer * self)
{
  if (!self->ready_timeout_source)
    return;

  g_source_destroy (self->ready_timeout_source);
  g_source_unref (self->ready_timeout_source);
  self->ready_timeout_source = NULL;
}

typedef struct
{
  GstPlayer *player;
  GError *err;
} ErrorSignalData;

static void
error_dispatch (gpointer user_data)
{
  ErrorSignalData *data = user_data;

  g_signal_emit (data->player, signals[SIGNAL_ERROR], 0, data->err);
}

static void
free_error_signal_data (ErrorSignalData * data)
{
  g_object_unref (data->player);
  g_clear_error (&data->err);
  g_free (data);
}

static void
emit_error (GstPlayer * self, GError * err)
{
  GST_ERROR_OBJECT (self, "Error: %s (%s, %d)", err->message,
      g_quark_to_string (err->domain), err->code);

  if (g_signal_handler_find (self, G_SIGNAL_MATCH_ID,
          signals[SIGNAL_ERROR], 0, NULL, NULL, NULL) != 0) {
    ErrorSignalData *data = g_new (ErrorSignalData, 1);

    data->player = g_object_ref (self);
    data->err = g_error_copy (err);
    gst_player_signal_dispatcher_dispatch (self->signal_dispatcher, self,
        error_dispatch, data, (GDestroyNotify) free_error_signal_data);
  }

  g_error_free (err);

  remove_tick_source (self);
  remove_ready_timeout_source (self);

  self->target_state = GST_STATE_NULL;
  self->current_state = GST_STATE_NULL;
  self->is_live = FALSE;
  self->is_eos = FALSE;
  gst_element_set_state (self->playbin, GST_STATE_NULL);
  change_state (self, GST_PLAYER_STATE_STOPPED);
  self->buffering = 100;

  g_mutex_lock (&self->lock);
  if (self->media_info) {
    g_object_unref (self->media_info);
    self->media_info = NULL;
  }

  if (self->global_tags) {
    gst_tag_list_unref (self->global_tags);
    self->global_tags = NULL;
  }

  self->seek_pending = FALSE;
  if (self->seek_source) {
    g_source_destroy (self->seek_source);
    g_source_unref (self->seek_source);
    self->seek_source = NULL;
  }
  self->seek_position = GST_CLOCK_TIME_NONE;
  self->last_seek_time = GST_CLOCK_TIME_NONE;
  g_mutex_unlock (&self->lock);
}

static void
dump_dot_file (GstPlayer * self, const gchar * name)
{
  gchar *full_name;

  full_name = g_strdup_printf ("gst-player.%p.%s", self, name);

  GST_DEBUG_BIN_TO_DOT_FILE_WITH_TS (GST_BIN (self->playbin),
      GST_DEBUG_GRAPH_SHOW_ALL, full_name);

  g_free (full_name);
}

typedef struct
{
  GstPlayer *player;
  GError *err;
} WarningSignalData;

static void
warning_dispatch (gpointer user_data)
{
  WarningSignalData *data = user_data;

  g_signal_emit (data->player, signals[SIGNAL_WARNING], 0, data->err);
}

static void
free_warning_signal_data (WarningSignalData * data)
{
  g_object_unref (data->player);
  g_clear_error (&data->err);
  g_free (data);
}

static void
emit_warning (GstPlayer * self, GError * err)
{
  GST_ERROR_OBJECT (self, "Warning: %s (%s, %d)", err->message,
      g_quark_to_string (err->domain), err->code);

  if (g_signal_handler_find (self, G_SIGNAL_MATCH_ID,
          signals[SIGNAL_WARNING], 0, NULL, NULL, NULL) != 0) {
    WarningSignalData *data = g_new (WarningSignalData, 1);

    data->player = g_object_ref (self);
    data->err = g_error_copy (err);
    gst_player_signal_dispatcher_dispatch (self->signal_dispatcher, self,
        warning_dispatch, data, (GDestroyNotify) free_warning_signal_data);
  }

  g_error_free (err);
}

static void
error_cb (GstBus * bus, GstMessage * msg, gpointer user_data)
{
  GstPlayer *self = GST_PLAYER (user_data);
  GError *err, *player_err;
  gchar *name, *debug, *message, *full_message;

  dump_dot_file (self, "error");

  gst_message_parse_error (msg, &err, &debug);

  name = gst_object_get_path_string (msg->src);
  message = gst_error_get_message (err->domain, err->code);

  if (debug)
    full_message =
        g_strdup_printf ("Error from element %s: %s\n%s\n%s", name, message,
        err->message, debug);
  else
    full_message =
        g_strdup_printf ("Error from element %s: %s\n%s", name, message,
        err->message);

  GST_ERROR_OBJECT (self, "ERROR: from element %s: %s\n", name, err->message);
  if (debug != NULL)
    GST_ERROR_OBJECT (self, "Additional debug info:\n%s\n", debug);

  player_err =
      g_error_new_literal (GST_PLAYER_ERROR, GST_PLAYER_ERROR_FAILED,
      full_message);
  emit_error (self, player_err);

  g_clear_error (&err);
  g_free (debug);
  g_free (name);
  g_free (full_message);
  g_free (message);
}

static void
warning_cb (GstBus * bus, GstMessage * msg, gpointer user_data)
{
  GstPlayer *self = GST_PLAYER (user_data);
  GError *err, *player_err;
  gchar *name, *debug, *message, *full_message;

  dump_dot_file (self, "warning");

  gst_message_parse_warning (msg, &err, &debug);

  name = gst_object_get_path_string (msg->src);
  message = gst_error_get_message (err->domain, err->code);

  if (debug)
    full_message =
        g_strdup_printf ("Warning from element %s: %s\n%s\n%s", name, message,
        err->message, debug);
  else
    full_message =
        g_strdup_printf ("Warning from element %s: %s\n%s", name, message,
        err->message);

  GST_WARNING_OBJECT (self, "WARNING: from element %s: %s\n", name,
      err->message);
  if (debug != NULL)
    GST_WARNING_OBJECT (self, "Additional debug info:\n%s\n", debug);

  player_err =
      g_error_new_literal (GST_PLAYER_ERROR, GST_PLAYER_ERROR_FAILED,
      full_message);
  emit_warning (self, player_err);

  g_clear_error (&err);
  g_free (debug);
  g_free (name);
  g_free (full_message);
  g_free (message);
}

static void
eos_dispatch (gpointer user_data)
{
  g_signal_emit (user_data, signals[SIGNAL_END_OF_STREAM], 0);
}

static void
eos_cb (GstBus * bus, GstMessage * msg, gpointer user_data)
{
  GstPlayer *self = GST_PLAYER (user_data);

  GST_DEBUG_OBJECT (self, "End of stream");

  tick_cb (self);
  remove_tick_source (self);

  if (g_signal_handler_find (self, G_SIGNAL_MATCH_ID,
          signals[SIGNAL_END_OF_STREAM], 0, NULL, NULL, NULL) != 0) {
    gst_player_signal_dispatcher_dispatch (self->signal_dispatcher, self,
        eos_dispatch, g_object_ref (self), (GDestroyNotify) g_object_unref);
  }
  change_state (self, GST_PLAYER_STATE_STOPPED);
  self->buffering = 100;
  self->is_eos = TRUE;
}

typedef struct
{
  GstPlayer *player;
  gint percent;
} BufferingSignalData;

static void
buffering_dispatch (gpointer user_data)
{
  BufferingSignalData *data = user_data;

  if (data->player->target_state >= GST_STATE_PAUSED) {
    g_signal_emit (data->player, signals[SIGNAL_BUFFERING], 0, data->percent);
  }
}

static void
buffering_signal_data_free (BufferingSignalData * data)
{
  g_object_unref (data->player);
  g_free (data);
}

static void
buffering_cb (GstBus * bus, GstMessage * msg, gpointer user_data)
{
  GstPlayer *self = GST_PLAYER (user_data);
  gint percent;

  if (self->target_state < GST_STATE_PAUSED)
    return;
  if (self->is_live)
    return;

  gst_message_parse_buffering (msg, &percent);
  GST_LOG_OBJECT (self, "Buffering %d%%", percent);

  if (percent < 100 && self->target_state >= GST_STATE_PAUSED) {
    GstStateChangeReturn state_ret;

    GST_DEBUG_OBJECT (self, "Waiting for buffering to finish");
    state_ret = gst_element_set_state (self->playbin, GST_STATE_PAUSED);

    if (state_ret == GST_STATE_CHANGE_FAILURE) {
      emit_error (self, g_error_new (GST_PLAYER_ERROR, GST_PLAYER_ERROR_FAILED,
              "Failed to handle buffering"));
      return;
    }

    change_state (self, GST_PLAYER_STATE_BUFFERING);
  }

  if (self->buffering != percent) {
    if (g_signal_handler_find (self, G_SIGNAL_MATCH_ID,
            signals[SIGNAL_BUFFERING], 0, NULL, NULL, NULL) != 0) {
      BufferingSignalData *data = g_new (BufferingSignalData, 1);

      data->player = g_object_ref (self);
      data->percent = percent;
      gst_player_signal_dispatcher_dispatch (self->signal_dispatcher, self,
          buffering_dispatch, data,
          (GDestroyNotify) buffering_signal_data_free);
    }

    self->buffering = percent;
  }


  g_mutex_lock (&self->lock);
  if (percent == 100 && (self->seek_position != GST_CLOCK_TIME_NONE ||
          self->seek_pending)) {
    g_mutex_unlock (&self->lock);

    GST_DEBUG_OBJECT (self, "Buffering finished - seek pending");
  } else if (percent == 100 && self->target_state >= GST_STATE_PLAYING
      && self->current_state >= GST_STATE_PAUSED) {
    GstStateChangeReturn state_ret;

    g_mutex_unlock (&self->lock);

    GST_DEBUG_OBJECT (self, "Buffering finished - going to PLAYING");
    state_ret = gst_element_set_state (self->playbin, GST_STATE_PLAYING);
    /* Application state change is happening when the state change happened */
    if (state_ret == GST_STATE_CHANGE_FAILURE)
      emit_error (self, g_error_new (GST_PLAYER_ERROR, GST_PLAYER_ERROR_FAILED,
              "Failed to handle buffering"));
  } else if (percent == 100 && self->target_state >= GST_STATE_PAUSED) {
    g_mutex_unlock (&self->lock);

    GST_DEBUG_OBJECT (self, "Buffering finished - staying PAUSED");
    change_state (self, GST_PLAYER_STATE_PAUSED);
  } else {
    g_mutex_unlock (&self->lock);
  }
}

static void
clock_lost_cb (GstBus * bus, GstMessage * msg, gpointer user_data)
{
  GstPlayer *self = GST_PLAYER (user_data);
  GstStateChangeReturn state_ret;

  GST_DEBUG_OBJECT (self, "Clock lost");
  if (self->target_state >= GST_STATE_PLAYING) {
    state_ret = gst_element_set_state (self->playbin, GST_STATE_PAUSED);
    if (state_ret != GST_STATE_CHANGE_FAILURE)
      state_ret = gst_element_set_state (self->playbin, GST_STATE_PLAYING);

    if (state_ret == GST_STATE_CHANGE_FAILURE)
      emit_error (self, g_error_new (GST_PLAYER_ERROR, GST_PLAYER_ERROR_FAILED,
              "Failed to handle clock loss"));
  }
}

typedef struct
{
  GstPlayer *player;
  gint width, height;
} VideoDimensionsChangedSignalData;

static void
video_dimensions_changed_dispatch (gpointer user_data)
{
  VideoDimensionsChangedSignalData *data = user_data;

  if (data->player->target_state >= GST_STATE_PAUSED) {
    g_signal_emit (data->player, signals[SIGNAL_VIDEO_DIMENSIONS_CHANGED], 0,
        data->width, data->height);
  }
}

static void
video_dimensions_changed_signal_data_free (VideoDimensionsChangedSignalData *
    data)
{
  g_object_unref (data->player);
  g_free (data);
}

static void
check_video_dimensions_changed (GstPlayer * self)
{
  GstElement *video_sink;
  GstPad *video_sink_pad;
  GstCaps *caps;
  GstVideoInfo info;
  gint width = 0, height = 0;

  g_object_get (self->playbin, "video-sink", &video_sink, NULL);
  if (!video_sink)
    goto out;

  video_sink_pad = gst_element_get_static_pad (video_sink, "sink");
  if (!video_sink_pad) {
    gst_object_unref (video_sink);
    goto out;
  }

  caps = gst_pad_get_current_caps (video_sink_pad);

  if (caps) {
    if (gst_video_info_from_caps (&info, caps)) {
      info.width = info.width * info.par_n / info.par_d;

      GST_DEBUG_OBJECT (self, "Video dimensions changed: %dx%d", info.width,
          info.height);
      width = info.width;
      height = info.height;
    }

    gst_caps_unref (caps);
  }
  gst_object_unref (video_sink_pad);
  gst_object_unref (video_sink);

out:
  if (g_signal_handler_find (self, G_SIGNAL_MATCH_ID,
          signals[SIGNAL_VIDEO_DIMENSIONS_CHANGED], 0, NULL, NULL, NULL) != 0) {
    VideoDimensionsChangedSignalData *data =
        g_new (VideoDimensionsChangedSignalData, 1);

    data->player = g_object_ref (self);
    data->width = width;
    data->height = height;
    gst_player_signal_dispatcher_dispatch (self->signal_dispatcher, self,
        video_dimensions_changed_dispatch, data,
        (GDestroyNotify) video_dimensions_changed_signal_data_free);
  }
}

static void
notify_caps_cb (GObject * object, GParamSpec * pspec, gpointer user_data)
{
  GstPlayer *self = GST_PLAYER (user_data);

  check_video_dimensions_changed (self);
}

typedef struct
{
  GstPlayer *player;
  GstClockTime duration;
} DurationChangedSignalData;

static void
duration_changed_dispatch (gpointer user_data)
{
  DurationChangedSignalData *data = user_data;

  if (data->player->target_state >= GST_STATE_PAUSED) {
    g_signal_emit (data->player, signals[SIGNAL_DURATION_CHANGED], 0,
        data->duration);
    g_object_notify_by_pspec (G_OBJECT (data->player),
        param_specs[PROP_DURATION]);
  }
}

static void
duration_changed_signal_data_free (DurationChangedSignalData * data)
{
  g_object_unref (data->player);
  g_free (data);
}

static void
emit_duration_changed (GstPlayer * self, GstClockTime duration)
{
  GST_DEBUG_OBJECT (self, "Duration changed %" GST_TIME_FORMAT,
      GST_TIME_ARGS (duration));

  if (g_signal_handler_find (self, G_SIGNAL_MATCH_ID,
          signals[SIGNAL_DURATION_CHANGED], 0, NULL, NULL, NULL) != 0) {
    DurationChangedSignalData *data = g_new (DurationChangedSignalData, 1);

    data->player = g_object_ref (self);
    data->duration = duration;
    gst_player_signal_dispatcher_dispatch (self->signal_dispatcher, self,
        duration_changed_dispatch, data,
        (GDestroyNotify) duration_changed_signal_data_free);
  }
}

typedef struct
{
  GstPlayer *player;
  GstClockTime position;
} SeekDoneSignalData;

static void
seek_done_dispatch (gpointer user_data)
{
  SeekDoneSignalData *data = user_data;

  g_signal_emit (data->player, signals[SIGNAL_SEEK_DONE], 0, data->position);
}

static void
seek_done_signal_data_free (SeekDoneSignalData * data)
{
  g_object_unref (data->player);
  g_free (data);
}

static void
emit_seek_done (GstPlayer * self)
{
  if (g_signal_handler_find (self, G_SIGNAL_MATCH_ID,
          signals[SIGNAL_SEEK_DONE], 0, NULL, NULL, NULL) != 0) {
    SeekDoneSignalData *data = g_new (SeekDoneSignalData, 1);

    data->player = g_object_ref (self);
    data->position = gst_player_get_position (self);
    gst_player_signal_dispatcher_dispatch (self->signal_dispatcher, self,
        seek_done_dispatch, data, (GDestroyNotify) seek_done_signal_data_free);
  }
}

static void
state_changed_cb (GstBus * bus, GstMessage * msg, gpointer user_data)
{
  GstPlayer *self = GST_PLAYER (user_data);
  GstState old_state, new_state, pending_state;

  gst_message_parse_state_changed (msg, &old_state, &new_state, &pending_state);

  if (GST_MESSAGE_SRC (msg) == GST_OBJECT (self->playbin)) {
    gchar *transition_name;

    GST_DEBUG_OBJECT (self, "Changed state old: %s new: %s pending: %s",
        gst_element_state_get_name (old_state),
        gst_element_state_get_name (new_state),
        gst_element_state_get_name (pending_state));

    transition_name = g_strdup_printf ("%s_%s",
        gst_element_state_get_name (old_state),
        gst_element_state_get_name (new_state));
    dump_dot_file (self, transition_name);
    g_free (transition_name);

    self->current_state = new_state;

    if (old_state == GST_STATE_READY && new_state == GST_STATE_PAUSED
        && pending_state == GST_STATE_VOID_PENDING) {
      GstElement *video_sink;
      GstPad *video_sink_pad;
      gint64 duration = -1;

      GST_DEBUG_OBJECT (self, "Initial PAUSED - pre-rolled");

      g_mutex_lock (&self->lock);
      if (self->media_info)
        g_object_unref (self->media_info);
      self->media_info = gst_player_media_info_create (self);
      g_mutex_unlock (&self->lock);
      emit_media_info_updated_signal (self);

      g_object_get (self->playbin, "video-sink", &video_sink, NULL);

      if (video_sink) {
        video_sink_pad = gst_element_get_static_pad (video_sink, "sink");

        if (video_sink_pad) {
          g_signal_connect (video_sink_pad, "notify::caps",
              (GCallback) notify_caps_cb, self);
          gst_object_unref (video_sink_pad);
        }
        gst_object_unref (video_sink);
      }

      check_video_dimensions_changed (self);
      gst_element_query_duration (self->playbin, GST_FORMAT_TIME, &duration);
      emit_duration_changed (self, duration);
    }

    if (new_state == GST_STATE_PAUSED
        && pending_state == GST_STATE_VOID_PENDING) {
      remove_tick_source (self);

      g_mutex_lock (&self->lock);
      if (self->seek_pending) {
        self->seek_pending = FALSE;

        if (!self->media_info->seekable) {
          GST_DEBUG_OBJECT (self, "Media is not seekable");
          if (self->seek_source) {
            g_source_destroy (self->seek_source);
            g_source_unref (self->seek_source);
            self->seek_source = NULL;
          }
          self->seek_position = GST_CLOCK_TIME_NONE;
          self->last_seek_time = GST_CLOCK_TIME_NONE;
        } else if (self->seek_source) {
          GST_DEBUG_OBJECT (self, "Seek finished but new seek is pending");
          gst_player_seek_internal_locked (self);
        } else {
          GST_DEBUG_OBJECT (self, "Seek finished");
          emit_seek_done (self);
        }
      }

      if (self->seek_position != GST_CLOCK_TIME_NONE) {
        GST_DEBUG_OBJECT (self, "Seeking now that we reached PAUSED state");
        gst_player_seek_internal_locked (self);
        g_mutex_unlock (&self->lock);
      } else if (!self->seek_pending) {
        g_mutex_unlock (&self->lock);

        tick_cb (self);

        if (self->target_state >= GST_STATE_PLAYING && self->buffering == 100) {
          GstStateChangeReturn state_ret;

          state_ret = gst_element_set_state (self->playbin, GST_STATE_PLAYING);
          if (state_ret == GST_STATE_CHANGE_FAILURE)
            emit_error (self, g_error_new (GST_PLAYER_ERROR,
                    GST_PLAYER_ERROR_FAILED, "Failed to play"));
        } else if (self->buffering == 100) {
          change_state (self, GST_PLAYER_STATE_PAUSED);
        }
      } else {
        g_mutex_unlock (&self->lock);
      }
    } else if (new_state == GST_STATE_PLAYING
        && pending_state == GST_STATE_VOID_PENDING) {

      /* If no seek is currently pending, add the tick source. This can happen
       * if we seeked already but the state-change message was still queued up */
      if (!self->seek_pending) {
        add_tick_source (self);
        change_state (self, GST_PLAYER_STATE_PLAYING);
      }
    } else if (new_state == GST_STATE_READY && old_state > GST_STATE_READY) {
      change_state (self, GST_PLAYER_STATE_STOPPED);
    } else {
      /* Otherwise we neither reached PLAYING nor PAUSED, so must
       * wait for something to happen... i.e. are BUFFERING now */
      change_state (self, GST_PLAYER_STATE_BUFFERING);
    }
  }
}

static void
duration_changed_cb (GstBus * bus, GstMessage * msg, gpointer user_data)
{
  GstPlayer *self = GST_PLAYER (user_data);
  gint64 duration;

  if (gst_element_query_duration (self->playbin, GST_FORMAT_TIME, &duration)) {
    emit_duration_changed (self, duration);
  }
}

static void
latency_cb (GstBus * bus, GstMessage * msg, gpointer user_data)
{
  GstPlayer *self = GST_PLAYER (user_data);

  GST_DEBUG_OBJECT (self, "Latency changed");

  gst_bin_recalculate_latency (GST_BIN (self->playbin));
}

static void
request_state_cb (GstBus * bus, GstMessage * msg, gpointer user_data)
{
  GstPlayer *self = GST_PLAYER (user_data);
  GstState state;
  GstStateChangeReturn state_ret;

  gst_message_parse_request_state (msg, &state);

  GST_DEBUG_OBJECT (self, "State %s requested",
      gst_element_state_get_name (state));

  self->target_state = state;
  state_ret = gst_element_set_state (self->playbin, state);
  if (state_ret == GST_STATE_CHANGE_FAILURE)
    emit_error (self, g_error_new (GST_PLAYER_ERROR, GST_PLAYER_ERROR_FAILED,
            "Failed to change to requested state %s",
            gst_element_state_get_name (state)));
}

static void
media_info_update (GstPlayer * self, GstPlayerMediaInfo * info)
{
  if (info->title)
    g_free (info->title);
  info->title = get_from_tags (self, info, get_title);

  if (info->container)
    g_free (info->container);
  info->container = get_from_tags (self, info, get_container_format);

  if (info->image_sample)
    gst_sample_unref (info->image_sample);
  info->image_sample = get_from_tags (self, info, get_cover_sample);

  GST_DEBUG_OBJECT (self, "title: %s, container: %s "
      "image_sample: %p", info->title, info->container, info->image_sample);
}

static void
tags_cb (GstBus * bus, GstMessage * msg, gpointer user_data)
{
  GstPlayer *self = GST_PLAYER (user_data);
  GstTagList *tags = NULL;

  gst_message_parse_tag (msg, &tags);

  /*
   * NOTE: Inorder to get global tag you must apply the following patches in
   * your gstreamer build.
   *
   * http://cgit.freedesktop.org/gstreamer/gst-plugins-good/commit/?id=9119fbd774093e3ae762c8652acd80d54b2c3b45
   * http://cgit.freedesktop.org/gstreamer/gstreamer/commit/?id=18b058100940bdcaed86fa412e3582a02871f995
   */
  GST_DEBUG_OBJECT (self, "recieved %s tags",
      gst_tag_list_get_scope (tags) ==
      GST_TAG_SCOPE_GLOBAL ? "global" : "stream");

  if (gst_tag_list_get_scope (tags) == GST_TAG_SCOPE_GLOBAL) {
    g_mutex_lock (&self->lock);
    if (self->media_info) {
      if (self->media_info->tags)
        gst_tag_list_unref (self->media_info->tags);
      self->media_info->tags = gst_tag_list_ref (tags);
      media_info_update (self, self->media_info);
      g_mutex_unlock (&self->lock);
      emit_media_info_updated_signal (self);
    } else {
      if (self->global_tags)
        gst_tag_list_unref (self->global_tags);
      self->global_tags = gst_tag_list_ref (tags);
      g_mutex_unlock (&self->lock);
    }
  }

  gst_tag_list_unref (tags);
}

static void
element_cb (GstBus * bus, GstMessage * msg, gpointer user_data)
{
  GstPlayer *self = GST_PLAYER (user_data);
  const GstStructure *s;

  s = gst_message_get_structure (msg);
  if (gst_structure_has_name (s, "redirect")) {
    const gchar *new_location;

    new_location = gst_structure_get_string (s, "new-location");
    if (!new_location) {
      const GValue *locations_list, *location_val;
      guint i, size;

      locations_list = gst_structure_get_value (s, "locations");
      size = gst_value_list_get_size (locations_list);
      for (i = 0; i < size; ++i) {
        const GstStructure *location_s;

        location_val = gst_value_list_get_value (locations_list, i);
        if (!GST_VALUE_HOLDS_STRUCTURE (location_val))
          continue;

        location_s = (const GstStructure *) g_value_get_boxed (location_val);
        if (!gst_structure_has_name (location_s, "redirect"))
          continue;

        new_location = gst_structure_get_string (location_s, "new-location");
        if (new_location)
          break;
      }
    }

    if (new_location) {
      GstState target_state;

      GST_DEBUG_OBJECT (self, "Redirect to '%s'", new_location);

      /* Remember target state and restore after setting the URI */
      target_state = self->target_state;

      g_mutex_lock (&self->lock);
      if (self->uri)
        g_free (self->uri);

      self->uri = g_strdup (new_location);
      g_mutex_unlock (&self->lock);

      gst_player_set_uri_internal (self);

      if (target_state == GST_STATE_PAUSED)
        gst_player_pause_internal (self);
      else if (target_state == GST_STATE_PLAYING)
        gst_player_play_internal (self);
    }
  }
}

static void
player_set_flag (GstPlayer * self, gint pos)
{
  gint flags;

  g_object_get (self->playbin, "flags", &flags, NULL);
  flags |= pos;
  g_object_set (self->playbin, "flags", flags, NULL);

  GST_DEBUG_OBJECT (self, "setting flags=%#x", flags);
}

static void
player_clear_flag (GstPlayer * self, gint pos)
{
  gint flags;

  g_object_get (self->playbin, "flags", &flags, NULL);
  flags &= ~pos;
  g_object_set (self->playbin, "flags", flags, NULL);

  GST_DEBUG_OBJECT (self, "setting flags=%#x", flags);
}

typedef struct
{
  GstPlayer *player;
  GstPlayerMediaInfo *info;
} MediaInfoUpdatedSignalData;

static void
media_info_updated_dispatch (gpointer user_data)
{
  MediaInfoUpdatedSignalData *data = user_data;

  if (data->player->target_state >= GST_STATE_PAUSED) {
    g_signal_emit (data->player, signals[SIGNAL_MEDIA_INFO_UPDATED], 0,
        data->info);
  }
}

static void
free_media_info_updated_signal_data (MediaInfoUpdatedSignalData * data)
{
  g_object_unref (data->player);
  g_object_unref (data->info);
  g_free (data);
}

/*
 * emit_media_info_updated_signal:
 *
 * create a new copy of self->media_info object and emits the newly created
 * copy to user application. The newly created media_info will be unref'ed
 * as part of signal finalize method.
 */
static void
emit_media_info_updated_signal (GstPlayer * self)
{
  MediaInfoUpdatedSignalData *data = g_new (MediaInfoUpdatedSignalData, 1);
  data->player = g_object_ref (self);
  g_mutex_lock (&self->lock);
  data->info = gst_player_media_info_copy (self->media_info);
  g_mutex_unlock (&self->lock);

  gst_player_signal_dispatcher_dispatch (self->signal_dispatcher, self,
      media_info_updated_dispatch, data,
      (GDestroyNotify) free_media_info_updated_signal_data);
}

static GstCaps *
get_caps (GstPlayer * self, gint stream_index, GType type)
{
  GstPad *pad = NULL;
  GstCaps *caps = NULL;

  if (type == GST_TYPE_PLAYER_VIDEO_INFO)
    g_signal_emit_by_name (G_OBJECT (self->playbin),
        "get-video-pad", stream_index, &pad);
  else if (type == GST_TYPE_PLAYER_AUDIO_INFO)
    g_signal_emit_by_name (G_OBJECT (self->playbin),
        "get-audio-pad", stream_index, &pad);
  else
    g_signal_emit_by_name (G_OBJECT (self->playbin),
        "get-text-pad", stream_index, &pad);

  if (pad) {
    caps = gst_pad_get_current_caps (pad);
    gst_object_unref (pad);
  }

  return caps;
}

static void
gst_player_subtitle_info_update (GstPlayer * self,
    GstPlayerStreamInfo * stream_info)
{
  GstPlayerSubtitleInfo *info = (GstPlayerSubtitleInfo *) stream_info;

  if (stream_info->tags) {

    /* free the old language info */
    if (info->language) {
      g_free (info->language);
      info->language = NULL;
    }

    /* First try to get the language full name from tag, if name is not
     * available then try language code. If we find the language code
     * then use gstreamer api to translate code to full name.
     */
    gst_tag_list_get_string (stream_info->tags, GST_TAG_LANGUAGE_NAME,
        &info->language);
    if (!info->language) {
      gchar *lang_code = NULL;

      gst_tag_list_get_string (stream_info->tags, GST_TAG_LANGUAGE_CODE,
          &lang_code);
      if (lang_code) {
        info->language = g_strdup (gst_tag_get_language_name (lang_code));
        g_free (lang_code);
      }
    }

    /* If we are still failed to find language name then check if external
     * subtitle is loaded and compare the stream index between current sub
     * stream index with our stream index and if matches then declare it as
     * external subtitle and use the filename.
     */
    if (!info->language) {
      gint text_index = -1;
      gchar *suburi = NULL;

      g_object_get (G_OBJECT (self->playbin), "current-suburi", &suburi, NULL);
      if (suburi) {
        g_object_get (G_OBJECT (self->playbin), "current-text", &text_index,
            NULL);
        if (text_index == gst_player_stream_info_get_index (stream_info))
          info->language = g_path_get_basename (suburi);
        g_free (suburi);
      }
    }

  } else {
    if (info->language) {
      g_free (info->language);
      info->language = NULL;
    }
  }

  GST_DEBUG_OBJECT (self, "language=%s", info->language);
}

static void
gst_player_video_info_update (GstPlayer * self,
    GstPlayerStreamInfo * stream_info)
{
  GstPlayerVideoInfo *info = (GstPlayerVideoInfo *) stream_info;

  if (stream_info->caps) {
    GstStructure *s;

    s = gst_caps_get_structure (stream_info->caps, 0);
    if (s) {
      gint width, height;
      gint fps_n, fps_d;
      gint par_n, par_d;

      if (gst_structure_get_int (s, "width", &width))
        info->width = width;
      else
        info->width = -1;

      if (gst_structure_get_int (s, "height", &height))
        info->height = height;
      else
        info->height = -1;

      if (gst_structure_get_fraction (s, "framerate", &fps_n, &fps_d)) {
        info->framerate_num = fps_n;
        info->framerate_denom = fps_d;
      } else {
        info->framerate_num = 0;
        info->framerate_denom = 1;
      }


      if (gst_structure_get_fraction (s, "pixel-aspect-ratio", &par_n, &par_d)) {
        info->par_num = par_n;
        info->par_denom = par_d;
      } else {
        info->par_num = 1;
        info->par_denom = 1;
      }
    }
  } else {
    info->width = info->height = -1;
    info->par_num = info->par_denom = 1;
    info->framerate_num = 0;
    info->framerate_denom = 1;
  }

  if (stream_info->tags) {
    guint bitrate, max_bitrate;

    if (gst_tag_list_get_uint (stream_info->tags, GST_TAG_BITRATE, &bitrate))
      info->bitrate = bitrate;
    else
      info->bitrate = -1;

    if (gst_tag_list_get_uint (stream_info->tags, GST_TAG_MAXIMUM_BITRATE,
            &max_bitrate) || gst_tag_list_get_uint (stream_info->tags,
            GST_TAG_NOMINAL_BITRATE, &max_bitrate))
      info->max_bitrate = max_bitrate;
    else
      info->max_bitrate = -1;
  } else {
    info->bitrate = info->max_bitrate = -1;
  }

  GST_DEBUG_OBJECT (self, "width=%d height=%d fps=%.2f par=%d:%d "
      "bitrate=%d max_bitrate=%d", info->width, info->height,
      (gdouble) info->framerate_num / info->framerate_denom,
      info->par_num, info->par_denom, info->bitrate, info->max_bitrate);
}

static void
gst_player_audio_info_update (GstPlayer * self,
    GstPlayerStreamInfo * stream_info)
{
  GstPlayerAudioInfo *info = (GstPlayerAudioInfo *) stream_info;

  if (stream_info->caps) {
    GstStructure *s;

    s = gst_caps_get_structure (stream_info->caps, 0);
    if (s) {
      gint rate, channels;

      if (gst_structure_get_int (s, "rate", &rate))
        info->sample_rate = rate;
      else
        info->sample_rate = -1;

      if (gst_structure_get_int (s, "channels", &channels))
        info->channels = channels;
      else
        info->channels = 0;
    }
  } else {
    info->sample_rate = -1;
    info->channels = 0;
  }

  if (stream_info->tags) {
    guint bitrate, max_bitrate;

    if (gst_tag_list_get_uint (stream_info->tags, GST_TAG_BITRATE, &bitrate))
      info->bitrate = bitrate;
    else
      info->bitrate = -1;

    if (gst_tag_list_get_uint (stream_info->tags, GST_TAG_MAXIMUM_BITRATE,
            &max_bitrate) || gst_tag_list_get_uint (stream_info->tags,
            GST_TAG_NOMINAL_BITRATE, &max_bitrate))
      info->max_bitrate = max_bitrate;
    else
      info->max_bitrate = -1;

    /* if we have old language the free it */
    if (info->language) {
      g_free (info->language);
      info->language = NULL;
    }

    /* First try to get the language full name from tag, if name is not
     * available then try language code. If we find the language code
     * then use gstreamer api to translate code to full name.
     */
    gst_tag_list_get_string (stream_info->tags, GST_TAG_LANGUAGE_NAME,
        &info->language);
    if (!info->language) {
      gchar *lang_code = NULL;

      gst_tag_list_get_string (stream_info->tags, GST_TAG_LANGUAGE_CODE,
          &lang_code);
      if (lang_code) {
        info->language = g_strdup (gst_tag_get_language_name (lang_code));
        g_free (lang_code);
      }
    }
  } else {
    if (info->language) {
      g_free (info->language);
      info->language = NULL;
    }
    info->max_bitrate = info->bitrate = -1;
  }

  GST_DEBUG_OBJECT (self, "language=%s rate=%d channels=%d bitrate=%d "
      "max_bitrate=%d", info->language, info->sample_rate, info->channels,
      info->bitrate, info->bitrate);
}

static GstPlayerStreamInfo *
gst_player_stream_info_find (GstPlayer * self, GstPlayerMediaInfo * media_info,
    GType type, gint stream_index)
{
  GList *list, *l;
  GstPlayerStreamInfo *info = NULL;

  if (!media_info)
    return NULL;

  list = gst_player_media_info_get_stream_list (media_info);
  for (l = list; l != NULL; l = l->next) {
    info = (GstPlayerStreamInfo *) l->data;
    if ((G_OBJECT_TYPE (info) == type) && (info->stream_index == stream_index)) {
      return info;
    }
  }

  return NULL;
}

static gboolean
is_track_enabled (GstPlayer * self, gint pos)
{
  gint flags;

  g_object_get (G_OBJECT (self->playbin), "flags", &flags, NULL);

  if ((flags & pos))
    return TRUE;

  return FALSE;
}

static GstPlayerStreamInfo *
gst_player_stream_info_get_current (GstPlayer * self, const gchar * prop,
    GType type)
{
  gint current;
  GstPlayerStreamInfo *info;

  if (!self->media_info)
    return NULL;

  g_object_get (G_OBJECT (self->playbin), prop, &current, NULL);
  g_mutex_lock (&self->lock);
  info = gst_player_stream_info_find (self, self->media_info, type, current);
  if (info)
    info = gst_player_stream_info_copy (info);
  g_mutex_unlock (&self->lock);

  return info;
}

static void
gst_player_stream_info_update (GstPlayer * self, GstPlayerStreamInfo * s)
{
  if (GST_IS_PLAYER_VIDEO_INFO (s))
    gst_player_video_info_update (self, s);
  else if (GST_IS_PLAYER_AUDIO_INFO (s))
    gst_player_audio_info_update (self, s);
  else
    gst_player_subtitle_info_update (self, s);
}

static gchar *
stream_info_get_codec (GstPlayerStreamInfo * s)
{
  const gchar *type;
  GstTagList *tags;
  gchar *codec = NULL;

  if (GST_IS_PLAYER_VIDEO_INFO (s))
    type = GST_TAG_VIDEO_CODEC;
  else if (GST_IS_PLAYER_AUDIO_INFO (s))
    type = GST_TAG_AUDIO_CODEC;
  else
    type = GST_TAG_SUBTITLE_CODEC;

  tags = gst_player_stream_info_get_tags (s);
  if (tags) {
    gst_tag_list_get_string (tags, type, &codec);
    if (!codec)
      gst_tag_list_get_string (tags, GST_TAG_CODEC, &codec);
  }

  if (!codec) {
    GstCaps *caps;
    caps = gst_player_stream_info_get_caps (s);
    if (caps) {
      codec = gst_pb_utils_get_codec_description (caps);
    }
  }

  return codec;
}

static void
gst_player_stream_info_update_tags_and_caps (GstPlayer * self,
    GstPlayerStreamInfo * s)
{
  GstTagList *tags;
  gint stream_index;

  stream_index = gst_player_stream_info_get_index (s);

  if (GST_IS_PLAYER_VIDEO_INFO (s))
    g_signal_emit_by_name (self->playbin, "get-video-tags",
        stream_index, &tags);
  else if (GST_IS_PLAYER_AUDIO_INFO (s))
    g_signal_emit_by_name (self->playbin, "get-audio-tags",
        stream_index, &tags);
  else
    g_signal_emit_by_name (self->playbin, "get-text-tags", stream_index, &tags);

  if (s->tags)
    gst_tag_list_unref (s->tags);
  s->tags = tags;

  if (s->caps)
    gst_caps_unref (s->caps);
  s->caps = get_caps (self, stream_index, G_OBJECT_TYPE (s));

  if (s->codec)
    g_free (s->codec);
  s->codec = stream_info_get_codec (s);

  GST_DEBUG_OBJECT (self, "%s index: %d tags: %p caps: %p",
      gst_player_stream_info_get_stream_type (s), stream_index,
      s->tags, s->caps);

  gst_player_stream_info_update (self, s);
}

static void
gst_player_streams_info_create (GstPlayer * self,
    GstPlayerMediaInfo * media_info, const gchar * prop, GType type)
{
  gint i;
  gint total = -1;
  GstPlayerStreamInfo *s;

  if (!media_info)
    return;

  g_object_get (G_OBJECT (self->playbin), prop, &total, NULL);

  GST_DEBUG_OBJECT (self, "%s: %d", prop, total);

  for (i = 0; i < total; i++) {
    /* check if stream already exist in the list */
    s = gst_player_stream_info_find (self, media_info, type, i);

    if (!s) {
      /* create a new stream info instance */
      s = gst_player_stream_info_new (i, type);

      /* add the object in stream list */
      media_info->stream_list = g_list_append (media_info->stream_list, s);

      /* based on type, add the object in its corresponding stream_ list */
      if (GST_IS_PLAYER_AUDIO_INFO (s))
        media_info->audio_stream_list = g_list_append
            (media_info->audio_stream_list, s);
      else if (GST_IS_PLAYER_VIDEO_INFO (s))
        media_info->video_stream_list = g_list_append
            (media_info->video_stream_list, s);
      else
        media_info->subtitle_stream_list = g_list_append
            (media_info->subtitle_stream_list, s);

      GST_DEBUG_OBJECT (self, "create %s stream stream_index: %d",
          gst_player_stream_info_get_stream_type (s), i);
    }

    gst_player_stream_info_update_tags_and_caps (self, s);
  }
}

static void
video_changed_cb (GObject * object, gpointer user_data)
{
  GstPlayer *self = GST_PLAYER (user_data);

  g_mutex_lock (&self->lock);
  gst_player_streams_info_create (self, self->media_info,
      "n-video", GST_TYPE_PLAYER_VIDEO_INFO);
  g_mutex_unlock (&self->lock);
}

static void
audio_changed_cb (GObject * object, gpointer user_data)
{
  GstPlayer *self = GST_PLAYER (user_data);

  g_mutex_lock (&self->lock);
  gst_player_streams_info_create (self, self->media_info,
      "n-audio", GST_TYPE_PLAYER_AUDIO_INFO);
  g_mutex_unlock (&self->lock);
}

static void
subtitle_changed_cb (GObject * object, gpointer user_data)
{
  GstPlayer *self = GST_PLAYER (user_data);

  g_mutex_lock (&self->lock);
  gst_player_streams_info_create (self, self->media_info,
      "n-text", GST_TYPE_PLAYER_SUBTITLE_INFO);
  g_mutex_unlock (&self->lock);
}

static void *
get_title (GstTagList * tags)
{
  gchar *title = NULL;

  gst_tag_list_get_string (tags, GST_TAG_TITLE, &title);
  if (!title)
    gst_tag_list_get_string (tags, GST_TAG_TITLE_SORTNAME, &title);

  return title;
}

static void *
get_container_format (GstTagList * tags)
{
  gchar *container = NULL;

  gst_tag_list_get_string (tags, GST_TAG_CONTAINER_FORMAT, &container);

  /* TODO: If container is not available then maybe consider
   * parsing caps or file extension to guess the container format.
   */

  return container;
}

static void *
get_from_tags (GstPlayer * self, GstPlayerMediaInfo * media_info,
    void *(*func) (GstTagList *))
{
  GList *l;
  void *ret = NULL;

  if (media_info->tags) {
    ret = func (media_info->tags);
    if (ret)
      return ret;
  }

  /* if global tag does not exit then try video and audio streams */
  GST_DEBUG_OBJECT (self, "trying video tags");
  for (l = gst_player_get_video_streams (media_info); l != NULL; l = l->next) {
    GstTagList *tags;

    tags = gst_player_stream_info_get_tags ((GstPlayerStreamInfo *) l->data);
    if (tags)
      ret = func (tags);

    if (ret)
      return ret;
  }

  GST_DEBUG_OBJECT (self, "trying audio tags");
  for (l = gst_player_get_audio_streams (media_info); l != NULL; l = l->next) {
    GstTagList *tags;

    tags = gst_player_stream_info_get_tags ((GstPlayerStreamInfo *) l->data);
    if (tags)
      ret = func (tags);

    if (ret)
      return ret;
  }

  GST_DEBUG_OBJECT (self, "failed to get the information from tags");
  return NULL;
}

static void *
get_cover_sample (GstTagList * tags)
{
  GstSample *cover_sample = NULL;

  gst_tag_list_get_sample (tags, GST_TAG_IMAGE, &cover_sample);
  if (!cover_sample)
    gst_tag_list_get_sample (tags, GST_TAG_PREVIEW_IMAGE, &cover_sample);

  return cover_sample;
}

static GstPlayerMediaInfo *
gst_player_media_info_create (GstPlayer * self)
{
  GstPlayerMediaInfo *media_info;
  GstQuery *query;

  GST_DEBUG_OBJECT (self, "begin");
  media_info = gst_player_media_info_new (self->uri);
  media_info->duration = gst_player_get_duration (self);
  media_info->tags = self->global_tags;
  self->global_tags = NULL;

  query = gst_query_new_seeking (GST_FORMAT_TIME);
  if (gst_element_query (self->playbin, query))
    gst_query_parse_seeking (query, NULL, &media_info->seekable, NULL, NULL);
  gst_query_unref (query);

  /* create audio/video/sub streams */
  gst_player_streams_info_create (self, media_info, "n-video",
      GST_TYPE_PLAYER_VIDEO_INFO);
  gst_player_streams_info_create (self, media_info, "n-audio",
      GST_TYPE_PLAYER_AUDIO_INFO);
  gst_player_streams_info_create (self, media_info, "n-text",
      GST_TYPE_PLAYER_SUBTITLE_INFO);

  media_info->title = get_from_tags (self, media_info, get_title);
  media_info->container =
      get_from_tags (self, media_info, get_container_format);
  media_info->image_sample = get_from_tags (self, media_info, get_cover_sample);

  GST_DEBUG_OBJECT (self, "uri: %s title: %s duration: %" GST_TIME_FORMAT
      " seekable: %s container: %s image_sample %p",
      media_info->uri, media_info->title, GST_TIME_ARGS (media_info->duration),
      media_info->seekable ? "yes" : "no", media_info->container,
      media_info->image_sample);

  GST_DEBUG_OBJECT (self, "end");
  return media_info;
}

static void
tags_changed_cb (GstPlayer * self, gint stream_index, GType type)
{
  GstPlayerStreamInfo *s;

  if (!self->media_info)
    return;

  /* update the stream information */
  g_mutex_lock (&self->lock);
  s = gst_player_stream_info_find (self, self->media_info, type, stream_index);
  gst_player_stream_info_update_tags_and_caps (self, s);
  g_mutex_unlock (&self->lock);

  emit_media_info_updated_signal (self);
}

static void
video_tags_changed_cb (GstElement * playbin, gint stream_index,
    gpointer user_data)
{
  tags_changed_cb (GST_PLAYER (user_data), stream_index,
      GST_TYPE_PLAYER_VIDEO_INFO);
}

static void
audio_tags_changed_cb (GstElement * playbin, gint stream_index,
    gpointer user_data)
{
  tags_changed_cb (GST_PLAYER (user_data), stream_index,
      GST_TYPE_PLAYER_AUDIO_INFO);
}

static void
subtitle_tags_changed_cb (GstElement * playbin, gint stream_index,
    gpointer user_data)
{
  tags_changed_cb (GST_PLAYER (user_data), stream_index,
      GST_TYPE_PLAYER_SUBTITLE_INFO);
}

static void
volume_changed_dispatch (gpointer user_data)
{
  g_signal_emit (user_data, signals[SIGNAL_VOLUME_CHANGED], 0);
  g_object_notify_by_pspec (G_OBJECT (user_data), param_specs[PROP_VOLUME]);
}

static void inline
volume_notify_cb (GObject * obj, GParamSpec * pspec, GstPlayer * self)
{
  if (g_signal_handler_find (self, G_SIGNAL_MATCH_ID,
          signals[SIGNAL_VOLUME_CHANGED], 0, NULL, NULL, NULL) != 0) {
    gst_player_signal_dispatcher_dispatch (self->signal_dispatcher, self,
        volume_changed_dispatch, g_object_ref (self),
        (GDestroyNotify) g_object_unref);
  }
}

static void
mute_changed_dispatch (gpointer user_data)
{
  g_signal_emit (user_data, signals[SIGNAL_MUTE_CHANGED], 0);
  g_object_notify_by_pspec (G_OBJECT (user_data), param_specs[PROP_MUTE]);
}

static void inline
mute_notify_cb (GObject * obj, GParamSpec * pspec, GstPlayer * self)
{
  if (g_signal_handler_find (self, G_SIGNAL_MATCH_ID,
          signals[SIGNAL_MUTE_CHANGED], 0, NULL, NULL, NULL) != 0) {
    gst_player_signal_dispatcher_dispatch (self->signal_dispatcher, self,
        mute_changed_dispatch, g_object_ref (self),
        (GDestroyNotify) g_object_unref);
  }
}

static gpointer
gst_player_main (gpointer data)
{
  GstPlayer *self = GST_PLAYER (data);
  GstBus *bus;
  GSource *source;
  GSource *bus_source;

  GST_TRACE_OBJECT (self, "Starting main thread");

  g_main_context_push_thread_default (self->context);

  source = g_idle_source_new ();
  g_source_set_callback (source, (GSourceFunc) main_loop_running_cb, self,
      NULL);
  g_source_attach (source, self->context);
  g_source_unref (source);

  self->playbin = gst_element_factory_make ("playbin", "playbin");

  if (self->video_renderer) {
    GstElement *video_sink =
        gst_player_video_renderer_create_video_sink (self->video_renderer,
        self);

    if (video_sink)
      g_object_set (self->playbin, "video-sink", video_sink, NULL);
  }

  self->bus = bus = gst_element_get_bus (self->playbin);
  bus_source = gst_bus_create_watch (bus);
  g_source_set_callback (bus_source, (GSourceFunc) gst_bus_async_signal_func,
      NULL, NULL);
  g_source_attach (bus_source, self->context);

  g_signal_connect (G_OBJECT (bus), "message::error", G_CALLBACK (error_cb),
      self);
  g_signal_connect (G_OBJECT (bus), "message::warning", G_CALLBACK (warning_cb),
      self);
  g_signal_connect (G_OBJECT (bus), "message::eos", G_CALLBACK (eos_cb), self);
  g_signal_connect (G_OBJECT (bus), "message::state-changed",
      G_CALLBACK (state_changed_cb), self);
  g_signal_connect (G_OBJECT (bus), "message::buffering",
      G_CALLBACK (buffering_cb), self);
  g_signal_connect (G_OBJECT (bus), "message::clock-lost",
      G_CALLBACK (clock_lost_cb), self);
  g_signal_connect (G_OBJECT (bus), "message::duration-changed",
      G_CALLBACK (duration_changed_cb), self);
  g_signal_connect (G_OBJECT (bus), "message::latency",
      G_CALLBACK (latency_cb), self);
  g_signal_connect (G_OBJECT (bus), "message::request-state",
      G_CALLBACK (request_state_cb), self);
  g_signal_connect (G_OBJECT (bus), "message::element",
      G_CALLBACK (element_cb), self);
  g_signal_connect (G_OBJECT (bus), "message::tag", G_CALLBACK (tags_cb), self);

  g_signal_connect (self->playbin, "video-changed",
      G_CALLBACK (video_changed_cb), self);
  g_signal_connect (self->playbin, "audio-changed",
      G_CALLBACK (audio_changed_cb), self);
  g_signal_connect (self->playbin, "text-changed",
      G_CALLBACK (subtitle_changed_cb), self);

  g_signal_connect (self->playbin, "video-tags-changed",
      G_CALLBACK (video_tags_changed_cb), self);
  g_signal_connect (self->playbin, "audio-tags-changed",
      G_CALLBACK (audio_tags_changed_cb), self);
  g_signal_connect (self->playbin, "text-tags-changed",
      G_CALLBACK (subtitle_tags_changed_cb), self);
  g_signal_connect (self->playbin, "notify::volume",
      G_CALLBACK (volume_notify_cb), self);
  g_signal_connect (self->playbin, "notify::mute",
      G_CALLBACK (mute_notify_cb), self);

  self->target_state = GST_STATE_NULL;
  self->current_state = GST_STATE_NULL;
  change_state (self, GST_PLAYER_STATE_STOPPED);
  self->buffering = 100;
  self->is_eos = FALSE;
  self->is_live = FALSE;

  GST_TRACE_OBJECT (self, "Starting main loop");
  g_main_loop_run (self->loop);
  GST_TRACE_OBJECT (self, "Stopped main loop");

  g_source_destroy (bus_source);
  g_source_unref (bus_source);
  gst_object_unref (bus);

  remove_tick_source (self);
  remove_ready_timeout_source (self);

  g_mutex_lock (&self->lock);
  if (self->media_info) {
    g_object_unref (self->media_info);
    self->media_info = NULL;
  }

  if (self->seek_source)
    g_source_unref (self->seek_source);
  self->seek_source = NULL;
  g_mutex_unlock (&self->lock);

  g_main_context_pop_thread_default (self->context);

  self->target_state = GST_STATE_NULL;
  self->current_state = GST_STATE_NULL;
  if (self->playbin) {
    gst_element_set_state (self->playbin, GST_STATE_NULL);
    gst_object_unref (self->playbin);
    self->playbin = NULL;
  }

  GST_TRACE_OBJECT (self, "Stopped main thread");

  return NULL;
}

static gpointer
gst_player_init_once (gpointer user_data)
{
  gst_init (NULL, NULL);

  GST_DEBUG_CATEGORY_INIT (gst_player_debug, "gst-player", 0, "GstPlayer");
  gst_player_error_quark ();

  return NULL;
}

/**
 * gst_player_new:
 *
 * Returns: a new #GstPlayer instance
 */
GstPlayer *
gst_player_new (void)
{
  return gst_player_new_full (NULL, NULL);
}

/**
 * gst_player_new_full:
 * @video_renderer: (transfer full) (allow-none): GstPlayerVideoRenderer to use
 * @signal_dispatcher: (transfer full) (allow-none): GstPlayerSignalDispatcher to use
 *
 * Creates a new #GstPlayer instance that uses @signal_dispatcher to dispatch
 * signals to some event loop system, or emits signals directly if NULL is
 * passed. See gst_player_g_main_context_signal_dispatcher_new().
 *
 * Video is going to be rendered by @video_renderer, or if %NULL is provided
 * no special video set up will be done and some default handling will be
 * performed.
 *
 * Returns: a new #GstPlayer instance
 */
GstPlayer *
gst_player_new_full (GstPlayerVideoRenderer * video_renderer,
    GstPlayerSignalDispatcher * signal_dispatcher)
{
  static GOnce once = G_ONCE_INIT;
  GstPlayer *self;

  g_once (&once, gst_player_init_once, NULL);

  self =
      g_object_new (GST_TYPE_PLAYER, "video-renderer", video_renderer,
      "signal-dispatcher", signal_dispatcher, NULL);

  if (video_renderer)
    g_object_unref (video_renderer);
  if (signal_dispatcher)
    g_object_unref (signal_dispatcher);

  return self;
}

static gboolean
gst_player_play_internal (gpointer user_data)
{
  GstPlayer *self = GST_PLAYER (user_data);
  GstStateChangeReturn state_ret;

  GST_DEBUG_OBJECT (self, "Play");

  g_mutex_lock (&self->lock);
  if (!self->uri) {
    g_mutex_unlock (&self->lock);
    return G_SOURCE_REMOVE;
  }
  g_mutex_unlock (&self->lock);

  remove_ready_timeout_source (self);
  self->target_state = GST_STATE_PLAYING;

  if (self->current_state < GST_STATE_PAUSED)
    change_state (self, GST_PLAYER_STATE_BUFFERING);

  if (self->current_state >= GST_STATE_PAUSED && !self->is_eos) {
    state_ret = gst_element_set_state (self->playbin, GST_STATE_PLAYING);
  } else {
    state_ret = gst_element_set_state (self->playbin, GST_STATE_PAUSED);
  }

  if (state_ret == GST_STATE_CHANGE_NO_PREROLL) {
    self->is_live = TRUE;
    GST_DEBUG_OBJECT (self, "Pipeline is live");
  }

  if (state_ret == GST_STATE_CHANGE_FAILURE) {
    emit_error (self, g_error_new (GST_PLAYER_ERROR, GST_PLAYER_ERROR_FAILED,
            "Failed to play"));
    return G_SOURCE_REMOVE;
  } else if (state_ret == GST_STATE_CHANGE_NO_PREROLL) {
    self->is_live = TRUE;
    GST_DEBUG_OBJECT (self, "Pipeline is live");
  }

  if (self->is_eos) {
    gboolean ret;

    GST_DEBUG_OBJECT (self, "Was EOS, seeking to beginning");
    self->is_eos = FALSE;
    ret =
        gst_element_seek_simple (self->playbin, GST_FORMAT_TIME,
        GST_SEEK_FLAG_FLUSH, 0);
    if (!ret) {
      GST_ERROR_OBJECT (self, "Seek to beginning failed");
      gst_player_stop_internal (self);
      gst_player_play_internal (self);
    }
  }

  return G_SOURCE_REMOVE;
}

/**
 * gst_player_play:
 * @player: #GstPlayer instance
 *
 * Request to play the loaded stream.
 */
void
gst_player_play (GstPlayer * self)
{
  g_return_if_fail (GST_IS_PLAYER (self));

  g_main_context_invoke_full (self->context, G_PRIORITY_DEFAULT,
      gst_player_play_internal, self, NULL);
}

static gboolean
gst_player_pause_internal (gpointer user_data)
{
  GstPlayer *self = GST_PLAYER (user_data);
  GstStateChangeReturn state_ret;

  GST_DEBUG_OBJECT (self, "Pause");

  g_mutex_lock (&self->lock);
  if (!self->uri) {
    g_mutex_unlock (&self->lock);
    return G_SOURCE_REMOVE;
  }
  g_mutex_unlock (&self->lock);

  tick_cb (self);
  remove_tick_source (self);
  remove_ready_timeout_source (self);

  self->target_state = GST_STATE_PAUSED;

  if (self->current_state < GST_STATE_PAUSED)
    change_state (self, GST_PLAYER_STATE_BUFFERING);

  state_ret = gst_element_set_state (self->playbin, GST_STATE_PAUSED);
  if (state_ret == GST_STATE_CHANGE_FAILURE) {
    emit_error (self, g_error_new (GST_PLAYER_ERROR, GST_PLAYER_ERROR_FAILED,
            "Failed to pause"));
    return G_SOURCE_REMOVE;
  } else if (state_ret == GST_STATE_CHANGE_NO_PREROLL) {
    self->is_live = TRUE;
    GST_DEBUG_OBJECT (self, "Pipeline is live");
  }

  if (self->is_eos) {
    gboolean ret;

    GST_DEBUG_OBJECT (self, "Was EOS, seeking to beginning");
    self->is_eos = FALSE;
    ret =
        gst_element_seek_simple (self->playbin, GST_FORMAT_TIME,
        GST_SEEK_FLAG_FLUSH, 0);
    if (!ret) {
      GST_ERROR_OBJECT (self, "Seek to beginning failed");
      gst_player_stop_internal (self);
      gst_player_pause_internal (self);
    }
  }

  return G_SOURCE_REMOVE;
}

/**
 * gst_player_pause:
 * @player: #GstPlayer instance
 *
 * Pauses the current stream.
 */
void
gst_player_pause (GstPlayer * self)
{
  g_return_if_fail (GST_IS_PLAYER (self));

  g_main_context_invoke_full (self->context, G_PRIORITY_DEFAULT,
      gst_player_pause_internal, self, NULL);
}

static gboolean
gst_player_stop_internal (gpointer user_data)
{
  GstPlayer *self = GST_PLAYER (user_data);

  GST_DEBUG_OBJECT (self, "Stop");

  tick_cb (self);
  remove_tick_source (self);

  add_ready_timeout_source (self);

  self->target_state = GST_STATE_NULL;
  self->current_state = GST_STATE_READY;
  self->is_live = FALSE;
  self->is_eos = FALSE;
  gst_bus_set_flushing (self->bus, TRUE);
  gst_element_set_state (self->playbin, GST_STATE_READY);
  gst_bus_set_flushing (self->bus, FALSE);
  change_state (self, GST_PLAYER_STATE_STOPPED);
  self->buffering = 100;
  g_mutex_lock (&self->lock);
  if (self->media_info) {
    g_object_unref (self->media_info);
    self->media_info = NULL;
  }
  if (self->global_tags) {
    gst_tag_list_unref (self->global_tags);
    self->global_tags = NULL;
  }
  self->seek_pending = FALSE;
  if (self->seek_source) {
    g_source_destroy (self->seek_source);
    g_source_unref (self->seek_source);
    self->seek_source = NULL;
  }
  self->seek_position = GST_CLOCK_TIME_NONE;
  self->last_seek_time = GST_CLOCK_TIME_NONE;
  self->rate = 1.0;
  g_mutex_unlock (&self->lock);

  return G_SOURCE_REMOVE;
}

/**
 * gst_player_stop:
 * @player: #GstPlayer instance
 *
 * Stops playing the current stream and resets to the first position
 * in the stream.
 */
void
gst_player_stop (GstPlayer * self)
{
  g_return_if_fail (GST_IS_PLAYER (self));

  g_main_context_invoke_full (self->context, G_PRIORITY_DEFAULT,
      gst_player_stop_internal, self, NULL);
}

/* Must be called with lock from main context, releases lock! */
static void
gst_player_seek_internal_locked (GstPlayer * self)
{
  gboolean ret;
  GstClockTime position;
  gdouble rate;
  GstStateChangeReturn state_ret;
  GstEvent *s_event;
  GstSeekFlags flags = 0;

  if (self->seek_source) {
    g_source_destroy (self->seek_source);
    g_source_unref (self->seek_source);
    self->seek_source = NULL;
  }

  /* Only seek in PAUSED */
  if (self->current_state < GST_STATE_PAUSED) {
    return;
  } else if (self->current_state != GST_STATE_PAUSED) {
    g_mutex_unlock (&self->lock);
    state_ret = gst_element_set_state (self->playbin, GST_STATE_PAUSED);
    if (state_ret == GST_STATE_CHANGE_FAILURE) {
      emit_error (self, g_error_new (GST_PLAYER_ERROR, GST_PLAYER_ERROR_FAILED,
              "Failed to seek"));
      g_mutex_lock (&self->lock);
      return;
    }
    g_mutex_lock (&self->lock);
    return;
  }

  self->last_seek_time = gst_util_get_timestamp ();
  position = self->seek_position;
  self->seek_position = GST_CLOCK_TIME_NONE;
  self->seek_pending = TRUE;
  rate = self->rate;
  g_mutex_unlock (&self->lock);

  remove_tick_source (self);
  self->is_eos = FALSE;

  flags |= GST_SEEK_FLAG_FLUSH;

#if GST_CHECK_VERSION(1,5,0)
  if (rate != 1.0) {
    flags |= GST_SEEK_FLAG_TRICKMODE;
  }
#endif

  if (rate >= 0.0) {
    s_event = gst_event_new_seek (rate, GST_FORMAT_TIME, flags,
        GST_SEEK_TYPE_SET, position, GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE);
  } else {
    s_event = gst_event_new_seek (rate, GST_FORMAT_TIME, flags,
        GST_SEEK_TYPE_SET, G_GINT64_CONSTANT (0), GST_SEEK_TYPE_SET, position);
  }

  GST_DEBUG_OBJECT (self, "Seek with rate %.2lf to %" GST_TIME_FORMAT,
      rate, GST_TIME_ARGS (position));

  ret = gst_element_send_event (self->playbin, s_event);
  if (!ret)
    emit_error (self, g_error_new (GST_PLAYER_ERROR, GST_PLAYER_ERROR_FAILED,
            "Failed to seek to %" GST_TIME_FORMAT, GST_TIME_ARGS (position)));

  g_mutex_lock (&self->lock);
}

static gboolean
gst_player_seek_internal (gpointer user_data)
{
  GstPlayer *self = GST_PLAYER (user_data);

  g_mutex_lock (&self->lock);
  gst_player_seek_internal_locked (self);
  g_mutex_unlock (&self->lock);

  return G_SOURCE_REMOVE;
}

static gboolean
gst_player_set_rate_internal (gpointer user_data)
{
  GstPlayer *self = user_data;

  g_mutex_lock (&self->lock);

  self->seek_position = gst_player_get_position (self);

  /* If there is no seek being dispatch to the main context currently do that,
   * otherwise we just updated the rate so that it will be taken by
   * the seek handler from the main context instead of the old one.
   */
  if (!self->seek_source) {
    /* If no seek is pending then create new seek source */
    if (!self->seek_pending) {
      self->seek_source = g_idle_source_new ();
      g_source_set_callback (self->seek_source,
          (GSourceFunc) gst_player_seek_internal, self, NULL);
      g_source_attach (self->seek_source, self->context);
    }
  }

  g_mutex_unlock (&self->lock);

  return G_SOURCE_REMOVE;
}

/**
 * gst_player_set_rate:
 * @player: #GstPlayer instance
 * @rate: playback rate
 *
 * Playback at specified rate
 */
void
gst_player_set_rate (GstPlayer * self, gdouble rate)
{
  g_return_if_fail (GST_IS_PLAYER (self));
  g_return_if_fail (rate != 0.0);

  g_mutex_lock (&self->lock);
  self->rate = rate;
  g_mutex_unlock (&self->lock);

  gst_player_set_rate_internal (self);
}

/**
 * gst_player_get_rate:
 * @player: #GstPlayer instance
 *
 * Returns: current playback rate
 */
gdouble
gst_player_get_rate (GstPlayer * self)
{
  g_return_val_if_fail (GST_IS_PLAYER (self), DEFAULT_RATE);

  return self->rate;
}

static gboolean
gst_player_set_position_update_interval_internal (gpointer user_data)
{
  GstPlayer *self = user_data;

  g_mutex_lock (&self->lock);

  if (self->tick_source) {
    remove_tick_source (self);
    add_tick_source (self);
  }

  g_mutex_unlock (&self->lock);

  return G_SOURCE_REMOVE;
}

/**
 * gst_player_set_position_update_interval:
 * @player: #GstPlayer instance
 * @interval: interval in ms
 *
 * Set interval in milliseconds between two position-updated signals.
 * Pass 0 to stop updating the position.
 */
void
gst_player_set_position_update_interval (GstPlayer * self, guint interval)
{
  g_return_if_fail (GST_IS_PLAYER (self));
  g_return_if_fail (interval <= 10000);

  g_mutex_lock (&self->lock);
  self->position_update_interval_ms = interval;
  g_mutex_unlock (&self->lock);

  gst_player_set_position_update_interval_internal (self);
}

/**
 * gst_player_get_position_update_interval:
 * @player: #GstPlayer instance
 *
 * Returns: current position update interval in milliseconds
 */
guint
gst_player_get_position_update_interval (GstPlayer * self)
{
  g_return_val_if_fail (GST_IS_PLAYER (self),
      DEFAULT_POSITION_UPDATE_INTERVAL_MS);

  return self->position_update_interval_ms;
}

/**
 * gst_player_seek:
 * @player: #GstPlayer instance
 * @position: position to seek in nanoseconds
 *
 * Seeks the currently-playing stream to the absolute @position time
 * in nanoseconds.
 */
void
gst_player_seek (GstPlayer * self, GstClockTime position)
{
  g_return_if_fail (GST_IS_PLAYER (self));
  g_return_if_fail (GST_CLOCK_TIME_IS_VALID (position));

  g_mutex_lock (&self->lock);
  if (self->media_info && !self->media_info->seekable) {
    GST_DEBUG_OBJECT (self, "Media is not seekable");
    g_mutex_unlock (&self->lock);
    return;
  }

  self->seek_position = position;

  /* If there is no seek being dispatch to the main context currently do that,
   * otherwise we just updated the seek position so that it will be taken by
   * the seek handler from the main context instead of the old one.
   */
  if (!self->seek_source) {
    GstClockTime now = gst_util_get_timestamp ();

    /* If no seek is pending or it was started more than 250 mseconds ago seek
     * immediately, otherwise wait until the 250 mseconds have passed */
    if (!self->seek_pending || (now - self->last_seek_time > 250 * GST_MSECOND)) {
      self->seek_source = g_idle_source_new ();
      g_source_set_callback (self->seek_source,
          (GSourceFunc) gst_player_seek_internal, self, NULL);
      GST_TRACE_OBJECT (self, "Dispatching seek to position %" GST_TIME_FORMAT,
          GST_TIME_ARGS (position));
      g_source_attach (self->seek_source, self->context);
    } else {
      guint delay = 250000 - (now - self->last_seek_time) / 1000;

      /* Note that last_seek_time must be set to something at this point and
       * it must be smaller than 250 mseconds */
      self->seek_source = g_timeout_source_new (delay);
      g_source_set_callback (self->seek_source,
          (GSourceFunc) gst_player_seek_internal, self, NULL);

      GST_TRACE_OBJECT (self,
          "Delaying seek to position %" GST_TIME_FORMAT " by %u us",
          GST_TIME_ARGS (position), delay);
      g_source_attach (self->seek_source, self->context);
    }
  }
  g_mutex_unlock (&self->lock);
}

/**
 * gst_player_get_uri:
 * @player: #GstPlayer instance
 *
 * Gets the URI of the currently-playing stream.
 *
 * Returns: (transfer full): a string containing the URI of the
 * currently-playing stream. g_free() after usage.
 */
gchar *
gst_player_get_uri (GstPlayer * self)
{
  gchar *val;

  g_return_val_if_fail (GST_IS_PLAYER (self), DEFAULT_URI);

  g_object_get (self, "uri", &val, NULL);

  return val;
}

/**
 * gst_player_set_uri:
 * @player: #GstPlayer instance
 * @uri: next URI to play.
 *
 * Sets the next URI to play.
 */
void
gst_player_set_uri (GstPlayer * self, const gchar * val)
{
  g_return_if_fail (GST_IS_PLAYER (self));

  g_object_set (self, "uri", val, NULL);
}

/**
 * gst_player_get_position:
 * @player: #GstPlayer instance
 *
 * Returns: the absolute position time, in nanoseconds, of the
 * currently-playing stream.
 */
GstClockTime
gst_player_get_position (GstPlayer * self)
{
  GstClockTime val;

  g_return_val_if_fail (GST_IS_PLAYER (self), DEFAULT_POSITION);

  g_object_get (self, "position", &val, NULL);

  return val;
}

/**
 * gst_player_get_duration:
 * @player: #GstPlayer instance
 *
 * Retrieves the duration of the media stream that self represents.
 *
 * Returns: the duration of the currently-playing media stream, in
 * nanoseconds.
 */
GstClockTime
gst_player_get_duration (GstPlayer * self)
{
  GstClockTime val;

  g_return_val_if_fail (GST_IS_PLAYER (self), DEFAULT_DURATION);

  g_object_get (self, "duration", &val, NULL);

  return val;
}

/**
 * gst_player_get_volume:
 * @player: #GstPlayer instance
 *
 * Returns the current volume level, as a percentage between 0 and 1.
 *
 * Returns: the volume as percentage between 0 and 1.
 */
gdouble
gst_player_get_volume (GstPlayer * self)
{
  gdouble val;

  g_return_val_if_fail (GST_IS_PLAYER (self), DEFAULT_VOLUME);

  g_object_get (self, "volume", &val, NULL);

  return val;
}

/**
 * gst_player_set_volume:
 * @player: #GstPlayer instance
 * @val: the new volume level, as a percentage between 0 and 1
 *
 * Sets the volume level of the stream as a percentage between 0 and 1.
 */
void
gst_player_set_volume (GstPlayer * self, gdouble val)
{
  g_return_if_fail (GST_IS_PLAYER (self));

  g_object_set (self, "volume", val, NULL);
}

/**
 * gst_player_get_mute:
 * @player: #GstPlayer instance
 *
 * Returns: %TRUE if the currently-playing stream is muted.
 */
gboolean
gst_player_get_mute (GstPlayer * self)
{
  gboolean val;

  g_return_val_if_fail (GST_IS_PLAYER (self), DEFAULT_MUTE);

  g_object_get (self, "mute", &val, NULL);

  return val;
}

/**
 * gst_player_set_mute:
 * @player: #GstPlayer instance
 * @val: Mute state the should be set
 *
 * %TRUE if the currently-playing stream should be muted.
 */
void
gst_player_set_mute (GstPlayer * self, gboolean val)
{
  g_return_if_fail (GST_IS_PLAYER (self));

  g_object_set (self, "mute", val, NULL);
}

/**
 * gst_player_get_pipeline:
 * @player: #GstPlayer instance
 *
 * Returns: (transfer full): The internal playbin instance
 */
GstElement *
gst_player_get_pipeline (GstPlayer * self)
{
  GstElement *val;

  g_return_val_if_fail (GST_IS_PLAYER (self), NULL);

  g_object_get (self, "pipeline", &val, NULL);

  return val;
}

/**
 * gst_player_get_media_info:
 * @player: #GstPlayer instance
 *
 * A Function to get the current media info #GstPlayerMediaInfo instance.
 *
 * Returns: (transfer full): media info instance.
 *
 * The caller should free it with g_object_unref()
 */
GstPlayerMediaInfo *
gst_player_get_media_info (GstPlayer * self)
{
  GstPlayerMediaInfo *info;

  g_return_val_if_fail (GST_IS_PLAYER (self), NULL);

  if (!self->media_info)
    return NULL;

  g_mutex_lock (&self->lock);
  info = gst_player_media_info_copy (self->media_info);
  g_mutex_unlock (&self->lock);

  return info;
}

/**
 * gst_player_get_current_audio_track:
 * @player: #GstPlayer instance
 *
 * A Function to get current audio #GstPlayerAudioInfo instance.
 *
 * Returns: (transfer full): current audio track.
 *
 * The caller should free it with g_object_unref()
 */
GstPlayerAudioInfo *
gst_player_get_current_audio_track (GstPlayer * self)
{
  GstPlayerAudioInfo *info;

  g_return_val_if_fail (GST_IS_PLAYER (self), NULL);

  if (!is_track_enabled (self, GST_PLAY_FLAG_AUDIO))
    return NULL;

  info = (GstPlayerAudioInfo *) gst_player_stream_info_get_current (self,
      "current-audio", GST_TYPE_PLAYER_AUDIO_INFO);
  return info;
}

/**
 * gst_player_get_current_video_track:
 * @player: #GstPlayer instance
 *
 * A Function to get current video #GstPlayerVideoInfo instance.
 *
 * Returns: (transfer full): current video track.
 *
 * The caller should free it with g_object_unref()
 */
GstPlayerVideoInfo *
gst_player_get_current_video_track (GstPlayer * self)
{
  GstPlayerVideoInfo *info;

  g_return_val_if_fail (GST_IS_PLAYER (self), NULL);

  if (!is_track_enabled (self, GST_PLAY_FLAG_VIDEO))
    return NULL;

  info = (GstPlayerVideoInfo *) gst_player_stream_info_get_current (self,
      "current-video", GST_TYPE_PLAYER_VIDEO_INFO);
  return info;
}

/**
 * gst_player_get_current_subtitle_track:
 * @player: #GstPlayer instance
 *
 * A Function to get current subtitle #GstPlayerSubtitleInfo instance.
 *
 * Returns: (transfer none): current subtitle track.
 *
 * The caller should free it with g_object_unref()
 */
GstPlayerSubtitleInfo *
gst_player_get_current_subtitle_track (GstPlayer * self)
{
  GstPlayerSubtitleInfo *info;

  g_return_val_if_fail (GST_IS_PLAYER (self), NULL);

  if (!is_track_enabled (self, GST_PLAY_FLAG_SUBTITLE))
    return NULL;

  info = (GstPlayerSubtitleInfo *) gst_player_stream_info_get_current (self,
      "current-text", GST_TYPE_PLAYER_SUBTITLE_INFO);
  return info;
}

/**
 * gst_player_set_audio_track:
 * @player: #GstPlayer instance
 * @stream_index: stream index
 *
 * Returns: %TRUE or %FALSE
 *
 * Sets the audio track @stream_idex.
 */
gboolean
gst_player_set_audio_track (GstPlayer * self, gint stream_index)
{
  GstPlayerStreamInfo *info;

  g_return_val_if_fail (GST_IS_PLAYER (self), 0);

  g_mutex_lock (&self->lock);
  info = gst_player_stream_info_find (self, self->media_info,
      GST_TYPE_PLAYER_AUDIO_INFO, stream_index);
  g_mutex_unlock (&self->lock);
  if (!info) {
    GST_ERROR_OBJECT (self, "invalid audio stream index %d", stream_index);
    return FALSE;
  }

  g_object_set (G_OBJECT (self->playbin), "current-audio", stream_index, NULL);
  GST_DEBUG_OBJECT (self, "set stream index '%d'", stream_index);
  return TRUE;
}

/**
 * gst_player_set_video_track:
 * @player: #GstPlayer instance
 * @stream_index: stream index
 *
 * Returns: %TRUE or %FALSE
 *
 * Sets the video track @stream_index.
 */
gboolean
gst_player_set_video_track (GstPlayer * self, gint stream_index)
{
  GstPlayerStreamInfo *info;

  g_return_val_if_fail (GST_IS_PLAYER (self), 0);

  /* check if stream_index exist in our internal media_info list */
  g_mutex_lock (&self->lock);
  info = gst_player_stream_info_find (self, self->media_info,
      GST_TYPE_PLAYER_VIDEO_INFO, stream_index);
  g_mutex_unlock (&self->lock);
  if (!info) {
    GST_ERROR_OBJECT (self, "invalid video stream index %d", stream_index);
    return FALSE;
  }

  g_object_set (G_OBJECT (self->playbin), "current-video", stream_index, NULL);
  GST_DEBUG_OBJECT (self, "set stream index '%d'", stream_index);
  return TRUE;
}

/**
 * gst_player_set_subtitle_track:
 * @player: #GstPlayer instance
 * @stream_index: stream index
 *
 * Returns: %TRUE or %FALSE
 *
 * Sets the subtitle strack @stream_index.
 */
gboolean
gst_player_set_subtitle_track (GstPlayer * self, gint stream_index)
{
  GstPlayerStreamInfo *info;

  g_return_val_if_fail (GST_IS_PLAYER (self), 0);

  g_mutex_lock (&self->lock);
  info = gst_player_stream_info_find (self, self->media_info,
      GST_TYPE_PLAYER_SUBTITLE_INFO, stream_index);
  g_mutex_unlock (&self->lock);
  if (!info) {
    GST_ERROR_OBJECT (self, "invalid subtitle stream index %d", stream_index);
    return FALSE;
  }

  g_object_set (G_OBJECT (self->playbin), "current-text", stream_index, NULL);
  GST_DEBUG_OBJECT (self, "set stream index '%d'", stream_index);
  return TRUE;
}

/**
 * gst_player_set_audio_track_enabled:
 * @player: #GstPlayer instance
 * @enabled: TRUE or FALSE
 *
 * Enable or disable the current audio track.
 */
void
gst_player_set_audio_track_enabled (GstPlayer * self, gboolean enabled)
{
  g_return_if_fail (GST_IS_PLAYER (self));

  if (enabled)
    player_set_flag (self, GST_PLAY_FLAG_AUDIO);
  else
    player_clear_flag (self, GST_PLAY_FLAG_AUDIO);

  GST_DEBUG_OBJECT (self, "track is '%s'", enabled ? "Enabled" : "Disabled");
}

/**
 * gst_player_set_video_track_enabled:
 * @player: #GstPlayer instance
 * @enabled: TRUE or FALSE
 *
 * Enable or disable the current video track.
 */
void
gst_player_set_video_track_enabled (GstPlayer * self, gboolean enabled)
{
  g_return_if_fail (GST_IS_PLAYER (self));

  if (enabled)
    player_set_flag (self, GST_PLAY_FLAG_VIDEO);
  else
    player_clear_flag (self, GST_PLAY_FLAG_VIDEO);

  GST_DEBUG_OBJECT (self, "track is '%s'", enabled ? "Enabled" : "Disabled");
}

/**
 * gst_player_set_subtitle_track_enabled:
 * @player: #GstPlayer instance
 * @enabled: TRUE or FALSE
 *
 * Enable or disable the current subtitle track.
 */
void
gst_player_set_subtitle_track_enabled (GstPlayer * self, gboolean enabled)
{
  g_return_if_fail (GST_IS_PLAYER (self));

  if (enabled)
    player_set_flag (self, GST_PLAY_FLAG_SUBTITLE);
  else
    player_clear_flag (self, GST_PLAY_FLAG_SUBTITLE);

  GST_DEBUG_OBJECT (self, "track is '%s'", enabled ? "Enabled" : "Disabled");
}

/**
 * gst_player_set_subtitle_uri:
 * @player: #GstPlayer instance
 * @uri: subtitle URI
 *
 * Returns: %TRUE or %FALSE
 *
 * Sets the external subtitle URI.
 */
gboolean
gst_player_set_subtitle_uri (GstPlayer * self, const gchar * suburi)
{
  g_return_val_if_fail (GST_IS_PLAYER (self), FALSE);

  g_mutex_lock (&self->lock);
  if (self->suburi)
    g_free (self->suburi);
  self->suburi = g_strdup (suburi);
  g_mutex_unlock (&self->lock);

  gst_player_set_suburi_internal (self);

  return TRUE;
}

/**
 * gst_player_get_subtitle_uri:
 * @player: #GstPlayer instance
 *
 * current subtitle URI
 *
 * Returns: (transfer full): URI of the current external subtitle.
 *   g_free() after usage.
 */
gchar *
gst_player_get_subtitle_uri (GstPlayer * self)
{
  gchar *val = NULL;

  g_return_val_if_fail (GST_IS_PLAYER (self), NULL);

  g_object_get (self, "suburi", &val, NULL);

  return val;
}

G_DEFINE_BOXED_TYPE (GstPlayerVisualization, gst_player_visualization,
    (GBoxedCopyFunc) gst_player_visualization_copy,
    (GBoxedFreeFunc) gst_player_visualization_free);

/**
 * gst_player_visualization_free:
 * @vis: #GstPlayerVisualization instance
 *
 * Frees #GstPlayerVisualization allocated using g_new() or
 * gst_player_visualization_copy().
 */
void
gst_player_visualization_free (GstPlayerVisualization * vis)
{
  g_return_if_fail (vis != NULL);

  g_free (vis->name);
  g_free (vis->description);
  g_free (vis);
}

/**
 * gst_player_visualization_copy:
 * @vis: #GstPlayerVisualization instance
 *
 * Makes a copy of the #GstPlayerVisualization. The result must be
 * freed using gst_player_visualization_free().
 *
 * Returns: (transfer full): an allocated copy of @vis.
 */
GstPlayerVisualization *
gst_player_visualization_copy (const GstPlayerVisualization * vis)
{
  GstPlayerVisualization *ret;

  g_return_val_if_fail (vis != NULL, NULL);

  ret = g_new0 (GstPlayerVisualization, 1);
  ret->name = vis->name ? g_strdup (vis->name) : NULL;
  ret->description = vis->description ? g_strdup (vis->description) : NULL;

  return ret;
}

/**
 * gst_player_visualizations_free:
 * @viss: a %NULL terminated array of #GstPlayerVisualization to free
 *
 * Frees a %NULL terminated array of #GstPlayerVisualization.
 */
void
gst_player_visualizations_free (GstPlayerVisualization ** viss)
{
  GstPlayerVisualization **p;

  g_return_if_fail (viss != NULL);

  p = viss;
  while (*p) {
    g_free ((*p)->name);
    g_free ((*p)->description);
    g_free (*p);
    p++;
  }
  g_free (viss);
}

static void
gst_player_update_visualization_list (void)
{
  GList *features;
  GList *l;
  guint32 cookie;
  GstPlayerVisualization *vis;

  g_mutex_lock (&vis_lock);

  /* check if we need to update the list */
  cookie = gst_registry_get_feature_list_cookie (gst_registry_get ());
  if (vis_cookie == cookie) {
    g_mutex_unlock (&vis_lock);
    return;
  }

  /* if update is needed then first free the existing list */
  while ((vis = g_queue_pop_head (&vis_list)))
    gst_player_visualization_free (vis);

  features = gst_registry_get_feature_list (gst_registry_get (),
      GST_TYPE_ELEMENT_FACTORY);

  for (l = features; l; l = l->next) {
    GstPluginFeature *feature = l->data;
    const gchar *klass;

    klass = gst_element_factory_get_metadata (GST_ELEMENT_FACTORY (feature),
        GST_ELEMENT_METADATA_KLASS);

    if (strstr (klass, "Visualization")) {
      vis = g_new0 (GstPlayerVisualization, 1);

      vis->name = g_strdup (gst_plugin_feature_get_name (feature));
      vis->description =
          g_strdup (gst_element_factory_get_metadata (GST_ELEMENT_FACTORY
              (feature), GST_ELEMENT_METADATA_DESCRIPTION));
      g_queue_push_tail (&vis_list, vis);
    }
  }
  gst_plugin_feature_list_free (features);

  vis_cookie = cookie;

  g_mutex_unlock (&vis_lock);
}

/**
 * gst_player_visualizations_get:
 *
 * Returns: (transfer full) (array zero-terminated=1) (element-type GstPlayerVisualization):
 *  a %NULL terminated array containing all available
 *  visualizations. Use gst_player_visualizations_free() after
 *  usage.
 */
GstPlayerVisualization **
gst_player_visualizations_get (void)
{
  gint i = 0;
  GList *l;
  GstPlayerVisualization **ret;

  gst_player_update_visualization_list ();

  g_mutex_lock (&vis_lock);
  ret = g_new0 (GstPlayerVisualization *, g_queue_get_length (&vis_list) + 1);
  for (l = vis_list.head; l; l = l->next)
    ret[i++] = gst_player_visualization_copy (l->data);
  g_mutex_unlock (&vis_lock);

  return ret;
}

/**
 * gst_player_set_visualization:
 * @player: #GstPlayer instance
 * @name: visualization element obtained from
 * #gst_player_visualizations_get()
 *
 * Returns: %TRUE if the visualizations was set correctly. Otherwise,
 * %FALSE.
 */
gboolean
gst_player_set_visualization (GstPlayer * self, const gchar * name)
{
  g_return_val_if_fail (GST_IS_PLAYER (self), FALSE);

  g_mutex_lock (&self->lock);
  if (self->current_vis_element) {
    gst_object_unref (self->current_vis_element);
    self->current_vis_element = NULL;
  }

  if (name) {
    self->current_vis_element = gst_element_factory_make (name, NULL);
    if (!self->current_vis_element)
      goto error_no_element;
    gst_object_ref_sink (self->current_vis_element);
  }
  g_object_set (self->playbin, "vis-plugin", self->current_vis_element, NULL);

  g_mutex_unlock (&self->lock);
  GST_DEBUG_OBJECT (self, "set vis-plugin to '%s'", name);

  return TRUE;

error_no_element:
  g_mutex_unlock (&self->lock);
  GST_WARNING_OBJECT (self, "could not find visualization '%s'", name);
  return FALSE;
}

/**
 * gst_player_get_current_visualization:
 * @player: #GstPlayer instance
 *
 * Returns: (transfer full): Name of the currently enabled visualization.
 *   g_free() after usage.
 */
gchar *
gst_player_get_current_visualization (GstPlayer * self)
{
  gchar *name = NULL;
  GstElement *vis_plugin = NULL;

  g_return_val_if_fail (GST_IS_PLAYER (self), NULL);

  if (!is_track_enabled (self, GST_PLAY_FLAG_VIS))
    return NULL;

  g_object_get (self->playbin, "vis-plugin", &vis_plugin, NULL);

  if (vis_plugin) {
    GstElementFactory *factory = gst_element_get_factory (vis_plugin);
    if (factory)
      name = g_strdup (gst_plugin_feature_get_name (factory));
    gst_object_unref (vis_plugin);
  }

  GST_DEBUG_OBJECT (self, "vis-plugin '%s' %p", name, vis_plugin);

  return name;
}

/**
 * gst_player_set_visualization_enabled:
 * @player: #GstPlayer instance
 * @enabled: TRUE or FALSE
 *
 * Enable or disable the visualization.
 */
void
gst_player_set_visualization_enabled (GstPlayer * self, gboolean enabled)
{
  g_return_if_fail (GST_IS_PLAYER (self));

  if (enabled)
    player_set_flag (self, GST_PLAY_FLAG_VIS);
  else
    player_clear_flag (self, GST_PLAY_FLAG_VIS);

  GST_DEBUG_OBJECT (self, "visualization is '%s'",
      enabled ? "Enabled" : "Disabled");
}

struct CBChannelMap
{
  const gchar *label;           /* channel label name */
  const gchar *name;            /* get_name () */
};

static const struct CBChannelMap cb_channel_map[] = {
  /* GST_PLAYER_COLOR_BALANCE_BRIGHTNESS */ {"BRIGHTNESS", "brightness"},
  /* GST_PLAYER_COLOR_BALANCE_CONTRAST   */ {"CONTRAST", "contrast"},
  /* GST_PLAYER_COLOR_BALANCE_SATURATION */ {"SATURATION", "saturation"},
  /* GST_PLAYER_COLOR_BALANCE_HUE        */ {"HUE", "hue"},
};

static GstColorBalanceChannel *
gst_player_color_balance_find_channel (GstPlayer * self,
    GstPlayerColorBalanceType type)
{
  GstColorBalanceChannel *channel;
  const GList *l, *channels;

  if (type < GST_PLAYER_COLOR_BALANCE_BRIGHTNESS ||
      type > GST_PLAYER_COLOR_BALANCE_HUE)
    return NULL;

  channels =
      gst_color_balance_list_channels (GST_COLOR_BALANCE (self->playbin));
  for (l = channels; l; l = l->next) {
    channel = l->data;
    if (g_strrstr (channel->label, cb_channel_map[type].label))
      return channel;
  }

  return NULL;
}

/**
 * gst_player_has_color_balance:
 * @player:#GstPlayer instance
 *
 * Checks whether the @player has color balance support available.
 *
 * Returns: %TRUE if @player has color balance support. Otherwise,
 *   %FALSE.
 */
gboolean
gst_player_has_color_balance (GstPlayer * self)
{
  const GList *channels;

  g_return_val_if_fail (GST_IS_PLAYER (self), FALSE);

  if (!GST_IS_COLOR_BALANCE (self->playbin))
    return FALSE;

  channels =
      gst_color_balance_list_channels (GST_COLOR_BALANCE (self->playbin));
  return (channels != NULL);
}

/**
 * gst_player_set_color_balance:
 * @player: #GstPlayer instance
 * @type: #GstPlayerColorBalanceType
 * @value: The new value for the @type, ranged [0,1]
 *
 * Sets the current value of the indicated channel @type to the passed
 * value.
 */
void
gst_player_set_color_balance (GstPlayer * self, GstPlayerColorBalanceType type,
    gdouble value)
{
  GstColorBalanceChannel *channel;
  gdouble new_val;

  g_return_if_fail (GST_IS_PLAYER (self));
  g_return_if_fail (value >= 0.0 && value <= 1.0);

  if (!GST_IS_COLOR_BALANCE (self->playbin))
    return;

  channel = gst_player_color_balance_find_channel (self, type);
  if (!channel)
    return;

  value = CLAMP (value, 0.0, 1.0);

  /* Convert to channel range */
  new_val = channel->min_value + value * ((gdouble) channel->max_value -
      (gdouble) channel->min_value);

  gst_color_balance_set_value (GST_COLOR_BALANCE (self->playbin), channel,
      new_val);
}

/**
 * gst_player_get_color_balance:
 * @player: #GstPlayer instance
 * @type: #GstPlayerColorBalanceType
 *
 * Retrieve the current value of the indicated @type.
 *
 * Returns: The current value of @type, between [0,1]. In case of
 *   error -1 is returned.
 */
gdouble
gst_player_get_color_balance (GstPlayer * self, GstPlayerColorBalanceType type)
{
  GstColorBalanceChannel *channel;
  gint value;

  g_return_val_if_fail (GST_IS_PLAYER (self), -1);

  if (!GST_IS_COLOR_BALANCE (self->playbin))
    return -1;

  channel = gst_player_color_balance_find_channel (self, type);
  if (!channel)
    return -1;

  value = gst_color_balance_get_value (GST_COLOR_BALANCE (self->playbin),
      channel);

  return ((gdouble) value -
      (gdouble) channel->min_value) / ((gdouble) channel->max_value -
      (gdouble) channel->min_value);
}

#define C_ENUM(v) ((gint) v)
#define C_FLAGS(v) ((guint) v)

GType
gst_player_color_balance_type_get_type (void)
{
  static gsize id = 0;
  static const GEnumValue values[] = {
    {C_ENUM (GST_PLAYER_COLOR_BALANCE_HUE), "GST_PLAYER_COLOR_BALANCE_HUE",
        "hue"},
    {C_ENUM (GST_PLAYER_COLOR_BALANCE_BRIGHTNESS),
        "GST_PLAYER_COLOR_BALANCE_BRIGHTNESS", "brightness"},
    {C_ENUM (GST_PLAYER_COLOR_BALANCE_SATURATION),
        "GST_PLAYER_COLOR_BALANCE_SATURATION", "saturation"},
    {C_ENUM (GST_PLAYER_COLOR_BALANCE_CONTRAST),
        "GST_PLAYER_COLOR_BALANCE_CONTRAST", "contrast"},
    {0, NULL, NULL}
  };

  if (g_once_init_enter (&id)) {
    GType tmp = g_enum_register_static ("GstPlayerColorBalanceType", values);
    g_once_init_leave (&id, tmp);
  }

  return (GType) id;
}

/**
 * gst_player_color_balance_type_get_name:
 * @type: a #GstPlayerColorBalanceType
 *
 * Gets a string representing the given color balance type.
 *
 * Returns: (transfer none): a string with the name of the color
 *   balance type.
 */
const gchar *
gst_player_color_balance_type_get_name (GstPlayerColorBalanceType type)
{
  g_return_val_if_fail (type >= GST_PLAYER_COLOR_BALANCE_BRIGHTNESS &&
      type <= GST_PLAYER_COLOR_BALANCE_HUE, NULL);

  return cb_channel_map[type].name;
}

GType
gst_player_state_get_type (void)
{
  static gsize id = 0;
  static const GEnumValue values[] = {
    {C_ENUM (GST_PLAYER_STATE_STOPPED), "GST_PLAYER_STATE_STOPPED", "stopped"},
    {C_ENUM (GST_PLAYER_STATE_BUFFERING), "GST_PLAYER_STATE_BUFFERING",
        "buffering"},
    {C_ENUM (GST_PLAYER_STATE_PAUSED), "GST_PLAYER_STATE_PAUSED", "paused"},
    {C_ENUM (GST_PLAYER_STATE_PLAYING), "GST_PLAYER_STATE_PLAYING", "playing"},
    {0, NULL, NULL}
  };

  if (g_once_init_enter (&id)) {
    GType tmp = g_enum_register_static ("GstPlayerState", values);
    g_once_init_leave (&id, tmp);
  }

  return (GType) id;
}

/**
 * gst_player_state_get_name:
 * @state: a #GstPlayerState
 *
 * Gets a string representing the given state.
 *
 * Returns: (transfer none): a string with the name of the state.
 */
const gchar *
gst_player_state_get_name (GstPlayerState state)
{
  switch (state) {
    case GST_PLAYER_STATE_STOPPED:
      return "stopped";
    case GST_PLAYER_STATE_BUFFERING:
      return "buffering";
    case GST_PLAYER_STATE_PAUSED:
      return "paused";
    case GST_PLAYER_STATE_PLAYING:
      return "playing";
  }

  g_assert_not_reached ();
  return NULL;
}

GType
gst_player_error_get_type (void)
{
  static gsize id = 0;
  static const GEnumValue values[] = {
    {C_ENUM (GST_PLAYER_ERROR_FAILED), "GST_PLAYER_ERROR_FAILED", "failed"},
    {0, NULL, NULL}
  };

  if (g_once_init_enter (&id)) {
    GType tmp = g_enum_register_static ("GstPlayerError", values);
    g_once_init_leave (&id, tmp);
  }

  return (GType) id;
}

/**
 * gst_player_error_get_name:
 * @error: a #GstPlayerError
 *
 * Gets a string representing the given error.
 *
 * Returns: (transfer none): a string with the given error.
 */
const gchar *
gst_player_error_get_name (GstPlayerError error)
{
  switch (error) {
    case GST_PLAYER_ERROR_FAILED:
      return "failed";
  }

  g_assert_not_reached ();
  return NULL;
}

G_DEFINE_INTERFACE (GstPlayerSignalDispatcher, gst_player_signal_dispatcher,
    G_TYPE_OBJECT);

static void
gst_player_signal_dispatcher_default_init (GstPlayerSignalDispatcherInterface *
    iface)
{

}

static void
gst_player_signal_dispatcher_dispatch (GstPlayerSignalDispatcher * self,
    GstPlayer * player, void (*emitter) (gpointer data), gpointer data,
    GDestroyNotify destroy)
{
  GstPlayerSignalDispatcherInterface *iface;

  if (!self) {
    emitter (data);
    if (destroy)
      destroy (data);
    return;
  }

  g_return_if_fail (GST_IS_PLAYER_SIGNAL_DISPATCHER (self));
  iface = GST_PLAYER_SIGNAL_DISPATCHER_GET_INTERFACE (self);
  g_return_if_fail (iface->dispatch != NULL);

  iface->dispatch (self, player, emitter, data, destroy);
}

struct _GstPlayerGMainContextSignalDispatcher
{
  GObject parent;
  GMainContext *application_context;
};

struct _GstPlayerGMainContextSignalDispatcherClass
{
  GObjectClass parent_class;
};

static void
    gst_player_g_main_context_signal_dispatcher_interface_init
    (GstPlayerSignalDispatcherInterface * iface);

enum
{
  G_MAIN_CONTEXT_SIGNAL_DISPATCHER_PROP_0,
  G_MAIN_CONTEXT_SIGNAL_DISPATCHER_PROP_APPLICATION_CONTEXT,
  G_MAIN_CONTEXT_SIGNAL_DISPATCHER_PROP_LAST
};

G_DEFINE_TYPE_WITH_CODE (GstPlayerGMainContextSignalDispatcher,
    gst_player_g_main_context_signal_dispatcher, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE (GST_TYPE_PLAYER_SIGNAL_DISPATCHER,
        gst_player_g_main_context_signal_dispatcher_interface_init));

static GParamSpec
    * g_main_context_signal_dispatcher_param_specs
    [G_MAIN_CONTEXT_SIGNAL_DISPATCHER_PROP_LAST] = { NULL, };

static void
gst_player_g_main_context_signal_dispatcher_finalize (GObject * object)
{
  GstPlayerGMainContextSignalDispatcher *self =
      GST_PLAYER_G_MAIN_CONTEXT_SIGNAL_DISPATCHER (object);

  if (self->application_context)
    g_main_context_unref (self->application_context);

  G_OBJECT_CLASS
      (gst_player_g_main_context_signal_dispatcher_parent_class)->finalize
      (object);
}

static void
gst_player_g_main_context_signal_dispatcher_set_property (GObject * object,
    guint prop_id, const GValue * value, GParamSpec * pspec)
{
  GstPlayerGMainContextSignalDispatcher *self =
      GST_PLAYER_G_MAIN_CONTEXT_SIGNAL_DISPATCHER (object);

  switch (prop_id) {
    case G_MAIN_CONTEXT_SIGNAL_DISPATCHER_PROP_APPLICATION_CONTEXT:
      self->application_context = g_value_dup_boxed (value);
      if (!self->application_context)
        self->application_context = g_main_context_ref_thread_default ();
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_player_g_main_context_signal_dispatcher_get_property (GObject * object,
    guint prop_id, GValue * value, GParamSpec * pspec)
{
  GstPlayerGMainContextSignalDispatcher *self =
      GST_PLAYER_G_MAIN_CONTEXT_SIGNAL_DISPATCHER (object);

  switch (prop_id) {
    case G_MAIN_CONTEXT_SIGNAL_DISPATCHER_PROP_APPLICATION_CONTEXT:
      g_value_set_boxed (value, self->application_context);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
    gst_player_g_main_context_signal_dispatcher_class_init
    (GstPlayerGMainContextSignalDispatcherClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->finalize =
      gst_player_g_main_context_signal_dispatcher_finalize;
  gobject_class->set_property =
      gst_player_g_main_context_signal_dispatcher_set_property;
  gobject_class->get_property =
      gst_player_g_main_context_signal_dispatcher_get_property;

  g_main_context_signal_dispatcher_param_specs
      [G_MAIN_CONTEXT_SIGNAL_DISPATCHER_PROP_APPLICATION_CONTEXT] =
      g_param_spec_boxed ("application-context", "Application Context",
      "Application GMainContext to dispatch signals to", G_TYPE_MAIN_CONTEXT,
      G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (gobject_class,
      G_MAIN_CONTEXT_SIGNAL_DISPATCHER_PROP_LAST,
      g_main_context_signal_dispatcher_param_specs);
}

static void
    gst_player_g_main_context_signal_dispatcher_init
    (GstPlayerGMainContextSignalDispatcher * self)
{
}

typedef struct
{
  void (*emitter) (gpointer data);
  gpointer data;
  GDestroyNotify destroy;
} GMainContextSignalDispatcherData;

static gboolean
g_main_context_signal_dispatcher_dispatch_gsourcefunc (gpointer user_data)
{
  GMainContextSignalDispatcherData *data = user_data;

  data->emitter (data->data);

  return G_SOURCE_REMOVE;
}

static void
g_main_context_signal_dispatcher_dispatch_destroy (gpointer user_data)
{
  GMainContextSignalDispatcherData *data = user_data;

  if (data->destroy)
    data->destroy (data->data);
  g_free (data);
}

static void
gst_player_g_main_context_signal_dispatcher_dispatch (GstPlayerSignalDispatcher
    * iface, GstPlayer * player, void (*emitter) (gpointer data), gpointer data,
    GDestroyNotify destroy)
{
  GstPlayerGMainContextSignalDispatcher *self =
      GST_PLAYER_G_MAIN_CONTEXT_SIGNAL_DISPATCHER (iface);
  GMainContextSignalDispatcherData *gsourcefunc_data =
      g_new (GMainContextSignalDispatcherData, 1);

  gsourcefunc_data->emitter = emitter;
  gsourcefunc_data->data = data;
  gsourcefunc_data->destroy = destroy;

  g_main_context_invoke_full (self->application_context,
      G_PRIORITY_DEFAULT, g_main_context_signal_dispatcher_dispatch_gsourcefunc,
      gsourcefunc_data, g_main_context_signal_dispatcher_dispatch_destroy);
}

static void
    gst_player_g_main_context_signal_dispatcher_interface_init
    (GstPlayerSignalDispatcherInterface * iface)
{
  iface->dispatch = gst_player_g_main_context_signal_dispatcher_dispatch;
}

/**
 * gst_player_g_main_context_signal_dispatcher_new:
 * @application_context: (allow-none): GMainContext to use or %NULL
 *
 * Returns: (transfer full):
 */
GstPlayerSignalDispatcher *
gst_player_g_main_context_signal_dispatcher_new (GMainContext *
    application_context)
{
  return g_object_new (GST_TYPE_PLAYER_G_MAIN_CONTEXT_SIGNAL_DISPATCHER,
      "application-context", application_context, NULL);
}

G_DEFINE_INTERFACE (GstPlayerVideoRenderer, gst_player_video_renderer,
    G_TYPE_OBJECT);

static void
gst_player_video_renderer_default_init (GstPlayerVideoRendererInterface * iface)
{

}

static GstElement *
gst_player_video_renderer_create_video_sink (GstPlayerVideoRenderer * self,
    GstPlayer * player)
{
  GstPlayerVideoRendererInterface *iface;

  g_return_val_if_fail (GST_IS_PLAYER_VIDEO_RENDERER (self), NULL);
  iface = GST_PLAYER_VIDEO_RENDERER_GET_INTERFACE (self);
  g_return_val_if_fail (iface->create_video_sink != NULL, NULL);

  return iface->create_video_sink (self, player);
}

struct _GstPlayerVideoOverlayVideoRenderer
{
  GObject parent;

  GstVideoOverlay *video_overlay;
  gpointer window_handle;
};

struct _GstPlayerVideoOverlayVideoRendererClass
{
  GObjectClass parent_class;
};

static void
    gst_player_video_overlay_video_renderer_interface_init
    (GstPlayerVideoRendererInterface * iface);

enum
{
  VIDEO_OVERLAY_VIDEO_RENDERER_PROP_0,
  VIDEO_OVERLAY_VIDEO_RENDERER_PROP_WINDOW_HANDLE,
  VIDEO_OVERLAY_VIDEO_RENDERER_PROP_LAST
};

G_DEFINE_TYPE_WITH_CODE (GstPlayerVideoOverlayVideoRenderer,
    gst_player_video_overlay_video_renderer, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE (GST_TYPE_PLAYER_VIDEO_RENDERER,
        gst_player_video_overlay_video_renderer_interface_init));

static GParamSpec
    * video_overlay_video_renderer_param_specs
    [VIDEO_OVERLAY_VIDEO_RENDERER_PROP_LAST] = { NULL, };

static void
gst_player_video_overlay_video_renderer_set_property (GObject * object,
    guint prop_id, const GValue * value, GParamSpec * pspec)
{
  GstPlayerVideoOverlayVideoRenderer *self =
      GST_PLAYER_VIDEO_OVERLAY_VIDEO_RENDERER (object);

  switch (prop_id) {
    case VIDEO_OVERLAY_VIDEO_RENDERER_PROP_WINDOW_HANDLE:
      self->window_handle = g_value_get_pointer (value);
      if (self->video_overlay)
        gst_video_overlay_set_window_handle (self->video_overlay,
            (guintptr) self->window_handle);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_player_video_overlay_video_renderer_get_property (GObject * object,
    guint prop_id, GValue * value, GParamSpec * pspec)
{
  GstPlayerVideoOverlayVideoRenderer *self =
      GST_PLAYER_VIDEO_OVERLAY_VIDEO_RENDERER (object);

  switch (prop_id) {
    case VIDEO_OVERLAY_VIDEO_RENDERER_PROP_WINDOW_HANDLE:
      g_value_set_pointer (value, self->window_handle);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_player_video_overlay_video_renderer_finalize (GObject * object)
{
  GstPlayerVideoOverlayVideoRenderer *self =
      GST_PLAYER_VIDEO_OVERLAY_VIDEO_RENDERER (object);

  if (self->video_overlay)
    gst_object_unref (self->video_overlay);

  G_OBJECT_CLASS
      (gst_player_video_overlay_video_renderer_parent_class)->finalize (object);
}

static void
    gst_player_video_overlay_video_renderer_class_init
    (GstPlayerVideoOverlayVideoRendererClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->set_property =
      gst_player_video_overlay_video_renderer_set_property;
  gobject_class->get_property =
      gst_player_video_overlay_video_renderer_get_property;
  gobject_class->finalize = gst_player_video_overlay_video_renderer_finalize;

  video_overlay_video_renderer_param_specs
      [VIDEO_OVERLAY_VIDEO_RENDERER_PROP_WINDOW_HANDLE] =
      g_param_spec_pointer ("window-handle", "Window Handle",
      "Window handle to embed the video into",
      G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (gobject_class,
      VIDEO_OVERLAY_VIDEO_RENDERER_PROP_LAST,
      video_overlay_video_renderer_param_specs);
}

static void
    gst_player_video_overlay_video_renderer_init
    (GstPlayerVideoOverlayVideoRenderer * self)
{
}

static GstElement *gst_player_video_overlay_video_renderer_create_video_sink
    (GstPlayerVideoRenderer * iface, GstPlayer * player)
{
  GstElement *video_overlay;
  GstPlayerVideoOverlayVideoRenderer *self =
      GST_PLAYER_VIDEO_OVERLAY_VIDEO_RENDERER (iface);

  if (self->video_overlay)
    gst_object_unref (self->video_overlay);

  video_overlay = gst_player_get_pipeline (player);
  g_return_val_if_fail (GST_IS_VIDEO_OVERLAY (video_overlay), NULL);

  self->video_overlay = GST_VIDEO_OVERLAY (video_overlay);

  gst_video_overlay_set_window_handle (self->video_overlay,
      (guintptr) self->window_handle);

  return NULL;
}

static void
    gst_player_video_overlay_video_renderer_interface_init
    (GstPlayerVideoRendererInterface * iface)
{
  iface->create_video_sink =
      gst_player_video_overlay_video_renderer_create_video_sink;
}

/**
 * gst_player_video_overlay_video_renderer_new:
 * @window_handle: (allow-none): Window handle to use or %NULL
 *
 * Returns: (transfer full):
 */
GstPlayerVideoRenderer *
gst_player_video_overlay_video_renderer_new (gpointer window_handle)
{
  return g_object_new (GST_TYPE_PLAYER_VIDEO_OVERLAY_VIDEO_RENDERER,
      "window-handle", window_handle, NULL);
}

/**
 * gst_player_video_overlay_video_renderer_set_window_handle:
 * @self: #GstPlayerVideoRenderer instance
 * @window_handle: handle referencing to the platform specific window
 *
 * Sets the platform specific window handle into which the video
 * should be rendered
 **/
void gst_player_video_overlay_video_renderer_set_window_handle
    (GstPlayerVideoOverlayVideoRenderer * self, gpointer window_handle)
{
  g_return_if_fail (GST_IS_PLAYER_VIDEO_OVERLAY_VIDEO_RENDERER (self));

  g_object_set (self, "window-handle", window_handle, NULL);
}

/**
 * gst_player_video_overlay_video_renderer_get_window_handle:
 * @self: #GstPlayerVideoRenderer instance
 *
 * Returns: (transfer none): The currently set, platform specific window
 * handle
 */
gpointer
    gst_player_video_overlay_video_renderer_get_window_handle
    (GstPlayerVideoOverlayVideoRenderer * self) {
  gpointer window_handle;

  g_return_val_if_fail (GST_IS_PLAYER_VIDEO_OVERLAY_VIDEO_RENDERER (self),
      NULL);

  g_object_get (self, "window-handle", &window_handle, NULL);

  return window_handle;
}