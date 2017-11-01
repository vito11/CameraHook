package me.tino.research.camerahook

import android.annotation.SuppressLint
import android.app.Activity
import android.content.Context
import android.content.Intent
import android.graphics.Bitmap
import android.graphics.BitmapFactory
import android.graphics.Matrix
import android.hardware.Camera
import android.net.Uri
import android.os.AsyncTask
import android.os.Bundle
import android.support.design.widget.TextInputEditText
import android.support.design.widget.TextInputLayout
import android.support.v7.app.AppCompatActivity
import android.text.Editable
import android.text.TextWatcher
import android.util.Log
import android.view.View
import android.widget.Button
import android.widget.Toast

import com.github.hiteshsondhi88.libffmpeg.FFmpeg
import com.github.hiteshsondhi88.libffmpeg.FFmpegExecuteResponseHandler
import com.github.hiteshsondhi88.libffmpeg.FFmpegLoadBinaryResponseHandler
import com.tino.research.camerahook.R

import java.io.DataInputStream
import java.io.DataOutputStream
import java.io.File
import java.io.FileInputStream
import java.io.FileOutputStream
import java.io.FilenameFilter
import java.io.IOException
import java.io.InputStream
import java.io.OutputStream
import java.util.UUID

class MainActivity : AppCompatActivity(), View.OnClickListener {

    internal var exeFileName = "injector"
    internal lateinit var exe_path: String
    internal var soFileName = "libhook.so"
    internal lateinit var so_path: String
    internal var imageInfoName = "fakeImage"
    internal var videoInfoName = "fakeVideo"
    internal lateinit var imageInfo_path: String
    internal lateinit var button: Button
    internal val SELECT_VIDEO = 2
    internal var bitmap: Bitmap? = null
    internal var yuv420sp: ByteArray? = null
    internal var preSize: List<Camera.Size>? = null
    internal var ffmpeg: FFmpeg? = null

    private var fpsEditText: TextInputEditText? = null
    private var fpsLayout: TextInputLayout? = null
    private var qualityEditText: TextInputEditText? = null
    private var qualityLayout: TextInputLayout? = null

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        button = findViewById(R.id.setvideo) as Button
        button.setOnClickListener(this)

        fpsLayout = findViewById(R.id.fpswrapper) as TextInputLayout
        fpsEditText = findViewById(R.id.fps) as TextInputEditText
        qualityEditText = findViewById(R.id.quality) as TextInputEditText
        qualityLayout = findViewById(R.id.qualitywrapper) as TextInputLayout

        fpsLayout!!.editText!!.addTextChangedListener(object : TextWatcher {
            var st: Int = 0;
            var ct: Int = 0;

            override fun afterTextChanged(s: Editable?) {
                if (s.isNullOrEmpty()) {
                    fpsLayout!!.isErrorEnabled = false
                    return
                }
                if (s?.toString()?.toInt() !in 1..30) {
                    val result = s?.delete(st, st + ct)
                    fpsLayout!!.editText!!.text = result
                    fpsLayout!!.error = "帧率只能在1到30之间"
                    fpsLayout!!.isErrorEnabled = true
                    fpsLayout!!.editText!!.setSelection(result?.length ?: 0)
                } else {
                    fpsLayout!!.isErrorEnabled = false
                }
            }

            override fun beforeTextChanged(s: CharSequence?, start: Int, count: Int, after: Int) {
                st = start
                ct = after
            }

            override fun onTextChanged(s: CharSequence?, start: Int, before: Int, count: Int) {
            }
        })

        qualityLayout!!.editText!!.addTextChangedListener(object : TextWatcher {
            var st: Int = 0;
            var ct: Int = 0;

            override fun afterTextChanged(s: Editable?) {
                if (s.isNullOrEmpty()) {
                    qualityLayout!!.isErrorEnabled = false
                    return
                }
                if (s!!.toString().toInt() !in 2..31) {
                    val result = s.delete(st, st + ct)
                    qualityLayout!!.editText!!.text = result
                    qualityLayout!!.error = "质量只能在2到31之间"
                    qualityLayout!!.isErrorEnabled = true
                    qualityLayout!!.editText!!.setSelection(result.length)
                } else {
                    qualityLayout!!.isErrorEnabled = false
                }
            }

            override fun beforeTextChanged(s: CharSequence?, start: Int, count: Int, after: Int) {
                st = start
                ct = after
            }

            override fun onTextChanged(s: CharSequence?, start: Int, before: Int, count: Int) {
            }
        })

