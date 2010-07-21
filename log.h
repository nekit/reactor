#ifndef MEGA_LOG_H
#define MEGA_LOG_H

typedef enum {

  MLL_TRACE,
  MLL_DEBUG,
  MLL_INFO,
  MLL_WARN,
  MLL_ERROR,
  MLL_FATAL,
  MLL_LAST,  
  
} mlog_level_t;

void log_message ( const char * file, const char * function, int line, mlog_level_t log_level, const char * template, ... );

mlog_level_t get_log_level ( const char * level_str );

#define MLOG_MSG(LOG_LEVEL, ARGS...) log_message ( __FILE__, __PRETTY_FUNCTION__, __LINE__, LOG_LEVEL, ARGS )
#define TRACE_MSG(ARGS...) MLOG_MSG(MLL_TRACE, ARGS )
#define DEBUG_MSG(ARGS...) MLOG_MSG(MLL_DEBUG, ARGS )
#define INFO_MSG(ARGS...) MLOG_MSG(MLL_INFO, ARGS )
#define WARN_MSG(ARGS...) MLOG_MSG(MLL_WARN, ARGS )
#define ERROR_MSG(ARGS...) MLOG_MSG(MLL_ERROR, ARGS )
#define FATAL_MSG(ARGS...) MLOG_MSG(MLL_FATAL, ARGS )

#define INIT_LOG_LEVEL(level_str) log_message ( NULL, NULL, 0, get_log_level(level_str), NULL )


#endif /* End of MEGA_LOG_H */
