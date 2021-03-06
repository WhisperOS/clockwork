/*
  Copyright 2011-2015 James Hunt <james@jameshunt.us>

  This file is part of Clockwork.

  Clockwork is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Clockwork is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with Clockwork.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "../src/clockwork.h"
#include "../src/policy.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <uuid/uuid.h>

#ifdef CW_CLI_DIR
#  define DEFAULT_EXEC_PATH  CW_CLI_DIR
#else
#  define DEFAULT_EXEC_PATH  "/lib/clockwork/cli"
#endif

static void usage(int rc)
{
	fprintf(stderr, "Usage: cw [--version] [--help|-h]\n"
	                "          [--exec <path>] [--dump]\n"
	                "          <command> [args]\n"
	                "\n"
	                "Commonly used commands:\n"
	                "  fact      Show local system facts\n"
	                "  cert      Generate certificates (for host identity)\n"
	                "  shell     Interactively explore a manifest\n"
	                "  trust     Manage a certificate trust database\n"
	                "\n"
	                "Mesh commands (for interacting with a network of machines):\n"
	                "\n"
	                "  show      Show version and ACL information\n"
	                "  ping      Gauge host response time\n"
	                "  query     Gather info about node-local resources\n"
	                "  exec      Execute arbitrary commands\n"
	                "  package   Manage package installations\n"
	                "  service   Start and stop services\n"
	                "  cfm       Trigger configuration management runs\n"
	                "\n"
	                "'cw help -c' lists available commands. and 'cw help -g' lists\n"
	                "some helpful Clockwork guides.  Run 'cw help <command>' and\n"
	                "'cw help <guide> to read the specifics.\n"
	                "\n");

	if (rc >= 0)
		exit(rc);
}

static int get_base_options(int argc, char **argv)
{
	char *exec_path = strdup(DEFAULT_EXEC_PATH);

	const char *short_opts = "+h";
	struct option long_opts[] = {
		{ "help",    no_argument,       NULL, 'h' },
		{ "version", no_argument,       NULL,  1  },
		{ "exec",    required_argument, NULL,  2  },
		{ "dump",    no_argument,       NULL,  3  },
		{ 0, 0, 0, 0 },
	};

	int opt, idx = 0;
	while ((opt = getopt_long(argc, argv, short_opts, long_opts, &idx)) != -1) {
		switch (opt) {
		case 'h':
		case '?':
			usage(0);

		case 1: /* --version */
			printf("Clockwork version %s\n", PACKAGE_VERSION);
			exit(0);

		case 2: /* --exec */
			free(exec_path);
			exec_path = strdup(optarg);
			break;

		case 3: /* --dump */
			printf("exec_path %s\n", DEFAULT_EXEC_PATH);
			exit(0);

		default:
			usage(1);
		}
	}

	char *old_path = getenv("PATH");
	char *path = old_path ? string("%s:%s", exec_path, old_path)
	                      : strdup(exec_path);
	setenv("PATH", path, 1);
	free(exec_path);
	free(path);

	if (optind >= argc)
		usage(0);

	return optind;
}

static int builtin_cw_cert    (int argc, char **argv);
static int builtin_cw_cfm     (int argc, char **argv);
static int builtin_cw_fact    (int argc, char **argv);
static int builtin_cw_help    (int argc, char **argv);
static int builtin_cw_trust   (int argc, char **argv);
static int builtin_cw_uuid    (int argc, char **argv);

static struct {
	const char *cmd;
	int (*fn)(int,char**);
} BUILTINS[] = {
	{ "cw-cert",      builtin_cw_cert      },
	{ "cw-cfm",       builtin_cw_cfm       },
	{ "cw-fact",      builtin_cw_fact      },
	{ "cw-help",      builtin_cw_help      },
	{ "cw-trust",     builtin_cw_trust     },
	{ "cw-uuid",      builtin_cw_uuid      },
	{ NULL, NULL },
};

int main(int argc, char **argv)
{
	char *bin = strrchr(argv[0], '/');
	if (bin) bin++;
	else bin = argv[0];

	int i;
	for (i = 0; BUILTINS[i].cmd; i++)
		if (strcmp(bin, BUILTINS[i].cmd) == 0)
			return (*BUILTINS[i].fn)(argc, argv);

	if (strncmp(bin, "cw-", 3) == 0) {
		fprintf(stderr, "cw: '%s' is not a Clockwork builtin.  See 'cw --help'.\n", bin);
		return 2;
	}

	/* dispatch via exec! */
	int next = get_base_options(argc, argv);
	argv[next] = string("cw-%s", argv[next]);
	execvp(argv[next], argv+next);

	fprintf(stderr, "cw: '%s' is not a Clockwork command.  See 'cw --help'.\n", argv[next]);
	return 1;
}

