#include "pub.h"



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
