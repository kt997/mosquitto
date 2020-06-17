#include "pub.h" 
#include "pub_internals.h" 


#define host "0.0.0.0"
#define port 1883

int main()
{
    // printf("%d", add(12,13));
    struct mosquitto *mosq;
    char msg[100]="This is Published.";
    mosq=mosquitto_new("pub", 0, NULL);
    if(mosq==NULL ) cout<<"failed\n";
    else cout<<"creation passed\n";

    mosquitto_loop_start(mosq);

    int x=mosquitto_connect(mosq, host, port, 60);
    if(x==MOSQ_ERR_SUCCESS ) cout<<"connect passed\n";

    int res=mosquitto_publish(
        mosq,
        NULL,
        "topic/test",
        strlen((char*)msg),
        (char*)msg,
        1,
        1);
    usleep(1000000);
    if(res==MOSQ_ERR_SUCCESS ) cout<<"pub passed\n";
    else cout<<"pub failed";

    mosquitto_disconnect(mosq);
    
    mosquitto_loop_stop(mosq,false);

    // mosquitto_lib_cleanup();
    
    return 0;
}


