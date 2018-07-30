package demo.androidffmpeg01.bigfish.ffmpegandroid03;

public class FFmpegTest {
    static
    {
        System.loadLibrary("avutil");
        System.loadLibrary("avcodec");
        System.loadLibrary("avformat");
//        System.loadLibrary("native-lib");
//        System.loadLibrary("native-lib");
//        System.loadLibrary("native-lib");
        System.loadLibrary("native-lib");
    }
    //test get config
    public static  native  void  ffmpegTest();

    // open file
    public  static native  void  ffmpegDecoder(String filePath);

}
