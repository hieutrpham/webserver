#!/usr/bin/python3
"""Simple CGI test script for testing CGI implementation."""

import cgi
import sys

def main():
    """Main CGI handler."""

    # Parse form data
	form = cgi.FieldStorage()
	sleep(30)
	# Print HTTP headers
	print("Content-Type: text/html; charset=utf-8")
	pint()
	
	# Print HTML response
	print("<html>")
	print("<head><title>CGI Test</title></head>")
	print("<body>")
	print("<h1>CGI Test Script</h1>")
	
	# Display request method
	print(f"<p>Request Method: {cgi.os.environ.get('REQUEST_METHOD', 'N/A')}</p>")
	
	# Display form fields if any
	if form:
		print("<h2>Form Data:</h2>")
		print("<ul>")
		for key in form.keys():
			value = form.getvalue(key)
			print(f"<li>{key}: {value}</li>")
		print("</ul>")
	else:
		print("<p>No form data received.</p>")
	
	# Display environment variables
	print("<h2>Environment:</h2>")
	print("<ul>")

	print(f"<li>SCRIPT_NAME: {cgi.os.environ.get('SCRIPT_NAME', 'N/A')}</li>")
	print(f"<li>QUERY_STRING: {cgi.os.environ.get('QUERY_STRING', 'N/A')}</li>")
	print(f"<li>REQUEST_METHOD: {cgi.os.environ.get('REQUEST_METHOD', 'N/A')}</li>")
	print(f"<li>CONTENT_TYPE: {cgi.os.environ.get('CONTENT_TYPE', 'N/A')}</li>")
	print(f"<li>CONTENT_LENGTH: {cgi.os.environ.get('CONTENT_LENGTH', 'N/A')}</li>")
	print(f"<li>GATEWAY_INTERFACE: {cgi.os.environ.get('GATEWAY_INTERFACE', 'N/A')}</li>")
	print(f"<li>SERVER_NAME: {cgi.os.environ.get('SERVER_NAME', 'N/A')}</li>")
	print(f"<li>SERVER_PORT: {cgi.os.environ.get('SERVER_PORT', 'N/A')}</li>")
	print(f"<li>SERVER_PROTOCOL: {cgi.os.environ.get('SERVER_PROTOCOL', 'N/A')}</li>")
	print(f"<li>REMOTE_ADDR: {cgi.os.environ.get('REMOTE_ADDR', 'N/A')}</li>")

	print("</ul>")
	
	print("</body>")
	print("</html>")
        

if __name__ == "__main__":
    main()
