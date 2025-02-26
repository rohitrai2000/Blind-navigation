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

var gulp = require('gulp');
var fs = require('fs');
var path = require('path');
var vulcanize = require('gulp-vulcanize');
var replace = require('gulp-replace');
var rename = require('gulp-rename');
var header = require('gulp-header');

var HEADER_STR = '<!-- Copyright 2015 The TensorFlow Authors. All Rights Reserved.\n\
\n\
Licensed under the Apache License, Version 2.0 (the "License");\n\
you may not use this file except in compliance with the License.\n\
You may obtain a copy of the License at\n\
\n\
   http://www.apache.org/licenses/LICENSE-2.0\n\
\n\
Unless required by applicable law or agreed to in writing, software\n\
distributed under the License is distributed on an "AS IS" BASIS,\n\
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.\n\
See the License for the specific language governing permissions and\n\
limitations under the License.\n\
============================================================================\n\
\n\
This file is generated by `gulp` & `vulcanize`. Do not directly change it.\n\
Instead, use `gulp regenerate` to create a new version with your changes.\n\
-->\n\n'

/**
 * Returns a list of non-tensorboard components inside the components
 * directory, i.e. components that don't begin with 'tf-' or 'vz-''.
 */
function getNonTensorBoardComponents() {
  return fs.readdirSync('components')
      .filter(function(file) {
        var prefix = file.slice(0,3);
        return fs.statSync(path.join('components', file)).isDirectory() &&
            prefix !== 'tf-'  && prefix !== 'vz-';
      })
      .map(function(dir) { return '/' + dir + '/'; });
}

var linkRegex = /<link rel="[^"]*" (type="[^"]*" )?href="[^"]*">\n/g;
var scriptRegex = /<script src="[^"]*"><\/script>\n/g;

module.exports = function(overwrite) {
  return function() {
    var suffix = overwrite ? '' : '.OPENSOURCE';
    // Vulcanize TensorBoard without external libraries.
    gulp.src('components/tf-tensorboard/tf-tensorboard.html')
        .pipe(vulcanize({
          inlineScripts: true,
          inlineCss: true,
          stripComments: true,
          excludes: getNonTensorBoardComponents(),
        }))
        // TODO(danmane): Remove this worrisome brittleness when vulcanize
        // fixes https://github.com/Polymer/vulcanize/issues/273
        .pipe(replace(linkRegex, ''))
        .pipe(replace(scriptRegex, ''))
        .pipe(header(HEADER_STR))
        .pipe(rename('tf-tensorboard.html' + suffix))
        .pipe(gulp.dest('./dist'));
  }
}
