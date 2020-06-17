#include "pub.h"




///////////////////////////////handle_ping.c///////////////////////////////////


int handle__pingreq(struct mosquitto *mosq)
{                       
        int state;
                        
        assert(mosq);
                        
        state = mosquitto__get_state(mosq);
        if(state != mosq_cs_active){
                return MOSQ_ERR_PROTOCOL;
        }               
        
// #ifdef WITH_BROKER
//         log__printf(NULL, MOSQ_LOG_DEBUG, "Received PINGREQ from %s", mosq->id);
// #else
        log__printf(mosq, MOSQ_LOG_DEBUG, "Client %s received PINGREQ", mosq->id);
// #endif
        return send__pingresp(mosq);
}

int handle__pingresp(struct mosquitto *mosq)
{
        int state;

        assert(mosq);

        state = mosquitto__get_state(mosq);
        if(state != mosq_cs_active){
                return MOSQ_ERR_PROTOCOL;
        }

        mosq->ping_t = 0; /* No longer waiting for a PINGRESP. */
// #ifdef WITH_BROKER
//         log__printf(NULL, MOSQ_LOG_DEBUG, "Received PINGRESP from %s", mosq->id);
// #else
        log__printf(mosq, MOSQ_LOG_DEBUG, "Client %s received PINGRESP", mosq->id);
// #endif
        return MOSQ_ERR_SUCCESS;
}


// ///////////////////////////////handle_pubackcomp.c///////////////////////////////////


int handle__pubackcomp(struct mosquitto *mosq, const char *type)
{
	uint8_t reason_code = 0;
	uint16_t mid;
	int rc;
	mosquitto_property *properties = NULL;
	int qos;
	int state;

	assert(mosq);

	state = mosquitto__get_state(mosq);
	if(state != mosq_cs_active){
		return MOSQ_ERR_PROTOCOL;
	}

	pthread_mutex_lock(&mosq->msgs_out.mutex);
	util__increment_send_quota(mosq);
	pthread_mutex_unlock(&mosq->msgs_out.mutex);

	rc = packet__read_uint16(&mosq->in_packet, &mid);
	if(rc) return rc;
	qos = type[3] == 'A'?1:2; /* pubAck or pubComp */
	if(mid == 0) return MOSQ_ERR_PROTOCOL;

	if(mosq->protocol == mosq_p_mqtt5 && mosq->in_packet.remaining_length > 2){
		rc = packet__read_byte(&mosq->in_packet, &reason_code);
		if(rc) return rc;

		if(mosq->in_packet.remaining_length > 3){
			rc = property__read_all(CMD_PUBACK, &mosq->in_packet, &properties);
			if(rc) return rc;
		}
	}

// #ifdef WITH_BROKER
// 	log__printf(NULL, MOSQ_LOG_DEBUG, "Received %s from %s (Mid: %d, RC:%d)", type, mosq->id, mid, reason_code);

// 	/* Immediately free, we don't do anything with Reason String or User Property at the moment */
// 	mosquitto_property_free_all(&properties);

// 	rc = db__message_delete_outgoing(db, mosq, mid, mosq_ms_wait_for_pubcomp, qos);
// 	if(rc == MOSQ_ERR_NOT_FOUND){
// 		log__printf(mosq, MOSQ_LOG_WARNING, "Warning: Received %s from %s for an unknown packet identifier %d.", type, mosq->id, mid);
// 		return MOSQ_ERR_SUCCESS;
// 	}else{
// 		return rc;
// 	}
// #else
	log__printf(mosq, MOSQ_LOG_DEBUG, "Client %s received %s (Mid: %d, RC:%d)", mosq->id, type, mid, reason_code);

	rc = message__delete(mosq, mid, mosq_md_out, qos);
	if(rc){
		return rc;
	}else{
		/* Only inform the client the message has been sent once. */
		pthread_mutex_lock(&mosq->callback_mutex);
		if(mosq->on_publish){
			mosq->in_callback = true;
			mosq->on_publish(mosq, mosq->userdata, mid);
			mosq->in_callback = false;
		}
		if(mosq->on_publish_v5){
			mosq->in_callback = true;
			mosq->on_publish_v5(mosq, mosq->userdata, mid, reason_code, properties);
			mosq->in_callback = false;
		}
		pthread_mutex_unlock(&mosq->callback_mutex);
		mosquitto_property_free_all(&properties);
	}
	pthread_mutex_lock(&mosq->msgs_out.mutex);
	message__release_to_inflight(mosq, mosq_md_out);
	pthread_mutex_unlock(&mosq->msgs_out.mutex);

	return MOSQ_ERR_SUCCESS;
// #endif
}


