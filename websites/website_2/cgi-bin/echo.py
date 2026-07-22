#!/usr/bin/env python3
import html
import os
import sys
from urllib.parse import parse_qs


def read_parameters():
    method = os.environ.get("REQUEST_METHOD", "GET").upper()
    raw = os.environ.get("QUERY_STRING", "")
    if method == "POST":
        try:
            length = int(os.environ.get("CONTENT_LENGTH", "0") or "0")
        except ValueError:
            length = 0
        raw = sys.stdin.read(length)
    return method, raw, parse_qs(raw, keep_blank_values=True)


def main():
    method, raw, params = read_parameters()
    print("Content-Type: text/html; charset=utf-8")
    print("X-CGI-Script: echo.py")
    print()
    print("<!DOCTYPE html><html><head><meta charset='utf-8'><title>CGI Echo</title>")
    print("<link rel='stylesheet' href='/css/styles.css'></head><body><main>")
    print("<section class='page-header'><p class='eyebrow'>CGI executed</p>")
    print("<h1>Python CGI echo result</h1>")
    print(f"<p>Request method: <code>{html.escape(method)}</code></p>")
    print(f"<p>Raw input: <code>{html.escape(raw)}</code></p></section>")
    print("<section class='content-section'><h2>Parsed parameters</h2><ul>")
    if params:
        for key, values in sorted(params.items()):
            safe_values = ", ".join(html.escape(value) for value in values)
            print(f"<li><strong>{html.escape(key)}</strong>: {safe_values}</li>")
    else:
        print("<li>No parameters were supplied.</li>")
    print("</ul><h2>Selected CGI environment</h2><table class='test-table'>")
    for key in ("REQUEST_METHOD", "QUERY_STRING", "CONTENT_LENGTH", "CONTENT_TYPE", "SCRIPT_NAME", "PATH_INFO", "SERVER_NAME", "SERVER_PORT", "SERVER_PROTOCOL"):
        value = os.environ.get(key, "(not set)")
        print(f"<tr><th>{html.escape(key)}</th><td>{html.escape(value)}</td></tr>")
    print("</table><p><a class='button primary' href='/cgi.html'>Back to CGI laboratory</a></p></section>")
    print("</main></body></html>")


if __name__ == "__main__":
    main()
