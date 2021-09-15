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

#define IN_SIZE 1*224*224*3
#define WEIGHT_SIZE 4608992
#define BIAS_SIZE_M4 52352
#define BIAS_SIZE 13088
#define OUT_SIZE_M4 1000*4
#define OUT_SIZE 1000
#define LAYER_INFO_SIZE 256
#define LAYER_INFO_SIZE_M4 1024
#define INFERENCE_CNT 1


extern "C" {
void krnl_effnet_inf(const unsigned char* in, // in
        const unsigned char* weight, // weight
        const int* bias, // bias
        const int* info, // layer info
        int *out, // out
        const int init_cnt
    ) {
    int _in[IN_SIZE];
    unsigned char _weight[WEIGHT_SIZE];
    int _bias[BIAS_SIZE];
    int _layer_out[OUT_SIZE];
    int _info[LAYER_INFO_SIZE];

read1:
    int max_in = IN_SIZE;
    if (init_cnt) max_in = init_cnt;
    for (int i = 0; i < max_in; i++) {
        _in[i] = in[i];
    }

read2:
    int max_weight = WEIGHT_SIZE;
    if (init_cnt) max_weight = init_cnt;
    for (int i = 0; i < max_weight; i++) {
        _weight[i] = weight[i];
    }

read3:
    int max_bias = BIAS_SIZE;
    if (init_cnt) max_bias = init_cnt;
    for (int i = 0; i < max_bias; i++) {
        _bias[i] = bias[i];
    }

read4:
    for (int i = 0; i < LAYER_INFO_SIZE; i++) {
        _info[i] = info[i];
    }

operations:
    int layer_cnt = _info[0];
    for (int i = 0; i < layer_cnt; i ++) {
        //in case of MAC operation type
        if (_info[i * 3 + 1] == 0) {
            int op_cnt = _info[i * 3 + 2];
            int op_filter_size = _info[i * 3 + 3];
            for(int j = 0; j < op_cnt; j += op_filter_size) {
                int tmp_out = 0;
                for (int k = 0; k < op_filter_size; k++) {
                    tmp_out += _weight[k] * _in[k]; //MAC
                }
                _layer_out[i] = tmp_out;
            }
        }

        //in case of ELTWISE operation type
        else if (_info[i * 3 + 1] == 1) {
            int op_cnt = _info[i * 3 + 2];
            for(int j = 0; j < op_cnt; j++) {
                _layer_out[i] = _weight[j] + _in[j]; //ADD
            }
        }

        //for debug
        else {
            _layer_out[0] = _info[0];
            _layer_out[1] = _info[i * 3 + 2];
            _layer_out[2] = _info[i * 3 + 3];
        }
    }
write:
    for (int i = 0; i < OUT_SIZE; i++) {
        out[i] = _layer_out[i]; //skip, last sort(softmax layer)
    }
}
}
