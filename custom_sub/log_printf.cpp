#include "sub.h"


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