static int builtin_cw_cert(int argc, char **argv)
{
	const char *short_opts = "h?f:i:u";
	struct option long_opts[] = {
		{ "help",     no_argument,       NULL, 'h' },
		{ "identity", required_argument, NULL, 'i' },
		{ "file",     required_argument, NULL, 'f' },
		{ "user",     no_argument,       NULL, 'u' },
		{ 0, 0, 0, 0 },
	};

	char *ident = NULL;
	char *file  = strdup("cwcert");
	int   type  = VIGOR_CERT_ENCRYPTION;

	int opt, idx = 0;
	while ( (opt = getopt_long(argc, argv, short_opts, long_opts, &idx)) != -1) {
		switch (opt) {
		case 'h':
		case '?':
			printf("USAGE: %s [--identity FQDN] [--user] [--file cwcert]\n", argv[0]);
			exit(0);

		case 'i':
			free(ident);
			ident = strdup(optarg);
			break;

		case 'f':
			free(file);
			file = strdup(optarg);
			break;

		case 'u':
			type = VIGOR_CERT_SIGNING;
			break;
		}
	}

	char *pubfile = string("%s.pub", file);
	int pubfd = open(pubfile, O_WRONLY|O_CREAT|O_EXCL, 0444);
	if (pubfd < 0) {
		perror(pubfile);
		exit(1);
	}
	int secfd = open(file, O_WRONLY|O_CREAT|O_EXCL, 0400);
	if (secfd < 0) {
		perror(file);
		unlink(pubfile);
		exit(1);
	}

	FILE *pubio = fdopen(pubfd, "w");
	FILE *secio = fdopen(secfd, "w");
	if (!pubio || !secio) {
		perror("fdopen");
		fclose(pubio); close(pubfd); unlink(pubfile);
		fclose(secio); close(secfd); unlink(file);
		exit(1);
	}

	cert_t *cert = cert_generate(type);
	assert(cert);
	if (ident) {
		cert->ident = ident;
	} else if (cert->type == VIGOR_CERT_SIGNING) {
		cert->ident = getenv("USER") ? strdup(getenv("USER")) : NULL;
	} else {
		cert->ident = fqdn();
	}
	if (!cert->ident || strlen(cert->ident) == 0) {
		fprintf(stderr, "Failed to determine certificate/key identity!\n");
		fclose(pubio); close(pubfd); unlink(pubfile);
		fclose(secio); close(secfd); unlink(file);
		exit(1);
	}

	cert_writeio(cert, pubio, 0);
	cert_writeio(cert, secio, 1);

	fclose(pubio);
	fclose(secio);

	cert_free(cert);
	return 0;
}

static int builtin_cw_cfm(int argc, char **argv)
{
	return 0;
}

static int builtin_cw_fact(int argc, char **argv)
{
	char *config_file = NULL;
	int brief = 0;

	const char *short_opts = "h?vqVc:b";
	struct option long_opts[] = {
		{ "help",        no_argument,       NULL, 'h' },
		{ "config",      required_argument, NULL, 'c' },
		{ "brief",       no_argument,       NULL, 'b' },
		{ 0, 0, 0, 0 },
	};
	int opt, idx = 0;
	while ( (opt = getopt_long(argc, argv, short_opts, long_opts, &idx)) != -1) {
		switch (opt) {
		case 'h':
		case '?':
			printf("usage: cw-fact [--config <path>] [--brief|-b] [fact(s)]\n");
			exit(0);

		case 'c':
			free(config_file);
			config_file = strdup(optarg);
			break;

		case 'b':
			brief = 1;
			break;
		}
	}

	LIST(config);
	config_set(&config, "gatherers", CW_GATHER_DIR "/*");
	FILE *io = NULL;

	if (config_file) {
		io = fopen(config_file, "r");
		if (!io) {
			fprintf(stderr, "%s: %s\n", config_file, strerror(errno));
			exit(2);
		}
	} else {
		io = fopen(CW_COGD_CONFIG_FILE, "r");
	}

	if (io) {
		if (config_read(&config, io) != 0) {
			fprintf(stderr, "%s: failed to parse\n", config_file);
			exit(3);
		}
		fclose(io);
	}

	char *gatherers = config_get(&config, "gatherers");

	hash_t facts;
	memset(&facts, 0, sizeof(hash_t));
	if (fact_gather(gatherers, &facts) != 0) {
		fprintf(stderr, "Failed to gather facts via %s\n", gatherers);
		exit(4);
	}

	int rc = 0;
	if (optind >= argc) {
		fact_write(stdout, &facts);

	} else {
		char *v;
		for (; optind < argc; optind++) {
			v = hash_get(&facts, argv[optind]);
			if (v) {
				if (brief)
					printf("%s\n", v);
				else
					printf("%s=%s\n", argv[optind], v);

			} else {
				fprintf(stderr, "No such fact '%s'\n", argv[optind]);
				rc = 5;
			}
		}
	}

	return rc;
}

