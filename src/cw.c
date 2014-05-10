#include "cw.h"
#include <assert.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <ctype.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <fcntl.h>
#include <pwd.h>
#include <grp.h>

/*
     ######  ####  ######   ##    ##    ###    ##        ######
    ##    ##  ##  ##    ##  ###   ##   ## ##   ##       ##    ##
    ##        ##  ##        ####  ##  ##   ##  ##       ##
     ######   ##  ##   #### ## ## ## ##     ## ##        ######
          ##  ##  ##    ##  ##  #### ######### ##             ##
    ##    ##  ##  ##    ##  ##   ### ##     ## ##       ##    ##
     ######  ####  ######   ##    ## ##     ## ########  ######
 */
static int CW_SIGNALLED = 0;
static void cw_sig_handler(int sig)
{
	CW_SIGNALLED = 1;
}

void cw_sig_catch(void)
{
	struct sigaction action;
	action.sa_handler = cw_sig_handler;
	action.sa_flags = SA_SIGINFO;

	sigemptyset(&action.sa_mask);
	sigaction(SIGINT,  &action, NULL);
	sigaction(SIGTERM, &action, NULL);
}

int cw_sig_interrupt(void)
{
	return CW_SIGNALLED > 0;
}

/*
    ##       ####  ######  ########  ######
    ##        ##  ##    ##    ##    ##    ##
    ##        ##  ##          ##    ##
    ##        ##   ######     ##     ######
    ##        ##        ##    ##          ##
    ##        ##  ##    ##    ##    ##    ##
    ######## ####  ######     ##     ######
 */

int cw_list_init(cw_list_t *l)
{
	assert(l);
	l->next = l->prev = l;
	return 0;
}

int cw_list_isempty(cw_list_t *l)
{
	assert(l);
	return l->next == l;
}

size_t cw_list_len(cw_list_t *lst)
{
	assert(lst);
	size_t len = 0;
	cw_list_t *n;

	for_each(n, lst)
		len++;

	return len;
}

int cw_list_splice(cw_list_t *prev, cw_list_t *next)
{
	assert(prev);
	assert(next);

	next->prev = prev;
	prev->next = next;
	return 0;
}

int cw_list_delete(cw_list_t *n)
{
	assert(n);
	return cw_list_splice(n->prev, n->next) == 0
	    && cw_list_init(n)                  == 0 ? 0 : -1;
}

int cw_list_replace(cw_list_t *o, cw_list_t *n)
{
	assert(o);
	assert(n);

	n->next = o->next;
	n->next->prev = n;

	n->prev = o->prev;
	n->prev->next = n;

	cw_list_init(o);
	return 0;
}

int cw_list_unshift(cw_list_t *l, cw_list_t *n)
{
	assert(l);
	assert(n);

	return cw_list_splice(n, l->next) == 0
	    && cw_list_splice(l, n)       == 0 ? 0 : -1;
}

int cw_list_push(cw_list_t *l, cw_list_t *n)
{
	assert(l);
	assert(n);

	return cw_list_splice(l->prev, n) == 0
	    && cw_list_splice(n,       l) == 0 ? 0 : -1;
}

cw_list_t *cw_list_shift(cw_list_t *l)
{
	assert(l);
	if (cw_list_isempty(l))
		return NULL;

	cw_list_t *n = l->next;
	if (cw_list_splice(l, n->next) != 0)
		return NULL;
	return n;
}

cw_list_t *cw_list_pop(cw_list_t *l)
{
	assert(l);
	if (cw_list_isempty(l))
		return NULL;

	cw_list_t *n = l->prev;
	if (cw_list_splice(n->prev, l) != 0)
		return NULL;
	return n;
}

/*
    ######## #### ##     ## ########
       ##     ##  ###   ### ##
       ##     ##  #### #### ##
       ##     ##  ## ### ## ######
       ##     ##  ##     ## ##
       ##     ##  ##     ## ##
       ##    #### ##     ## ########
 */

int32_t cw_time_s(void)
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_sec;
}

int64_t cw_time_ms(void)
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (int64_t) ((int64_t) tv.tv_sec * 1000 + (int64_t) tv.tv_usec / 1000);
}

