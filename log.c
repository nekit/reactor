#include "log.h"
#include <signal.h>
#include <pthread.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#define DEFAULT_LOG_LEVEL MLL_TRACE
#define MLL_INIT(LEVEL) [MLL_##LEVEL] = #LEVEL

static const char * log_level_str[] = { MLL_INIT(TRACE), MLL_INIT(DEBUG), MLL_INIT(INFO), MLL_INIT(WARN), MLL_INIT(ERROR), MLL_INIT(FATAL) };

void log_message ( const char * file, const char * function, int line, mlog_level_t log_level, const char * template, ... ) {

  static sig_atomic_t idx = 0;
  static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
  static mlog_level_t current_log_level = DEFAULT_LOG_LEVEL;  
  va_list args;

  if ( NULL == file ) {
    
    current_log_level = log_level;
    INFO_MSG ("current log level: %d\n", current_log_level );
    return;
  }

  if ( current_log_level <= log_level ) {

    const char * mll_level_str = "Unknown";
    if ( 0 <= log_level && (log_level <= sizeof (log_level_str) / sizeof (log_level_str[0])) && log_level_str[log_level] )
      mll_level_str = log_level_str[log_level];

    pthread_mutex_lock ( &mutex );

    fprintf ( stderr,  "[%d] <%s> thread %u file '%s' function '%s' line %d:\n", idx++, mll_level_str, (int)pthread_self (), file, function, line );

    va_start (args, template);
    vfprintf (stderr, template, args);
    va_end (args);
    fflush (stderr);

    pthread_mutex_unlock ( &mutex );    
  }
  
} /* End of log_message() */


// case insensitive
mlog_level_t get_log_level ( const char * level_str ) {

  TRACE_MSG ( "getting log level %s\n", level_str );  

  int i;
  for ( i = 0; i < (sizeof ( log_level_str ) / sizeof (log_level_str[0])) ; ++i )
    if ( log_level_str[i] && ( 0 == strcasecmp( log_level_str[i], level_str ) ) )
      return i;

  DEBUG_MSG( "Unknown log level: %s\n", level_str );

  return DEFAULT_LOG_LEVEL;
}
