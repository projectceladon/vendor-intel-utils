// Copyright (C) 2018-2022 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions
// and limitations under the License.
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __TIME_LOG_H__
#define __TIME_LOG_H__

#include <stdlib.h>
#include <time.h>
#include <sys/time.h>

#include <string>

#define IRR_TIME_LOG_DIFF_THRESHOLD_US  (0.5*1000)  // 0.5ms

// Timelog control
#define IRR_TIME_LOG			1 // timelog on/off

#if BUILD_FOR_HOST
#define ATRACE_NAME(...)
#define ATRACE_CALL()
#define ATRACE_BEGIN(...)
#define ATRACE_END()
#define ATRACE_INT(...)
#define ATRACE_INDEX(...)
#else

#include <android/trace.h>

#define ATRACE_NAME(name) ScopedTrace ___tracer(name)

// ATRACE_CALL is an ATRACE_NAME that uses the current function name.
#define ATRACE_CALL() ATRACE_NAME(__FUNCTION__)

#define ATRACE_BEGIN(name) ATrace_beginSection(name);
#define ATRACE_END() ATrace_endSection();
#define ATRACE_INT(name, value) ATrace_setCounter(name, value)

#define ATRACE_INDEX(name, value1, value2)                      \
    if (ATrace_isEnabled()) {                                   \
        char ___traceBuf[1024];                                 \
        snprintf(___traceBuf, sizeof(___traceBuf), "%s__%d_%d", \
                (name), (value1), (value2));                    \
        ATRACE_NAME(___traceBuf);                               \
    }

class ScopedTrace {
    public:
        inline ScopedTrace(const char *name) {
            ATrace_beginSection(name);
        }

        inline ~ScopedTrace() {
            ATrace_endSection();
        }
    };
#endif


// controlled by property "persist.irr.timelog"
typedef enum {
    TIMELOG_LEVEL_NONE= 0,   // no print
    TIMELOG_LEVEL_NORMAL = 1, // normal print
    TIMELOG_LEVEL_CTIME = 2,  // ctime print
    TIMELOG_LEVEL_VERB = 3,   // verbose print
    TIMELOG_LEVEL_ALL = 4,    // all print (ignore diff)
} timelog_level_t;


typedef enum{
    TIME_NORMAL = 0x00,
    TIME_NONE = 0x01,   // no print
    TIME_IRRF = 0x02,   // add prefix
    TIME_IRRB = 0x04,   // add prefix
    TIME_DIFF = 0x08,   // only diff > threshold
    TIME_VERB = 0x10,   // only property = verbose
} timelog_mode_t;


/*
  *   Usage :
  *	1)  Simple usage, log on TimeLog object constructor and desctructor, 
  *        Set mode = 0,  this is the easist way to get function enter and exit timestamp.
  *        Example :
  *			void function(...) 
  *  			{
  *				TimeLog log("func_name");
  *
  *				// ...
  * 			}
  *
  *	2) Advance usages 
  *        2-1) Set mode = 1,  manually control when to begin and end time log.
  *               Example :
  *			void function(...) 
  *  			{
  *				TimeLog log("func_name", 1);
  *
  *				// ... other logic ...
  *								
  *				log.begin("task_name");
  *				// ... the task to be measured ...	  			
  *				log.end();
  *
  *				// ... other logic ...
  * 			}
  *
  *	    	
  *        2-2) Use idx1 and idx2 parameters to pass meaningful info in log, like function call count or task index
  *               Example :
  *			static long func_call_index = 0;
  *			void function(...) 
  *  			{
  *				TimeLog log("func_name", 0, func_call_index++);
  *
  *				// ...
  * 			}
  *
  * 	   2-3) Use mask mode for advanced printing:
  * 	        TIME_IRRF and TIME_IRRB will add IRRB_/IRRF_ prefix to the head of the 1st parameter.
  * 	        TIME_DIFF will only print the function with time cost larger than IRR_TIME_LOG_DIFF_THRESHOLD_US (0.5ms currently)
  * 	        TIME_VERB will print more verbose log when property set to be verbose
  * 	        Example:
  * 	            TimeLog log(__FUNCTION__, TIME_IRRF|TIME_DIFF);
  * 	            TimeLog log(__FUNCTION__, TIME_VERB);
  */

class TimeLog {
public :
	TimeLog(const char* name, int mode = 0, unsigned long idx1 = 0,  unsigned long idx2 = 0);
	~TimeLog();

	void begin(const char* name, unsigned long idx1 = 0,  unsigned long idx2 = 0);
	void end();

    void updateProperty();

private :
	std::string		m_enter_name;
    std::string     m_begin_name;
    std::string     m_prefix;
	int				m_mode;
	unsigned long 	m_idx1;
	unsigned long 	m_idx2;
	struct timeval 	m_enter;
	struct timeval 	m_begin;
	struct timeval 	m_end;
	struct timeval 	m_exit;

    static int g_timelog_level;
    static bool g_isInitialized;
};


#endif /* __TIME_LOG_H__ */

