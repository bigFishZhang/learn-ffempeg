#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libavutil/common.h>
#include <libavutil/frame.h>
#include <libavutil/samplefmt.h>

/* check that a given sample format is supported by the encoder */
static int check_sample_fmt(const AVCodec *codec, enum AVSampleFormat sample_fmt)
{
  const enum AVSampleFormat *p = codec->sample_fmts;
  while (*p != AV_SAMPLE_FMT_NONE)
  {
    if (*p == sample_fmt)
    {
      return 1;
    }
    p++;
  }
  return 0;
}

/* just pick the highest supported samplerate */
static int select_sample_rate(const AVCodec *codec)
{
  const int *p;
  int best_sampletate = 0;
  if (!codec->supported_samplerates)
  {
    return 44100;
  }
  p = codec->supported_samplerates;
  while (*p)
  {
    if (!best_sampletate || abs(44100 - *p) < abs(44100 - best_sampletate))
    {
      best_sampletate = *p;
    }
    p++;
  }
  return best_sampletate;
}
/* select layout with the highest channel count */
static int select_channel_layout(const AVCodec *codec)
{
  const uint64_t *p;
  uint64_t best_ch_layout = 0;
  int best_nb_channels = 0; // number of channels in the channel layout
  if (!codec->channel_layouts)
    return AV_CH_LAYOUT_STEREO;

  p = codec->channel_layouts; //array of support channel layouts

  while (*p)
  {
    int nb_channels = av_get_channel_layout_nb_channels(*p); //Return the number of channels in the channel layout.
    if (nb_channels > best_nb_channels)
    {
      best_ch_layout = *p;
      best_nb_channels = nb_channels;
    }
    p++;
  }
  return best_ch_layout;
}

int encode_audio(const char *output_file)
{
  int ret = -1;
  int i, j, k, got_output;
  const AVCodec *codec;
  AVCodecContext *codec_ctx = NULL;
  AVFrame *frame = NULL;
  AVPacket pkt;
  FILE *f;
  uint16_t *samples;
  float t, tincr;

  av_log_set_level(AV_LOG_INFO);
  // 1 find encoder
  codec = avcodec_find_encoder(AV_CODEC_ID_MP2);
  if (!codec)
  {
    av_log(NULL, AV_LOG_ERROR, "Finde encoder failed! \n");
    goto __FAIL;
  }
  // codec_ctx = avcodec_alloc_context3(codec);
  // if (!codec_ctx)
  // {
  //   av_log(NULL, AV_LOG_ERROR, "Could not allocate audio codec context\n");
  //   goto __FAIL;
  // }
  //2 put sample parameters
  codec_ctx->bit_rate = 64000;
  // check encoder supports s16 pcm input
  codec_ctx->sample_fmt = AV_SAMPLE_FMT_S16;
  if (!check_sample_fmt(codec, codec_ctx->sample_fmt))
  {
    av_log(NULL, AV_LOG_ERROR, "Encoder does not support sample format %s \n", av_get_sample_fmt_name(codec_ctx->sample_fmt));
    goto __FAIL;
  }
  //3 select other audio parameters supported by the encoder
  codec_ctx->sample_rate = select_sample_rate(codec);
  codec_ctx->channel_layout = select_channel_layout(codec);
  codec_ctx->channels = av_get_channel_layout_nb_channels(codec_ctx->channel_layout);

  // 4 open codec
  ret = avcodec_open2(codec_ctx, codec, NULL);
  if (ret < 0)
  {
    av_log(NULL, AV_LOG_ERROR, "Could not open codec \n");
    goto __FAIL;
  }
  //5 open file
  f = fopen(output_file, "wb");
  if (!f)
  {
    av_log(NULL, AV_LOG_ERROR, "Could not open output_file \n");
    goto __FAIL;
  }
  //6 frame containing input raw audio
  frame = av_frame_alloc();
  if (!frame)
  {
    av_log(NULL, AV_LOG_ERROR, "Could not alloc audio frame \n");
    goto __FAIL;
  }
  frame->nb_samples = codec_ctx->frame_size;
  frame->format = codec_ctx->sample_fmt;
  frame->channel_layout = codec_ctx->channel_layout;

  //7 allocate the data buffers
  ret = av_frame_get_buffer(frame, 0);
  if (ret < 0)
  {
    av_log(NULL, AV_LOG_ERROR, "Could not allocate audio data buffers\n");
    goto __FAIL;
  }
  //8 encode a single tone sound
  t = 0;
  tincr = 2 * M_PI * 440.0 / codec_ctx->sample_rate;
  for (i = 0; i < 200; i++)
  {
    av_init_packet(&pkt);
    pkt.data = NULL; // packet data will be allocated by the encoder
    pkt.size = 0;
    // pkt.pts = 0;

    /* make sure the frame is writable -- makes a copy if the encoder
         * kept a reference internally */
    ret = av_frame_make_writable(frame);
    if (ret < 0)
    {
      av_log(NULL, AV_LOG_ERROR, "Frame not writable \n");
      goto __FAIL;
    }
    samples = (uint16_t *)frame->data[0];
    for (j = 0; j < codec_ctx->frame_size; j++)
    {
      samples[2 * j] = (int)(sin(t) * 10000);

      for (k = 1; k < codec_ctx->channels; k++)
        samples[2 * j + k] = samples[2 * j];
      t += tincr;
    }
    // encode the samples
    ret = avcodec_encode_audio2(codec_ctx, &pkt, frame, &got_output);
    if (ret < 0)
    {
      av_log(NULL, AV_LOG_ERROR, "Error encoding audio frame\n");
      goto __FAIL;
    }
    if (got_output)
    {
      fwrite(pkt.data, 1, pkt.size, f);
      av_packet_unref(&pkt);
    }
  }
  /* get the delayed frames */
  for (got_output = 1; got_output; i++)
  {
    ret = avcodec_encode_audio2(codec_ctx, &pkt, NULL, &got_output);
    if (ret < 0)
    {
      av_log(NULL, AV_LOG_ERROR, "Error encoding frame\n");
      goto __FAIL;
    }

    if (got_output)
    {
      fwrite(pkt.data, 1, pkt.size, f);
      av_packet_unref(&pkt);
    }
  }

__FAIL:

  if (f)
  {
    fclose(f);
  }
  if (frame)
  {
    av_frame_free(&frame);
  }
  if (codec_ctx)
  {
    avcodec_free_context(&codec_ctx);
  }
  return ret;
}

int main(int argc, char *argv[])
{
  int ret = 0;
  if (argc < 2)
  {
    printf("Usage: \n "
           "Command  output_file \n");
    return 0;
  }
  char *output_file = argv[1];

  ret = encode_audio(output_file);
  if (ret < 0)
  {
    printf("encode audio failed! \n");
  }
  return 0;
}
