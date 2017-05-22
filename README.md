# CameraHook
CameraHook is an example to hook and modify android camera preview.

It is just a protype and the code  may be ugly.

## Supported platform
Android 2.3 - 4.4

To use this tool, devices must be root.

If you want to make any change to hook.c while libhook.so has already been installed in /system/lib/, please comment a 'if' condition in mainActivity.java as follow:
```java
//if(result.length()<10) {

    String copySoToSystem = "cat " + so_path + " > " + "/system/lib/libhook.so \n" +
    
            "chmod 777 /system/lib/libhook.so \n";
            
    runLocalRootUserCommand(copySoToSystem);
    
//}
```
If you have any question about this project, feel free to contact me. 

Email: limm.hq@gmail.com

