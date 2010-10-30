#include <assert.h>
#include <string.h>

#include "policy.h"
#include "mem.h"

void policy_init(struct policy *pol, const char *name, uint32_t version)
{
	assert(pol);

	pol->name = xstrdup(name);
	pol->version = version;

	list_init(&pol->res_files);
	list_init(&pol->res_groups);
	list_init(&pol->res_users);
}

void policy_free(struct policy *pol)
{
	xfree(pol->name);
}

int policy_add_file_resource(struct policy *pol, struct res_file *rf)
{
	assert(pol);
	assert(rf);

	list_add_tail(&pol->res_files, &rf->res);
	return 0;
}

int policy_add_group_resource(struct policy *pol, struct res_group *rg)
{
	assert(pol);
	assert(rg);

	list_add_tail(&pol->res_groups, &rg->res);
	return 0;
}

int policy_add_user_resource(struct policy *pol, struct res_user *ru)
{
	assert(pol);
	assert(ru);

	list_add_tail(&pol->res_users, &ru->res);
	return 0;
}

int policy_serialize(struct policy *pol, char **dst, size_t *len)
{
	assert(pol);
	assert(dst);
	assert(len);

	char *object = NULL;
	size_t l; /* unused... FIXME: can we modify the serializer API? */
	stringlist *serialized_objects;

	struct res_user  *ru;
	struct res_group *rg;
	struct res_file  *rf;

	serialized_objects = stringlist_new(NULL);
	if (!serialized_objects) {
		return -1;
	}

	/* serialize res_user objects */
	for_each_node(ru, &pol->res_users, res) {
		if (res_user_serialize(ru, &object, &l) != 0
		 || stringlist_add(serialized_objects, object) != 0) {
			/* how do we fail? */
			xfree(object);
			stringlist_free(serialized_objects);
			return -1; /* fail noisily! */
		}
		xfree(object);
	}

	/* serialize res_group objects */
	for_each_node(rg, &pol->res_groups, res) {
		if (res_group_serialize(rg, &object, &l) != 0
		 || stringlist_add(serialized_objects, object) != 0) {
			/* how do we fail? */
			xfree(object);
			stringlist_free(serialized_objects);
			return -1; /* fail noisily! */
		}
		xfree(object);
	}

	/* serialize res_file objects */
	for_each_node(rf, &pol->res_files, res) {
		if (res_file_serialize(rf, &object, &l) != 0
		 || stringlist_add(serialized_objects, object) != 0) {
			/* how do we fail? */
			xfree(object);
			stringlist_free(serialized_objects);
			return -1; /* fail noisily! */
		}
		xfree(object);
	}

	*dst = stringlist_join(serialized_objects, "\n");
	*len = strlen(*dst);

	stringlist_free(serialized_objects);
	return 0;
}

int policy_unserialize(struct policy *pol, char *src, size_t len)
{
	assert(pol);
	assert(src);

	stringlist *serialized_objects;
	char *serial;
	size_t i;

	struct res_user *ru;
	struct res_group *rg;
	struct res_file *rf;

	serialized_objects = stringlist_split(src, len, "\n");
	if (!serialized_objects) {
		return -1;
	}

	for (i = 0; i < serialized_objects->num; i++) {
		serial = serialized_objects->strings[i];
		if (strncmp(serial, "res_user ", 9) == 0) {
			ru = malloc(sizeof(struct res_user));
			if (!ru) {
				return -1;
			}

			res_user_init(ru);
			if (res_user_unserialize(ru, serial, strlen(serial)) != 0) {
				res_user_free(ru);
				return -1;
			}

			policy_add_user_resource(pol, ru);
		} else if (strncmp(serial, "res_group ", 10) == 0) {
			rg = malloc(sizeof(struct res_group));
			if (!rg) {
				return -1;
			}

			res_group_init(rg);
			if (res_group_unserialize(rg, serial, strlen(serial)) != 0) {
				res_group_free(rg);
				stringlist_free(serialized_objects);
				return -1;
			}

			policy_add_group_resource(pol, rg);
		} else if (strncmp(serial, "res_file ", 9) == 0) {
			rf = malloc(sizeof(struct res_file));
			if (!rf) {
				stringlist_free(serialized_objects);
				return -1;
			}

			res_file_init(rf);
			if (res_file_unserialize(rf, serial, strlen(serial)) != 0) {
				res_file_free(rf);
				stringlist_free(serialized_objects);
				return -1;
			}

			policy_add_file_resource(pol, rf);
		} else {
			stringlist_free(serialized_objects);
			return -2; /* unknown policy object type! */
		}
	}

	stringlist_free(serialized_objects);
	return 0;
}
