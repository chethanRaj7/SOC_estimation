#include <stdio.h>
#include <stdlib.h>
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
        //printf("voltage: %.15lf", value);
        break;
    case 2:
        current[index] = value;
        //printf("current: %.15lf", value);
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
    //better not implement, read directly.
    double v = volt[index];
    double i = current[index];
    return v, i;                                    //cannot return multiple variables like this, only returns I.
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


void dot(double mat1[3][3], double mat2[3][3], double res[3][3]){
    //Multiply 2 3x3 matrices mutually.
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            res[i][j] = 0; // Initialize the result matrix cell
            for (int k = 0; k < 3; k++) {
                res[i][j] += mat1[i][k] * mat2[k][j];
            }
        }
    }
}

void transpose(double mat[3][3], double transposed[3][3]) { 
    //transpose an 3x3 matrix.
    for (int i = 0; i < 3; i++) { 
        for (int j = 0; j < 3; j++) { 
            transposed[j][i] = mat[i][j]; 
            }
    }
}

void transpose3x1(double mat[1][3], double transposed[3][1]) { 
    //transpose an 1x3 and 3x1
    for (int i = 0; i < 1; i++) { 
        for (int j = 0; j < 3; j++) { 
            transposed[j][i] = mat[i][j]; 
            } 
    } 
}


void dot1x3(double mat1[1][3], double mat2[3][3], double res[1][3]) { 
    for (int i = 0; i < 3; i++) { 
        res[0][i] = 0; // Initialize the result matrix cell 
        for (int j = 0; j < 3; j++) { 
            res[0][i] += mat1[0][j] * mat2[j][i]; 
        } 
    }
}

void dot_product(double mat1[3][1], double mat2[3], double res[3][3]) { 
    for (int i = 0; i < 3; i++) { 
        for (int j = 0; j < 3; j++) { 
            res[i][j] = mat1[i][0] * mat2[j]; 
            } 
    }
}

double dot1(double mat1[1][3], double mat2[3][1]) { 
    double result = 0; 
    for (int i = 0; i < 3; i++) { 
        result += mat1[0][i] * mat2[i][0]; 
    } 
    return result; 
}


void add_scalar(double mat[3][3], double scalar) { 
    for (int i = 0; i < 3; i++) { 
        for (int j = 0; j < 3; j++) { 
            mat[i][j] += scalar; 
        } 
    } 
}

void copy_matrix(double src[3][3], double dest[3][3]) { 
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) { 
            dest[i][j] = src[i][j]; 
        }
    }
}

void main()
{
    float xHat[] = {1.0, 0.0, 0.0};
    double sigmaX[3][3] = {{1e-5, 0.0, 0.0},
                        {0.0, 1e-5, 0.0},
                        {0.0, 0.0, 1e-5}
                        };

    float socVar = 1e1;
    float iRC1Var = 1e1;
    float iRC2Var = 1e1;

    double sigmaW[3][3] = {{socVar, 0.0, 0.0},
                        {0.0, iRC1Var, 0.0},
                        {0.0, 0.0, iRC2Var}
                        };

    double sigmaV = 1e-3;
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
    float eta = 1.0;
    float capacityOCV = disCapacity[0];

    float RC1 = exp(-dt / (r1 * c1));
    float RC2 = exp(-dt / (r2 * c2));
    float V, I;

    double Ahat[3][3] = { {1.0, 0.0, 0.0},
                          {0.0, RC1, 0.0},
                          {0.0, 0.0, RC2} };
    
    //double Bhat[3] = {(-dt * eta / (3600 * capacityOCV)), (1-RC1), (1-RC2)};
    double Bhat[1][3] = {{(-dt * eta / (3600 * capacityOCV)), (1-RC1), (1-RC2)}};
    double Chat[3] = {1.0, -r1, -r2};                           //replace the 1.0 during iteration.
    double Dhat[1] = {1.0};


    double dZ = calculateMeanDifference(SOC, num_lines);        //constant, doesnt change.
    printf("dZ: %.15lf\n", dZ);
    dZ = -0.0009115753413114999;

    /*
    //dZ = -0.0009115753413114999;
    for(int k =0;k< 10;k++){
        float df = gendOCVSOC(k, SOC, OCV, num_lines, dZ );
        //printf("value: %f\n", df);                            //yields correct output when dZ is correct. Rest working fine.
    } */

    //start simulation test.
    double dOCVSOC;
    double temp_ocv;
    sim_num_lines = 4;

    for(int iter = 0; iter < sim_num_lines; iter++){
        //V, I = gen_measure(iter, volt, current);            //temporary, to be written.
        V = volt[iter];
        I = current[iter] * -1;
        printf("v: %.10lf, I: %.10lf, \n", V, I);

        dOCVSOC = gendOCVSOC(xHat[0], SOC, OCV, num_lines, dZ);
        printf("dOCVSOC %.10lf\n", dOCVSOC);
        //OCV = voltOCV[argmin(abs(SOCOCV - xHat[0]))]
        temp_ocv = volt[argmin(SOC, xHat[0], num_lines)];         //sharp drop's in OCV, check the argmin function.
        printf("OCV %.10lf\n", temp_ocv);

        xHat[0] = xHat[0] - (dt * I * eta / (3600 * capacityOCV));
        xHat[1] = xHat[1] * RC1 + (1 - RC1) * I;
        xHat[2] = xHat[2] * RC2 + (1 - RC2) * I;

        // for( int i = 0; i<3; i++){
        //     printf("%.10lf ", xHat[i]);
        // }
        
        //sigmaX = np.dot( np.dot(Ahat, sigmaX), Ahat.T) + np.dot( np.dot(Bhat, sigmaW), Bhat.T)
        double A_x[3][3];   dot(Ahat, sigmaX, A_x);

        double A_t[3][3];   transpose(Ahat, A_t);                           //works perfectly fine.

        double temp_mat1[3][3];     dot(A_x, A_t, temp_mat1);
        
        double B_w[1][3];       dot1x3(Bhat, sigmaW, B_w);                         //dot 1x3 and 3x3    res = 1x3

        double B_t[3][1];       transpose3x1(Bhat, B_t);                           //transpose 1x3 bhat to 3x1
        double scalar = dot1(B_w, B_t);
        add_scalar(temp_mat1, scalar);

        copy_matrix(temp_mat1, sigmaX);                                            //replace this fn later by passing pointer, instead of copy.

        double yHat = ( temp_ocv - r0 * I - r1 * xHat[1] - r2 * xHat[2] );
        printf("yhat: %.10lf\n", yHat);

        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                printf("%lf ", sigmaX[i][j]);
            }
            printf("\n");
        }
        printf("\n");

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