///////////////////////////////handle_publish.c///////////////////////////////////


int handle__publish(struct mosquitto *mosq)
{
	uint8_t header;
	struct mosquitto_message_all *message;
	int rc = 0;
	uint16_t mid;
	int slen;
	mosquitto_property *properties = NULL;
	int state;

	assert(mosq);

	state = mosquitto__get_state(mosq);
	if(state != mosq_cs_active){
		return MOSQ_ERR_PROTOCOL;
	}

	message = (mosquitto_message_all*)calloc(1, sizeof(struct mosquitto_message_all));
	if(!message) return MOSQ_ERR_NOMEM;

	header = mosq->in_packet.command;

	message->dup = (header & 0x08)>>3;
	message->msg.qos = (header & 0x06)>>1;
	message->msg.retain = (header & 0x01);

	rc = packet__read_string(&mosq->in_packet, &message->msg.topic, &slen);
	if(rc){
		message__cleanup(&message);
		return rc;
	}
	if(!slen){
		message__cleanup(&message);
		return MOSQ_ERR_PROTOCOL;
	}

	if(message->msg.qos > 0){
		if(mosq->protocol == mosq_p_mqtt5){
			if(mosq->msgs_in.inflight_quota == 0){
				message__cleanup(&message);
				/* FIXME - should send a DISCONNECT here */
				return MOSQ_ERR_PROTOCOL;
			}
		}

		rc = packet__read_uint16(&mosq->in_packet, &mid);
		if(rc){
			message__cleanup(&message);
			return rc;
		}
		if(mid == 0){
			message__cleanup(&message);
			return MOSQ_ERR_PROTOCOL;
		}
		message->msg.mid = (int)mid;
	}

	if(mosq->protocol == mosq_p_mqtt5){
		rc = property__read_all(CMD_PUBLISH, &mosq->in_packet, &properties);
		if(rc) return rc;
	}

	message->msg.payloadlen = mosq->in_packet.remaining_length - mosq->in_packet.pos;
	if(message->msg.payloadlen){
		message->msg.payload = (uint8_t*)calloc(message->msg.payloadlen+1, sizeof(uint8_t));
		if(!message->msg.payload){
			message__cleanup(&message);
			mosquitto_property_free_all(&properties);
			return MOSQ_ERR_NOMEM;
		}
		rc = packet__read_bytes(&mosq->in_packet, message->msg.payload, message->msg.payloadlen);
		if(rc){
			message__cleanup(&message);
			mosquitto_property_free_all(&properties);
			return rc;
		}
	}
	log__printf(mosq, MOSQ_LOG_DEBUG,
			"Client %s received PUBLISH (d%d, q%d, r%d, m%d, '%s', ... (%ld bytes))",
			mosq->id, message->dup, message->msg.qos, message->msg.retain,
			message->msg.mid, message->msg.topic,
			(long)message->msg.payloadlen);

	message->timestamp = mosquitto_time();
	switch(message->msg.qos){
		case 0:
			pthread_mutex_lock(&mosq->callback_mutex);
			if(mosq->on_message){
				mosq->in_callback = true;
				mosq->on_message(mosq, mosq->userdata, &message->msg);
				mosq->in_callback = false;
			}
			if(mosq->on_message_v5){
				mosq->in_callback = true;
				mosq->on_message_v5(mosq, mosq->userdata, &message->msg, properties);
				mosq->in_callback = false;
			}
			pthread_mutex_unlock(&mosq->callback_mutex);
			message__cleanup(&message);
			mosquitto_property_free_all(&properties);
			return MOSQ_ERR_SUCCESS;
		case 1:
			util__decrement_receive_quota(mosq);
			rc = send__puback(mosq, message->msg.mid, 0);
			pthread_mutex_lock(&mosq->callback_mutex);
			if(mosq->on_message){
				mosq->in_callback = true;
				mosq->on_message(mosq, mosq->userdata, &message->msg);
				mosq->in_callback = false;
			}
			if(mosq->on_message_v5){
				mosq->in_callback = true;
				mosq->on_message_v5(mosq, mosq->userdata, &message->msg, properties);
				mosq->in_callback = false;
			}
			pthread_mutex_unlock(&mosq->callback_mutex);
			message__cleanup(&message);
			mosquitto_property_free_all(&properties);
			return rc;
		case 2:
			message->properties = properties;
			util__decrement_receive_quota(mosq);
			rc = send__pubrec(mosq, message->msg.mid, 0);
			pthread_mutex_lock(&mosq->msgs_in.mutex);
			message->state = mosq_ms_wait_for_pubrel;
			message__queue(mosq, message, mosq_md_in);
			pthread_mutex_unlock(&mosq->msgs_in.mutex);
			return rc;
		default:
			message__cleanup(&message);
			mosquitto_property_free_all(&properties);
			return MOSQ_ERR_PROTOCOL;
	}
}


