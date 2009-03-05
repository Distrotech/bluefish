#!/usr/bin/python


import xml.dom.minidom

bflib = xml.dom.minidom.parse('/home/olivier/svnbluefish/data/bflib/bflib_css2.xml')

def getText(parent):
	rc = ""
	for node in parent.childNodes:
		if node.nodeType == node.TEXT_NODE:
			rc = rc + node.data
	return rc

def getCssProperties(node):
	tmp = ''
	for child in node.childNodes:
		if (child.nodeName == 'property'):
			for child2 in child.childNodes:
				if (child2.nodeName == 'description'):
					tmp = tmp + getText(child2)
	return tmp

def getCssElement(node):
	tmp = ''
	name = None
	if (node.attributes and node.attributes.has_key('name')):
		name=node.attributes['name'].value.strip()
	for child in node.childNodes:
		if (child.nodeName == 'description'):
			tmp = tmp + getText(child)+'\n'
		elif (child.nodeName == 'properties'):
			tmp = tmp + getCssProperties(child)
	return name,tmp

def printchilds(node):
	if node.nodeType == node.TEXT_NODE:
		tmp = node.data.strip()
		if (len(tmp)>1):
			print tmp
	else:
		if (node.attributes and node.attributes.has_key('name')):
			print 'element=',node.nodeName.strip(),'name=', node.attributes['name'].value.strip()
		else:
			print 'element=',node.nodeName.strip()
		for child in node.childNodes:
			printchilds(child)

for node in bflib.getElementsByTagName('element'):
	name,data = getCssElement(node)
	print name,data

#printchilds(bflib)		