static int builtin_cw_help(int argc, char **argv)
{
	return 0;
}

static int builtin_cw_trust(int argc, char **argv)
{
	const char *short_opts = "h?trd:";
	struct option long_opts[] = {
		{ "help",     no_argument,        NULL, 'h' },
		{ "trust",    no_argument,        NULL, 't' },
		{ "revoke",   no_argument,        NULL, 'r' },
		{ "database", required_argument,  NULL, 'd' },
		{ 0, 0, 0, 0 },
	};

	char mode = 't';
	char *path = strdup("/etc/clockwork/certs/trusted");

	int opt, idx = 0;
	while ( (opt = getopt_long(argc, argv, short_opts, long_opts, &idx)) != -1) {
		switch (opt) {
		case 'h':
		case '?':
			printf("USAGE: %s [OPTIONS] FILE1 FILE2\n", argv[0]);
			exit(0);

		case 't':
		case 'r':
			mode = opt;
			break;

		case 'd':
			free(path);
			path = strdup(optarg);
			break;
		}
	}

	trustdb_t *db = trustdb_read(path);
	if (!db) db = trustdb_new();

	int rc;
	int i, n;
	for (i = optind, n = 0; i < argc; i++) {
		cert_t *cert = cert_read(argv[i]);
		if (!cert) {
			fprintf(stderr, "skipping %s: %s\n", argv[i], strerror(errno));
			continue;
		}

		if (!cert->pubkey) {
			fprintf(stderr, "skipping %s: no public key found in certificate\n", argv[i]);
			cert_free(cert);
			continue;
		}

		if (mode == 't') {
			rc = trustdb_trust(db, cert);
			assert(rc == 0);
			printf("TRUST %s %s\n", cert->pubkey_b16, cert->ident ? cert->ident : "(no ident)");
		} else {
			rc = trustdb_revoke(db, cert);
			assert(rc == 0);
			printf("REVOKE %s %s\n", cert->pubkey_b16, cert->ident ? cert->ident : "(no ident)");
		}
		cert_free(cert);
		n++;
	}

	printf("Processed %i certificate%s\n", n, n == 1 ? "" : "s");
	if (!n) exit(0);

	rc = trustdb_write(db, path);
	if (rc != 0) {
		fprintf(stderr, "Failed to write out trustdb %s: %s\n", path, strerror(errno));
		exit(2);
	}

	printf("Wrote %s\n", path);
	free(path);
	trustdb_free(db);
	return 0;
}

#define UUID_FILE  CW_SYSCONF_DIR "/.uuid"

static int builtin_cw_uuid(int argc, char **argv)
{
	const char *short_opts = "h?";
	struct option long_opts[] = {
		{ "help",     no_argument,        NULL, 'h' },
		{ "regen",    no_argument,        NULL,  1 },
		{ 0, 0, 0, 0 },
	};

	int regen = 0;

	int opt, idx = 0;
	while ( (opt = getopt_long(argc, argv, short_opts, long_opts, &idx)) != -1) {
		switch (opt) {
		case 'h':
		case '?':
			printf("USAGE: %s [--regen]\n", argv[0]);
			exit(0);

		case 1:
			regen = 1;
			break;
		}
	}

	char s[37];
	FILE *io;
	uuid_t uuid;

	if (regen) {
regen:
		io = fopen(UUID_FILE, "w");
		if (!io) {
			perror(UUID_FILE);
			exit(1);
		}

		uuid_generate(uuid);
		uuid_unparse_lower(uuid, s);
		fprintf(io, "%s\n", s);
		fclose(io);

	} else {
		io = fopen(UUID_FILE, "r");
		if (!io) {
			if (errno == ENOENT) goto regen;
			perror(UUID_FILE);
			exit(2);
		}

		if (!fgets(s, 37, io)) {
			perror(UUID_FILE);
			exit(3);
		}
		s[36] = '\0';
		fclose(io);
	}

	printf("%s\n", s);
	return 0;
}
