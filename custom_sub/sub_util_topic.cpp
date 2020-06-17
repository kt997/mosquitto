#include "sub.h"



int mosquitto_sub_topic_check(const char *str)
{
	char c = '\0';
	int len = 0;
// #ifdef WITH_BROKER
// 	int hier_count = 0;
// #endif

	while(str && str[0]){
		if(str[0] == '+'){
			if((c != '\0' && c != '/') || (str[1] != '\0' && str[1] != '/')){
				return MOSQ_ERR_INVAL;
			}
		}else if(str[0] == '#'){
			if((c != '\0' && c != '/')  || str[1] != '\0'){
				return MOSQ_ERR_INVAL;
			}
		}
// #ifdef WITH_BROKER
// 		else if(str[0] == '/'){
// 			hier_count++;
// 		}
// #endif
		len++;
		c = str[0];
		str = &str[1];
	}
	if(len > 65535) return MOSQ_ERR_INVAL;
// #ifdef WITH_BROKER
// 	if(hier_count > TOPIC_HIERARCHY_LIMIT) return MOSQ_ERR_INVAL;
// #endif

	return MOSQ_ERR_SUCCESS;
}