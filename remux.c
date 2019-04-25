/* 
  mp4 to flv
*/

#include <stdio.h>
#include <libavutil/timestamp.h>
#include <libavformat/avformat.h>

static void log_packet(const AVFormatContext *fmt_ctx, const AVPacket *pkt, const char *tag)
{
  AVRational *time_base = &fmt_ctx->streams[pkt->stream_index]->time_base;

  printf("%s: pts:%s pts_time:%s dts:%s dts_time:%s duration:%s duration_time:%s stream_index:%d\n",
         tag,
         av_ts2str(pkt->pts), av_ts2timestr(pkt->pts, time_base),
         av_ts2str(pkt->dts), av_ts2timestr(pkt->dts, time_base),
         av_ts2str(pkt->duration), av_ts2timestr(pkt->duration, time_base),
         pkt->stream_index);
}

int main(int argc, char *argv[])
{
  AVOutputFormat *out_fmt = NULL;
  AVFormatContext *input_fmt_ctx = NULL;
  AVFormatContext *output_fmt_ctx = NULL;
  AVPacket pkt;
  char *src_filename = NULL;
  char *dst_filename = NULL;
  int ret, i;
  int stream_index = 0;
  int *stream_mapping = NULL;
  int stream_mapping_size = 0;

  if (argc < 3)
  {
    printf("Params error  params should be more than three \n");
    return -1;
  }
  src_filename = argv[1]; // input file
  dst_filename = argv[2]; // output file

  if ((ret = avformat_open_input(&input_fmt_ctx, src_filename, 0, 0)) < 0)
  {
    fprintf(stderr, "Could not open input file: %s \n", src_filename);
    goto end;
  }

  if ((ret = avformat_find_stream_info(input_fmt_ctx, 0)) < 0)
  {
    fprintf(stderr, "Failed to retrieve input steam information: %s \n", src_filename);
    goto end;
  }

  av_dump_format(input_fmt_ctx, 0, src_filename, 0);

  avformat_alloc_output_context2(&output_fmt_ctx, NULL, NULL, dst_filename);
  if (!output_fmt_ctx)
  {
    fprintf(stderr, "Could not create output context \n");
    ret = AVERROR_UNKNOWN;
    goto end;
  }
  stream_mapping_size = input_fmt_ctx->nb_streams;
  stream_mapping = av_malloc_array(stream_mapping_size, sizeof(*stream_mapping));
  if (!stream_mapping)
  {
    fprintf(stderr, "Create stream_mapping failed \n");
    ret = AVERROR(ENOMEM);
    goto end;
  }
  out_fmt = output_fmt_ctx->oformat;
  for (i = 0; i < input_fmt_ctx->nb_streams; i++)
  {
    AVStream *out_stream;
    AVStream *in_stream = input_fmt_ctx->streams[i];
    AVCodecParameters *in_codecpar = in_stream->codecpar;
    //过滤 只保留音频流 视频流 字母流
    if (in_codecpar->codec_type != AVMEDIA_TYPE_AUDIO &&
        in_codecpar->codec_type != AVMEDIA_TYPE_VIDEO &&
        in_codecpar->codec_type != AVMEDIA_TYPE_SUBTITLE)
    {
      stream_mapping[i] = -1;
      continue;
    }
    stream_mapping[i] = stream_index++;
    //每一路流创建一个对应的流用于写入输出文件
    out_stream = avformat_new_stream(output_fmt_ctx, NULL);
    if (!out_stream)
    {
      fprintf(stderr, "Failed allocating output stream\n");
      ret = AVERROR_UNKNOWN;
      goto end;
    }
    //复制对应的流信息
    ret = avcodec_parameters_copy(out_stream->codecpar, in_codecpar);
    if (ret < 0)
    {
      fprintf(stderr, "Failed to copy codec parameters\n");
      goto end;
    }
    out_stream->codecpar->codec_tag = 0;
  }
  av_dump_format(output_fmt_ctx, 0, dst_filename, 1);
  if (!(out_fmt->flags & AVFMT_NOFILE))
  {
    //打开输出文件
    ret = avio_open(&output_fmt_ctx->pb, dst_filename, AVIO_FLAG_WRITE);
    if (ret < 0)
    {
      fprintf(stderr, "Could not open output file '%s'", dst_filename);
      goto end;
    }
  }
  //1 写头
  ret = avformat_write_header(output_fmt_ctx, NULL);
  if (ret < 0)
  {
    fprintf(stderr, "Error occurred when opening output file\n");
    goto end;
  }
  while (1)
  {
    AVStream *in_stream, *out_stream;
    ret = av_read_frame(input_fmt_ctx, &pkt);
    if (ret < 0)
    {
      break;
    }
    in_stream = input_fmt_ctx->streams[pkt.stream_index];
    if (pkt.stream_index >= stream_mapping_size ||
        stream_mapping[pkt.stream_index] < 0)
    {
      av_packet_unref(&pkt);
      continue;
    }
    pkt.stream_index = stream_mapping[pkt.stream_index];
    out_stream = output_fmt_ctx->streams[pkt.stream_index];
    //log_packet(input_fmt_ctx, &pkt, "in");

    /*copy packt 时间基础调整*/
    pkt.pts = av_rescale_q_rnd(pkt.pts, in_stream->time_base, out_stream->time_base, AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX);
    pkt.dts = av_rescale_q_rnd(pkt.dts, in_stream->time_base, out_stream->time_base, AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX);
    pkt.duration = av_rescale_q(pkt.duration, in_stream->time_base, out_stream->time_base);
    pkt.pos = -1;
    //log_packet(output_fmt_ctx, &pkt, "out");
    //2 写入文件
    ret = av_interleaved_write_frame(output_fmt_ctx, &pkt);
    if (ret < 0)
    {
      fprintf(stderr, "Error muxing packet\n");
      break;
    }
    av_packet_unref(&pkt);
  }
  // 3
  av_write_trailer(output_fmt_ctx);

end:
  avformat_close_input(&input_fmt_ctx);
  if (output_fmt_ctx && !(out_fmt->flags & AVFMT_NOFILE))
  {
    avio_closep(&output_fmt_ctx->pb);
  }
  avformat_free_context(output_fmt_ctx);
  av_freep(&stream_mapping);
  if (ret < 0 && ret != AVERROR_EOF)
  {
    fprintf(stderr, "Error occurred: %s \n", av_err2str(ret));
    return 1;
  }

  return 0;
}