int cw_sleep_ms(int64_t ms)
{
	struct timespec ts;
	ts.tv_sec  = (time_t)(ms / 1000.0);
	ts.tv_nsec = (long)((ms - (ts.tv_sec * 1000)) / 1000.0);
	return nanosleep(&ts, NULL);
}

/*
    ########     ###    ##    ## ########
    ##     ##   ## ##   ###   ## ##     ##
    ##     ##  ##   ##  ####  ## ##     ##
    ########  ##     ## ## ## ## ##     ##
    ##   ##   ######### ##  #### ##     ##
    ##    ##  ##     ## ##   ### ##     ##
    ##     ## ##     ## ##    ## ########
 */

static int CW_RAND_SEED = 0;
void cw_srand(void)
{
	if (CW_RAND_SEED)
		return;
	srand(CW_RAND_SEED = cw_time_ms());
}

/*
    ######## ##     ##  #######
         ##  ###   ### ##     ##
        ##   #### #### ##     ##
       ##    ## ### ## ##     ##
      ##     ##     ## ##  ## ##
     ##      ##     ## ##    ##
    ######## ##     ##  ##### ##
 */

#define rnd(num) ((int)((float)(num) * random() / (RAND_MAX + 1.0)))
void * cw_zmq_ident(void *zocket, void *buf)
{
	char *id = (char*)buf;
	if (!id) {
		id = malloc(8);
		assert(id);

		cw_srand();
		id[0] = rnd(256); id[1] = rnd(256);
		id[2] = rnd(256); id[3] = rnd(256);
		id[4] = rnd(256); id[5] = rnd(256);
		id[6] = getpid() & 0xff;
		id[7] = getpid() >> 8;
	}
	zmq_setsockopt(zocket, ZMQ_IDENTITY, id, 8);
	return id;
}

static cw_frame_t * s_cw_frame_build(cw_frame_t *f)
{
	assert(f);

	f->size = zmq_msg_size(&f->msg);
	f->data = calloc(f->size+1, sizeof(char));
	assert(f->data);
	assert(zmq_msg_data(&f->msg));
	memcpy(f->data, zmq_msg_data(&f->msg), f->size);

	size_t i;
	for (i = 0; i < f->size; i++) {
		if (!isprint(f->data[i])) {
			f->binary = 1;
			break;
		}
	}

	return f;
}

cw_frame_t *cw_frame_recv(void *zocket)
{
	assert(zocket);

	cw_frame_t *f = calloc(1, sizeof(cw_frame_t));
	assert(f);

	int rc = zmq_msg_init(&f->msg);
	assert(rc == 0);

	if (zmq_msg_recv(&f->msg, zocket, 0) == -1) {
		zmq_msg_close(&f->msg);
		free(f);
		return NULL;
	}

	size_t len;
	rc = zmq_getsockopt(zocket, ZMQ_RCVMORE, &f->more, &len);
	assert(rc == 0);

	return s_cw_frame_build(f);
}

int cw_frame_send(void *zocket, cw_frame_t *f)
{
	return zmq_msg_send(&f->msg, zocket,
			f->more ? ZMQ_SNDMORE : 0);
}

cw_frame_t *cw_frame_new(const char *s)
{
	assert(s);

	cw_frame_t *f = calloc(1, sizeof(cw_frame_t));
	assert(f);

	size_t len = strlen(s);
	int rc = zmq_msg_init_size(&f->msg, len);
	assert(rc == 0);

	memcpy(zmq_msg_data(&f->msg), s, len);

	return s_cw_frame_build(f);
}

cw_frame_t *cw_frame_copy(cw_frame_t *f)
{
	cw_frame_t *new = calloc(1, sizeof(cw_frame_t));
	assert(new);

	zmq_msg_init_size(&new->msg, f->size);
	memcpy(&new->msg, &f->msg, f->size);
	return s_cw_frame_build(new);
}

void cw_frame_close(cw_frame_t *f)
{
	f->size = 0;
	f->data = NULL;
	f->binary = 0;
	zmq_msg_close(&f->msg);
}

char *cw_frame_text(cw_frame_t *f)
{
	if (!f)
		return NULL;

	char *s = calloc(f->size + 1, sizeof(char));
	assert(s);
	memcpy(s, f->data, f->size);
	return s;
}

