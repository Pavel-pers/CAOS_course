#!/bin/bash

set -eu

find tasks -type f -name 'testing.yaml' -print0 | \
    xargs -0 -I{} /bin/bash -euc '
        echo "Testing {}"
        cd $(dirname {})
        cargo xtask test
    '