///////////////////////////////handle_pubrec.c///////////////////////////////////


int handle__pubrec(struct mosquitto_db *db, struct mosquitto *mosq)
{
	uint8_t reason_code = 0;
	uint16_t mid;
	int rc;
	mosquitto_property *properties = NULL;
	int state;

	assert(mosq);

	state = mosquitto__get_state(mosq);
	if(state != mosq_cs_active){
		return MOSQ_ERR_PROTOCOL;
	}

	rc = packet__read_uint16(&mosq->in_packet, &mid);
	if(rc) return rc;
	if(mid == 0) return MOSQ_ERR_PROTOCOL;

	if(mosq->protocol == mosq_p_mqtt5 && mosq->in_packet.remaining_length > 2){
		rc = packet__read_byte(&mosq->in_packet, &reason_code);
		if(rc) return rc;

		if(mosq->in_packet.remaining_length > 3){
			rc = property__read_all(CMD_PUBREC, &mosq->in_packet, &properties);
			if(rc) return rc;
			/* Immediately free, we don't do anything with Reason String or User Property at the moment */
			mosquitto_property_free_all(&properties);
		}
	}

// #ifdef WITH_BROKER
// 	log__printf(NULL, MOSQ_LOG_DEBUG, "Received PUBREC from %s (Mid: %d)", mosq->id, mid);

// 	if(reason_code < 0x80){
// 		rc = db__message_update_outgoing(mosq, mid, mosq_ms_wait_for_pubcomp, 2);
// 	}else{
// 		return db__message_delete_outgoing(db, mosq, mid, mosq_ms_wait_for_pubrec, 2);
// 	}
// #else
	UNUSED(db);

	log__printf(mosq, MOSQ_LOG_DEBUG, "Client %s received PUBREC (Mid: %d)", mosq->id, mid);

	if(reason_code < 0x80 || mosq->protocol != mosq_p_mqtt5){
		rc = message__out_update(mosq, mid, mosq_ms_wait_for_pubcomp, 2);
	}else{
		if(!message__delete(mosq, mid, mosq_md_out, 2)){
			/* Only inform the client the message has been sent once. */
			pthread_mutex_lock(&mosq->callback_mutex);
			if(mosq->on_publish_v5){
				mosq->in_callback = true;
				mosq->on_publish_v5(mosq, mosq->userdata, mid, reason_code, properties);
				mosq->in_callback = false;
			}
			pthread_mutex_unlock(&mosq->callback_mutex);
		}
		util__increment_send_quota(mosq);
		pthread_mutex_lock(&mosq->msgs_out.mutex);
		message__release_to_inflight(mosq, mosq_md_out);
		pthread_mutex_unlock(&mosq->msgs_out.mutex);
		return MOSQ_ERR_SUCCESS;
	}
// #endif
	if(rc == MOSQ_ERR_NOT_FOUND){
		log__printf(mosq, MOSQ_LOG_WARNING, "Warning: Received PUBREC from %s for an unknown packet identifier %d.", mosq->id, mid);
	}else if(rc != MOSQ_ERR_SUCCESS){
		return rc;
	}
	rc = send__pubrel(mosq, mid);
	if(rc) return rc;

	return MOSQ_ERR_SUCCESS;
}


