#include <stdio.h>
#include <libavformat/avformat.h>

void creatTestFile()
{
  FILE *file;
  file = fopen("mytest.txt", "a+");
  fwrite("hello", 1, 5, file);
  fclose(file);
}

int main(int argc, char *argv[])
{
  int ret;
  creatTestFile();
  ret = avpriv_io_delete("./mytest.txt");
  if (ret < 0)
  {
    av_log(NULL, AV_LOG_ERROR, "Failed to delete file mytest.txt \n");
    return ret;
  }
  av_log(NULL, AV_LOG_INFO, "Success to delete file mytest.txt \n");
  return ret;
  return 0;
}
