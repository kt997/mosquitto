#include "pub.h"




int send__disconnect(struct mosquitto *mosq, uint8_t reason_code, const mosquitto_property *properties)
{
	struct mosquitto__packet *packet = NULL;
	int rc;
	int proplen, varbytes;

	assert(mosq);
// #ifdef WITH_BROKER
// #  ifdef WITH_BRIDGE
// 	if(mosq->bridge){
// 		log__printf(mosq, MOSQ_LOG_DEBUG, "Bridge %s sending DISCONNECT", mosq->id);
// 	}else
// #  else
// 	{
// 		log__printf(mosq, MOSQ_LOG_DEBUG, "Sending DISCONNECT to %s (rc%d)", mosq->id, reason_code);
// 	}
// #  endif
// #else
	log__printf(mosq, MOSQ_LOG_DEBUG, "Client %s sending DISCONNECT", mosq->id);
// #endif
	assert(mosq);
	packet = (mosquitto__packet*)calloc(1, sizeof(struct mosquitto__packet));
	if(!packet) return MOSQ_ERR_NOMEM;

	packet->command = CMD_DISCONNECT;
	if(mosq->protocol == mosq_p_mqtt5 && (reason_code != 0 || properties)){
		packet->remaining_length = 1;
		if(properties){
			proplen = property__get_length_all(properties);
			varbytes = packet__varint_bytes(proplen);
			packet->remaining_length += proplen + varbytes;
		}
	}else{
		packet->remaining_length = 0;
	}

	rc = packet__alloc(packet);
	if(rc){
		free(packet);
		return rc;
	}
	if(mosq->protocol == mosq_p_mqtt5 && (reason_code != 0 || properties)){
		packet__write_byte(packet, reason_code);
		if(properties){
			property__write_all(packet, properties, true);
		}
	}

	return packet__queue(mosq, packet);
}
