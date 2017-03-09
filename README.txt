
Kewen Gu
Feb 14, 2016

There're 6 files included in this project nutella.c, nutella.h, mocules.c, msock.c, msock.h, and Makefile. The msock.c and msock.h are the two multicasr middleware files that provided by Prof. Claypool.

Use command "make" to compile the files.
to run nutella as a server, enter "./nutella -t server"
to run nutella as a client, enter "./nutella -t client"

The default multicast address is 239.0.0.1, you specify it yourself by using the '-s' option.
The default multicast port is 5123, you can specify it yourself by using the '-m' option.
The default unicast port is 5124, you can specify it yourself by using the '-u' option.
