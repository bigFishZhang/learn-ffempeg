#include <stdio.h>
#include <libavutil/log.h>

int main(int agrc, char *argv[])
{
  av_log_set_level(AV_LOG_DEBUG);
  av_log(NULL, AV_LOG_INFO, "Hello FFmpeg! %s \n", "zzb");
  return 0;
}
