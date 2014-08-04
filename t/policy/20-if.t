#!/usr/bin/perl
use strict;
use warnings;

use Test::More;
use t::common;

resources_ok "use host if1.test", <<'EOF', "basic IF tests";
  package 00:always-there
  package 01:double-equals
  package 02:is
  package 03:not-equals
  package 04:is-not
EOF

resources_ok "use host if2.test", <<'EOF', "compound IF";
  package 00:always-there
  package 01:and
  package 02:or
EOF

resources_ok "use host if3.test", <<'EOF', "fact <=> fact";
  package 00:always-there
  package 01:fact-equality
  package 02:fact-inequality
  package 03:literal-to-literal
  package 04:literal-to-fact
EOF

cwpol_ok "use host map1.test; show package literals",
<<'EOF', "simple map conditional";

package "literals" {
  installed : "yes"
  name      : "literals"
  version   : "1.2.3"
}
EOF

cwpol_ok "use host map1.test; show package facts",
<<'EOF', "fact == fact map conditional";

package "facts" {
  installed : "yes"
  name      : "facts"
  version   : "correct"
}
EOF

cwpol_ok "use host map1.test; show package fallthrough",
<<'EOF', "fact == fact map conditional";

package "fallthrough" {
  installed : "yes"
  name      : "fallthrough"
  version   : "default correct"
}
EOF

done_testing;
