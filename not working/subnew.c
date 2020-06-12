
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mosquitto.h>

#define client_id1 "sub1"
#define HOST "localhost"
#define PORT 8883
#define CAFILE "ca.crt"


int my_message_callback(struct mosquitto *mosq, void *userdata, const struct mosquitto_message *message)
{
	if(message->payloadlen){
        // cout<<(char *)userdata<<" ";
		// cout<<message->topic<<endl<<(char*)message->payload<<endl;
	}else{
		// cout<<message->topic<<endl;/
	}
	// fflush(stdout);
    return 0;
}

int main()
{
    struct mosquitto *mosq;
    // void *usr_obj;
    char *caloc="ca.crt";
    mosquitto_lib_init();
    int res=0;
    mosq=mosquitto_new(client_id1, 0, NULL);
    if(!mosq){
		fprintf(stderr, "Error: Instance not made.\n");
		exit(1);
	}
    mosquitto_loop_start(mosq);
	
    struct libmosquitto_tls *tls;
    tls->cafile=caloc;

    // mosquitto_connect(mosq, HOST, PORT, 60);     //not needed because the sub_callb function itself makes a connection.
    
    res=mosquitto_subscribe_callback(
        my_message_callback,
        (void*)"hello",
        "#",
        2,
        HOST,
        PORT,
        client_id1,
        60,
        0,
        NULL,
        NULL,
        NULL,
        tls);
    mosquitto_disconnect(mosq);
    mosquitto_loop_stop(mosq, false);
    mosquitto_lib_cleanup();
    return 0;
}








