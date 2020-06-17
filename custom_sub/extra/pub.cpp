#include "pub.h"
#include "pub_internals.h"


uint64_t g_bytes_received = 0;
uint64_t g_bytes_sent = 0;
uint64_t g_pub_bytes_received = 0;
uint64_t g_pub_bytes_sent = 0;
unsigned long g_msgs_received = 0;
unsigned long g_msgs_sent = 0;
unsigned long g_pub_msgs_received = 0;
unsigned long g_pub_msgs_sent = 0;
unsigned long g_msgs_dropped = 0;
int g_clients_expired = 0;
unsigned int g_socket_connections = 0;
unsigned int g_connection_count = 0;

// int add(int a, int b)
// {
//     return a+b;
// }

// void server()
// {

//     struct sockaddr_in address;
//     int addrlen=sizeof(address);
//     char buffer[LEN];
//     char *hello="Hello from the other side";

//     int sock_desc=socket(AF_INET,SOCK_STREAM,0);

//     address.sin_family=AF_INET;
//     address.sin_addr.s_addr=INADDR_ANY;
//     address.sin_port=htons(1883);

//     if(bind(sock_desc, (struct sockaddr *)&address, sizeof(address))<0)
//         printf("bind Failed.\n");

//     listen(sock_desc, 4);
//         // printf("bind Failed.\n");

//     int sock1=accept(sock_desc, (struct sockaddr *)&address,(socklen_t *)&addrlen);


//     send(sock1, hello, strlen(hello),0);
// }


///////////////////////////////mosquitto.c///////////////////////////////////

void mosquitto_destroy(struct mosquitto *mosq)
{       
        if(!mosq) return;
        
        mosquitto__destroy(mosq);
        free(mosq);
}

void mosquitto__destroy(struct mosquitto *mosq)
{
	struct mosquitto__packet *packet;
	if(!mosq) return;

// #ifdef WITH_THREADING
// #  ifdef HAVE_PTHREAD_CANCEL
	if(mosq->threaded == mosq_ts_self && !pthread_equal(mosq->thread_id, pthread_self())){
		pthread_cancel(mosq->thread_id);
		pthread_join(mosq->thread_id, NULL);
		mosq->threaded = mosq_ts_none;
	}
// #  endif

	if(mosq->id){
		/* If mosq->id is not NULL then the client has already been initialised
		 * and so the mutexes need destroying. If mosq->id is NULL, the mutexes
		 * haven't been initialised. */
		pthread_mutex_destroy(&mosq->callback_mutex);
		pthread_mutex_destroy(&mosq->log_callback_mutex);
		pthread_mutex_destroy(&mosq->state_mutex);
		pthread_mutex_destroy(&mosq->out_packet_mutex);
		pthread_mutex_destroy(&mosq->current_out_packet_mutex);
		pthread_mutex_destroy(&mosq->msgtime_mutex);
		pthread_mutex_destroy(&mosq->msgs_in.mutex);
		pthread_mutex_destroy(&mosq->msgs_out.mutex);
		pthread_mutex_destroy(&mosq->mid_mutex);
	}
// #endif
	if(mosq->sock != INVALID_SOCKET){
		net__socket_close(mosq);
	}
	message__cleanup_all(mosq);
	// will__clear(mosq);
// #ifdef WITH_TLS
// 	if(mosq->ssl){
// 		SSL_free(mosq->ssl);
// 	}
// 	if(mosq->ssl_ctx){
// 		SSL_CTX_free(mosq->ssl_ctx);
// 	}
// 	free(mosq->tls_cafile);
// 	free(mosq->tls_capath);
// 	free(mosq->tls_certfile);
// 	free(mosq->tls_keyfile);
// 	if(mosq->tls_pw_callback) mosq->tls_pw_callback = NULL;
// 	free(mosq->tls_version);
// 	free(mosq->tls_ciphers);
// 	free(mosq->tls_psk);
// 	free(mosq->tls_psk_identity);
// 	free(mosq->tls_alpn);
// #endif

	free(mosq->address);
	mosq->address = NULL;

	free(mosq->id);
	mosq->id = NULL;

	free(mosq->username);
	mosq->username = NULL;

	free(mosq->password);
	mosq->password = NULL;

	free(mosq->host);
	mosq->host = NULL;

	free(mosq->bind_address);
	mosq->bind_address = NULL;

	/* Out packet cleanup */
	if(mosq->out_packet && !mosq->current_out_packet){
		mosq->current_out_packet = mosq->out_packet;
		mosq->out_packet = mosq->out_packet->next;
	}
	while(mosq->current_out_packet){
		packet = mosq->current_out_packet;
		/* Free data and reset values */
		mosq->current_out_packet = mosq->out_packet;
		if(mosq->out_packet){
			mosq->out_packet = mosq->out_packet->next;
		}

		packet__cleanup(packet);
		free(packet);
	}

	packet__cleanup(&mosq->in_packet);
	if(mosq->sockpairR != INVALID_SOCKET){
		close(mosq->sockpairR);
		mosq->sockpairR = INVALID_SOCKET;
	}
	if(mosq->sockpairW != INVALID_SOCKET){
		close(mosq->sockpairW);
		mosq->sockpairW = INVALID_SOCKET;
	}
}

struct mosquitto *mosquitto_new(const char *id, bool clean_start, void *userdata)
{
	struct mosquitto *mosq = NULL;
	int rc;

	if(clean_start == false && id == NULL){
		errno = EINVAL;
		return NULL;
	}

// #ifndef WIN32
// 	signal(SIGPIPE, SIG_IGN);
// #endif

	mosq = (struct mosquitto *)calloc(1, sizeof(struct mosquitto));
	if(mosq){
		mosq->sock = INVALID_SOCKET;
		mosq->sockpairR = INVALID_SOCKET;
		mosq->sockpairW = INVALID_SOCKET;

		rc = mosquitto_reinitialise(mosq, id, clean_start, userdata);
		if(rc){
			mosquitto_destroy(mosq);
			if(rc == MOSQ_ERR_INVAL){
				errno = EINVAL;
			}else if(rc == MOSQ_ERR_NOMEM){
				errno = ENOMEM;
			}
			return NULL;
		}
	}else{
		errno = ENOMEM;
	}
	return mosq;
}

int mosquitto_reinitialise(struct mosquitto *mosq, const char *id, bool clean_start, void *userdata)
{
	if(!mosq) return MOSQ_ERR_INVAL;

	if(clean_start == false && id == NULL){
		return MOSQ_ERR_INVAL;
	}

	mosquitto__destroy(mosq);
	memset(mosq, 0, sizeof(struct mosquitto));

	if(userdata){
		mosq->userdata = userdata;
	}else{
		mosq->userdata = mosq;
	}
	mosq->protocol = mosq_p_mqtt311;
	mosq->sock = INVALID_SOCKET;
	mosq->sockpairR = INVALID_SOCKET;
	mosq->sockpairW = INVALID_SOCKET;
	mosq->keepalive = 60;
	mosq->clean_start = clean_start;
	if(id){
		if(STREMPTY(id)){
			return MOSQ_ERR_INVAL;
		}
		if(mosquitto_validate_utf8(id, strlen(id))){
			return MOSQ_ERR_MALFORMED_UTF8;
		}
		mosq->id = strdup(id);
	}
	mosq->in_packet.payload = NULL;
	packet__cleanup(&mosq->in_packet);
	mosq->out_packet = NULL;
	mosq->current_out_packet = NULL;
	mosq->last_msg_in = mosquitto_time();
	mosq->next_msg_out = mosquitto_time() + mosq->keepalive;
	mosq->ping_t = 0;
	mosq->last_mid = 0;
	mosq->state = mosq_cs_new;
	mosq->maximum_qos = 2;
	mosq->msgs_in.inflight_maximum = 20;
	mosq->msgs_out.inflight_maximum = 20;
	mosq->msgs_in.inflight_quota = 20;
	mosq->msgs_out.inflight_quota = 20;
	mosq->will = NULL;
	mosq->on_connect = NULL;
	mosq->on_publish = NULL;
	mosq->on_message = NULL;
	mosq->on_subscribe = NULL;
	mosq->on_unsubscribe = NULL;
	mosq->host = NULL;
	mosq->port = 1883;
	mosq->in_callback = false;
	mosq->reconnect_delay = 1;
	mosq->reconnect_delay_max = 1;
	mosq->reconnect_exponential_backoff = false;
	mosq->threaded = mosq_ts_none;
// #ifdef WITH_TLS
// 	mosq->ssl = NULL;
// 	mosq->ssl_ctx = NULL;
// 	mosq->tls_cert_reqs = SSL_VERIFY_PEER;
// 	mosq->tls_insecure = false;
// 	mosq->want_write = false;
// 	mosq->tls_ocsp_required = false;
// // #endif
// // #ifdef WITH_THREADING
	pthread_mutex_init(&mosq->callback_mutex, NULL);
	pthread_mutex_init(&mosq->log_callback_mutex, NULL);
	pthread_mutex_init(&mosq->state_mutex, NULL);
	pthread_mutex_init(&mosq->out_packet_mutex, NULL);
	pthread_mutex_init(&mosq->current_out_packet_mutex, NULL);
	pthread_mutex_init(&mosq->msgtime_mutex, NULL);
	pthread_mutex_init(&mosq->msgs_in.mutex, NULL);
	pthread_mutex_init(&mosq->msgs_out.mutex, NULL);
	pthread_mutex_init(&mosq->mid_mutex, NULL);
	mosq->thread_id = pthread_self();
// #endif

	return MOSQ_ERR_SUCCESS;
}


///////////////////////////////connect.c///////////////////////////////////

void do_client_disconnect(struct mosquitto *mosq, int reason_code, const mosquitto_property *properties)              
{                               
        mosquitto__set_state(mosq, mosq_cs_disconnected);
        net__socket_close(mosq);

        /* Free data and reset values */
        pthread_mutex_lock(&mosq->out_packet_mutex);
        mosq->current_out_packet = mosq->out_packet;
        if(mosq->out_packet){
                mosq->out_packet = mosq->out_packet->next;
                if(!mosq->out_packet){
                        mosq->out_packet_last = NULL;
                }       
        }
        pthread_mutex_unlock(&mosq->out_packet_mutex);

        pthread_mutex_lock(&mosq->msgtime_mutex);
        mosq->next_msg_out = mosquitto_time() + mosq->keepalive;
        pthread_mutex_unlock(&mosq->msgtime_mutex);
                
        pthread_mutex_lock(&mosq->callback_mutex);
        if(mosq->on_disconnect){
                mosq->in_callback = true;
                mosq->on_disconnect(mosq, mosq->userdata, reason_code);
                mosq->in_callback = false;
        }       
        if(mosq->on_disconnect_v5){
                mosq->in_callback = true;
                mosq->on_disconnect_v5(mosq, mosq->userdata, reason_code, properties);
                mosq->in_callback = false;
        }
        pthread_mutex_unlock(&mosq->callback_mutex);
        pthread_mutex_unlock(&mosq->current_out_packet_mutex);
}

static int mosquitto__connect_init(struct mosquitto *mosq, const char *host, int port, int keepalive, const char *bind_address)
{
	int i;
	int rc;

	if(!mosq) return MOSQ_ERR_INVAL;
	if(!host || port <= 0) return MOSQ_ERR_INVAL;
	if(keepalive < 5) return MOSQ_ERR_INVAL;

	if(mosq->id == NULL && (mosq->protocol == mosq_p_mqtt31 || mosq->protocol == mosq_p_mqtt311)){
		mosq->id = (char *)calloc(24, sizeof(char));
		if(!mosq->id){
			return MOSQ_ERR_NOMEM;
		}
		mosq->id[0] = 'm';
		mosq->id[1] = 'o';
		mosq->id[2] = 's';
		mosq->id[3] = 'q';
		mosq->id[4] = '-';

		rc = util__random_bytes(&mosq->id[5], 18);
		if(rc) return rc;

		for(i=5; i<23; i++){
			mosq->id[i] = alphanum[(mosq->id[i]&0x7F)%(sizeof(alphanum)-1)];
		}
	}

	free(mosq->host);
	mosq->host = strdup(host);
	if(!mosq->host) return MOSQ_ERR_NOMEM;
	mosq->port = port;

	free(mosq->bind_address);
	// if(bind_address){
	// 	mosq->bind_address = mosquitto__strdup(bind_address);
	// 	if(!mosq->bind_address) return MOSQ_ERR_NOMEM;
	// }

	mosq->keepalive = keepalive;
	mosq->msgs_in.inflight_quota = mosq->msgs_in.inflight_maximum;
	mosq->msgs_out.inflight_quota = mosq->msgs_out.inflight_maximum;

	if(mosq->sockpairR != INVALID_SOCKET){
		close(mosq->sockpairR);
		mosq->sockpairR = INVALID_SOCKET;
	}
	if(mosq->sockpairW != INVALID_SOCKET){
		close(mosq->sockpairW);
		mosq->sockpairW = INVALID_SOCKET;
	}

	if(net__socketpair(&mosq->sockpairR, &mosq->sockpairW)){
		log__printf(mosq, MOSQ_LOG_WARNING,
				"Warning: Unable to open socket pair, outgoing publish commands may be delayed.");
	}

	return MOSQ_ERR_SUCCESS;
}

int mosquitto_connect(struct mosquitto *mosq, const char *host, int port, int keepalive)
{
	return mosquitto_connect_bind(mosq, host, port, keepalive, NULL);
}


int mosquitto_connect_bind(struct mosquitto *mosq, const char *host, int port, int keepalive, const char *bind_address)
{
	return mosquitto_connect_bind_v5(mosq, host, port, keepalive, bind_address, NULL);
}

int mosquitto_connect_bind_v5(struct mosquitto *mosq, const char *host, int port, int keepalive, const char *bind_address, const mosquitto_property *properties)
{
	int rc;

	// if(properties){
	// 	rc = mosquitto_property_check_all(CMD_CONNECT, properties);
	// 	if(rc) return rc;
	// }

	rc = mosquitto__connect_init(mosq, host, port, keepalive, bind_address);
	if(rc) 
    return rc;

	mosquitto__set_state(mosq, mosq_cs_new);

	return mosquitto__reconnect(mosq, true, properties);
}

int mosquitto_reconnect(struct mosquitto *mosq)
{
	return mosquitto__reconnect(mosq, true, NULL);
}


