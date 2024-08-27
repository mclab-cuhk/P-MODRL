#include "PMODRL.h"



struct Row *readFromTrace(FILE *file, int *realNumRow) {
    int currentCapacity=500;
    struct Row *rows = malloc(currentCapacity * sizeof(struct Row));
    int numRowCount = 0;
    int bufferLength = 255;
    char buffer[bufferLength];
    while (fgets(buffer, bufferLength, file)) {
        unsigned long size = strlen(buffer);
        char *end = buffer + size - 1;
        if (*end == '\n') {
            *end = '\0';
        } else {
            fprintf(stderr, "Input file error\n");
            exit(-1);
        }
        char *part = strtok(buffer, " ");
        int count = 0;
        while (part != NULL) {
            if (count == 0) {
                double timestamp;
                sscanf(part, "%lf", &timestamp);
                rows[numRowCount].timestamp = timestamp;
                count++;
            } else if (count == 1) {
                int byte;
                sscanf(part, "%d", &byte);
                rows[numRowCount].bytesReceived = byte;
                count++;
            } else {
                int accBytes;
                sscanf(part, "%d", &accBytes);
                rows[numRowCount].accByteReceived = accBytes;
                count++;
            }
            part = strtok(NULL, " ");
        }
        if (count != 3) {
            fprintf(stderr, "Input file error\n");
            exit(-1);
        }
        numRowCount++;
        if(numRowCount==currentCapacity){
            currentCapacity*=2;
            rows = realloc(rows, currentCapacity * sizeof(struct Row));
        }
    }
    *realNumRow = numRowCount;
    return rows;
}

char **getFolders(int *numFolder, char* parentFolder) {
    char **folders = malloc(0 * sizeof(char *));
    struct dirent *dir;
    char* fold=parentFolder;
    DIR *d = opendir(fold);
    if (d) {
        while ((dir = readdir(d)) != NULL) {
            int len = strlen(dir->d_name);
            char *folder = malloc((len + 1) * sizeof(char));
            strncpy(folder, dir->d_name, len + 1);
            (*numFolder)++;
            folders = realloc(folders, (*numFolder) * sizeof(char *));
            folders[(*numFolder) - 1] = folder;
        }
    }
    return folders;
}

int startWith(char *pre, char *str) {
    return strncmp(pre, str, strlen(pre)) == 0;
}

char **filterFolders(char **folders, int *numFolder, char *prefix) {
    int validFolder = 0;
    for (int i = 0; i < (*numFolder); i++) {
        int bool = startWith(prefix, folders[i]);
        if (bool) {
            validFolder++;
            int len = strlen(folders[i]);
            folders[validFolder - 1] = realloc(folders[validFolder - 1], (len + 1) * sizeof(char));
            strncpy(folders[validFolder - 1], folders[i], len + 1);
        }
    }
    for (int i = validFolder; i < (*numFolder); i++) {
        free(folders[i]);
    }
    folders = realloc(folders, validFolder * sizeof(char *));
    *numFolder = validFolder;
    return folders;
}

double rateForBucket(struct Row* rows, int index, struct BucketInfor* bucketInforArr, int bucketIndex){
    double B=bucketInforArr[bucketIndex].bucketSize;
    double R;
    if(bucketInforArr[bucketIndex].index==0){
        double slope = -FLT_MAX;
        for (int i = 1; i <= index; i++) {
            if(rows[i].timestamp!=0){
                double newSlope = (rows[i].accByteReceived - B) / rows[i].timestamp;
                if (newSlope > slope) {
                    slope = newSlope;
                }
            }
        }
        if (slope <= 0) {
            R=0;
        } else {
            R=slope;
        }
    }
    else{
        R=bucketInforArr[bucketIndex].tokenGenerateRate;
        for(int i=bucketInforArr[bucketIndex].index+1;i<=index;i++){
            double newSlope = (rows[i].accByteReceived - B) / rows[i].timestamp;
            if (newSlope > R) {
                R=newSlope;
            }
        }
    }
    return R;
}