        runLocalRootUserCommand("setenforce 0")
        remountSystem()

        initExecutableFile()
        getCameraSupportedSize()
        exe()
        loadFFmpeg()
    }

    // remount /system to read and write
    fun remountSystem() {
        var s = runLocalRootUserCommand("mount")

        var a = s.indexOf("/system", 0)
        while (true) {
            if (a < 0) {
                Log.e("msg", "error: can not remount system!")
                return
            }
            if (a + 7 > s.length) {
                Log.e("msg", "error: can not remount system!")
                return
            }
            if (s[a - 1] == ' ' && s[a + 7] == ' ') {
                break
            }
            a = s.indexOf("/system", a + 7)
        }

        s = s.substring(0, a)
        val temp = s.split("\n".toRegex()).dropLastWhile { it.isEmpty() }.toTypedArray()
        if (temp.size == 0) {
            Log.e("msg", "error: can not remount system!")
            return
        }
        s = temp[temp.size - 1]

        val remountString = "mount -o remount $s /system"
        Log.i("msg", remountString)
        runLocalRootUserCommand(remountString)
    }

    fun initExecutableFile() {
        exe_path = "data/data/$packageName/$exeFileName"
        so_path = "data/data/$packageName/$soFileName"
        imageInfo_path = "data/data/$packageName/$imageInfoName"
        try {
            copyDataToExePath(exeFileName, exe_path)
            copyDataToExePath(soFileName, so_path)
            val result = runLocalRootUserCommand("ls /system/lib/libhook.so")
            //            if (result.length() < 10) {
            val copySoToSystem = "cat " + so_path + " > " + "/system/lib/libhook.so \n" +
                    "chmod 777 /system/lib/libhook.so \n"
            runLocalRootUserCommand(copySoToSystem)
            //            }

        } catch (e: Exception) {
            e.printStackTrace()
        }

    }

    fun exe(): String? {
        var s = runLocalRootUserCommand("ps mediaserver")
        Log.i("msg", s)

        val items = s.split("media".toRegex()).dropLastWhile { it.isEmpty() }.toTypedArray()
        val item = items[1].trim { it <= ' ' }
        val pid = item.split(" ".toRegex()).dropLastWhile { it.isEmpty() }.toTypedArray()[0]
        Log.i("msg", pid)

        s = runLocalRootUserCommand("ps " + exeFileName)
        return if (!s.contains(exeFileName)) {
            runExecutable(pid)
        } else null
    }

    fun runExecutable(args: String): String {
        val exeCmd = "chmod 777 " + exe_path + "\n" +
                exe_path + " " + args + "\n"

        return runLocalRootUserCommand(exeCmd)
    }

    @Throws(IOException::class)
    private fun copyDataToExePath(srcFileName: String, strOutFileName: String) {
        val myInput: InputStream
        val myOutput = FileOutputStream(strOutFileName)
        myInput = assets.open(srcFileName)
        val buffer = ByteArray(1024)
        var length = myInput.read(buffer)
        while (length > 0) {
            myOutput.write(buffer, 0, length)
            length = myInput.read(buffer)
        }
        myOutput.flush()
        myInput.close()
        myOutput.close()
    }

    private fun runLocalRootUserCommand(command: String): String {
        var result = ""
        try {
            val p = Runtime.getRuntime().exec("su")
            val inputStream = DataInputStream(p.inputStream)
            val outputStream = p.outputStream

            val dataOutputStream = DataOutputStream(outputStream)
            dataOutputStream.writeBytes(command + "\n")
            dataOutputStream.writeBytes("exit\n")
            dataOutputStream.flush()
            p.waitFor()

            val buffer = ByteArray(1024)
            while (inputStream.read(buffer) > 0) {
                val s = String(buffer)
                result = result + s
            }
            dataOutputStream.close()
            outputStream.close()
        } catch (t: Throwable) {
            t.printStackTrace()
        }

        return result
    }

    override fun onClick(v: View) {
        when (v.id) {
            R.id.setvideo -> {
                if (!checkParameter()) {
                    return
                }
                val intent = Intent(Intent.ACTION_GET_CONTENT)
                intent.type = "video/*"
                startActivityForResult(Intent.createChooser(intent, "选择视频"), SELECT_VIDEO)
            }
        }
        //        CharSequence[] items = {"视频"};
        //        new AlertDialog.Builder(this)
        //                .setTitle("选择来源")
        //                .setItems(items, new AlertDialog.OnClickListener() {
        //                    public void onClick(DialogInterface dialog, int which) {
        //                        if (which == SELECT_PICTURE) {
        //                            Intent intent = new Intent(Intent.ACTION_GET_CONTENT);
        //                            intent.addCategory(Intent.CATEGORY_OPENABLE);
        //                            intent.setType("image/*");
        //                            startActivityForResult(Intent.createChooser(intent, "选择图片"), SELECT_PICTURE);
        //                        } else if (which == SELECT_CAMER) {
        //                            Intent intent = new Intent(MediaStore.ACTION_IMAGE_CAPTURE);
        //                            startActivityForResult(intent, SELECT_CAMER);
        //                        } else {
        //
        //                        }
        //                    }
        //                })
        //                .create().show();
    }

    private fun checkParameter(): Boolean {
        if (fpsLayout!!.editText!!.text.toString().toIntOrNull()?:0 !in 1..30) {
            fpsLayout!!.error = "帧率只能在1到30之间"
            return false
        }
        if (qualityLayout!!.editText!!.text.toString().toIntOrNull()?:0 !in 2..31) {
            qualityLayout!!.error = "质量只能在2到31之间"
            return false
        }
        return true
    }

    @SuppressLint("WorldReadableFiles")
    fun writeImageInfoToFile() {
        if (bitmap != null && yuv420sp != null && preSize != null) {
            try {
                val fout = openFileOutput(imageInfoName, Context.MODE_WORLD_READABLE)
                Log.i("msg", bitmap!!.width.toString() + "")
                fout.write(longToByteArray(System.currentTimeMillis()))
                fout.write(intToByteArray(bitmap!!.width))
                fout.write(intToByteArray(bitmap!!.height))
                fout.write(intToByteArray(yuv420sp!!.size))
                fout.write(yuv420sp!!)
                fout.write(intToByteArray(preSize!!.size))
                for (i in preSize!!.indices) {
                    fout.write(intToByteArray(preSize!![i].width))
                    fout.write(intToByteArray(preSize!![i].height))
                }
                fout.close()
                val filePath = "data/data/$packageName/files/$imageInfoName"
                val copySoToSystem = "cat $filePath > /system/hook \n"
                runLocalRootUserCommand(copySoToSystem)
            } catch (e: Exception) {

                e.printStackTrace()

            }

        }
    }

    override fun onActivityResult(requestCode: Int, resultCode: Int, data: Intent) {
        super.onActivityResult(requestCode, resultCode, data)
        if (resultCode == Activity.RESULT_OK) {
            when (requestCode) {
                SELECT_VIDEO -> {
                    showProgress()
                    val uri = data.data
                    val oldPath = uri.path
                    Log.e(TAG, "oldPath " + oldPath)
                    val path = UriUtils.getPath(this, uri)
                    try {
                        cleanPreviousRes()
                        covertVideo(path)
                    } catch (throwable: Throwable) {
                        hideProgress()
                        throwable.printStackTrace()
                    }

                }
            }
        } else {
            Toast.makeText(this, "请重新选择图片", Toast.LENGTH_SHORT).show()
        }

    }

    private fun scaleBitmap(origin: Bitmap?, newWidth: Int, newHeight: Int): Bitmap? {
        if (origin == null) {
            return null
        }
        val height = origin.height
        val width = origin.width
        val scaleWidth = newWidth.toFloat() / width
        val scaleHeight = newHeight.toFloat() / height
        val matrix = Matrix()
        matrix.postScale(scaleWidth, scaleHeight)// 使用后乘
        val newBM = Bitmap.createBitmap(origin, 0, 0, width, height, matrix, false)
        if (!origin.isRecycled) {
            origin.recycle()
        }
        return newBM
    }

    internal fun getNV21(inputWidth: Int, inputHeight: Int, scaled: Bitmap): ByteArray {

        val argb = IntArray(inputWidth * inputHeight)

        scaled.getPixels(argb, 0, inputWidth, 0, 0, inputWidth, inputHeight)

        val yuv = ByteArray(inputWidth * inputHeight * 3 / 2)
        encodeYUV420SP(yuv, argb, inputWidth, inputHeight)

        return yuv
    }

    internal fun encodeYUV420SP(yuv420sp: ByteArray, argb: IntArray, width: Int, height: Int) {
        val frameSize = width * height

        var yIndex = 0
        var uvIndex = frameSize

        var a: Int
        var R: Int
        var G: Int
        var B: Int
        var Y: Int
        var U: Int
        var V: Int
        var index = 0
        for (j in 0 until height) {
            for (i in 0 until width) {

                a = argb[index] and -0x1000000 shr 24 // a is not used obviously
                R = argb[index] and 0xff0000 shr 16
                G = argb[index] and 0xff00 shr 8
                B = argb[index] and 0xff shr 0

                // well known RGB to YUV algorithm
                Y = (66 * R + 129 * G + 25 * B + 128 shr 8) + 16
                U = (-38 * R - 74 * G + 112 * B + 128 shr 8) + 128
                V = (112 * R - 94 * G - 18 * B + 128 shr 8) + 128

                yuv420sp[yIndex++] = (if (Y < 0) 0 else if (Y > 255) 255 else Y).toByte()
                if (j % 2 == 0 && index % 2 == 0) {
                    yuv420sp[uvIndex++] = (if (V < 0) 0 else if (V > 255) 255 else V).toByte()
                    yuv420sp[uvIndex++] = (if (U < 0) 0 else if (U > 255) 255 else U).toByte()
                }

                index++
            }
        }
    }

    fun getCameraSupportedSize() {
        val mCamera = Camera.open()

        val parameters = mCamera.parameters

        preSize = parameters.supportedPreviewSizes

        mCamera.stopPreview()
        mCamera.release()
    }

    private fun hideProgress() {
        //        runOnUiThread(new Runnable() {
        //            @Override
        //            public void run() {
        //                if (progressDialog != null) {
        //                    progressDialog.dismiss();
        //                    progressDialog = null;
        //                }
        //            }
        //        });
    }

    private fun showProgress() {
        //        if (progressDialog != null) {
        //            return;
        //        }
        //        progressDialog = new ProgressDialog(this);
        //        progressDialog.show(this, "提醒", "正在处理中...");
    }

    private fun cleanPreviousRes() {
        val s = runLocalRootUserCommand("rm -rf $filesDir/tmp/* \n")
        //        runLocalRootUserCommand("rm -rf /system/myResource/* \n");
        //        runLocalRootUserCommand("rm -rf " + getFilesDir() + "/fake* \n");
        Log.i(TAG, "cleanPreviousRes: " + s)
        val file = File(filesDir, "tmp")
        if (!file.exists()) {
            file.mkdir()
        }
    }

    //把视频文件转成一系列图片
    @Throws(Throwable::class)
    private fun covertVideo(path: String?) {
        Log.d(TAG, "covertVideo() called with: path = [$path]")

        val args = split("-y -i  $path -vf scale=480/640,setdar=3/4 -r ${fpsLayout!!.editText!!.text} -q:v ${qualityLayout!!.editText!!.text} $filesDir/tmp/%03d.jpg")
        ffmpeg!!.execute(args, object : SimpleFFmpegHandler() {
            override fun onSuccess(message: String) {
                super.onSuccess(message)

                BackgroundTask().execute()
            }

            override fun onFailure(message: String) {
                super.onFailure(message)
                hideProgress()
            }
        })
    }

    //加载ffmpeg
    private fun loadFFmpeg() {
        ffmpeg = FFmpeg.getInstance(this)
        try {
            ffmpeg!!.loadBinary(object : FFmpegLoadBinaryResponseHandler {
                override fun onFailure() {

                }

                override fun onSuccess() {

                }

                override fun onStart() {

                }

                override fun onFinish() {

                }
            })
        } catch (throwable: Throwable) {
            throwable.printStackTrace()
        }

    }

    private fun split(src: String): Array<String> {
        val regulation = "[ \\t]+"
        return src.split(regulation.toRegex()).dropLastWhile { it.isEmpty() }.toTypedArray()
    }

    private inner class BackgroundTask : AsyncTask<Void, Void, Boolean>() {
        @SuppressLint("WorldReadableFiles")
        override fun doInBackground(vararg params: Void): Boolean? {
            try {
                val file = File(filesDir, "tmp")
                val resultFiles = file.listFiles(JPGFileNameFilter())
                val fout = openFileOutput(videoInfoName, Context.MODE_WORLD_READABLE)
                fout.write(intToByteArray(UUID.randomUUID().hashCode()))
                fout.write(intToByteArray(480))
                fout.write(intToByteArray(640))
                fout.write(intToByteArray(resultFiles.size))
                for (f in resultFiles) {
                    var bmp: Bitmap?
                    var yuv420: ByteArray?

                    val inputStream = FileInputStream(f)
                    bmp = BitmapFactory.decodeStream(inputStream)
                    println("the bmp toString: " + bmp!!)
                    yuv420 = getNV21(bmp.width, bmp.height, bmp)
                    fout.write(intToByteArray(yuv420.size))
                    fout.write(yuv420)

                    bmp.recycle()
                    bmp = null
                }
                fout.write(intToByteArray(preSize!!.size))
                for (i in preSize!!.indices) {
                    fout.write(intToByteArray(preSize!![i].width))
                    fout.write(intToByteArray(preSize!![i].height))
                }
                fout.close()
                val filePath = "$filesDir/$videoInfoName"
                val copySoToSystem = ("cat $filePath > /system/hook \n"
                        + "chmod 777 /system/hook \n")
                runLocalRootUserCommand(copySoToSystem)

                //                FileOutputStream fos = openFileOutput("index", MODE_WORLD_READABLE);
                //                fos.write(resultFiles.length);
                //                fos.close();
                //                String indexPath = getFilesDir() + "/index";
                //                String copyToSystem = "cat " + indexPath + " > " + " /system/myIndex \n" +
                //                        "chmod 755 /system/myIndex \n";
                //                runLocalRootUserCommand(copyToSystem);

            } catch (e: Exception) {
                e.printStackTrace()
            }

            return null
        }

        override fun onPostExecute(aVoid: Boolean?) {
            Log.e(TAG, "onPostExecute: finish")
            Toast.makeText(this@MainActivity, "finish", Toast.LENGTH_SHORT).show()
            hideProgress()
        }
    }

    private class JPGFileNameFilter : FilenameFilter {
        override fun accept(dir: File, name: String): Boolean =
                name.endsWith(".jpg") || name.endsWith(".Jpg")
    }

    private open class SimpleFFmpegHandler : FFmpegExecuteResponseHandler {
        override fun onSuccess(message: String) {
            Log.i(TAG, "onSuccess() called with: message = [$message]")
        }

        override fun onProgress(message: String) {
            Log.i(TAG, "onProgress() called with: message = [$message]")
        }

        override fun onFailure(message: String) {
            Log.e(TAG, "onFailure() called with: message = [$message]")
        }

        override fun onStart() {
            Log.d(TAG, "onStart() called")
        }

        override fun onFinish() {
            Log.d(TAG, "onFinish() called")
        }
    }

    companion object {
        private val TAG = MainActivity::class.java.simpleName

        fun longToByteArray(i: Long): ByteArray {
            val result = ByteArray(8)
            result[7] = (i shr 56 and 0xFF).toByte()
            result[6] = (i shr 48 and 0xFF).toByte()
            result[5] = (i shr 40 and 0xFF).toByte()
            result[4] = (i shr 32 and 0xFF).toByte()
            result[3] = (i shr 24 and 0xFF).toByte()
            result[2] = (i shr 16 and 0xFF).toByte()
            result[1] = (i shr 8 and 0xFF).toByte()
            result[0] = (i and 0xFF).toByte()
            return result
        }

        fun intToByteArray(i: Int): ByteArray {
            val result = ByteArray(4)
            result[3] = (i shr 24 and 0xFF).toByte()
            result[2] = (i shr 16 and 0xFF).toByte()
            result[1] = (i shr 8 and 0xFF).toByte()
            result[0] = (i and 0xFF).toByte()
            return result
        }
    }
}
