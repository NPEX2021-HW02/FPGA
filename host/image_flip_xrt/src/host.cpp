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

#include "cmdlineparser.h"
#include <iostream>
#include <cstring>

// XRT includes
#include "experimental/xrt_bo.h"
#include "experimental/xrt_device.h"
#include "experimental/xrt_kernel.h"

#define IMG_WIDTH 1024
#define IMG_HEIGHT 768

int main(int argc, char** argv) {
    // Command Line Parser
    sda::utils::CmdLineParser parser;

    // Switches
    //**************//"<Full Arg>",  "<Short Arg>", "<Description>", "<Default>"
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

    struct timeval start, end;
    long long sec, msec;
    char next_step;

    // Host code starts here
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

    //get kernel inst.
    gettimeofday(&start, NULL);
    auto krnl = xrt::kernel(device, uuid, "img_flip");
    gettimeofday(&end, NULL);
    sec = (end.tv_sec - start.tv_sec);
    msec = ((sec * 1000000L) + end.tv_usec) - (start.tv_usec);
    printf("Get kernel function instance [time:%lldus]\n", msec);
    next_step = getchar();

    gettimeofday(&start, NULL);
    auto bo_in = xrt::bo(device, sizeof(int8_t) * IMG_WIDTH * IMG_HEIGHT,  krnl.group_id(0));
    auto bo_out = xrt::bo(device, sizeof(int8_t) * IMG_WIDTH * IMG_HEIGHT, krnl.group_id(1));
    gettimeofday(&end, NULL);
    sec = (end.tv_sec - start.tv_sec);
    msec = ((sec * 1000000L) + end.tv_usec) - (start.tv_usec);
    printf("Init Buffer object in FPGA::Global Memory [time:%lldus]\n", msec);
    next_step = getchar();


    // Map the contents of the buffer object into host memory
    auto bo_in_map = bo_in.map<unsigned char*>();
    auto bo_out_map = bo_out.map<unsigned char*>();
    std::fill(bo_out_map, bo_out_map + IMG_WIDTH * IMG_HEIGHT, 0); //init. output buffer only.

    // Create the test data
    int xy = 0;
    unsigned char bufReference[IMG_HEIGHT][IMG_WIDTH];
    for (int y = 0; y < IMG_HEIGHT; y++) {
        for (int x = IMG_WIDTH - 1; x >= 0; x--) {
            bo_in_map[xy] = rand() % 256;
            bufReference[y][x] = bo_in_map[xy++];//Golden
	}
    }

    // Synchronize buffer content with device side for input
    gettimeofday(&start, NULL);
    bo_in.sync(XCL_BO_SYNC_BO_TO_DEVICE);
        gettimeofday(&end, NULL);
        sec = (end.tv_sec - start.tv_sec);
        msec = ((sec * 1000000L) + end.tv_usec) - (start.tv_usec);
        printf("Synchronize in/weight/bias buffers to device global memory [time:%lldus]\n", msec);
	next_step = getchar();
 
    gettimeofday(&start, NULL);
    auto run = krnl(bo_in, bo_out, IMG_WIDTH, IMG_HEIGHT);
    run.wait();
        gettimeofday(&end, NULL);
        sec = (end.tv_sec - start.tv_sec);
        msec = ((sec * 1000000L) + end.tv_usec) - (start.tv_usec);
        printf("Execution of the kernel [time:%lldus]\n", msec);
	next_step = getchar();


    // Get the output;
    gettimeofday(&start, NULL);
    bo_out.sync(XCL_BO_SYNC_BO_FROM_DEVICE);
        gettimeofday(&end, NULL);
        sec = (end.tv_sec - start.tv_sec);
        msec = ((sec * 1000000L) + end.tv_usec) - (start.tv_usec);
        printf("Get the output data from the device [time:%lldus]\n", msec);
	next_step = getchar();



    // Validate our results
    if (std::memcmp(bo_out_map, bufReference, IMG_WIDTH * IMG_HEIGHT))
        throw std::runtime_error("Value read back does not match reference");
    printf("%c done\n", next_step);
    std::cout << "TEST PASSED\n";
    return 0;
}
