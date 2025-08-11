#!/bin/bash

set -euo pipefail

find tasks -type f -name 'testing.yaml' -print0 | \
    xargs -0 -I{} /bin/bash -euo pipefail -c '
        echo "Testing {}"
        cd $(dirname {})
        cargo xtask test || exit 255
    '
