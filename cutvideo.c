#include <stdio.h>
#include <libavutil/timestamp.h>
#include <libavformat/avformat.h>

int cutVideo(double fromSeconds, double endSeconds, const char *inFileName, const char *outFileName)
{
  int ret, i;
  AVOutputFormat *outFmt = NULL;
  AVFormatContext *inputFmtCtx = NULL, *outFmtCtx = NULL;
  AVPacket pkt;
  //1
  if ((ret = avformat_open_input(&inputFmtCtx, inFileName, 0, 0)) < 0)
  {
    fprintf(stderr, "Could not open input file '%s'", inFileName);
    goto end;
  }
  printf("1\n");
  //2
  if ((ret = avformat_find_stream_info(inputFmtCtx, 0)) < 0)
  {
    fprintf(stderr, "Failed to retrieve input stream information");
    goto end;
  }
  printf("2\n");
  //print info
  av_dump_format(inputFmtCtx, 0, inFileName, 0);

  //3
  avformat_alloc_output_context2(&outFmtCtx, NULL, NULL, outFileName);
  if (!outFmtCtx)
  {
    fprintf(stderr, "Could not create output context\n");
    ret = AVERROR_UNKNOWN;
    goto end;
  }
  printf("3\n");
  outFmt = outFmtCtx->oformat;
  //4
  for (i = 0; i < inputFmtCtx->nb_streams; i++)
  {
    AVStream *inStream = inputFmtCtx->streams[i];
    AVCodec *codec = avcodec_find_decoder(inStream->codecpar->codec_id);
    AVStream *outStream = avformat_new_stream(outFmtCtx, codec);
    if (!outStream)
    {
      fprintf(stderr, "Failed alloc output stream\n");
      ret = AVERROR_UNKNOWN;
      goto end;
    }
    //deprecated
    // ret = avcodec_copy_context(outStream->codec, inStream->codec);
    // if (ret < 0)
    // {
    //   fprintf(stderr, "Failed to copy context from input to output stream codec context\n");
    //   goto end;
    // }
    // outStream->codec->codec_tag = 0;
    // if (outFmtCtx->oformat->flags & AVFMT_GLOBALHEADER)
    // {
    //   outStream->codec->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    // }
    // ---------------------
    // inputFmtCtx->codec

    //复制AVCodecContext的设置（Copy the settings of AVCodecContext
    AVCodecContext *codecCtx = avcodec_alloc_context3(codec);
    ret = avcodec_parameters_to_context(codecCtx, inStream->codecpar);
    if (ret < 0)
    {
      printf("Failed to copy in_stream codecpar to codec context\n");
      goto end;
    }
    codecCtx->codec_tag = 0;
    if (outFmtCtx->oformat->flags & AVFMT_GLOBALHEADER)
    {
      codecCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }
    ret = avcodec_parameters_from_context(outStream->codecpar, codecCtx);
    if (ret < 0)
    {
      printf("Failed to copy codec context to out_stream codecpar context\n");
      goto end;
    }
    printf("4\n");
    // ---------------------
  }
  printf("5\n");

  av_dump_format(outFmtCtx, 0, outFileName, 1);

  if (!(outFmt->flags & AVFMT_NOFILE))
  {
    ret = avio_open(&outFmtCtx->pb, outFileName, AVIO_FLAG_WRITE);
    if (ret < 0)
    {
      fprintf(stderr, "Could not open output file '%s'", outFileName);
      goto end;
    }
  }
  ret = avformat_write_header(outFmtCtx, NULL);
  if (ret < 0)
  {
    fprintf(stderr, "Error occurred when opening output file\n");
    goto end;
  }
  //    int64_t start_from = 8*AV_TIME_BASE;
  ret = av_seek_frame(inputFmtCtx, -1, fromSeconds * AV_TIME_BASE, AVSEEK_FLAG_ANY);
  if (ret < 0)
  {
    fprintf(stderr, "Error seek\n");
    goto end;
  }
  int64_t *dstStartFrom = malloc(sizeof(int64_t) * inputFmtCtx->nb_streams);
  memset(dstStartFrom, 0, sizeof(int64_t) * inputFmtCtx->nb_streams);
  int64_t *ptsStartFrom = malloc(sizeof(int64_t) * inputFmtCtx->nb_streams);
  memset(ptsStartFrom, 0, sizeof(int64_t) * inputFmtCtx->nb_streams);
  printf("6\n");
  while (1)
  {
    AVStream *inStream, *outStream;
    ret = av_read_frame(inputFmtCtx, &pkt);
    if (ret < 0)
      break;
    inStream = inputFmtCtx->streams[pkt.stream_index];
    outStream = outFmtCtx->streams[pkt.stream_index];
    if (av_q2d(inStream->time_base) * pkt.pts > endSeconds)
    {
      // av_free_packet(&pkt);
      av_packet_unref(&pkt);
      break;
    }
    if (dstStartFrom[pkt.stream_index] == 0)
    {
      dstStartFrom[pkt.stream_index] = pkt.dts;
      printf("dstStartFrom: %s\n", av_ts2str(dstStartFrom[pkt.stream_index]));
    }
    if (ptsStartFrom[pkt.stream_index] == 0)
    {
      ptsStartFrom[pkt.stream_index] = pkt.dts;
      printf("ptsStartFrom: %s\n", av_ts2str(ptsStartFrom[pkt.stream_index]));
    }
    /* copy packet */
    pkt.pts = av_rescale_q_rnd(pkt.pts - ptsStartFrom[pkt.stream_index],
                               inStream->time_base,
                               outStream->time_base,
                               AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX);
    pkt.dts = av_rescale_q_rnd(pkt.dts - dstStartFrom[pkt.stream_index],
                               inStream->time_base,
                               outStream->time_base,
                               AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX);
    if (pkt.pts < 0)
    {
      pkt.pts = 0;
    }
    if (pkt.dts < 0)
    {
      pkt.dts = 0;
    }
    pkt.duration = (int)av_rescale_q((int64_t)pkt.duration, inStream->time_base, outStream->time_base);
    pkt.pos = -1;

    ret = av_interleaved_write_frame(outFmtCtx, &pkt);
    if (ret < 0)
    {
      fprintf(stderr, "Error muxing packet\n");
      break;
    }
    av_packet_unref(&pkt);
  }
  printf("7\n");
  free(dstStartFrom);
  free(ptsStartFrom);

  av_write_trailer(outFmtCtx);

end:
  avformat_close_input(&inputFmtCtx);
  //close output
  if (outFmtCtx && !(outFmt->flags & AVFMT_NOFILE))
  {
    avio_closep(&outFmtCtx->pb);
  }
  avformat_free_context(outFmtCtx);
  if (ret < 0 && ret != AVERROR_EOF)
  {
    fprintf(stderr, "Error occurred: %s\n", av_err2str(ret));
    return 1;
  }
  printf("8\n");
  return 0;
}

//需要输入5个参数 包含了开始时间 结束之间 输入文件 输出文件
int main(int argc, char *argv[])
{
  int ret = 0;
  if (argc < 5)
  {
    fprintf(stderr, "Usage: \
                command startime, endtime, srcfile, outfile");
    return -1;
  }
  double startTime = atof(argv[1]);
  double endTime = atof(argv[2]);
  char *srcFileName = argv[3];
  char *dstFileName = argv[4];
  ret = cutVideo(startTime, endTime, srcFileName, dstFileName);
  if (ret < 0)
  {
    fprintf(stderr, "cut video failed! %s \n", srcFileName);
  }
  return 0;
}