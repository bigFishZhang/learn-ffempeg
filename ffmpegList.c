#include <stdio.h>
#include <libavutil/log.h>
#include <libavformat/avformat.h>

int main(int argc, char *argv[])
{
  int ret;
  AVIODirContext *ctx = NULL;
  AVIODirEntry *entry = NULL;
  av_log_set_level(AV_LOG_INFO);
  //打开目录
  ret = avio_open_dir(&ctx, "./", NULL);
  if (ret < 0)
  {
    av_log(NULL, AV_LOG_ERROR, "Can not open dir:%s \n ", av_err2str(ret));
    goto __fail;
  }
  //访问目录中文件
  while (1)
  {
    ret = avio_read_dir(ctx, &entry);
    if (ret < 0)
    {
      av_log(NULL, AV_LOG_ERROR, "Can not read dir:%s \n ", av_err2str(ret));
      goto __fail;
    }
    if (!entry) //目录末尾
    {
      break;
    }
    av_log(NULL, AV_LOG_INFO, "%12" PRId64 "   %s \n", entry->size, entry->name);

    avio_free_directory_entry(&entry);
  }
__fail:
  avio_close_dir(&ctx);
  return 0;
}
