// -*- mode: c++ -*-
#define _BSD_EXTENSION
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <errno.h>
#include <ctype.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <limits.h>
#include "osp2p.h"

/******************************************************************************
 * HELPER FUNCTIONS FOR WRITING TO AND FROM PEERS, AND FOR ERROR MESSAGES
 * You don't need to understand this code to do the lab,
 * but you do need to understand the functions' specifications,
 * which are in osp2p.h.
 */

static char *http_quote(const char *s)
{
	char *os, *x;
	const char xvalue[] = "0123456789ABCDEF";

	if (!s) {
		errno = EINVAL;
		return NULL;
	}
	if (!(os = malloc(3*strlen(s) + 1))) {
		errno = ENOMEM;
		return NULL;
	}

	x = os;
	while (*s) {
		if (isalnum((unsigned char) *s) || *s == '$' || *s == '-'
		    || *s == '_' || *s == '.' || *s == '+' || *s == '!'
		    || *s == '*' || *s == '\'' || *s == '(' || *s == ')'
		    || *s == ',')
			*x++ = *s++;
		else {
			*x++ = '%';
			*x++ = xvalue[((unsigned char) *s) / 16];
			*x++ = xvalue[((unsigned char) *s) & 15];
			s++;
		}
	}
	*x = '\0';

	return os;
}


static int xvalue(unsigned char c) {
	if (c >= '0' && c <= '9')
		return c - '0';
	else
		return tolower(c) - 'a' + 10;
}


static int ensure_buf(char **buf, size_t *capacity, size_t len)
{
	if (len >= *capacity) {
		char *new_buf;
		if (!(new_buf = (char *) realloc(*buf, len + 2048))) {
			errno = ENOMEM;
			return -1;
		}
		*buf = new_buf;
		*capacity = len + 2048;
	}
	return 0;
}


int osp2p_writef(int fd, const char *format, ...)
{
	char *buf = 0;
	size_t pos, len, capacity = 0;
	char *s;
	va_list val;
	int ret = -1;

	if (ensure_buf(&buf, &capacity, 1024) < 0)
		goto exit;

	// develop the command string
	va_start(val, format);
	len = 0;
	while (*format) {
		if (format[0] == '%' && format[1] == 's') {
			const char *str = va_arg(val, const char *);
			char *quoted_str = http_quote(str);
			size_t quoted_len = strlen(quoted_str);
			if (ensure_buf(&buf, &capacity, len + quoted_len) < 0) {
				free(quoted_str);
				goto exit;
			}
			strcpy(&buf[len], quoted_str);
			len += quoted_len;
			free(quoted_str);
			format += 2;
		} else if (format[0] == '%' && format[1] == 'd') {
			int i = va_arg(val, int);
			if (ensure_buf(&buf, &capacity, len + 10) < 0)
				goto exit;
			len += sprintf(&buf[len], "%d", i);
			format += 2;
		} else if (format[0] == '%' && format[1] == 'I') {
			struct in_addr a = va_arg(val, struct in_addr);
			const char *addr_str = inet_ntoa(a);
			if (ensure_buf(&buf, &capacity, len + 16) < 0)
				goto exit;
			strcpy(&buf[len], addr_str);
			len += strlen(addr_str);
			format += 2;
		} else if (format[0] == '%' && format[1] == '%') {
			if (ensure_buf(&buf, &capacity, len + 1) < 0)
				goto exit;
			buf[len++] = '%';
			format += 2;
		} else {
			if (ensure_buf(&buf, &capacity, len + 1) < 0)
				goto exit;
			buf[len++] = *format++;
		}
	}
	va_end(val);

	// now write it
	pos = 0;
	while (pos < len) {
		ssize_t amt = write(fd, &buf[pos], len - pos);
		if (amt == -1 && (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK))
			/* do nothing */;
		else if (amt == -1)
			goto exit;
		else if (amt == 0) {
			errno = ECANCELED;
			goto exit;
		} else
			pos += amt;
	}

	// done!
	ret = 0;

    exit:
	free(buf);
	return ret;
}