double areaDifferenceIntersection(int calculatedIndex, int index, double time, struct BucketInfor* bucketInforArr){
    double calculatedBucket=bucketInforArr[calculatedIndex].bucketSize;
    double calculatedRate=bucketInforArr[calculatedIndex].tokenGenerateRate;
    double calculatedAreaDifference=bucketInforArr[calculatedIndex].areaDifference;
    double B=bucketInforArr[index].bucketSize;
    double R=bucketInforArr[index].tokenGenerateRate;
    if(calculatedRate==0 && bucketInforArr[index].tokenGenerateRate==0){
        double area=fabs(B - calculatedBucket)*time;
        if(B>calculatedBucket){
            return calculatedAreaDifference+area;
        }
        else{
            return calculatedAreaDifference-area;
        }
   
    }
    else{
        double intersectionT=(B - calculatedBucket)/(calculatedRate - R);
        if(intersectionT>time){
            double area=(calculatedBucket - B+calculatedBucket+calculatedRate*time - B -R*time)*time/2;
            return calculatedAreaDifference - area;
        }
        double leftTriangle=fabs(B - calculatedBucket)*intersectionT/2;
        double rightTriangle=fabs(calculatedBucket+calculatedRate*time-B-R*time)*(time - intersectionT)/2;
        if(B<calculatedBucket){
            return calculatedAreaDifference-leftTriangle+rightTriangle;
        }
        else{
            return calculatedAreaDifference+leftTriangle-rightTriangle;
        }   
    }
}

double areaDifferenceFromScratch(struct Row* rows, int index, struct BucketInfor* bucketInforArr, int bucketIndex){
    double sumDifference=0;
    double B=bucketInforArr[bucketIndex].bucketSize;
    double R=bucketInforArr[bucketIndex].tokenGenerateRate;
    for(int i=1;i<=index;i++){
        struct Row currentRow = rows[i];
        struct Row previousRow = rows[i - 1];
        double difference = (currentRow.timestamp - previousRow.timestamp) *
                              (B+R * currentRow.timestamp - previousRow.accByteReceived + B+R*previousRow.timestamp- previousRow.accByteReceived) / 2;
        sumDifference += difference;
    }
    return sumDifference;
}

double areaDifferencePacket(struct Row* rows, int index, struct BucketInfor* bucketInforArr, int bucketIndex, double previousR){
    double B=bucketInforArr[bucketIndex].bucketSize;
    double R=bucketInforArr[bucketIndex].tokenGenerateRate;
    int calculatedIndex=bucketInforArr[bucketIndex].index;
    double sumDifference;
    if(previousR<R){
        struct Row previousCalculatedRow = rows[calculatedIndex];
        double lefTriangleArea = previousCalculatedRow.timestamp * (previousCalculatedRow.timestamp * (R - previousR)) / 2;
        double rightTrapezoidArea=0;
        for(int i=calculatedIndex+1;i<=index;i++){
            rightTrapezoidArea+=(rows[i].timestamp - rows[i-1].timestamp) *
                                  (B+R * rows[i].timestamp - rows[i-1].accByteReceived + B+R*rows[i-1].timestamp- rows[i-1].accByteReceived) / 2;
        }
        sumDifference = bucketInforArr[bucketIndex].areaDifference + lefTriangleArea + rightTrapezoidArea;
    }
    else {
        double sumTrapezoid=0;
        for(int i=calculatedIndex+1;i<=index;i++){
            sumTrapezoid+=(rows[i].timestamp - rows[i-1].timestamp) *
                                      (B+R * rows[i].timestamp - rows[i-1].accByteReceived + B+R*rows[i-1].timestamp- rows[i-1].accByteReceived) / 2;
        }
        sumDifference = sumTrapezoid+1.0*bucketInforArr[bucketIndex].areaDifference;
    }
    return sumDifference;
}

