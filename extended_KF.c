#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>


void choose_model_variable(int variable, double value, int index, double *time ,double *OCV, double *SOC, double *disCapacity ){
    switch (variable) {
    case 1:
        time[index] = value;
        break;
    case 2:
        OCV[index] = value;
        break;
    case 3:
        SOC[index] = value;
        break;
    case 4:
        disCapacity[index] = value;
        break;
    default:
        printf("default value");
}
}

void choose_sim_variable(int variable, double value, int index, double *volt ,double *current, double *discap, double *time){
    switch (variable) {
    case 1:
        volt[index] = value;
        break;
    case 2:
        current[index] = value;
        break;
    case 3:
        //temp[index] = value;
        break;
    case 4:
        //cur_load[index] = value;
        break;
    case 5:
        //vol_load[index] = value;
        break;
    case 6:
        time[index] = value;
        break;
    case 7: 
        discap[index] = value;
        break;
    default:
        printf("default value");
}
}

int count_lines(FILE* file)
{
    char buf[40];
    int counter = 0;
    for(;;)
    {
        size_t res = fread(buf, 1, 40, file);
        if (ferror(file))
            return -1;

        int i;
        for(i = 0; i < res; i++)
            if (buf[i] == '\n')
                counter++;

        if (feof(file))
            break;
    }
    return counter;
}

void importmodel(FILE *fp, int num_lines, double *time,double *OCV, double *SOC, double *disCapacity )
{
    char chunk[124];
    char letter[20];
    int temp_index = 0;
    fseek(fp, 0, SEEK_SET);                                         //reset the pointer to start of file.

    while(temp_index< num_lines){
        fgets(chunk, sizeof(chunk), fp) != NULL;
        
        int j = 1;
        int temp_i = 0;
        for(int i = 0; i< sizeof(chunk); i++){

            if (chunk[i] == '\0'){
                char *endptr;                                        // Pointer to track invalid characters
                double temp_value = strtod(letter, &endptr);          // Convert string to float
                choose_model_variable(j, temp_value, temp_index, time, OCV, SOC, disCapacity);           //populate the values to index.
                temp_i = 0;                                           //reset the index after update

                break;
            }
            else if (chunk[i] != ','){
                letter[temp_i] = chunk[i];
                temp_i++;
            }

            else if(chunk[i] == ','){
                char *endptr;                                        // Pointer to track invalid characters
                double temp_value = strtod(letter, &endptr);          // Convert string to float
                choose_model_variable(j, temp_value, temp_index, time, OCV, SOC, disCapacity);           //populate the values to index.

                j++;
                temp_i = 0;                                           //reset the index after update
                continue;
            }
        }
        if (temp_index == 10){
            break;
        }
        temp_index++;
    }
    fclose(fp);
}

double gen_measure(int index, double *volt, double *current){
    double v = volt[index];
    double i = current[index];
    return v, i;
}

double calculateMeanDifference(double *SOC, int size) {
    double sum = 0.0;

    for (int i = 0; i < size - 1; i++) {
        //sum += fabs(SOC[i + 1] - SOC[i]); // Difference between consecutive elements
        sum += (SOC[i + 1] - SOC[i]);
    }
    return (sum / (size - 1));
}

int argmin(double *array, double scalar, int size){
    //int size = sizeof(array)/sizeof(array[0]);

    double *temp = (double *)malloc(size * sizeof(double));

    for(int i =0; i< size; i++){
        temp[i] = fabs(array[i] - scalar);            //compute the differece and store in temporary array
    }

    double smallest = temp[0];                 // Assume the first element is the smallest
    int index = 0;
                              
    for (int i = 1; i < size; i++) {           // Iterate through the array to find the smallest value
        if (temp[i] < smallest) {
            smallest = temp[i];
            index = i;
        }
    }
    free(temp);
    return index;
}

double gendOCVSOC(float soc, double *SOC, double *OCV,int length, double dz){
    //works great
    int index = argmin(SOC, soc, length); //works good too
    double dOCVSOC_fwd, dOCVSOC_rvs;

    if( index != (length-1)){
        dOCVSOC_fwd = fabs(OCV[index] - OCV[index + 1]) / dz ;
    }
    if( index != 0){
        dOCVSOC_rvs = fabs(OCV[index-1] - OCV[index]) / dz ;
    }
    if(index == (length -1)){
        dOCVSOC_fwd = dOCVSOC_rvs;
    }
    if(index == 0){
        dOCVSOC_rvs = dOCVSOC_fwd;
    }
    double dOCVSOC = -(dOCVSOC_fwd + dOCVSOC_rvs) / 2;
    return dOCVSOC;
}

