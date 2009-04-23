#!/usr/bin/python
#
import re
import os
import string
import sys
from xml.sax.saxutils import escape

# from http://mhordecki.wordpress.com/2008/02/10/sphinx-official-python-documentation-tool/
#
#svn co http://svn.python.org/projects/python/trunk # checking out Python tree
#cd trunk/Doc
#make latex
#
# then run this script (on latex/library.tex)
#
#
# further info: http://sphinx.pocoo.org/contents.html
#

#reg1 = re.compile("\\begin\{funcdesc\}\{([a-z]+)\}\{(.*)\}")
reg1 = re.compile("(\\\\hypertarget\\{[a-z0-9A-Z._]+\\}\\{\\})?\\\\begin\\{funcdesc\\}\\{([a-zA-Z0-9._\\\\]+)\\}\\{(.*)\\}.*")
reg2 = re.compile("\\\\end\{funcdesc\}")

def striptex(buf):
	skipcommands = 'hyperlink', 'hline', 'index'
	ret=''
	incommand=0
	command=''
	skipcommand=0
	for c in buf:
		if (incommand):
			if (c=='{'):
				incommand=0
#				print 'found',command
				if (command in skipcommands):
#					print 'skip',command
					skipcommand=1
				command=''
			else:
				command+=c
		elif (skipcommand):
			if (c=='}'):
				skipcommand=0
		else:
			if (c=='\\'):
				incommand=1
			elif (c=='}' or c=='{'):
				pass
			else:
				ret+=c
	return ret

class ParsedFunc:
	name = ''
	description = ''
	returntype = ''
	params = {}
	def __init__(self, fd, match):
		self.name = match.group(2)
		#print 'name',self.name
		#print 'params', match.group(3)
		line = fd.readline()
		endnotfound = 1
		while (endnotfound):
			m = reg2.match(line)
			if (m):
				#print 'description',striptex(self.description)
				return
			else:
				self.description += line
			line = fd.readline()

	def printelement(self):
		if (not self.name or not self.description):
			return
		self.name = escape(string.strip(self.name))
		self.description = escape(striptex(string.strip(self.description)))
		print '<element pattern="'+self.name+'"><reference>'+self.description+'</reference></element>'


def parsefile(infile):
	fd = open(infile)
	line = fd.readline()
	while (line):
#		print '"'+line+'"'
		func = reg1.match(line[:-1])
		if (func):
#			print line[:-1]
			f = ParsedFunc(fd,func)
			f.printelement()
		line = fd.readline()

if (len(sys.argv)>1):
	parsefile(sys.argv[1])
else:
	parsefile('latex/library.tex')