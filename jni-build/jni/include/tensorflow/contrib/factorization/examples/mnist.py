# Copyright 2016 The TensorFlow Authors. All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
# ==============================================================================
"""Example mnist model with jointly computed k-means clustering.

This is a toy example of how clustering can be embedded into larger tensorflow
graphs. In this case, we learn a clustering on-the-fly and transform the input
into the 'distance to clusters' space. These are then fed into hidden layers to
learn the supervised objective.

To train this model on real mnist data, run this model as follows:
  mnist --nofake_data --max_steps=2000
"""

# pylint: disable=missing-docstring
from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

import math
import tempfile
import time

from six.moves import xrange  # pylint: disable=redefined-builtin
import tensorflow as tf

from tensorflow.contrib.factorization.python.ops import clustering_ops
from tensorflow.examples.tutorials.mnist import input_data
from tensorflow.examples.tutorials.mnist import mnist

# Basic model parameters as external flags.
flags = tf.app.flags
FLAGS = flags.FLAGS
flags.DEFINE_float('learning_rate', 0.3, 'Initial learning rate.')
flags.DEFINE_integer('max_steps', 200, 'Number of steps to run trainer.')
flags.DEFINE_integer('num_clusters', 384, 'Number of input feature clusters')
flags.DEFINE_integer('hidden1', 256, 'Number of units in hidden layer 1.')
flags.DEFINE_integer('hidden2', 32, 'Number of units in hidden layer 2.')
flags.DEFINE_integer('batch_size', 100, 'Batch size.  '
                     'Must divide evenly into the dataset sizes.')
flags.DEFINE_string('train_dir', 'data', 'Directory to put the training data.')
flags.DEFINE_bool('fake_data', True, 'Use fake input data.')

# The MNIST dataset has 10 classes, representing the digits 0 through 9.
NUM_CLASSES = 10

# The MNIST images are always 28x28 pixels.
IMAGE_SIZE = 28
IMAGE_PIXELS = IMAGE_SIZE * IMAGE_SIZE


def placeholder_inputs():
  """Generate placeholder variables to represent the input tensors.

  Returns:
    images_placeholder: Images placeholder.
    labels_placeholder: Labels placeholder.
  """
  images_placeholder = tf.placeholder(tf.float32, shape=(None,
                                                         mnist.IMAGE_PIXELS))
  labels_placeholder = tf.placeholder(tf.int32, shape=(None))
  return images_placeholder, labels_placeholder


def fill_feed_dict(data_set, images_pl, labels_pl, batch_size):
  """Fills the feed_dict for training the given step.

  Args:
    data_set: The set of images and labels, from input_data.read_data_sets()
    images_pl: The images placeholder, from placeholder_inputs().
    labels_pl: The labels placeholder, from placeholder_inputs().
    batch_size: Batch size of data to feed.

  Returns:
    feed_dict: The feed dictionary mapping from placeholders to values.
  """
  # Create the feed_dict for the placeholders filled with the next
  # `batch size ` examples.
  images_feed, labels_feed = data_set.next_batch(batch_size, FLAGS.fake_data)
  feed_dict = {
      images_pl: images_feed,
      labels_pl: labels_feed,
  }
  return feed_dict


def do_eval(sess,
            eval_correct,
            images_placeholder,
            labels_placeholder,
            data_set):
  """Runs one evaluation against the full epoch of data.

  Args:
    sess: The session in which the model has been trained.
    eval_correct: The Tensor that returns the number of correct predictions.
    images_placeholder: The images placeholder.
    labels_placeholder: The labels placeholder.
    data_set: The set of images and labels to evaluate, from
      input_data.read_data_sets().
  Returns:
    Precision value on the dataset.
  """
  # And run one epoch of eval.
  true_count = 0  # Counts the number of correct predictions.
  steps_per_epoch = data_set.num_examples // FLAGS.batch_size
  num_examples = steps_per_epoch * FLAGS.batch_size
  for step in xrange(steps_per_epoch):
    feed_dict = fill_feed_dict(data_set,
                               images_placeholder,
                               labels_placeholder,
                               FLAGS.batch_size)
    true_count += sess.run(eval_correct, feed_dict=feed_dict)
  precision = true_count / num_examples
  print('  Num examples: %d  Num correct: %d  Precision @ 1: %0.04f' %
        (num_examples, true_count, precision))
  return precision