void importsim(FILE *fp, double *volt, double* current, double *discap, double *time ,int num_lines){
    char chunk[124];
    char letter[20];
    int temp_index = 0;
    fseek(fp, 0, SEEK_SET);                                         //reset the pointer to start of file.

    while(temp_index< num_lines){
        fgets(chunk, sizeof(chunk), fp) != NULL;
        
        int j = 1;
        int temp_i = 0;
        for(int i = 0; i< sizeof(chunk); i++){

            if (chunk[i] == '\0'){
                char *endptr;                                        // Pointer to track invalid characters
                double temp_value = strtod(letter, &endptr);          // Convert string to float
                choose_sim_variable(j, temp_value, temp_index, volt, current, discap, time);           //populate the values to index.
                temp_i = 0;                                           //reset the index after update

                break;
            }
            else if (chunk[i] != ','){
                letter[temp_i] = chunk[i];
                temp_i++;
            }

            else if(chunk[i] == ','){
                char *endptr;                                        // Pointer to track invalid characters
                double temp_value = strtod(letter, &endptr);          // Convert string to float
                choose_sim_variable(j, temp_value, temp_index, volt, current, discap, time);           //populate the values to index.

                j++;
                temp_i = 0;                                           //reset the index after update
                continue;
            }
        }
        temp_index++;
    }
    fclose(fp);
}

void main()
{
    float xHat[] = {1.0, 0.0, 0.0};
    float sigmaX[3][3] = {{1e-5, 0.0, 0.0},
                        {0.0, 1e-5, 0.0},
                        {0.0, 0.0, 1e-5}
                        };

    float socVar = 1e1;
    float iRC1Var = 1e1;
    float iRC2Var = 1e1;

    float sigmaW[3][3] = {{socVar, 0.0, 0.0},
                        {0.0, iRC1Var, 0.0},
                        {0.0, 0.0, iRC2Var}
                        };

    float sigmaV = 1e-3;
    float socStore[] = {};

    //Import necessary datasets and initialize constant variables.

    FILE *model_dat = fopen("OCV--25degC--549_C20DisCh.csv", "r");
    if(model_dat == NULL) {
        perror("Unable to open file!");
        exit(1);
    }

    int num_lines = count_lines(model_dat);

    printf("Number of lines: %d", num_lines);
    
    double *OCV = (double *)malloc(num_lines * sizeof(double));                    //allocate heap for storage, instead of stack.
    double *SOC = (double *)malloc(num_lines * sizeof(double));
    double *disCapacity = (double *)malloc(num_lines * sizeof(double));
    double *time_ocv = (double *)malloc(num_lines * sizeof(double));
    importmodel(model_dat, num_lines, time_ocv, OCV, SOC, disCapacity);

    FILE *sim_dat = fopen("00001.csv", "r");
    if(sim_dat == NULL) {
        perror("Unable to open sim file!");
        exit(1);
    }
    int sim_num_lines = count_lines(sim_dat);

    double *volt = (double *)malloc(sim_num_lines * sizeof(double));                   //sample, reinitialize after loading.
    double *current = (double *)malloc(sim_num_lines * sizeof(double));
    double *discap = (double *)malloc(sim_num_lines * sizeof(double));
    double *time_sim = (double *)malloc(sim_num_lines * sizeof(double));

    printf("Number of lines in sim: %d", sim_num_lines);
    importsim(sim_dat, volt, current, discap, time_sim ,sim_num_lines);


    double r0 = 0.009512549;
    double r1 = 0.007916353;
    double r2 = 26.50709648;
    double c1 = 3200.923755;
    double c2 = 129917.9637;

    float dt = 13.161842;

    float RC1 = exp(-dt / (r1 * c1));
    float RC2 = exp(-dt / (r2 * c2));
    float V, I;

    double dZ = calculateMeanDifference(SOC, num_lines);        //constant, doesnt change.
    printf("dZ: %.15lf\n", dZ);
    //dZ = -0.0009115753413114999;

    //gen_measure()                                             //load the df befor this.
    for(int k =0;k< 10;k++){
        float df = gendOCVSOC(k, SOC,OCV,num_lines,dZ );
        printf("value: %f\n", df);
    }

    for(int iter = 0; iter < 10; iter++){
        V, I = gen_measure(iter, volt, current);            //temporary, to be written.
        printf("v: %.10lf, I: %.10lf, \n", V, I);
        //compute the dOCVSOC.
    }

    for(int k = 0; k< 1; k++){
        //printf("index: %d",argmin(SOC, ));
        printf("OCV: %f\n", OCV[k]);
        printf("SOC: %lf\n", SOC[k]);
        printf("disCap: %f\n", disCapacity[k]);
        printf("time: %f\n", time_ocv[k]);
        printf("---------\n");
    }
    free(OCV);
    free(SOC);
    free(time_ocv);
    free(disCapacity);

    free(volt);
    free(discap);
    free(current);
    free(time_sim);
}