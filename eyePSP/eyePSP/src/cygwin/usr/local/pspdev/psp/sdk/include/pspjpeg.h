#ifndef __PSPJPEG_H__
#define __PSPJPEG_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <psptypes.h>

/**
 * Inits the MJpeg library 
 *
 * @returns 0 on success, < 0 on error
*/
int sceJpegInitMJpeg(void);

/**
 * Finishes the MJpeg library
 *
 * @returns 0 on success, < 0 on error
*/
int sceJpegFinishMJpeg(void);

/**
 * Creates the decoder context.
 *
 * @param width - The width of the frame
 * @param height - The height of the frame
 *
 * @returns 0 on success, < 0 on error
*/
int sceJpegCreateMJpeg(int width, int height);

/**
 * Deletes the current decoder context.
 *
 * @returns 0 on success, < 0 on error
*/
int sceJpegDeleteMJpeg(void);

/**
 * Decodes a mjpeg frame.
 *
 * @param jpegbuf - the buffer with the mjpeg frame
 * @param size - size of the buffer pointed by jpegbuf
 * @param rgba - buffer where the decoded data in RGBA format will be stored.
 *				 It should have a size of width*height*4.
 * @param unk - Unknown, pass 0
 *
 * @returns (width*65536)+height on success, < 0 on error 
*/
int sceJpegDecodeMJpeg(u8 *jpegbuf,	SceSize size, void *rgba, u32 unk);

#ifdef __cplusplus
}
#endif

#endif
