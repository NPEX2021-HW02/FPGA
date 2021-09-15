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

//#define READ_FROM_JPEG
#include "cmdlineparser.h"
#include <iostream>
#include <cstring>
#include <time.h>
#ifdef READ_FROM_JPEG
//OpenCV includes
#include <opencv2/opencv.hpp>
#endif

// XRT includes
#include "experimental/xrt_bo.h"
#include "experimental/xrt_device.h"
#include "experimental/xrt_kernel.h"

#define IN_SIZE 1*224*224*3
#define WEIGHT_SIZE 4608992
#define BIAS_SIZE_M4 52352
#define BIAS_SIZE 13088
#define OUT_SIZE_M4 1000*4
#define OUT_SIZE 1000
#define LAYER_INFO_SIZE 256
#define LAYER_INFO_SIZE_M4 1024
#define INFERENCE_CNT 1

uint8_t g_in[IN_SIZE];
uint8_t g_weight[WEIGHT_SIZE];
int32_t g_bias[BIAS_SIZE];
int32_t g_layer_info[LAYER_INFO_SIZE];
int32_t g_out[OUT_SIZE];

void set_layer_info();//setting g_layer_info up from effinet.cpp
void fill_buffers();

int main(int argc, char** argv) {
    struct timeval start, end;
    long long sec, msec;
    char next_step;

    // Command Line Parser
    sda::utils::CmdLineParser parser;
    parser.addSwitch("--xclbin_file", "-x", "input binary file string", "");
    parser.addSwitch("--device_id", "-d", "device index", "0");
    parser.parse(argc, argv);

    // Read settings
    std::string binaryFile = parser.value("xclbin_file");
    int device_index = stoi(parser.value("device_id"));
    if (argc < 3) {
        parser.printHelp();
        return EXIT_FAILURE;
    }

    printf("Set layer info and init in/out/weight/bias buffers\n");
    set_layer_info();
    fill_buffers(); //load image and fill fake buffer to test

    gettimeofday(&start, NULL);
    auto device = xrt::device(device_index);
    gettimeofday(&end, NULL);
    sec = (end.tv_sec - start.tv_sec);
    msec = ((sec * 1000000L) + end.tv_usec) - (start.tv_usec);
    printf("Open the device: %d [time:%lldus]\n", device_index, msec);
    next_step = getchar();

    gettimeofday(&start, NULL);
    auto uuid = device.load_xclbin(binaryFile);
    gettimeofday(&end, NULL);
    sec = (end.tv_sec - start.tv_sec);
    msec = ((sec * 1000000L) + end.tv_usec) - (start.tv_usec);
    printf("Load the xclbin instance from %s file [time:%lldus]\n", binaryFile.c_str(), msec);
    next_step = getchar();

    gettimeofday(&start, NULL);
    auto krnl = xrt::kernel(device, uuid, "krnl_effnet_inf");
    gettimeofday(&end, NULL);
    sec = (end.tv_sec - start.tv_sec);
    msec = ((sec * 1000000L) + end.tv_usec) - (start.tv_usec);
    printf("Get kernel function instance [time:%lldus]\n", msec);
    next_step = getchar();
 
    gettimeofday(&start, NULL);
    auto bo_in = xrt::bo(device, IN_SIZE, krnl.group_id(0));
    auto bo_weight = xrt::bo(device, WEIGHT_SIZE, krnl.group_id(1));
    auto bo_bias = xrt::bo(device, BIAS_SIZE_M4, krnl.group_id(2));
    auto bo_info = xrt::bo(device, LAYER_INFO_SIZE_M4 , krnl.group_id(3));
    auto bo_out = xrt::bo(device, OUT_SIZE, krnl.group_id(4));
    gettimeofday(&end, NULL);
    sec = (end.tv_sec - start.tv_sec);
    msec = ((sec * 1000000L) + end.tv_usec) - (start.tv_usec);
    printf("Init Buffer in FPGA::Global Memory [time:%lldus]\n", msec);
    next_step = getchar();
 
    for (int i = 0; i < INFERENCE_CNT; i++) {
        gettimeofday(&start, NULL);
        bo_in.write(g_in);
        bo_weight.write(g_weight);
        bo_bias.write(g_bias);
	bo_info.write(g_layer_info);
        gettimeofday(&end, NULL);
        sec = (end.tv_sec - start.tv_sec);
        msec = ((sec * 1000000L) + end.tv_usec) - (start.tv_usec);
        printf("Write the input data to buffer objects [time:%lldus]\n", msec);
	next_step = getchar();

        gettimeofday(&start, NULL);
        bo_in.sync(XCL_BO_SYNC_BO_TO_DEVICE);
        bo_weight.sync(XCL_BO_SYNC_BO_TO_DEVICE);
        bo_bias.sync(XCL_BO_SYNC_BO_TO_DEVICE);
	bo_info.sync(XCL_BO_SYNC_BO_TO_DEVICE);
        gettimeofday(&end, NULL);
        sec = (end.tv_sec - start.tv_sec);
        msec = ((sec * 1000000L) + end.tv_usec) - (start.tv_usec);
        printf("Synchronize in/weight/bias buffers to device global memory [time:%lldus]\n", msec);
	next_step = getchar();
        
        gettimeofday(&start, NULL);
        auto run = krnl(bo_in, bo_weight, bo_bias, bo_info, bo_out, 1);
        //do_next_buffer_init;
        run.wait();
        gettimeofday(&end, NULL);
        sec = (end.tv_sec - start.tv_sec);
        msec = ((sec * 1000000L) + end.tv_usec) - (start.tv_usec);
        printf("Execution of the kernel [time:%lldus]\n", msec);
	next_step = getchar();

        gettimeofday(&start, NULL);
        bo_out.sync(XCL_BO_SYNC_BO_FROM_DEVICE);
        gettimeofday(&end, NULL);
        sec = (end.tv_sec - start.tv_sec);
        msec = ((sec * 1000000L) + end.tv_usec) - (start.tv_usec);
        printf("Get the output data from the device [time:%lldus]\n", msec);
	next_step = getchar();
 
        //check the results
	bo_out.read(g_out);
	for (int j = 0; j < 1000; j++) {
	    printf("%d,", g_out[j]);
	    if (j % 64 == 63) printf("\n");
	}
	printf("done[%c]\n", next_step);
    }

    return 0;
}


