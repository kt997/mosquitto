#pragma once
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <stddef.h>
#include <assert.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <pthread.h>
#include <fcntl.h>
#include <netdb.h>
// #include <ares.h>

#include <iostream>
#include <cstddef>

using namespace std;



// int add(int , int );

typedef struct mqtt5__property mosquitto_property;

typedef int mosq_sock_t;

typedef unsigned char uint8_t;
// typedef int int8_t;
typedef unsigned short uint16_t;
// typedef short int16_t;
typedef unsigned int uint32_t;
// typedef int int32_t;
// typedef unsigned long long uint64_t;
// typedef long long int64_t;



extern uint64_t g_bytes_received;
extern uint64_t g_bytes_sent;
extern uint64_t g_pub_bytes_received;
extern uint64_t g_pub_bytes_sent;
extern unsigned long g_msgs_received;
extern unsigned long g_msgs_sent;
extern unsigned long g_pub_msgs_received;
extern unsigned long g_pub_msgs_sent;
extern unsigned long g_msgs_dropped;
extern int g_clients_expired;   
extern unsigned int g_socket_connections;
extern unsigned int g_connection_count;


#define MQTT_MAX_PAYLOAD 268435455U

#define INVALID_SOCKET -1
#define MOSQ_MQTT_ID_MAX_LENGTH 23
#define MQTT_PROTOCOL_V31 3
#define MQTT_PROTOCOL_V311 4
#define MQTT_PROTOCOL_V5 5
                
#define PROTOCOL_NAME_v31 "MQIsdp"
#define PROTOCOL_NAME "MQTT"


#define STREMPTY(str) (str[0] == '\0')

/* Log types */
#define MOSQ_LOG_NONE			0
#define MOSQ_LOG_INFO			(1<<0)
#define MOSQ_LOG_NOTICE			(1<<1)
#define MOSQ_LOG_WARNING		(1<<2)
#define MOSQ_LOG_ERR			(1<<3)
#define MOSQ_LOG_DEBUG			(1<<4)
#define MOSQ_LOG_SUBSCRIBE		(1<<5)
#define MOSQ_LOG_UNSUBSCRIBE	(1<<6)
// #define MOSQ_LOG_WEBSOCKETS		(1<<7)
#define MOSQ_LOG_INTERNAL		0x80000000U
#define MOSQ_LOG_ALL			0xFFFFFFFFU

/* Message types */
#define CMD_CONNECT 0x10
#define CMD_CONNACK 0x20
#define CMD_PUBLISH 0x30
#define CMD_PUBACK 0x40
#define CMD_PUBREC 0x50
#define CMD_PUBREL 0x60
#define CMD_PUBCOMP 0x70
#define CMD_SUBSCRIBE 0x80
#define CMD_SUBACK 0x90
#define CMD_UNSUBSCRIBE 0xA0
#define CMD_UNSUBACK 0xB0
#define CMD_PINGREQ 0xC0
#define CMD_PINGRESP 0xD0
#define CMD_DISCONNECT 0xE0
#define CMD_AUTH 0xF0

#define CMD_WILL 0x100

#define MOSQ_MSB(A) (uint8_t)((A & 0xFF00) >> 8)
#define MOSQ_LSB(A) (uint8_t)(A & 0x00FF)


#define G_BYTES_RECEIVED_INC(A) (g_bytes_received+=(A))
#define G_BYTES_SENT_INC(A) (g_bytes_sent+=(A))
#define G_PUB_BYTES_RECEIVED_INC(A) (g_pub_bytes_received+=(A))
#define G_PUB_BYTES_SENT_INC(A) (g_pub_bytes_sent+=(A))
#define G_MSGS_RECEIVED_INC(A) (g_msgs_received+=(A))
#define G_MSGS_SENT_INC(A) (g_msgs_sent+=(A))   
#define G_PUB_MSGS_RECEIVED_INC(A) (g_pub_msgs_received+=(A))
#define G_PUB_MSGS_SENT_INC(A) (g_pub_msgs_sent+=(A))
#define G_MSGS_DROPPED_INC() (g_msgs_dropped++)
#define G_CLIENTS_EXPIRED_INC() (g_clients_expired++)
#define G_SOCKET_CONNECTIONS_INC() (g_socket_connections++)
#define G_CONNECTION_COUNT_INC() (g_connection_count++)


#define DL_DELETE(head,del)                                                                    \
    DL_DELETE2(head,del,prev,next)

