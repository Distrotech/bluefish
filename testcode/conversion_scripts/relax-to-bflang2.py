#!/usr/bin/python
import xml.dom.minidom
import tobflang
import sys

class RelaxToBflang(tobflang.ToBflang):
	refs = {}
#	def __init__(self):
#		super(relaxtobflang, self).__init__()

	def parse_children(self, pnode, parentname):
		for node in pnode.childNodes:
			if (node.nodeType == node.ELEMENT_NODE):
				if (node.tagName == 'element'):
					self.parse_element(node, parentname)
				elif (node.tagName == 'attribute'):
					name = node.getAttributeNode('name')
					self.attributes[parentname].append(name.nodeValue)
				elif (node.tagName in ('oneOrMore', 'zeroOrMore', 'optional', 'choice', 'group', 'interleave')):
					self.parse_children(node, parentname)
				elif (node.tagName == 'ref'):
					name = node.getAttributeNode('name')
					refname = name.nodeValue
					self.parse_children(self.refs[refname], parentname)
				elif (node.tagName == 'externalRef'):
					href = node.getAttributeNode('href').nodeValue
					self.parse(href, parentname)
			else:
				#print node
				pass
	
	def parse_element(self, element, parentname):
		# first pass the element itself
		myname = element.getAttributeNode('name').nodeValue
		created = 0
		if (self.elementchildren.has_key(myname)):
			print 'element '+myname+' was already parsed'
		else:
			print 'create element '+myname+''
			self.elementchildren[myname] = []
			self.attributes[myname] = []
			created = 1
		
		if (parentname == ''):
			self.root = myname
		else:
			self.elementchildren[parentname].append(myname)
		# then parse the children
		if (created):
			self.parse_children(element, myname)

	def parse_define(self, element):
		myname = element.getAttributeNode('name').nodeValue
		print 'registering define '+myname
		self.refs[myname] = element

	def parse_grammar(self, element, parentname):
		print 'parse_grammar'
		# first parse all defines
		elms = element.getElementsByTagName("define")
		for node in elms:
			print 'parse define'
			self.parse_define(node)
		#then parse start
		elms = element.getElementsByTagName("start")
		for node in elms:
			for node2 in node.childNodes:
				if (node2.nodeType == node2.ELEMENT_NODE):
					if (node2.tagName == 'element'):
						print 'have element'
						self.parse_element(node2, parentname);
					elif (node2.tagName == 'ref'):
						refname = node2.getAttributeNode('name').nodeValue
						print 'have ref '+refname
						if (self.refs[refname].tagName == 'element'):
							self.parse_element(self.refs[refname], parentname)
						else:
							self.parse_children(self.refs[refname], parentname)
						
	def parse(self, filename, parentname):
		dom1 = xml.dom.minidom.parse(filename)
		if (dom1.documentElement.tagName == 'grammar'):
			print 'have grammar file'
			self.parse_grammar(dom1.documentElement, parentname)
		elif (dom1.documentElement.tagName == 'element'):
			print 'have simple file'
			self.parse_element(dom1.documentElement, parentname)
		else:
			print 'unknown format'

if __name__ == "__main__":
	print 'loop over argv'
	for filename in sys.argv[1:]:
		print filename
		if (filename[-4:]=='.rng'):
			newfilename = filename[:-4]+'.bflang2'
		else:
			newfilename = filename+'.bflang2'
		r2b = RelaxToBflang()
		r2b.parse(filename, '')
		r2b.write(open(newfilename, 'w'))
