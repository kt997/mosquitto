#include <stdio.h>
#include <mosquitto.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void mosq_log_callback(struct mosquitto *mosq, void *userdata, int level, const char *str)
{
	/* Pring all log messages regardless of level. */
  
  switch(level){
    //case MOSQ_LOG_DEBUG:
    //case MOSQ_LOG_INFO:
    //case MOSQ_LOG_NOTICE:
    case MOSQ_LOG_WARNING:
    case MOSQ_LOG_ERR: {
      printf("%i:%s\n", level, str);
    }
  }
  
	
}

struct mosquitto *mosq = NULL;
const char *topic = NULL;
void mqtt_setup(){

	const char *host = "localhost";
	int port = 1883;
	int keepalive = 60;
	bool clean_session = true;
  topic = "topic/test";
  
  mosquitto_lib_init();
  mosq = mosquitto_new(NULL, clean_session, NULL);
  if(!mosq){
		fprintf(stderr, "Error: Out of memory.\n");
		exit(1);
	}
  
  mosquitto_log_callback_set(mosq, mosq_log_callback);
  
  if(mosquitto_connect(mosq, host, port, keepalive)){
		fprintf(stderr, "Unable to connect.\n");
		exit(1);
	}
  int loop = mosquitto_loop_start(mosq);
  if(loop != MOSQ_ERR_SUCCESS){
    fprintf(stderr, "Unable to start loop: %i\n", loop);
    exit(1);
  }
}

int mqtt_send(char *msg){
  return mosquitto_publish(mosq, NULL, topic, strlen(msg), msg, 0, 0);
}

int main(int argc, char *argv[])
{
  mqtt_setup();
  int i = -10;
//   char *buf = malloc(64);
  char buf[64]="Published\n";
//   mqtt_send(buf);
//   fflush(stdout);
    int snd = mqtt_send(buf);
    // sprintf(buf,"i=%i",i++);
    // if(snd != 0) printf("mqtt_send error=%i\n", snd);
    usleep(100000);
//   while(1){
//     int snd = mqtt_send(buf);
//     sprintf(buf,"i=%i",i++);
//     if(snd != 0) printf("mqtt_send error=%i\n", snd);
//     usleep(100000);
//   }
}














// #include <stdio.h>
// #include <mosquitto.h>
// #include <stdlib.h>
// #include <string.h>
// #include <unistd.h>

// void mosq_log_callback(struct mosquitto *mosq, void *userdata, int level, const char *str)
// {
// 	/* Pring all log messages regardless of level. */
  
//   switch(level){
//     //case MOSQ_LOG_DEBUG:
//     //case MOSQ_LOG_INFO:
//     //case MOSQ_LOG_NOTICE:
//     case MOSQ_LOG_WARNING:
//     case MOSQ_LOG_ERR: {
//       printf("%i:%s\n", level, str);
//     }
//   }
  
	
// }

// struct mosquitto *mosq = NULL;
// const char *topic = NULL;
// void mqtt_setup(){

// 	const char *host = "localhost";
// 	int port = 1883;
// 	int keepalive = 60;
// 	bool clean_session = true;
//   topic = "topic/test";
  
//   mosquitto_lib_init();
//   mosq = mosquitto_new(NULL, clean_session, NULL);
//   if(!mosq){
// 		fprintf(stderr, "Error: Out of memory.\n");
// 		exit(1);
// 	}
  
//   mosquitto_log_callback_set(mosq, mosq_log_callback);
  
//   if(mosquitto_connect(mosq, host, port, keepalive)){
// 		fprintf(stderr, "Unable to connect.\n");
// 		exit(1);
// 	}
//   int loop = mosquitto_loop_start(mosq);
//   if(loop != MOSQ_ERR_SUCCESS){
//     fprintf(stderr, "Unable to start loop: %i\n", loop);
//     exit(1);
//   }
// }

// int mqtt_send(char *msg){
//   return mosquitto_publish(mosq, NULL, topic, strlen(msg), msg, 0, 0);
// }

// int main(int argc, char *argv[])
// {
//   mqtt_setup();
//   int i = -1000;
// //   char *buf = malloc(64);
//   char buf[64]="";
//   while(1){
//     sprintf(buf,"i=%i",i++);
//     int snd = mqtt_send(buf);
//     if(snd != 0) printf("mqtt_send error=%i\n", snd);
//     usleep(100000);
//   }
// }