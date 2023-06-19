#!/usr/bin/python

HTML_TEMPLATE = "<!DOCTYPE html><html>"\
"<head><title>CGI demo</title><style>body{{text-align: center}};</style></head>"\
"<body><h1>{}</h1><p>{}</p></body></html>"

print("Conten-type: text/html")
print("\r\n",end='')
print(HTML_TEMPLATE.format("Robert Frost", "A poet"))
