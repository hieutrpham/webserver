#!/usr/bin/env python3

import time
import sys

sys.stdout.write("Content-Type: text/plain\r\n\r\n")
sys.stdout.flush()

time.sleep(30)
print("CGI fnished after 30 seconds")