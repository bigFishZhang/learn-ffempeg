package demo.androidffmpeg01.bigfish.ffmpegandroid03;

import android.os.Environment;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.widget.TextView;

public class MainActivity extends AppCompatActivity {

    // Used to load the 'native-lib' library on application startup.
    static {
        System.loadLibrary("native-lib");
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        FFmpegTest.ffmpegTest();
        String rootPath = Environment.getExternalStorageDirectory().getAbsolutePath();
        String inFilePath = rootPath.concat("FFmpegAndroid/cat.mp4");
        FFmpegTest.ffmpegDecoder(inFilePath);
    }


}