///////////////////////////////handle_pubrel.c///////////////////////////////////


int handle__pubrel(struct mosquitto_db *db, struct mosquitto *mosq)
{
	uint8_t reason_code;
	uint16_t mid;
// #ifndef WITH_BROKER
	struct mosquitto_message_all *message = NULL;
// #endif
	int rc;
	mosquitto_property *properties = NULL;
	int state;

	assert(mosq);

	state = mosquitto__get_state(mosq);
	if(state != mosq_cs_active){
		return MOSQ_ERR_PROTOCOL;
	}

	if(mosq->protocol != mosq_p_mqtt31){
		if((mosq->in_packet.command&0x0F) != 0x02){
			return MOSQ_ERR_PROTOCOL;
		}
	}
	rc = packet__read_uint16(&mosq->in_packet, &mid);
	if(rc) return rc;
	if(mid == 0) return MOSQ_ERR_PROTOCOL;

	if(mosq->protocol == mosq_p_mqtt5 && mosq->in_packet.remaining_length > 2){
		rc = packet__read_byte(&mosq->in_packet, &reason_code);
		if(rc) return rc;

		if(mosq->in_packet.remaining_length > 3){
			rc = property__read_all(CMD_PUBREL, &mosq->in_packet, &properties);
			if(rc) return rc;
		}
	}

// #ifdef WITH_BROKER
// 	log__printf(NULL, MOSQ_LOG_DEBUG, "Received PUBREL from %s (Mid: %d)", mosq->id, mid);

// 	/* Immediately free, we don't do anything with Reason String or User Property at the moment */
// 	mosquitto_property_free_all(&properties);

// 	rc = db__message_release_incoming(db, mosq, mid);
// 	if(rc == MOSQ_ERR_NOT_FOUND){
// 		/* Message not found. Still send a PUBCOMP anyway because this could be
// 		 * due to a repeated PUBREL after a client has reconnected. */
// 	}else if(rc != MOSQ_ERR_SUCCESS){
// 		return rc;
// 	}

// 	rc = send__pubcomp(mosq, mid);
// 	if(rc) return rc;
// #else
	UNUSED(db);

	log__printf(mosq, MOSQ_LOG_DEBUG, "Client %s received PUBREL (Mid: %d)", mosq->id, mid);

	rc = send__pubcomp(mosq, mid);
	if(rc){
		message__remove(mosq, mid, mosq_md_in, &message, 2);
		return rc;
	}

	rc = message__remove(mosq, mid, mosq_md_in, &message, 2);
	if(rc){
		return rc;
	}else{
		/* Only pass the message on if we have removed it from the queue - this
		 * prevents multiple callbacks for the same message. */
		pthread_mutex_lock(&mosq->callback_mutex);
		if(mosq->on_message){
			mosq->in_callback = true;
			mosq->on_message(mosq, mosq->userdata, &message->msg);
			mosq->in_callback = false;
		}
		if(mosq->on_message_v5){
			mosq->in_callback = true;
			mosq->on_message_v5(mosq, mosq->userdata, &message->msg, message->properties);
			mosq->in_callback = false;
		}
		pthread_mutex_unlock(&mosq->callback_mutex);
		mosquitto_property_free_all(&properties);
		message__cleanup(&message);
	}
// #endif

	return MOSQ_ERR_SUCCESS;
}


