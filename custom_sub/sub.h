 #pragma once
#include "sub_internals.h"



void mosquitto_destroy(struct mosquitto *mosq);


void mosquitto__destroy(struct mosquitto *mosq);


struct mosquitto *mosquitto_new(const char *id, bool clean_start, void *userdata);


int mosquitto_reinitialise(struct mosquitto *mosq, const char *id, bool clean_start, void *userdata);


void do_client_disconnect(struct mosquitto *mosq, int reason_code, const mosquitto_property *properties);


static int mosquitto__connect_init(struct mosquitto *mosq, const char *host, int port, int keepalive, const char *bind_address);


int mosquitto_connect(struct mosquitto *mosq, const char *host, int port, int keepalive);


int mosquitto_connect_bind(struct mosquitto *mosq, const char *host, int port, int keepalive, const char *bind_address);


int mosquitto_connect_bind_v5(struct mosquitto *mosq, const char *host, int port, int keepalive, const char *bind_address, const mosquitto_property *properties);


int mosquitto_reconnect(struct mosquitto *mosq);


static int mosquitto__reconnect(struct mosquitto *mosq, bool blocking, const mosquitto_property *properties);


int mosquitto_disconnect(struct mosquitto *mosq);


int mosquitto_disconnect_v5(struct mosquitto *mosq, int reason_code, const mosquitto_property *properties);


int mosquitto__set_state(struct mosquitto *mosq, enum mosquitto_client_state state);


enum mosquitto_client_state mosquitto__get_state(struct mosquitto *mosq);


int util__random_bytes(void *bytes, int count);


uint16_t mosquitto__mid_generate(struct mosquitto *mosq);


void util__increment_receive_quota(struct mosquitto *mosq);


void util__increment_send_quota(struct mosquitto *mosq);


void util__decrement_receive_quota(struct mosquitto *mosq);


void util__decrement_send_quota(struct mosquitto *mosq);


int mosquitto__check_keepalive(struct mosquitto *mosq);


// int mosquitto_publish(struct mosquitto *mosq, int *mid, const char *topic, int payloadlen, const void *payload, int qos, bool retain);


// int mosquitto_publish_v5(struct mosquitto *mosq, int *mid, const char *topic, int payloadlen, const void *payload, int qos, bool retain, const mosquitto_property *properties);


int mosquitto_subscribe(struct mosquitto *mosq, int *mid, const char *sub, int qos);


int mosquitto_subscribe_v5(struct mosquitto *mosq, int *mid, const char *sub, int qos, int options, const mosquitto_property *properties);


int mosquitto_subscribe_multiple(struct mosquitto *mosq, int *mid, int sub_count, char *const *const sub, int qos, int options, const mosquitto_property *properties);


int mosquitto_unsubscribe(struct mosquitto *mosq, int *mid, const char *sub);


int mosquitto_unsubscribe_v5(struct mosquitto *mosq, int *mid, const char *sub, const mosquitto_property *properties);


int mosquitto_unsubscribe_multiple(struct mosquitto *mosq, int *mid, int sub_count, char *const *const sub, const mosquitto_property *properties);


void mosquitto_message_callback_set(struct mosquitto *mosq, void (*on_message)(struct mosquitto *, void *, const struct mosquitto_message *));


int mosquitto_validate_utf8(const char *str, int len);


// int mosquitto_pub_topic_check(const char *str);


int mosquitto_sub_topic_check(const char *str);


int property__get_length_all(const mosquitto_property *property);


int property__get_length(const mosquitto_property *property);


int property__write(struct mosquitto__packet *packet, const mosquitto_property *property);


int property__write_all(struct mosquitto__packet *packet, const mosquitto_property *properties, bool write_len);


int property__read_all(int command, struct mosquitto__packet *packet, mosquitto_property **properties);


int property__read(struct mosquitto__packet *packet, int32_t *len, mosquitto_property *property);


void property__free(mosquitto_property **property);


void mosquitto_property_free_all(mosquitto_property **property);


// int mosquitto_property_copy_all(mosquitto_property **dest, const mosquitto_property *src);


int mosquitto_property_check_all(int command, const mosquitto_property *properties);


const mosquitto_property *mosquitto_property_read_int16(const mosquitto_property *proplist, int identifier, uint16_t *value, bool skip_first);


const mosquitto_property *property__get_property(const mosquitto_property *proplist, int identifier, bool skip_first);


int mosquitto_property_add_int16(mosquitto_property **proplist, int identifier, uint16_t value);


static void property__add(mosquitto_property **proplist, struct mqtt5__property *prop);


int mosquitto_property_check_command(int command, int identifier);


const mosquitto_property *mosquitto_property_read_byte(const mosquitto_property *proplist, int identifier, uint8_t *value, bool skip_first);


const mosquitto_property *mosquitto_property_read_int32(const mosquitto_property *proplist, int identifier, uint32_t *value, bool skip_first);


const mosquitto_property *mosquitto_property_read_string(const mosquitto_property *proplist, int identifier, char **value, bool skip_first);


void packet__cleanup(struct mosquitto__packet *packet);


int packet__queue(struct mosquitto *mosq, struct mosquitto__packet *packet);


int packet__check_oversize(struct mosquitto *mosq, uint32_t remaining_length);


int packet__alloc(struct mosquitto__packet *packet);


int packet__write(struct mosquitto *mosq);


void packet__cleanup_all(struct mosquitto *mosq);


int packet__read(struct mosquitto *mosq);


int packet__varint_bytes(int32_t word);


void packet__write_string(struct mosquitto__packet *packet, const char *str, uint16_t length);


void packet__write_uint16(struct mosquitto__packet *packet, uint16_t word);


void packet__write_uint32(struct mosquitto__packet *packet, uint32_t word);


