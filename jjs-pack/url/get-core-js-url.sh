#!/usr/bin/env bash

set -e

SCRIPT_DIR=$(realpath "$(dirname "${BASH_SOURCE[0]}")")
cd "${SCRIPT_DIR}"

npm install --no-package-lock core-js-pure@3.33.2

esbuild --bundle "node_modules/core-js-pure/web/url.js" --format=cjs --outfile="url/url.js"
esbuild --bundle "node_modules/core-js-pure/web/url-search-params.js" --format=cjs --outfile="url/url-search-params.js"
