#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <float.h>
#include <math.h>

struct Row {
    double timestamp;
    int bytesReceived;
    int accByteReceived;

    double estimateBucket;
    double estimateTokenGenerateRate;
    double correspondingArea;
};

struct Param {
    double areaDifference;
    double bucketSize;
    double tokenGenerateRate;
    int bucketIndex;
    int result1;
    int result2;
};

struct BucketInfor{
    double areaDifference;
    double bucketSize;
    double tokenGenerateRate;
    int index;
};

struct Row *readFromTrace(FILE *file, int *realNumRow);

char **getFolders(int *numFolder, char* parentFolder);

int startWith(char *pre, char *str);

char **filterFolders(char **folders, int *numFolder, char *prefix);

double rateForBucket(struct Row* rows, int index, struct BucketInfor* bucketInforArr, int bucketIndex);

double areaDifferenceIntersection(int calculatedIndex, int index, double time, struct BucketInfor* bucketInforArr);

double areaDifferenceFromScratch(struct Row* rows, int index, struct BucketInfor* bucketInforArr, int bucketIndex);

double areaDifferencePacket(struct Row* rows, int index, struct BucketInfor* bucketInforArr, int bucketIndex, double previousR);

int initBucketInforArr(struct BucketInfor* bucketInforArr, int start, int end);

int getBucketInfor(struct Row* rows, int index,struct BucketInfor* bucketInforArr, int bucketIndex, int upperBucketIndex);

struct Param estimation(struct Row *rows, int index, struct BucketInfor** bucketInforArr, int* upperBucketIndex, int* lowerBucketIndex, struct Param lastParam);

int classify(struct Row *rows, int index, struct Param param, double decreaseThre, double areaThre, int packetThre, double* areaDifferencePercent);

int writeCSV(struct Param *params, int numRow, FILE *csvFileHandler, struct Row *rows);

struct Param estimationClassify(struct Row *rows, int numRow, FILE *csvFileHandler);