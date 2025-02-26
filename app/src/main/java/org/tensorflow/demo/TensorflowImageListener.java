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

/*  This file has been modified by Nataniel Ruiz affiliated with Wall Lab
 *  at the Georgia Institute of Technology School of Interactive Computing
 */

package org.tensorflow.demo;

import android.content.Context;
import android.content.Intent;
import android.content.res.AssetManager;
import android.graphics.Bitmap;
import android.graphics.Bitmap.Config;
import android.graphics.Canvas;
import android.graphics.Matrix;
import android.media.Image;
import android.media.Image.Plane;
import android.media.ImageReader;
import android.media.ImageReader.OnImageAvailableListener;
import android.os.Build;
import android.os.Handler;
import android.os.Looper;
import android.os.Trace;
import android.os.Vibrator;
import android.speech.tts.TextToSpeech;

import junit.framework.Assert;

import org.tensorflow.demo.env.ImageUtils;
import org.tensorflow.demo.env.Logger;

import java.util.List;
import java.util.Locale;

/**
 * Class that takes in preview frames and converts the image to Bitmaps to process with Tensorflow.
 */
class TensorFlowImageListener implements OnImageAvailableListener {
  private static final Logger LOGGER = new Logger();

  static Context context;
  private static final boolean SAVE_PREVIEW_BITMAP = true;

  // These are the settings for the original v1 Inception model. If you want to
  // use a model that's been produced from the TensorFlow for Poets codelab,
  // you'll need to set IMAGE_SIZE = 299, IMAGE_MEAN = 128, IMAGE_STD = 128,
  // INPUT_NAME = "Mul:0", and OUTPUT_NAME = "final_result:0".
  // You'll also need to update the MODEL_FILE and LABEL_FILE paths to point to
  // the ones you produced.
  private static final int NUM_CLASSES = 1470;
  private static final int INPUT_SIZE = 448;
  private static final int IMAGE_MEAN = 128;
  private static final float IMAGE_STD = 128;
  private static final String INPUT_NAME = "Placeholder";
  private static final String OUTPUT_NAME = "19_fc";

  private static final String MODEL_FILE = "file:///android_asset/android_graph.pb";
  private static final String LABEL_FILE =
          "file:///android_asset/label_strings.txt";

  private Integer sensorOrientation;

  private final TensorFlowClassifier tensorflow = new TensorFlowClassifier();

  private int previewWidth = 0;
  private int previewHeight = 0;
  private byte[][] yuvBytes;
  private int[] rgbBytes = null;
  private Bitmap rgbFrameBitmap = null;
  private Bitmap croppedBitmap = null;

  private boolean computing = false;
  private boolean readyForNextImage = true;
  private Handler handler;

  private RecognitionScoreView scoreView;
  private BoundingBoxView boundingView;

  static List<Classifier.Recognition> results;

  public void initialize(
          final AssetManager assetManager,
          final RecognitionScoreView scoreView,
          final BoundingBoxView boundingView,
          final Handler handler,
          final Integer sensorOrientation) {
    Assert.assertNotNull(sensorOrientation);
    tensorflow.initializeTensorFlow(
            assetManager, MODEL_FILE, LABEL_FILE, NUM_CLASSES, INPUT_SIZE, IMAGE_MEAN, IMAGE_STD,
            INPUT_NAME, OUTPUT_NAME);
    this.scoreView = scoreView;
    this.boundingView = boundingView;
    this.handler = handler;
    this.sensorOrientation = sensorOrientation;
  }

