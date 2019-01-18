# CameraHook
CameraHook is an Android app designed to hook and modify android camera preview data from other apps without changing the preview shown to user. 
More specifically, CameraHook can modify camera data of any third-party app or system camera app which use the ```java onPreviewFrame ``` system callback
```java
public void onPreviewFrame(byte[] data, Camera camera) {
  //CameraHook can hook the data here without changing the preview
}
```

For example, you can use this tool to change a QR code scanner result by replacing the camera data with a picture set by yourself.
It is just a prototype and the code may be ugly

## Supported platform
Android 2.3 - 4.4

## Tips
You should use Android studio to build this project.
To use this tool, devices must be root.

If you want to make any change to hook.c while libhook.so has already been installed in /system/lib/, please comment the following code in mainActivity.java:
```java
if(result.length()<10) {

    String copySoToSystem = "cat " + so_path + " > " + "/system/lib/libhook.so \n" +
    
            "chmod 777 /system/lib/libhook.so \n";
            
    runLocalRootUserCommand(copySoToSystem);
    
}
```
If you have any question about this project, feel free to contact me. 

Email: limm.hq@gmail.com