char *cw_frame_hex(cw_frame_t *f)
{
	assert(f);

	static const char hex[] = "0123456789abcdef";
	size_t i;

	char *s = calloc(2 * f->size + 1, sizeof(char));
	assert(s);

	for (i = 0; i < f->size; i++) {
		s[i * 2 + 0] = hex[f->data[i] >> 4];
		s[i * 2 + 1] = hex[f->data[i] & 0x0f];
	}
	s[i * 2] = '\0';
	return s;
}

int cw_frame_is(cw_frame_t *f, const char *s)
{
	assert(f);
	assert(s);

	return memcmp(f->data, s, strlen(s)) == 0;
}

int cw_frame_cmp(cw_frame_t *a, cw_frame_t *b)
{
	assert(a);
	assert(b);

	size_t len = a->size > b->size ? a->size : b->size;
	return memcmp(a->data, b->data, len) == 0;
}

void cw_frame_dump(FILE *io, cw_frame_t *f)
{
	fprintf(io, "[%i] ", (int)(f->size));

	size_t len = f->size;
	size_t max = f->binary ? 35 : 70;
	const char *ellips = "";
	if (len > max) {
		len = max;
		ellips = "...";
	}

	size_t i;
	for (i = 0; i < len; i++)
		fprintf(stderr, (f->binary ? "%02x" : "%c"), (f->data[i]));
	fprintf(stderr, "%s%s\n", ellips,
		(f->more ? " (+)" : ""));
}

zmq_msg_t * cw_zmq_msg_recv(void *zocket, zmq_msg_t *msg, int *more)
{
	assert(zocket);
	assert(more);
	assert(msg);

	if (zmq_msg_recv(msg, zocket, 0) != -1) {
		size_t len = sizeof(int);
		int rc = zmq_getsockopt(zocket, ZMQ_RCVMORE, more, &len);
		assert(rc == 0);

		return msg;
	}

	free(msg);
	return NULL;
}

void cw_zmq_shutdown(void *zocket, int linger)
{
	if (linger <= 0) linger = 500;
	int rc = zmq_setsockopt(zocket, ZMQ_LINGER, &linger, sizeof(linger));
	assert(rc == 0);

	rc = zmq_close(zocket);
	assert(rc == 0);
}

int cw_pdu_init(cw_pdu_t *pdu)
{
	assert(pdu);

	pdu->client = NULL;
	pdu->type   = NULL;
	pdu->data   = NULL;

	pdu->src = NULL;
	pdu->len = 0;
	cw_list_init(&pdu->frames);

	return 0;
}

void cw_pdu_destroy(cw_pdu_t *pdu)
{
	if (!pdu)
		return;

	free(pdu->client);
	free(pdu->type);
	free(pdu->data);

	cw_frame_t *f, *tmp;
	for_each_object_safe(f, tmp, &pdu->frames, l) {
		cw_frame_close(f);
		free(f);
	}
}

void cw_pdu_dump(FILE *io, cw_pdu_t *pdu)
{
	fprintf(io, "-----------------------------------\n");
	if (pdu->src) {
		cw_frame_dump(io, pdu->src);
		fprintf(io, "---\n");
	}
	cw_frame_t *f;
	for_each_object(f, &pdu->frames, l)
		cw_frame_dump(io, f);
	fprintf(io, "...\n");
	fprintf(io, "client: |%s|\n", pdu->client);
	fprintf(io, "type:   |%s|\n", pdu->type);
	fprintf(io, "data:   |%s|\n", pdu->data);
	fprintf(io, "EOF\n");
}

cw_frame_t *cw_pdu_frame(cw_pdu_t *pdu, size_t n)
{
	assert(pdu);
	assert(n >= 0);
	if (n >= pdu->len)
		return NULL;

	cw_frame_t *f = cw_list_head(&pdu->frames, cw_frame_t, l);
	while (n-- > 0)
		f = cw_list_next(f, l);
	return f;
}

static cw_pdu_t * s_cw_pdu_new(void)
{
	cw_pdu_t *pdu = malloc(sizeof(cw_pdu_t));
	assert(pdu);

	int rc = cw_pdu_init(pdu);
	assert(rc == 0);

	pdu->len = 0;
	pdu->src = NULL;

	return pdu;
}

