#include <iostream>
#include <string.h>
#include <mosquitto.h>

#define client_id1 "sub1"
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

int main()
{
    mosquitto *mosq;
    // char *host= "localhost";
    void *usr_obj;
    mosquitto_lib_init();
    mosq=mosquitto_new(client_id1, 0, NULL);
    mosquitto_loop_start(mosq);
    // mosquitto_connect(mosq, host, port, 60);     //not needed because the sub_callb function itself makes a connection.
    
    int res=0;
    res=mosquitto_subscribe_callback(
        my_message_callback,
        (void*)"hello",
        "#",
        2,
        "0.0.0.0",
        1883,
        "sub1",
        60,
        0,
        NULL,
        NULL,
        NULL,
        NULL);
    mosquitto_disconnect(mosq);
    mosquitto_loop_stop(mosq, false);
    mosquitto_lib_cleanup();
    return 0;
}








// #include <stdio.h>
// #include <mosquitto.h>

// #define mqtt_host "127.0.0.1"
// #define mqtt_port 1883

// int func(struct mosquitto *mosq, void *userdata, const struct mosquitto_message *m)
// {
//     char *ud = (char *)userdata;

//     printf("%s -> %s %s\n", ud, m->topic, (char*)m->payload);
//     return (0);	/* True to break out of loop */
// }

// int main(int argc, char *argv[])
// {
//     int rc;

//     mosquitto_lib_init();

//     rc = mosquitto_subscribe_callback(
//         func,            /* callback function */
//         (void*)"hello",         /* user data for callback */
//         "topic/test",          /* topic */
//         0,               /* QoS */
//         mqtt_host,       /* host */
//         mqtt_port,       /* port */
//         "jp-cb",         /* clientID */
//         60,              /* keepalive */
//         1,               /* clean session */
//         NULL,            /* username */
//         NULL,            /* password */
//         NULL,            /* libmosquitto_will */
//         NULL);             /* libmosquitto_tls */
        

//     if (rc != MOSQ_ERR_SUCCESS) {
//         printf("rc = %d %s\n", rc, mosquitto_strerror(rc));
//     }

//     mosquitto_lib_cleanup();
//     return (rc);
// }