  private void drawResizedBitmap(final Bitmap src, final Bitmap dst) {
    Assert.assertEquals(dst.getWidth(), dst.getHeight());
    final float minDim = Math.min(src.getWidth(), src.getHeight());

    final Matrix matrix = new Matrix();

    // We only want the center square out of the original rectangle.
    final float translateX = -Math.max(0, (src.getWidth() - minDim) / 2);
    final float translateY = -Math.max(0, (src.getHeight() - minDim) / 2);
    matrix.preTranslate(translateX, translateY);

    final float scaleFactor = dst.getHeight() / minDim;
    matrix.postScale(scaleFactor, scaleFactor);

    // Rotate around the center if necessary.
    // Nataniel: added rotation because image is rotated on my device (Pixel C tablet)
    // TODO: Find out if this is happenning in every device.
    sensorOrientation = 90;
    if (sensorOrientation != 0) {
      matrix.postTranslate(-dst.getWidth() / 2.0f, -dst.getHeight() / 2.0f);
      matrix.postRotate(sensorOrientation);
      matrix.postTranslate(dst.getWidth() / 2.0f, dst.getHeight() / 2.0f);
    }
    //LOGGER.i("sensorOrientationImageListener" + sensorOrientation.toString());

    final Canvas canvas = new Canvas(dst);
    canvas.drawBitmap(src, matrix, null);
  }

  @Override
  public void onImageAvailable(final ImageReader reader) {
    Image image = null;
    try {
      image = reader.acquireLatestImage();

      if (image == null) {
        return;
      }

      // No mutex needed as this method is not reentrant.
      if (computing || !readyForNextImage) {
        image.close();
        return;
      }
      readyForNextImage = true;
      computing = true;

      Trace.beginSection("imageAvailable");

      final Plane[] planes = image.getPlanes();

      // Initialize the storage bitmaps once when the resolution is known.
      if (previewWidth != image.getWidth() || previewHeight != image.getHeight()) {
        previewWidth = image.getWidth();
        previewHeight = image.getHeight();

        LOGGER.i("Initializing at size %dx%d", previewWidth, previewHeight);
        rgbBytes = new int[previewWidth * previewHeight];
        rgbFrameBitmap = Bitmap.createBitmap(previewWidth, previewHeight, Config.ARGB_8888);
        croppedBitmap = Bitmap.createBitmap(INPUT_SIZE, INPUT_SIZE, Config.ARGB_8888);

        yuvBytes = new byte[planes.length][];
        for (int i = 0; i < planes.length; ++i) {
          yuvBytes[i] = new byte[planes[i].getBuffer().capacity()];
        }
      }

      for (int i = 0; i < planes.length; ++i) {
        planes[i].getBuffer().get(yuvBytes[i]);
      }

      final int yRowStride = planes[0].getRowStride();
      final int uvRowStride = planes[1].getRowStride();
      final int uvPixelStride = planes[1].getPixelStride();
      ImageUtils.convertYUV420ToARGB8888(
              yuvBytes[0],
              yuvBytes[1],
              yuvBytes[2],
              rgbBytes,
              previewWidth,
              previewHeight,
              yRowStride,
              uvRowStride,
              uvPixelStride,
              false);

      image.close();
    } catch (final Exception e) {
      if (image != null) {
        image.close();
      }
      LOGGER.e(e, "Exception!");
      Trace.endSection();
      return;
    }

    rgbFrameBitmap.setPixels(rgbBytes, 0, previewWidth, 0, 0, previewWidth, previewHeight);
    drawResizedBitmap(rgbFrameBitmap, croppedBitmap);

    // For examining the actual TF input.
    if (SAVE_PREVIEW_BITMAP) {
      ImageUtils.saveBitmap(croppedBitmap);
    }

    handler.post(
            new Runnable() {
              @Override
              public void run() {
                results = tensorflow.recognizeImage(croppedBitmap);

                System.out.println("Resul Sanjadgsajhdg*****  "+results);

                LOGGER.v("%d results", results.size());
                for (final Classifier.Recognition result : results) {
                  LOGGER.v("Result: " + result.getTitle());
                }
                scoreView.setResults(results);
                boundingView.setResults(results);

                System.out.println("Object to search is : "+CameraActivity.getObjectToSearch());
                if (results.get(0).getTitle().equals(CameraActivity.getObjectToSearch())){
                  CameraConnectionFragment.objectFound();
                }
                CameraConnectionFragment.prevObj = TensorFlowImageListener.getResults().get(0).getTitle();
                computing = false;
              }
            });

    Trace.endSection();
  }

  public static List<Classifier.Recognition> getResults(){
    return results;
  }

  public void takePic() {
    readyForNextImage = true;
    LOGGER.v("Taking picture");
    return;
  }

  public static int getInputSize() {
    return INPUT_SIZE;
  }
}