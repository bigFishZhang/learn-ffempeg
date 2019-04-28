/*encode h264*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
#include <libavutil/imgutils.h>

int encode_video(const char *src_file, const char *codec_name)
{
  const AVCodec *codec;
  AVCodecContext *codec_ctx;
  int ret = -1, i, x, y, got_output;
  FILE *f;
  AVFrame *frame;
  AVPacket pkt;
  uint8_t endecode[] = {0, 0, 1, 0xb7};

  av_log_set_level(AV_LOG_INFO);

  //1,find the video encoder
  codec = avcodec_find_encoder_by_name(codec_name);
  if (!codec)
  {
    av_log(NULL, AV_LOG_ERROR, "Finde codec failed! \n");
    goto __FAIL;
  }
  //2 Alloc context
  codec_ctx = avcodec_alloc_context3(codec);
  if (!codec_ctx)
  {
    av_log(NULL, AV_LOG_ERROR, "Could not alloc video codec context \n");
    goto __FAIL;
  }
  //3 Put sample parameters
  codec_ctx->bit_rate = 400000;
  // Resolution must be a multiple of two
  codec_ctx->width = 352;
  codec_ctx->height = 288;
  // Frame per second
  codec_ctx->time_base = (AVRational){1, 25};
  codec_ctx->framerate = (AVRational){25, 1};
  /* emit one intra frame every ten frames
     * check frame pict_type before passing frame
     * to encoder, if frame->pict_type is AV_PICTURE_TYPE_I
     * then gop_size is ignored and the output of encoder
     * will always be I frame irrespective to gop_size
     */
  codec_ctx->gop_size = 10;
  codec_ctx->max_b_frames = 1;
  codec_ctx->pix_fmt = AV_PIX_FMT_YUV420P;

  // Set write speed
  if (codec->id == AV_CODEC_ID_H264)
  {
    av_opt_set(codec_ctx->priv_data, "preset", "slow", 0);
  }
  //4 Open codec
  if (avcodec_open2(codec_ctx, codec, NULL) < 0)
  {
    av_log(NULL, AV_LOG_ERROR, "Could not open codec \n");
    goto __FAIL;
  }
  //5 Open file
  f = fopen(src_file, "wb");
  if (!f)
  {
    av_log(NULL, AV_LOG_ERROR, "Could not open src_file \n");
    goto __FAIL;
  }

  //6 Alloc frame
  frame = av_frame_alloc();
  if (!frame)
  {
    av_log(NULL, AV_LOG_ERROR, "Could not alloc video frame \n");
    goto __FAIL;
  }
  frame->format = codec_ctx->pix_fmt;
  frame->width = codec_ctx->width;
  frame->height = codec_ctx->height;

  //7 Allocate new buffer(s) for  video data.
  ret = av_frame_get_buffer(frame, 32);
  if (ret < 0)
  {
    av_log(NULL, AV_LOG_ERROR, "Could not alloc video frame data \n");
    goto __FAIL;
  }

  //8 Encode 1 second of video
  for (int i = 0; i < 25; i++)
  {
    av_init_packet(&pkt);
    pkt.data = NULL;
    pkt.size = 0;

    fflush(stdout);

    // Make sure the frame data is writable
    ret = av_frame_make_writable(frame);
    if (ret < 0)
    {
      av_log(NULL, AV_LOG_ERROR, "Frame not writable \n");
      goto __FAIL;
    }

    // Prepare a dummy image
    /* Y */
    for (y = 0; y < codec_ctx->height; y++)
    {
      for (x = 0; x < codec_ctx->width; x++)
      {
        frame->data[0][y * frame->linesize[0] + x] = x + y + i * 3;
      }
    }

    /* Cb and Cr */
    for (y = 0; y < codec_ctx->height / 2; y++)
    {
      for (x = 0; x < codec_ctx->width / 2; x++)
      {
        frame->data[1][y * frame->linesize[1] + x] = 128 + y + i * 2;
        frame->data[2][y * frame->linesize[2] + x] = 64 + x + i * 5;
      }
    }

    frame->pts = i;

    // Encode the image
    ret = avcodec_encode_video2(codec_ctx, &pkt, frame, &got_output);
    if (ret < 0)
    {
      av_log(NULL, AV_LOG_ERROR, "Error encoding frame  num: %d \n", i);
      goto __FAIL;
    }
    if (got_output)
    {
      av_log(NULL, AV_LOG_INFO, "Write frame %3d (size = %5d) \n", i, pkt.size);
      fwrite(pkt.data, 1, pkt.size, f);
      av_packet_unref(&pkt);
    }
  }
  // Get the delayed frames
  for (got_output = 1; got_output; i++)
  {
    fflush(stdout);
    ret = avcodec_encode_video2(codec_ctx, &pkt, NULL, &got_output);
    if (ret < 0)
    {
      av_log(NULL, AV_LOG_ERROR, "Error encoding  delayed frame  num: %d \n", i);
      goto __FAIL;
    }
    if (got_output)
    {
      av_log(NULL, AV_LOG_INFO, "Write  delayed frame %3d (size = %5d) \n", i, pkt.size);
      fwrite(pkt.data, 1, pkt.size, f);
      av_packet_unref(&pkt);
    }
  }

  //9 add sequence en code to have a real MPEG file
  fwrite(endecode, 1, sizeof(endecode), f);
  fclose(f);
__FAIL:
  if (codec_ctx)
  {
    avcodec_free_context(&codec_ctx);
  }
  if (frame)
  {
    av_frame_free(&frame);
  }

  return 0;
}

int main(int argc, char *argv[])
{
  int ret = 0;
  if (argc < 3)
  {
    printf("Usage: \n "
           "Command src_file codec_name \n");
  }
  char *src_file = argv[1];
  char *codec_name = argv[2];
  ret = encode_video(src_file, codec_name);
  if (ret < 0)
  {
    printf("encode video failed! \n");
  }

  return 0;
}