def inference(inp, num_clusters, hidden1_units, hidden2_units):
  """Build the MNIST model up to where it may be used for inference.

  Args:
    inp: input data
    num_clusters: number of clusters of input features to train.
    hidden1_units: Size of the first hidden layer.
    hidden2_units: Size of the second hidden layer.

  Returns:
    logits: Output tensor with the computed logits.
    clustering_loss: Clustering loss.
    kmeans_training_op: An op to train the clustering.
  """
  # Clustering
  kmeans = clustering_ops.KMeans(
      inp,
      num_clusters,
      distance_metric=clustering_ops.COSINE_DISTANCE,
      # TODO(agarwal): kmeans++ is currently causing crash in dbg mode.
      # Enable this after fixing.
      # initial_clusters=clustering_ops.KMEANS_PLUS_PLUS_INIT,
      use_mini_batch=True)

  all_scores, _, clustering_scores, kmeans_training_op = kmeans.training_graph()
  # Some heuristics to approximately whiten this output.
  all_scores = (all_scores[0] - 0.5) * 5
  # Here we avoid passing the gradients from the supervised objective back to
  # the clusters by creating a stop_gradient node.
  all_scores = tf.stop_gradient(all_scores)
  clustering_loss = tf.reduce_sum(clustering_scores[0])
  # Hidden 1
  with tf.name_scope('hidden1'):
    weights = tf.Variable(
        tf.truncated_normal([num_clusters, hidden1_units],
                            stddev=1.0 / math.sqrt(float(IMAGE_PIXELS))),
        name='weights')
    biases = tf.Variable(tf.zeros([hidden1_units]),
                         name='biases')
    hidden1 = tf.nn.relu(tf.matmul(all_scores, weights) + biases)
  # Hidden 2
  with tf.name_scope('hidden2'):
    weights = tf.Variable(
        tf.truncated_normal([hidden1_units, hidden2_units],
                            stddev=1.0 / math.sqrt(float(hidden1_units))),
        name='weights')
    biases = tf.Variable(tf.zeros([hidden2_units]),
                         name='biases')
    hidden2 = tf.nn.relu(tf.matmul(hidden1, weights) + biases)
  # Linear
  with tf.name_scope('softmax_linear'):
    weights = tf.Variable(
        tf.truncated_normal([hidden2_units, NUM_CLASSES],
                            stddev=1.0 / math.sqrt(float(hidden2_units))),
        name='weights')
    biases = tf.Variable(tf.zeros([NUM_CLASSES]),
                         name='biases')
    logits = tf.matmul(hidden2, weights) + biases
  return logits, clustering_loss, kmeans_training_op


def run_training():
  """Train MNIST for a number of steps."""
  # Get the sets of images and labels for training, validation, and
  # test on MNIST.
  train_dir = tempfile.mkdtemp()
  data_sets = input_data.read_data_sets(train_dir, FLAGS.fake_data)

  # Tell TensorFlow that the model will be built into the default Graph.
  with tf.Graph().as_default():
    # Generate placeholders for the images and labels.
    images_placeholder, labels_placeholder = placeholder_inputs()

    # Build a Graph that computes predictions from the inference model.
    logits, clustering_loss, kmeans_training_op = inference(images_placeholder,
                                                            FLAGS.num_clusters,
                                                            FLAGS.hidden1,
                                                            FLAGS.hidden2)

    # Add to the Graph the Ops for loss calculation.
    loss = mnist.loss(logits, labels_placeholder)

    # Add to the Graph the Ops that calculate and apply gradients.
    train_op = tf.group(mnist.training(loss, FLAGS.learning_rate),
                        kmeans_training_op)

    # Add the Op to compare the logits to the labels during evaluation.
    eval_correct = mnist.evaluation(logits, labels_placeholder)

    # Add the variable initializer Op.
    init = tf.initialize_all_variables()

    # Create a session for running Ops on the Graph.
    sess = tf.Session()

    feed_dict = fill_feed_dict(data_sets.train,
                               images_placeholder,
                               labels_placeholder,
                               batch_size=5000)
    # Run the Op to initialize the variables.
    sess.run(init, feed_dict=feed_dict)

    # Start the training loop.
    max_test_prec = 0
    for step in xrange(FLAGS.max_steps):
      start_time = time.time()

      # Fill a feed dictionary with the actual set of images and labels
      # for this particular training step.
      feed_dict = fill_feed_dict(data_sets.train,
                                 images_placeholder,
                                 labels_placeholder,
                                 FLAGS.batch_size)

      # Run one step of the model.
      _, loss_value, clustering_loss_value = sess.run([train_op,
                                                       loss,
                                                       clustering_loss],
                                                      feed_dict=feed_dict)

      duration = time.time() - start_time
      if step % 100 == 0:
        # Print status to stdout.
        print('Step %d: loss = %.2f, clustering_loss = %.2f (%.3f sec)' % (
            step, loss_value, clustering_loss_value, duration))

      # Save a checkpoint and evaluate the model periodically.
      if (step + 1) % 1000 == 0 or (step + 1) == FLAGS.max_steps:
        # Evaluate against the training set.
        print('Training Data Eval:')
        do_eval(sess,
                eval_correct,
                images_placeholder,
                labels_placeholder,
                data_sets.train)
        # Evaluate against the validation set.
        print('Validation Data Eval:')
        do_eval(sess,
                eval_correct,
                images_placeholder,
                labels_placeholder,
                data_sets.validation)
        # Evaluate against the test set.
        print('Test Data Eval:')
        test_prec = do_eval(sess,
                            eval_correct,
                            images_placeholder,
                            labels_placeholder,
                            data_sets.test)
        max_test_prec = max(max_test_prec, test_prec)
    return max_test_prec


class MnistTest(tf.test.TestCase):

  def test_train(self):
    self.assertTrue(run_training() > 0.6)


if __name__ == '__main__':
  tf.test.main()
