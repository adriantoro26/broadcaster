# Data Stream Application

## Description

This application will periodically take a sample, *every 3 seconds*, of the *seconds since boot* and the *sample number*. Every *10 seconds* The data is flushed into the console in a FIFO manner (first sampled, first appears in the screen).
We want to be able to configure the following things in the project:
- Sample rate (in seconds).
- Flush rate (in seconds).
- Flush order (FIFO or LIFO).

## Sample Output

    Flushing data...
    0 - Uptime 0 s
    1 - Uptime 3 s
    2 - Uptime 6 s
    3 - Uptime 9 s
    Flushing data...
    4 - Uptime 12 s
    5 - Uptime 15 s
    6 - Uptime 18 s
