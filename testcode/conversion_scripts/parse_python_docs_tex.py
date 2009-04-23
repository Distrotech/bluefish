#!/usr/bin/python
#
import re
import os
import string
#svn co http://svn.python.org/projects/python/trunk # checking out Python tree
#cd trunk/Doc
#make latex
#then run this script (on latex/library.tex)

#reg1 = re.compile("\\begin\{funcdesc\}\{([a-z]+)\}\{(.*)\}")
reg1 = re.compile("(\\\\hypertarget\\{[a-z0-9A-Z._]+\\}\\{\\})?\\\\begin\\{funcdesc\\}\\{([a-zA-Z0-9._\\\\]+)\\}\\{(.*)\\}.*")
reg2 = re.compile("\\\\end\{funcdesc\}")

skipcommands = 'hyperlink', 'hline', 'index'

def striptex(buf):
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

class ParsedParam:
	name = ''
	def __init__(self,name):
		self.name = name
	def printfref(self):
		self.name = string.strip(self.name)
		self.type = string.strip(self.type)
		self.description = string.strip(self.description)
		print '<param name="'+self.name+'" required="1" vallist="0" default="" type="'+self.type+'">'+self.description+'</param>'

class ParsedFunc:
	name = ''
	description = ''
	returntype = ''
	params = {}
	def __init__(self, fd, match):
		self.name = match.group(2)
		print 'name',self.name
		print 'params', match.group(3)
		line = fd.readline()
		endnotfound = 1
		while (endnotfound):
			m = reg2.match(line)
			if (m):
				print 'description',striptex(self.description)
				return
			else:
				self.description += line
			line = fd.readline()
	def printfref(self):
		if (not self.name or not self.description):
			return
		self.name = string.strip(self.name)
		self.returntype = string.strip(self.returntype)
		self.returndescription = string.strip(self.returndescription)
		self.description = string.strip(self.description)
		print '<function name="'+self.name+'">'
		print '<description>'+self.description+'</description>'
		dialogstr = self.name+'('
		insertstr = self.name+'('
		paramnum = 0
		for key in self.params:
			p = self.params[key]
			p.printfref()
			if (paramnum > 0):
				insertstr += ', '
				dialogstr += ', '
			dialogstr += '%'+str(paramnum)
			insertstr += p.name
			paramnum += 1
		insertstr += ')'
		dialogstr += ')'
		print '<return type="'+self.returntype+'">'+self.returndescription+'</return>'
		print '<dialog title="'+self.name+'">'+dialogstr+'</dialog>'
		print '<insert>'+insertstr+'</insert>'
		print '<info title="'+self.name+'">'+self.name+'</info>'
		print '</function>'


def parsefile(infile):
	fd = open(infile)
	line = fd.readline()
	while (line):
#		print '"'+line+'"'
		func = reg1.match(line[:-1])
		if (func):
#			print line[:-1]
			f = ParsedFunc(fd,func)
		line = fd.readline()
		
parsefile('latex/library.tex')