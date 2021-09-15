
#include <stdio.h>
#include <string.h>

#define LAYER_INFO_SIZE 256

extern int g_layer_info[LAYER_INFO_SIZE];
void set_layer_info()
{
    FILE *fp = fopen("effinet_ops.txt", "r");

    char layer_name[16];
    int op_cnt;
    int op_filter;
    int i;
    for (i = 0; !feof(fp); i++) {
	fscanf(fp, "%s\n", layer_name);
	fscanf(fp, "%d,%d\n", &op_cnt, &op_filter);
	if (!strncmp("conv", layer_name, 4) ||
	    !strncmp("dwconv", layer_name, 6) ||
	    !strncmp("FC", layer_name, 2)) { //MAC_operation
	    g_layer_info[i * 3 + 1] = 0;
	    g_layer_info[i * 3 + 2] = op_cnt;
	    g_layer_info[i * 3 + 3] = op_filter;
	} else if (!strncmp("add", layer_name, 3)) { //ADD_operation
	    g_layer_info[i * 3 + 1] = 1;
	    g_layer_info[i * 3 + 2] = op_cnt;
	    g_layer_info[i * 3 + 3] = 0;
	} else {
            //global avg pool or softmax. PASS
	}
    }
    fclose(fp);
    g_layer_info[0] = i;

    //for simple test
    /*g_layer_info[0] = 1;
    g_layer_info[1] = 2;
    g_layer_info[2] = 1000;
    g_layer_info[3] = 2;*/
    printf("layer info[%d]:\n", g_layer_info[0]);
    for(int x = 0; x < (256 / 3); x++) {
	printf("%d %d %d\n", g_layer_info[x * 3 + 1], g_layer_info[x * 3 + 2], g_layer_info[x * 3 + 3]);
    }
}
