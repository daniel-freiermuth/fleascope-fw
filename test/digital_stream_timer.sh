#! /bin/sh
# host test for the digital_stream Timer3 prescaler/period selection

set -e

cd `dirname "$0"`
${CC:-cc} -std=c99 -Wall -Wextra -Werror -o digital_stream_timer_test digital_stream_timer_test.c
./digital_stream_timer_test
rm -f digital_stream_timer_test
