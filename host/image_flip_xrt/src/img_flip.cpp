/**
* Copyright (C) 2019-2021 Xilinx, Inc
*
* Licensed under the Apache License, Version 2.0 (the "License"). You may
* not use this file except in compliance with the License. A copy of the
* License is located at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
* WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
* License for the specific language governing permissions and limitations
* under the License.
*/

#define IMG_WIDTH 1024
#define IMG_HEIGHT 768

/*
    Vector Addition Kernel Implementation
    Arguments:
        in      (input)     --> Input image
        out     (output)    --> Output image
        size_w  (input)     --> Size of image width
        size_h  (input)     --> Size of image height
*/

extern "C" {
void img_flip(const unsigned char* in, // Read-Only input
          unsigned char* out,          // Output Result
          int size_w,                  // Size in integer
          int size_h                   // Size in integer
          ) {
    // In Vitis flow, a kernel can be defined without interface pragma. For such
    // case, it follows default behavioral.
    // All pointer arguments (in, out) will be referred as Memory Mapped
    // pointers using single M_AXI interface.
    // All the scalar arguments (size) and return argument will be associated to
    // single s_axilite interface.
    unsigned char vin_buffer[IMG_HEIGHT][IMG_WIDTH];  // Local memory to store input image
    unsigned char vout_buffer[IMG_HEIGHT][IMG_WIDTH]; // Local Memory to store result

    int wc = 0;
    int hc = 0;
    int in_xy = 0;
    int out_xy = 0;
    int x_r;

    for (int y = 0; y < IMG_HEIGHT; y++) { //outer loop
    read:
	for (int x = 0; x < IMG_WIDTH; x++) {
#pragma HLS LOOP_TRIPCOUNT min = 1024 max = 1024
#pragma HLS PIPELINE II = 1
	    vin_buffer[y][x] = in[in_xy];
	    in_xy++;
	}

    // PIPELINE pragma reduces the initiation interval for loop by allowing the
    // concurrent executions of operations
    flip:
	for (int x = 0; x < IMG_WIDTH; x++) {
#pragma HLS LOOP_TRIPCOUNT min = 1024 max = 1024
#pragma HLS PIPELINE II = 1
            // perform image flip for x-axis
	    x_r = IMG_WIDTH - x - 1;
            vout_buffer[y][x] = vin_buffer[y][x_r];
        }

    // PIPELINE pragma reduces the initiation interval for loop by allowing the
    // concurrent executions of operations
    // burst write the result
    write:
	for (int x = 0; x < IMG_WIDTH; x++) {
#pragma HLS LOOP_TRIPCOUNT min = 1024 max = 1024
#pragma HLS PIPELINE II = 1
            out[out_xy] = vout_buffer[y][x];
	    out_xy++;
        }
    }
}
}
