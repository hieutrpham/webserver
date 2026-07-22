#!/usr/bin/env python3
import os
import time
from urllib.parse import parse_qs

params = parse_qs(os.environ.get("QUERY_STRING", ""))
try:
    seconds = max(0, min(30, int(params.get("seconds", ["3"])[0])))
except ValueError:
    seconds = 3
time.sleep(seconds)
print("Content-Type: text/html; charset=utf-8")
print()
print("<!DOCTYPE html><html><head><meta charset='utf-8'><title>Slow CGI</title><link rel='stylesheet' href='/css/styles.css'></head><body><main>")
print(f"<section class='page-header'><p class='eyebrow'>Delayed CGI complete</p><h1>Slept for {seconds} seconds</h1><p>While this script was running, other clients should still have been served if CGI handling is non-blocking.</p><a class='button primary' href='/cgi.html'>Back</a></section>")
print("</main></body></html>")
