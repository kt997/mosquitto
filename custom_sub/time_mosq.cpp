#include "sub.h"



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