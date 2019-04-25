#include <stdio.h>
#include <libavutil/log.h>
#include <libavutil/timestamp.h>
#include <libavformat/avformat.h>

#define ERROR_STR_SIZE 1024
int av_merge(const char *src_file1, const char *src_file2, const char *out_file)
{
  int ret = -1;
  int err_code;
  char errors[ERROR_STR_SIZE];
  AVFormatContext *ifmat_ctx1 = NULL;
  AVFormatContext *ifmat_ctx2 = NULL;

  AVFormatContext *ofmt_ctx = NULL;
  AVOutputFormat *ofmt = NULL;

  AVStream *in_stream1 = NULL;
  AVStream *in_stream2 = NULL;

  AVStream *out_stream1 = NULL;
  AVStream *out_stream2 = NULL;

  int64_t cur_pts1 = 0, cur_pts2 = 0;

  int b_use_video_ts = 1;
  uint32_t packets = 0;
  AVPacket pkt;

  int stream1 = 0, stream2 = 0;

  av_log_set_level(AV_LOG_INFO);

  //1 Open first file
  if ((err_code = avformat_open_input(&ifmat_ctx1, src_file1, 0, 0)) < 0)
  {
    av_strerror(err_code, errors, ERROR_STR_SIZE);
    av_log(NULL, AV_LOG_ERROR, "Could not open file1: %s, %d(%s) \n", src_file1, err_code, errors);
    goto __FAIL;
  }
  // Retrieve stream info
  if ((err_code = avformat_find_stream_info(ifmat_ctx1, 0)) < 0)
  {
    av_strerror(err_code, errors, ERROR_STR_SIZE);
    av_log(NULL, AV_LOG_ERROR, "Could not retrieve input stream info  with file1: %s, %d(%s) \n", src_file1, err_code, errors);
    goto __FAIL;
  }
  av_dump_format(ifmat_ctx1, 0, src_file1, 0);

  //2 Open second file
  if ((err_code = avformat_open_input(&ifmat_ctx2, src_file2, 0, 0)) < 0)
  {
    av_strerror(err_code, errors, ERROR_STR_SIZE);
    av_log(NULL, AV_LOG_ERROR, "Could not open file2: %s, %d(%s) \n", src_file2, err_code, errors);
    goto __FAIL;
  }
  // Retrieve stream info
  if ((err_code = avformat_find_stream_info(ifmat_ctx2, 0)) < 0)
  {
    av_strerror(err_code, errors, ERROR_STR_SIZE);
    av_log(NULL, AV_LOG_ERROR, "Could not retrieve input stream info  with file2: %s, %d(%s) \n", src_file2, err_code, errors);
    goto __FAIL;
  }
  av_dump_format(ifmat_ctx2, 0, src_file2, 0);

  //3 Create out context
  if ((err_code = avformat_alloc_output_context2(&ofmt_ctx, NULL, NULL, out_file)) < 0)
  {
    av_strerror(err_code, errors, ERROR_STR_SIZE);
    av_log(NULL, AV_LOG_ERROR, "Failed to create an context of outfile,%d(%s) \n", err_code, errors);
    goto __FAIL;
  }
  // AVOutputFormat
  ofmt = ofmt_ctx->oformat;

  //4 Create out stream1 according to input stream1
  if (ifmat_ctx1->nb_streams > 0)
  {
    in_stream1 = ifmat_ctx1->streams[0];
    stream1 = 1;
    AVCodecParameters *in_codecpar = in_stream1->codecpar;

    // Exclude invalid stream
    if (in_codecpar->codec_type != AVMEDIA_TYPE_AUDIO &&
        in_codecpar->codec_type != AVMEDIA_TYPE_VIDEO &&
        in_codecpar->codec_type != AVMEDIA_TYPE_SUBTITLE)
    {
      av_log(NULL, AV_LOG_ERROR, "The Codec type is invalid!\n");
      goto __FAIL;
    }
    //new a stream for out file
    out_stream1 = avformat_new_stream(ofmt_ctx, NULL);
    if (!out_stream1)
    {
      av_log(NULL, AV_LOG_ERROR, "Failed to alloc out stream of src_file1 \n");
      goto __FAIL;
    }
    // Copy codec_parameters from in_stream
    if ((err_code = avcodec_parameters_copy(out_stream1->codecpar, in_codecpar)) < 0)
    {
      av_strerror(err_code, errors, ERROR_STR_SIZE);
      av_log(NULL, AV_LOG_ERROR, "Failed to copy codec paramters from in_stream1,%d(%s) \n", err_code, errors);
      goto __FAIL;
    }
    out_stream1->codecpar->codec_tag = 0;
  }
  //5 Create out stream2 according to input stream2
  if (ifmat_ctx2->nb_streams > 0)
  {
    in_stream2 = ifmat_ctx2->streams[0];
    stream2 = 1;
    AVCodecParameters *in_codecpar = in_stream2->codecpar;

    // Exclude invalid stream
    if (in_codecpar->codec_type != AVMEDIA_TYPE_AUDIO &&
        in_codecpar->codec_type != AVMEDIA_TYPE_VIDEO &&
        in_codecpar->codec_type != AVMEDIA_TYPE_SUBTITLE)
    {
      av_log(NULL, AV_LOG_ERROR, "The Codec type is invalid!\n");
      goto __FAIL;
    }
    //new a stream for out file
    out_stream2 = avformat_new_stream(ofmt_ctx, NULL);
    if (!out_stream2)
    {
      av_log(NULL, AV_LOG_ERROR, "Failed to alloc out stream of src_file2 \n");
      goto __FAIL;
    }
    // Copy codec_parameters from in_stream
    if ((err_code = avcodec_parameters_copy(out_stream2->codecpar, in_codecpar)) < 0)
    {
      av_strerror(err_code, errors, ERROR_STR_SIZE);
      av_log(NULL, AV_LOG_ERROR, "Failed to copy codec paramters from in_stream1,%d(%s) \n", err_code, errors);
      goto __FAIL;
    }
    out_stream2->codecpar->codec_tag = 0;
  }
  // Print out context
  av_dump_format(ofmt_ctx, 0, out_file, 1);

  //6 Open out file
  if (!(ofmt->flags & AVFMT_NOFILE))
  {
    // Open
    if ((err_code = avio_open(&ofmt_ctx->pb, out_file, AVIO_FLAG_WRITE)) > 0)
    {
      av_strerror(err_code, errors, ERROR_STR_SIZE);
      av_log(NULL, AV_LOG_ERROR, "Could not open output file,%s,%d(%s) \n", out_file, err_code, errors);
      goto __FAIL;
    }
  }
  // Write media header
  if ((err_code = avformat_write_header(ofmt_ctx, NULL)) > 0)
  {
    av_strerror(err_code, errors, ERROR_STR_SIZE);
    av_log(NULL, AV_LOG_ERROR, "Error when writing media header \n");
    goto __FAIL;
  }
  // 8 Write media pkt
  // Init one pkt
  av_init_packet(&pkt);
  // Write pkt
  while (stream1 || stream2)
  {
    //select stream to encode
    // return ts_a == ts_b ? 0 : ts_a < ts_b ? -1 : 1  means get video frome stream1
    if (stream1 && (!stream2 || av_compare_ts(cur_pts1, in_stream1->time_base, cur_pts2, in_stream2->time_base) <= 0))
    {
      //read frame frome pkt
      ret = av_read_frame(ifmat_ctx1, &pkt);
      pkt.pts = 0;
      if (ret < 0)
      {
        stream1 = 0;
        continue;
      }

      if (!b_use_video_ts && (in_stream1->codecpar->codec_type == AVMEDIA_TYPE_VIDEO))
      {
        pkt.pts = ++packets;
        in_stream1->time_base = (AVRational){in_stream1->r_frame_rate.den, in_stream1->r_frame_rate.num};
        pkt.pts = av_rescale_q_rnd(pkt.pts, in_stream1->time_base, out_stream1->time_base, (AV_ROUND_INF | AV_ROUND_PASS_MINMAX));

        pkt.dts = pkt.pts;
        av_log(NULL, AV_LOG_DEBUG, "video packets before convert %d, dts=%lld,  pts= %lld \n", packets, pkt.dts, pkt.pts);
      }

      // FIX: No PTS (Example: Raw H.264)
      // Write PTS
      if (pkt.pts == AV_NOPTS_VALUE)
      {
        AVRational time_base1 = in_stream1->time_base;
        // Duration between 2 frames
        av_log(NULL, AV_LOG_DEBUG, "AV_TIME_BASE=%d,av_q2d=%f(num=%d,den=%d) \n",
               AV_TIME_BASE,
               av_q2d(in_stream1->r_frame_rate),
               in_stream1->r_frame_rate.num,
               in_stream1->r_frame_rate.den);
        int64_t calc_duration = (double)AV_TIME_BASE / av_q2d(in_stream1->r_frame_rate);
        // Parameters
        pkt.pts = (double)(packets * calc_duration) / (double)(av_q2d(time_base1) * AV_TIME_BASE);
        pkt.dts = pkt.dts;
        cur_pts1 = pkt.pts;
        pkt.duration = (double)calc_duration / (double)(av_q2d(time_base1) * AV_TIME_BASE);
        packets++;
      }
      // Convert PTS/DTS
      pkt.pts = av_rescale_q_rnd(pkt.pts, in_stream1->time_base, out_stream1->time_base, (AV_ROUND_INF | AV_ROUND_PASS_MINMAX));
      pkt.dts = pkt.pts;

      pkt.duration = av_rescale_q(pkt.duration, in_stream1->time_base, out_stream1->time_base);
      pkt.pos = -1;
      pkt.stream_index = 0;
      av_log(NULL, AV_LOG_DEBUG, "video packets after convert %d, dts=%lld,  pts= %lld \n", packets, pkt.dts, pkt.pts);
      // Write pkt to out stream1
      stream1 = !av_interleaved_write_frame(ofmt_ctx, &pkt);
    }
    else if (stream2)
    {
      ret = av_read_frame(ifmat_ctx2, &pkt);
      pkt.pts = 0;
      if (ret < 0)
      {
        stream2 = 0;
        continue;
      }
      if (!b_use_video_ts && (in_stream2->codecpar->codec_type == AVMEDIA_TYPE_VIDEO))
      {
        pkt.pts = packets++;
        pkt.dts = pkt.pts;
      }

      cur_pts2 = pkt.pts;
      // Convert PTS/DTS
      pkt.pts = av_rescale_q_rnd(pkt.pts, in_stream2->time_base, out_stream2->time_base, (AV_ROUND_INF | AV_ROUND_PASS_MINMAX));
      pkt.dts = pkt.pts;

      pkt.duration = av_rescale_q(pkt.duration, in_stream2->time_base, out_stream2->time_base);
      pkt.pos = -1;
      pkt.stream_index = 1;
      av_log(NULL, AV_LOG_DEBUG, "Write stream2 Packet. size:%5d\tpts:%lld\tdts:%lld\n", pkt.size, pkt.pts, pkt.dts);
      // Write pkt to out stream2
      stream2 = !av_interleaved_write_frame(ofmt_ctx, &pkt);
    }
    // Free pkt after whrite
    av_packet_unref(&pkt);
  }
  // 9 Write medea tailer
  if ((err_code = av_write_trailer(ofmt_ctx)) < 0)
  {
    av_strerror(err_code, errors, ERROR_STR_SIZE);
    av_log(NULL, AV_LOG_ERROR, "Error when writing media tailer \n");
    goto __FAIL;
  }
  ret = 0;

__FAIL:
  // 10 free context
  if (ifmat_ctx1)
  {
    avformat_close_input(&ifmat_ctx1);
  }
  if (ifmat_ctx2)
  {
    avformat_close_input(&ifmat_ctx2);
  }
  if (ofmt_ctx)
  {
    if (!(ofmt->flags & AVFMT_NOFILE))
    { // close file
      avio_closep(&ofmt_ctx->pb);
    }
    avformat_free_context(ofmt_ctx);
  }
  return 0;
}
/*
argv[0] ./name
argv[1] src_file1
argv[2] src_file2
argv[3] out_file
*/
int main(int argc, char *argv[])
{
  int ret = 0;

  if (argc < 4)
  {
    printf("Usage: \n "
           "Command src_file1 src_file2 out_file \n");
  }
  char *src_file1 = argv[1];
  char *src_file2 = argv[2];
  char *out_file = argv[3];
  ret = av_merge(src_file1, src_file2, out_file);
  if (ret < 0)
  {
    printf("Merge file failed!");
  }
  return 0;
}