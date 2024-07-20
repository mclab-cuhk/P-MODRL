# P-MODRL

Source code for the paper "On Rate-Limiting in Mobile Data Networks".

The algorithm has three outputs: the classification result, estimated rate limit, and estimated bucket size.
If the classification result is 1, it means rate limiting is detected. Otherwise, it is 0.
The unit of the estimated rate limit is Bytes per second. The unit of the bucket size is Bytes.

P-MODRLS is designed to analyze the trace. The `estimationClassify` function is invoked to run the analysis. The necessary input parameter is the array of the trace, which is defined in the header file. Therefore, you should use a tool like tcpdump to extract the information, such as the ACK timestamp and the acknowledged bytes of each ACK packet, from the PCAP file.
