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

const { now, timeOrigin } = module.bindings;

const { toStringTag } = Symbol;
const { prototype:ObjectPrototype, getPrototypeOf } = Object;

function isObject(value) {
  return (value !== null && typeof value === 'object' && getPrototypeOf(value) === ObjectPrototype);
}

class ERR_PERFORMANCE_MEASURE_INVALID_OPTIONS extends TypeError {
  constructor(msg) {
    super(msg);
    this.code = 'ERR_PERFORMANCE_MEASURE_INVALID_OPTIONS';
  }
}

class ERR_PERFORMANCE_INVALID_TIMESTAMP extends TypeError {
  constructor(timestamp) {
    super(`${timestamp} is not a valid timestamp`);
    this.code = 'ERR_PERFORMANCE_INVALID_TIMESTAMP';
  }
}

class Performance {
  #entriesByStartTime = [];
  #entriesByName = new Map();

  get timeOrigin () {
    return timeOrigin;
  }

  now() {
    return now();
  }

  mark(name, opts = undefined) {
    const startTime = opts?.startTime ?? now();

    if (typeof startTime !== 'number' || startTime < 0) {
      throw new ERR_PERFORMANCE_INVALID_TIMESTAMP(startTime);
    }

    const mark = new PerformanceMark(`${name}`, 'mark', startTime, 0, opts?.detail ?? null);

    this.#entriesByStartTime.push(mark);
    this.#entriesByName.set(mark.name, mark);

    return mark;
  }

  measure(name, startMarkOrOptions, endMark) {
    let start;
    let end;
    let duration;
    let detail;
    let optionsValid = false;

    if (isObject(startMarkOrOptions)) {
      ({ start, end, duration, detail } = startMarkOrOptions);
      optionsValid = start !== undefined || end !== undefined;
    }

    if (optionsValid) {
      if (endMark !== undefined) {
        throw new ERR_PERFORMANCE_MEASURE_INVALID_OPTIONS(
          'endMark must not be specified');
      }

      if (start === undefined && end === undefined) {
        throw new ERR_PERFORMANCE_MEASURE_INVALID_OPTIONS(
          'One of options.start or options.end is required');
      }

      if (start !== undefined && end !== undefined && duration !== undefined) {
        throw new ERR_PERFORMANCE_MEASURE_INVALID_OPTIONS(
          'Must not have options.start, options.end, and options.duration specified');
      }
    }

    if (endMark !== undefined) {
      end = this.#getStartTime(endMark);
    } else if (optionsValid && end !== undefined) {
      end = this.#getStartTime(end);
    } else if (optionsValid && start !== undefined && duration !== undefined) {
      end = this.#getStartTime(start) + this.#getStartTime(duration);
    } else {
      end = now();
    }

    if (typeof startMarkOrOptions === 'string') {
      start = this.#getStartTime(startMarkOrOptions);
    } else if (optionsValid && start !== undefined) {
      start = this.#getStartTime(start);
    } else if (optionsValid && duration !== undefined && end !== undefined) {
      start = end - this.#getStartTime(duration);
    } else {
      start = 0;
    }

    const measure = new PerformanceMeasure(`${name}`, 'measure', start, duration ?? (end - start), detail ?? null);

    this.#entriesByStartTime.push(measure);

    return measure;
  }

  getEntries() {
    return [ ...this.#entriesByStartTime ];
  }

  getEntriesByType(type) {
    return this.#entriesByStartTime.filter(entry => entry.entryType === type);
  }

  getEntriesByName(name) {
    return this.#entriesByStartTime.filter(entry => entry.name === name);
  }

  clearMarks(name) {
    this.#clear(name, 'mark');
  }

  clearMeasures(name) {
    this.#clear(name, 'measure');
  }

  [toStringTag] = 'Performance';

  #clear(name, type) {
    if (name !== undefined) {
      this.#entriesByName.delete((name = `${name}`));
    }
    this.#entriesByStartTime = this.#entriesByStartTime.filter(entry => entry.entryType !== type && entry.name !== name);
  }

  #getStartTime(mark) {
    if (mark === undefined) return;

    if (typeof mark === 'number') {
      if (mark < 0) {
        throw new ERR_PERFORMANCE_INVALID_TIMESTAMP(mark);
      }
      return mark;
    }

    const entry = this.#entriesByName.get(mark);

    if (entry === undefined) {
      throw new (globalThis.DOMException ?? SyntaxError) (`The '${mark}' performance mark has not been set.`, 'SyntaxError');
    }

    return entry.startTime;
  }
}

class PerformanceEntry {
  #name;
  #entryType;
  #startTime;
  #duration;

  constructor(name, entryType, startTime, duration) {
    this.#name = name;
    this.#entryType = entryType;
    this.#startTime = startTime;
    this.#duration = duration;
  }

  get name() {
    return this.#name;
  }

  get entryType() {
    return this.#entryType;
  }

  get startTime() {
    return this.#startTime;
  }

  get duration() {
    return this.#duration;
  }

  [toStringTag] = 'PerformanceEntry';
}

class PerformanceMark extends PerformanceEntry {
  #detail;

  constructor (name, entryType, startTime, duration, detail) {
    super(name, entryType, startTime, duration);
    this.#detail = detail;
  }

  [toStringTag] = 'PerformanceMark';
}

class PerformanceMeasure extends PerformanceEntry {
  #detail;

  constructor(name, entryType, startTime, duration, detail) {
    super(name, entryType, startTime, duration);
    this.#detail = detail;
  }

  [toStringTag] = 'PerformanceMeasure';
}

module.exports = new Performance();
