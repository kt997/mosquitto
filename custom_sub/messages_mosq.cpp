#include "sub.h"



void message__cleanup_all(struct mosquitto *mosq)
{
        struct mosquitto_message_all *tail, *tmp;
        
        assert(mosq); 

        DL_FOREACH_SAFE(mosq->msgs_in.inflight, tail, tmp){
                DL_DELETE(mosq->msgs_in.inflight, tail);
                message__cleanup(&tail);
        }
        DL_FOREACH_SAFE(mosq->msgs_out.inflight, tail, tmp){
                DL_DELETE(mosq->msgs_out.inflight, tail);
                message__cleanup(&tail);
        }
}

void message__cleanup(struct mosquitto_message_all **message)
{                               
        struct mosquitto_message_all *msg;
                
        if(!message || !*message) return;
                        
        msg = *message; 
                        
        free(msg->msg.topic);
        free(msg->msg.payload);
        mosquitto_property_free_all(&msg->properties);
        free(msg);
}  

int message__queue(struct mosquitto *mosq, struct mosquitto_message_all *message, enum mosquitto_msg_direction dir)           
{                               
        /* mosq->*_message_mutex should be locked before entering this function */
        assert(mosq);   
        assert(message);
        assert(message->msg.qos != 0);
                        
        if(dir == mosq_md_out){
                DL_APPEND(mosq->msgs_out.inflight, message);
                mosq->msgs_out.queue_len++;
        }else{                  
                DL_APPEND(mosq->msgs_in.inflight, message);
                mosq->msgs_in.queue_len++;
        }               
                
        return message__release_to_inflight(mosq, dir);
} 

void message__reconnect_reset(struct mosquitto *mosq)
{               
        struct mosquitto_message_all *message, *tmp;
        assert(mosq);   
        
        pthread_mutex_lock(&mosq->msgs_in.mutex);
        mosq->msgs_in.inflight_quota = mosq->msgs_in.inflight_maximum;
        mosq->msgs_in.queue_len = 0;
        DL_FOREACH_SAFE(mosq->msgs_in.inflight, message, tmp){
                mosq->msgs_in.queue_len++;
                message->timestamp = 0;
                if(message->msg.qos != 2){
                        DL_DELETE(mosq->msgs_in.inflight, message);
                        message__cleanup(&message);
                }else{
                        /* Message state can be preserved here because it should match
                        * whatever the client has got. */
                        util__decrement_receive_quota(mosq);
                }
        }       
        pthread_mutex_unlock(&mosq->msgs_in.mutex);
        
        
        pthread_mutex_lock(&mosq->msgs_out.mutex);
        mosq->msgs_out.inflight_quota = mosq->msgs_out.inflight_maximum;
        mosq->msgs_out.queue_len = 0;
        DL_FOREACH_SAFE(mosq->msgs_out.inflight, message, tmp){
                mosq->msgs_out.queue_len++;

                message->timestamp = 0;
                if(mosq->msgs_out.inflight_quota != 0){
                        util__decrement_send_quota(mosq);
                        if(message->msg.qos == 1){
                                message->state = mosq_ms_publish_qos1;
                        }else if(message->msg.qos == 2){
                                if(message->state == mosq_ms_wait_for_pubrec){
                                        message->state = mosq_ms_publish_qos2;
                                }else if(message->state == mosq_ms_wait_for_pubcomp){
                                        message->state = mosq_ms_resend_pubrel;
                                }
                                /* Should be able to preserve state. */
                        }
                }else{
                        message->state = mosq_ms_invalid;
                }
        }
        pthread_mutex_unlock(&mosq->msgs_out.mutex);
}

int message__release_to_inflight(struct mosquitto *mosq, enum mosquitto_msg_direction dir)
{
	/* mosq->*_message_mutex should be locked before entering this function */
	struct mosquitto_message_all *cur, *tmp;
	int rc = MOSQ_ERR_SUCCESS;

	if(dir == mosq_md_out){
		DL_FOREACH_SAFE(mosq->msgs_out.inflight, cur, tmp){
			if(mosq->msgs_out.inflight_quota > 0){
				if(cur->msg.qos > 0 && cur->state == mosq_ms_invalid){
					if(cur->msg.qos == 1){
						cur->state = mosq_ms_wait_for_puback;
					}else if(cur->msg.qos == 2){
						cur->state = mosq_ms_wait_for_pubrec;
					}
					rc = send__publish(mosq, cur->msg.mid, cur->msg.topic, cur->msg.payloadlen, cur->msg.payload, cur->msg.qos, cur->msg.retain, cur->dup, cur->properties, NULL, 0);
					if(rc){
						return rc;
					}
					util__decrement_send_quota(mosq);
				}
			}else{
				return MOSQ_ERR_SUCCESS;
			}
		}
	}

	return rc;
}

