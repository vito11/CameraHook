package com.vito.research.camerahook;

import android.Manifest;
import android.app.Activity;
import android.app.AlertDialog;
import android.content.ContentResolver;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Matrix;
import android.hardware.Camera;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.provider.MediaStore;
import android.provider.SyncStateContract;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.Toast;

import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.ArrayList;
import java.util.List;

public class MainActivity extends Activity implements Button.OnClickListener {


    String exeFileName = "injector";
    String exe_path;
    String soFileName = "libhook.so";
    String so_path;
    String imageInfoName = "fakeImage";
    String imageInfo_path;
    Button button;
    final int SELECT_PICTURE = 0;
    final int SELECT_CAMER = 1;
    Bitmap bitmap = null;
    ImageView imageView;
    byte[] yuv420sp = null;
    List<Camera.Size> preSize = null;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);


        button = (Button) findViewById(R.id.button2);
        button.setOnClickListener(this);

        imageView = (ImageView) findViewById(R.id.imageView);
        runLocalRootUserCommand("setenforce 0");
        remountSystem();


        initExecutableFile();
        getCameraSupportedSize();
        exe();
    }
// remount /system to read and write
    public void remountSystem()
    {
        String s = runLocalRootUserCommand("mount");

        int a = s.indexOf("/system",0);
        while (true)
        {
            if(a<0)
            {
                Log.e("msg", "error: can not remount system!");
                return;
            }
            if(a+7>s.length())
            {
                Log.e("msg", "error: can not remount system!");
                return;
            }
            if(s.charAt(a-1) ==' ' && s.charAt(a+7) ==' ')
            {
                break;
            }
            a = s.indexOf("/system",a+7);
        }

        s = s.substring(0,a);
        String[] temp = s.split("\n");
        if(temp.length == 0)
        {
            Log.e("msg", "error: can not remount system!");
            return;
        }
        s = temp[temp.length-1];

        String remountString = "mount -o remount "+s+" /system";
        Log.i("msg", remountString);
        runLocalRootUserCommand(remountString);
    }

    public void initExecutableFile() {
        exe_path = "data/data/" + getPackageName() + "/" + exeFileName;
        so_path = "data/data/" + getPackageName() + "/" + soFileName;
        imageInfo_path = "data/data/" + getPackageName() + "/" + imageInfoName;
        try {
            copyDataToExePath(exeFileName, exe_path);
            copyDataToExePath(soFileName, so_path);
            String  result = runLocalRootUserCommand("ls /system/lib/libhook.so");
            if(result.length()<10) {
                String copySoToSystem = "cat " + so_path + " > " + "/system/lib/libhook.so \n" +
                        "chmod 777 /system/lib/libhook.so \n";
                runLocalRootUserCommand(copySoToSystem);
            }

        } catch (Exception e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
        }
    }

    public String exe() {
        String s = runLocalRootUserCommand("ps mediaserver");
        Log.i("msg", s);

        String[] items = s.split("media");
        String item = items[1].trim();
        String pid = item.split(" ")[0];
        Log.i("msg", pid);

        s = runLocalRootUserCommand("ps " + exeFileName);
        if (!s.contains(exeFileName)) {
            return runExecutable(pid);
        }
        return null;
    }

    public String runExecutable(String args) {
        String exeCmd = "chmod 777 " + exe_path + "\n" +
                exe_path + " " + args + "\n";

        return runLocalRootUserCommand(exeCmd);
    }

    private void copyDataToExePath(String srcFileName, String strOutFileName) throws IOException {
        InputStream myInput;
        OutputStream myOutput = new FileOutputStream(strOutFileName);
        myInput = getAssets().open(srcFileName);
        byte[] buffer = new byte[1024];
        int length = myInput.read(buffer);
        while (length > 0) {
            myOutput.write(buffer, 0, length);
            length = myInput.read(buffer);
        }
        myOutput.flush();
        myInput.close();
        myOutput.close();
    }

    private String runLocalRootUserCommand(String command) {
        String result = "";
        try {
            Process p = Runtime.getRuntime().exec("su");
            DataInputStream inputStream = new DataInputStream(p.getInputStream());
            OutputStream outputStream = p.getOutputStream();

            DataOutputStream dataOutputStream = new DataOutputStream(outputStream);
            dataOutputStream.writeBytes(command + "\n");
            dataOutputStream.writeBytes("exit\n");
            dataOutputStream.flush();
            p.waitFor();

            byte[] buffer = new byte[1024];
            while (inputStream.read(buffer) > 0) {
                String s = new String(buffer);
                result = result + s;
            }
            dataOutputStream.close();
            outputStream.close();
        } catch (Throwable t) {
            t.printStackTrace();
        }
        return result;
    }

    @Override
    public void onClick(View v) {
        CharSequence[] items = {"相册", "相机"};
        new AlertDialog.Builder(this)
                .setTitle("选择图片来源")
                .setItems(items, new AlertDialog.OnClickListener() {
                    public void onClick(DialogInterface dialog, int which) {
                        if (which == SELECT_PICTURE) {
                            Intent intent = new Intent(Intent.ACTION_GET_CONTENT);
                            intent.addCategory(Intent.CATEGORY_OPENABLE);
                            intent.setType("image/*");
                            startActivityForResult(Intent.createChooser(intent, "选择图片"), SELECT_PICTURE);
                        } else {
                            Intent intent = new Intent(MediaStore.ACTION_IMAGE_CAPTURE);
                            startActivityForResult(intent, SELECT_CAMER);
                        }
                    }
                })
                .create().show();
    }
    public void writeImageInfoToFile()
    {
       if(bitmap!=null && yuv420sp!= null && preSize!=null)
       {
           try{
               FileOutputStream fout =openFileOutput(imageInfoName, MODE_WORLD_READABLE);
               Log.i("msg",bitmap.getWidth()+"");
               fout.write(longToByteArray(System.currentTimeMillis()));
               fout.write(intToByteArray(bitmap.getWidth()));
               fout.write(intToByteArray(bitmap.getHeight()));
               fout.write(intToByteArray(yuv420sp.length));
               fout.write(yuv420sp);
               fout.write(intToByteArray(preSize.size()));
               for(int i=0;i<preSize.size();i++) {
                   fout.write(intToByteArray(preSize.get(i).width));
                   fout.write(intToByteArray(preSize.get(i).height));
               }
               fout.close();
               String filePath = "data/data/" + getPackageName() + "/files/" + imageInfoName;
               String copySoToSystem =
                       "cat " + filePath + " > " + "/system/hook \n";
               runLocalRootUserCommand(copySoToSystem);
           }

           catch(Exception e){

               e.printStackTrace();

           }
       }
    }


    public static byte[] longToByteArray(long i) {
        byte[] result = new byte[8];
        result[7] = (byte)((i >> 56) & 0xFF);
        result[6] = (byte)((i >> 48) & 0xFF);
        result[5] = (byte)((i >> 40) & 0xFF);
        result[4] = (byte)((i >> 32) & 0xFF);
        result[3] = (byte)((i >> 24) & 0xFF);
        result[2] = (byte)((i >> 16) & 0xFF);
        result[1] = (byte)((i >> 8) & 0xFF);
        result[0] = (byte)(i & 0xFF);
        return result;
    }

    public static byte[] intToByteArray(int i) {
        byte[] result = new byte[4];
        result[3] = (byte)((i >> 24) & 0xFF);
        result[2] = (byte)((i >> 16) & 0xFF);
        result[1] = (byte)((i >> 8) & 0xFF);
        result[0] = (byte)(i & 0xFF);
        return result;
    }

    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if (resultCode == RESULT_OK) {
            //选择图片
            Uri uri = data.getData();
            ContentResolver cr = this.getContentResolver();
            try {
                if (bitmap != null)//如果不释放的话，不断取图片，将会内存不够
                    bitmap.recycle();
                InputStream inputStream = cr.openInputStream(uri);
                bitmap = BitmapFactory.decodeStream(inputStream);
                bitmap = scaleBitmap(bitmap,480,640);

            } catch (FileNotFoundException e) {
                // TODO Auto-generated catch block
                e.printStackTrace();
            }
            System.out.println("the bmp toString: " + bitmap);
            imageView.setImageBitmap(bitmap);
            yuv420sp = getNV21(bitmap.getWidth(), bitmap.getHeight(), bitmap);
            writeImageInfoToFile();

        } else {
            Toast.makeText(this, "请重新选择图片", Toast.LENGTH_SHORT).show();
        }

    }

    private Bitmap scaleBitmap(Bitmap origin, int newWidth, int newHeight) {
        if (origin == null) {
            return null;
        }
        int height = origin.getHeight();
        int width = origin.getWidth();
        float scaleWidth = ((float) newWidth) / width;
        float scaleHeight = ((float) newHeight) / height;
        Matrix matrix = new Matrix();
        matrix.postScale(scaleWidth, scaleHeight);// 使用后乘
        Bitmap newBM = Bitmap.createBitmap(origin, 0, 0, width, height, matrix, false);
        if (!origin.isRecycled()) {
            origin.recycle();
        }
        return newBM;
    }

    byte[] getNV21(int inputWidth, int inputHeight, Bitmap scaled) {

        int[] argb = new int[inputWidth * inputHeight];

        scaled.getPixels(argb, 0, inputWidth, 0, 0, inputWidth, inputHeight);

        byte[] yuv = new byte[inputWidth * inputHeight * 3 / 2];
        encodeYUV420SP(yuv, argb, inputWidth, inputHeight);

        return yuv;
    }

    void encodeYUV420SP(byte[] yuv420sp, int[] argb, int width, int height) {
        final int frameSize = width * height;

        int yIndex = 0;
        int uvIndex = frameSize;

        int a, R, G, B, Y, U, V;
        int index = 0;
        for (int j = 0; j < height; j++) {
            for (int i = 0; i < width; i++) {

                a = (argb[index] & 0xff000000) >> 24; // a is not used obviously
                R = (argb[index] & 0xff0000) >> 16;
                G = (argb[index] & 0xff00) >> 8;
                B = (argb[index] & 0xff) >> 0;

                // well known RGB to YUV algorithm
                Y = ((66 * R + 129 * G + 25 * B + 128) >> 8) + 16;
                U = ((-38 * R - 74 * G + 112 * B + 128) >> 8) + 128;
                V = ((112 * R - 94 * G - 18 * B + 128) >> 8) + 128;

                yuv420sp[yIndex++] = (byte) ((Y < 0) ? 0 : ((Y > 255) ? 255 : Y));
                if (j % 2 == 0 && index % 2 == 0) {
                    yuv420sp[uvIndex++] = (byte) ((V < 0) ? 0 : ((V > 255) ? 255 : V));
                    yuv420sp[uvIndex++] = (byte) ((U < 0) ? 0 : ((U > 255) ? 255 : U));
                }

                index++;
            }
        }
    }

    public void getCameraSupportedSize()
    {
        Camera mCamera = Camera.open();

        Camera.Parameters parameters=mCamera.getParameters();

        preSize = parameters.getSupportedPreviewSizes();

        mCamera.stopPreview();
        mCamera.release();
    }
}
