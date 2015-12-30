from __future__ import print_function
import sys

from pycparser import c_ast, parse_file
from mako.template import Template


class TypedefVisitor(c_ast.NodeVisitor):
	def __init__(self):
		self.enums_list = list()
		self.node_tag_enums = list()

	def visit_Typedef(self, node):
		if type(node.type.type) == c_ast.Enum:
			self.enums_list.append(node.name)
			if node.name == "NodeTag":
				enumerators = node.type.type.values.enumerators
				self.node_tag_enums = [child.name for child in enumerators]


class StructMixin(object):
	def get_type(self, var_node, decl):
		type_name = "NotFound"
		if type(decl.type) == c_ast.ArrayDecl:
			type_name = decl.type.type.declname
		elif type(decl.type) == c_ast.TypeDecl:
			if type(decl.type.type) == c_ast.IdentifierType:
				type_name = decl.type.type.names[0]
			elif type(decl.type.type) == c_ast.FuncDecl:
				type_name = decl.type.type.args
		elif type(decl.type) == c_ast.PtrDecl:
			if type(decl.type.type) == c_ast.FuncDecl:
				type_name = decl.type.type.args
			else:
				local_node = decl.type.type.type
				if type(local_node) == c_ast.IdentifierType:
					type_name = local_node.names[0]
					var_node["pointer"] = True
				elif type(local_node) == c_ast.Struct:
					type_name = local_node.name
					var_node["pointer"] = True
		else:
			type_name = decl.type.name
		return type_name


class StructExVisitor(c_ast.NodeVisitor, StructMixin):
	def __init__(self, node_tags_structs):
		self.nodes_structs = []
		self.node_tags_structs = node_tags_structs

	def visit_Struct(self, node):
		if "/nodes/" not in node.coord.file:
			return

		if node.decls:
			for decl in node.decls:
				var_node = {"pointer": False}
				type_name = self.get_type(var_node, decl)

				if var_node["pointer"] is False and type_name in self.node_tags_structs:
					self.nodes_structs.append(node.name)


class StructVisitor(c_ast.NodeVisitor, StructMixin):
	def __init__(self, node_tag_enums):
		self.structs_dict = dict()
		self.node_tags_structs = []
		self.node_tag_enums = node_tag_enums

	def visit_Struct(self, node):
		if "/nodes/" not in node.coord.file:
			return
		#print(str(node.name)+" "+str(type(node)))

		struct_node = dict()

		if node.decls:
			# base node type ["type", "xpr", "plan", "scan", "join", "base"]
			for decl in node.decls:
				var_node = {"pointer": False}
				type_name = self.get_type(var_node, decl)

				#print("\t%s %s" % (decl.name, type_name))
				if type_name == "NodeTag" and var_node["pointer"] is False:
					self.node_tags_structs.append(node.name)

				if "T_" + node.name not in self.node_tag_enums or node.name == "List":
					return

				var_node["name"] = type_name
				var_node["node"] = decl.type
				struct_node[decl.name] = var_node

		if "T_" + node.name not in self.node_tag_enums or node.name == "List":
			return

		self.structs_dict[str(node.name)] = struct_node

if __name__ == "__main__":
	filename = sys.argv[1]
	postgres_include = r""
	if len(sys.argv) == 3:
		postgres_include = r"-I" + sys.argv[2]

	ast = parse_file(
		filename,
		use_cpp=True,
		cpp_path='cpp',
		cpp_args=[r"-I./fake_libc_include", postgres_include]
	)
	typedef_visitor = TypedefVisitor()
	typedef_visitor.visit(ast)

	struct_visitor = StructVisitor(node_tag_enums=typedef_visitor.node_tag_enums)
	struct_visitor.visit(ast)
	struct_visitor_2 = StructExVisitor(struct_visitor.node_tags_structs)
	struct_visitor_2.visit(ast)

	deserialize_tmpl = Template(filename='./deserialize.mako')
	serialize_tmpl = Template(filename='./serialize.mako')
	out_file = open("./deserialize.c", "w")
	out_file.write(deserialize_tmpl.render(
		node_tree=struct_visitor.structs_dict,
		enums_list=typedef_visitor.enums_list,
		node_tags_refs=struct_visitor_2.nodes_structs,
		node_tags_structs=struct_visitor.node_tags_structs,
		node_tag_enums=typedef_visitor.node_tag_enums
	))
	out_file.close()
	out_file = open("./serialize.c", "w")
	out_file.write(serialize_tmpl.render(
		node_tree=struct_visitor.structs_dict,
		enums_list=typedef_visitor.enums_list,
		node_tags_refs=struct_visitor_2.nodes_structs,
		node_tags_structs=struct_visitor.node_tags_structs,
		node_tag_enums=typedef_visitor.node_tag_enums
	))
	out_file.close()
	#ast.show()