cw_pdu_t *cw_pdu_recv(void *zocket)
{
	cw_pdu_t *pdu = s_cw_pdu_new();
	assert(pdu);

	int body = 0;
	cw_frame_t *f;
	do {
		f = cw_frame_recv(zocket);
		assert(f);

		if (!body) {
			if (f->size == 0) {
				body = 1;
			} else {
				pdu->src = f;
			}
		} else {
			int rc = cw_pdu_extend(pdu, f);
			assert(rc == 0);
		}
	} while (f && f->more);

	free(pdu->client);
	pdu->client = pdu->src ? cw_frame_hex(pdu->src) : strdup("none");

	free(pdu->type); pdu->type = cw_pdu_text(pdu, 0);
	if (!pdu->type)  pdu->type = strdup("NOOP");

	free(pdu->data); pdu->data = cw_pdu_text(pdu, 1);
	if (!pdu->data)  pdu->data = strdup(".");

	//cw_pdu_dump(stderr, pdu);
	return pdu;
}

cw_pdu_t *cw_pdu_make(cw_frame_t *dest, int n, ...)
{
	cw_pdu_t *pdu = s_cw_pdu_new();
	assert(pdu);

	if (dest)
		pdu->src = cw_frame_copy(dest);

	va_list ap;
	va_start(ap, n);
	while (n-- > 0)
		cw_pdu_extend(pdu, cw_frame_new(va_arg(ap, char *)));
	va_end(ap);

	return pdu;
}

int cw_pdu_extend(cw_pdu_t *pdu, cw_frame_t *f)
{
	assert(pdu);
	assert(f);

	cw_list_push(&pdu->frames, &f->l);
	pdu->len++;
	return 0;
}

int cw_pdu_send(void *zocket, cw_pdu_t *pdu)
{
	int rc;
	cw_frame_t *f;

	//cw_pdu_dump(stderr, pdu);
	if (pdu->src) {
		pdu->src->more = 1;
		rc = cw_frame_send(zocket, pdu->src);
		assert(rc >= 0);
	}

	f = cw_frame_new("");
	f->more = 1;
	rc = cw_frame_send(zocket, f);
	assert(rc >= 0);

	for_each_object(f, &pdu->frames, l) {
		f->more = (f->l.next == &pdu->frames ? 0 : 1);
		rc = cw_frame_send(zocket, f);
		assert(rc >= 0);
	}

	return 0;
}

/*
     ######   #######  ##    ## ######## ####  ######
    ##    ## ##     ## ###   ## ##        ##  ##    ##
    ##       ##     ## ####  ## ##        ##  ##
    ##       ##     ## ## ## ## ######    ##  ##   ####
    ##       ##     ## ##  #### ##        ##  ##    ##
    ##    ## ##     ## ##   ### ##        ##  ##    ##
     ######   #######  ##    ## ##       ####  ######
 */

int cw_cfg_set(cw_list_t *cfg, const char *key, const char *val)
{
	cw_keyval_t *kv;
	for_each_object(kv, cfg, l) {
		if (strcmp(kv->key, key) != 0)
			continue;
		free(kv->val);
		kv->val = strdup(val);
		return 0;
	}

	kv = malloc(sizeof(cw_keyval_t));
	assert(kv);

	kv->key = strdup(key);
	kv->val = strdup(val);
	cw_list_unshift(cfg, &kv->l);
	return 0;
}

char * cw_cfg_get(cw_list_t *cfg, const char *key)
{
	cw_keyval_t *kv;
	for_each_object(kv, cfg, l)
		if (strcmp(kv->key, key) == 0)
			return kv->val;
	return NULL;
}

int cw_cfg_isset(cw_list_t *cfg, const char *key)
{
	cw_keyval_t *kv;
	for_each_object(kv, cfg, l)
		if (strcmp(kv->key, key) == 0)
			return 1;
	return 0;
}

