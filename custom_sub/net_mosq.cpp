#include "sub.h"




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

// // int net__socket_connect_step3(struct mosquitto *mosq, const char *host)
// // {       
// // #ifdef WITH_TLS 
// //         BIO *bio;

// //         int rc = net__init_ssl_ctx(mosq);
// //         if(rc) return rc;

// //         if(mosq->ssl_ctx){
// //                 if(mosq->ssl){
// //                         SSL_free(mosq->ssl);
// //                 }
// //                 mosq->ssl = SSL_new(mosq->ssl_ctx);
// //                 if(!mosq->ssl){
// //                         COMPAT_CLOSE(mosq->sock);
// //                         mosq->sock = INVALID_SOCKET;
// //                         net__print_ssl_error(mosq);
// //                         return MOSQ_ERR_TLS;
// //                 }

// //                 SSL_set_ex_data(mosq->ssl, tls_ex_index_mosq, mosq);
// //                 bio = BIO_new_socket(mosq->sock, BIO_NOCLOSE);
// //                 if(!bio){
// //                         COMPAT_CLOSE(mosq->sock);
// //                         mosq->sock = INVALID_SOCKET;
// //                         net__print_ssl_error(mosq);
// //                         return MOSQ_ERR_TLS;
// //                 }
// //                 SSL_set_bio(mosq->ssl, bio, bio);

// //                 /*
// //                  * required for the SNI resolving
// //                  */
// //                 if(SSL_set_tlsext_host_name(mosq->ssl, host) != 1) {
// //                         COMPAT_CLOSE(mosq->sock);
// //                         mosq->sock = INVALID_SOCKET;
// //                         return MOSQ_ERR_TLS;
// //                 }

// //                 if(net__socket_connect_tls(mosq)){
// //                         return MOSQ_ERR_TLS;
// //                 }

// //         }
// // #endif
// //         return MOSQ_ERR_SUCCESS;
// // }