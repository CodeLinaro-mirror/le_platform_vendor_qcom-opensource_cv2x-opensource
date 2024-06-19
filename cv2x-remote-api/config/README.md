Build and run C-V2X program on client side (ubuntu x86)

1. run the build script in the /scripts directory
    NOTE: please read the comments in "do_build_native.sh" to see what packages you need to install on ubuntu
    before running the script.

    ./do_build_native.sh telux_header_path [telux_src_path]

    If only "telux_header_path" specified, the build script will build the C-V2X remote API client
    side library(C++ APIs), telux_cv2x_remote.so. Your application that calls the telSDK C-V2X C++ API can be compiled and
    linked against this library as if you are linking with native telSDK library.

    If "telux_src_path" is also specified, in addition of above, the build cript will also build the C-V2X radio C API and
    acme application.

2. If the above build script finished succesfully, it will install all the library in the "build/install"
   directory where the do_build_native.sh reside.


3. Run the client side program.
   Assuming all the client side libraries are installd in "/path_to_build_script/build/install"

   3.1 Copy configuration files "config/commonapi4someip.ini" and "vsomeip-client.json" into
   "build/install" directory

   3.2 Make neccesary modification of "vsomeip-client.json" , change the unicast address of "172.17.0.3" 
   to the ethernet IPv4 address of your ubuntu machine.

   3.3 set the environment variables
       a. export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/path_to_build_script/build/install/lib"
       b. export COMMONAPI_CONFIG=/path_to_build_script/build/install/commonapi4someip.ini
       c. export VSOMEIP_CONFIGURATION=/path_to_build_script/build/install/vsomeip-client.json

   3.4 Make sure the ubuntu ethernet interface is receiving multicast message(assuming interface
   name is "enx00110311eaa5" by executing following command:
       a. route add -nv 224.244.224.245 dev enx00110311eaa5

   3.5 Ensure the service side c-v2x remoting daemon started succesfully. YOu can than start the
   client side program, for example "acme -l 200" to send cv2x packets.

