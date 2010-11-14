#ifndef _POLICY_H
#define _POLICY_H

#include "list.h"
#include "hash.h"
#include "res_file.h"
#include "res_group.h"
#include "res_user.h"

enum oper {
	NOOP = 0,
	PROG,
	IF,
	INCLUDE,
	ENFORCE,
	POLICY,
	HOST,
	RESOURCE,
	ATTR
};

struct stree {
	enum oper op;
	char *data1, *data2;

	unsigned int size;
	struct stree **nodes;
};

struct manifest {
	struct hash *policies;  /* POLICY stree nodes, hashed by name */

	struct host **hosts;
	size_t hosts_len;

	struct stree **nodes;
	size_t nodes_len;

	struct stree *root;
};

/* According to RFC 1034 */
#define HOST_NAME_MAX 256
struct host {
	char name[HOST_NAME_MAX];
	struct stree *policy;
};

/** policy - Defines a single, independent policy

  A policy consists of a set of resources, and the attributes that
  must be enforced for each.

  Resources are stored in two ways: a sequential list (by type) that
  reflects the order in which the resources were defined, and a hash
  table containing all resources (keyed by "TYPE:pkey").

  The lists are there so that each resource type can be walked, and
  the hash exists to ease searching.
 */
struct policy {
	char        *name;       /* User-assigned name of policy */
	uint32_t     version;    /* Policy version number */

	/* Components */
	struct list  res_files;
	struct list  res_groups;
	struct list  res_users;

	struct hash *resources;  /* Searchable hash table, keyed "TYPE:pkey" */
};

struct manifest* manifest_new(void);
void manifest_free(struct manifest *m);
struct host*  manifest_new_host(struct manifest *m, const char *name, struct stree *node);
struct stree* manifest_new_stree(struct manifest *m, enum oper op, const char *data1, const char *data2);

int stree_add(struct stree *parent, struct stree *child);
int stree_expand(struct stree *root, struct hash *policies); /* FIXME: maybe move this into spec/parser.c as a static */
int stree_compare(const struct stree *a, const struct stree *b);

struct hash* fact_read(FILE *io);
int fact_parse(const char *line, char **k, char **v);

struct policy* policy_generate(struct stree *root, struct hash *facts);

struct policy* policy_new(const char *name, uint32_t version);
int  policy_init(struct policy *pol, const char *name, uint32_t version);
void policy_deinit(struct policy *pol);
void policy_free(struct policy *pol);
void policy_free_all(struct policy *pol);

uint32_t policy_latest_version(void);

int policy_add_file_resource(struct policy *pol, const char *key, struct res_file *rf);
int policy_add_group_resource(struct policy *pol, const char *key, struct res_group *rg);
int policy_add_user_resource(struct policy *pol, const char *key, struct res_user *ru);

char* policy_pack(struct policy *pol);
struct policy* policy_unpack(const char *packed);

#endif /* _POLICY_H */
