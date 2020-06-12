64      in ../sysdeps/unix/sysv/linux/x86_64/sigaction.c
(gdb) f
#0  __GI___libc_sigaction (sig=sig@entry=13, act=act@entry=0x7fffffffdb60, 
    oact=oact@entry=0x7fffffffdc00) at ../sysdeps/unix/sysv/linux/x86_64/sigaction.c:69
69      in ../sysdeps/unix/sysv/linux/x86_64/sigaction.c
(gdb) l
64      in ../sysdeps/unix/sysv/linux/x86_64/sigaction.c
(gdb) fin
Run till exit from #0  __GI___libc_sigaction (sig=sig@entry=13, 
    act=act@entry=0x7fffffffdb60, oact=oact@entry=0x7fffffffdc00)
    at ../sysdeps/unix/sysv/linux/x86_64/sigaction.c:69
__bsd_signal (sig=sig@entry=13, handler=handler@entry=0x1) at ../sysdeps/posix/signal.c:48
48      ../sysdeps/posix/signal.c: No such file or directory.
Value returned is $2 = 0
(gdb) l
43      in ../sysdeps/posix/signal.c
(gdb) fin
Run till exit from #0  __bsd_signal (sig=sig@entry=13, handler=handler@entry=0x1)
    at ../sysdeps/posix/signal.c:48
mosquitto_new (id=0x400eb4 "pub1", clean_start=<optimized out>, userdata=0x0)
    at mosquitto.c:92
92              mosq = (struct mosquitto *)mosquitto__calloc(1, sizeof(struct mosquitto));
Value returned is $3 = (void (*)(int)) 0x1
(gdb) l
87
88      #ifndef WIN32
89              signal(SIGPIPE, SIG_IGN);
90      #endif
91
92              mosq = (struct mosquitto *)mosquitto__calloc(1, sizeof(struct mosquitto));
93              if(mosq){
94                      mosq->sock = INVALID_SOCKET;
95                      mosq->sockpairR = INVALID_SOCKET;
96                      mosq->sockpairW = INVALID_SOCKET;
(gdb) s
mosquitto__calloc (nmemb=nmemb@entry=1, size=size@entry=968) at memory_mosq.c:55
55              void *mem = calloc(nmemb, size);
(gdb) bt
#0  mosquitto__calloc (nmemb=nmemb@entry=1, size=size@entry=968) at memory_mosq.c:55
#1  0x00007ffff7bc3c3f in mosquitto_new (id=0x400eb4 "pub1", clean_start=<optimized out>, 
    userdata=0x0) at mosquitto.c:92
#2  0x0000000000400ce7 in main () at Documentation/mos_pub.cpp:20
(gdb) s
__libc_calloc (n=1, elem_size=968) at malloc.c:3172
3172    malloc.c: No such file or directory.
(gdb) s
3187    in malloc.c
(gdb) l
3182    in malloc.c
(gdb) fin
Run till exit from #0  __libc_calloc (n=1, elem_size=968) at malloc.c:3187
mosquitto_new (id=0x400eb4 "pub1", clean_start=<optimized out>, userdata=0x0)
    at mosquitto.c:93
