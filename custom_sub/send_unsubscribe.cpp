#include "sub.h"


int send__unsubscribe(struct mosquitto *mosq, int *mid, int topic_count, char *const *const topic, const mosquitto_property *properties)
{
	/* FIXME - only deals with a single topic */
	struct mosquitto__packet *packet = NULL;
	uint32_t packetlen;
	uint16_t local_mid;
	int rc;
	int proplen, varbytes;
	int i;

	assert(mosq);
	assert(topic);

	packet = (mosquitto__packet*)calloc(1, sizeof(struct mosquitto__packet));
	if(!packet) return MOSQ_ERR_NOMEM;

	packetlen = 2;

	for(i=0; i<topic_count; i++){
		packetlen += 2+strlen(topic[i]);
	}
	if(mosq->protocol == mosq_p_mqtt5){
		proplen = property__get_length_all(properties);
		varbytes = packet__varint_bytes(proplen);
		packetlen += proplen + varbytes;
	}

	packet->command = CMD_UNSUBSCRIBE | (1<<1);
	packet->remaining_length = packetlen;
	rc = packet__alloc(packet);
	if(rc){
		free(packet);
		return rc;
	}

	/* Variable header */
	local_mid = mosquitto__mid_generate(mosq);
	if(mid) *mid = (int)local_mid;
	packet__write_uint16(packet, local_mid);

	if(mosq->protocol == mosq_p_mqtt5){
		/* We don't use User Property yet. */
		property__write_all(packet, properties, true);
	}

	/* Payload */
	for(i=0; i<topic_count; i++){
		packet__write_string(packet, topic[i], strlen(topic[i]));
	}

// #ifdef WITH_BROKER
// # ifdef WITH_BRIDGE
// 	for(i=0; i<topic_count; i++){
// 		log__printf(mosq, MOSQ_LOG_DEBUG, "Bridge %s sending UNSUBSCRIBE (Mid: %d, Topic: %s)", mosq->id, local_mid, topic[i]);
// 	}
// # endif
// #else
	for(i=0; i<topic_count; i++){
		log__printf(mosq, MOSQ_LOG_DEBUG, "Client %s sending UNSUBSCRIBE (Mid: %d, Topic: %s)", mosq->id, local_mid, topic[i]);
	}
// #endif
	return packet__queue(mosq, packet);
}

