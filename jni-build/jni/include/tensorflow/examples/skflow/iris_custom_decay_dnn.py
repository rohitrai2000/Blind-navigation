#  Copyright 2016 The TensorFlow Authors. All Rights Reserved.
#
#  Licensed under the Apache License, Version 2.0 (the "License");
#  you may not use this file except in compliance with the License.
#  You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.
from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

from sklearn import datasets
from sklearn import metrics
from sklearn.cross_validation import train_test_split
import tensorflow as tf


def optimizer_exp_decay():
  global_step = tf.contrib.framework.get_or_create_global_step()
  learning_rate = tf.train.exponential_decay(
      learning_rate=0.1, global_step=global_step,
      decay_steps=100, decay_rate=0.001)
  return tf.train.AdagradOptimizer(learning_rate=learning_rate)


def main(unused_argv):
  iris = datasets.load_iris()
  x_train, x_test, y_train, y_test = train_test_split(
      iris.data, iris.target, test_size=0.2, random_state=42)

  feature_columns = tf.contrib.learn.infer_real_valued_columns_from_input(
      x_train)
  classifier = tf.contrib.learn.DNNClassifier(feature_columns=feature_columns,
                                              hidden_units=[10, 20, 10],
                                              n_classes=3,
                                              optimizer=optimizer_exp_decay)

  classifier.fit(x_train, y_train, steps=800)
  score = metrics.accuracy_score(y_test, classifier.predict(x_test))
  print('Accuracy: {0:f}'.format(score))


if __name__ == '__main__':
  tf.app.run()
