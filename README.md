This is an application that can be used to send data freely between processes.

It is easy to use with couple of API calls and integrating to other application source codes.
## How To Use
- please check example applications before start
- you need to configure the paths in the 'Config' file first
- then, you need to execute the 'source environment' command
- please note that, sipc daemon should be compiled first because other daemons uses libsipc.so while linking stage
- note that, you may make 'n' the OPEN_DEBUG config in the 'Config' file to disable debugs
- after these steps, you are ready to test it



## Important Notes
- sipcd should be executed before other applications' start
- It is not important to start app_A while app_B wants to send data to app_A because it is queued. BUT, queued data can be sent only once

## How To Use


    //'title' arg should be unique
    sipc_register("App_A_Configuration", &my_callback);
    //example definition of the callback
    int my_callback(void *prm, unsigned int len)
    //while registering, your app is listening data pushed to title by other applications
    //you may unregister from the title bu using the below call
    sipc_unregister(title)
    //you may send data to any title by using;
    sipc_send_data(title, data, len)
    //you may broadcast a data to all subscriber wit the call below;
    sipc_broadcast(data, len)
    //have to use destroy call before exiting your app
    sipc_destroy()