static int mosquitto__reconnect(struct mosquitto *mosq, bool blocking, const mosquitto_property *properties)
{
	const mosquitto_property *outgoing_properties = NULL;
	mosquitto_property local_property;
	int rc;

	if(!mosq) return MOSQ_ERR_INVAL;
	if(!mosq->host || mosq->port <= 0) return MOSQ_ERR_INVAL;
	if(mosq->protocol != mosq_p_mqtt5 && properties) return MOSQ_ERR_NOT_SUPPORTED;

	if(properties){
		if(properties->client_generated){
			outgoing_properties = properties;
		}else{
			memcpy(&local_property, properties, sizeof(mosquitto_property));
			local_property.client_generated = true;
			local_property.next = NULL;
			outgoing_properties = &local_property;
		}
		// rc = mosquitto_property_check_all(CMD_CONNECT, outgoing_properties);
		// if(rc) return rc;
	}

	pthread_mutex_lock(&mosq->msgtime_mutex);
	mosq->last_msg_in = mosquitto_time();
	mosq->next_msg_out = mosq->last_msg_in + mosq->keepalive;
	pthread_mutex_unlock(&mosq->msgtime_mutex);

	mosq->ping_t = 0;

	packet__cleanup(&mosq->in_packet);

	packet__cleanup_all(mosq);

	message__reconnect_reset(mosq);

	if(mosq->sock != INVALID_SOCKET){
        net__socket_close(mosq); //close socket
    }

// #ifdef WITH_SOCKS
	// if(mosq->socks5_host){
	// 	rc = net__socket_connect(mosq, mosq->socks5_host, mosq->socks5_port, mosq->bind_address, blocking);
	// }else
// #endif
	{
		rc = net__socket_connect(mosq, mosq->host, mosq->port, mosq->bind_address, blocking);
	}
	if(rc>0){
		mosquitto__set_state(mosq, mosq_cs_connect_pending);
		return rc;
	}

// #ifdef WITH_SOCKS
	// if(mosq->socks5_host){
	// 	mosquitto__set_state(mosq, mosq_cs_socks5_new);
	// 	return socks5__send(mosq);
	// }else
// #endif
	{
		mosquitto__set_state(mosq, mosq_cs_connected);
		rc = send__connect(mosq, mosq->keepalive, mosq->clean_start, outgoing_properties);
		if(rc){
			packet__cleanup_all(mosq);
			net__socket_close(mosq);
			mosquitto__set_state(mosq, mosq_cs_new);
		}
		return rc;
	}
}

int mosquitto_disconnect(struct mosquitto *mosq)
{
	return mosquitto_disconnect_v5(mosq, 0, NULL);
}

int mosquitto_disconnect_v5(struct mosquitto *mosq, int reason_code, const mosquitto_property *properties)
{
	const mosquitto_property *outgoing_properties = NULL;
	mosquitto_property local_property;
	int rc;
	if(!mosq) return MOSQ_ERR_INVAL;
	if(mosq->protocol != mosq_p_mqtt5 && properties) return MOSQ_ERR_NOT_SUPPORTED;

	if(properties){
		if(properties->client_generated){
			outgoing_properties = properties;
		}else{
			memcpy(&local_property, properties, sizeof(mosquitto_property));
			local_property.client_generated = true;
			local_property.next = NULL;
			outgoing_properties = &local_property;
		}
		rc = mosquitto_property_check_all(CMD_DISCONNECT, outgoing_properties);
		if(rc) return rc;
	}

	mosquitto__set_state(mosq, mosq_cs_disconnected);
	if(mosq->sock == INVALID_SOCKET){
		return MOSQ_ERR_NO_CONN;
	}else{
		return send__disconnect(mosq, reason_code, outgoing_properties);
	}
}

///////////////////////////////util_mosq.c///////////////////////////////////

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

///////////////////////////////actions.c///////////////////////////////////


int mosquitto_publish(struct mosquitto *mosq, int *mid, const char *topic, int payloadlen, const void *payload, int qos, bool retain)
{
	return mosquitto_publish_v5(mosq, mid, topic, payloadlen, payload, qos, retain, NULL);
}

int mosquitto_publish_v5(struct mosquitto *mosq, int *mid, const char *topic, int payloadlen, const void *payload, int qos, bool retain, const mosquitto_property *properties)
{
	struct mosquitto_message_all *message;
	uint16_t local_mid;
	const mosquitto_property *p;
	const mosquitto_property *outgoing_properties = NULL;
	mosquitto_property *properties_copy = NULL;
	mosquitto_property local_property;
	bool have_topic_alias;
	int rc;
	int tlen = 0;
	uint32_t remaining_length;

	if(!mosq || qos<0 || qos>2) return MOSQ_ERR_INVAL;
	if(mosq->protocol != mosq_p_mqtt5 && properties) return MOSQ_ERR_NOT_SUPPORTED;
	if(qos > mosq->maximum_qos) return MOSQ_ERR_QOS_NOT_SUPPORTED;

	if(properties){
		if(properties->client_generated){
			outgoing_properties = properties;
		}else{
			memcpy(&local_property, properties, sizeof(mosquitto_property));
			local_property.client_generated = true;
			local_property.next = NULL;
			outgoing_properties = &local_property;
		}
		// rc = mosquitto_property_check_all(CMD_PUBLISH, outgoing_properties);
		// if(rc) 
            return rc;
	}

	if(!topic || STREMPTY(topic)){
		if(topic) topic = NULL;

		if(mosq->protocol == mosq_p_mqtt5){
			p = outgoing_properties;
			have_topic_alias = false;
			while(p){
				if(p->identifier == MQTT_PROP_TOPIC_ALIAS){
					have_topic_alias = true;
					break;
				}
				p = p->next;
			}
			if(have_topic_alias == false){
				return MOSQ_ERR_INVAL;
			}
		}else{
			return MOSQ_ERR_INVAL;
		}
	}else{
		tlen = strlen(topic);
		if(mosquitto_validate_utf8(topic, tlen)) return MOSQ_ERR_MALFORMED_UTF8;
		if(payloadlen < 0 || payloadlen > MQTT_MAX_PAYLOAD) return MOSQ_ERR_PAYLOAD_SIZE;
		if(mosquitto_pub_topic_check(topic) != MOSQ_ERR_SUCCESS){
			return MOSQ_ERR_INVAL;
		}
	}

	if(mosq->maximum_packet_size > 0){
		remaining_length = 1 + 2+tlen + payloadlen + property__get_length_all(outgoing_properties);
		if(qos > 0){
			remaining_length++;
		}
		if(packet__check_oversize(mosq, remaining_length)){
			return MOSQ_ERR_OVERSIZE_PACKET;
		}
	}

	local_mid = mosquitto__mid_generate(mosq);
	if(mid){
		*mid = local_mid;
	}

	if(qos == 0){
		return send__publish(mosq, local_mid, topic, payloadlen, payload, qos, retain, false, outgoing_properties, NULL, 0);
	}else{
		if(outgoing_properties){
			rc = mosquitto_property_copy_all(&properties_copy, outgoing_properties);
			if(rc) return rc;
		}
		message = (struct mosquitto_message_all*)calloc(1, sizeof(struct mosquitto_message_all));
		if(!message){
			mosquitto_property_free_all(&properties_copy);
			return MOSQ_ERR_NOMEM;
		}

		message->next = NULL;
		message->timestamp = mosquitto_time();
		message->msg.mid = local_mid;
		if(topic){
			message->msg.topic = strdup(topic);
			if(!message->msg.topic){
				message__cleanup(&message);
				mosquitto_property_free_all(&properties_copy);
				return MOSQ_ERR_NOMEM;
			}
		}
		if(payloadlen){
			message->msg.payloadlen = payloadlen;
			message->msg.payload = (uint8_t*)malloc(payloadlen*sizeof(uint8_t));
			if(!message->msg.payload){
				message__cleanup(&message);
				mosquitto_property_free_all(&properties_copy);
				return MOSQ_ERR_NOMEM;
			}
			memcpy(message->msg.payload, payload, payloadlen*sizeof(uint8_t));
		}else{
			message->msg.payloadlen = 0;
			message->msg.payload = NULL;
		}
		message->msg.qos = qos;
		message->msg.retain = retain;
		message->dup = false;
		message->properties = properties_copy;

		pthread_mutex_lock(&mosq->msgs_out.mutex);
		message->state = mosq_ms_invalid;
		message__queue(mosq, message, mosq_md_out);
		pthread_mutex_unlock(&mosq->msgs_out.mutex);
		return MOSQ_ERR_SUCCESS;
	}
}


///////////////////////////////utf8_mosq.c///////////////////////////////////


int mosquitto_validate_utf8(const char *str, int len)
{
	int i;
	int j;
	int codelen;
	int codepoint;
	const unsigned char *ustr = (const unsigned char *)str;

	if(!str) return MOSQ_ERR_INVAL;
	if(len < 0 || len > 65536) return MOSQ_ERR_INVAL;

	for(i=0; i<len; i++){
		if(ustr[i] == 0){
			return MOSQ_ERR_MALFORMED_UTF8;
		}else if(ustr[i] <= 0x7f){
			codelen = 1;
			codepoint = ustr[i];
		}else if((ustr[i] & 0xE0) == 0xC0){
			/* 110xxxxx - 2 byte sequence */
			if(ustr[i] == 0xC0 || ustr[i] == 0xC1){
				/* Invalid bytes */
				return MOSQ_ERR_MALFORMED_UTF8;
			}
			codelen = 2;
			codepoint = (ustr[i] & 0x1F);
		}else if((ustr[i] & 0xF0) == 0xE0){
			// 1110xxxx - 3 byte sequence
			codelen = 3;
			codepoint = (ustr[i] & 0x0F);
		}else if((ustr[i] & 0xF8) == 0xF0){
			// 11110xxx - 4 byte sequence
			if(ustr[i] > 0xF4){
				/* Invalid, this would produce values > 0x10FFFF. */
				return MOSQ_ERR_MALFORMED_UTF8;
			}
			codelen = 4;
			codepoint = (ustr[i] & 0x07);
		}else{
			/* Unexpected continuation byte. */
			return MOSQ_ERR_MALFORMED_UTF8;
		}

		/* Reconstruct full code point */
		if(i == len-codelen+1){
			/* Not enough data */
			return MOSQ_ERR_MALFORMED_UTF8;
		}
		for(j=0; j<codelen-1; j++){
			if((ustr[++i] & 0xC0) != 0x80){
				/* Not a continuation byte */
				return MOSQ_ERR_MALFORMED_UTF8;
			}
			codepoint = (codepoint<<6) | (ustr[i] & 0x3F);
		}
		
		/* Check for UTF-16 high/low surrogates */
		if(codepoint >= 0xD800 && codepoint <= 0xDFFF){
			return MOSQ_ERR_MALFORMED_UTF8;
		}

		/* Check for overlong or out of range encodings */
		/* Checking codelen == 2 isn't necessary here, because it is already
		 * covered above in the C0 and C1 checks.
		 * if(codelen == 2 && codepoint < 0x0080){
		 *	 return MOSQ_ERR_MALFORMED_UTF8;
		 * }else
		*/
		if(codelen == 3 && codepoint < 0x0800){
			return MOSQ_ERR_MALFORMED_UTF8;
		}else if(codelen == 4 && (codepoint < 0x10000 || codepoint > 0x10FFFF)){
			return MOSQ_ERR_MALFORMED_UTF8;
		}

		/* Check for non-characters */
		if(codepoint >= 0xFDD0 && codepoint <= 0xFDEF){
			return MOSQ_ERR_MALFORMED_UTF8;
		}
		if((codepoint & 0xFFFF) == 0xFFFE || (codepoint & 0xFFFF) == 0xFFFF){
			return MOSQ_ERR_MALFORMED_UTF8;
		}
		/* Check for control characters */
		if(codepoint <= 0x001F || (codepoint >= 0x007F && codepoint <= 0x009F)){
			return MOSQ_ERR_MALFORMED_UTF8;
		}
	}
	return MOSQ_ERR_SUCCESS;
}


///////////////////////////////util_topic.c///////////////////////////////////


int mosquitto_pub_topic_check(const char *str)
{
        int len = 0;
// #ifdef WITH_BROKER
//         int hier_count = 0;
// #endif
        while(str && str[0]){
                if(str[0] == '+' || str[0] == '#'){
                        return MOSQ_ERR_INVAL;
                }
// #ifdef WITH_BROKER
//                 else if(str[0] == '/'){
//                         hier_count++;
//                 }
// #endif
                len++;
                str = &str[1];
        }
        if(len > 65535) return MOSQ_ERR_INVAL;
// #ifdef WITH_BROKER  //???????????????????????????????
//         if(hier_count > TOPIC_HIERARCHY_LIMIT) return MOSQ_ERR_INVAL;
// #endif

        return MOSQ_ERR_SUCCESS;
}


///////////////////////////////property_mosq.c///////////////////////////////////

                
int property__get_length_all(const mosquitto_property *property)
{               
        const mosquitto_property *p;
        int len = 0;    
        
        p = property;
        while(p){
                len += property__get_length(p); 
                p = p->next;
        }       
        return len;     
}               
      
int property__get_length(const mosquitto_property *property)
{
	if(!property) return 0;

	switch(property->identifier){
		/* Byte */
		case MQTT_PROP_PAYLOAD_FORMAT_INDICATOR:
		case MQTT_PROP_REQUEST_PROBLEM_INFORMATION:
		case MQTT_PROP_REQUEST_RESPONSE_INFORMATION:
		case MQTT_PROP_MAXIMUM_QOS:
		case MQTT_PROP_RETAIN_AVAILABLE:
		case MQTT_PROP_WILDCARD_SUB_AVAILABLE:
		case MQTT_PROP_SUBSCRIPTION_ID_AVAILABLE:
		case MQTT_PROP_SHARED_SUB_AVAILABLE:
			return 2; /* 1 (identifier) + 1 byte */

		/* uint16 */
		case MQTT_PROP_SERVER_KEEP_ALIVE:
		case MQTT_PROP_RECEIVE_MAXIMUM:
		case MQTT_PROP_TOPIC_ALIAS_MAXIMUM:
		case MQTT_PROP_TOPIC_ALIAS:
			return 3; /* 1 (identifier) + 2 bytes */

		/* uint32 */
		case MQTT_PROP_MESSAGE_EXPIRY_INTERVAL:
		case MQTT_PROP_WILL_DELAY_INTERVAL:
		case MQTT_PROP_MAXIMUM_PACKET_SIZE:
		case MQTT_PROP_SESSION_EXPIRY_INTERVAL:
			return 5; /* 1 (identifier) + 4 bytes */

		/* varint */
		case MQTT_PROP_SUBSCRIPTION_IDENTIFIER:
			if(property->value.varint < 128){
				return 2;
			}else if(property->value.varint < 16384){
				return 3;
			}else if(property->value.varint < 2097152){
				return 4;
			}else if(property->value.varint < 268435456){
				return 5;
			}else{
				return 0;
			}

		/* binary */
		case MQTT_PROP_CORRELATION_DATA:
		case MQTT_PROP_AUTHENTICATION_DATA:
			return 3 + property->value.bin.len; /* 1 + 2 bytes (len) + X bytes (payload) */

		/* string */
		case MQTT_PROP_CONTENT_TYPE:
		case MQTT_PROP_RESPONSE_TOPIC:
		case MQTT_PROP_ASSIGNED_CLIENT_IDENTIFIER:
		case MQTT_PROP_AUTHENTICATION_METHOD:
		case MQTT_PROP_RESPONSE_INFORMATION:
		case MQTT_PROP_SERVER_REFERENCE:
		case MQTT_PROP_REASON_STRING:
			return 3 + property->value.s.len; /* 1 + 2 bytes (len) + X bytes (string) */

		/* string pair */
		case MQTT_PROP_USER_PROPERTY:
			return 5 + property->value.s.len + property->name.len; /* 1 + 2*(2 bytes (len) + X bytes (string))*/

		default:
			return 0;
	}
	return 0;
}

