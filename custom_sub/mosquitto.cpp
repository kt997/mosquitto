#include "sub.h"



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
