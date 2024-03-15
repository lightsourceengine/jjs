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

const { isArray } = Array;
const { stringify } = JSON;

const { println, now } = module.bindings;

class Console {
  #timeTags = new Map();
  #countTags = new Map();

  log(...args) {
    println(format(args));
  }

  error(...args) {
    println(format(args));
  }

  warn(...args) {
    println(format(args));
  }

  time(tag = 'default') {
    tag = toString(tag);
    if (this.#timeTags.has(tag)) {
      this.error(`tag '${tag}' already exists for console.time()`);
    } else {
      this.#timeTags.set(tag, now());
    }
  }

  timeEnd(tag = 'default') {
    tag = toString(tag);

    const start = this.#timeTags.get(tag);

    if (start === undefined) {
      this.error(`No such label '${tag}' for console.timeEnd()`);
    } else {
      println(`${tag}: ${now() - start}ms`);
      this.#timeTags.delete(tag);
    }
  }

  count(label = 'default') {
    label = toString(label);
    const count = (this.#countTags.get(label) ?? 0) + 1;

    this.#countTags.set(label, count);

    println(`${label}: ${count}`);
  }

  countReset(label = 'default') {
    this.#countTags.delete(toString(label));
  }
}

function format(args) {
  const [s] = args;

  if (typeof s === 'string') {
    if (s.includes('%')) {
      // fall through to format
    } else if (args.length === 1) {
      return s;
    } else {
      return args.map(formatValue).join(' ');
    }
  } else {
    if (args.length === 0) {
      return '';
    } else if (args.length === 1) {
      return formatValue(s);
    } else {
      return args.map(formatValue).join(' ');
    }
  }

  let i = 1;
  let arg_string;
  let str = '';
  let start = 0;
  let end = 0;

  while (end < s.length) {
    if (s.charAt(end) !== '%') {
      end++;
      continue;
    }

    str += s.slice(start, end);

    switch (s.charAt(end + 1)) {
      case 's':
        arg_string = String(args[i]);
        break;
      case 'd':
        arg_string = Number(args[i]);
        break;
      case 'j':
        try {
          arg_string = stringify(args[i]);
        } catch {
          arg_string = '[Circular]';
        }
        break;
      case '%':
        str += '%';
        start = end = end + 2;
        continue;
      default:
        str = `${str}%${s.charAt(end + 1)}`;
        start = end = end + 2;
        continue;
    }

    if (i >= args.length) {
      str = `${str}%${s.charAt(end + 1)}`;
    } else {
      i++;
      str += arg_string;
    }

    start = end = end + 2;
  }

  str += s.slice(start, end);

  while (i < args.length) {
    str += ' ' + formatValue(args[i++]);
  }

  return str;
}

function formatValue(v) {
  if (!v) {
    return `${v}`;
  }

  const type = typeof v;

  if (type === 'string') {
    return v;
  }

  if (type === 'symbol') {
    return v.toString();
  }

  if (isArray(v)) {
    return `[${v.toString()}]`;
  }

  if (v instanceof Error) {
    const { message, stack } = v;
    const result = v.code ? `Error [${v.code}]: ${message}` : `Error: ${message}`;

    if (isArray(stack)) {
      return result + '\n' + stack.map((line) => `    at ${line}`).join('\n');
    }

    return result;
  }

  if (type === 'object') {
    return stringify(v, null, 2);
  }

  return toString(v);
}

function toString(value) {
  return (typeof value === 'string') ? value :`${value}`;
}

globalThis.console = new Console();