int initBucketInforArr(struct BucketInfor* bucketInforArr, int start, int end){
    double step=1*1024;
    for(int i=start;i<=end;i++){
        bucketInforArr[i].bucketSize=step*i;
        bucketInforArr[i].tokenGenerateRate=-1;
        bucketInforArr[i].areaDifference=-1;
        bucketInforArr[i].index=0;
    }
}

int getBucketInfor(struct Row* rows, int index,struct BucketInfor* bucketInforArr, int bucketIndex, int upperBucketIndex){
    if(index==bucketInforArr[bucketIndex].index){
        return 0;
    }
    double R=rateForBucket(rows, index, bucketInforArr, bucketIndex);
    double previousR=bucketInforArr[bucketIndex].tokenGenerateRate;
    bucketInforArr[bucketIndex].tokenGenerateRate=R;
    double areaDifference=-1;
    if(bucketInforArr[bucketIndex].index==0){
        if(bucketIndex==upperBucketIndex){
            // no need to run this every time for each new Upper bucket
            areaDifference=areaDifferenceFromScratch(rows, index, bucketInforArr, bucketIndex);    
        }
        else{
            areaDifference=areaDifferenceIntersection(upperBucketIndex, bucketIndex, rows[index].timestamp, bucketInforArr);
        }
    }
    else{
        if(bucketIndex==upperBucketIndex){
            areaDifference=areaDifferencePacket(rows, index, bucketInforArr, bucketIndex, previousR);
        }
        else{
            areaDifference=areaDifferenceIntersection(upperBucketIndex, bucketIndex, rows[index].timestamp, bucketInforArr);
        }
    }
    bucketInforArr[bucketIndex].index=index;
    bucketInforArr[bucketIndex].areaDifference=areaDifference;
}

struct Param estimation(struct Row *rows, int index, struct BucketInfor** bucketInforArr, int* upperBucketIndex, int* lowerBucketIndex, struct Param lastParam){

    while(1){
        getBucketInfor(rows, index, *bucketInforArr, *upperBucketIndex, *upperBucketIndex);
        getBucketInfor(rows, index, *bucketInforArr, (*upperBucketIndex)-1, *upperBucketIndex);
        if((*bucketInforArr)[(*upperBucketIndex)-1].areaDifference > (*bucketInforArr)[*upperBucketIndex].areaDifference){
            (*bucketInforArr)=(struct BucketInfor*)realloc((*bucketInforArr), ((*upperBucketIndex)*2+1)*sizeof(struct BucketInfor));
            initBucketInforArr(*bucketInforArr, (*upperBucketIndex)+1, 2*(*upperBucketIndex));
            *upperBucketIndex=(*upperBucketIndex)*2;
        }
        else{
            break;
        }
    }

    struct Param bestParam;
    if(lastParam.bucketIndex!=-1){
        getBucketInfor(rows, index, *bucketInforArr, lastParam.bucketIndex, *upperBucketIndex);
        getBucketInfor(rows, index, *bucketInforArr, lastParam.bucketIndex+1, *upperBucketIndex);
        if(lastParam.bucketIndex==0){
            if((*bucketInforArr)[lastParam.bucketIndex+1].areaDifference >= (*bucketInforArr)[lastParam.bucketIndex].areaDifference){
                bestParam.bucketSize=(*bucketInforArr)[lastParam.bucketIndex].bucketSize;
                bestParam.areaDifference=(*bucketInforArr)[lastParam.bucketIndex].areaDifference;
                bestParam.tokenGenerateRate=(*bucketInforArr)[lastParam.bucketIndex].tokenGenerateRate;
                bestParam.bucketIndex=lastParam.bucketIndex;
                rows[index].estimateBucket=bestParam.bucketSize;
                rows[index].estimateTokenGenerateRate=bestParam.tokenGenerateRate;
                rows[index].correspondingArea=bestParam.areaDifference;
                return bestParam;
            }
        }
        else{
            getBucketInfor(rows, index, *bucketInforArr, lastParam.bucketIndex-1, *upperBucketIndex);
            if((*bucketInforArr)[lastParam.bucketIndex-1].areaDifference >= (*bucketInforArr)[lastParam.bucketIndex].areaDifference && (*bucketInforArr)[lastParam.bucketIndex+1].areaDifference>=(*bucketInforArr)[lastParam.bucketIndex].areaDifference){
                bestParam.bucketSize=(*bucketInforArr)[lastParam.bucketIndex].bucketSize;
                bestParam.areaDifference=(*bucketInforArr)[lastParam.bucketIndex].areaDifference;
                bestParam.tokenGenerateRate=(*bucketInforArr)[lastParam.bucketIndex].tokenGenerateRate;
                bestParam.bucketIndex=lastParam.bucketIndex;
                rows[index].estimateBucket=bestParam.bucketSize;
                rows[index].estimateTokenGenerateRate=bestParam.tokenGenerateRate;
                rows[index].correspondingArea=bestParam.areaDifference;
                return bestParam;
            }
        }

    }

