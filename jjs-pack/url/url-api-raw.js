/* Copyright Light Source Software, LLC and other contributors.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

'use strict';

// node_modules/core-js-pure/web/url.js only exports URL, but JJS needs URLSearchParams
// too. This file reworks the original to export both.

require('./node_modules/core-js-pure/web/url-search-params');
require('./node_modules/core-js-pure/modules/web.url');
require('./node_modules/core-js-pure/modules/web.url.can-parse');
require('./node_modules/core-js-pure/modules/web.url.to-json');

const { URL, URLSearchParams } = require('./node_modules/core-js-pure/internals/path');

module.exports = { URL, URLSearchParams };
