#include "libuvc/libuvc.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

/* This callback function runs once per frame. Use it to perform any
 * quick processing you need, or have it put the frame into your application's
 * input queue. If this function takes too long, you'll start losing frames. */
void cb(uvc_frame_t *frame, void *ptr) {
  uvc_frame_t *bgr;
  uvc_error_t ret;
  enum uvc_frame_format *frame_format = (enum uvc_frame_format *)ptr;
  /* FILE *fp;
   * static int jpeg_count = 0;
   * static const char *H264_FILE = "iOSDevLog.h264";
   * static const char *MJPEG_FILE = ".jpeg";
   * char filename[16]; */

  /* We'll convert the image from YUV/JPEG to BGR, so allocate space */
  bgr = uvc_allocate_frame(frame->width * frame->height * 3);
  if (!bgr) {
    printf("unable to allocate bgr frame!\n");
    return;
  }

  printf("callback! frame_format = %d, width = %d, height = %d, length = %lu, ptr = %p\n",
    frame->frame_format, frame->width, frame->height, frame->data_bytes, ptr);

  switch (frame->frame_format) {
  case UVC_FRAME_FORMAT_H264:
    /* use `ffplay H264_FILE` to play */
    /* fp = fopen(H264_FILE, "a");
     * fwrite(frame->data, 1, frame->data_bytes, fp);
     * fclose(fp); */
    break;
  case UVC_COLOR_FORMAT_MJPEG:
    /* sprintf(filename, "%d%s", jpeg_count++, MJPEG_FILE);
     * fp = fopen(filename, "w");
     * fwrite(frame->data, 1, frame->data_bytes, fp);
     * fclose(fp); */
    break;
  case UVC_COLOR_FORMAT_YUYV:
    /* Do the BGR conversion */
    ret = uvc_any2bgr(frame, bgr);
    if (ret) {
      uvc_perror(ret, "uvc_any2bgr");
      uvc_free_frame(bgr);
      return;
    }
    break;
  default:
    break;
  }

  /* Call a user function:
   *
   * my_type *my_obj = (*my_type) ptr;
   * my_user_function(ptr, bgr);
   * my_other_function(ptr, bgr->data, bgr->width, bgr->height);
   */

  /* Call a C++ method:
   *
   * my_type *my_obj = (*my_type) ptr;
   * my_obj->my_func(bgr);
   */

  /* Use opencv.highgui to display the image:
   *
   * cvImg = cvCreateImageHeader(
   *     cvSize(bgr->width, bgr->height),
   *     IPL_DEPTH_8U,
   *     3);
   *
   * cvSetData(cvImg, bgr->data, bgr->width * 3);
   *
   * cvNamedWindow("Test", CV_WINDOW_AUTOSIZE);
   * cvShowImage("Test", cvImg);
   * cvWaitKey(10);
   *
   * cvReleaseImageHeader(&cvImg);
   */

  uvc_free_frame(bgr);
}

int main(int argc, char **argv) {

  if (argc < 3) {
    printf("Usage: %s brightness_pct white_balance_kelvin [gain, default=200]", argv[0]);
    return 1;
  }

  int16_t brightness = atoi(argv[1]);
  uint8_t wb_auto = 0;
  uint16_t wb = 0;
  uint16_t gain = 200;

  if (strcmp(argv[2], "auto") == 0) {
    wb_auto = 1;
    printf("Setting brightness = %d%%, white balance = (auto)...\n", brightness);
  } else {
    wb = atoi(argv[2]);
    printf("Setting brightness = %d%%, white balance = %d K...\n", brightness, wb);
  }
  if (argc > 3) {
    gain = atoi(argv[3]);
  }

  uvc_context_t *ctx;
  uvc_device_t *dev;
  uvc_device_handle_t *devh;
  uvc_stream_ctrl_t ctrl;
  uvc_error_t res;

  /* Initialize a UVC service context. Libuvc will set up its own libusb
   * context. Replace NULL with a libusb_context pointer to run libuvc
   * from an existing libusb context. */
  res = uvc_init(&ctx, NULL);

  if (res < 0) {
    uvc_perror(res, "uvc_init");
    return res;
  }

  puts("UVC initialized");

  /* Locates the first attached UVC device, stores in dev */
  res = uvc_find_device(
      ctx, &dev,
      0, 0, NULL); /* filter devices: vendor_id, product_id, "serial_num" */

  if (res < 0) {
    uvc_perror(res, "uvc_find_device"); /* no devices found */
  } else {
    puts("Device found");

    /* Try to open the device: requires exclusive access */
    res = uvc_open(dev, &devh);

    if (res < 0) {
      uvc_perror(res, "uvc_open"); /* unable to open device */
    } else {
      puts("Device opened");

      res = uvc_set_ae_mode(devh, 1);  // manual exposure
      if (res < 0) {
        uvc_perror(res, "set_ae_mode");
      }

      res = uvc_set_gain(devh, gain);  // gain control
      if (res < 0) {
        uvc_perror(res, "set_gain");
      }

      res = uvc_set_brightness(devh, brightness);
      if (res < 0) {
        uvc_perror(res, "set_brightness");
      }

      res = uvc_set_white_balance_temperature_auto(devh, wb_auto);
      if (res < 0) {
        uvc_perror(res, "set_white_balance_temperature_auto");
      }

      if (wb) {
        res = uvc_set_white_balance_temperature(devh, wb);
        if (res < 0) {
          uvc_perror(res, "set_white_balance_temperature");
        }
      }

      /* Release our handle on the device */
      uvc_close(devh);
      puts("Device closed");
    }

    /* Release the device descriptor */
    uvc_unref_device(dev);
  }

  /* Close the UVC context. This closes and cleans up any existing device handles,
   * and it closes the libusb context if one was not provided. */
  uvc_exit(ctx);
  puts("UVC exited");

  return 0;
}