int message__remove(struct mosquitto *mosq, uint16_t mid, enum mosquitto_msg_direction dir, struct mosquitto_message_all **message, int qos)
{
	struct mosquitto_message_all *cur, *tmp;
	bool found = false;
	assert(mosq);
	assert(message);

	if(dir == mosq_md_out){
		pthread_mutex_lock(&mosq->msgs_out.mutex);

		DL_FOREACH_SAFE(mosq->msgs_out.inflight, cur, tmp){
			if(found == false && cur->msg.mid == mid){
				if(cur->msg.qos != qos){
					pthread_mutex_unlock(&mosq->msgs_out.mutex);
					return MOSQ_ERR_PROTOCOL;
				}
				DL_DELETE(mosq->msgs_out.inflight, cur);

				*message = cur;
				mosq->msgs_out.queue_len--;
				found = true;
				break;
			}
		}
		pthread_mutex_unlock(&mosq->msgs_out.mutex);
		if(found){
			return MOSQ_ERR_SUCCESS;
		}else{
			return MOSQ_ERR_NOT_FOUND;
		}
	}else{
		pthread_mutex_lock(&mosq->msgs_in.mutex);
		DL_FOREACH_SAFE(mosq->msgs_in.inflight, cur, tmp){
			if(cur->msg.mid == mid){
				if(cur->msg.qos != qos){
					pthread_mutex_unlock(&mosq->msgs_in.mutex);
					return MOSQ_ERR_PROTOCOL;
				}
				DL_DELETE(mosq->msgs_in.inflight, cur);
				*message = cur;
				mosq->msgs_in.queue_len--;
				found = true;
				break;
			}
		}

		pthread_mutex_unlock(&mosq->msgs_in.mutex);
		if(found){
			return MOSQ_ERR_SUCCESS;
		}else{
			return MOSQ_ERR_NOT_FOUND;
		}
	}
}

// int message__delete(struct mosquitto *mosq, uint16_t mid, enum mosquitto_msg_direction dir, int qos)
// {
// 	struct mosquitto_message_all *message;
// 	int rc;
// 	assert(mosq);

// 	rc = message__remove(mosq, mid, dir, &message, qos);
// 	if(rc == MOSQ_ERR_SUCCESS){
// 		message__cleanup(&message);
// 	}
// 	return rc;
// }

// int message__out_update(struct mosquitto *mosq, uint16_t mid, enum mosquitto_msg_state state, int qos)
// {
// 	struct mosquitto_message_all *message, *tmp;
// 	assert(mosq);

// 	pthread_mutex_lock(&mosq->msgs_out.mutex);
// 	DL_FOREACH_SAFE(mosq->msgs_out.inflight, message, tmp){
// 		if(message->msg.mid == mid){
// 			if(message->msg.qos != qos){
// 				pthread_mutex_unlock(&mosq->msgs_out.mutex);
// 				return MOSQ_ERR_PROTOCOL;
// 			}
// 			message->state = state;
// 			message->timestamp = mosquitto_time();
// 			pthread_mutex_unlock(&mosq->msgs_out.mutex);
// 			return MOSQ_ERR_SUCCESS;
// 		}
// 	}
// 	pthread_mutex_unlock(&mosq->msgs_out.mutex);
// 	return MOSQ_ERR_NOT_FOUND;
// }

void message__retry_check(struct mosquitto *mosq)
{
	struct mosquitto_message_all *msg;
	time_t now = mosquitto_time();
	assert(mosq);

// #ifdef WITH_THREADING
	pthread_mutex_lock(&mosq->msgs_out.mutex);
// #endif

	DL_FOREACH(mosq->msgs_out.inflight, msg){
		switch(msg->state){
			case mosq_ms_publish_qos1:
			case mosq_ms_publish_qos2:
				msg->timestamp = now;
				msg->dup = true;
				send__publish(mosq, msg->msg.mid, msg->msg.topic, msg->msg.payloadlen, msg->msg.payload, msg->msg.qos, msg->msg.retain, msg->dup, msg->properties, NULL, 0);
				break;
			case mosq_ms_wait_for_pubrel:
				msg->timestamp = now;
				msg->dup = true;
				send__pubrec(mosq, msg->msg.mid, 0);
				break;
			case mosq_ms_resend_pubrel:
			case mosq_ms_wait_for_pubcomp:
				msg->timestamp = now;
				msg->dup = true;
				send__pubrel(mosq, msg->msg.mid);
				break;
			default:
				break;
		}
	}
// #ifdef WITH_THREADING
	pthread_mutex_unlock(&mosq->msgs_out.mutex);
// #endif
}