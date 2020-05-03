#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <mosquitto.h>

#define host "localhost"
#define port 8883
#define cafile "ca.crt"

using namespace std;

// void my_publish_callback()

int main()
{
    mosquitto_lib_init();
    mosquitto *mosq;
    
    char msg[100]="This is Published.";
    mosq=mosquitto_new("pub1", 0, NULL);
    mosquitto_loop_start(mosq);
    // mosquitto_tls_insecure_set(mosq, true);
    int ret=mosquitto_tls_set(mosq, cafile, NULL, NULL, NULL, NULL);
    mosquitto_connect(mosq, host, port, 60);
    
    cout<<"hello"<<endl;
    int res=mosquitto_publish(
        mosq,
        NULL,
        "topic/test",
        strlen((char*)msg),
        (char*)msg,
        2,
        1);
    cout<<res<<endl;
    usleep(1000000);     //Neccessary. Won't work without this.
    
    mosquitto_disconnect(mosq);
    
    mosquitto_loop_stop(mosq,false);

    mosquitto_lib_cleanup();
    return 0;
}