void fill_buffers() {
    //FILE *fp;
#ifdef READ_FROM_JPEG
    char img_src[128] = "/home/seongeun/test_xrt/Vitis_Accel_Examples/host_xrt/effinet_hdl_xrt/img/imagenet/ILSVRC2012_val_00009111.jpeg";

    cv::Mat img = cv::imread(img_src, cv::IMREAD_COLOR);
    cv::imshow("test image", img);
    printf("sample image loading!...\n");
    printf("If you close the pop-up image window, then do next step.");
    int r = cv::waitKey(0);
   
    std::memcpy(img.data, g_in, IN_SIZE);
#else
    //fp = fopen("bins/1_conv1_in1_1_224_224_3.bin", "rb");
    //for (int i = 0; !feof(fp); i++) g_in[i] = fgetc(fp);
    for (int i = 0; i < IN_SIZE; i++) g_in[i] = 1;
#endif
    //fclose(fp);
    //fp = fopen("bins/60_fc60_weights_1000_1280.bin", "rb");
    //for (int i = 0; !feof(fp); i++) g_weight[i] = fgetc(fp);
    for (int i = 0; i < WEIGHT_SIZE; i++) g_weight[i] = 0;

    //fclose(fp);
    //fp = fopen("bins/58_conv58_bias_1280.bin", "rb");
    //char *p_bias = (char *)g_bias;
    //for (int i = 0; !feof(fp); i++) p_bias[i] = fgetc(fp);
    //fclose(fp);
    for (int i = 0; i < BIAS_SIZE; i++) g_bias[i] = 0;
}
