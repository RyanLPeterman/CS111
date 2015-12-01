#ifndef OSP2P_H
#define OSP2P_H

// ERROR MESSAGE FUNCTIONS

// die(format, ...)
//	Print an error message to stderr and exits with exit status 1.
//	'format' can contain printf-style escapes, such as %s and %d.
//	For example: die("The string %s is bad!", "BAD");
//	Also prints strerror(errno) if 'format' doesn't end with "\n".
void die(const char *format, ...) __attribute__((noreturn));


// error(format, ...)
//	Print an error message to stderr.
//	Like die(), but does not exit after printing the message.
void error(const char *format, ...);


// message(format, ...)
//	Print a message to stderr.
//	Like error(), but for non-error messages, such as progress information.
void message(const char *format, ...);


// PEER COMMUNICATION FUNCTIONS
// Use these functions to send messages to peers/the tracker, and parse
// messages from peers/the tracker.

// osp2p_writef(fd, format, ...)
//	Write to file descriptor 'fd' the message in 'format'.
//	Blocks until the whole message is written (although for short
//	messages, will never block because of kernel buffering).
//	Returns 0 on success, and -1 on error (errno is set to indicate the
//	error type).
//
//      The 'format' string can contain the following format conversions:
//      %s      Takes a string argument (const char *arg).  Quotes 'arg'
//              and writes it to 'fd'.  For example, the string "Hi! you"
//              becomes "Hi!%20you".
//      %I      Takes an IP address argument (struct in_addr arg).  Prints
//              'arg' to 'fd' in dotted-quad form (e.g., "131.179.80.139").
//      %d      Takes an integer argument (int arg).  Prints it to 'fd'.
//      %%      Prints a literal % character to 'fd'.
//
//      Examples:
//      osp2p_writef(fd, "GET %s OSP2P\n", "filename")
//              => writes "GET filename OSP2P\n" to fd
//      osp2p_writef(fd, "ADDR %s %I:%d\n", alias, ipaddr, port)
//              => might write "ADDR alias 10.2.3.4:80\n" to fd
int osp2p_writef(int fd, const char *format, ...);


// osp2p_sscanf(str, format, ...)
//      Read from 'str' a message formatted according to 'format'.
//      'str' is null-terminated.
//      Returns 0 if 'str' matches 'format', and -1 if it does not.
//
//      'str' must match 'format' exactly, except for the following format
//      conversions:
//      %s      Takes a string argument (char *arg).  The input string must
//              contain a sequence of one or more non-whitespace characters.
//              These characters are unquoted and written to 'arg'.
//              'arg' should have enough space to hold the unquoted string
//              plus a terminating null character.
//      %I      Takes an IP address argument (struct in_addr *arg).  The input
//              string must contain a textual IP address (e.g., "127.0.0.1").
//              The address is stored in *arg.
//      %d      Takes an integer argument (int *arg).  The input string must
//              contain a decimal integer (e.g., "120").  The integer is
//              stored in *arg.
//      %n      Takes an integer argument (int *arg).  Stores the number of
//              characters parsed so far in *arg.
//      %%      The input string must contain a literal % character.
//
//      Examples:
//
//      char buf[BUFSIZ], buf2[BUFSIZ];
//	struct in_addr ina;
//	int port;
//
//      osp2p_sscanf("GET filename OSP2P\n", "GET %s %s\n", buf, buf2)
//              => returns 0 (the string matches the format)
//                 stores "filename" in buf
//                 stores "OSP2P" in buf2
//      osp2p_sscanf("GET f 12:40\n", "GET %s %I:%d\n", buf, &ina, &port)
//              => returns -1 (the string does not match the format)
//      osp2p_sscanf("GET %20 1.0.0.2:4\n", "GET %s %I:%d\n", buf, &ina, &port)
//              => returns 0 (the string matches the format)
//                 stores " " in buf (the '%20' was unquoted)
//                 stores 1.0.0.2 in ina
//                 stores 4 in port
int osp2p_sscanf(const char *str, const char *format, ...);


// osp2p_snscanf(str, format, ...)
//	Read from 'str' a message formatted according to 'format',
//	like osp2p_sscanf.  However, 'str' is NOT null-terminated; it is
//      exactly 'len' characters long.
int osp2p_snscanf(const char *str, size_t len, const char *format, ...);

#endif
