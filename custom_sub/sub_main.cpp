#include "sub.h" 
#include "sub_internals.h" 

#define client_id1 "sub2"
#define pattern "#"
#define host "localhost"
#define port 1883
#define cafile "ca.crt"

using namespace std;

// int on_password_check(char *buf, int size, int rwflag, void *userdata)
// {
// 	int length = 0;
// 	// if(!buf)
// 		return 0;
// }


void my_message_callback(struct mosquitto *mosq, void *userdata, const struct mosquitto_message *message)
{
	if(message->payloadlen){
        cout<<(char *)userdata<<" ";
		cout<<message->topic<<endl<<(char*)message->payload<<endl;
	}else{
		cout<<message->topic<<endl;
	}
	// fflush(stdout);
    // return 0;
}

void my_subscribe_callback(mosquitto *mosq, void *obj, int mid, int qos_count, const int *granted_qos)
    {
        int i;

        printf("Subscribed (mid: %d): %d", mid, granted_qos[0]);
        for(i=1; i<qos_count; i++){
                printf(", %d", granted_qos[i]);
        }
        printf("\n");
    }

int main()
{
    mosquitto *mosq;

    // mosquitto_lib_init();
    int res=0;
    mosq=mosquitto_new(client_id1, 0, NULL);
    if(!mosq){
		fprintf(stderr, "Error: Instance not made.\n");
		exit(1);
	}
    else cout<<"passes\n";
    mosquitto_loop_start(mosq);

    mosquitto_connect(mosq, host, port, 60);     
    mosquitto_subscribe(mosq, NULL, pattern, 2);
    mosquitto_message_callback_set(mosq, my_message_callback);
    
    // mosquitto_disconnect(mosq);
    mosquitto_loop_stop(mosq, false);
    // mosquitto_lib_cleanup();
    return 0;
}