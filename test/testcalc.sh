#! /bin/sh
# host unit test for cpustick/fleascope-calc.h

set -e

cd `dirname $0`
${CC:-cc} -std=gnu99 -Wall -Wextra -Werror -I../cpustick -o fleascope-calc-test fleascope-calc-test.c
./fleascope-calc-test