    int upperBound=*upperBucketIndex;
    int lowerBound=*lowerBucketIndex;
    while(1){
        int middleIndex=(int)((lowerBound+upperBound)/2);
        getBucketInfor(rows, index, (*bucketInforArr), middleIndex, upperBound);
        getBucketInfor(rows, index, (*bucketInforArr), middleIndex-1, upperBound);
        if((*bucketInforArr)[middleIndex-1].areaDifference<(*bucketInforArr)[middleIndex].areaDifference){
            upperBound=middleIndex;
            //*upperBucketIndex=middleIndex;
        }
        else{
            getBucketInfor(rows, index, (*bucketInforArr), middleIndex+1, upperBound);
            if((*bucketInforArr)[middleIndex+1].areaDifference<(*bucketInforArr)[middleIndex].areaDifference){
                lowerBound=middleIndex;
                //*lowerBucketIndex=middleIndex;
            }
            else{
                bestParam.areaDifference=(*bucketInforArr)[middleIndex].areaDifference;
                bestParam.bucketSize=(*bucketInforArr)[middleIndex].bucketSize;
                bestParam.tokenGenerateRate=(*bucketInforArr)[middleIndex].tokenGenerateRate;
                bestParam.bucketIndex=middleIndex;
                break;
            }
        }
        if(upperBound - lowerBound<=1){
            if(middleIndex==upperBound){
                bestParam.areaDifference=(*bucketInforArr)[lowerBound].areaDifference;
                bestParam.bucketSize=(*bucketInforArr)[lowerBound].bucketSize;
                bestParam.tokenGenerateRate=(*bucketInforArr)[lowerBound].tokenGenerateRate;
                bestParam.bucketIndex=lowerBound;
            }
            else{
                bestParam.areaDifference=(*bucketInforArr)[upperBound].areaDifference;
                bestParam.bucketSize=(*bucketInforArr)[upperBound].bucketSize;
                bestParam.tokenGenerateRate=(*bucketInforArr)[upperBound].tokenGenerateRate;
                bestParam.bucketIndex=upperBound;
            }
            break;
        }

    }
    rows[index].estimateBucket=bestParam.bucketSize;
    rows[index].estimateTokenGenerateRate=bestParam.tokenGenerateRate;
    rows[index].correspondingArea=bestParam.areaDifference;
    return bestParam;
}