#define DL_DELETE2(head,del,prev,next)                                                         \
do {                                                                                           \
  assert((head) != NULL);                                                                      \
  assert((del)->prev != NULL);                                                                 \
  if ((del)->prev == (del)) {                                                                  \
      (head)=NULL;                                                                             \
  } else if ((del)==(head)) {                                                                  \
      (del)->next->prev = (del)->prev;                                                         \
      (head) = (del)->next;                                                                    \
  } else {                                                                                     \
      (del)->prev->next = (del)->next;                                                         \
      if ((del)->next) {                                                                       \
          (del)->next->prev = (del)->prev;                                                     \
      } else {                                                                                 \
          (head)->prev = (del)->prev;                                                          \
      }                                                                                        \
  }                                                                                            \
} while (0)

#define DL_FOREACH(head,el)                                                                    \
    DL_FOREACH2(head,el,next)
                        
#define DL_FOREACH2(head,el,next)                                                              \
    for ((el) = (head); el; (el) = (el)->next)

#define DL_FOREACH_SAFE(head,el,tmp)                                                           \
    DL_FOREACH_SAFE2(head,el,tmp,next)
                
#define DL_FOREACH_SAFE2(head,el,tmp,next)                                                     \
  for ((el) = (head); (el) && ((tmp) = (el)->next, 1); (el) = (tmp))

#define DL_APPEND(head,add)                                                                    \
    DL_APPEND2(head,add,prev,next)

#define DL_APPEND2(head,add,prev,next)                                                         \
do {                                                                                           \
  if (head) {                                                                                  \
      (add)->prev = (head)->prev;                                                              \
      (head)->prev->next = (add);                                                              \
      (head)->prev = (add);                                                                    \
      (add)->next = NULL;                                                                      \
  } else {                                                                                     \
      (head)=(add);                                                                            \
      (head)->prev = (head);                                                                   \
      (head)->next = NULL;                                                                     \
  }                                                                                            \
} while (0)

// #ifdef WITH_TLS 
// #  define SSL_DATA_PENDING(A) ((A)->ssl && SSL_pending((A)->ssl))
// #else                   
#define SSL_DATA_PENDING(A) 0
// #endif 

#define UNUSED(A) (void)(A)

typedef struct UT_hash_bucket {
   struct UT_hash_handle *hh_head;
   unsigned count;

   /* expand_mult is normally set to 0. In this situation, the max chain length
    * threshold is enforced at its default value, HASH_BKT_CAPACITY_THRESH. (If
    * the bucket's chain exceeds this length, bucket expansion is triggered). 
    * However, setting expand_mult to a non-zero value delays bucket expansion
    * (that would be triggered by additions to this particular bucket)
    * until its chain length reaches a *multiple* of HASH_BKT_CAPACITY_THRESH.
    * (The multiplier is simply expand_mult+1). The whole idea of this
    * multiplier is to reduce bucket expansions, since they are expensive, in
    * situations where we know that a particular bucket tends to be overused.
    * It is better to let its chain length grow to a longer yet-still-bounded
    * value, than to do an O(n) bucket expansion too often. 
    */
   unsigned expand_mult;

} UT_hash_bucket;

typedef struct UT_hash_table {
   UT_hash_bucket *buckets;
   unsigned num_buckets, log2_num_buckets;
   unsigned num_items;
   struct UT_hash_handle *tail; /* tail hh in app order, for fast append    */
   ptrdiff_t hho; /* hash handle offset (byte pos of hash handle in element */

   /* in an ideal situation (all buckets used equally), no bucket would have
    * more than ceil(#items/#buckets) items. that's the ideal chain length. */ 
   unsigned ideal_chain_maxlen;
        
   /* nonideal_items is the number of items in the hash whose chain position
    * exceeds the ideal chain maxlen. these items pay the penalty for an uneven
    * hash distribution; reaching them in a chain traversal takes >ideal steps */
   unsigned nonideal_items;
        
   /* ineffective expands occur when a bucket doubling was performed, but 
    * afterward, more than half the items in the hash had nonideal chain
    * positions. If this happens on two consecutive expansions we inhibit any
    * further expansion, as it's not helping; this happens when the hash
    * function isn't a good fit for the key domain. When expansion is inhibited
    * the hash will still work, albeit no longer in constant time. */
   unsigned ineff_expands, noexpand;

   uint32_t signature; /* used only to find hash tables in external analysis */
#ifdef HASH_BLOOM
   uint32_t bloom_sig; /* used only to test bloom exists in external analysis */
   uint8_t *bloom_bv;
   char bloom_nbits;
#endif  
        
} UT_hash_table;

