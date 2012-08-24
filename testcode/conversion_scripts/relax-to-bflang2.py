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
		self.refs[myname] = element

	def parse_grammar(self, element):
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
						self.parse_element(node2, '');
					elif (node2.tagName == 'ref'):
						refname = node2.getAttributeNode('name').nodeValue
						self.parse_element(self.refs[refname], '')
						

	def parse(self, filename):
		dom1 = xml.dom.minidom.parse(filename)
		if (dom1.documentElement.tagName == 'grammar'):
			print 'have grammar file'
			self.parse_grammar(dom1.documentElement)
		elif (dom1.documentElement.tagName == 'element'):
			print 'have simple file'
			self.parse_element(dom1.documentElement, '')
		else:
			print 'unknown format'
	
for test in ('1.rng', '2.rng', 'mini-relax-ng.xml', 'mini-grammar-relax-ng.xml', '3.rng'):
	print '*** parsing '+test+' ***'
	r2b = RelaxToBflang()
	r2b.parse(test)
	r2b.write(open('out.bflang2', 'w'))
	
