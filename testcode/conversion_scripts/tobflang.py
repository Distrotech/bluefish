import sys

class ToBflang():

	def __init__(self):
		self.root = None
		self.elementchildren = {}
		self.attributes = {}

	def contextstring(self,children):
		ret = ''
		needcomma=False
		for k in children:
			if (needcomma):
				ret += ','
			ret += k
			needcomma=True
		return ret
	
	def indent(self,level):
		i=level
		while (i > 0):
			self.fd.write('\t')
			i -= 1
	
	def dump(self, element, level):
		self.indent(level)
		if (element in self.writtenelements):
			self.fd.write('<tag idref="tag-'+element+'" />\n')
			return
		self.fd.write('<tag name="'+element+'" id="tag-'+element+'" highlight="tag"')
		self.writtenelements.append(element)
		if (self.attributes.has_key(element) and len(self.attributes[element])>0):
			self.fd.write(' attributes="')
			needcomma = False
			for i in self.attributes[element]:
				if (needcomma):
					self.fd.write(',')
				self.fd.write(i)
				needcomma=True
			self.fd.write('"')
		if (self.elementchildren.has_key(element) and len(self.elementchildren[element])>0):
			self.fd.write('>\n')
			self.indent(level)
			# see if we have this context already
			tmp = self.contextstring(self.elementchildren[element])
			if (tmp in self.writtencontexts):
				num = self.writtencontexts.index(tmp)
				self.fd.write('<context idref="context-'+str(num)+'" />\n')
			else:
				self.writtencontexts.append(tmp)
				self.fd.write('<context id="context-'+str(len(self.writtencontexts)-1)+'" symbols="&gt;&lt;&amp;; &#9;&#10;&#13;-">\n')
				for c in self.elementchildren[element]:
					self.dump(c, level+1)
				self.indent(level)
				self.fd.write('</context>\n')
			self.indent(level)
			self.fd.write('</tag>\n')
		else:
			self.fd.write('/>\n')
	
	def write(self, fd):
		self.fd = fd
		self.writtencontexts = []
		self.writtenelements = []
		self.fd.write('<?xml version="1.0"?>\n<bflang name="tmp" version="2.0">\n<header><mime type="text/xml+custom"/><highlight name="tag" style="tag"  /></header>\n')
		self.fd.write('<definition>\n<context symbols=" &gt;&lt;">\n')
		self.dump(self.root, 1)
		self.fd.write('</context>\n</definition></bflang>\n')
