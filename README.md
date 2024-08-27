# Introduction

This repository provides the source code implementation for the research presented in the paper "On Rate-Limiting in Mobile Data Networks," which was published in IEEE Transactions on Mobile Computing.

This code implements functions for detecting rate limiting and estimating token bucket parameters, with the core function being `estimationClassify`. Auxiliary functions support the analysis.

## Functions

### `char **getFolders(int *numFolder, char* parentFolder)`

This function retrieves the names of all subfolders within the specified parent folder.

**Parameters:**

* `numFolder`: A pointer to the variable storing the count of subfolders within the specified parent folder.

* `parentFolder`: The directory path of the parent folder.

**Returns:**

* An array containing the names of all subfolders within the parent folder.

### `char **filterFolders(char **folders, int *numFolder, char *prefix)`

Not all subfolders within the parent folder may contain network traces. This function filters out folders lacking the specified prefix, updating the folder count accordingly.

**Parameters:**

* `folders`: An array containing the names of all subfolders within the parent folder.

* `numFolder`: A pointer to the variable storing the count of subfolders within the specified parent folder that match the given prefix.

* `prefix`: A string used to filter out folders that do not begin with this string.

**Returns:**

* An array containing the names of all subfolders within the parent folder that match the specified prefix.

### `struct Row *readFromTrace(FILE *file, int *realNumRow)`

The function reads a trace file, storing its information in an array. Each row in the file represents an ACK packet, containing four data segments separated by spaces. These segments represent:

1. Time of ACK packet reception.
2. Number of bytes acknowledged in this ACK packet.
3. Total bytes acknowledged since the beginning of the trace.
4. Round-trip time.

Packet loss is indicated by the last three data segments set to 0.

**Parameters:**

* `file`: A file handler is associated with the trace file.

* `realNumRow`: A pointer to the variable storing the counts of ACK and loss packets.

**Returns:**

* An array containing the data segments of all ACK and loss packets.

### `struct Param estimationClassify(struct Row *rows, int numRow, FILE *csvFileHandler)`

The function analyzes trace file data to detect rate limiting and estimate relevant parameters.

**Parameters:**

* `rows`: An array containing the data segments of all ACK and loss packets.

* `numRow`:  A pointer to the variable storing the counts of ACK and loss packets.

* `csvFileHandler`:  A file handler is associated with the result csv file.

**Returns:**

* The detection and estimation result.
