#include "pub.h"




int mosquitto__set_state(struct mosquitto *mosq, enum mosquitto_client_state state)
{               
        pthread_mutex_lock(&mosq->state_mutex);
// #ifdef WITH_BROKER
        // if(mosq->state != mosq_cs_disused)
// #endif  
        {
                mosq->state = state;
        }       
        pthread_mutex_unlock(&mosq->state_mutex);
        
        return MOSQ_ERR_SUCCESS;
}  

enum mosquitto_client_state mosquitto__get_state(struct mosquitto *mosq)
{
        enum mosquitto_client_state state;

        pthread_mutex_lock(&mosq->state_mutex);
        state = mosq->state;
        pthread_mutex_unlock(&mosq->state_mutex);

        return state;
}   

int util__random_bytes(void *bytes, int count)
{
	int rc = MOSQ_ERR_UNKNOWN;

// #ifdef WITH_TLS
// 	if(RAND_bytes(bytes, count) == 1){
// 		rc = MOSQ_ERR_SUCCESS;
// 	}
// #elif defined(HAVE_GETRANDOM)
// 	if(getrandom(bytes, count, 0) == count){
// 		rc = MOSQ_ERR_SUCCESS;
// 	}
// #elif defined(WIN32)
// 	HCRYPTPROV provider;

// 	if(!CryptAcquireContext(&provider, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)){
// 		return MOSQ_ERR_UNKNOWN;
// 	}

// 	if(CryptGenRandom(provider, count, bytes)){
// 		rc = MOSQ_ERR_SUCCESS;
// 	}

// 	CryptReleaseContext(provider, 0);
// #else
	int i;

	for(i=0; i<count; i++){
		((uint8_t *)bytes)[i] = (uint8_t )(random()&0xFF);
	}
	rc = MOSQ_ERR_SUCCESS;
// #endif
	return rc;
}

uint16_t mosquitto__mid_generate(struct mosquitto *mosq)
{                       
        /* FIXME - this would be better with atomic increment, but this is safer
         * for now for a bug fix release.
         *      
         * If this is changed to use atomic increment, callers of this function
         * will have to be aware that they may receive a 0 result, which may not be
         * used as a mid.
         */
        uint16_t mid;
        assert(mosq);
                
        pthread_mutex_lock(&mosq->mid_mutex);
        mosq->last_mid++;
        if(mosq->last_mid == 0) mosq->last_mid++;
        mid = mosq->last_mid;
        pthread_mutex_unlock(&mosq->mid_mutex);
                        
        return mid;
}  

void util__increment_receive_quota(struct mosquitto *mosq)
{
	if(mosq->msgs_in.inflight_quota < mosq->msgs_in.inflight_maximum){
		mosq->msgs_in.inflight_quota++;
	}
}

void util__increment_send_quota(struct mosquitto *mosq)
{
	if(mosq->msgs_out.inflight_quota < mosq->msgs_out.inflight_maximum){
		mosq->msgs_out.inflight_quota++;
	}
}

void util__decrement_receive_quota(struct mosquitto *mosq)
{       
        if(mosq->msgs_in.inflight_quota > 0){
                mosq->msgs_in.inflight_quota--;
        }
}       
        
void util__decrement_send_quota(struct mosquitto *mosq)
{               
        if(mosq->msgs_out.inflight_quota > 0){
                mosq->msgs_out.inflight_quota--;
        }               
} 

int mosquitto__check_keepalive(struct mosquitto *mosq)
{
	time_t next_msg_out;
	time_t last_msg_in;
	time_t now = mosquitto_time();
// #ifndef WITH_BROKER
	int rc;
// #endif
	int state;

	assert(mosq);
// #if defined(WITH_BROKER) && defined(WITH_BRIDGE)
// 	/* Check if a lazy bridge should be timed out due to idle. */
// 	if(mosq->bridge && mosq->bridge->start_type == bst_lazy
// 				&& mosq->sock != INVALID_SOCKET
// 				&& now - mosq->next_msg_out - mosq->keepalive >= mosq->bridge->idle_timeout){

// 		log__printf(NULL, MOSQ_LOG_NOTICE, "Bridge connection %s has exceeded idle timeout, disconnecting.", mosq->id);
// 		net__socket_close(db, mosq);
// 		return MOSQ_ERR_SUCCESS;
// 	}
// #endif
	pthread_mutex_lock(&mosq->msgtime_mutex);
	next_msg_out = mosq->next_msg_out;
	last_msg_in = mosq->last_msg_in;
	pthread_mutex_unlock(&mosq->msgtime_mutex);
	if(mosq->keepalive && mosq->sock != INVALID_SOCKET &&
			(now >= next_msg_out || now - last_msg_in >= mosq->keepalive)){

		state = mosquitto__get_state(mosq);
		if(state == mosq_cs_active && mosq->ping_t == 0){
			send__pingreq(mosq);
			/* Reset last msg times to give the server time to send a pingresp */
			pthread_mutex_lock(&mosq->msgtime_mutex);
			mosq->last_msg_in = now;
			mosq->next_msg_out = now + mosq->keepalive;
			pthread_mutex_unlock(&mosq->msgtime_mutex);
		}else{
// #ifdef WITH_BROKER
// 			net__socket_close(db, mosq);
// #else
			net__socket_close(mosq);
			state = mosquitto__get_state(mosq);
			if(state == mosq_cs_disconnecting){
				rc = MOSQ_ERR_SUCCESS;
			}else{
				rc = MOSQ_ERR_KEEPALIVE;
			}
			pthread_mutex_lock(&mosq->callback_mutex);
			if(mosq->on_disconnect){
				mosq->in_callback = true;
				mosq->on_disconnect(mosq, mosq->userdata, rc);
				mosq->in_callback = false;
			}
			if(mosq->on_disconnect_v5){
				mosq->in_callback = true;
				mosq->on_disconnect_v5(mosq, mosq->userdata, rc, NULL);
				mosq->in_callback = false;
			}
			pthread_mutex_unlock(&mosq->callback_mutex);

			return rc;
// #endif
		}
	}
	return MOSQ_ERR_SUCCESS;
}