#!/bin/bash

ipcs -mp
egrep -l "shmid" /proc/[1-9]*/maps
lsof | egrep "shmid"