///////////////////////////////handle_connack.c///////////////////////////////////


static void connack_callback(struct mosquitto *mosq, uint8_t reason_code, uint8_t connect_flags, const mosquitto_property *properties)
{
	log__printf(mosq, MOSQ_LOG_DEBUG, "Client %s received CONNACK (%d)", mosq->id, reason_code);
	if(reason_code == MQTT_RC_SUCCESS){
		mosq->reconnects = 0;
	}
	pthread_mutex_lock(&mosq->callback_mutex);
	if(mosq->on_connect){
		mosq->in_callback = true;
		mosq->on_connect(mosq, mosq->userdata, reason_code);
		mosq->in_callback = false;
	}
	if(mosq->on_connect_with_flags){
		mosq->in_callback = true;
		mosq->on_connect_with_flags(mosq, mosq->userdata, reason_code, connect_flags);
		mosq->in_callback = false;
	}
	if(mosq->on_connect_v5){
		mosq->in_callback = true;
		mosq->on_connect_v5(mosq, mosq->userdata, reason_code, connect_flags, properties);
		mosq->in_callback = false;
	}
	pthread_mutex_unlock(&mosq->callback_mutex);
}


int handle__connack(struct mosquitto *mosq)
{
	uint8_t connect_flags;
	uint8_t reason_code;
	int rc;
	mosquitto_property *properties = NULL;
	char *clientid = NULL;

	assert(mosq);
	rc = packet__read_byte(&mosq->in_packet, &connect_flags);
	if(rc) return rc;
	rc = packet__read_byte(&mosq->in_packet, &reason_code);
	if(rc) return rc;

	if(mosq->protocol == mosq_p_mqtt5){
		rc = property__read_all(CMD_CONNACK, &mosq->in_packet, &properties);

		if(rc == MOSQ_ERR_PROTOCOL && reason_code == CONNACK_REFUSED_PROTOCOL_VERSION){
			/* This could occur because we are connecting to a v3.x broker and
			 * it has replied with "unacceptable protocol version", but with a
			 * v3 CONNACK. */

			connack_callback(mosq, MQTT_RC_UNSUPPORTED_PROTOCOL_VERSION, connect_flags, NULL);
			return rc;
		}else if(rc){
			return rc;
		}
	}

	mosquitto_property_read_string(properties, MQTT_PROP_ASSIGNED_CLIENT_IDENTIFIER, &clientid, false);
	if(clientid){
		if(mosq->id){
			/* We've been sent a client identifier but already have one. This
			 * shouldn't happen. */
			free(clientid);
			mosquitto_property_free_all(&properties);
			return MOSQ_ERR_PROTOCOL;
		}else{
			mosq->id = clientid;
			clientid = NULL;
		}
	}

	mosquitto_property_read_byte(properties, MQTT_PROP_MAXIMUM_QOS, &mosq->maximum_qos, false);
	mosquitto_property_read_int16(properties, MQTT_PROP_RECEIVE_MAXIMUM, &mosq->msgs_out.inflight_maximum, false);
	mosquitto_property_read_int16(properties, MQTT_PROP_SERVER_KEEP_ALIVE, &mosq->keepalive, false);
	mosquitto_property_read_int32(properties, MQTT_PROP_MAXIMUM_PACKET_SIZE, &mosq->maximum_packet_size, false);

	mosq->msgs_out.inflight_quota = mosq->msgs_out.inflight_maximum;

	connack_callback(mosq, reason_code, connect_flags, properties);
	mosquitto_property_free_all(&properties);

	switch(reason_code){
		case 0:
			pthread_mutex_lock(&mosq->state_mutex);
			if(mosq->state != mosq_cs_disconnecting){
				mosq->state = mosq_cs_active;
			}
			pthread_mutex_unlock(&mosq->state_mutex);
			message__retry_check(mosq);
			return MOSQ_ERR_SUCCESS;
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
			return MOSQ_ERR_CONN_REFUSED;
		default:
			return MOSQ_ERR_PROTOCOL;
	}
}