int osp2p_vsnscanf(const char *s, size_t len, const char *format, va_list val)
{
	const char *begin_s = s;
	const char *end_s = s + len;
	size_t pos;
	int nconversions = 0;

	while (*format) {
		if (format[0] == '%' && format[1] == 's') {
			char *str = va_arg(val, char *);
			while (s != end_s && !isspace((unsigned char) *s))
				if (s[0] == '%'
				    && isxdigit((unsigned char) s[1])
				    && isxdigit((unsigned char) s[2])) {
					*str++ = xvalue(s[1]) * 16 + xvalue(s[2]);
					s += 3;
				} else
					*str++ = *s++;
			*str++ = '\0';
			nconversions++;
			format += 2;
		} else if (format[0] == '%' && format[1] == 'I') {
			struct in_addr *addr = va_arg(val, struct in_addr *);
			uint32_t value = 0;
			int i;
			for (i = 0; i < 4; i++) {
				int x = -1;
				while (s != end_s
				       && isdigit((unsigned char) *s)
				       && x <= 255)
					x = (x < 0 ? 0 : 10*x) + (*s++ - '0');
				if (x < 0 || x > 255
				    || (i < 3 && (s == end_s || *s != '.')))
					goto exit;
				value = (value << 8) + x;
				s += (i < 3 ? 1 : 0);
			}
			addr->s_addr = htonl(value);
			nconversions++;
			format += 2;
		} else if (format[0] == '%' && format[1] == 'd') {
			int *iptr = va_arg(val, int *);
			int value = 0;
			if (s == end_s || !isdigit((unsigned char) *s))
				goto exit;
			while (s != end_s && isdigit((unsigned char) *s)
			       && value < (INT_MAX - (*s - '0'))/10)
				value = (10*value) + (*s++ - '0');
			if (s != end_s && isdigit((unsigned char) *s))
				goto exit;
			*iptr = value;
			nconversions++;
			format += 2;
		} else if (format[0] == '%' && format[1] == 'n') {
			int *iptr = va_arg(val, int *);
			*iptr = s - begin_s;
			format += 2;
		} else if (format[0] == '%' && format[1] == '%') {
			if (s == end_s || *s != '%')
				goto exit;
			s++;
			format += 2;
		} else if (isspace((unsigned char) *format)) {
			if (s == end_s || !isspace((unsigned char) *s))
				goto exit;
			while (s != end_s && isspace((unsigned char) *s))
				++s;
			while (isspace((unsigned char) *format))
				++format;
		} else {
			if (s == end_s || *s != *format)
				goto exit;
			++s;
			++format;
		}
	}

    exit:
	return *format ? -1 : 0;
}



int osp2p_snscanf(const char *s, size_t len, const char *format, ...)
{
	int ret;

	va_list val;
	va_start(val, format);
	ret = osp2p_vsnscanf(s, len, format, val);
	va_end(val);

	return ret;
}



int osp2p_sscanf(const char *s, const char *format, ...)
{
	int ret;

	va_list val;
	va_start(val, format);
	ret = osp2p_vsnscanf(s, strlen(s), format, val);
	va_end(val);

	return ret;
}


void die(const char *format, ...)
{
	const char *errstr;
	va_list val;
	va_start(val, format);
	errstr = strerror(errno);
	vfprintf(stderr, format, val);
	if (strchr(format, '\n') == NULL)
		fprintf(stderr, ": %s\n", errstr);
	exit(1);
}


void error(const char *format, ...)
{
	const char *errstr = strerror(errno);
	va_list val;
	va_start(val, format);
	vfprintf(stderr, format, val);
	if (strchr(format, '\n') == NULL)
		fprintf(stderr, ": %s\n", errstr);
	va_end(val);
}


void message(const char *format, ...)
{
	va_list val;
	va_start(val, format);
	vfprintf(stderr, format, val);
	va_end(val);
}
