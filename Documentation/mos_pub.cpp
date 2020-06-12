#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <mosquitto.h>

#define host "0.0.0.0"
#define port 1883

using namespace std;

// void my_publish_callback()

int main()
{
    mosquitto_lib_init();
    mosquitto *mosq;
    
    char msg[100]="This is Published.";
    mosq=mosquitto_new("pub1", 0, NULL);
    int loop=mosquitto_loop_start(mosq);

    // cout<<"loop:"<<loop<<endl<<"errno:"<<MOSQ_ERR_SUCCESS<<endl;
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