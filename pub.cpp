#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <mosquitto.h>
#include <stdio.h>
#include <pthread.h>


#define client_id1 "sub"
#define host "0.0.0.0"
#define port 1883

using namespace std;

int my_message_callback(struct mosquitto *mosq, void *userdata, const struct mosquitto_message *message)
{
	if(message->payloadlen){
        cout<<(char *)userdata<<" ";
		cout<<message->topic<<endl<<(char*)message->payload<<endl;
	}else{
		cout<<message->topic<<endl;
	}
	// fflush(stdout);
    return 0;
}

void * pub_it(void *args)
{
    mosquitto *mosq;
    
    char msg[100]="This is Published.";
    mosq=mosquitto_new("pub1", 0, NULL);
    if(!mosq){
		fprintf(stderr, "Error: Instance not made.\n");
		exit(1);
	}
    mosquitto_loop_start(mosq);

    int rc=mosquitto_connect(mosq, host, port, 60);
    while(1){
    cout<<"hello"<<endl;
    int res=mosquitto_publish(
        mosq,
        NULL,
        "topic/test",
        strlen((char*)msg),
        (char*)msg,
        2,
        1);
    
    // cout<<res<<endl<<rc<<endl;   
    usleep(5000000);     //Neccessary. Won't work without this.
    }
    mosquitto_disconnect(mosq);
    
    mosquitto_loop_stop(mosq,false);

}

void * sub_it(void *args)
{
    mosquitto *mosqs;
    // char *host= "localhost";
    void *usr_obj;
    int res=0;
    mosqs=mosquitto_new(client_id1, 0, NULL);
    if(!mosqs){
		fprintf(stderr, "Error: Instance not made.\n");
		exit(1);
	}
    mosquitto_loop_start(mosqs);
    // mosquitto_connect(mosq, host, port, 60);     //not needed because the sub_callb function itself makes a connection.
    
    res=mosquitto_subscribe_callback(
        my_message_callback,
        (void*)"hello",
        "topic/testt",
        2,
        "0.0.0.0",
        1883,
        client_id1,
        60,
        0,
        NULL,
        NULL,
        NULL,
        NULL);
    mosquitto_disconnect(mosqs);
    mosquitto_loop_stop(mosqs, false);
}

int main()
{
    pthread_t subt, pubt;
    mosquitto_lib_init();
    pthread_create(&subt, NULL, sub_it, NULL);
    pthread_create(&pubt, NULL, pub_it, NULL);
    // sub_it();
    // pub_it();
    pthread_join(subt,NULL);
    pthread_join(pubt,NULL);

    mosquitto_lib_cleanup();
    return 0;
}