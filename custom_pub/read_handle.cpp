#include "pub.h"




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