This is an application that can be used to send data freely between processes.

It is easy to use with couple of API calls and integrating to other application source codes.

## How To Use
- sipcd daemon should be compiled and started first. This is the manager daemon and must be started before other daemons' registration!
- Note that, you may type 'n' the OPEN_DEBUG config in the 'Config' file to disable debugs
- After compilation, libsipc.so should be created in the current directory.
- After the library creation, test applications can be compiled and run
- You need to configure the paths in the 'Config' file first (Config file exists for test-app-A and test-app-B seperately)
- Then, you need to execute the 'source environment' command. This is necessary for the applications to find the libsipc.so in runtime
- Then we are OK. Test applications can be run



## Important Notes
- sipcd must be executed before other applications' registration
- sipcd can serve number of 'BACKLOG' applications
- If an application sends data to a title and if there is no application registered to this title before, we are calling this data as orphan. sipcd queues these orphan data and serves them when an application registers the specified title. Please note that, after sending these orphans to any related application, these data are cleared. It means, queuing system works just fo once for the first registered application to the orphan title.

## How To Use

> _sipc_identifier identifier  
>> this type definition is used for all API cals. It is initalized with subscription  

> int sipc_register(_sipc_identifier *identifier, char *title, int (*callback)(void *, unsigned int));  
>> This function is used to register a 'title'  
>> 'callback' is the callback function that automatically executed if there is any incoming data. Passing args to that callback are data itself and its lenght  
>> eg callback definition: **int my_callback(void *prm, unsigned int len)**  
>> Please note that, single callback function can be defined and it must be used as argument for the **first** 'sipc_register' call.  
>> For the other 'sipc_register' calls, callback arg can bu set as NULL

> int sipc_broadcast(_sipc_identifier *identifier, void *data, unsigned int len);  
>> used to send data to all sipcd registerers including the caller process  

> int sipc_send_data(_sipc_identifier *identifier, char *title, void *data, unsigned int len);  
>> used to send data to specific 'title' listeners  

> int sipc_unregister(_sipc_identifier *identifier, char *title);  
>> used to be removed from 'title' caller list  

> int sipc_destroy(_sipc_identifier *identifier);  
>> used for freed all allocated memories hold by the library  
