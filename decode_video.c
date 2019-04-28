#include <stdio.h>
#include <string.h>

#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>

#define INBUF_ZISE 1024 * 4
#define WORD uint16_t
#define DWORD uint32_t
#define LONG int32_t

#pragma pack(2)
typedef struct tagBITMAPFILEHEADER
{
  WORD bfType;
  DWORD bfSize;
  WORD bfReserved1;
  WORD bfReserved2;
  DWORD bfOffBits;
} BITMAPFILEHEADER, *PBITMAPFILEHEADER;

typedef struct tagBITMAPINFOHEADER
{
  DWORD biSize;
  LONG biWidth;
  LONG biHeight;
  WORD biPlanes;
  WORD biBitCount;
  DWORD biCompression;
  DWORD biSizeImage;
  LONG biXPelsPerMeter;
  LONG biYPelsPerMeter;
  DWORD biClrUsed;
  DWORD biClrImportant;
} BITMAPINFOHEADER, *PBITMAPINFOHEADER;

void saveBMP(struct SwsContext *img_convert_ctx, AVFrame *frame, char *filename)
{
  //1 先进行转换,  YUV420=>RGB24:
  int w = frame->width;
  int h = frame->height;

  int numBytes = avpicture_get_size(AV_PIX_FMT_BGR24, w, h);
  uint8_t *buffer = (uint8_t *)av_malloc(numBytes * sizeof(uint8_t));

  AVFrame *pFrameRGB = av_frame_alloc();
  /* buffer is going to be written to rawvideo file, no alignment */
  /*
    if (av_image_alloc(pFrameRGB->data, pFrameRGB->linesize,
                              w, h, AV_PIX_FMT_BGR24, pix_fmt, 1) < 0) {
        fprintf(stderr, "Could not allocate destination image\n");
        exit(1);
    }
    */
  avpicture_fill((AVPicture *)pFrameRGB, buffer, AV_PIX_FMT_BGR24, w, h);

  sws_scale(img_convert_ctx, frame->data, frame->linesize,
            0, h, pFrameRGB->data, pFrameRGB->linesize);

  //2 构造 BITMAPINFOHEADER
  BITMAPINFOHEADER header;
  header.biSize = sizeof(BITMAPINFOHEADER);

  header.biWidth = w;
  header.biHeight = h * (-1);
  header.biBitCount = 24;
  header.biCompression = 0;
  header.biSizeImage = 0;
  header.biClrImportant = 0;
  header.biClrUsed = 0;
  header.biXPelsPerMeter = 0;
  header.biYPelsPerMeter = 0;
  header.biPlanes = 1;

  //3 构造文件头
  BITMAPFILEHEADER bmpFileHeader = {
      0,
  };
  //HANDLE hFile = NULL;
  DWORD dwTotalWriten = 0;
  DWORD dwWriten;

  bmpFileHeader.bfType = 0x4d42; //'BM';
  bmpFileHeader.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + numBytes;
  bmpFileHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

  FILE *pf = fopen(filename, "wb");
  fwrite(&bmpFileHeader, sizeof(BITMAPFILEHEADER), 1, pf);
  fwrite(&header, sizeof(BITMAPINFOHEADER), 1, pf);
  fwrite(pFrameRGB->data[0], 1, numBytes, pf);
  fclose(pf);

  //释放资源
  //av_free(buffer);
  av_freep(&pFrameRGB[0]);
  av_free(pFrameRGB);
}

static void pgm_save(unsigned char *buf, int wrap, int xsize, int ysize,
                     char *filename)
{
  FILE *f;
  int i;

  f = fopen(filename, "w");
  fprintf(f, "P5\n%d %d\n%d\n", xsize, ysize, 255);
  for (i = 0; i < ysize; i++)
    fwrite(buf + i * wrap, 1, xsize, f);
  fclose(f);
}

static int decode_write_frame(const char *output_path,
                              AVCodecContext *codec_ctx,
                              struct SwsContext *img_convert_ctx,
                              AVFrame *frame,
                              int *frame_count,
                              AVPacket *pkt,
                              int last)
{
  int len, got_frame;
  char buf[1024];
  len = avcodec_decode_video2(codec_ctx, frame, &got_frame, pkt);
  if (len < 0)
  {
    av_log(NULL, AV_LOG_ERROR, "Error while decoding frame %d\n", *frame_count);
    return len;
  }
  if (got_frame)
  {
    printf("Saving %sframe %3d\n", last ? "last " : "", *frame_count);
    fflush(stdout);
    /* the picture is allocated by the decoder, no need to free it */
    snprintf(buf, sizeof(buf), "%s-%d.bmp", output_path, *frame_count);
    /*
        pgm_save(frame->data[0], frame->linesize[0],
                 frame->width, frame->height, buf);
        */
    saveBMP(img_convert_ctx, frame, buf);

    (*frame_count)++;
  }
  if (pkt->data)
  {
    pkt->size -= len;
    pkt->data += len;
  }

  return 0;
}