int cw_cfg_read(cw_list_t *cfg, FILE *io)
{
	cw_keyval_t *kv;
	char line[8192];
	while (fgets(line, 8191, io)) {
		/* FIXME: doesn't handle large lines (>8192) */
		/*
		   "   directive    value  \n"
		    ^  ^        ^   ^    ^
		    |  |        |   |    |
		    |  |        |   |    `--- d (= '\0')
		    |  |        |   `-------- c
		    |  |        `------------ b (= '\0')
		    |  `--------------------- a
		    `------------------------ line
		 */
		char *a, *b, *c, *d;
		for (a = line; *a &&  isspace(*a); a++);
		for (b = a;    *b && !isspace(*b); b++);
		for (c = b;    *c &&  isspace(*c); c++);
		for (d = c;    *d && !isspace(*d); d++);
		*b = *d = '\0';

		if (!*a) continue;
		if (*a == '#') continue;
		if (*a == '\n') continue;

		kv = malloc(sizeof(cw_keyval_t));
		assert(kv);
		kv->key = strdup(a);
		kv->val = strdup(c);

		cw_list_unshift(cfg, &kv->l);
	}
	return 0;
}

int cw_cfg_uniq(cw_list_t *dest, cw_list_t *src)
{
	cw_keyval_t *kv, *tmp;
	for_each_object_safe(kv, tmp, src, l) {
		if (cw_cfg_isset(dest, kv->key)) continue;
		cw_cfg_set(dest, kv->key, kv->val);
	}
	return 0;
}

/*
    ##        #######   ######    ######
    ##       ##     ## ##    ##  ##    ##
    ##       ##     ## ##        ##
    ##       ##     ## ##   ####  ######
    ##       ##     ## ##    ##        ##
    ##       ##     ## ##    ##  ##    ##
    ########  #######   ######    ######
 */

static struct {
	FILE *console;
	char *ident;
	int   level;
} CW_LOG = {
	.console = NULL,
	.ident   = NULL,
	.level   = LOG_INFO
};

void cw_log_open(const char *ident, const char *facility)
{
	assert(ident);
	assert(facility);

	free(CW_LOG.ident);
	CW_LOG.ident = strdup(ident);
	assert(CW_LOG.ident);

	if (strcmp(facility, "stdout") == 0) {
		CW_LOG.console = stdout;
		return;
	}
	if (strcmp(facility, "stderr")  == 0
	 || strcmp(facility, "console") == 0) {
		CW_LOG.console = stderr;
		return;
	}

	if (strncmp(facility, "file:", 5) == 0) {
		char *path = strchr(facility, ':'); path++;
		CW_LOG.console = fopen(path, "w+");
		return;
	}

	int fac = strcmp(facility, "local0") == 0 ? LOG_LOCAL0
	        : strcmp(facility, "local1") == 0 ? LOG_LOCAL1
	        : strcmp(facility, "local2") == 0 ? LOG_LOCAL2
	        : strcmp(facility, "local3") == 0 ? LOG_LOCAL3
	        : strcmp(facility, "local4") == 0 ? LOG_LOCAL4
	        : strcmp(facility, "local5") == 0 ? LOG_LOCAL5
	        : strcmp(facility, "local6") == 0 ? LOG_LOCAL6
	        : strcmp(facility, "local7") == 0 ? LOG_LOCAL7
	        :                                   LOG_DAEMON;

	CW_LOG.console = NULL;
	openlog(ident, LOG_PID, fac);
}

void cw_log_close(void)
{
	if (CW_LOG.console) {
		fclose(CW_LOG.console);
		CW_LOG.console = NULL;
	} else {
		closelog();
	}
}

int cw_log_level(int level, const char *name)
{
	int was = CW_LOG.level;
	if (name) {
		level = strcmp(name, "emerg")     == 0 ? LOG_EMERG
		      : strcmp(name, "emergency") == 0 ? LOG_EMERG
		      : strcmp(name, "alert")     == 0 ? LOG_ALERT
		      : strcmp(name, "crit")      == 0 ? LOG_CRIT
		      : strcmp(name, "critical")  == 0 ? LOG_CRIT
		      : strcmp(name, "err")       == 0 ? LOG_ERR
		      : strcmp(name, "error")     == 0 ? LOG_ERR
		      : strcmp(name, "warn")      == 0 ? LOG_WARNING
		      : strcmp(name, "warning")   == 0 ? LOG_WARNING
		      : strcmp(name, "notice")    == 0 ? LOG_NOTICE
		      : strcmp(name, "info")      == 0 ? LOG_INFO
		      : strcmp(name, "debug")     == 0 ? LOG_DEBUG
		      : CW_LOG.level;
	}
	if (level >= 0) {
		if (level > LOG_DEBUG)
			level = LOG_DEBUG;
		CW_LOG.level = level;
	}
	return was;
}

