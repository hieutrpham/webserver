#!/usr/bin/env python3
import html
import os
from urllib.parse import parse_qs

params = parse_qs(os.environ.get("QUERY_STRING", ""))
try:
    code = int(params.get("code", ["200"])[0])
except ValueError:
    code = 400
reasons = {200: "OK", 201: "Created", 400: "Bad Request", 404: "Not Found", 418: "I'm a teapot", 500: "Internal Server Error"}
reason = reasons.get(code, "Custom Status")
print(f"Status: {code} {reason}")
print("Content-Type: text/html; charset=utf-8")
print()
print("<!DOCTYPE html><html><head><meta charset='utf-8'><title>CGI Status</title><link rel='stylesheet' href='/css/styles.css'></head><body><main>")
print(f"<section class='page-header'><p class='eyebrow'>CGI status test</p><h1>{code} {html.escape(reason)}</h1><p>The CGI script emitted a Status header. Verify that Webserv used it in the HTTP response status line.</p><a class='button primary' href='/cgi.html'>Back</a></section>")
print("</main></body></html>")