int decode_video_2_image(const char *src_file, const char *output_path)
{
  int ret = -1;
  FILE *f;
  AVFormatContext *fmt_ctx = NULL;
  const AVCodec *codec;
  AVCodecContext *codec_ctx = NULL;
  AVStream *st = NULL;
  int stream_index;
  int frame_count = 0;
  AVFrame *frame;
  struct SwsContext *img_convert_ctx;
  AVPacket pkt;
  av_log_set_level(AV_LOG_INFO);
  //1 open file,and allocate format context
  ret = avformat_open_input(&fmt_ctx, src_file, NULL, NULL);
  if (ret < 0)
  {
    av_log(NULL, AV_LOG_ERROR, "Open file failed! \n");
    goto __FAIL;
  }
  //2 retrieve stream information
  ret = avformat_find_stream_info(fmt_ctx, NULL);
  if (ret < 0)
  {
    av_log(NULL, AV_LOG_ERROR, "Retrieve stream information failed! \n");
    goto __FAIL;
  }
  av_dump_format(fmt_ctx, 0, src_file, 0);

  //3 init pkt
  av_init_packet(&pkt);

  //4 find video stream id
  ret = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
  if (ret < 0)
  {
    av_log(NULL, AV_LOG_ERROR, "Find video stream  failed! \n");
    goto __FAIL;
  }
  //stream index
  stream_index = ret;
  // video AVStream
  st = fmt_ctx->streams[stream_index];

  //5 finde decoder
  codec = avcodec_find_decoder(st->codecpar->codec_id);
  if (!codec)
  {
    av_log(NULL, AV_LOG_ERROR, "Find video decoder  failed! \n");
    goto __FAIL;
  }
  //AVCodecContext
  codec_ctx = avcodec_alloc_context3(codec);
  if (!codec_ctx)
  {
    av_log(NULL, AV_LOG_ERROR, "Could not allocate video codec context! \n");
    goto __FAIL;
  }
  //6 Copy codec parameters from input stream to output codec context
  ret = avcodec_parameters_to_context(codec_ctx, st->codecpar);
  if (ret < 0)
  {
    av_log(NULL, AV_LOG_ERROR, "Failed to copy %s codec parameters to decoder context! \n", av_get_media_type_string(AVMEDIA_TYPE_VIDEO));
    goto __FAIL;
  }

  //7 open codec
  ret = avcodec_open2(codec_ctx, codec, NULL);
  if (ret < 0)
  {
    av_log(NULL, AV_LOG_ERROR, "Could not open codec! \n");
    goto __FAIL;
  }

  //8 initialize the conversion context
  img_convert_ctx = sws_getContext(codec_ctx->width,
                                   codec_ctx->height,
                                   codec_ctx->pix_fmt,
                                   codec_ctx->width,
                                   codec_ctx->height,
                                   AV_PIX_FMT_RGB24,
                                   SWS_BICUBIC,
                                   NULL,
                                   NULL,
                                   NULL);

  if (!img_convert_ctx)
  {
    av_log(NULL, AV_LOG_ERROR, "Cannot initialize the conversion context! \n");
    goto __FAIL;
  }
  // 9 allocate video frame
  frame = av_frame_alloc();
  if (!frame)
  {
    av_log(NULL, AV_LOG_ERROR, "Could not allocate video frame! \n");
    goto __FAIL;
  }

  while (av_read_frame(fmt_ctx, &pkt) >= 0)
  {

    /* NOTE1: some codecs are stream based (mpegvideo, mpegaudio)
           and this is the only method to use them because you cannot
           know the compressed data size before analysing it.

           BUT some other codecs (msmpeg4, mpeg4) are inherently frame
           based, so you must call them with all the data for one
           frame exactly. You must also initialize 'width' and
           'height' before initializing them. */

    /* NOTE2: some codecs allow the raw parameters (frame size,
           sample rate) to be changed at any frame. We handle this, so
           you should also take care of it */

    /* here, we use a stream based decoder (mpeg1video), so we
           feed decoder and see if it could decode a frame */

    if (pkt.stream_index == stream_index)
    {
      if (decode_write_frame(output_path, codec_ctx, img_convert_ctx, frame, &frame_count, &pkt, 0) < 0)
      {
        goto __FAIL;
      }
    }

    av_packet_unref(&pkt);
  }
  pkt.data = NULL;
  pkt.size = 0;
  decode_write_frame(output_path, codec_ctx, img_convert_ctx, frame, &frame_count, &pkt, 1);
  //decode_write_frame (AVFormatContext *s, AVPacket *pkt)
  av_write_frame(fmt_ctx, &pkt);
  fclose(f);

  ret = 0;
__FAIL:
  if (fmt_ctx)
  {
    avformat_close_input(&fmt_ctx);
  }
  if (img_convert_ctx)
  {
    sws_freeContext(img_convert_ctx);
    img_convert_ctx = NULL;
  }
  if (codec_ctx)
  {
    avcodec_free_context(&codec_ctx);
  }
  if (frame)
  {
    av_frame_free(&frame);
  }

  return ret;
}

int main(int argc, char *argv[])
{
  int ret = 0;
  if (argc < 3)
  {
    printf("Usage: \n "
           "Command src_file output_path \n");
  }
  char *src_file = argv[1];
  char *output_path = argv[2];

  ret = decode_video_2_image(src_file, output_path);
  if (ret < 0)
  {
    printf("decode video  to image failed! \n");
  }
  return 0;
}