int property__write(struct mosquitto__packet *packet, const mosquitto_property *property)
{
	int rc;

	rc = packet__write_varint(packet, property->identifier);
	if(rc) return rc;

	switch(property->identifier){
		case MQTT_PROP_PAYLOAD_FORMAT_INDICATOR:
		case MQTT_PROP_REQUEST_PROBLEM_INFORMATION:
		case MQTT_PROP_REQUEST_RESPONSE_INFORMATION:
		case MQTT_PROP_MAXIMUM_QOS:
		case MQTT_PROP_RETAIN_AVAILABLE:
		case MQTT_PROP_WILDCARD_SUB_AVAILABLE:
		case MQTT_PROP_SUBSCRIPTION_ID_AVAILABLE:
		case MQTT_PROP_SHARED_SUB_AVAILABLE:
			packet__write_byte(packet, property->value.i8);
			break;

		case MQTT_PROP_SERVER_KEEP_ALIVE:
		case MQTT_PROP_RECEIVE_MAXIMUM:
		case MQTT_PROP_TOPIC_ALIAS_MAXIMUM:
		case MQTT_PROP_TOPIC_ALIAS:
			packet__write_uint16(packet, property->value.i16);
			break;

		case MQTT_PROP_MESSAGE_EXPIRY_INTERVAL:
		case MQTT_PROP_SESSION_EXPIRY_INTERVAL:
		case MQTT_PROP_WILL_DELAY_INTERVAL:
		case MQTT_PROP_MAXIMUM_PACKET_SIZE:
			packet__write_uint32(packet, property->value.i32);
			break;

		case MQTT_PROP_SUBSCRIPTION_IDENTIFIER:
			return packet__write_varint(packet, property->value.varint);

		case MQTT_PROP_CONTENT_TYPE:
		case MQTT_PROP_RESPONSE_TOPIC:
		case MQTT_PROP_ASSIGNED_CLIENT_IDENTIFIER:
		case MQTT_PROP_AUTHENTICATION_METHOD:
		case MQTT_PROP_RESPONSE_INFORMATION:
		case MQTT_PROP_SERVER_REFERENCE:
		case MQTT_PROP_REASON_STRING:
			packet__write_string(packet, property->value.s.v, property->value.s.len);
			break;

		case MQTT_PROP_AUTHENTICATION_DATA:
		case MQTT_PROP_CORRELATION_DATA:
			packet__write_uint16(packet, property->value.bin.len);
			packet__write_bytes(packet, property->value.bin.v, property->value.bin.len);
			break;

		case MQTT_PROP_USER_PROPERTY:
			packet__write_string(packet, property->name.v, property->name.len);
			packet__write_string(packet, property->value.s.v, property->value.s.len);
			break;

		default:
			log__printf(NULL, MOSQ_LOG_DEBUG, "Unsupported property type: %d", property->identifier);
			return MOSQ_ERR_INVAL;
	}

	return MOSQ_ERR_SUCCESS;
}

int property__write_all(struct mosquitto__packet *packet, const mosquitto_property *properties, bool write_len)
{       
        int rc; 
        const mosquitto_property *p;
                
        if(write_len){
                rc = packet__write_varint(packet, property__get_length_all(properties));
                if(rc) return rc;
        }

        p = properties;
        while(p){
                rc = property__write(packet, p);
                if(rc) return rc;
                p = p->next;
        }               
                
        return MOSQ_ERR_SUCCESS;
} 

int property__read_all(int command, struct mosquitto__packet *packet, mosquitto_property **properties)                       
{                       
        int rc; 
        int32_t proplen;
        mosquitto_property *p, *tail = NULL;
                
        rc = packet__read_varint(packet, &proplen, NULL);
        if(rc) return rc;
                
        *properties = NULL;
                        
        /* The order of properties must be preserved for some types, so keep the
         * same order for all */
        while(proplen > 0){
                p = (mosquitto_property*)calloc(1, sizeof(mosquitto_property));
                if(!p){
                        mosquitto_property_free_all(properties);
                        return MOSQ_ERR_NOMEM;
                }
        
                rc = property__read(packet, &proplen, p);
                if(rc){
                        free(p);
                        mosquitto_property_free_all(properties);
                        return rc;
                }

                if(!(*properties)){
                        *properties = p;
                }else{
                        tail->next = p;
                }
                tail = p;

        }

        rc = mosquitto_property_check_all(command, *properties);
        if(rc){
                mosquitto_property_free_all(properties);
                return rc;
        }
        return MOSQ_ERR_SUCCESS;
}

int property__read(struct mosquitto__packet *packet, int32_t *len, mosquitto_property *property)
{
	int rc;
	int32_t property_identifier;
	uint8_t byte;
	int8_t byte_count;
	uint16_t uint16;
	uint32_t uint32;
	int32_t varint;
	char *str1, *str2;
	int slen1, slen2;

	if(!property) return MOSQ_ERR_INVAL;

	rc = packet__read_varint(packet, &property_identifier, NULL);
	if(rc) return rc;
	*len -= 1;

	memset(property, 0, sizeof(mosquitto_property));

	property->identifier = property_identifier;

	switch(property_identifier){
		case MQTT_PROP_PAYLOAD_FORMAT_INDICATOR:
		case MQTT_PROP_REQUEST_PROBLEM_INFORMATION:
		case MQTT_PROP_REQUEST_RESPONSE_INFORMATION:
		case MQTT_PROP_MAXIMUM_QOS:
		case MQTT_PROP_RETAIN_AVAILABLE:
		case MQTT_PROP_WILDCARD_SUB_AVAILABLE:
		case MQTT_PROP_SUBSCRIPTION_ID_AVAILABLE:
		case MQTT_PROP_SHARED_SUB_AVAILABLE:
			rc = packet__read_byte(packet, &byte);
			if(rc) return rc;
			*len -= 1; /* byte */
			property->value.i8 = byte;
			break;

		case MQTT_PROP_SERVER_KEEP_ALIVE:
		case MQTT_PROP_RECEIVE_MAXIMUM:
		case MQTT_PROP_TOPIC_ALIAS_MAXIMUM:
		case MQTT_PROP_TOPIC_ALIAS:
			rc = packet__read_uint16(packet, &uint16);
			if(rc) return rc;
			*len -= 2; /* uint16 */
			property->value.i16 = uint16;
			break;

		case MQTT_PROP_MESSAGE_EXPIRY_INTERVAL:
		case MQTT_PROP_SESSION_EXPIRY_INTERVAL:
		case MQTT_PROP_WILL_DELAY_INTERVAL:
		case MQTT_PROP_MAXIMUM_PACKET_SIZE:
			rc = packet__read_uint32(packet, &uint32);
			if(rc) return rc;
			*len -= 4; /* uint32 */
			property->value.i32 = uint32;
			break;

		case MQTT_PROP_SUBSCRIPTION_IDENTIFIER:
			rc = packet__read_varint(packet, &varint, &byte_count);
			if(rc) return rc;
			*len -= byte_count;
			property->value.varint = varint;
			break;

		case MQTT_PROP_CONTENT_TYPE:
		case MQTT_PROP_RESPONSE_TOPIC:
		case MQTT_PROP_ASSIGNED_CLIENT_IDENTIFIER:
		case MQTT_PROP_AUTHENTICATION_METHOD:
		case MQTT_PROP_RESPONSE_INFORMATION:
		case MQTT_PROP_SERVER_REFERENCE:
		case MQTT_PROP_REASON_STRING:
			rc = packet__read_string(packet, &str1, &slen1);
			if(rc) return rc;
			*len = (*len) - 2 - slen1; /* uint16, string len */
			property->value.s.v = str1;
			property->value.s.len = slen1;
			break;

		case MQTT_PROP_AUTHENTICATION_DATA:
		case MQTT_PROP_CORRELATION_DATA:
			rc = packet__read_binary(packet, (uint8_t **)&str1, &slen1);
			if(rc) return rc;
			*len = (*len) - 2 - slen1; /* uint16, binary len */
			property->value.bin.v = str1;
			property->value.bin.len = slen1;
			break;

		case MQTT_PROP_USER_PROPERTY:
			rc = packet__read_string(packet, &str1, &slen1);
			if(rc) return rc;
			*len = (*len) - 2 - slen1; /* uint16, string len */

			rc = packet__read_string(packet, &str2, &slen2);
			if(rc){
				free(str1);
				return rc;
			}
			*len = (*len) - 2 - slen2; /* uint16, string len */

			property->name.v = str1;
			property->name.len = slen1;
			property->value.s.v = str2;
			property->value.s.len = slen2;
			break;

		default:
			log__printf(NULL, MOSQ_LOG_DEBUG, "Unsupported property type: %d", property_identifier);
			return MOSQ_ERR_MALFORMED_PACKET;
	}

	return MOSQ_ERR_SUCCESS;
}

void property__free(mosquitto_property **property)
{       
        if(!property || !(*property)) return;
        
        switch((*property)->identifier){
                case MQTT_PROP_CONTENT_TYPE:
                case MQTT_PROP_RESPONSE_TOPIC:
                case MQTT_PROP_ASSIGNED_CLIENT_IDENTIFIER:
                case MQTT_PROP_AUTHENTICATION_METHOD:
                case MQTT_PROP_RESPONSE_INFORMATION:
                case MQTT_PROP_SERVER_REFERENCE:
                case MQTT_PROP_REASON_STRING:
                        free((*property)->value.s.v);
                        break;
                
                case MQTT_PROP_AUTHENTICATION_DATA:
                case MQTT_PROP_CORRELATION_DATA:
                        free((*property)->value.bin.v);
                        break;
                
                case MQTT_PROP_USER_PROPERTY:
                        free((*property)->name.v);
                        free((*property)->value.s.v);
                        break;
                
                case MQTT_PROP_PAYLOAD_FORMAT_INDICATOR:
                case MQTT_PROP_MESSAGE_EXPIRY_INTERVAL:
                case MQTT_PROP_SUBSCRIPTION_IDENTIFIER:
                case MQTT_PROP_SESSION_EXPIRY_INTERVAL:
                case MQTT_PROP_SERVER_KEEP_ALIVE:
                case MQTT_PROP_REQUEST_PROBLEM_INFORMATION:
                case MQTT_PROP_WILL_DELAY_INTERVAL:
                case MQTT_PROP_REQUEST_RESPONSE_INFORMATION:
                case MQTT_PROP_RECEIVE_MAXIMUM:
                case MQTT_PROP_TOPIC_ALIAS_MAXIMUM:
                case MQTT_PROP_TOPIC_ALIAS:
                case MQTT_PROP_MAXIMUM_QOS:
                case MQTT_PROP_RETAIN_AVAILABLE:
                case MQTT_PROP_MAXIMUM_PACKET_SIZE:
                case MQTT_PROP_WILDCARD_SUB_AVAILABLE:
                case MQTT_PROP_SUBSCRIPTION_ID_AVAILABLE:
                case MQTT_PROP_SHARED_SUB_AVAILABLE:
                        /* Nothing to free */
                        break;
        }
        
        free(*property);
        *property = NULL;
}


void mosquitto_property_free_all(mosquitto_property **property)
{
        mosquitto_property *p, *next;

        if(!property) return;

        p = *property;
        while(p){
                next = p->next;
                property__free(&p);
                p = next;
        }
        *property = NULL;
}

int mosquitto_property_copy_all(mosquitto_property **dest, const mosquitto_property *src)
{
	mosquitto_property *pnew, *plast = NULL;

	if(!src) return MOSQ_ERR_SUCCESS;
	if(!dest) return MOSQ_ERR_INVAL;

	*dest = NULL;

	while(src){
		pnew = (mosquitto_property *)calloc(1, sizeof(mosquitto_property));
		if(!pnew){
			mosquitto_property_free_all(dest);
			return MOSQ_ERR_NOMEM;
		}
		if(plast){
			plast->next = pnew;
		}else{
			*dest = pnew;
		}
		plast = pnew;

		pnew->identifier = src->identifier;
		switch(pnew->identifier){
			case MQTT_PROP_PAYLOAD_FORMAT_INDICATOR:
			case MQTT_PROP_REQUEST_PROBLEM_INFORMATION:
			case MQTT_PROP_REQUEST_RESPONSE_INFORMATION:
			case MQTT_PROP_MAXIMUM_QOS:
			case MQTT_PROP_RETAIN_AVAILABLE:
			case MQTT_PROP_WILDCARD_SUB_AVAILABLE:
			case MQTT_PROP_SUBSCRIPTION_ID_AVAILABLE:
			case MQTT_PROP_SHARED_SUB_AVAILABLE:
				pnew->value.i8 = src->value.i8;
				break;

			case MQTT_PROP_SERVER_KEEP_ALIVE:
			case MQTT_PROP_RECEIVE_MAXIMUM:
			case MQTT_PROP_TOPIC_ALIAS_MAXIMUM:
			case MQTT_PROP_TOPIC_ALIAS:
				pnew->value.i16 = src->value.i16;
				break;

			case MQTT_PROP_MESSAGE_EXPIRY_INTERVAL:
			case MQTT_PROP_SESSION_EXPIRY_INTERVAL:
			case MQTT_PROP_WILL_DELAY_INTERVAL:
			case MQTT_PROP_MAXIMUM_PACKET_SIZE:
				pnew->value.i32 = src->value.i32;
				break;

			case MQTT_PROP_SUBSCRIPTION_IDENTIFIER:
				pnew->value.varint = src->value.varint;
				break;

			case MQTT_PROP_CONTENT_TYPE:
			case MQTT_PROP_RESPONSE_TOPIC:
			case MQTT_PROP_ASSIGNED_CLIENT_IDENTIFIER:
			case MQTT_PROP_AUTHENTICATION_METHOD:
			case MQTT_PROP_RESPONSE_INFORMATION:
			case MQTT_PROP_SERVER_REFERENCE:
			case MQTT_PROP_REASON_STRING:
				pnew->value.s.len = src->value.s.len;
				pnew->value.s.v = strdup(src->value.s.v);
				if(!pnew->value.s.v){
					mosquitto_property_free_all(dest);
					return MOSQ_ERR_NOMEM;
				}
				break;

			case MQTT_PROP_AUTHENTICATION_DATA:
			case MQTT_PROP_CORRELATION_DATA:
				pnew->value.bin.len = src->value.bin.len;
				pnew->value.bin.v = (char*)malloc(pnew->value.bin.len);
				if(!pnew->value.bin.v){
					mosquitto_property_free_all(dest);
					return MOSQ_ERR_NOMEM;
				}
				memcpy(pnew->value.bin.v, src->value.bin.v, pnew->value.bin.len);
				break;

			case MQTT_PROP_USER_PROPERTY:
				pnew->value.s.len = src->value.s.len;
				pnew->value.s.v = strdup(src->value.s.v);
				if(!pnew->value.s.v){
					mosquitto_property_free_all(dest);
					return MOSQ_ERR_NOMEM;
				}

				pnew->name.len = src->name.len;
				pnew->name.v = strdup(src->name.v);
				if(!pnew->name.v){
					mosquitto_property_free_all(dest);
					return MOSQ_ERR_NOMEM;
				}
				break;

			default:
				mosquitto_property_free_all(dest);
				return MOSQ_ERR_INVAL;
		}

		src = src->next;
	}

	return MOSQ_ERR_SUCCESS;
}

int mosquitto_property_check_all(int command, const mosquitto_property *properties)
{       
        const mosquitto_property *p, *tail;
        int rc;
        
        p = properties;
        
        while(p){
                /* Validity checks */
                if(p->identifier == MQTT_PROP_REQUEST_PROBLEM_INFORMATION
                                || p->identifier == MQTT_PROP_REQUEST_RESPONSE_INFORMATION
                                || p->identifier == MQTT_PROP_MAXIMUM_QOS
                                || p->identifier == MQTT_PROP_RETAIN_AVAILABLE
                                || p->identifier == MQTT_PROP_WILDCARD_SUB_AVAILABLE
                                || p->identifier == MQTT_PROP_SUBSCRIPTION_ID_AVAILABLE
                                || p->identifier == MQTT_PROP_SHARED_SUB_AVAILABLE){
                        
                        if(p->value.i8 > 1){
                                return MOSQ_ERR_PROTOCOL;
                        }
                }else if(p->identifier == MQTT_PROP_MAXIMUM_PACKET_SIZE){
                        if( p->value.i32 == 0){
                                return MOSQ_ERR_PROTOCOL;
                        }
                }else if(p->identifier == MQTT_PROP_RECEIVE_MAXIMUM
                                || p->identifier == MQTT_PROP_TOPIC_ALIAS){
                        
                        if(p->value.i16 == 0){
                                return MOSQ_ERR_PROTOCOL;
                        }
                }
                
                /* Check for properties on incorrect commands */
                rc = mosquitto_property_check_command(command, p->identifier);
                if(rc) return rc;
                
                /* Check for duplicates */
                tail = p->next;
                while(tail){
                        if(p->identifier == tail->identifier
                                        && p->identifier != MQTT_PROP_USER_PROPERTY){
                                
                                return MOSQ_ERR_DUPLICATE_PROPERTY;
                        }
                        tail = tail->next;
                }
                
                p = p->next;
        }
        
        return MOSQ_ERR_SUCCESS;
}

