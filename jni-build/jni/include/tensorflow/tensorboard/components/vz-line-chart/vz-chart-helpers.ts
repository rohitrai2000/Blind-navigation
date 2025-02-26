/* Copyright 2015 The TensorFlow Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/
/* tslint:disable:no-namespace variable-name */
module VZ.ChartHelpers {
  export interface Datum {
    wall_time: Date;
    step: number;
  }

  export interface Scalar {
    scalar: number;
    smoothed: number;
  }

  export type ScalarDatum = Datum & Scalar;

  export type DataFn = (run: string, tag: string) =>
      Promise<Array<Datum>>;

  export let Y_TOOLTIP_FORMATTER_PRECISION = 4;
  export let STEP_FORMATTER_PRECISION = 4;
  export let Y_AXIS_FORMATTER_PRECISION = 3;
  export let TOOLTIP_Y_PIXEL_OFFSET = 20;
  export let TOOLTIP_CIRCLE_SIZE = 4;
  export let NAN_SYMBOL_SIZE = 6;

  export interface Point {
    x: number;  // pixel space
    y: number;  // pixel space
    datum: ScalarDatum;
    dataset: Plottable.Dataset;
  }

  /* Create a formatter function that will switch between exponential and
   * regular display depending on the scale of the number being formatted,
   * and show `digits` significant digits.
   */
  export function multiscaleFormatter(digits: number): ((v: number) => string) {
    return (v: number) => {
      let absv = Math.abs(v);
      if (absv < 1E-15) {
        // Sometimes zero-like values get an annoying representation
        absv = 0;
      }
      let f: (x: number) => string;
      if (absv >= 1E4) {
        f = d3.format('.' + digits + 'e');
      } else if (absv > 0 && absv < 0.01) {
        f = d3.format('.' + digits + 'e');
      } else {
        f = d3.format('.' + digits + 'g');
      }
      return f(v);
    };
  }

  export function accessorize(key: string): Plottable.Accessor<number> {
    return (d: any, index: number, dataset: Plottable.Dataset) => d[key];
  }

  export interface XComponents {
    /* tslint:disable */
    scale: Plottable.Scales.Linear|Plottable.Scales.Time,
        axis: Plottable.Axes.Numeric|Plottable.Axes.Time,
        accessor: Plottable.Accessor<number|Date>,
    /* tslint:enable */
  }

  export let stepFormatter =
      Plottable.Formatters.siSuffix(STEP_FORMATTER_PRECISION);
  export function stepX(): XComponents {
    let scale = new Plottable.Scales.Linear();
    let axis = new Plottable.Axes.Numeric(scale, 'bottom');
    axis.formatter(stepFormatter);
    return {
      scale: scale,
      axis: axis,
      accessor: (d: Datum) => d.step,
    };
  }

  export let timeFormatter = Plottable.Formatters.time('%a %b %e, %H:%M:%S');

  export function wallX(): XComponents {
    let scale = new Plottable.Scales.Time();
    return {
      scale: scale,
      axis: new Plottable.Axes.Time(scale, 'bottom'),
      accessor: (d: Datum) => d.wall_time,
    };
  }
  export let relativeAccessor =
      (d: any, index: number, dataset: Plottable.Dataset) => {
        // We may be rendering the final-point datum for scatterplot.
        // If so, we will have already provided the 'relative' property
        if (d.relative != null) {
          return d.relative;
        }
        let data = dataset.data();
        // I can't imagine how this function would be called when the data is
        // empty (after all, it iterates over the data), but lets guard just
        // to be safe.
        let first = data.length > 0 ? +data[0].wall_time : 0;
        return (+d.wall_time - first) / (60 * 60 * 1000);  // ms to hours
      };

  export let relativeFormatter = (n: number) => {
    // we will always show 2 units of precision, e.g days and hours, or
    // minutes and seconds, but not hours and minutes and seconds
    let ret = '';
    let days = Math.floor(n / 24);
    n -= (days * 24);
    if (days) {
      ret += days + 'd ';
    }
    let hours = Math.floor(n);
    n -= hours;
    n *= 60;
    if (hours || days) {
      ret += hours + 'h ';
    }
    let minutes = Math.floor(n);
    n -= minutes;
    n *= 60;
    if (minutes || hours || days) {
      ret += minutes + 'm ';
    }
    let seconds = Math.floor(n);
    return ret + seconds + 's';
  };
  export function relativeX(): XComponents {
    let scale = new Plottable.Scales.Linear();
    return {
      scale: scale,
      axis: new Plottable.Axes.Numeric(scale, 'bottom'),
      accessor: relativeAccessor,
    };
  }

  // a very literal definition of NaN: true for NaN for a non-number type
  // or null, etc. False for Infinity or -Infinity
  export let isNaN = (x) => +x !== x;

  export function getXComponents(xType: string): XComponents {
    switch (xType) {
      case 'step':
        return stepX();
      case 'wall_time':
        return wallX();
      case 'relative':
        return relativeX();
      default:
        throw new Error('invalid xType: ' + xType);
    }
  }
}
