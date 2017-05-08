# CameraHook
CameraHook is an example to hook and modify android camera preview.
It is just a protype and the code  may be ugly.

Supported platform
Android 2.3 - 4.4

To use this tool, devices must be root.

If you want to make some changes to hook.c and libhook.so has already been installed in /system/lib/, please comment a condition block in mainActivity.java as follow:

//if(result.length()<10) {

    String copySoToSystem = "cat " + so_path + " > " + "/system/lib/libhook.so \n" +
    
            "chmod 777 /system/lib/libhook.so \n";
            
    runLocalRootUserCommand(copySoToSystem);
    
//}

If you have any question about this project, feel free to contact me. 

Email: limm.hq@gmail.com

