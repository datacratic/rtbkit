#
# to_junit.py
# Nicolas Kructhen, 2013-03-26
# Mich, 2015-03-05
# Copyright (c) 2013 Datacratic Inc.  All rights reserved.
#
# make test | python junit.py > testresults.xml

import os
import fileinput
from xml.etree import ElementTree
from StringIO import StringIO

passed = set()
failed = set()

for l in fileinput.input():
    pieces = l.strip().replace("\x1b[0m", "").replace("\x1b[32m", "") \
        .replace("\x1b[31m", "").split()
    if not len(pieces) == 2:
        continue
    if pieces[1] == "passed":
        passed.add(pieces[0])
    if pieces[1] == "FAILED":
        failed.add(pieces[0])

failed.difference_update(passed)

builder = ElementTree.TreeBuilder()
builder.start('testsuite', {
    'errors'   : '0',
    'tests'    : str(len(passed) + len(failed)),
    'time'     : '0',
    'failures' : str(len(failed)),
    'name'     : 'tests'
})

for f in failed:
    failContent = ""

    if os.path.isfile("build/x86_64/tests/%s.failed" % f):
        with open("build/x86_64/tests/%s.failed" % f, "r") as failFile:
            failContent = failFile.read()
    builder.start('testcase', {
        'time' : '0',
        'name' : f
    })
    builder.start('failure', {
        'type' : 'failure',
        'message' : 'Check log'
    })
    builder.data(failContent)
    builder.end('failure')
    builder.end('testcase')

for p in passed:
    builder.start('testcase', {
        'time' : '0',
        'name' : p
    })
    builder.end('testcase')

builder.end('testsuite')

tree = ElementTree.ElementTree()
element = builder.close()
tree._setroot(element)
io = StringIO()
tree.write(io, encoding='ascii', xml_declaration=True)
print io.getvalue()
