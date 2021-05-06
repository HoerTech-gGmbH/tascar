/*
 * This file is part of the TASCAR software, see <http://tascar.org/>
 *
 * Copyright (c) 2019 Giso Grimm
 */
/*
 * TASCAR is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation, version 3 of the License.
 *
 * TASCAR is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHATABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License, version 3 for more details.
 *
 * You should have received a copy of the GNU General Public License,
 * Version 3 along with TASCAR. If not, see <http://www.gnu.org/licenses/>.
 */

/*
The MIT License

Copyright (c) 2009 Tulthix, uglyDwarf

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/


#ifndef LINUX_TRACK__H
#define LINUX_TRACK__H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif


typedef enum {
  LINUXTRACK_OK = 0,
  INITIALIZING = 1,
  RUNNING = 2,
  PAUSED = 3,
  STOPPED = 4,
  //Error codes
  err_NOT_INITIALIZED = -1,
  err_SYMBOL_LOOKUP = -2,
  err_NO_CONFIG = -3,
  err_NOT_FOUND = -4,
  err_PROCESSING_FRAME = -5
}linuxtrack_state_type;

linuxtrack_state_type linuxtrack_init(const char *cust_section);
linuxtrack_state_type linuxtrack_shutdown(void);
linuxtrack_state_type linuxtrack_suspend(void);
linuxtrack_state_type linuxtrack_wakeup(void);
linuxtrack_state_type linuxtrack_recenter(void);
const char *linuxtrack_explain(linuxtrack_state_type err);
linuxtrack_state_type linuxtrack_get_tracking_state(void);

int linuxtrack_get_pose(float *heading,
                         float *pitch,
                         float *roll,
                         float *tx,
                         float *ty,
                         float *tz,
                         uint32_t *counter);


typedef struct{
  float pitch;
  float yaw;
  float roll;
  float tx;
  float ty;
  float tz;
  uint32_t counter;
  uint32_t resolution_x;
  uint32_t resolution_y;
  float raw_pitch;
  float raw_yaw;
  float raw_roll;
  float raw_tx;
  float raw_ty;
  float raw_tz;
  uint8_t status;
} linuxtrack_pose_t;

int linuxtrack_get_pose_full(linuxtrack_pose_t *pose, float blobs[], int num_blobs, int *blobs_read);

int linuxtrack_get_abs_pose(float *heading,
                            float *pitch,
                            float *roll,
                            float *tx,
                            float *ty,
                            float *tz,
                            uint32_t *counter);

linuxtrack_state_type linuxtrack_request_frames(void);
int linuxtrack_get_frame(int *req_width, int *req_height, size_t buf_size, uint8_t *buffer);
linuxtrack_state_type linuxtrack_notification_on(void);
int linuxtrack_get_notify_pipe(void);
int linuxtrack_wait(int timeout);

#ifdef __cplusplus
}
#endif

#endif

