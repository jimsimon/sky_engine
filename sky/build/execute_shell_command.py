#!/usr/bin/env python

import sys, subprocess

returnCode = subprocess.call(sys.argv[1:])
print returnCode
sys.exit(returnCode)