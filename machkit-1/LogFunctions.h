/*
 * Copyright 1991 NeXT Computer, Inc.
 */

#ifndef LOG_FUNCTIONS
#define LOG_FUNCTIONS 
#import <stdarg.h>


// this should be an object; one should define both factory and
// instance methods so that one can have several loggers running
// simultaneously...

void log_set_options(
	const char *header,		// short string at head of log msgs
	int show_pid,			// each msg has pid label
	int show_thread,		// each msg has thread # label
	int show_info_msgs		// log all info (verbose) messages
);

 
void log_info(const char *format, ...);		// logs if enableWarning
void log_warning(const char *format, ...);	// logs all the time
void log_error(const char *format, ...);	// logs & raises exception

extern int enableInfo;
#endif LOG_FUNCTIONS
