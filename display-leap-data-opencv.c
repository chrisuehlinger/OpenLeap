/*
 ** Author: Elina Lijouvni
 ** License: GPL v3
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>

#include <cv.h>
#include <highgui.h>

typedef struct ctx_s ctx_t;
struct ctx_s
{
  int quit;
};

#if 0
static void
fprintf_data(FILE *fp, const char * title, unsigned char *data, size_t size)
{
  int i;

  printf("%s", title);
  for (i = 0; i < size; i++) {
    if ( ! (i % 16) )
      printf("\n");
    printf("%02x ", data[i]);
  }
  printf("\n");
}
#endif

#define VFRAME_WIDTH  640
#define VFRAME_HEIGHT 240
#define VFRAME_SIZE   (VFRAME_WIDTH * VFRAME_HEIGHT)

typedef struct frame_s frame_t;
struct frame_s
{
  IplImage* frame;
  uint32_t id;
  uint32_t data_len;
  uint32_t state;
};

#define UVC_STREAM_EOF                                  (1 << 1)

static void
process_video_frame(ctx_t *ctx, frame_t *frame)
{
  int key;

  cvShowImage("mainWin", frame->frame );
  key = cvWaitKey(1);
  if (key == 'q' || key == 0x1B)
    ctx->quit = 1;
}

static void
process_usb_frame(ctx_t *ctx, frame_t *frame, unsigned char *data, int size)
{
  int i;

  int bHeaderLen = data[0];
  int bmHeaderInfo = data[1];

  uint32_t dwPresentationTime = *( (uint32_t *) &data[2] );
  fprintf(stderr, "frame time: %u\n", dwPresentationTime);

  if (frame->id == 0)
    frame->id = dwPresentationTime;

  
  for (i = bHeaderLen; i < size ; i += 2) {
    if (frame->data_len >= VFRAME_SIZE)
      break ;

    CvScalar sLeft;
    sLeft.val[2] = data[i];
    sLeft.val[1] = data[i];
    sLeft.val[0] = data[i];
    int xLeft = frame->data_len % VFRAME_WIDTH;
    int y = frame->data_len / VFRAME_WIDTH;
    cvSet2D(frame->frame, 2 * y,     xLeft, sLeft);
    cvSet2D(frame->frame, 2 * y + 1, xLeft, sLeft);
    
    CvScalar sRight;
    sRight.val[2] = data[i+1];
    sRight.val[1] = data[i+1];
    sRight.val[0] = data[i+1];
    int xRight = VFRAME_WIDTH + frame->data_len % VFRAME_WIDTH;
    cvSet2D(frame->frame, 2 * y,     xRight, sRight);
    cvSet2D(frame->frame, 2 * y + 1, xRight, sRight);
    frame->data_len++;
  }

  if (bmHeaderInfo & UVC_STREAM_EOF) {
    fprintf(stderr, "End-of-Frame.  Got %i\n", frame->data_len);

    if (frame->data_len != VFRAME_SIZE) {
      fprintf(stderr, "wrong frame size got %i expected %i\n", frame->data_len, VFRAME_SIZE);
      frame->data_len = 0;
      frame->id = 0;
      return ;
    }

    process_video_frame(ctx, frame);
    frame->data_len = 0;
    frame->id = 0;
  }
  else {
    if (dwPresentationTime != frame->id && frame->id > 0) {
      fprintf(stderr,"mixed frame TS: dropping frame\n");
      frame->id = dwPresentationTime;
      /* frame->data_len = 0; */
      /* frame->id = 0; */
      /* return ; */
    }
  }
}


int
main(int argc, char *argv[])
{
  fprintf(stderr,"1");
  ctx_t ctx_data;
  fprintf(stderr,"2");
  memset(&ctx_data, 0, sizeof (ctx_data));
  fprintf(stderr,"3");
  ctx_t *ctx = &ctx_data;

  cvNamedWindow("mainWin", 0);
  fprintf(stderr, "4");
  cvResizeWindow("mainWin", VFRAME_WIDTH*2, VFRAME_HEIGHT * 2);
  fprintf(stderr, "5");

  frame_t frame;
  memset(&frame, 0, sizeof (frame));
  frame.frame = cvCreateImage( cvSize(2*VFRAME_WIDTH, 2 * VFRAME_HEIGHT), IPL_DEPTH_8U, 3);
  fprintf(stderr, "6");

  for (int i = 0; ; i++) {
    
    fprintf(stderr, "\nframe: %d\n", i);
    unsigned char data[16384];
    int usb_frame_size;
    //fprintf(stderr, "Post-declaration\n");

    if ( feof(stdin) || ctx->quit )
      break ;

    //fprintf(stderr, "Post-guard\n");

    fread(&usb_frame_size, sizeof (usb_frame_size), 1, stdin);
    fprintf(stderr, "Post-First Read: %d\n", usb_frame_size);
    fread(data, usb_frame_size, 1, stdin);
    //fprintf(stderr, "Post-Second Read\n");
    process_usb_frame(ctx, &frame, data, usb_frame_size);
    //fprintf(stderr, "Post-Process\n");

    usleep(1000);
  }
  fprintf(stderr, "\npre-release");

  cvReleaseImage(&frame.frame);
  fprintf(stderr, "\nend");

  return (0);
}
