#include "pub.h"



int send__connect(struct mosquitto *mosq, uint16_t keepalive, bool clean_session, const mosquitto_property *properties)
{
	struct mosquitto__packet *packet = NULL;
	int payloadlen;
	uint8_t will = 0;
	uint8_t byte;
	int rc;
	uint8_t version;
	char *clientid, *username, *password;
	int headerlen;
	int proplen = 0, will_proplen, varbytes;
	mosquitto_property *local_props = NULL;
	uint16_t receive_maximum;

	assert(mosq);

	if(mosq->protocol == mosq_p_mqtt31 && !mosq->id) return MOSQ_ERR_PROTOCOL;

// #if defined(WITH_BROKER) && defined(WITH_BRIDGE)
// 	if(mosq->bridge){
// 		clientid = mosq->bridge->remote_clientid;
// 		username = mosq->bridge->remote_username;
// 		password = mosq->bridge->remote_password;
// 	}else{
// 		clientid = mosq->id;
// 		username = mosq->username;
// 		password = mosq->password;
// 	}
// #else
	clientid = mosq->id;
	username = mosq->username;
	password = mosq->password;
// #endif

	if(mosq->protocol == mosq_p_mqtt5){
		/* Generate properties from options */
		if(!mosquitto_property_read_int16(properties, MQTT_PROP_RECEIVE_MAXIMUM, &receive_maximum, false)){
			rc = mosquitto_property_add_int16(&local_props, MQTT_PROP_RECEIVE_MAXIMUM, mosq->msgs_in.inflight_maximum);
			if(rc) return rc;
		}else{
			mosq->msgs_in.inflight_maximum = receive_maximum;
			mosq->msgs_in.inflight_quota = receive_maximum;
		}

		version = MQTT_PROTOCOL_V5;
		headerlen = 10;
		proplen = 0;
		proplen += property__get_length_all(properties);
		proplen += property__get_length_all(local_props);
		varbytes = packet__varint_bytes(proplen);
		headerlen += proplen + varbytes;
	}else if(mosq->protocol == mosq_p_mqtt311){
		version = MQTT_PROTOCOL_V311;
		headerlen = 10;
	}else if(mosq->protocol == mosq_p_mqtt31){
		version = MQTT_PROTOCOL_V31;
		headerlen = 12;
	}else{
		return MOSQ_ERR_INVAL;
	}

	packet = (mosquitto__packet*)calloc(1, sizeof(struct mosquitto__packet));
	if(!packet) return MOSQ_ERR_NOMEM;

	if(clientid){
		payloadlen = 2+strlen(clientid);
	}else{
		payloadlen = 2;
	}
	if(mosq->will){
		will = 1;
		assert(mosq->will->msg.topic);

		payloadlen += 2+strlen(mosq->will->msg.topic) + 2+mosq->will->msg.payloadlen;
		if(mosq->protocol == mosq_p_mqtt5){
			will_proplen = property__get_length_all(mosq->will->properties);
			varbytes = packet__varint_bytes(will_proplen);
			payloadlen += will_proplen + varbytes;
		}
	}

	/* After this check we can be sure that the username and password are
	 * always valid for the current protocol, so there is no need to check
	 * username before checking password. */
	if(mosq->protocol == mosq_p_mqtt31 || mosq->protocol == mosq_p_mqtt311){
		if(password != NULL && username == NULL){
			return MOSQ_ERR_INVAL;
		}
	}

	if(username){
		payloadlen += 2+strlen(username);
	}
	if(password){
		payloadlen += 2+strlen(password);
	}

	packet->command = CMD_CONNECT;
	packet->remaining_length = headerlen + payloadlen;
	rc = packet__alloc(packet);
	if(rc){
		free(packet);
		return rc;
	}

	/* Variable header */
	if(version == MQTT_PROTOCOL_V31){
		packet__write_string(packet, PROTOCOL_NAME_v31, strlen(PROTOCOL_NAME_v31));
	}else{
		packet__write_string(packet, PROTOCOL_NAME, strlen(PROTOCOL_NAME));
	}
// #if defined(WITH_BROKER) && defined(WITH_BRIDGE)
// 	if(mosq->bridge && mosq->bridge->try_private && mosq->bridge->try_private_accepted){
// 		version |= 0x80;
// 	}else{
// 	}
// #endif
	packet__write_byte(packet, version);
	byte = (clean_session&0x1)<<1;
	if(will){
		byte = byte | ((mosq->will->msg.retain&0x1)<<5) | ((mosq->will->msg.qos&0x3)<<3) | ((will&0x1)<<2);
	}
	if(username){
		byte = byte | 0x1<<7;
	}
	if(mosq->password){
		byte = byte | 0x1<<6;
	}
	packet__write_byte(packet, byte);
	packet__write_uint16(packet, keepalive);

	if(mosq->protocol == mosq_p_mqtt5){
		/* Write properties */
		packet__write_varint(packet, proplen);
		property__write_all(packet, properties, false);
		property__write_all(packet, local_props, false);
	}
	mosquitto_property_free_all(&local_props);

	/* Payload */
	if(clientid){
		packet__write_string(packet, clientid, strlen(clientid));
	}else{
		packet__write_uint16(packet, 0);
	}
	if(will){
		if(mosq->protocol == mosq_p_mqtt5){
			/* Write will properties */
			property__write_all(packet, mosq->will->properties, true);
		}
		packet__write_string(packet, mosq->will->msg.topic, strlen(mosq->will->msg.topic));
		packet__write_string(packet, (const char *)mosq->will->msg.payload, mosq->will->msg.payloadlen);
	}

	if(username){
		packet__write_string(packet, username, strlen(username));
	}
	if(password){
		packet__write_string(packet, password, strlen(password));
	}

	mosq->keepalive = keepalive;
// #ifdef WITH_BROKER
// # ifdef WITH_BRIDGE
// 	log__printf(mosq, MOSQ_LOG_DEBUG, "Bridge %s sending CONNECT", clientid);
// # endif
// #else
	log__printf(mosq, MOSQ_LOG_DEBUG, "Client %s sending CONNECT", clientid);
// #endif
	return packet__queue(mosq, packet);
}