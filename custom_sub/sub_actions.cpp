#include "sub.h"



int mosquitto_subscribe(struct mosquitto *mosq, int *mid, const char *sub, int qos)
// int mosquitto_subscribe(struct mosquitto *mosq, int *mid, string sub, int qos)
{
	return mosquitto_subscribe_multiple(mosq, mid, 1, (char *const *const)&sub, qos, 0, NULL);
	// return mosquitto_subscribe_multiple(mosq, mid, 1, sub, qos, 0, NULL);
}


int mosquitto_subscribe_v5(struct mosquitto *mosq, int *mid, const char *sub, int qos, int options, const mosquitto_property *properties)
// int mosquitto_subscribe_v5(struct mosquitto *mosq, int *mid, string sub, int qos, int options, const mosquitto_property *properties)
{
	return mosquitto_subscribe_multiple(mosq, mid, 1, (char *const *const)&sub, qos, options, properties);
	// return mosquitto_subscribe_multiple(mosq, mid, 1, &sub, qos, options, properties);
}


int mosquitto_subscribe_multiple(struct mosquitto *mosq, int *mid, int sub_count, char *const *const sub, int qos, int options, const mosquitto_property *properties)
// int mosquitto_subscribe_multiple(struct mosquitto *mosq, int *mid, int sub_count, string *sub, int qos, int options, const mosquitto_property *properties)
{
	const mosquitto_property *outgoing_properties = NULL;
	mosquitto_property local_property;
	int i;
	int rc;
	uint32_t remaining_length = 0;
	int slen;

	if(!mosq || !sub_count || !sub) return MOSQ_ERR_INVAL;
	if(mosq->protocol != mosq_p_mqtt5 && properties) return MOSQ_ERR_NOT_SUPPORTED;
	if(qos < 0 || qos > 2) return MOSQ_ERR_INVAL;
	if((options & 0x30) == 0x30 || (options & 0xC0) != 0) return MOSQ_ERR_INVAL;
	if(mosq->sock == INVALID_SOCKET) return MOSQ_ERR_NO_CONN;

	if(properties){
		if(properties->client_generated){
			outgoing_properties = properties;
		}else{
			memcpy(&local_property, properties, sizeof(mosquitto_property));
			local_property.client_generated = true;
			local_property.next = NULL;
			outgoing_properties = &local_property;
		}
		rc = mosquitto_property_check_all(CMD_SUBSCRIBE, outgoing_properties);
		if(rc) return rc;
	}

	for(i=0; i<sub_count; i++){
		if(mosquitto_sub_topic_check(sub[i])) return MOSQ_ERR_INVAL;
		slen = strlen(sub[i]);
		if(mosquitto_validate_utf8(sub[i], slen)) return MOSQ_ERR_MALFORMED_UTF8;
		remaining_length += 2+slen + 1;
	}

	if(mosq->maximum_packet_size > 0){
		remaining_length += 2 + property__get_length_all(outgoing_properties);
		if(packet__check_oversize(mosq, remaining_length)){
			return MOSQ_ERR_OVERSIZE_PACKET;
		}
	}
	if(mosq->protocol == mosq_p_mqtt311 || mosq->protocol == mosq_p_mqtt31){
		options = 0;
	}

	return send__subscribe(mosq, mid, sub_count, (const char**)sub, qos|options, outgoing_properties);
}


int mosquitto_unsubscribe(struct mosquitto *mosq, int *mid, const char *sub)
{
	return mosquitto_unsubscribe_multiple(mosq, mid, 1, (char *const *const)&sub, NULL);
}

int mosquitto_unsubscribe_v5(struct mosquitto *mosq, int *mid, const char *sub, const mosquitto_property *properties)
{
	return mosquitto_unsubscribe_multiple(mosq, mid, 1, (char *const *const)&sub, properties);
}

int mosquitto_unsubscribe_multiple(struct mosquitto *mosq, int *mid, int sub_count, char *const *const sub, const mosquitto_property *properties)
{
	const mosquitto_property *outgoing_properties = NULL;
	mosquitto_property local_property;
	int rc;
	int i;
	uint32_t remaining_length = 0;
	int slen;

	if(!mosq) return MOSQ_ERR_INVAL;
	if(mosq->protocol != mosq_p_mqtt5 && properties) return MOSQ_ERR_NOT_SUPPORTED;
	if(mosq->sock == INVALID_SOCKET) return MOSQ_ERR_NO_CONN;

	if(properties){
		if(properties->client_generated){
			outgoing_properties = properties;
		}else{
			memcpy(&local_property, properties, sizeof(mosquitto_property));
			local_property.client_generated = true;
			local_property.next = NULL;
			outgoing_properties = &local_property;
		}
		rc = mosquitto_property_check_all(CMD_UNSUBSCRIBE, outgoing_properties);
		if(rc) return rc;
	}

	for(i=0; i<sub_count; i++){
		if(mosquitto_sub_topic_check(sub[i])) return MOSQ_ERR_INVAL;
		slen = strlen(sub[i]);
		if(mosquitto_validate_utf8(sub[i], slen)) return MOSQ_ERR_MALFORMED_UTF8;
		remaining_length += 2+slen;
	}

	if(mosq->maximum_packet_size > 0){
		remaining_length += 2 + property__get_length_all(outgoing_properties);
		if(packet__check_oversize(mosq, remaining_length)){
			return MOSQ_ERR_OVERSIZE_PACKET;
		}
	}

	return send__unsubscribe(mosq, mid, sub_count, sub, outgoing_properties);
}


