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

#include "test.h"
#include "../src/clockwork.h"
#include "../src/resources.h"

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

TESTS {
	subtest {
		struct res_exec *exec;
		char *key;

		exec = res_exec_new("exec-key");
		key = res_exec_key(exec);
		is_string(key, "exec:exec-key", "exec key");

		free(key);
		res_exec_free(exec);
	}

	subtest {
		res_exec_free(NULL);
		pass("res_exec_free(NULL) doesn't segfault");
	}

	subtest {
		struct res_exec *r;
		r = res_exec_new("run-stuff");

		res_exec_set(r, "user",     "root");
		res_exec_set(r, "group",    "adm");
		res_exec_set(r, "test",     "/usr/local/bin/test-it");
		res_exec_set(r, "command",  "/usr/local/bin/run-it");
		res_exec_set(r, "ondemand", "yes");

		ok(res_exec_match(r, "user", "root") != 0,
				"user is not a matchable attr");
		ok(res_exec_match(r, "group", "adm") != 0,
				"group is not a matchable attr");
		ok(res_exec_match(r, "test", "/usr/local/bin/test-it") != 0,
				"test is not a matchable attr");
		ok(res_exec_match(r, "ondemand", "yes") != 0,
				"ondemand is not a matchable attr");

		ok(res_exec_match(r, "command", "/usr/local/bin/run-it") == 0,
				"match command=/usr/local/bin/run-it");
		ok(res_exec_match(r, "command", "/some/other/command") != 0,
				"!match command=/some/other/command");

		is_int(res_exec_set(r, "what-does-the-fox-say", "ring-ding-ring-ding"),
			-1, "res_exec_set doesn't like nonsensical attributes");

		res_exec_free(r);
	}

	subtest {
		struct res_exec *r;
		hash_t *h;

		h = vmalloc(sizeof(hash_t));
		r = res_exec_new("do-it");

		ok(res_exec_attrs(r, h) == 0, "got exec attrs");
		is_string(hash_get(h, "command"),  "do-it", "h.command (uses key)");
		is_null(hash_get(h, "test"),    "h.test");
		is_null(hash_get(h, "user"),    "h.user");
		is_null(hash_get(h, "group"),   "h.group");
		is_string(hash_get(h, "ondemand"), "no", "h.ondemand");

		res_exec_set(r, "command",  "/bin/run");
		res_exec_set(r, "test",     "/bin/tester");
		res_exec_set(r, "ondemand", "yes");

		ok(res_exec_attrs(r, h) == 0, "got exec attrs");
		is_string(hash_get(h, "command"),  "/bin/run",    "h.command");
		is_string(hash_get(h, "test"),     "/bin/tester", "h.test");
		is_string(hash_get(h, "ondemand"), "yes",         "h.ondemand");

		ok(res_exec_set(r, "xyzzy", "BAD") != 0, "xyzzy is a bad attr");
		ok(res_exec_attrs(r, h) == 0, "got exec attrs");
		is_null(hash_get(h, "xyzzy"), "h.xyzzy is unset (bad attr)");

		hash_done(h, 1);
		free(h);
		res_exec_free(r);
	}

	done_testing();
}
