#!/usr/bin/python
import cgi
import cgitb


# HTML_HEAD = r""
# HTML_BODY = r""
HTML_TEMPLATE = "<!DOCTYPE html><html><head><title>CGI demo</title><style>body{{text-align: center}};</style></head><body><h1>{}</h1><p>{}</p></body></html>"

print(HTML_TEMPLATE.format("title", "content"))
