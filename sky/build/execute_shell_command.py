#!/usr/bin/env python

import sys, subprocess

args = sys.argv
returnCode = subprocess.call(args[1:])
sys.exit(returnCode);