int classify(struct Row *rows, int index, struct Param param, double decreaseThre, double areaThre, int packetThre, double* areaDifferencePercent){
    struct Row row=rows[index];
    double B=param.bucketSize;
    double R=param.tokenGenerateRate;
    int packetV=-1;
    int last_packetv=-1;
    if(last_packetv==-1){
        for(int i=0;i<=index;i++){
            if(rows[i].accByteReceived<=B){
                packetV=i;
            }
            else{
                break;
            }
        }        
    }
    else{
        if(rows[last_packetv].accByteReceived<=B){
            for(int i=last_packetv;i<=index;i++){
                if(rows[i].accByteReceived<=B){
                    packetV=i;
                }
                else{
                    break;
                }
            }   
        }
        else{
            for(int i=0;i<=last_packetv;i++){
                if(rows[i].accByteReceived<=B){
                    packetV=i;
                }
                else{
                    break;
                }
            } 
        }
    }

    if(packetV==-1){
        return 0;
    }

    last_packetv=packetV;
    
    if(packetV + packetThre >= index){
        return 0;
    }

    double now_B_RT_packetV=B+R*rows[packetV].timestamp;
    double previous_B_RT_packetV=rows[packetV].estimateBucket+rows[packetV].estimateTokenGenerateRate*rows[packetV].timestamp;
    double bigTrapezoid=(row.timestamp - rows[packetV].timestamp)*(now_B_RT_packetV+B+R*row.timestamp)/2;
    double smallArea=0;
    double below_A_packetV_area=(rows[packetV].estimateBucket+previous_B_RT_packetV)*rows[packetV].timestamp/2-rows[packetV].correspondingArea;
    double minusArea=(B + now_B_RT_packetV)*rows[packetV].timestamp/2 - below_A_packetV_area;
    smallArea=param.areaDifference - minusArea;

    int areaDifferenceFlag=0;
    if(smallArea/bigTrapezoid<areaThre){
        areaDifferenceFlag=1;
    }
    double rateDecreaseFlag=0;
    double averageRate=rows[packetV].accByteReceived/rows[packetV].timestamp;
    if(R/averageRate<decreaseThre){
        rateDecreaseFlag=1;
    }
    if(rateDecreaseFlag && areaDifferenceFlag){
        *areaDifferencePercent=smallArea/bigTrapezoid;
        return 1;
    }
    return 0;
}

int writeCSV(struct Param *params, int numRow, FILE *csvFileHandler, struct Row *rows) {
    for (int i = 1; i < numRow; i++) {
        fprintf(csvFileHandler, "%d,%d,%lf,%lf,%d,%lf\n", params[i].result1, params[i].result2, params[i].bucketSize, params[i].tokenGenerateRate, rows[i].accByteReceived, params[i].areaDifference);
    }
    return 0;
}

struct Param estimationClassify(struct Row *rows, int numRow, FILE *csvFileHandler) {
    double decreaseThre=0.6;
    double areaThre=0.075;
    int packetThre=10;
    int triggerInt = 20;

    int upperBucketIndex=2;
    int lowerBucketIndex=0;
    struct BucketInfor* bucketInforArr=(struct BucketInfor*)malloc((upperBucketIndex+1)*sizeof(struct BucketInfor));

    struct Param *params = malloc(numRow * sizeof(struct Param));
    params[0].areaDifference = 0;
    params[0].tokenGenerateRate = 0;
    params[0].bucketSize = 0;
    params[0].bucketIndex = -1;
    initBucketInforArr(bucketInforArr, 0, upperBucketIndex);
    int classifyFlag=0, finalClassifyFlag=0;
    struct Param lastParam;
    for (int i = 1; i < numRow; i++) {
        struct Param param=estimation(rows, i, &bucketInforArr, &upperBucketIndex, &lowerBucketIndex, params[i-1]);
        params[i]=param;
        params[i].result1=0;
        params[i].result2=0;
        lastParam=param;
        double areaDifferencePercent;
        if(finalClassifyFlag==0){
            int classifyResult=classify(rows,i,param,decreaseThre,areaThre,packetThre, &areaDifferencePercent);
            if(classifyResult==1){
                params[i].result1=1;
                classifyFlag+=1; 
                if(classifyFlag>=triggerInt){
                    finalClassifyFlag=1;
                    params[i].result2=1;
                }
            }
            else{
                classifyFlag=0;
            }            
        }
    }
    // writeCSV(params, numRow, csvFileHandler, rows);
    free(params);
    free(bucketInforArr);
    if(finalClassifyFlag==1)
        return lastParam;
    else{
        lastParam.bucketSize=-1;
        return lastParam;
    }
}