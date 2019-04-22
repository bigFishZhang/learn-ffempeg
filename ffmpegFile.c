#include <stdio.h>
#include <libavformat/avformat.h>

void creatTestFile()
{
  FILE *file;
  file = fopen("mytest.txt", "a+");
  fwrite("hello", 1, 5, file);
  fclose(file);
}
int changeName(char *newNme)
{
  int ret;
  ret = avpriv_io_move("mytest.txt", newNme);
  return ret;
}

int main(int argc, char *argv[])
{
  int ret, re;
  creatTestFile();
  re = changeName("newText.txt");
  if (re < 0)
  {
    av_log(NULL, AV_LOG_ERROR, "Failed to reName file mytest.txt \n");
    return re;
  }
  av_log(NULL, AV_LOG_INFO, "Success to reName file mytest.txt \n");

  ret = avpriv_io_delete("./newText.txt");
  if (ret < 0)
  {
    av_log(NULL, AV_LOG_ERROR, "Failed to delete file newText.txt \n");
    return ret;
  }
  av_log(NULL, AV_LOG_INFO, "Success to delete file mytest.txt \n");
  return ret;
  return 0;
}