93              if(mosq){
Value returned is $4 = (void *) 0x633a30
(gdb) l
88      #ifndef WIN32
89              signal(SIGPIPE, SIG_IGN);
90      #endif
91
92              mosq = (struct mosquitto *)mosquitto__calloc(1, sizeof(struct mosquitto));
93              if(mosq){
94                      mosq->sock = INVALID_SOCKET;
95                      mosq->sockpairR = INVALID_SOCKET;
96                      mosq->sockpairW = INVALID_SOCKET;
97      #ifdef WITH_THREADING
(gdb) s
92              mosq = (struct mosquitto *)mosquitto__calloc(1, sizeof(struct mosquitto));
(gdb) s
93              if(mosq){
(gdb) s
94                      mosq->sock = INVALID_SOCKET;
(gdb) s
95                      mosq->sockpairR = INVALID_SOCKET;
(gdb) s
96                      mosq->sockpairW = INVALID_SOCKET;
(gdb) s
98                      mosq->thread_id = pthread_self();
(gdb) s
pthread_self () at forward.c:197
197     forward.c: No such file or directory.
(gdb) l
192     in forward.c
(gdb) fin
Run till exit from #0  pthread_self () at forward.c:197
mosquitto_new (id=0x400eb4 "pub1", clean_start=<optimized out>, userdata=0x0)
    at mosquitto.c:100
100                     rc = mosquitto_reinitialise(mosq, id, clean_start, userdata);
Value returned is $5 = 140737353926464
(gdb) l
95                      mosq->sockpairR = INVALID_SOCKET;
96                      mosq->sockpairW = INVALID_SOCKET;
97      #ifdef WITH_THREADING
98                      mosq->thread_id = pthread_self();
99      #endif
100                     rc = mosquitto_reinitialise(mosq, id, clean_start, userdata);
101                     if(rc){
102                             mosquitto_destroy(mosq);
103                             if(rc == MOSQ_ERR_INVAL){
104                                     errno = EINVAL;
(gdb) s
98                      mosq->thread_id = pthread_self();
(gdb) s
100                     rc = mosquitto_reinitialise(mosq, id, clean_start, userdata);
(gdb) s
mosquitto_reinitialise (mosq=0x633a30, id=0x400eb4 "pub1", clean_start=false, userdata=0x0)
    at mosquitto.c:118
118             if(!mosq) return MOSQ_ERR_INVAL;
(gdb) s
120             if(clean_start == false && id == NULL){
(gdb) s
117     {
(gdb) s
124             mosquitto__destroy(mosq);
(gdb) s
mosquitto__destroy (mosq=mosq@entry=0x633a30) at mosquitto.c:202
202             if(!mosq) return;
(gdb) s
200     {
(gdb) s
206             if(mosq->threaded == mosq_ts_self && !pthread_equal(mosq->thread_id, pthread_self())){
(gdb) s
213             if(mosq->id){
(gdb) s
228             if(mosq->sock != INVALID_SOCKET){
(gdb) s
231             message__cleanup_all(mosq);
(gdb) s
message__cleanup_all (mosq=mosq@entry=0x633a30) at messages_mosq.c:47
47      {
(gdb) s
50              assert(mosq);
(gdb) s
52              DL_FOREACH_SAFE(mosq->msgs_in.inflight, tail, tmp){
(gdb) s
56              DL_FOREACH_SAFE(mosq->msgs_out.inflight, tail, tmp){
(gdb) s
60      }
(gdb) s
mosquitto__destroy (mosq=mosq@entry=0x633a30) at mosquitto.c:232
232             will__clear(mosq);
(gdb) s
will__clear (mosq=mosq@entry=0x633a30) at will_mosq.c:111
111             if(!mosq->will) return MOSQ_ERR_SUCCESS;
(gdb) s
mosquitto__destroy (mosq=mosq@entry=0x633a30) at mosquitto.c:234
234             if(mosq->ssl){
(gdb) s
237             if(mosq->ssl_ctx){
(gdb) s
240             mosquitto__free(mosq->tls_cafile);
(gdb) s
mosquitto__free (mem=0x0) at memory_mosq.c:77
77              free(mem);
(gdb) s
__GI___libc_free (mem=0x0) at malloc.c:2934
2934    malloc.c: No such file or directory.
(gdb) s
2939    in malloc.c
(gdb) s
2940    in malloc.c
(gdb) f
#0  __GI___libc_free (mem=0x0) at malloc.c:2940
2940    in malloc.c
(gdb) s
2946    in malloc.c
(gdb) fin
Run till exit from #0  __GI___libc_free (mem=0x0) at malloc.c:2946
mosquitto__destroy (mosq=mosq@entry=0x633a30) at mosquitto.c:241
241             mosquitto__free(mosq->tls_capath);
(gdb) s
mosquitto__free (mem=0x0) at memory_mosq.c:77
77              free(mem);
(gdb) s
__GI___libc_free (mem=0x0) at malloc.c:2934
2934    malloc.c: No such file or directory.
(gdb) fin
Run till exit from #0  __GI___libc_free (mem=0x0) at malloc.c:2934
mosquitto__destroy (mosq=mosq@entry=0x633a30) at mosquitto.c:242
242             mosquitto__free(mosq->tls_certfile);
(gdb) s
mosquitto__free (mem=0x0) at memory_mosq.c:77
77              free(mem);
(gdb) s
__GI___libc_free (mem=0x0) at malloc.c:2934
2934    malloc.c: No such file or directory.
(gdb) s
2939    in malloc.c
(gdb) s
2940    in malloc.c
(gdb) sfin
Undefined command: "sfin".  Try "help".
(gdb) fin
Run till exit from #0  __GI___libc_free (mem=0x0) at malloc.c:2940
mosquitto__destroy (mosq=mosq@entry=0x633a30) at mosquitto.c:243
243             mosquitto__free(mosq->tls_keyfile);
(gdb) s
mosquitto__free (mem=0x0) at memory_mosq.c:77
77              free(mem);
(gdb) s
__GI___libc_free (mem=0x0) at malloc.c:2934
2934    malloc.c: No such file or directory.
(gdb) s
2939    in malloc.c
(gdb) fin
Run till exit from #0  __GI___libc_free (mem=0x0) at malloc.c:2939
mosquitto__destroy (mosq=mosq@entry=0x633a30) at mosquitto.c:244
244             if(mosq->tls_pw_callback) mosq->tls_pw_callback = NULL;
(gdb) l
239             }
240             mosquitto__free(mosq->tls_cafile);
241             mosquitto__free(mosq->tls_capath);
242             mosquitto__free(mosq->tls_certfile);
243             mosquitto__free(mosq->tls_keyfile);
244             if(mosq->tls_pw_callback) mosq->tls_pw_callback = NULL;
245             mosquitto__free(mosq->tls_version);
246             mosquitto__free(mosq->tls_ciphers);
247             mosquitto__free(mosq->tls_psk);
248             mosquitto__free(mosq->tls_psk_identity);
(gdb) fin
Run till exit from #0  mosquitto__destroy (mosq=mosq@entry=0x633a30) at mosquitto.c:244
mosquitto_reinitialise (mosq=0x633a30, id=0x400eb4 "pub1", clean_start=<optimized out>, 
    userdata=0x0) at mosquitto.c:125
125             memset(mosq, 0, sizeof(struct mosquitto));
(gdb) s
memset (__len=968, __ch=0, __dest=0x633a30)
    at /usr/include/x86_64-linux-gnu/bits/string3.h:90
90        return __builtin___memset_chk (__dest, __ch, __len, __bos0 (__dest));
(gdb) l
85          {
86            __warn_memset_zero_len ();
87            return __dest;
88          }
89      #endif
90        return __builtin___memset_chk (__dest, __ch, __len, __bos0 (__dest));
91      }
92
93      #ifdef __USE_MISC
94      __fortify_function void
(gdb) fin
Run till exit from #0  memset (__len=968, __ch=0, __dest=0x633a30)
    at /usr/include/x86_64-linux-gnu/bits/string3.h:90
mosquitto_reinitialise (mosq=0x633a30, id=0x400eb4 "pub1", clean_start=<optimized out>, 
    userdata=0x0) at mosquitto.c:136
136             mosq->keepalive = 60;
(gdb) l
131             }
132             mosq->protocol = mosq_p_mqtt311;
133             mosq->sock = INVALID_SOCKET;
134             mosq->sockpairR = INVALID_SOCKET;
135             mosq->sockpairW = INVALID_SOCKET;
136             mosq->keepalive = 60;
137             mosq->clean_start = clean_start;
138             if(id){
139                     if(STREMPTY(id)){
140                             return MOSQ_ERR_INVAL;
(gdb) s
125             memset(mosq, 0, sizeof(struct mosquitto));
(gdb) s
memset (__len=968, __ch=0, __dest=0x633a30)
    at /usr/include/x86_64-linux-gnu/bits/string3.h:90
90        return __builtin___memset_chk (__dest, __ch, __len, __bos0 (__dest));
(gdb) s
mosquitto_reinitialise (mosq=0x633a30, id=0x400eb4 "pub1", clean_start=<optimized out>, 
    userdata=0x0) at mosquitto.c:128
128                     mosq->userdata = userdata;
(gdb) s
125             memset(mosq, 0, sizeof(struct mosquitto));
(gdb) s
memset (__len=968, __ch=0, __dest=0x633a30)
    at /usr/include/x86_64-linux-gnu/bits/string3.h:90
90        return __builtin___memset_chk (__dest, __ch, __len, __bos0 (__dest));
(gdb) s
mosquitto_reinitialise (mosq=0x633a30, id=0x400eb4 "pub1", clean_start=<optimized out>, 
    userdata=0x0) at mosquitto.c:128
128                     mosq->userdata = userdata;
(gdb) s
132             mosq->protocol = mosq_p_mqtt311;
(gdb) s
133             mosq->sock = INVALID_SOCKET;
(gdb) s
128                     mosq->userdata = userdata;
(gdb) s
138             if(id){
(gdb) s
134             mosq->sockpairR = INVALID_SOCKET;
(gdb) s
128                     mosq->userdata = userdata;
(gdb) s
135             mosq->sockpairW = INVALID_SOCKET;
(gdb) s
136             mosq->keepalive = 60;
(gdb) s
137             mosq->clean_start = clean_start;
(gdb) s
138             if(id){
(gdb) s
139                     if(STREMPTY(id)){
(gdb) s
118             if(!mosq) return MOSQ_ERR_INVAL;
(gdb) s
139                     if(STREMPTY(id)){
(gdb) s
142                     if(mosquitto_validate_utf8(id, strlen(id))){
(gdb) s
strlen () at ../sysdeps/x86_64/strlen.S:66
66      ../sysdeps/x86_64/strlen.S: No such file or directory.
(gdb) s
67      in ../sysdeps/x86_64/strlen.S
(gdb) s
68      in ../sysdeps/x86_64/strlen.S
(gdb) fin
Run till exit from #0  strlen () at ../sysdeps/x86_64/strlen.S:68
0x00007ffff7bc39e0 in mosquitto_reinitialise (mosq=0x633a30, id=0x400eb4 "pub1", 
    clean_start=<optimized out>, userdata=0x0) at mosquitto.c:142
142                     if(mosquitto_validate_utf8(id, strlen(id))){
(gdb) s
mosquitto_validate_utf8 (str=str@entry=0x400eb4 "pub1", len=4) at utf8_mosq.c:30
30              if(!str) return MOSQ_ERR_INVAL;
(gdb) s
31              if(len < 0 || len > 65536) return MOSQ_ERR_INVAL;
(gdb) s
33              for(i=0; i<len; i++){
(gdb) s
34                      if(ustr[i] == 0){
(gdb) s
23      {
(gdb) s
34                      if(ustr[i] == 0){
(gdb) s
36                      }else if(ustr[i] <= 0x7f){
(gdb) s
65                      if(i == len-codelen+1){
(gdb) s
38                              codepoint = ustr[i];
(gdb) s
78                      if(codepoint >= 0xD800 && codepoint <= 0xDFFF){
(gdb) s
96                      if(codepoint >= 0xFDD0 && codepoint <= 0xFDEF){
(gdb) s
99                      if((codepoint & 0xFFFF) == 0xFFFE || (codepoint & 0xFFFF) == 0xFFFF){
(gdb) s
103                     if(codepoint <= 0x001F || (codepoint >= 0x007F && codepoint <= 0x009F)){
(gdb) s
33              for(i=0; i<len; i++){
(gdb) s
34                      if(ustr[i] == 0){
(gdb) s
36                      }else if(ustr[i] <= 0x7f){
(gdb) s
65                      if(i == len-codelen+1){
(gdb) s
38                              codepoint = ustr[i];
(gdb) s
78                      if(codepoint >= 0xD800 && codepoint <= 0xDFFF){
(gdb) s
96                      if(codepoint >= 0xFDD0 && codepoint <= 0xFDEF){
(gdb) s
99                      if((codepoint & 0xFFFF) == 0xFFFE || (codepoint & 0xFFFF) == 0xFFFF){
(gdb) s
103                     if(codepoint <= 0x001F || (codepoint >= 0x007F && codepoint <= 0x009F)){
(gdb) s
33              for(i=0; i<len; i++){
(gdb) s
34                      if(ustr[i] == 0){
(gdb) s
36                      }else if(ustr[i] <= 0x7f){
(gdb) s
65                      if(i == len-codelen+1){
(gdb) s
38                              codepoint = ustr[i];
(gdb) s
78                      if(codepoint >= 0xD800 && codepoint <= 0xDFFF){
(gdb) s
96                      if(codepoint >= 0xFDD0 && codepoint <= 0xFDEF){
(gdb) s
99                      if((codepoint & 0xFFFF) == 0xFFFE || (codepoint & 0xFFFF) == 0xFFFF){
(gdb) s
103                     if(codepoint <= 0x001F || (codepoint >= 0x007F && codepoint <= 0x009F)){
(gdb) s
33              for(i=0; i<len; i++){
(gdb) s
34                      if(ustr[i] == 0){
(gdb) s
36                      }else if(ustr[i] <= 0x7f){
(gdb) s
65                      if(i == len-codelen+1){
(gdb) s
38                              codepoint = ustr[i];
(gdb) s
78                      if(codepoint >= 0xD800 && codepoint <= 0xDFFF){
(gdb) s
96                      if(codepoint >= 0xFDD0 && codepoint <= 0xFDEF){
(gdb) s
99                      if((codepoint & 0xFFFF) == 0xFFFE || (codepoint & 0xFFFF) == 0xFFFF){
(gdb) s
103                     if(codepoint <= 0x001F || (codepoint >= 0x007F && codepoint <= 0x009F)){
(gdb) s
33              for(i=0; i<len; i++){
(gdb) s
107             return MOSQ_ERR_SUCCESS;
(gdb) s
108     }
(gdb) s
mosquitto_reinitialise (mosq=0x633a30, id=0x400eb4 "pub1", clean_start=<optimized out>, 
    userdata=0x0) at mosquitto.c:143
143                             return MOSQ_ERR_MALFORMED_UTF8;
(gdb) s
142                     if(mosquitto_validate_utf8(id, strlen(id))){
(gdb) s
145                     mosq->id = mosquitto__strdup(id);
(gdb) s
mosquitto__strdup (s=0x400eb4 "pub1") at memory_mosq.c:147
147             char *str = strdup(s);
(gdb) s
__GI___strdup (s=0x400eb4 "pub1") at strdup.c:40
40      strdup.c: No such file or directory.
(gdb) s
41      in strdup.c
(gdb) s
strlen () at ../sysdeps/x86_64/strlen.S:66
66      ../sysdeps/x86_64/strlen.S: No such file or directory.
(gdb) s
67      in ../sysdeps/x86_64/strlen.S
(gdb) s
68      in ../sysdeps/x86_64/strlen.S
(gdb) s
69      in ../sysdeps/x86_64/strlen.S
(gdb) fin
Run till exit from #0  strlen () at ../sysdeps/x86_64/strlen.S:69
0x00007ffff74fd47e in __GI___strdup (s=0x400eb4 "pub1") at strdup.c:41
41      strdup.c: No such file or directory.
(gdb) s
42      in strdup.c
(gdb) s
44      in strdup.c
(gdb) fin
Run till exit from #0  __GI___strdup (s=0x400eb4 "pub1") at strdup.c:44
0x00007ffff7bc39fd in mosquitto_reinitialise (mosq=0x633a30, id=0x400eb4 "pub1", 
    clean_start=<optimized out>, userdata=0x0) at mosquitto.c:145
145                     mosq->id = mosquitto__strdup(id);
Value returned is $6 = 0x6383f0 "pub1"
(gdb) s
148             packet__cleanup(&mosq->in_packet);
(gdb) 
147             mosq->in_packet.payload = NULL;
(gdb) 
148             packet__cleanup(&mosq->in_packet);
(gdb) 
packet__cleanup (packet=packet@entry=0x633a80) at packet_mosq.c:89
89              if(!packet) return;
(gdb) 
88      {
(gdb) 
92              packet->command = 0;
(gdb) 
93              packet->remaining_count = 0;
(gdb) l
88      {
89              if(!packet) return;
90
91              /* Free data and reset values */
92              packet->command = 0;
93              packet->remaining_count = 0;
94              packet->remaining_mult = 1;
95              packet->remaining_length = 0;
96              mosquitto__free(packet->payload);
97              packet->payload = NULL;
(gdb) 
98              packet->to_process = 0;
99              packet->pos = 0;
100     }
101
102
103     void packet__cleanup_all(struct mosquitto *mosq)
104     {
105             struct mosquitto__packet *packet;
106
107             pthread_mutex_lock(&mosq->current_out_packet_mutex);
(gdb) 
108             pthread_mutex_lock(&mosq->out_packet_mutex);
109
110             /* Out packet cleanup */
111             if(mosq->out_packet && !mosq->current_out_packet){
112                     mosq->current_out_packet = mosq->out_packet;
113                     mosq->out_packet = mosq->out_packet->next;
114             }
115             while(mosq->current_out_packet){
116                     packet = mosq->current_out_packet;
117                     /* Free data and reset values */
(gdb) 
118                     mosq->current_out_packet = mosq->out_packet;
119                     if(mosq->out_packet){
120                             mosq->out_packet = mosq->out_packet->next;
121                     }
122
123                     packet__cleanup(packet);
124                     mosquitto__free(packet);
125             }
126
127             packet__cleanup(&mosq->in_packet);
(gdb) s
94              packet->remaining_mult = 1;
(gdb) s
95              packet->remaining_length = 0;
(gdb) 
96              mosquitto__free(packet->payload);
(gdb) 
mosquitto__free (mem=0x0) at memory_mosq.c:77
77              free(mem);
(gdb) s
__GI___libc_free (mem=0x0) at malloc.c:2934
2934    malloc.c: No such file or directory.
(gdb) fin
Run till exit from #0  __GI___libc_free (mem=0x0) at malloc.c:2934
packet__cleanup (packet=packet@entry=0x633a80) at packet_mosq.c:97
97              packet->payload = NULL;
(gdb) s
98              packet->to_process = 0;
(gdb) 
99              packet->pos = 0;
(gdb) 
100     }
(gdb) 
mosquitto_reinitialise (mosq=0x633a30, id=0x400eb4 "pub1", clean_start=<optimized out>, 
    userdata=0x0) at mosquitto.c:149
149             mosq->out_packet = NULL;
(gdb) 
150             mosq->current_out_packet = NULL;
(gdb) 
151             mosq->last_msg_in = mosquitto_time();
(gdb) 
mosquitto_time () at time_mosq.c:36
36      {
(gdb) 
42              clock_gettime(CLOCK_MONOTONIC, &tp);
(gdb) fin
Run till exit from #0  mosquitto_time () at time_mosq.c:42
0x00007ffff7bc3a2a in mosquitto_reinitialise (mosq=0x633a30, id=0x400eb4 "pub1", 
    clean_start=<optimized out>, userdata=0x0) at mosquitto.c:151
151             mosq->last_msg_in = mosquitto_time();
Value returned is $7 = 67412
(gdb) s
152             mosq->next_msg_out = mosquitto_time() + mosq->keepalive;
(gdb) 
mosquitto_time () at time_mosq.c:36
36      {
(gdb) 
42              clock_gettime(CLOCK_MONOTONIC, &tp);
(gdb) 
36      {
(gdb) 
42              clock_gettime(CLOCK_MONOTONIC, &tp);
(gdb) 
__GI___clock_gettime (clock_id=1, tp=0x7fffffffdc50) at ../sysdeps/unix/clock_gettime.c:93
93      ../sysdeps/unix/clock_gettime.c: No such file or directory.
(gdb) 
115     in ../sysdeps/unix/clock_gettime.c
(gdb) 
134     in ../sysdeps/unix/clock_gettime.c
(gdb) fin
Run till exit from #0  __GI___clock_gettime (clock_id=1, tp=0x7fffffffdc50)
    at ../sysdeps/unix/clock_gettime.c:134
mosquitto_time () at time_mosq.c:60
60      }
Value returned is $8 = 0
(gdb) s
mosquitto_reinitialise (mosq=0x633a30, id=0x400eb4 "pub1", clean_start=<optimized out>, 
    userdata=0x0) at mosquitto.c:158
158             mosq->msgs_out.inflight_maximum = 20;
(gdb) 
183             pthread_mutex_init(&mosq->callback_mutex, NULL);
(gdb) 
158             mosq->msgs_out.inflight_maximum = 20;
(gdb) 
183             pthread_mutex_init(&mosq->callback_mutex, NULL);
(gdb) 
153             mosq->ping_t = 0;
(gdb) 
155             mosq->state = mosq_cs_new;
(gdb) 
156             mosq->maximum_qos = 2;
(gdb) 
159             mosq->msgs_in.inflight_quota = 20;
(gdb) 
160             mosq->msgs_out.inflight_quota = 20;
(gdb) 
152             mosq->next_msg_out = mosquitto_time() + mosq->keepalive;
(gdb) 
157             mosq->msgs_in.inflight_maximum = 20;
(gdb) 
161             mosq->will = NULL;
(gdb) 
152             mosq->next_msg_out = mosquitto_time() + mosq->keepalive;
(gdb) 
154             mosq->last_mid = 0;
(gdb) 
157             mosq->msgs_in.inflight_maximum = 20;
(gdb) 
154             mosq->last_mid = 0;
(gdb) 
162             mosq->on_connect = NULL;
(gdb) 
163             mosq->on_publish = NULL;
(gdb) 
164             mosq->on_message = NULL;
(gdb) 
165             mosq->on_subscribe = NULL;
(gdb) 
166             mosq->on_unsubscribe = NULL;
(gdb) 
167             mosq->host = NULL;
(gdb) 
168             mosq->port = 1883;
(gdb) 
169             mosq->in_callback = false;
(gdb) 
170             mosq->reconnect_delay = 1;
(gdb) 
171             mosq->reconnect_delay_max = 1;
(gdb) 
172             mosq->reconnect_exponential_backoff = false;
(gdb) 
173             mosq->threaded = mosq_ts_none;
(gdb) 
175             mosq->ssl = NULL;
(gdb) 
176             mosq->ssl_ctx = NULL;
(gdb) 
177             mosq->tls_cert_reqs = SSL_VERIFY_PEER;
(gdb) 
178             mosq->tls_insecure = false;
(gdb) 
179             mosq->want_write = false;
(gdb) 
180             mosq->tls_ocsp_required = false;
(gdb) 
183             pthread_mutex_init(&mosq->callback_mutex, NULL);
(gdb) 
pthread_mutex_init (mutex=mutex@entry=0x633b70, mutexattr=mutexattr@entry=0x0)
    at forward.c:188
188     forward.c: No such file or directory.
(gdb) 
__GI___pthread_mutex_init (mutex=0x633b70, mutexattr=0x0) at pthread_mutex_init.c:60
60      pthread_mutex_init.c: No such file or directory.
(gdb) s
66      in pthread_mutex_init.c
(gdb) 
90      in pthread_mutex_init.c
(gdb) s
93      in pthread_mutex_init.c
(gdb) fin
Run till exit from #0  __GI___pthread_mutex_init (mutex=0x633b70, mutexattr=<optimized out>)
    at pthread_mutex_init.c:93
mosquitto_reinitialise (mosq=0x633a30, id=0x400eb4 "pub1", clean_start=<optimized out>, 
    userdata=0x0) at mosquitto.c:184
184             pthread_mutex_init(&mosq->log_callback_mutex, NULL);
Value returned is $9 = 0
(gdb) s
pthread_mutex_init (mutex=mutex@entry=0x633b98, mutexattr=mutexattr@entry=0x0)
    at forward.c:188
188     forward.c: No such file or directory.
(gdb) 
__GI___pthread_mutex_init (mutex=0x633b98, mutexattr=0x0) at pthread_mutex_init.c:60
60      pthread_mutex_init.c: No such file or directory.
(gdb) 
66      in pthread_mutex_init.c
(gdb) 
90      in pthread_mutex_init.c
(gdb) fin
Run till exit from #0  __GI___pthread_mutex_init (mutex=0x633b98, mutexattr=<optimized out>)
    at pthread_mutex_init.c:90
mosquitto_reinitialise (mosq=0x633a30, id=0x400eb4 "pub1", clean_start=<optimized out>, 
    userdata=0x0) at mosquitto.c:185
185             pthread_mutex_init(&mosq->state_mutex, NULL);
Value returned is $10 = 0
(gdb) s
pthread_mutex_init (mutex=mutex@entry=0x633c38, mutexattr=mutexattr@entry=0x0)
    at forward.c:188
188     forward.c: No such file or directory.
(gdb) s
__GI___pthread_mutex_init (mutex=0x633c38, mutexattr=0x0) at pthread_mutex_init.c:60
60      pthread_mutex_init.c: No such file or directory.
(gdb) s
66      in pthread_mutex_init.c
(gdb) s
90      in pthread_mutex_init.c
(gdb) s
93      in pthread_mutex_init.c
(gdb) s
95      in pthread_mutex_init.c
(gdb) fin
Run till exit from #0  __GI___pthread_mutex_init (mutex=0x633c38, mutexattr=<optimized out>)
    at pthread_mutex_init.c:95
mosquitto_reinitialise (mosq=0x633a30, id=0x400eb4 "pub1", clean_start=<optimized out>, 
    userdata=0x0) at mosquitto.c:186
186             pthread_mutex_init(&mosq->out_packet_mutex, NULL);
Value returned is $11 = 0
(gdb) l
181     #endif
182     #ifdef WITH_THREADING
183             pthread_mutex_init(&mosq->callback_mutex, NULL);
184             pthread_mutex_init(&mosq->log_callback_mutex, NULL);
185             pthread_mutex_init(&mosq->state_mutex, NULL);
186             pthread_mutex_init(&mosq->out_packet_mutex, NULL);
187             pthread_mutex_init(&mosq->current_out_packet_mutex, NULL);
188             pthread_mutex_init(&mosq->msgtime_mutex, NULL);
189             pthread_mutex_init(&mosq->msgs_in.mutex, NULL);
190             pthread_mutex_init(&mosq->msgs_out.mutex, NULL);
(gdb) s
pthread_mutex_init (mutex=mutex@entry=0x633be8, mutexattr=mutexattr@entry=0x0)
    at forward.c:188
188     forward.c: No such file or directory.
(gdb) fin
Run till exit from #0  pthread_mutex_init (mutex=mutex@entry=0x633be8, 
    mutexattr=mutexattr@entry=0x0) at forward.c:188
mosquitto_reinitialise (mosq=0x633a30, id=0x400eb4 "pub1", clean_start=<optimized out>, 
    userdata=0x0) at mosquitto.c:187
187             pthread_mutex_init(&mosq->current_out_packet_mutex, NULL);
Value returned is $12 = 0
(gdb) 
Run till exit from #0  mosquitto_reinitialise (mosq=0x633a30, id=0x400eb4 "pub1", 
    clean_start=<optimized out>, userdata=0x0) at mosquitto.c:187
mosquitto_new (id=0x400eb4 "pub1", clean_start=<optimized out>, userdata=0x0)
    at mosquitto.c:101
101                     if(rc){
Value returned is $13 = 0
(gdb) s
100                     rc = mosquitto_reinitialise(mosq, id, clean_start, userdata);
(gdb) 
101                     if(rc){
(gdb) l
96                      mosq->sockpairW = INVALID_SOCKET;
97      #ifdef WITH_THREADING
98                      mosq->thread_id = pthread_self();
99      #endif
100                     rc = mosquitto_reinitialise(mosq, id, clean_start, userdata);
101                     if(rc){
102                             mosquitto_destroy(mosq);
103                             if(rc == MOSQ_ERR_INVAL){
104                                     errno = EINVAL;
105                             }else if(rc == MOSQ_ERR_NOMEM){
(gdb) s
114     }
(gdb) 
main () at Documentation/mos_pub.cpp:21
21          mosquitto_loop_start(mosq);
(gdb) 
mosquitto_loop_start (mosq=0x633a30) at thread_mosq.c:32
32              if(!mosq || mosq->threaded != mosq_ts_none) return MOSQ_ERR_INVAL;
(gdb) 
30      {
(gdb) 
35              if(!pthread_create(&mosq->thread_id, NULL, mosquitto__thread_main, mosq)){
(gdb) 
34              mosq->threaded = mosq_ts_self;
(gdb) 
35              if(!pthread_create(&mosq->thread_id, NULL, mosquitto__thread_main, mosq)){
(gdb) 
__pthread_create_2_1 (newthread=0x633c88, attr=attr@entry=0x0, 
    start_routine=start_routine@entry=0x7ffff7bcef30 <mosquitto__thread_main>, arg=0x633a30)
    at pthread_create.c:511
511     pthread_create.c: No such file or directory.
(gdb) 
505     in pthread_create.c
(gdb) fin
Run till exit from #0  __pthread_create_2_1 (newthread=0x633c88, attr=attr@entry=0x0, 
    start_routine=start_routine@entry=0x7ffff7bcef30 <mosquitto__thread_main>, arg=0x633a30)
    at pthread_create.c:505
[New Thread 0x7ffff627b700 (LWP 21599)]
mosquitto_loop_start (mosq=<optimized out>) at thread_mosq.c:38
38                      return MOSQ_ERR_ERRNO;
Value returned is $14 = 0
(gdb) s
43      }
(gdb) 
main () at Documentation/mos_pub.cpp:23
23          mosquitto_connect(mosq, host, port, 60);
(gdb) 
mosquitto_connect (mosq=0x633a30, host=0x400eb9 "0.0.0.0", port=1883, keepalive=60)
    at connect.c:102
102             return mosquitto_connect_bind(mosq, host, port, keepalive, NULL);
(gdb) s
mosquitto_connect_bind (mosq=0x633a30, host=0x400eb9 "0.0.0.0", port=1883, keepalive=60, 
    bind_address=0x0) at connect.c:108
108             return mosquitto_connect_bind_v5(mosq, host, port, keepalive, bind_address, NULL);
(gdb) 
mosquitto_connect_bind_v5 (mosq=0x633a30, host=0x400eb9 "0.0.0.0", port=1883, keepalive=60, 
    bind_address=0x0, properties=0x0) at connect.c:112
112     {
(gdb) 
115             if(properties){
(gdb) 
120             rc = mosquitto__connect_init(mosq, host, port, keepalive, bind_address);
(gdb) 
mosquitto__connect_init (bind_address=0x0, keepalive=60, port=1883, host=0x400eb9 "0.0.0.0", 
    mosq=0x633a30) at connect.c:44
44              if(!mosq) return MOSQ_ERR_INVAL;
(gdb) 
46              if(keepalive < 5) return MOSQ_ERR_INVAL;
(gdb) 
mosquitto__connect_init (mosq=mosq@entry=0x633a30, host=host@entry=0x400eb9 "0.0.0.0", 
    port=port@entry=1883, keepalive=keepalive@entry=60, bind_address=bind_address@entry=0x0)
    at connect.c:39
39      static int mosquitto__connect_init(struct mosquitto *mosq, const char *host, int port, int keepalive, const char *bind_address)
(gdb) l
34
35      static int mosquitto__reconnect(struct mosquitto *mosq, bool blocking, const mosquitto_property *properties);
36      static int mosquitto__connect_init(struct mosquitto *mosq, const char *host, int port, int keepalive, const char *bind_address);
37
38
39      static int mosquitto__connect_init(struct mosquitto *mosq, const char *host, int port, int keepalive, const char *bind_address)
40      {
41              int i;
42              int rc;
43
(gdb) 
44              if(!mosq) return MOSQ_ERR_INVAL;
45              if(!host || port <= 0) return MOSQ_ERR_INVAL;
46              if(keepalive < 5) return MOSQ_ERR_INVAL;
47
48              if(mosq->id == NULL && (mosq->protocol == mosq_p_mqtt31 || mosq->protocol == mosq_p_mqtt311)){
49                      mosq->id = (char *)mosquitto__calloc(24, sizeof(char));
50                      if(!mosq->id){
51                              return MOSQ_ERR_NOMEM;
52                      }
53                      mosq->id[0] = 'm';
(gdb) s
48              if(mosq->id == NULL && (mosq->protocol == mosq_p_mqtt31 || mosq->protocol == mosq_p_mqtt311)){
(gdb) 
39      static int mosquitto__connect_init(struct mosquitto *mosq, const char *host, int port, int keepalive, const char *bind_address)
(gdb) 
48              if(mosq->id == NULL && (mosq->protocol == mosq_p_mqtt31 || mosq->protocol == mosq_p_mqtt311)){
(gdb) 
67              mosquitto__free(mosq->host);
(gdb) 
mosquitto__free (mem=0x0) at memory_mosq.c:77
77              free(mem);
(gdb) 
__GI___libc_free (mem=0x0) at malloc.c:2934
2934    malloc.c: No such file or directory.
(gdb) fin
Run till exit from #0  __GI___libc_free (mem=0x0) at malloc.c:2934
mosquitto__connect_init (mosq=mosq@entry=0x633a30, host=host@entry=0x400eb9 "0.0.0.0", 
    port=port@entry=1883, keepalive=keepalive@entry=60, bind_address=bind_address@entry=0x0)
    at connect.c:68
68              mosq->host = mosquitto__strdup(host);
(gdb) s
mosquitto__strdup (s=0x400eb9 "0.0.0.0") at memory_mosq.c:147
147             char *str = strdup(s);
(gdb) 
__GI___strdup (s=0x400eb9 "0.0.0.0") at strdup.c:40
40      strdup.c: No such file or directory.
(gdb) fin
Run till exit from #0  __GI___strdup (s=0x400eb9 "0.0.0.0") at strdup.c:40
mosquitto__connect_init (mosq=mosq@entry=0x633a30, host=host@entry=0x400eb9 "0.0.0.0", 
    port=port@entry=1883, keepalive=keepalive@entry=60, bind_address=bind_address@entry=0x0)
    at connect.c:69
69              if(!mosq->host) return MOSQ_ERR_NOMEM;
Value returned is $15 = 0x638030 "0.0.0.0"
(gdb) s
68              mosq->host = mosquitto__strdup(host);
(gdb) 
69              if(!mosq->host) return MOSQ_ERR_NOMEM;
(gdb) 
72              mosquitto__free(mosq->bind_address);
(gdb) 
70              mosq->port = port;
(gdb) 
72              mosquitto__free(mosq->bind_address);
(gdb) s
mosquitto__free (mem=0x0) at memory_mosq.c:77
77              free(mem);
(gdb) fin
Run till exit from #0  mosquitto__free (mem=0x0) at memory_mosq.c:77
mosquitto__connect_init (mosq=mosq@entry=0x633a30, host=host@entry=0x400eb9 "0.0.0.0", 
    port=port@entry=1883, keepalive=keepalive@entry=60, bind_address=bind_address@entry=0x0)
    at connect.c:73
73              if(bind_address){
(gdb) 
Run till exit from #0  mosquitto__connect_init (mosq=mosq@entry=0x633a30, 
    host=host@entry=0x400eb9 "0.0.0.0", port=port@entry=1883, keepalive=keepalive@entry=60, 
    bind_address=bind_address@entry=0x0) at connect.c:73
mosquitto_connect_bind_v5 (mosq=0x633a30, host=0x400eb9 "0.0.0.0", port=1883, keepalive=60, 
    bind_address=0x0, properties=0x0) at connect.c:121
121             if(rc) return rc;
Value returned is $16 = 0
(gdb) s
123             mosquitto__set_state(mosq, mosq_cs_new);
(gdb) l
118             }
119
120             rc = mosquitto__connect_init(mosq, host, port, keepalive, bind_address);
121             if(rc) return rc;
122
123             mosquitto__set_state(mosq, mosq_cs_new);
124
125             return mosquitto__reconnect(mosq, true, properties);
126     }
127
(gdb) s
mosquitto__set_state (mosq=mosq@entry=0x633a30, state=state@entry=mosq_cs_new)
    at util_mosq.c:273
273     {
(gdb) s
274             pthread_mutex_lock(&mosq->state_mutex);
(gdb) l
269     }
270
271
272     int mosquitto__set_state(struct mosquitto *mosq, enum mosquitto_client_state state)
273     {
274             pthread_mutex_lock(&mosq->state_mutex);
275     #ifdef WITH_BROKER
276             if(mosq->state != mosq_cs_disused)
277     #endif
278             {
(gdb) fin
Run till exit from #0  mosquitto__set_state (mosq=mosq@entry=0x633a30, 
    state=state@entry=mosq_cs_new) at util_mosq.c:274
mosquitto_connect_bind_v5 (mosq=0x633a30, host=0x400eb9 "0.0.0.0", port=1883, keepalive=60, 
    bind_address=0x0, properties=0x0) at connect.c:126
126     }
Value returned is $17 = 0
(gdb) s
125             return mosquitto__reconnect(mosq, true, properties);
(gdb) s
126     }
(gdb) s
125             return mosquitto__reconnect(mosq, true, properties);
(gdb) s
mosquitto__reconnect (mosq=0x633a30, blocking=true, properties=0x0) at connect.c:157
157     {
(gdb) s
162             if(!mosq) return MOSQ_ERR_INVAL;
(gdb) s
163             if(!mosq->host || mosq->port <= 0) return MOSQ_ERR_INVAL;
(gdb) s
164             if(mosq->protocol != mosq_p_mqtt5 && properties) return MOSQ_ERR_NOT_SUPPORTED;
(gdb) s
179             pthread_mutex_lock(&mosq->msgtime_mutex);
(gdb) s
pthread_mutex_lock (mutex=mutex@entry=0x633bc0) at forward.c:192
192     forward.c: No such file or directory.
(gdb) s
__GI___pthread_mutex_lock (mutex=0x633bc0) at ../nptl/pthread_mutex_lock.c:64
64      ../nptl/pthread_mutex_lock.c: No such file or directory.
(gdb) s
67      in ../nptl/pthread_mutex_lock.c
(gdb) s
69      in ../nptl/pthread_mutex_lock.c
(gdb) s
71      in ../nptl/pthread_mutex_lock.c
(gdb) fin
Run till exit from #0  __GI___pthread_mutex_lock (mutex=0x633bc0)
    at ../nptl/pthread_mutex_lock.c:71
mosquitto__reconnect (mosq=0x633a30, blocking=<optimized out>, properties=<optimized out>)
    at connect.c:180
180             mosq->last_msg_in = mosquitto_time();
Value returned is $18 = 0
(gdb) s
mosquitto_time () at time_mosq.c:36
36      {
(gdb) s
42              clock_gettime(CLOCK_MONOTONIC, &tp);
(gdb) fin
Run till exit from #0  mosquitto_time () at time_mosq.c:42
mosquitto__reconnect (mosq=0x633a30, blocking=<optimized out>, properties=<optimized out>)
    at connect.c:181
181             mosq->next_msg_out = mosq->last_msg_in + mosq->keepalive;
Value returned is $19 = 67671
(gdb) s
180             mosq->last_msg_in = mosquitto_time();
(gdb) s
182             pthread_mutex_unlock(&mosq->msgtime_mutex);
(gdb) fin
Run till exit from #0  mosquitto__reconnect (mosq=0x633a30, blocking=<optimized out>, 
    properties=<optimized out>) at connect.c:182
main () at Documentation/mos_pub.cpp:25
25          cout<<"hello"<<endl;
Value returned is $20 = 0
(gdb) l
20          mosq=mosquitto_new("pub1", 0, NULL);
21          mosquitto_loop_start(mosq);
22
23          mosquitto_connect(mosq, host, port, 60);
24          
25          cout<<"hello"<<endl;
26          int res=mosquitto_publish(
27              mosq,
28              NULL,
29              "topic/test",
 
