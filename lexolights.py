#!/usr/bin/python

import time
import subprocess
import sys
import os

t = time.time()  # UTC time
os.environ[ "START_TIME" ] = str( t )
a = sys.argv
a[0] = "./lexolights"
#print "Starting lexolight with time", t
child = subprocess.Popen( a,
                          0, None, sys.stdin, sys.stdout, sys.stderr,
                          None, False, False, None,
                          None,
                          False, None, 0 )
child.wait();
sys.exit( child.returncode )