void packet__write_byte(struct mosquitto__packet *packet, uint8_t byte);


int packet__write_varint(struct mosquitto__packet *packet, int32_t word);


void packet__write_bytes(struct mosquitto__packet *packet, const void *bytes, uint32_t count);


int packet__read_string(struct mosquitto__packet *packet, char **str, int *length);


int packet__read_binary(struct mosquitto__packet *packet, uint8_t **data, int *length);


int packet__read_uint16(struct mosquitto__packet *packet, uint16_t *word);


int packet__read_bytes(struct mosquitto__packet *packet, void *bytes, uint32_t count);


int packet__read_byte(struct mosquitto__packet *packet, uint8_t *byte);


int packet__read_varint(struct mosquitto__packet *packet, int32_t *word, int8_t *bytes);


int packet__read_uint32(struct mosquitto__packet *packet, uint32_t *word);


int send__publish(struct mosquitto *mosq, uint16_t mid, const char *topic, uint32_t payloadlen, const void *payload, int qos, bool retain, bool dup, const mosquitto_property *cmsg_props, const mosquitto_property *store_props, uint32_t expiry_interval);


int send__real_publish(struct mosquitto *mosq, uint16_t mid, const char *topic, uint32_t payloadlen, const void *payload, int qos, bool retain, bool dup, const mosquitto_property *cmsg_props, const mosquitto_property *store_props, uint32_t expiry_interval);


int send__subscribe(struct mosquitto *mosq, int *mid, int topic_count, const char **topic, int topic_qos, const mosquitto_property *properties);


int send__unsubscribe(struct mosquitto *mosq, int *mid, int topic_count, char *const *const topic, const mosquitto_property *properties);


int log__printf(struct mosquitto *mosq, int priority, const char *fmt, ...);


time_t mosquitto_time(void);


void message__cleanup_all(struct mosquitto *mosq);


void message__cleanup(struct mosquitto_message_all **message);


int message__queue(struct mosquitto *mosq, struct mosquitto_message_all *message, enum mosquitto_msg_direction dir);


void message__reconnect_reset(struct mosquitto *mosq);


int message__release_to_inflight(struct mosquitto *mosq, enum mosquitto_msg_direction dir);


int message__remove(struct mosquitto *mosq, uint16_t mid, enum mosquitto_msg_direction dir, struct mosquitto_message_all **message, int qos);


// int message__delete(struct mosquitto *mosq, uint16_t mid, enum mosquitto_msg_direction dir, int qos);


// int message__out_update(struct mosquitto *mosq, uint16_t mid, enum mosquitto_msg_state state, int qos);


void message__retry_check(struct mosquitto *mosq);


ssize_t net__write(struct mosquitto *mosq, void *buf, size_t count);


ssize_t net__read(struct mosquitto *mosq, void *buf, size_t count);


int net__socket_close(struct mosquitto *mosq);


int net__socketpair(mosq_sock_t *pairR, mosq_sock_t *pairW);


int net__socket_nonblock(mosq_sock_t *sock);


int net__socket_connect(struct mosquitto *mosq, const char *host, uint16_t port, const char *bind_address, bool blocking);


int net__try_connect(const char *host, uint16_t port, mosq_sock_t *sock, const char *bind_address, bool blocking);


// int net__socket_connect_step3(struct mosquitto *mosq, const char *host);


int send__connect(struct mosquitto *mosq, uint16_t keepalive, bool clean_session, const mosquitto_property *properties);


int send__disconnect(struct mosquitto *mosq, uint8_t reason_code, const mosquitto_property *properties);


int mosquitto_loop_start(struct mosquitto *mosq);


int mosquitto_loop_stop(struct mosquitto *mosq, bool force);


void *mosquitto__thread_main(void *obj);


int mosquitto_loop_forever(struct mosquitto *mosq, int timeout, int max_packets);


int mosquitto_loop(struct mosquitto *mosq, int timeout, int max_packets);


int mosquitto_loop_misc(struct mosquitto *mosq);


int mosquitto_loop_read(struct mosquitto *mosq, int max_packets);


int mosquitto_loop_write(struct mosquitto *mosq, int max_packets);


static int mosquitto__loop_rc_handle(struct mosquitto *mosq, int rc);


int handle__packet(struct mosquitto *mosq);


int handle__pingreq(struct mosquitto *mosq);


int handle__pingresp(struct mosquitto *mosq);


int handle__pubackcomp(struct mosquitto *mosq, const char *type);


int handle__publish(struct mosquitto *mosq);


int handle__pubrec(struct mosquitto_db *db, struct mosquitto *mosq);


int handle__pubrel(struct mosquitto_db *db, struct mosquitto *mosq);


static void connack_callback(struct mosquitto *mosq, uint8_t reason_code, uint8_t connect_flags, const mosquitto_property *properties);


int handle__connack(struct mosquitto *mosq);


int handle__suback(struct mosquitto *mosq);


// int handle__unsuback(struct mosquitto *mosq);


int handle__disconnect(struct mosquitto *mosq);


int send__pingreq(struct mosquitto *mosq);


int send__pingresp(struct mosquitto *mosq);


int send__puback(struct mosquitto *mosq, uint16_t mid, uint8_t reason_code);


int send__pubcomp(struct mosquitto *mosq, uint16_t mid);


int send__pubrec(struct mosquitto *mosq, uint16_t mid, uint8_t reason_code);


int send__pubrel(struct mosquitto *mosq, uint16_t mid);


int send__command_with_mid(struct mosquitto *mosq, uint8_t command, uint16_t mid, bool dup, uint8_t reason_code, const mosquitto_property *properties);


int send__simple_command(struct mosquitto *mosq, uint8_t command);
