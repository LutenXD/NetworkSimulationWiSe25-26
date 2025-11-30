message CustomerMsg
{
    int customerId;
    float totalWaitingTime = 0.0;
    int numberOfItems;  // 1 <= numberOfItems <= 25
    simtime_t arrivalTime;
    simtime_t serviceStartTime = 0;
}