typedef struct UT_hash_handle {
   struct UT_hash_table *tbl;
   void *prev;                       /* prev element in app order      */
   void *next;                       /* next element in app order      */
   struct UT_hash_handle *hh_prev;   /* previous hh in bucket order    */
   struct UT_hash_handle *hh_next;   /* next hh in bucket order        */
   void *key;                        /* ptr to enclosing struct's key  */
   unsigned keylen;                  /* enclosing struct's key len     */
   unsigned hashv;                   /* result of hash-fcn(key)        */
} UT_hash_handle;


static char alphanum[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

struct mqtt__string {
        char *v;
        int len;
};

enum mosquitto_msg_direction {
	mosq_md_in = 0,
	mosq_md_out = 1
};

enum mosquitto__threaded_state {
        mosq_ts_none,           /* No threads in use */ 
        mosq_ts_self,           /* Threads started by libmosquitto */
        mosq_ts_external        /* Threads started by external code */
};

enum mqtt311_connack_codes {
        CONNACK_ACCEPTED = 0,
        CONNACK_REFUSED_PROTOCOL_VERSION = 1,
        CONNACK_REFUSED_IDENTIFIER_REJECTED = 2,
        CONNACK_REFUSED_SERVER_UNAVAILABLE = 3,
        CONNACK_REFUSED_BAD_USERNAME_PASSWORD = 4,
        CONNACK_REFUSED_NOT_AUTHORIZED = 5,
};

enum mqtt5_return_codes {
        MQTT_RC_SUCCESS = 0,                                            /* CONNACK, PUBACK, PUBREC, PUBREL, PUBCOMP, UNSUBACK, AUTH */
        MQTT_RC_NORMAL_DISCONNECTION = 0,                       /* DISCONNECT */
        MQTT_RC_GRANTED_QOS0 = 0,                                       /* SUBACK */
        MQTT_RC_GRANTED_QOS1 = 1,                                       /* SUBACK */
        MQTT_RC_GRANTED_QOS2 = 2,                                       /* SUBACK */
        MQTT_RC_DISCONNECT_WITH_WILL_MSG = 4,           /* DISCONNECT */
        MQTT_RC_NO_MATCHING_SUBSCRIBERS = 16,           /* PUBACK, PUBREC */
        MQTT_RC_NO_SUBSCRIPTION_EXISTED = 17,           /* UNSUBACK */
        MQTT_RC_CONTINUE_AUTHENTICATION = 24,           /* AUTH */
        MQTT_RC_REAUTHENTICATE = 25,                            /* AUTH */

        MQTT_RC_UNSPECIFIED = 128,                                      /* CONNACK, PUBACK, PUBREC, SUBACK, UNSUBACK, DISCONNECT */
        MQTT_RC_MALFORMED_PACKET = 129,                         /* CONNACK, DISCONNECT */
        MQTT_RC_PROTOCOL_ERROR = 130,                           /* DISCONNECT */
        MQTT_RC_IMPLEMENTATION_SPECIFIC = 131,          /* CONNACK, PUBACK, PUBREC, SUBACK, UNSUBACK, DISCONNECT */
        MQTT_RC_UNSUPPORTED_PROTOCOL_VERSION = 132,     /* CONNACK */
        MQTT_RC_CLIENTID_NOT_VALID = 133,                       /* CONNACK */
        MQTT_RC_BAD_USERNAME_OR_PASSWORD = 134,         /* CONNACK */
        MQTT_RC_NOT_AUTHORIZED = 135,                           /* CONNACK, PUBACK, PUBREC, SUBACK, UNSUBACK, DISCONNECT */
        MQTT_RC_SERVER_UNAVAILABLE = 136,                       /* CONNACK */
        MQTT_RC_SERVER_BUSY = 137,                                      /* CONNACK, DISCONNECT */
        MQTT_RC_BANNED = 138,                                           /* CONNACK */
        MQTT_RC_SERVER_SHUTTING_DOWN = 139,                     /* DISCONNECT */
        MQTT_RC_BAD_AUTHENTICATION_METHOD = 140,        /* CONNACK */
        MQTT_RC_KEEP_ALIVE_TIMEOUT = 141,                       /* DISCONNECT */
        MQTT_RC_SESSION_TAKEN_OVER = 142,                       /* DISCONNECT */
        MQTT_RC_TOPIC_FILTER_INVALID = 143,                     /* SUBACK, UNSUBACK, DISCONNECT */
        MQTT_RC_TOPIC_NAME_INVALID = 144,                       /* CONNACK, PUBACK, PUBREC, DISCONNECT */
        MQTT_RC_PACKET_ID_IN_USE = 145,                         /* PUBACK, SUBACK, UNSUBACK */
        MQTT_RC_PACKET_ID_NOT_FOUND = 146,                      /* PUBREL, PUBCOMP */
        MQTT_RC_RECEIVE_MAXIMUM_EXCEEDED = 147,         /* DISCONNECT */
        MQTT_RC_TOPIC_ALIAS_INVALID = 148,                      /* DISCONNECT */
        MQTT_RC_PACKET_TOO_LARGE = 149,                         /* CONNACK, PUBACK, PUBREC, DISCONNECT */
        MQTT_RC_MESSAGE_RATE_TOO_HIGH = 150,            /* DISCONNECT */
        MQTT_RC_QUOTA_EXCEEDED = 151,                           /* PUBACK, PUBREC, SUBACK, DISCONNECT */
        MQTT_RC_ADMINISTRATIVE_ACTION = 152,            /* DISCONNECT */
        MQTT_RC_PAYLOAD_FORMAT_INVALID = 153,           /* CONNACK, DISCONNECT */
        MQTT_RC_RETAIN_NOT_SUPPORTED = 154,                     /* CONNACK, DISCONNECT */
        MQTT_RC_QOS_NOT_SUPPORTED = 155,                        /* CONNACK, DISCONNECT */
        MQTT_RC_USE_ANOTHER_SERVER = 156,                       /* CONNACK, DISCONNECT */
        MQTT_RC_SERVER_MOVED = 157,                                     /* CONNACK, DISCONNECT */
        MQTT_RC_SHARED_SUBS_NOT_SUPPORTED = 158,        /* SUBACK, DISCONNECT */
        MQTT_RC_CONNECTION_RATE_EXCEEDED = 159,         /* CONNACK, DISCONNECT */
        MQTT_RC_MAXIMUM_CONNECT_TIME = 160,                     /* DISCONNECT */
        MQTT_RC_SUBSCRIPTION_IDS_NOT_SUPPORTED = 161, /* SUBACK, DISCONNECT */
        MQTT_RC_WILDCARD_SUBS_NOT_SUPPORTED = 162,      /* SUBACK, DISCONNECT */
};


enum mosquitto_msg_state {
        mosq_ms_invalid = 0,
        mosq_ms_publish_qos0 = 1,
        mosq_ms_publish_qos1 = 2,
        mosq_ms_wait_for_puback = 3,
        mosq_ms_publish_qos2 = 4,
        mosq_ms_wait_for_pubrec = 5,
        mosq_ms_resend_pubrel = 6,
        mosq_ms_wait_for_pubrel = 7,
        mosq_ms_resend_pubcomp = 8,
        mosq_ms_wait_for_pubcomp = 9,
        mosq_ms_send_pubrec = 10,
        mosq_ms_queued = 11
};

enum mosquitto_client_state {
	mosq_cs_new = 0,
	mosq_cs_connected = 1,
	mosq_cs_disconnecting = 2,
	mosq_cs_active = 3,
	mosq_cs_connect_pending = 4,
	mosq_cs_connect_srv = 5,
	mosq_cs_disconnect_ws = 6,
	mosq_cs_disconnected = 7,
	mosq_cs_socks5_new = 8,
	mosq_cs_socks5_start = 9,
	mosq_cs_socks5_request = 10,
	mosq_cs_socks5_reply = 11,
	mosq_cs_socks5_auth_ok = 12,
	mosq_cs_socks5_userpass_reply = 13,
	mosq_cs_socks5_send_userpass = 14,
	mosq_cs_expiring = 15,
	mosq_cs_duplicate = 17, /* client that has been taken over by another with the same id */
	mosq_cs_disconnect_with_will = 18,
	mosq_cs_disused = 19, /* client that has been added to the disused list to be freed */
	mosq_cs_authenticating = 20, /* Client has sent CONNECT but is still undergoing extended authentication */
	mosq_cs_reauthenticating = 21, /* Client is undergoing reauthentication and shouldn't do anything else until complete */
};

enum mosquitto__protocol {
	mosq_p_invalid = 0,
	mosq_p_mqtt31 = 1,
	mosq_p_mqtt311 = 2,
	mosq_p_mqtts = 3,
	mosq_p_mqtt5 = 5,
};

/* Error values */
enum mosq_err_t {
	MOSQ_ERR_AUTH_CONTINUE = -4,
	MOSQ_ERR_NO_SUBSCRIBERS = -3,
	MOSQ_ERR_SUB_EXISTS = -2,
	MOSQ_ERR_CONN_PENDING = -1,
	MOSQ_ERR_SUCCESS = 0,
	MOSQ_ERR_NOMEM = 1,
	MOSQ_ERR_PROTOCOL = 2,
	MOSQ_ERR_INVAL = 3,
	MOSQ_ERR_NO_CONN = 4,
	MOSQ_ERR_CONN_REFUSED = 5,
	MOSQ_ERR_NOT_FOUND = 6,
	MOSQ_ERR_CONN_LOST = 7,
	MOSQ_ERR_TLS = 8,
	MOSQ_ERR_PAYLOAD_SIZE = 9,
	MOSQ_ERR_NOT_SUPPORTED = 10,
	MOSQ_ERR_AUTH = 11,
	MOSQ_ERR_ACL_DENIED = 12,
	MOSQ_ERR_UNKNOWN = 13,
	MOSQ_ERR_ERRNO = 14,
	MOSQ_ERR_EAI = 15,
	MOSQ_ERR_PROXY = 16,
	MOSQ_ERR_PLUGIN_DEFER = 17,
	MOSQ_ERR_MALFORMED_UTF8 = 18,
	MOSQ_ERR_KEEPALIVE = 19,
	MOSQ_ERR_LOOKUP = 20,
	MOSQ_ERR_MALFORMED_PACKET = 21,
	MOSQ_ERR_DUPLICATE_PROPERTY = 22,
	MOSQ_ERR_TLS_HANDSHAKE = 23,
	MOSQ_ERR_QOS_NOT_SUPPORTED = 24,
	MOSQ_ERR_OVERSIZE_PACKET = 25,
	MOSQ_ERR_OCSP = 26,
};

/* Option values */
enum mosq_opt_t {
	MOSQ_OPT_PROTOCOL_VERSION = 1,
	MOSQ_OPT_SSL_CTX = 2,
	MOSQ_OPT_SSL_CTX_WITH_DEFAULTS = 3,
	MOSQ_OPT_RECEIVE_MAXIMUM = 4,
	MOSQ_OPT_SEND_MAXIMUM = 5,
	MOSQ_OPT_TLS_KEYFORM = 6,
	MOSQ_OPT_TLS_ENGINE = 7,
	MOSQ_OPT_TLS_ENGINE_KPASS_SHA1 = 8,
	MOSQ_OPT_TLS_OCSP_REQUIRED = 9,
	MOSQ_OPT_TLS_ALPN = 10,
};

struct mosquitto_message{
	int mid;
	char *topic;
	void *payload;
	int payloadlen;
	int qos;
	bool retain;
};

struct mosquitto__packet{
	uint8_t *payload;
	struct mosquitto__packet *next;
	uint32_t remaining_mult;
	uint32_t remaining_length;
	uint32_t packet_length;
	uint32_t to_process;
	uint32_t pos;
	uint16_t mid;
	uint8_t command;
	int8_t remaining_count;	
};




struct mosquitto_message_all{
	struct mosquitto_message_all *next;
	struct mosquitto_message_all *prev;
	mosquitto_property *properties;
	time_t timestamp;
	//enum mosquitto_msg_direction direction;
	enum mosquitto_msg_state state;
	bool dup;
	struct mosquitto_message msg;
	uint32_t expiry_interval;
};

struct mqtt5__property {
        struct mqtt5__property *next;
        union {
                uint8_t i8;
                uint16_t i16;
                uint32_t i32;
                uint32_t varint;
                struct mqtt__string bin;
                struct mqtt__string s;
        } value;
        struct mqtt__string name;
        int32_t identifier;
        bool client_generated;
}; 

enum mqtt5_property {
	MQTT_PROP_PAYLOAD_FORMAT_INDICATOR = 1,		/* Byte :				PUBLISH, Will Properties */
	MQTT_PROP_MESSAGE_EXPIRY_INTERVAL = 2,		/* 4 byte int :			PUBLISH, Will Properties */
	MQTT_PROP_CONTENT_TYPE = 3,					/* UTF-8 string :		PUBLISH, Will Properties */
	MQTT_PROP_RESPONSE_TOPIC = 8,				/* UTF-8 string :		PUBLISH, Will Properties */
	MQTT_PROP_CORRELATION_DATA = 9,				/* Binary Data :		PUBLISH, Will Properties */
	MQTT_PROP_SUBSCRIPTION_IDENTIFIER = 11,		/* Variable byte int :	PUBLISH, SUBSCRIBE */
	MQTT_PROP_SESSION_EXPIRY_INTERVAL = 17,		/* 4 byte int :			CONNECT, CONNACK, DISCONNECT */
	MQTT_PROP_ASSIGNED_CLIENT_IDENTIFIER = 18,	/* UTF-8 string :		CONNACK */
	MQTT_PROP_SERVER_KEEP_ALIVE = 19,			/* 2 byte int :			CONNACK */
	MQTT_PROP_AUTHENTICATION_METHOD = 21,		/* UTF-8 string :		CONNECT, CONNACK, AUTH */
	MQTT_PROP_AUTHENTICATION_DATA = 22,			/* Binary Data :		CONNECT, CONNACK, AUTH */
	MQTT_PROP_REQUEST_PROBLEM_INFORMATION = 23,	/* Byte :				CONNECT */
	MQTT_PROP_WILL_DELAY_INTERVAL = 24,			/* 4 byte int :			Will properties */
	MQTT_PROP_REQUEST_RESPONSE_INFORMATION = 25,/* Byte :				CONNECT */
	MQTT_PROP_RESPONSE_INFORMATION = 26,		/* UTF-8 string :		CONNACK */
	MQTT_PROP_SERVER_REFERENCE = 28,			/* UTF-8 string :		CONNACK, DISCONNECT */
	MQTT_PROP_REASON_STRING = 31,				/* UTF-8 string :		All except Will properties */
	MQTT_PROP_RECEIVE_MAXIMUM = 33,				/* 2 byte int :			CONNECT, CONNACK */
	MQTT_PROP_TOPIC_ALIAS_MAXIMUM = 34,			/* 2 byte int :			CONNECT, CONNACK */
	MQTT_PROP_TOPIC_ALIAS = 35,					/* 2 byte int :			PUBLISH */
	MQTT_PROP_MAXIMUM_QOS = 36,					/* Byte :				CONNACK */
	MQTT_PROP_RETAIN_AVAILABLE = 37,			/* Byte :				CONNACK */
	MQTT_PROP_USER_PROPERTY = 38,				/* UTF-8 string pair :	All */
	MQTT_PROP_MAXIMUM_PACKET_SIZE = 39,			/* 4 byte int :			CONNECT, CONNACK */
	MQTT_PROP_WILDCARD_SUB_AVAILABLE = 40,		/* Byte :				CONNACK */
	MQTT_PROP_SUBSCRIPTION_ID_AVAILABLE = 41,	/* Byte :				CONNACK */
	MQTT_PROP_SHARED_SUB_AVAILABLE = 42,		/* Byte :				CONNACK */
};

struct mosquitto_msg_data{
#ifdef WITH_BROKER
        struct mosquitto_client_msg *inflight;
        struct mosquitto_client_msg *queued;
        unsigned long msg_bytes;
        unsigned long msg_bytes12;
        int msg_count;
        int msg_count12;
#else
        struct mosquitto_message_all *inflight;
        int queue_len;
// #  ifdef WITH_THREADING
        pthread_mutex_t mutex;
// #  endif
#endif
        int inflight_quota;
        uint16_t inflight_maximum;
};

struct mosquitto__acl_user{
        struct mosquitto__acl_user *next;
        char *username;
        struct mosquitto__acl *acl;
}; 

enum mosquitto_protocol {
        mp_mqtt,
        mp_mqttsn,
        mp_websockets
}; 

enum mosquitto_bridge_start_type{
        bst_automatic = 0,
        bst_lazy = 1,
        bst_manual = 2,
        bst_once = 3
}; 

struct mosquitto__bridge{
        char *name;
        struct bridge_address *addresses;
        int cur_address;
        int address_count;
        time_t primary_retry;
        mosq_sock_t primary_retry_sock;
        bool round_robin;
        bool try_private;
        bool try_private_accepted;
        bool clean_start;
        int keepalive;
        struct mosquitto__bridge_topic *topics;
        int topic_count;
        bool topic_remapping;
        enum mosquitto__protocol protocol_version;
        time_t restart_t;
        char *remote_clientid;
        char *remote_username;
        char *remote_password;
        char *local_clientid;
        char *local_username;
        char *local_password;
        char *notification_topic;
        bool notifications;
        bool notifications_local_only;
        enum mosquitto_bridge_start_type start_type;
        int idle_timeout;
        int restart_timeout;
        int backoff_base;
        int backoff_cap;
        int threshold;
        bool lazy_reconnect;
        bool attempt_unsubscribe;
        bool initial_notification_done;
#ifdef WITH_TLS
        bool tls_insecure;
        bool tls_ocsp_required;
        char *tls_cafile;
        char *tls_capath;
        char *tls_certfile;
        char *tls_keyfile;
        char *tls_version;
        char *tls_alpn;
#  ifdef FINAL_WITH_TLS_PSK
        char *tls_psk_identity;
        char *tls_psk;
#  endif
#endif
};

struct mosquitto__security_options {
        /* Any options that get added here also need considering
         * in config__read() with regards whether allow_anonymous
         * should be disabled when these options are set.
         */
        struct mosquitto__acl_user *acl_list;
        struct mosquitto__acl *acl_patterns;
        char *password_file;
        char *psk_file;
        char *acl_file;
        struct mosquitto__auth_plugin_config *auth_plugin_configs;
        int auth_plugin_config_count;
        int8_t allow_anonymous;
        bool allow_zero_length_clientid;
        char *auto_id_prefix;
        int auto_id_prefix_len;
};      

struct mosquitto__listener {
        int fd;
        uint16_t port;
        char *host;
        char *bind_interface;
        int max_connections;
        char *mount_point;
        mosq_sock_t *socks;
        int sock_count;
        int client_count;
        enum mosquitto_protocol protocol;
        int socket_domain;
        bool use_username_as_clientid;
        uint8_t maximum_qos;
        uint16_t max_topic_alias;
#ifdef WITH_TLS
        char *cafile;
        char *capath;
        char *certfile;
        char *keyfile;
        char *tls_engine;
        char *tls_engine_kpass_sha1;
        char *ciphers;
        char *psk_hint;
        SSL_CTX *ssl_ctx;
        char *crlfile;
        char *tls_version;
        char *dhparamfile;
        bool use_identity_as_username;
        bool use_subject_as_username;
        bool require_certificate;
        // enum mosquitto__keyform tls_keyform;
#endif
// #ifdef WITH_WEBSOCKETS
//         struct libwebsocket_context *ws_context;
//         char *http_dir;
//         struct libwebsocket_protocols *ws_protocol;
// #endif
        struct mosquitto__security_options security_options;
        struct mosquitto__unpwd *unpwd;
        struct mosquitto__unpwd *psk_id;
};

struct mosquitto__subhier {
        UT_hash_handle hh;
        struct mosquitto__subhier *parent;
        struct mosquitto__subhier *children;
        struct mosquitto__subleaf *subs;
        struct mosquitto__subshared *shared;
        struct mosquitto_msg_store *retained;
        char *topic;
        uint16_t topic_len;
};  

struct mosquitto__subshared_ref { 
        struct mosquitto__subhier *hier;
        struct mosquitto__subshared *shared;
};      

struct mosquitto {
	mosq_sock_t sock;
// #ifndef WITH_BROKER
	mosq_sock_t sockpairR, sockpairW;
// #endif
#if defined(__GLIBC__) && defined(WITH_ADNS)
	struct gaicb *adns; /* For getaddrinfo_a */
#endif
	enum mosquitto__protocol protocol;
	char *address;
	char *id;
	char *username;
	char *password;
	uint16_t keepalive;
	uint16_t last_mid;
	enum mosquitto_client_state state;
	time_t last_msg_in;
	time_t next_msg_out;
	time_t ping_t;
	struct mosquitto__packet in_packet;
	struct mosquitto__packet *current_out_packet;
	struct mosquitto__packet *out_packet;
	struct mosquitto_message_all *will;
	struct mosquitto__alias *aliases;
	struct will_delay_list *will_delay_entry;
	uint32_t maximum_packet_size;
	int alias_count;
	uint32_t will_delay_interval;
	time_t will_delay_time;
// #ifdef WITH_TLS
// 	SSL *ssl;
// 	SSL_CTX *ssl_ctx;
// 	char *tls_cafile;
// 	char *tls_capath;
// 	char *tls_certfile;
// 	char *tls_keyfile;
// 	int (*tls_pw_callback)(char *buf, int size, int rwflag, void *userdata);
// 	char *tls_version;
// 	char *tls_ciphers;
// 	char *tls_psk;
// 	char *tls_psk_identity;
// 	int tls_cert_reqs;
// 	bool tls_insecure;
// 	bool ssl_ctx_defaults;
// 	bool tls_ocsp_required;
// 	char *tls_engine;
// 	char *tls_engine_kpass_sha1;
// 	enum mosquitto__keyform tls_keyform;
// 	char *tls_alpn;
// #endif
	bool want_write;
	bool want_connect;
// #if defined(WITH_THREADING) && !defined(WITH_BROKER)
	pthread_mutex_t callback_mutex;
	pthread_mutex_t log_callback_mutex;
	pthread_mutex_t msgtime_mutex;
	pthread_mutex_t out_packet_mutex;
	pthread_mutex_t current_out_packet_mutex;
	pthread_mutex_t state_mutex;
	pthread_mutex_t mid_mutex;
	pthread_t thread_id;
// #endif
	bool clean_start;
	uint32_t session_expiry_interval;
	time_t session_expiry_time;
// #ifdef WITH_BROKER
// 	bool removed_from_by_id; /* True if removed from by_id hash */
// 	bool is_dropping;
// 	bool is_bridge;
// 	struct mosquitto__bridge *bridge;
// 	struct mosquitto_msg_data msgs_in;
// 	struct mosquitto_msg_data msgs_out;
// 	struct mosquitto__acl_user *acl_list;
// 	struct mosquitto__listener *listener;
// 	struct mosquitto__packet *out_packet_last;
// 	struct mosquitto__subhier **subs;
// 	struct mosquitto__subshared_ref **shared_subs;
// 	char *auth_method;
// 	int sub_count;
// 	int shared_sub_count;
// 	int pollfd_index;
// // #  ifdef WITH_WEBSOCKETS
// // #    if defined(LWS_LIBRARY_VERSION_NUMBER)
// // 	struct lws *wsi;
// // #    else
// // 	struct libwebsocket_context *ws_context;
// // 	struct libwebsocket *wsi;
// // #    endif
// // #  endif
// 	bool ws_want_write;
// 	bool assigned_id;
// #else
// #  ifdef WITH_SOCKS
	char *socks5_host;
	int socks5_port;
	char *socks5_username;
	char *socks5_password;
// #  endif
	void *userdata;
	bool in_callback;
	struct mosquitto_msg_data msgs_in;
	struct mosquitto_msg_data msgs_out;
	void (*on_connect)(struct mosquitto *, void *userdata, int rc);
	void (*on_connect_with_flags)(struct mosquitto *, void *userdata, int rc, int flags);
	void (*on_connect_v5)(struct mosquitto *, void *userdata, int rc, int flags, const mosquitto_property *props);
	void (*on_disconnect)(struct mosquitto *, void *userdata, int rc);
	void (*on_disconnect_v5)(struct mosquitto *, void *userdata, int rc, const mosquitto_property *props);
	void (*on_publish)(struct mosquitto *, void *userdata, int mid);
	void (*on_publish_v5)(struct mosquitto *, void *userdata, int mid, int reason_code, const mosquitto_property *props);
	void (*on_message)(struct mosquitto *, void *userdata, const struct mosquitto_message *message);
	void (*on_message_v5)(struct mosquitto *, void *userdata, const struct mosquitto_message *message, const mosquitto_property *props);
	void (*on_subscribe)(struct mosquitto *, void *userdata, int mid, int qos_count, const int *granted_qos);
	void (*on_subscribe_v5)(struct mosquitto *, void *userdata, int mid, int qos_count, const int *granted_qos, const mosquitto_property *props);
	void (*on_unsubscribe)(struct mosquitto *, void *userdata, int mid);
	void (*on_unsubscribe_v5)(struct mosquitto *, void *userdata, int mid, const mosquitto_property *props);
	void (*on_log)(struct mosquitto *, void *userdata, int level, const char *str);
	//void (*on_error)();
	char *host;
	int port;
	char *bind_address;
	unsigned int reconnects;
	unsigned int reconnect_delay;
	unsigned int reconnect_delay_max;
	bool reconnect_exponential_backoff;
	char threaded;
	struct mosquitto__packet *out_packet_last;
// #  ifdef WITH_SRV
	// ares_channel achan;
// #  endif
// #endif
	uint8_t maximum_qos;

// #ifdef WITH_BROKER
// 	UT_hash_handle hh_id;
// 	UT_hash_handle hh_sock;
// 	struct mosquitto *for_free_next;
// 	struct session_expiry_list *expiry_list_item;
// #endif
// #ifdef WITH_EPOLL
// 	uint32_t events;
// #endif
};