const mosquitto_property *mosquitto_property_read_int16(const mosquitto_property *proplist, int identifier, uint16_t *value, bool skip_first)
{ 
        const mosquitto_property *p;
        if(!proplist) return NULL;
                        
        p = property__get_property(proplist, identifier, skip_first);
        if(!p) return NULL;
        if(p->identifier != MQTT_PROP_SERVER_KEEP_ALIVE
                        && p->identifier != MQTT_PROP_RECEIVE_MAXIMUM
                        && p->identifier != MQTT_PROP_TOPIC_ALIAS_MAXIMUM
                        && p->identifier != MQTT_PROP_TOPIC_ALIAS){
                return NULL;
        }       
                
        if(value) *value = p->value.i16;
        
        return p;
}    

const mosquitto_property *property__get_property(const mosquitto_property *proplist, int identifier, bool skip_first)
{
        const mosquitto_property *p;
        bool is_first = true;

        p = proplist;

        while(p){
                if(p->identifier == identifier){
                        if(!is_first || !skip_first){
                                return p;
                        }
                        is_first = false;
                }
                p = p->next;
        }
        return NULL;
}

int mosquitto_property_add_int16(mosquitto_property **proplist, int identifier, uint16_t value)
{
        mosquitto_property *prop;
                
        if(!proplist) return MOSQ_ERR_INVAL;
        if(identifier != MQTT_PROP_SERVER_KEEP_ALIVE
                        && identifier != MQTT_PROP_RECEIVE_MAXIMUM
                        && identifier != MQTT_PROP_TOPIC_ALIAS_MAXIMUM
                        && identifier != MQTT_PROP_TOPIC_ALIAS){
                return MOSQ_ERR_INVAL;
        } 

        prop = (mosquitto_property*)calloc(1, sizeof(mosquitto_property));
        if(!prop) return MOSQ_ERR_NOMEM;
        
        prop->client_generated = true;
        prop->identifier = identifier;
        prop->value.i16 = value;
                
        property__add(proplist, prop);
        return MOSQ_ERR_SUCCESS;
}   

static void property__add(mosquitto_property **proplist, struct mqtt5__property *prop)
{
        mosquitto_property *p;

        if(!(*proplist)){
                *proplist = prop;
        }

        p = *proplist;
        while(p->next){
                p = p->next;
        }
        p->next = prop;
        prop->next = NULL;
}

int mosquitto_property_check_command(int command, int identifier)
{
	switch(identifier){
		case MQTT_PROP_PAYLOAD_FORMAT_INDICATOR:
		case MQTT_PROP_MESSAGE_EXPIRY_INTERVAL:
		case MQTT_PROP_CONTENT_TYPE:
		case MQTT_PROP_RESPONSE_TOPIC:
		case MQTT_PROP_CORRELATION_DATA:
			if(command != CMD_PUBLISH && command != CMD_WILL){
				return MOSQ_ERR_PROTOCOL;
			}
			break;

		case MQTT_PROP_SUBSCRIPTION_IDENTIFIER:
			if(command != CMD_PUBLISH && command != CMD_SUBSCRIBE){
				return MOSQ_ERR_PROTOCOL;
			}
			break;

		case MQTT_PROP_SESSION_EXPIRY_INTERVAL:
			if(command != CMD_CONNECT && command != CMD_CONNACK && command != CMD_DISCONNECT){
				return MOSQ_ERR_PROTOCOL;
			}
			break;

		case MQTT_PROP_AUTHENTICATION_METHOD:
		case MQTT_PROP_AUTHENTICATION_DATA:
			if(command != CMD_CONNECT && command != CMD_CONNACK && command != CMD_AUTH){
				return MOSQ_ERR_PROTOCOL;
			}
			break;

		case MQTT_PROP_ASSIGNED_CLIENT_IDENTIFIER:
		case MQTT_PROP_SERVER_KEEP_ALIVE:
		case MQTT_PROP_RESPONSE_INFORMATION:
		case MQTT_PROP_MAXIMUM_QOS:
		case MQTT_PROP_RETAIN_AVAILABLE:
		case MQTT_PROP_WILDCARD_SUB_AVAILABLE:
		case MQTT_PROP_SUBSCRIPTION_ID_AVAILABLE:
		case MQTT_PROP_SHARED_SUB_AVAILABLE:
			if(command != CMD_CONNACK){
				return MOSQ_ERR_PROTOCOL;
			}
			break;

		case MQTT_PROP_WILL_DELAY_INTERVAL:
			if(command != CMD_WILL){
				return MOSQ_ERR_PROTOCOL;
			}
			break;

		case MQTT_PROP_REQUEST_PROBLEM_INFORMATION:
		case MQTT_PROP_REQUEST_RESPONSE_INFORMATION:
			if(command != CMD_CONNECT){
				return MOSQ_ERR_PROTOCOL;
			}
			break;

		case MQTT_PROP_SERVER_REFERENCE:
			if(command != CMD_CONNACK && command != CMD_DISCONNECT){
				return MOSQ_ERR_PROTOCOL;
			}
			break;

		case MQTT_PROP_REASON_STRING:
			if(command == CMD_CONNECT || command == CMD_PUBLISH || command == CMD_SUBSCRIBE || command == CMD_UNSUBSCRIBE){
				return MOSQ_ERR_PROTOCOL;
			}
			break;

		case MQTT_PROP_RECEIVE_MAXIMUM:
		case MQTT_PROP_TOPIC_ALIAS_MAXIMUM:
		case MQTT_PROP_MAXIMUM_PACKET_SIZE:
			if(command != CMD_CONNECT && command != CMD_CONNACK){
				return MOSQ_ERR_PROTOCOL;
			}
			break;

		case MQTT_PROP_TOPIC_ALIAS:
			if(command != CMD_PUBLISH){
				return MOSQ_ERR_PROTOCOL;
			}
			break;

		case MQTT_PROP_USER_PROPERTY:
			break;

		default:
			return MOSQ_ERR_PROTOCOL;
	}
	return MOSQ_ERR_SUCCESS;
}

const mosquitto_property *mosquitto_property_read_byte(const mosquitto_property *proplist, int identifier, uint8_t *value, bool skip_first)
{
	const mosquitto_property *p;
	if(!proplist) return NULL;

	p = property__get_property(proplist, identifier, skip_first);
	if(!p) return NULL;
	if(p->identifier != MQTT_PROP_PAYLOAD_FORMAT_INDICATOR
			&& p->identifier != MQTT_PROP_REQUEST_PROBLEM_INFORMATION
			&& p->identifier != MQTT_PROP_REQUEST_RESPONSE_INFORMATION
			&& p->identifier != MQTT_PROP_MAXIMUM_QOS
			&& p->identifier != MQTT_PROP_RETAIN_AVAILABLE
			&& p->identifier != MQTT_PROP_WILDCARD_SUB_AVAILABLE
			&& p->identifier != MQTT_PROP_SUBSCRIPTION_ID_AVAILABLE
			&& p->identifier != MQTT_PROP_SHARED_SUB_AVAILABLE){
		return NULL;
	}

	if(value) *value = p->value.i8;

	return p;
}

const mosquitto_property *mosquitto_property_read_int32(const mosquitto_property *proplist, int identifier, uint32_t *value, bool skip_first)
{
	const mosquitto_property *p;
	if(!proplist) return NULL;

	p = property__get_property(proplist, identifier, skip_first);
	if(!p) return NULL;
	if(p->identifier != MQTT_PROP_MESSAGE_EXPIRY_INTERVAL
			&& p->identifier != MQTT_PROP_SESSION_EXPIRY_INTERVAL
			&& p->identifier != MQTT_PROP_WILL_DELAY_INTERVAL
			&& p->identifier != MQTT_PROP_MAXIMUM_PACKET_SIZE){

		return NULL;
	}

	if(value) *value = p->value.i32;

	return p;
}

const mosquitto_property *mosquitto_property_read_string(const mosquitto_property *proplist, int identifier, char **value, bool skip_first)
{
	const mosquitto_property *p;
	if(!proplist) return NULL;

	p = property__get_property(proplist, identifier, skip_first);
	if(!p) return NULL;
	if(p->identifier != MQTT_PROP_CONTENT_TYPE
			&& p->identifier != MQTT_PROP_RESPONSE_TOPIC
			&& p->identifier != MQTT_PROP_ASSIGNED_CLIENT_IDENTIFIER
			&& p->identifier != MQTT_PROP_AUTHENTICATION_METHOD
			&& p->identifier != MQTT_PROP_RESPONSE_INFORMATION
			&& p->identifier != MQTT_PROP_SERVER_REFERENCE
			&& p->identifier != MQTT_PROP_REASON_STRING){

		return NULL;
	}

	if(value){
		*value = (char*)calloc(1, p->value.s.len+1);
		if(!(*value)) return NULL;

		memcpy(*value, p->value.s.v, p->value.s.len);
	}

	return p;
}

///////////////////////////////packet_mosq.c///////////////////////////////////

void packet__cleanup(struct mosquitto__packet *packet)
{       
        if(!packet) return; 
                
        /* Free data and reset values */
        packet->command = 0;
        packet->remaining_count = 0;
        packet->remaining_mult = 1;
        packet->remaining_length = 0;
        free(packet->payload);
        packet->payload = NULL;
        packet->to_process = 0;
        packet->pos = 0;
}

int packet__queue(struct mosquitto *mosq, struct mosquitto__packet *packet)
{
// #ifndef WITH_BROKER
	char sockpair_data = 0;
// #endif
	assert(mosq);
	assert(packet);

	packet->pos = 0;
	packet->to_process = packet->packet_length;

	packet->next = NULL;
	pthread_mutex_lock(&mosq->out_packet_mutex);
	if(mosq->out_packet){
		mosq->out_packet_last->next = packet;
	}else{
		mosq->out_packet = packet;
	}
	mosq->out_packet_last = packet;
	pthread_mutex_unlock(&mosq->out_packet_mutex);
// #ifdef WITH_BROKER
// #  ifdef WITH_WEBSOCKETS
// 	if(mosq->wsi){
// 		libwebsocket_callback_on_writable(mosq->ws_context, mosq->wsi);
// 		return MOSQ_ERR_SUCCESS;
// 	}else{
// 		return packet__write(mosq);
// 	}
// #  else
//  	return packet__write(mosq);
// #  endif
// #else

// 	/* Write a single byte to sockpairW (connected to sockpairR) to break out
// 	 * of select() if in threaded mode. */
	if(mosq->sockpairW != INVALID_SOCKET){
// // #ifndef WIN32
		if(write(mosq->sockpairW, &sockpair_data, 1)){
		}
// // #else
// 		send(mosq->sockpairW, &sockpair_data, 1, 0);
// // #endif
	}

	if(mosq->in_callback == false && mosq->threaded == mosq_ts_none){
		return packet__write(mosq);
	}else{
		return MOSQ_ERR_SUCCESS;
	}
// #endif
}

int packet__check_oversize(struct mosquitto *mosq, uint32_t remaining_length)
{               
        uint32_t len;   
                
        if(mosq->maximum_packet_size == 0) return MOSQ_ERR_SUCCESS;
                        
        len = remaining_length + packet__varint_bytes(remaining_length);
        if(len > mosq->maximum_packet_size){
                return MOSQ_ERR_OVERSIZE_PACKET;
        }else{
                return MOSQ_ERR_SUCCESS;
        }       
}    

int packet__alloc(struct mosquitto__packet *packet)
{
	uint8_t remaining_bytes[5], byte;
	uint32_t remaining_length;
	int i;

	assert(packet);

	remaining_length = packet->remaining_length;
	packet->payload = NULL;
	packet->remaining_count = 0;
	do{
		byte = remaining_length % 128;
		remaining_length = remaining_length / 128;
		/* If there are more digits to encode, set the top bit of this digit */
		if(remaining_length > 0){
			byte = byte | 0x80;
		}
		remaining_bytes[packet->remaining_count] = byte;
		packet->remaining_count++;
	}while(remaining_length > 0 && packet->remaining_count < 5);
	if(packet->remaining_count == 5) return MOSQ_ERR_PAYLOAD_SIZE;
	packet->packet_length = packet->remaining_length + 1 + packet->remaining_count;
// #ifdef WITH_WEBSOCKETS
// 	packet->payload = (uint8_t*)malloc(sizeof(uint8_t)*packet->packet_length + LWS_SEND_BUFFER_PRE_PADDING + LWS_SEND_BUFFER_POST_PADDING);
// #else
	packet->payload = (uint8_t*)malloc(sizeof(uint8_t)*packet->packet_length);
// #endif
	if(!packet->payload) return MOSQ_ERR_NOMEM;

	packet->payload[0] = packet->command;
	for(i=0; i<packet->remaining_count; i++){
		packet->payload[i+1] = remaining_bytes[i];
	}
	packet->pos = 1 + packet->remaining_count;

	return MOSQ_ERR_SUCCESS;
}

