message JobMsg
{
    int jobId;
    simtime_t arrivalTime;
    simtime_t queueEnterTime = 0;
    simtime_t serviceStartTime = 0;
    simtime_t serviceEndTime = 0;
}