const char* cw_log_level_name(int level)
{
	if (level < 0) level = CW_LOG.level;
	switch (level) {
	case LOG_EMERG:   return "emergency";
	case LOG_ALERT:   return "alert";
	case LOG_CRIT:    return "critical";
	case LOG_ERR:     return "error";
	case LOG_WARNING: return "warning";
	case LOG_NOTICE:  return "notice";
	case LOG_INFO:    return "info";
	case LOG_DEBUG:   return "debug";
	default:          return "UNKNOWN";
	}
}

void cw_log(int level, const char *fmt, ...)
{
	if (level > CW_LOG.level)
		return;

	va_list ap1, ap2;
	va_start(ap1, fmt);
	va_copy(ap2, ap1);
	size_t n = vsnprintf(NULL, 0, fmt, ap1);
	assert(n >= 0);
	va_end(ap1);

	char *msg = calloc(n + 1, sizeof(char));
	assert(msg);
	vsnprintf(msg, n + 1, fmt, ap2);
	msg[n] = '\0';
	va_end(ap2);

	if (CW_LOG.console) {
		assert(level >= 0 && level <= LOG_DEBUG);

		pid_t pid = getpid();
		assert(pid);

		fprintf(CW_LOG.console, "%s[%i] %s\n",
				CW_LOG.ident, pid, msg);
		fflush(CW_LOG.console);
	} else {
		syslog(level, "%s", msg);
	}
	free(msg);
}

/*
    #######   ##     ## ##    ##
    ##    ##  ##     ## ###   ##
    ##    ##  ##     ## ####  ##
    #######   ##     ## ## ## ##
    ##   ##   ##     ## ##  ####
    ##    ##  ##     ## ##   ###
    ##     ##  #######  ##    ##
*/

# define MAXARGS 100

int cw_run(char *cmd, ...)
{
	pid_t pid = fork();
	if (pid < 0)
		return -1;
	else if (pid > 0) {
		int status = 0;
		waitpid(pid, &status, 0);
		if (WIFEXITED(status))
			return WEXITSTATUS(status);
		else
			return -2;
	} else {
		char *args[MAXARGS];
		int argno = 1;

		char *p = strrchr(cmd, '/');
		if (p) { args[0] = p++; } else { p = cmd; }
		args[0] = strdup(p);

		va_list argv;
		va_start(argv, cmd);
		while ((args[argno++] = va_arg(argv, char *)) != (char *)0)
			;

		execv(cmd, args);
		exit(127); // exit if exec failed
	}
}

/*
    ########     ###    ######## ##     ##  #######  ##    ##
    ##     ##   ## ##   ##       ###   ### ##     ## ###   ##
    ##     ##  ##   ##  ##       #### #### ##     ## ####  ##
    ##     ## ##     ## ######   ## ### ## ##     ## ## ## ##
    ##     ## ######### ##       ##     ## ##     ## ##  ####
    ##     ## ##     ## ##       ##     ## ##     ## ##   ###
    ########  ##     ## ######## ##     ##  #######  ##    ##
 */

