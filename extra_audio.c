#include <libavutil/log.h>
#include <libavformat/avformat.h>
#include <libavformat/avio.h>
#include <stdio.h>

#define ADTS_HEADER_LEN 7;

void adts_header(char *adtsHeader, int datalen)
{
  int audio_object_type = 2;
  int sampling_frequency_index = 7;
  int channel_config = 2;

  int adtsLen = datalen + 7;

  adtsHeader[0] = 0xff;      //syncword:0xfff                          高8bits
  adtsHeader[1] = 0xf0;      //syncword:0xfff                          低4bits
  adtsHeader[1] |= (0 << 3); //MPEG Version:0 for MPEG-4,1 for MPEG-2  1bit
  adtsHeader[1] |= (0 << 1); //Layer:0                                 2bits
  adtsHeader[1] |= 1;        //protection absent:1                     1bit

  adtsHeader[2] = (audio_object_type - 1) << 6;            //profile:audio_object_type - 1                      2bits
  adtsHeader[2] |= (sampling_frequency_index & 0x0f) << 2; //sampling frequency index:sampling_frequency_index  4bits
  adtsHeader[2] |= (0 << 1);                               //private bit:0                                      1bit
  adtsHeader[2] |= (channel_config & 0x04) >> 2;           //channel configuration:channel_config               高1bit

  adtsHeader[3] = (channel_config & 0x03) << 6; //channel configuration:channel_config      低2bits
  adtsHeader[3] |= (0 << 5);                    //original：0                               1bit
  adtsHeader[3] |= (0 << 4);                    //home：0                                   1bit
  adtsHeader[3] |= (0 << 3);                    //copyright id bit：0                       1bit
  adtsHeader[3] |= (0 << 2);                    //copyright id start：0                     1bit
  adtsHeader[3] |= ((adtsLen & 0x1800) >> 11);  //frame length：value   高2bits

  adtsHeader[4] = (uint8_t)((adtsLen & 0x7f8) >> 3); //frame length:value    中间8bits
  adtsHeader[5] = (uint8_t)((adtsLen & 0x7) << 5);   //frame length:value    低3bits
  adtsHeader[5] |= 0x1f;                             //buffer fullness:0x7ff 高5bits
  adtsHeader[6] = 0xfc;
}

int main(int argc, char *argv[])
{
  int ret;
  int len;
  int audio_index;
  char *src = NULL;
  char *dst = NULL;
  AVFormatContext *fmt_ctx = NULL;
  AVPacket pkt;
  //log level
  av_log_set_level(AV_LOG_INFO);
  // av_register_all();
  //1,read parmas from console
  if (argc < 3)
  {
    av_log(NULL, AV_LOG_ERROR, "params error \n");
    return -1;
  }
  src = argv[1]; // input file
  dst = argv[2]; // output file
  if (!src || !dst)
  {
    av_log(NULL, AV_LOG_ERROR, "src or dst is null \n");
    return -1;
  }
  //open input file
  ret = avformat_open_input(&fmt_ctx, src, NULL, NULL);
  if (ret < 0)
  {
    av_log(NULL, AV_LOG_ERROR, "open file filed!  %s \n", av_err2str(ret));
    return -1;
  }
  //open output file
  FILE *dst_fd = fopen(dst, "wb");
  if (!dst_fd)
  {
    av_log(NULL, AV_LOG_ERROR, "open dst file error \n");
    return -1;
  }
  //read info
  av_dump_format(fmt_ctx, 0, src, 0);
  //2 get stream
  ret = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
  if (ret < 0)
  {
    av_log(NULL, AV_LOG_ERROR, "av_find_best_stream error %s \n", av_err2str(ret));
    avformat_close_input(&fmt_ctx);
    fclose(dst_fd);
    return -1;
  }
  audio_index = ret;
  av_init_packet(&pkt);
  //read fream
  int isFirst = 0;
  while (av_read_frame(fmt_ctx, &pkt) >= 0)
  {
    if (pkt.stream_index == audio_index)
    {
      //TODO
      if (isFirst == 0)
      {
        char adts_header_buf[7];
        adts_header(adts_header_buf, pkt.size);
        fwrite(adts_header_buf, 1, 7, dst_fd);
        isFirst = 1;
      }

      //write file
      len = fwrite(pkt.data, 1, pkt.size, dst_fd);
      if (len != pkt.size)
      {
        av_log(NULL, AV_LOG_WARNING, "fwrite length is not equal pkt.size \n");
      }
    }
    av_packet_unref(&pkt);
  }
  //3 write audio data to aac file
  avformat_close_input(&fmt_ctx);
  if (dst_fd)
  {
    fclose(dst_fd);
  }
  return 0;
}