int packet__write(struct mosquitto *mosq)
{
	ssize_t write_length;
	struct mosquitto__packet *packet;
	int state;

	if(!mosq) return MOSQ_ERR_INVAL;
	if(mosq->sock == INVALID_SOCKET) return MOSQ_ERR_NO_CONN;

	pthread_mutex_lock(&mosq->current_out_packet_mutex);
	pthread_mutex_lock(&mosq->out_packet_mutex);
	if(mosq->out_packet && !mosq->current_out_packet){
		mosq->current_out_packet = mosq->out_packet;
		mosq->out_packet = mosq->out_packet->next;
		if(!mosq->out_packet){
			mosq->out_packet_last = NULL;
		}
	}
	pthread_mutex_unlock(&mosq->out_packet_mutex);

	state = mosquitto__get_state(mosq);
#if defined(WITH_TLS) && !defined(WITH_BROKER)
	if((state == mosq_cs_connect_pending) || mosq->want_connect){
#else
	if(state == mosq_cs_connect_pending){
#endif
		pthread_mutex_unlock(&mosq->current_out_packet_mutex);
		return MOSQ_ERR_SUCCESS;
	}

	while(mosq->current_out_packet){
		packet = mosq->current_out_packet;

		while(packet->to_process > 0){
			write_length = net__write(mosq, &(packet->payload[packet->pos]), packet->to_process);
			if(write_length > 0){
				G_BYTES_SENT_INC(write_length);
				packet->to_process -= write_length;
				packet->pos += write_length;
			}else{
// #ifdef WIN32
// 				errno = WSAGetLastError();
// #endif
				if(errno == EAGAIN || errno == EWOULDBLOCK
// #ifdef WIN32
// 						|| errno == WSAENOTCONN
// #endif
						){
					pthread_mutex_unlock(&mosq->current_out_packet_mutex);
					return MOSQ_ERR_SUCCESS;
				}else{
					pthread_mutex_unlock(&mosq->current_out_packet_mutex);
					switch(errno){
						case ECONNRESET:
							return MOSQ_ERR_CONN_LOST;
						default:
							return MOSQ_ERR_ERRNO;
					}
				}
			}
		}

		G_MSGS_SENT_INC(1);
		if(((packet->command)&0xF6) == CMD_PUBLISH){
			G_PUB_MSGS_SENT_INC(1);
#ifndef WITH_BROKER
			pthread_mutex_lock(&mosq->callback_mutex);
			if(mosq->on_publish){
				/* This is a QoS=0 message */
				mosq->in_callback = true;
				mosq->on_publish(mosq, mosq->userdata, packet->mid);
				mosq->in_callback = false;
			}
			if(mosq->on_publish_v5){
				/* This is a QoS=0 message */
				mosq->in_callback = true;
				mosq->on_publish_v5(mosq, mosq->userdata, packet->mid, 0, NULL);
				mosq->in_callback = false;
			}
			pthread_mutex_unlock(&mosq->callback_mutex);
		}else if(((packet->command)&0xF0) == CMD_DISCONNECT){
			do_client_disconnect(mosq, MOSQ_ERR_SUCCESS, NULL);
			packet__cleanup(packet);
			free(packet);
			return MOSQ_ERR_SUCCESS;
#endif
		}

		/* Free data and reset values */
		pthread_mutex_lock(&mosq->out_packet_mutex);
		mosq->current_out_packet = mosq->out_packet;
		if(mosq->out_packet){
			mosq->out_packet = mosq->out_packet->next;
			if(!mosq->out_packet){
				mosq->out_packet_last = NULL;
			}
		}
		pthread_mutex_unlock(&mosq->out_packet_mutex);

		packet__cleanup(packet);
		free(packet);

		pthread_mutex_lock(&mosq->msgtime_mutex);
		mosq->next_msg_out = mosquitto_time() + mosq->keepalive;
		pthread_mutex_unlock(&mosq->msgtime_mutex);
	}
	pthread_mutex_unlock(&mosq->current_out_packet_mutex);
	return MOSQ_ERR_SUCCESS;
}

void packet__cleanup_all(struct mosquitto *mosq)
{               
        struct mosquitto__packet *packet;
                
        pthread_mutex_lock(&mosq->current_out_packet_mutex);
        pthread_mutex_lock(&mosq->out_packet_mutex);
        
        /* Out packet cleanup */
        if(mosq->out_packet && !mosq->current_out_packet){
                mosq->current_out_packet = mosq->out_packet;
                mosq->out_packet = mosq->out_packet->next;
        }
        while(mosq->current_out_packet){
                packet = mosq->current_out_packet;
                /* Free data and reset values */
                mosq->current_out_packet = mosq->out_packet;
                if(mosq->out_packet){
                        mosq->out_packet = mosq->out_packet->next;
                }
    
                packet__cleanup(packet);
                free(packet);
        }
                
        packet__cleanup(&mosq->in_packet);
        
        pthread_mutex_unlock(&mosq->out_packet_mutex);
        pthread_mutex_unlock(&mosq->current_out_packet_mutex);
}

int packet__read(struct mosquitto *mosq)
{
	uint8_t byte;
	ssize_t read_length;
	int rc = 0;
	int state;

	if(!mosq){
		return MOSQ_ERR_INVAL;
	}
	if(mosq->sock == INVALID_SOCKET){
		return MOSQ_ERR_NO_CONN;
	}

	state = mosquitto__get_state(mosq);
	if(state == mosq_cs_connect_pending){
		return MOSQ_ERR_SUCCESS;
	}

	/* This gets called if pselect() indicates that there is network data
	 * available - ie. at least one byte.  What we do depends on what data we
	 * already have.
	 * If we've not got a command, attempt to read one and save it. This should
	 * always work because it's only a single byte.
	 * Then try to read the remaining length. This may fail because it is may
	 * be more than one byte - will need to save data pending next read if it
	 * does fail.
	 * Then try to read the remaining payload, where 'payload' here means the
	 * combined variable header and actual payload. This is the most likely to
	 * fail due to longer length, so save current data and current position.
	 * After all data is read, send to mosquitto__handle_packet() to deal with.
	 * Finally, free the memory and reset everything to starting conditions.
	 */
	if(!mosq->in_packet.command){
		read_length = net__read(mosq, &byte, 1);
		if(read_length == 1){
			mosq->in_packet.command = byte;
// #ifdef WITH_BROKER
// 			G_BYTES_RECEIVED_INC(1);
// 			/* Clients must send CONNECT as their first command. */
// 			if(!(mosq->bridge) && mosq->state == mosq_cs_connected && (byte&0xF0) != CMD_CONNECT){
// 				return MOSQ_ERR_PROTOCOL;
// 			}
// #endif
		}else{
			if(read_length == 0){
				return MOSQ_ERR_CONN_LOST; /* EOF */
			}
// #ifdef WIN32
// 			errno = WSAGetLastError();
// #endif
			if(errno == EAGAIN || errno == EWOULDBLOCK){
				return MOSQ_ERR_SUCCESS;
			}else{
				switch(errno){
					case ECONNRESET:
						return MOSQ_ERR_CONN_LOST;
					default:
						return MOSQ_ERR_ERRNO;
				}
			}
		}
	}
	/* remaining_count is the number of bytes that the remaining_length
	 * parameter occupied in this incoming packet. We don't use it here as such
	 * (it is used when allocating an outgoing packet), but we must be able to
	 * determine whether all of the remaining_length parameter has been read.
	 * remaining_count has three states here:
	 *   0 means that we haven't read any remaining_length bytes
	 *   <0 means we have read some remaining_length bytes but haven't finished
	 *   >0 means we have finished reading the remaining_length bytes.
	 */
	if(mosq->in_packet.remaining_count <= 0){
		do{
			read_length = net__read(mosq, &byte, 1);
			if(read_length == 1){
				mosq->in_packet.remaining_count--;
				/* Max 4 bytes length for remaining length as defined by protocol.
				 * Anything more likely means a broken/malicious client.
				 */
				if(mosq->in_packet.remaining_count < -4){
					return MOSQ_ERR_PROTOCOL;
				}

				G_BYTES_RECEIVED_INC(1);
				mosq->in_packet.remaining_length += (byte & 127) * mosq->in_packet.remaining_mult;
				mosq->in_packet.remaining_mult *= 128;
			}else{
				if(read_length == 0){
					return MOSQ_ERR_CONN_LOST; /* EOF */
				}
// #ifdef WIN32
// 				errno = WSAGetLastError();
// #endif
				if(errno == EAGAIN || errno == EWOULDBLOCK){
					return MOSQ_ERR_SUCCESS;
				}else{
					switch(errno){
						case ECONNRESET:
							return MOSQ_ERR_CONN_LOST;
						default:
							return MOSQ_ERR_ERRNO;
					}
				}
			}
		}while((byte & 128) != 0);
		/* We have finished reading remaining_length, so make remaining_count
		 * positive. */
		mosq->in_packet.remaining_count *= -1;

// #ifdef WITH_BROKER
// 		if(db->config->max_packet_size > 0 && mosq->in_packet.remaining_length+1 > db->config->max_packet_size){
// 			log__printf(NULL, MOSQ_LOG_INFO, "Client %s sent too large packet %d, disconnecting.", mosq->id, mosq->in_packet.remaining_length+1);
// 			if(mosq->protocol == mosq_p_mqtt5){
// 				send__disconnect(mosq, MQTT_RC_PACKET_TOO_LARGE, NULL);
// 			}
// 			return MOSQ_ERR_OVERSIZE_PACKET;
// 		}
// #else
		// FIXME - client case for incoming message received from broker too large
// #endif
		if(mosq->in_packet.remaining_length > 0){
			mosq->in_packet.payload = (uint8_t*)malloc(mosq->in_packet.remaining_length*sizeof(uint8_t));
			if(!mosq->in_packet.payload){
				return MOSQ_ERR_NOMEM;
			}
			mosq->in_packet.to_process = mosq->in_packet.remaining_length;
		}
	}
	while(mosq->in_packet.to_process>0){
		read_length = net__read(mosq, &(mosq->in_packet.payload[mosq->in_packet.pos]), mosq->in_packet.to_process);
		if(read_length > 0){
			G_BYTES_RECEIVED_INC(read_length);
			mosq->in_packet.to_process -= read_length;
			mosq->in_packet.pos += read_length;
		}else{
// #ifdef WIN32
// 			errno = WSAGetLastError();
// #endif
			if(errno == EAGAIN || errno == EWOULDBLOCK){
				if(mosq->in_packet.to_process > 1000){
					/* Update last_msg_in time if more than 1000 bytes left to
					 * receive. Helps when receiving large messages.
					 * This is an arbitrary limit, but with some consideration.
					 * If a client can't send 1000 bytes in a second it
					 * probably shouldn't be using a 1 second keep alive. */
					pthread_mutex_lock(&mosq->msgtime_mutex);
					mosq->last_msg_in = mosquitto_time();
					pthread_mutex_unlock(&mosq->msgtime_mutex);
				}
				return MOSQ_ERR_SUCCESS;
			}else{
				switch(errno){
					case ECONNRESET:
						return MOSQ_ERR_CONN_LOST;
					default:
						return MOSQ_ERR_ERRNO;
				}
			}
		}
	}

	/* All data for this packet is read. */
	mosq->in_packet.pos = 0;
// #ifdef WITH_BROKER
// 	G_MSGS_RECEIVED_INC(1);
// 	if(((mosq->in_packet.command)&0xF5) == CMD_PUBLISH){
// 		G_PUB_MSGS_RECEIVED_INC(1);
// 	}
// 	rc = handle__packet(db, mosq);
// #else
	rc = handle__packet(mosq);
// #endif

	/* Free data and reset values */
	packet__cleanup(&mosq->in_packet);

	pthread_mutex_lock(&mosq->msgtime_mutex);
	mosq->last_msg_in = mosquitto_time();
	pthread_mutex_unlock(&mosq->msgtime_mutex);
	return rc;
}

///////////////////////////////packet_datatypes.c///////////////////////////////////


int packet__varint_bytes(int32_t word)
{
        if(word < 128){
                return 1;
        }else if(word < 16384){
                return 2;
        }else if(word < 2097152){
                return 3;
        }else if(word < 268435456){
                return 4;
        }else{
                return 5;
        }
} 

void packet__write_string(struct mosquitto__packet *packet, const char *str, uint16_t length)
{       
        assert(packet);
        packet__write_uint16(packet, length);
        packet__write_bytes(packet, str, length);
}  

void packet__write_uint16(struct mosquitto__packet *packet, uint16_t word)
{
        packet__write_byte(packet, MOSQ_MSB(word));
        packet__write_byte(packet, MOSQ_LSB(word));
}

void packet__write_uint32(struct mosquitto__packet *packet, uint32_t word)
{                       
        packet__write_byte(packet, (word & 0xFF000000) >> 24);
        packet__write_byte(packet, (word & 0x00FF0000) >> 16);
        packet__write_byte(packet, (word & 0x0000FF00) >> 8);
        packet__write_byte(packet, (word & 0x000000FF));
}  

void packet__write_byte(struct mosquitto__packet *packet, uint8_t byte)
{
        assert(packet);
        assert(packet->pos+1 <= packet->packet_length);

        packet->payload[packet->pos] = byte;
        packet->pos++;
}

int packet__write_varint(struct mosquitto__packet *packet, int32_t word)
{       
        uint8_t byte;
        int count = 0;
                
        do{
                byte = word % 128;
                word = word / 128;
                /* If there are more digits to encode, set the top bit of this digit */
                if(word > 0){
                        byte = byte | 0x80;
                }
                packet__write_byte(packet, byte);
                count++;
        }while(word > 0 && count < 5);
        
        if(count == 5){
                return MOSQ_ERR_PROTOCOL;
        }
        return MOSQ_ERR_SUCCESS;
}  

void packet__write_bytes(struct mosquitto__packet *packet, const void *bytes, uint32_t count)
{
        assert(packet);
        assert(packet->pos+count <= packet->packet_length);

        memcpy(&(packet->payload[packet->pos]), bytes, count);
        packet->pos += count;
}

int packet__read_string(struct mosquitto__packet *packet, char **str, int *length)
{       
        int rc;
        
        rc = packet__read_binary(packet, (uint8_t **)str, length);
        if(rc) return rc;
        if(*length == 0) return MOSQ_ERR_SUCCESS;
        
        if(mosquitto_validate_utf8(*str, *length)){
                free(*str);
                *str = NULL;
                *length = -1;
                return MOSQ_ERR_MALFORMED_UTF8;
        }
                                
        return MOSQ_ERR_SUCCESS;
}

int packet__read_binary(struct mosquitto__packet *packet, uint8_t **data, int *length)
{       
        uint16_t slen;
        int rc;
                
        assert(packet);
        rc = packet__read_uint16(packet, &slen);
        if(rc) return rc;

        if(slen == 0){
                *data = NULL;
                *length = 0;
                return MOSQ_ERR_SUCCESS;
        }
        
        if(packet->pos+slen > packet->remaining_length) return MOSQ_ERR_PROTOCOL;

        *data = (uint8_t*)malloc(slen+1);
        if(*data){
                memcpy(*data, &(packet->payload[packet->pos]), slen);
                ((uint8_t *)(*data))[slen] = '\0';
                packet->pos += slen;
        }else{
                return MOSQ_ERR_NOMEM;
        }

        *length = slen;
        return MOSQ_ERR_SUCCESS;
}

int packet__read_uint16(struct mosquitto__packet *packet, uint16_t *word)
{       
        uint8_t msb, lsb;
                
        assert(packet);
        if(packet->pos+2 > packet->remaining_length) return MOSQ_ERR_PROTOCOL;
                
        msb = packet->payload[packet->pos];
        packet->pos++;
        lsb = packet->payload[packet->pos];
        packet->pos++;
                
        *word = (msb<<8) + lsb;
                                
        return MOSQ_ERR_SUCCESS;
}   

int packet__read_bytes(struct mosquitto__packet *packet, void *bytes, uint32_t count)
{
        assert(packet);
        if(packet->pos+count > packet->remaining_length) return MOSQ_ERR_PROTOCOL;

        memcpy(bytes, &(packet->payload[packet->pos]), count);
        packet->pos += count;

        return MOSQ_ERR_SUCCESS;
}

int packet__read_byte(struct mosquitto__packet *packet, uint8_t *byte)
{
	assert(packet);
	if(packet->pos+1 > packet->remaining_length) return MOSQ_ERR_PROTOCOL;

	*byte = packet->payload[packet->pos];
	packet->pos++;

	return MOSQ_ERR_SUCCESS;
}

int packet__read_varint(struct mosquitto__packet *packet, int32_t *word, int8_t *bytes)
{
	int i;
	uint8_t byte;
	int remaining_mult = 1;
	int32_t lword = 0;
	uint8_t lbytes = 0;

	for(i=0; i<4; i++){
		if(packet->pos < packet->remaining_length){
			lbytes++;
			byte = packet->payload[packet->pos];
			lword += (byte & 127) * remaining_mult;
			remaining_mult *= 128;
			packet->pos++;
			if((byte & 128) == 0){
				if(lbytes > 1 && byte == 0){
					/* Catch overlong encodings */
					return MOSQ_ERR_PROTOCOL;
				}else{
					*word = lword;
					if(bytes) (*bytes) = lbytes;
					return MOSQ_ERR_SUCCESS;
				}
			}
		}else{
			return MOSQ_ERR_PROTOCOL;
		}
	}
	return MOSQ_ERR_PROTOCOL;
}

int packet__read_uint32(struct mosquitto__packet *packet, uint32_t *word)
{
	uint32_t val = 0;
	int i;

	assert(packet);
	if(packet->pos+4 > packet->remaining_length) return MOSQ_ERR_PROTOCOL;

	for(i=0; i<4; i++){
		val = (val << 8) + packet->payload[packet->pos];
		packet->pos++;
	}

	*word = val;

	return MOSQ_ERR_SUCCESS;
}

///////////////////////////////send_publish.c///////////////////////////////////


int send__publish(struct mosquitto *mosq, uint16_t mid, const char *topic, uint32_t payloadlen, const void *payload, int qos, bool retain, bool dup, const mosquitto_property *cmsg_props, const mosquitto_property *store_props, uint32_t expiry_interval)
{
// #ifdef WITH_BROKER
// 	size_t len;
// #ifdef WITH_BRIDGE
// 	int i;
// 	struct mosquitto__bridge_topic *cur_topic;
// 	bool match;
// 	int rc;
// 	char *mapped_topic = NULL;
// 	char *topic_temp = NULL;
// #endif
// #endif
	assert(mosq);

// #if defined(WITH_BROKER) && defined(WITH_WEBSOCKETS)
// 	if(mosq->sock == INVALID_SOCKET && !mosq->wsi) return MOSQ_ERR_NO_CONN;
// #else
	if(mosq->sock == INVALID_SOCKET) return MOSQ_ERR_NO_CONN;
// #endif

// #ifdef WITH_BROKER
// 	if(mosq->listener && mosq->listener->mount_point){
// 		len = strlen(mosq->listener->mount_point);
// 		if(len < strlen(topic)){
// 			topic += len;
// 		}else{
// 			/* Invalid topic string. Should never happen, but silently swallow the message anyway. */
// 			return MOSQ_ERR_SUCCESS;
// 		}
// 	}
// #ifdef WITH_BRIDGE
// 	if(mosq->bridge && mosq->bridge->topics && mosq->bridge->topic_remapping){
// 		for(i=0; i<mosq->bridge->topic_count; i++){
// 			cur_topic = &mosq->bridge->topics[i];
// 			if((cur_topic->direction == bd_both || cur_topic->direction == bd_out)
// 					&& (cur_topic->remote_prefix || cur_topic->local_prefix)){
// 				/* Topic mapping required on this topic if the message matches */

// 				rc = mosquitto_topic_matches_sub(cur_topic->local_topic, topic, &match);
// 				if(rc){
// 					return rc;
// 				}
// 				if(match){
// 					mapped_topic = strdup(topic);
// 					if(!mapped_topic) return MOSQ_ERR_NOMEM;
// 					if(cur_topic->local_prefix){
// 						/* This prefix needs removing. */
// 						if(!strncmp(cur_topic->local_prefix, mapped_topic, strlen(cur_topic->local_prefix))){
// 							topic_temp = strdup(mapped_topic+strlen(cur_topic->local_prefix));
// 							free(mapped_topic);
// 							if(!topic_temp){
// 								return MOSQ_ERR_NOMEM;
// 							}
// 							mapped_topic = topic_temp;
// 						}
// 					}

// 					if(cur_topic->remote_prefix){
// 						/* This prefix needs adding. */
// 						len = strlen(mapped_topic) + strlen(cur_topic->remote_prefix)+1;
// 						topic_temp = malloc(len+1);
// 						if(!topic_temp){
// 							free(mapped_topic);
// 							return MOSQ_ERR_NOMEM;
// 						}
// 						snprintf(topic_temp, len, "%s%s", cur_topic->remote_prefix, mapped_topic);
// 						topic_temp[len] = '\0';
// 						free(mapped_topic);
// 						mapped_topic = topic_temp;
// 					}
					// log__printf(NULL, MOSQ_LOG_DEBUG, "Sending PUBLISH to %s (d%d, q%d, r%d, m%d, '%s', ... (%ld bytes))", mosq->id, dup, qos, retain, mid, mapped_topic, (long)payloadlen);
// 					G_PUB_BYTES_SENT_INC(payloadlen);
// 					rc =  send__real_publish(mosq, mid, mapped_topic, payloadlen, payload, qos, retain, dup, cmsg_props, store_props, expiry_interval);
// 					free(mapped_topic);
// 					return rc;
// 				}
// 			}
// 		}
// 	}
// #endif
// 	log__printf(NULL, MOSQ_LOG_DEBUG, "Sending PUBLISH to %s (d%d, q%d, r%d, m%d, '%s', ... (%ld bytes))", mosq->id, dup, qos, retain, mid, topic, (long)payloadlen);
// 	G_PUB_BYTES_SENT_INC(payloadlen);
// #else
	log__printf(mosq, MOSQ_LOG_DEBUG, "Client %s sending PUBLISH (d%d, q%d, r%d, m%d, '%s', ... (%ld bytes))", mosq->id, dup, qos, retain, mid, topic, (long)payloadlen);
// #endif

	return send__real_publish(mosq, mid, topic, payloadlen, payload, qos, retain, dup, cmsg_props, store_props, expiry_interval);
}


int send__real_publish(struct mosquitto *mosq, uint16_t mid, const char *topic, uint32_t payloadlen, const void *payload, int qos, bool retain, bool dup, const mosquitto_property *cmsg_props, const mosquitto_property *store_props, uint32_t expiry_interval)
{
	struct mosquitto__packet *packet = NULL;
	int packetlen;
	int proplen = 0, varbytes;
	int rc;
	mosquitto_property expiry_prop;

	assert(mosq);

	if(topic){
		packetlen = 2+strlen(topic) + payloadlen;
	}else{
		packetlen = 2 + payloadlen;
	}
	if(qos > 0) packetlen += 2; /* For message id */
	if(mosq->protocol == mosq_p_mqtt5){
		proplen = 0;
		proplen += property__get_length_all(cmsg_props);
		proplen += property__get_length_all(store_props);
		if(expiry_interval > 0){
			expiry_prop.next = NULL;
			expiry_prop.value.i32 = expiry_interval;
			expiry_prop.identifier = MQTT_PROP_MESSAGE_EXPIRY_INTERVAL;
			expiry_prop.client_generated = false;

			proplen += property__get_length_all(&expiry_prop);
		}

		varbytes = packet__varint_bytes(proplen);
		if(varbytes > 4){
			/* FIXME - Properties too big, don't publish any - should remove some first really */
			cmsg_props = NULL;
			store_props = NULL;
			expiry_interval = 0;
		}else{
			packetlen += proplen + varbytes;
		}
	}
	if(packet__check_oversize(mosq, packetlen)){
#ifdef WITH_BROKER
		log__printf(NULL, MOSQ_LOG_NOTICE, "Dropping too large outgoing PUBLISH for %s (%d bytes)", mosq->id, packetlen);
#else
		log__printf(NULL, MOSQ_LOG_NOTICE, "Dropping too large outgoing PUBLISH (%d bytes)", packetlen);
#endif
		return MOSQ_ERR_OVERSIZE_PACKET;
	}

	packet = (struct mosquitto__packet*)calloc(1, sizeof(struct mosquitto__packet));
	if(!packet) return MOSQ_ERR_NOMEM;

	packet->mid = mid;
	packet->command = CMD_PUBLISH | ((dup&0x1)<<3) | (qos<<1) | retain;
	packet->remaining_length = packetlen;
	rc = packet__alloc(packet);
	if(rc){
		free(packet);
		return rc;
	}
	/* Variable header (topic string) */
	if(topic){
		packet__write_string(packet, topic, strlen(topic));
	}else{
		packet__write_uint16(packet, 0);
	}
	if(qos > 0){
		packet__write_uint16(packet, mid);
	}

	if(mosq->protocol == mosq_p_mqtt5){
		packet__write_varint(packet, proplen);
		property__write_all(packet, cmsg_props, false);
		property__write_all(packet, store_props, false);
		if(expiry_interval > 0){
			property__write_all(packet, &expiry_prop, false);
		}
	}

	/* Payload */
	if(payloadlen){
		packet__write_bytes(packet, payload, payloadlen);
	}

	return packet__queue(mosq, packet);
}


///////////////////////////////log_printf.c///////////////////////////////////


int log__printf(struct mosquitto *mosq, int priority, const char *fmt, ...)
{                                       
        va_list va;
        char *s;                        
        int len;                                
                                                
        assert(mosq);
        assert(fmt);                            
                                                
        pthread_mutex_lock(&mosq->log_callback_mutex);  
        if(mosq->on_log){                               
                len = strlen(fmt) + 500;        
                s = (char*)malloc(len*sizeof(char));
                if(!s){
                        pthread_mutex_unlock(&mosq->log_callback_mutex);
                        return MOSQ_ERR_NOMEM;  
                }                               
                                        
                va_start(va, fmt);      
                vsnprintf(s, len, fmt, va);
                va_end(va);
                s[len-1] = '\0'; /* Ensure string is null terminated. */
                                        
                mosq->on_log(mosq, mosq->userdata, priority, s);

                free(s);
        }
        pthread_mutex_unlock(&mosq->log_callback_mutex);

        return MOSQ_ERR_SUCCESS;
}


///////////////////////////////time_mosq.c///////////////////////////////////


time_t mosquitto_time(void)
{               
// #ifdef WIN32    
//         return GetTickCount64()/1000;
// #elif _POSIX_TIMERS>0 && defined(_POSIX_MONOTONIC_CLOCK)
        struct timespec tp;
                                
        clock_gettime(CLOCK_MONOTONIC, &tp);
        return tp.tv_sec;       
// #elif defined(__APPLE__)
//         static mach_timebase_info_data_t tb;
//     uint64_t ticks;
//         uint64_t sec;   
                        
//         ticks = mach_absolute_time();
                                
//         if(tb.denom == 0){      
//                 mach_timebase_info(&tb);
//         }
//         sec = ticks*tb.numer/tb.denom/1000000000;

//         return (time_t)sec;
// #else
//         return time(NULL);
// #endif
}


///////////////////////////////messages_mosq.c///////////////////////////////////

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

int message__delete(struct mosquitto *mosq, uint16_t mid, enum mosquitto_msg_direction dir, int qos)
{
	struct mosquitto_message_all *message;
	int rc;
	assert(mosq);

	rc = message__remove(mosq, mid, dir, &message, qos);
	if(rc == MOSQ_ERR_SUCCESS){
		message__cleanup(&message);
	}
	return rc;
}

int message__out_update(struct mosquitto *mosq, uint16_t mid, enum mosquitto_msg_state state, int qos)
{
	struct mosquitto_message_all *message, *tmp;
	assert(mosq);

	pthread_mutex_lock(&mosq->msgs_out.mutex);
	DL_FOREACH_SAFE(mosq->msgs_out.inflight, message, tmp){
		if(message->msg.mid == mid){
			if(message->msg.qos != qos){
				pthread_mutex_unlock(&mosq->msgs_out.mutex);
				return MOSQ_ERR_PROTOCOL;
			}
			message->state = state;
			message->timestamp = mosquitto_time();
			pthread_mutex_unlock(&mosq->msgs_out.mutex);
			return MOSQ_ERR_SUCCESS;
		}
	}
	pthread_mutex_unlock(&mosq->msgs_out.mutex);
	return MOSQ_ERR_NOT_FOUND;
}

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

///////////////////////////////net_mosq.c///////////////////////////////////


ssize_t net__write(struct mosquitto *mosq, void *buf, size_t count)
{                       
// #ifdef WITH_TLS                 
//         int ret;                
//         int err;                
// #endif                  
    assert(mosq);               
    errno = 0;
// #ifdef WITH_TLS                 
//         if(mosq->ssl){
//                 mosq->want_write = false;       
//                 ret = SSL_write(mosq->ssl, buf, count);
//                 if(ret < 0){                    
//                         err = SSL_get_error(mosq->ssl, ret);
//                         if(err == SSL_ERROR_WANT_READ){
//                                 ret = -1;
//                                 errno = EAGAIN;
//                         }else if(err == SSL_ERROR_WANT_WRITE){
//                                 ret = -1;
//                                 mosq->want_write = true;
//                                 errno = EAGAIN; 
//                         }else{                          
//                                 net__print_ssl_error(mosq);
//                                 errno = EPROTO;         
//                         }               
//                         ERR_clear_error();
// #ifdef WIN32
//                         WSASetLastError(errno);
// #endif
//                 }
//                 return (ssize_t )ret;
//         }else{
//                 /* Call normal write/send */
// #endif

// #ifndef WIN32
//         return write(mosq->sock, buf, count);
// #else
        return send(mosq->sock, buf, count, 0);
// #endif

// #ifdef WITH_TLS
        // }
// #endif
}

ssize_t net__read(struct mosquitto *mosq, void *buf, size_t count)
{
// #ifdef WITH_TLS
// 	int ret;
// 	int err;
// #endif
	assert(mosq);
	errno = 0;
// #ifdef WITH_TLS
// 	if(mosq->ssl){
// 		ret = SSL_read(mosq->ssl, buf, count);
// 		if(ret <= 0){
// 			err = SSL_get_error(mosq->ssl, ret);
// 			if(err == SSL_ERROR_WANT_READ){
// 				ret = -1;
// 				errno = EAGAIN;
// 			}else if(err == SSL_ERROR_WANT_WRITE){
// 				ret = -1;
// 				mosq->want_write = true;
// 				errno = EAGAIN;
// 			}else{
// 				net__print_ssl_error(mosq);
// 				errno = EPROTO;
// 			}
// 			ERR_clear_error();
// #ifdef WIN32
// 			WSASetLastError(errno);
// #endif
// 		}
// 		return (ssize_t )ret;
// 	}else{
// 		/* Call normal read/recv */

// #endif

// #ifndef WIN32
	return read(mosq->sock, buf, count);
// #else
// 	return recv(mosq->sock, buf, count, 0);
// #endif

// #ifdef WITH_TLS
// 	}
// #endif
}

int net__socket_close(struct mosquitto *mosq)  
{
        int rc = 0;

        assert(mosq);
// #ifdef WITH_TLS
// #ifdef WITH_WEBSOCKETS
//         if(!mosq->wsi)
// #endif
//         {
//                 if(mosq->ssl){
//                         if(!SSL_in_init(mosq->ssl)){
//                                 SSL_shutdown(mosq->ssl);
//                         }
//                         SSL_free(mosq->ssl);
//                         mosq->ssl = NULL;
//                 }
//         }
// #endif

// #ifdef WITH_WEBSOCKETS
//         if(mosq->wsi) 
//         {
//                 if(mosq->state != mosq_cs_disconnecting){
//                         mosquitto__set_state(mosq, mosq_cs_disconnect_ws);
//                 }
//                 libwebsocket_callback_on_writable(mosq->ws_context, mosq->wsi);
//         }else
// #endif
        {
                if(mosq->sock != INVALID_SOCKET){
// #ifdef WITH_BROKER
//                         // HASH_DELETE(hh_sock, db->contexts_by_sock, mosq);
// #endif
                        rc = close(mosq->sock);
                        mosq->sock = INVALID_SOCKET;
                }
        }

// #ifdef WITH_BROKER
//         if(mosq->listener){
//                 mosq->listener->client_count--;
//         }
// #endif

        return rc;
}

int net__socketpair(mosq_sock_t *pairR, mosq_sock_t *pairW)
{
// #ifdef WIN32
// 	int family[2] = {AF_INET, AF_INET6};
// 	int i;
// 	struct sockaddr_storage ss;
// 	struct sockaddr_in *sa = (struct sockaddr_in *)&ss;
// 	struct sockaddr_in6 *sa6 = (struct sockaddr_in6 *)&ss;
// 	socklen_t ss_len;
// 	mosq_sock_t spR, spW;

// 	mosq_sock_t listensock;

// 	*pairR = INVALID_SOCKET;
// 	*pairW = INVALID_SOCKET;

// 	for(i=0; i<2; i++){
// 		memset(&ss, 0, sizeof(ss));
// 		if(family[i] == AF_INET){
// 			sa->sin_family = family[i];
// 			sa->sin_addr.s_addr = htonl(INADDR_LOOPBACK);
// 			sa->sin_port = 0;
// 			ss_len = sizeof(struct sockaddr_in);
// 		}else if(family[i] == AF_INET6){
// 			sa6->sin6_family = family[i];
// 			sa6->sin6_addr = in6addr_loopback;
// 			sa6->sin6_port = 0;
// 			ss_len = sizeof(struct sockaddr_in6);
// 		}else{
// 			return MOSQ_ERR_INVAL;
// 		}

// 		listensock = socket(family[i], SOCK_STREAM, IPPROTO_TCP);
// 		if(listensock == -1){
// 			continue;
// 		}

// 		if(bind(listensock, (struct sockaddr *)&ss, ss_len) == -1){
// 			close(listensock);
// 			continue;
// 		}

// 		if(listen(listensock, 1) == -1){
// 			close(listensock);
// 			continue;
// 		}
// 		memset(&ss, 0, sizeof(ss));
// 		ss_len = sizeof(ss);
// 		if(getsockname(listensock, (struct sockaddr *)&ss, &ss_len) < 0){
// 			close(listensock);
// 			continue;
// 		}

// 		if(family[i] == AF_INET){
// 			sa->sin_family = family[i];
// 			sa->sin_addr.s_addr = htonl(INADDR_LOOPBACK);
// 			ss_len = sizeof(struct sockaddr_in);
// 		}else if(family[i] == AF_INET6){
// 			sa6->sin6_family = family[i];
// 			sa6->sin6_addr = in6addr_loopback;
// 			ss_len = sizeof(struct sockaddr_in6);
// 		}

// 		spR = socket(family[i], SOCK_STREAM, IPPROTO_TCP);
// 		if(spR == -1){
// 			close(listensock);
// 			continue;
// 		}
// 		if(net__socket_nonblock(&spR)){
// 			close(listensock);
// 			continue;
// 		}
// 		if(connect(spR, (struct sockaddr *)&ss, ss_len) < 0){
// // #ifdef WIN32
// // 			errno = WSAGetLastError();
// // #endif
// 			if(errno != EINPROGRESS && errno != EWOULDBLOCK){
// 				close(spR);
// 				close(listensock);
// 				continue;
// 			}
// 		}
// 		spW = accept(listensock, NULL, 0);
// 		if(spW == -1){
// // #ifdef WIN32
// // 			errno = WSAGetLastError();
// // #endif
// 			if(errno != EINPROGRESS && errno != EWOULDBLOCK){
// 				close(spR);
// 				close(listensock);
// 				continue;
// 			}
// 		}

// 		if(net__socket_nonblock(&spW)){
// 			close(spR);
// 			close(listensock);
// 			continue;
// 		}
// 		close(listensock);

// 		*pairR = spR;
// 		*pairW = spW;
// 		return MOSQ_ERR_SUCCESS;
// 	}
// 	return MOSQ_ERR_UNKNOWN;
// #else
	int sv[2];

	if(socketpair(AF_UNIX, SOCK_STREAM, 0, sv) == -1){
		return MOSQ_ERR_ERRNO;
	}
	if(net__socket_nonblock(&sv[0])){
		close(sv[1]);
		return MOSQ_ERR_ERRNO;
	}
	if(net__socket_nonblock(&sv[1])){
		close(sv[0]);
		return MOSQ_ERR_ERRNO;
	}
	*pairR = sv[0];
	*pairW = sv[1];
	return MOSQ_ERR_SUCCESS;
// #endif
}

int net__socket_nonblock(mosq_sock_t *sock)
{
// #ifndef WIN32
        int opt;
        /* Set non-blocking */
        opt = fcntl(*sock, F_GETFL, 0);
        if(opt == -1){
                close(*sock);
                *sock = INVALID_SOCKET;
                return MOSQ_ERR_ERRNO;
        }
        if(fcntl(*sock, F_SETFL, opt | O_NONBLOCK) == -1){
                /* If either fcntl fails, don't want to allow this client to connect. */
                close(*sock);
                *sock = INVALID_SOCKET;
                return MOSQ_ERR_ERRNO;
        }
// #else
        // unsigned long opt = 1;
        // if(ioctlsocket(*sock, FIONBIO, &opt)){
        //         close(*sock);
        //         *sock = INVALID_SOCKET;
        //         return MOSQ_ERR_ERRNO;
        // }
// #endif
        return MOSQ_ERR_SUCCESS;
}

/* Create a socket and connect it to 'ip' on port 'port'.  */
int net__socket_connect(struct mosquitto *mosq, const char *host, uint16_t port, const char *bind_address, bool blocking)
{
        mosq_sock_t sock = INVALID_SOCKET;
        int rc, rc2;
                
        if(!mosq || !host || !port) return MOSQ_ERR_INVAL;

        rc = net__try_connect(host, port, &sock, bind_address, blocking);
        if(rc > 0) return rc;
         
        mosq->sock = sock;
         
// #if defined(WITH_SOCKS) && !defined(WITH_BROKER)
//         if(!mosq->socks5_host)
// #endif       
//         {
//                 rc2 = net__socket_connect_step3(mosq, host);
//                 if(rc2) return rc2;
//         }

        return MOSQ_ERR_SUCCESS;
}   

int net__try_connect(const char *host, uint16_t port, mosq_sock_t *sock, const char *bind_address, bool blocking)
{
	struct addrinfo hints;
	struct addrinfo *ainfo, *rp;
	struct addrinfo *ainfo_bind, *rp_bind;
	int s;
	int rc = MOSQ_ERR_SUCCESS;
// #ifdef WIN32
// 	uint32_t val = 1;
// #endif

	*sock = INVALID_SOCKET;
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	s = getaddrinfo(host, NULL, &hints, &ainfo);
	if(s){
		errno = s;
		return MOSQ_ERR_EAI;
	}

	if(bind_address){
		s = getaddrinfo(bind_address, NULL, &hints, &ainfo_bind);
		if(s){
			freeaddrinfo(ainfo);
			errno = s;
			return MOSQ_ERR_EAI;
		}
	}

	for(rp = ainfo; rp != NULL; rp = rp->ai_next){
		*sock = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		if(*sock == INVALID_SOCKET) continue;

		if(rp->ai_family == AF_INET){
			((struct sockaddr_in *)rp->ai_addr)->sin_port = htons(port);
		}else if(rp->ai_family == AF_INET6){
			((struct sockaddr_in6 *)rp->ai_addr)->sin6_port = htons(port);
		}else{
			close(*sock);
			*sock = INVALID_SOCKET;
			continue;
		}

		if(bind_address){
			for(rp_bind = ainfo_bind; rp_bind != NULL; rp_bind = rp_bind->ai_next){
				if(bind(*sock, rp_bind->ai_addr, rp_bind->ai_addrlen) == 0){
					break;
				}
			}
			if(!rp_bind){
				close(*sock);
				*sock = INVALID_SOCKET;
				continue;
			}
		}

		if(!blocking){
			/* Set non-blocking */
			if(net__socket_nonblock(sock)){
				continue;
			}
		}

		rc = connect(*sock, rp->ai_addr, rp->ai_addrlen);
// #ifdef WIN32
// 		errno = WSAGetLastError();
// #endif
		if(rc == 0 || errno == EINPROGRESS || errno == EWOULDBLOCK){
			if(rc < 0 && (errno == EINPROGRESS || errno == EWOULDBLOCK)){
				rc = MOSQ_ERR_CONN_PENDING;
			}

			if(blocking){
				/* Set non-blocking */
				if(net__socket_nonblock(sock)){
					continue;
				}
			}
			break;
		}

		close(*sock);
		*sock = INVALID_SOCKET;
	}
	freeaddrinfo(ainfo);
	if(bind_address){
		freeaddrinfo(ainfo_bind);
	}
	if(!rp){
		return MOSQ_ERR_ERRNO;
	}
	return rc;
}

// int net__socket_connect_step3(struct mosquitto *mosq, const char *host)
// {       
// #ifdef WITH_TLS 
//         BIO *bio;

//         int rc = net__init_ssl_ctx(mosq);
//         if(rc) return rc;

//         if(mosq->ssl_ctx){
//                 if(mosq->ssl){
//                         SSL_free(mosq->ssl);
//                 }
//                 mosq->ssl = SSL_new(mosq->ssl_ctx);
//                 if(!mosq->ssl){
//                         COMPAT_CLOSE(mosq->sock);
//                         mosq->sock = INVALID_SOCKET;
//                         net__print_ssl_error(mosq);
//                         return MOSQ_ERR_TLS;
//                 }

//                 SSL_set_ex_data(mosq->ssl, tls_ex_index_mosq, mosq);
//                 bio = BIO_new_socket(mosq->sock, BIO_NOCLOSE);
//                 if(!bio){
//                         COMPAT_CLOSE(mosq->sock);
//                         mosq->sock = INVALID_SOCKET;
//                         net__print_ssl_error(mosq);
//                         return MOSQ_ERR_TLS;
//                 }
//                 SSL_set_bio(mosq->ssl, bio, bio);

//                 /*
//                  * required for the SNI resolving
//                  */
//                 if(SSL_set_tlsext_host_name(mosq->ssl, host) != 1) {
//                         COMPAT_CLOSE(mosq->sock);
//                         mosq->sock = INVALID_SOCKET;
//                         return MOSQ_ERR_TLS;
//                 }

//                 if(net__socket_connect_tls(mosq)){
//                         return MOSQ_ERR_TLS;
//                 }

//         }
// #endif
//         return MOSQ_ERR_SUCCESS;
// }


///////////////////////////////send_connect.c///////////////////////////////////


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


///////////////////////////////send_disconnect.c///////////////////////////////////


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


///////////////////////////////thread_mosq.c///////////////////////////////////


int mosquitto_loop_start(struct mosquitto *mosq)
{
// #if defined(WITH_THREADING) && defined(HAVE_PTHREAD_CANCEL)
	if(!mosq || mosq->threaded != mosq_ts_none) return MOSQ_ERR_INVAL;

	mosq->threaded = mosq_ts_self;
	if(!pthread_create(&mosq->thread_id, NULL, mosquitto__thread_main, mosq)){
		return MOSQ_ERR_SUCCESS;
	}else{
		return MOSQ_ERR_ERRNO;
	}
// #else
// 	return MOSQ_ERR_NOT_SUPPORTED;
// #endif
}

int mosquitto_loop_stop(struct mosquitto *mosq, bool force)
{
// #if defined(WITH_THREADING) && defined(HAVE_PTHREAD_CANCEL)
// #  ifndef WITH_BROKER
	char sockpair_data = 0;
// #  endif

	if(!mosq || mosq->threaded != mosq_ts_self) return MOSQ_ERR_INVAL;


	/* Write a single byte to sockpairW (connected to sockpairR) to break out
	 * of select() if in threaded mode. */
	if(mosq->sockpairW != INVALID_SOCKET){
// #ifndef WIN32
		if(write(mosq->sockpairW, &sockpair_data, 1)){
		}
// #else
// 		send(mosq->sockpairW, &sockpair_data, 1, 0);
// #endif
	}
	
	if(force){
		pthread_cancel(mosq->thread_id);
	}
	pthread_join(mosq->thread_id, NULL);
	mosq->thread_id = pthread_self();
	mosq->threaded = mosq_ts_none;

	return MOSQ_ERR_SUCCESS;
// #else
// 	return MOSQ_ERR_NOT_SUPPORTED;
// #endif
}

// #ifdef WITH_THREADING
void *mosquitto__thread_main(void *obj)
{
	struct mosquitto *mosq = (struct mosquitto*)obj;
	int state;
// #ifndef WIN32
	struct timespec ts;
	ts.tv_sec = 0;
	ts.tv_nsec = 10000000;
// #endif

	if(!mosq) return NULL;

	do{
		state = mosquitto__get_state(mosq);
		if(state == mosq_cs_new){
// #ifdef WIN32
// 			Sleep(10);
// #else
			nanosleep(&ts, NULL);
// #endif
		}else{
			break;
		}
	}while(1);

	if(!mosq->keepalive){
		/* Sleep for a day if keepalive disabled. */
		mosquitto_loop_forever(mosq, 1000*86400, 1);
	}else{
		/* Sleep for our keepalive value. publish() etc. will wake us up. */
		mosquitto_loop_forever(mosq, mosq->keepalive*1000, 1);
	}

	return obj;
}
// #endif


///////////////////////////////loop.c///////////////////////////////////


int mosquitto_loop_forever(struct mosquitto *mosq, int timeout, int max_packets)
{
	int run = 1;
	int rc;
	unsigned long reconnect_delay;
// #ifndef WIN32
	struct timespec req, rem;
// #endif
	int state;

	if(!mosq) return MOSQ_ERR_INVAL;

	mosq->reconnects = 0;

	while(run){
		do{
			rc = mosquitto_loop(mosq, timeout, max_packets);
		}while(run && rc == MOSQ_ERR_SUCCESS);
		/* Quit after fatal errors. */
		switch(rc){
			case MOSQ_ERR_NOMEM:
			case MOSQ_ERR_PROTOCOL:
			case MOSQ_ERR_INVAL:
			case MOSQ_ERR_NOT_FOUND:
			case MOSQ_ERR_TLS:
			case MOSQ_ERR_PAYLOAD_SIZE:
			case MOSQ_ERR_NOT_SUPPORTED:
			case MOSQ_ERR_AUTH:
			case MOSQ_ERR_ACL_DENIED:
			case MOSQ_ERR_UNKNOWN:
			case MOSQ_ERR_EAI:
			case MOSQ_ERR_PROXY:
				return rc;
			case MOSQ_ERR_ERRNO:
				break;
		}
		if(errno == EPROTO){
			return rc;
		}
		do{
			rc = MOSQ_ERR_SUCCESS;
			state = mosquitto__get_state(mosq);
			if(state == mosq_cs_disconnecting || state == mosq_cs_disconnected){
				run = 0;
			}else{
				if(mosq->reconnect_delay_max > mosq->reconnect_delay){
					if(mosq->reconnect_exponential_backoff){
						reconnect_delay = mosq->reconnect_delay*(mosq->reconnects+1)*(mosq->reconnects+1);
					}else{
						reconnect_delay = mosq->reconnect_delay*(mosq->reconnects+1);
					}
				}else{
					reconnect_delay = mosq->reconnect_delay;
				}

				if(reconnect_delay > mosq->reconnect_delay_max){
					reconnect_delay = mosq->reconnect_delay_max;
				}else{
					mosq->reconnects++;
				}

// #ifdef WIN32
// 				Sleep(reconnect_delay*1000);
// #else
				req.tv_sec = reconnect_delay;
				req.tv_nsec = 0;
				while(nanosleep(&req, &rem) == -1 && errno == EINTR){
					req = rem;
				}
// #endif

				state = mosquitto__get_state(mosq);
				if(state == mosq_cs_disconnecting || state == mosq_cs_disconnected){
					run = 0;
				}else{
					rc = mosquitto_reconnect(mosq);
				}
			}
		}while(run && rc != MOSQ_ERR_SUCCESS);
	}
	return rc;
}

int mosquitto_loop(struct mosquitto *mosq, int timeout, int max_packets)
{
// #ifdef HAVE_PSELECT
	struct timespec local_timeout;
// #else
	// struct timeval local_timeout;
// #endif
	fd_set readfds, writefds;
	int fdcount;
	int rc;
	char pairbuf;
	int maxfd = 0;
	time_t now;
// #ifdef WITH_SRV
	int state;
// #endif

	if(!mosq || max_packets < 1) return MOSQ_ERR_INVAL;
// #ifndef WIN32
	if(mosq->sock >= FD_SETSIZE || mosq->sockpairR >= FD_SETSIZE){
		return MOSQ_ERR_INVAL;
	}
// #endif

	FD_ZERO(&readfds);
	FD_ZERO(&writefds);
	if(mosq->sock != INVALID_SOCKET){
		maxfd = mosq->sock;
		FD_SET(mosq->sock, &readfds);
		pthread_mutex_lock(&mosq->current_out_packet_mutex);
		pthread_mutex_lock(&mosq->out_packet_mutex);
		if(mosq->out_packet || mosq->current_out_packet){
			FD_SET(mosq->sock, &writefds);
		}
// #ifdef WITH_TLS
// 		if(mosq->ssl){
// 			if(mosq->want_write){
// 				FD_SET(mosq->sock, &writefds);
// 			}else if(mosq->want_connect){
// 				/* Remove possible FD_SET from above, we don't want to check
// 				 * for writing if we are still connecting, unless want_write is
// 				 * definitely set. The presence of outgoing packets does not
// 				 * matter yet. */
// 				FD_CLR(mosq->sock, &writefds);
// 			}
// 		}
// #endif
		pthread_mutex_unlock(&mosq->out_packet_mutex);
		pthread_mutex_unlock(&mosq->current_out_packet_mutex);
	}else{
// #ifdef WITH_SRV
		// if(mosq->achan){
		// 	state = mosquitto__get_state(mosq);
		// 	if(state == mosq_cs_connect_srv){
		// 		rc = ares_fds(mosq->achan, &readfds, &writefds);
		// 		if(rc > maxfd){
		// 			maxfd = rc;
		// 		}
		// 	}else{
		// 		return MOSQ_ERR_NO_CONN;
		// 	}
		// }
// #else
// 		return MOSQ_ERR_NO_CONN;
// #endif
	}
	if(mosq->sockpairR != INVALID_SOCKET){
		/* sockpairR is used to break out of select() before the timeout, on a
		 * call to publish() etc. */
		FD_SET(mosq->sockpairR, &readfds);
		if(mosq->sockpairR > maxfd){
			maxfd = mosq->sockpairR;
		}
	}

	if(timeout < 0){
		timeout = 1000;
	}

	now = mosquitto_time();
	if(mosq->next_msg_out && now + timeout/1000 > mosq->next_msg_out){
		timeout = (mosq->next_msg_out - now)*1000;
	}

	if(timeout < 0){
		/* There has been a delay somewhere which means we should have already
		 * sent a message. */
		timeout = 0;
	}

	local_timeout.tv_sec = timeout/1000;
// #ifdef HAVE_PSELECT
	local_timeout.tv_nsec = (timeout-local_timeout.tv_sec*1000)*1e6;
// #else
	// local_timeout.tv_usec = (timeout-local_timeout.tv_sec*1000)*1000;
// #endif

// #ifdef HAVE_PSELECT
	fdcount = pselect(maxfd+1, &readfds, &writefds, NULL, &local_timeout, NULL);
// #else
	// fdcount = select(maxfd+1, &readfds, &writefds, NULL, &local_timeout);
// #endif
	if(fdcount == -1){
// #ifdef WIN32
// 		errno = WSAGetLastError();
// #endif
		if(errno == EINTR){
			return MOSQ_ERR_SUCCESS;
		}else{
			return MOSQ_ERR_ERRNO;
		}
	}else{
		if(mosq->sock != INVALID_SOCKET){
			if(FD_ISSET(mosq->sock, &readfds)){
				rc = mosquitto_loop_read(mosq, max_packets);
				if(rc || mosq->sock == INVALID_SOCKET){
					return rc;
				}
			}
			if(mosq->sockpairR != INVALID_SOCKET && FD_ISSET(mosq->sockpairR, &readfds)){
// #ifndef WIN32
				if(read(mosq->sockpairR, &pairbuf, 1) == 0){
				}
// #else
// 				recv(mosq->sockpairR, &pairbuf, 1, 0);
// #endif
				/* Fake write possible, to stimulate output write even though
				 * we didn't ask for it, because at that point the publish or
				 * other command wasn't present. */
				if(mosq->sock != INVALID_SOCKET)
					FD_SET(mosq->sock, &writefds);
			}
			if(mosq->sock != INVALID_SOCKET && FD_ISSET(mosq->sock, &writefds)){
// #ifdef WITH_TLS
// 				if(mosq->want_connect){
// 					rc = net__socket_connect_tls(mosq);
// 					if(rc) return rc;
// 				}else
// #endif
				{
					rc = mosquitto_loop_write(mosq, max_packets);
					if(rc || mosq->sock == INVALID_SOCKET){
						return rc;
					}
				}
			}
		}
// #ifdef WITH_SRV
		// if(mosq->achan){
		// 	ares_process(mosq->achan, &readfds, &writefds);
		// }
// #endif
	}
	return mosquitto_loop_misc(mosq);
}

int mosquitto_loop_misc(struct mosquitto *mosq)
{
	if(!mosq) return MOSQ_ERR_INVAL;
	if(mosq->sock == INVALID_SOCKET) return MOSQ_ERR_NO_CONN;

	return mosquitto__check_keepalive(mosq);
}

int mosquitto_loop_read(struct mosquitto *mosq, int max_packets)
{
	int rc;
	int i;
	if(max_packets < 1) return MOSQ_ERR_INVAL;

// #ifdef WITH_TLS
// 	if(mosq->want_connect){
// 		return net__socket_connect_tls(mosq);
// 	}
// #endif

	pthread_mutex_lock(&mosq->msgs_out.mutex);
	max_packets = mosq->msgs_out.queue_len;
	pthread_mutex_unlock(&mosq->msgs_out.mutex);

	pthread_mutex_lock(&mosq->msgs_in.mutex);
	max_packets += mosq->msgs_in.queue_len;
	pthread_mutex_unlock(&mosq->msgs_in.mutex);

	if(max_packets < 1) max_packets = 1;
	/* Queue len here tells us how many messages are awaiting processing and
	 * have QoS > 0. We should try to deal with that many in this loop in order
	 * to keep up. */
	for(i=0; i<max_packets || SSL_DATA_PENDING(mosq); i++){
// #ifdef WITH_SOCKS
		// if(mosq->socks5_host){
		// 	rc = socks5__read(mosq);
		// }else
// #endif
		{
			rc = packet__read(mosq);
		}
		if(rc || errno == EAGAIN || errno == EWOULDBLOCK){
			return mosquitto__loop_rc_handle(mosq, rc);
		}
	}
	return rc;
}


int mosquitto_loop_write(struct mosquitto *mosq, int max_packets)
{
	int rc;
	int i;
	if(max_packets < 1) return MOSQ_ERR_INVAL;

	pthread_mutex_lock(&mosq->msgs_out.mutex);
	max_packets = mosq->msgs_out.queue_len;
	pthread_mutex_unlock(&mosq->msgs_out.mutex);

	pthread_mutex_lock(&mosq->msgs_in.mutex);
	max_packets += mosq->msgs_in.queue_len;
	pthread_mutex_unlock(&mosq->msgs_in.mutex);

	if(max_packets < 1) max_packets = 1;
	/* Queue len here tells us how many messages are awaiting processing and
	 * have QoS > 0. We should try to deal with that many in this loop in order
	 * to keep up. */
	for(i=0; i<max_packets; i++){
		rc = packet__write(mosq);
		if(rc || errno == EAGAIN || errno == EWOULDBLOCK){
			return mosquitto__loop_rc_handle(mosq, rc);
		}
	}
	return rc;
}

static int mosquitto__loop_rc_handle(struct mosquitto *mosq, int rc)
{
	int state;

	if(rc){
		net__socket_close(mosq);
		state = mosquitto__get_state(mosq);
		if(state == mosq_cs_disconnecting || state == mosq_cs_disconnected){
			rc = MOSQ_ERR_SUCCESS;
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
	}
	return rc;
}


///////////////////////////////read_handle.c///////////////////////////////////


int handle__packet(struct mosquitto *mosq)
{
        assert(mosq);

        switch((mosq->in_packet.command)&0xF0){
                case CMD_PINGREQ:
                        return handle__pingreq(mosq);
                case CMD_PINGRESP:
                        return handle__pingresp(mosq);
                case CMD_PUBACK:
                        return handle__pubackcomp(mosq, "PUBACK");
                case CMD_PUBCOMP:
                        return handle__pubackcomp(mosq, "PUBCOMP");
                case CMD_PUBLISH:
                        return handle__publish(mosq);
                case CMD_PUBREC:
                        return handle__pubrec(NULL, mosq);
                case CMD_PUBREL:
                        return handle__pubrel(NULL, mosq);
                case CMD_CONNACK:
                        return handle__connack(mosq);
                // case CMD_SUBACK:
                //         return handle__suback(mosq);
                // case CMD_UNSUBACK:
                //         return handle__unsuback(mosq);
                // case CMD_DISCONNECT:
                //         return handle__disconnect(mosq);
                // case CMD_AUTH:
                //         return handle__auth(mosq);
                default:
                        /* If we don't recognise the command, return an error straight away. */
                        log__printf(mosq, MOSQ_LOG_ERR, "Error: Unrecognised command %d\n", (mosq->in_packet.command)&0xF0);
                        return MOSQ_ERR_PROTOCOL;
        }
}


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


///////////////////////////////send_mosq.c///////////////////////////////////

int send__pingreq(struct mosquitto *mosq)
{
	int rc;
	assert(mosq);
// #ifdef WITH_BROKER
// 	log__printf(NULL, MOSQ_LOG_DEBUG, "Sending PINGREQ to %s", mosq->id);
// #else
	log__printf(mosq, MOSQ_LOG_DEBUG, "Client %s sending PINGREQ", mosq->id);
// #endif
	rc = send__simple_command(mosq, CMD_PINGREQ);
	if(rc == MOSQ_ERR_SUCCESS){
		mosq->ping_t = mosquitto_time();
	}
	return rc;
}

int send__pingresp(struct mosquitto *mosq)
{
// #ifdef WITH_BROKER
// 	log__printf(NULL, MOSQ_LOG_DEBUG, "Sending PINGRESP to %s", mosq->id);
// #else
	log__printf(mosq, MOSQ_LOG_DEBUG, "Client %s sending PINGRESP", mosq->id);
// #endif
	return send__simple_command(mosq, CMD_PINGRESP);
}

int send__puback(struct mosquitto *mosq, uint16_t mid, uint8_t reason_code)
{
// #ifdef WITH_BROKER
// 	log__printf(NULL, MOSQ_LOG_DEBUG, "Sending PUBACK to %s (m%d, rc%d)", mosq->id, mid, reason_code);
// #else
	log__printf(mosq, MOSQ_LOG_DEBUG, "Client %s sending PUBACK (m%d, rc%d)", mosq->id, mid, reason_code);
// #endif
	util__increment_receive_quota(mosq);
	/* We don't use Reason String or User Property yet. */
	return send__command_with_mid(mosq, CMD_PUBACK, mid, false, reason_code, NULL);
}

int send__pubcomp(struct mosquitto *mosq, uint16_t mid)
{
// #ifdef WITH_BROKER
// 	log__printf(NULL, MOSQ_LOG_DEBUG, "Sending PUBCOMP to %s (m%d)", mosq->id, mid);
// #else
	log__printf(mosq, MOSQ_LOG_DEBUG, "Client %s sending PUBCOMP (m%d)", mosq->id, mid);
// #endif
	util__increment_receive_quota(mosq);
	/* We don't use Reason String or User Property yet. */
	return send__command_with_mid(mosq, CMD_PUBCOMP, mid, false, 0, NULL);
}


int send__pubrec(struct mosquitto *mosq, uint16_t mid, uint8_t reason_code)
{
// #ifdef WITH_BROKER
// 	log__printf(NULL, MOSQ_LOG_DEBUG, "Sending PUBREC to %s (m%d, rc%d)", mosq->id, mid, reason_code);
// #else
	log__printf(mosq, MOSQ_LOG_DEBUG, "Client %s sending PUBREC (m%d, rc%d)", mosq->id, mid, reason_code);
// #endif
	if(reason_code >= 0x80 && mosq->protocol == mosq_p_mqtt5){
		util__increment_receive_quota(mosq);
	}
	/* We don't use Reason String or User Property yet. */
	return send__command_with_mid(mosq, CMD_PUBREC, mid, false, reason_code, NULL);
}

int send__pubrel(struct mosquitto *mosq, uint16_t mid)
{
// #ifdef WITH_BROKER
// 	log__printf(NULL, MOSQ_LOG_DEBUG, "Sending PUBREL to %s (m%d)", mosq->id, mid);
// #else
	log__printf(mosq, MOSQ_LOG_DEBUG, "Client %s sending PUBREL (m%d)", mosq->id, mid);
// #endif
	/* We don't use Reason String or User Property yet. */
	return send__command_with_mid(mosq, CMD_PUBREL|2, mid, false, 0, NULL);
}

/* For PUBACK, PUBCOMP, PUBREC, and PUBREL */
int send__command_with_mid(struct mosquitto *mosq, uint8_t command, uint16_t mid, bool dup, uint8_t reason_code, const mosquitto_property *properties)
{
	struct mosquitto__packet *packet = NULL;
	int rc;
	int proplen, varbytes;

	assert(mosq);
	packet = (struct mosquitto__packet*)calloc(1, sizeof(struct mosquitto__packet));
	if(!packet) return MOSQ_ERR_NOMEM;

	packet->command = command;
	if(dup){
		packet->command |= 8;
	}
	packet->remaining_length = 2;

	if(mosq->protocol == mosq_p_mqtt5){
		if(reason_code != 0 || properties){
			packet->remaining_length += 1;
		}

		if(properties){
			proplen = property__get_length_all(properties);
			varbytes = packet__varint_bytes(proplen);
			packet->remaining_length += varbytes + proplen;
		}
	}

	rc = packet__alloc(packet);
	if(rc){
		free(packet);
		return rc;
	}

	packet__write_uint16(packet, mid);

	if(mosq->protocol == mosq_p_mqtt5){
		if(reason_code != 0 || properties){
			packet__write_byte(packet, reason_code);
		}
		if(properties){
			property__write_all(packet, properties, true);
		}
	}

	return packet__queue(mosq, packet);
}

int send__simple_command(struct mosquitto *mosq, uint8_t command)
{
	struct mosquitto__packet *packet = NULL;
	int rc;

	assert(mosq);
	packet = (struct mosquitto__packet*)calloc(1, sizeof(struct mosquitto__packet));
	if(!packet) return MOSQ_ERR_NOMEM;

	packet->command = command;
	packet->remaining_length = 0;

	rc = packet__alloc(packet);
	if(rc){
		free(packet);
		return rc;
	}

	return packet__queue(mosq, packet);
}