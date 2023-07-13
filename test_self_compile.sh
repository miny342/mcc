#!/bin/sh
set -e
diff main_2.s main_3.s
diff parse_2.s parse_3.s
diff codegen_2.s codegen_3.s
diff tool_2.s tool_3.s
echo OK
