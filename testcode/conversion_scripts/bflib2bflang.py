#!/usr/bin/python


import xml.dom.minidom

references = {}

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

bflib = xml.dom.minidom.parse('/home/olivier/svnbluefish/data/bflib/bflib_css2.xml')
for node in bflib.getElementsByTagName('element'):
	name,data = getCssElement(node)
	if (name != None and len(data)>5):
		references[name] = data

print 'have '+str(len(references))+' references'

haveprops = {}		

def element_has_reference(node):
	for e in node.childNodes:
		if (e.nodeType == e.ELEMENT_NODE and e.nodeName == 'reference'):
			return True
	return False

def add_reference(node, pattern):
	print 'add to '+pattern
	child = bflang.createElement("reference")
	txt = bflang.createTextNode(references[pattern])
	child.appendChild(txt)
	node.appendChild(child)

def fill_css_references(node):
	for child in node.childNodes:
		if (child.nodeType == child.ELEMENT_NODE and child.nodeName == 'element'):
			if (child.attributes and child.attributes.has_key('pattern')):
				pattern = child.attributes['pattern'].value
				haveprops[pattern] = True
				if (not element_has_reference(child) and references.has_key(pattern)):
					add_reference(child, pattern)

def add_missing_patterns(node):
	for key in references.keys():
		if (not haveprops.has_key(key)):
			print key, 'is missing'

bflang = xml.dom.minidom.parse('/home/olivier/svnbluefish/data/bflang/css.bflang2')
for node in bflang.getElementsByTagName('context'):
	if (node.attributes and node.attributes.has_key('id') and node.attributes['id'].value == 'css_properties'):
		fill_css_references(node)
		add_missing_patterns(node)

fp = open("css.bflang2","w")
# writexml(self, writer, indent='', addindent='', newl='', encoding=None)
bflang.writexml(fp)

print 'have '+str(len(haveprops))+' properties'

#print bflang.toxml()