int cw_daemonize(const char *pidfile, const char *user, const char *group)
{
	umask(0);

	size_t n;
	int rc;
	int fd = -1;
	if (pidfile) {
		fd = open(pidfile, O_RDWR|O_CREAT, S_IRUSR|S_IWUSR);
		if (fd == -1) {
			perror(pidfile);
			exit(2);
		}
	}

	errno=0;
	struct passwd *pw = getpwnam(user);
	if (!pw) {
		fprintf(stderr, "Failed to look up user '%s': %s\n",
				user, (errno == 0 ? "user not found" : strerror(errno)));
		exit(2);
	}

	errno = 0;
	struct group  *gr = getgrnam(group);
	if (!gr) {
		fprintf(stderr, "Failed to look up group '%s': %s\n",
				group, (errno == 0 ? "group not found" : strerror(errno)));
		exit(2);
	}

	/* clean up the environment */
	const char *keepers[] = { "LANG", "SHLVL", "_", "PATH", "SHELL", "TERM" };
	rc = cw_cleanenv(6, keepers);
	assert(rc == 0);

	setenv("PWD",     "/",                           1);
	setenv("HOME",    pw->pw_dir,                    1);
	setenv("LOGNAME", pw->pw_name,                   1);
	setenv("USER",    pw->pw_name,                   1);

	/* chdir to fs root to avoid tying up mountpoints */
	rc = chdir("/");
	assert(rc == 0);

	/* child -> parent error communication pipe */
	int pfds[2];
	rc = pipe(pfds);
	assert(rc == 0);

	/* fork */
	pid_t pid = fork();
	assert(pid >= 0);

	if (pid > 0) {
		close(pfds[1]);
		char buf[8192];
		while ( (n = read(pfds[0], buf, 8192)) > 0) {
			buf[n] = '\0';
			fprintf(stderr, "%s", buf);
		}
		exit(0);
	}
	close(pfds[0]);
	char error[8192];

	if (pidfile) {
		struct flock lock;
		size_t n;

		lock.l_type   = F_WRLCK;
		lock.l_whence = SEEK_SET;
		lock.l_start  = 0;
		lock.l_len    = 0; /* whole file */

		rc = fcntl(fd, F_SETLK, &lock);
		if (rc == -1) {
			snprintf(error, 8192, "Failed to acquire lock on %s.%s\n",
					pidfile,
					(errno == EACCES || errno == EAGAIN
						? "  Is another copy running?"
						: strerror(errno)));
			n = write(pfds[1], error, strlen(error));
			if (n < 0)
				perror("failed to inform parent of our error condition");
			if (n < strlen(error))
				fprintf(stderr, "child->parent inform - only wrote %li of %li bytes\n",
					n, strlen(error));
			exit(2);
		}
	}

	/* leave session group, lose the controlling term */
	rc = (int)setsid();
	assert(rc != -1);

	if (pidfile) {
		/* write the pid file */
		char buf[8];
		snprintf(buf, 8, "%i\n", getpid());
		n = write(fd, buf, strlen(buf));
		if (n < 0)
			perror("failed to write PID to pidfile");
		if (n < strlen(buf))
			fprintf(stderr, "only wrote %li of %li bytes to pidfile\n",
				n, strlen(error));
		rc = fsync(fd);
		assert(rc == 0);

		if (getuid() == 0) {
			/* chmod the pidfile, so it can be removed */
			rc = fchown(fd, pw->pw_uid, gr->gr_gid);
			assert(rc == 0);
		}
	}

	if (getuid() == 0) {
		/* set UID/GID */
		if (gr->gr_gid != getgid()) {
			rc = setgid(gr->gr_gid);
			assert(rc == 0);
		}
		if (pw->pw_uid != getuid()) {
			rc = setuid(pw->pw_uid);
			assert(rc == 0);
		}
	}

	/* redirect standard IO streams to/from /dev/null */
	if (!freopen("/dev/null", "r", stdin))
		perror("Failed to reopen stdin </dev/null");
	if (!freopen("/dev/null", "w", stdout))
		perror("Failed to reopen stdout >/dev/null");
	if (!freopen("/dev/null", "w", stderr))
		perror("Failed to reopen stderr >/dev/null");
	close(pfds[1]);

	return 0;
}

int cw_cleanenv(int n, const char **keep)
{
	extern char **environ;
	/* clean up the environment */
	int i, j;
	for (i = 0; environ[i]; i++) {
		int skip = 0;
		for (j = 0; j < n; j++) {
			size_t len = strlen(keep[j]);
			if (strncmp(environ[i], keep[j], len) == 0
			 && environ[i][len] == '=') {
				skip = 1;
				break;
			}
		}

		if (skip)
			continue;

		char *equals = strchr(environ[i], '=');
		char *name = calloc(equals - environ[i] + 1, sizeof(char));
		memcpy(name, environ[i], equals - environ[i]);
		unsetenv(name);
		free(name);
	}
	return 0;
}
