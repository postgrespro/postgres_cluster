#include "sr_plan.h"

static
void *jsonb_to_node(JsonbContainer *container);

static List *
list_deser(JsonbContainer *container, bool oid);

typedef int (*myFuncDef)(int, int);
static void *(*hook) (void *);

<%
	elog = False
	write_type_node = False
	list_types = ["List", "IntList", "OidList"]
	enum_likes_types = ["AttrNumber", "char"] + enums_list+["int16"]
	numeric_types = [
		"Oid", "int32", "uint32",
		"int", "long", "Index",
		"AclMode", "double", "Cost",
		"Selectivity", "float", "int16",
		"bits32"
	]
	numeric_types += enum_likes_types

	node_types = node_tags_refs + node_tags_structs
	
	def camel_split(s):
		return (''.join(map(lambda x: x if x.islower() else " "+x, s))).split()
%>

<%def name="make_key(var_name, key)">
	JsonbValue ${var_name};
	${var_name}.type = jbvString;
	${var_name}.val.string.len = strlen("${key}");
	${var_name}.val.string.val = strdup("${key}");
</%def>

<%def name="deser_node(var_name, type_node)">
	%if elog:
		elog(WARNING, "Start deserailize node ${var_name}");
	%endif
	if (var_value->type == jbvNull) 
	{
		local_node->${var_name} = NULL;
	} else {
		local_node->${var_name} = (${type_node["name"]} *) jsonb_to_node(var_value->val.binary.data);
	}
	
</%def>


<%def name="make_key_value(var_name)">
	JsonbValue *var_value;
	${make_key("var_key", var_name)}
	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);
</%def>

static List *
list_deser(JsonbContainer *container, bool oid)
{
	int type;
	JsonbValue v;
	JsonbIterator *it;
	List *l = NIL;
	it = JsonbIteratorInit(container);
	
	while ((type = JsonbIteratorNext(&it, &v, true)) != WJB_DONE)
	{
		if (type == WJB_ELEM)
		{
			switch(v.type)
			{
				case jbvNumeric:
					if (oid)
						l = lappend_oid(l, DatumGetUInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(v.val.numeric))));
					else
						l = lappend_int(l, DatumGetInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(v.val.numeric))));
					break;
				case jbvString:
					{
						char *result = palloc(v.val.string.len + 1);
						result[v.val.string.len] = '\0';
						memcpy(result, v.val.string.val, v.val.string.len);
						l = lappend(l, makeString(result));
					}
					break;
				case jbvNull:
					l = lappend(l, NULL);
					break;
				default:
					l = lappend(l, jsonb_to_node(v.val.binary.data));
			}
		}
	}
	return l;
}

<%def name="deser_list(var_name, type_node)">
	%if elog:
		elog(WARNING, "Start deserailize list ${var_name}");
	%endif

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->${var_name} = NULL;
	else
		local_node->${var_name} = list_deser(var_value->val.binary.data, ${"true" if "Oid" in var_name or var_name == "arbiterIndexes" else "false"});
</%def>

<%def name="deser_array(var_name, type_node, num_col='local_node->numCols')">
	%if elog:
		elog(WARNING, "Start deserailize array ${var_name}");
	%endif
	{
		int type;
		int i = 0;
		JsonbValue v;
		JsonbIterator *it = JsonbIteratorInit(var_value->val.binary.data);
		%if num_col:
			${num_col} = it->nElems;
		%endif
		local_node->${var_name} = (${type_node["name"]}*) palloc(sizeof(${type_node["name"]})*it->nElems);
		while ((type = JsonbIteratorNext(&it, &v, true)) != WJB_DONE)
		{
			if (type == WJB_ELEM)
			{
				%if type_node["name"] == "bool":
					${deser_bool(var_name+"[i]", type_node, "v.")}
				%else:
					${deser_numeric(var_name+"[i]", type_node, "v.")}
				%endif
				i++;
			}
		}
	}
</%def>

<%def name="deser_bool(var_name, type_node, value_name='var_value->')">
	%if elog:
		elog(WARNING, "Start deserailize bool ${var_name}");
	%endif
	local_node->${var_name} = ${value_name}val.boolean;
</%def>

<%def name="deser_numeric(var_name, type_node, value_name='var_value->')">
	%if elog:
		elog(WARNING, "Start deserailize numeric ${var_name}");
	%endif
	%if type_node["name"] == "long":
#ifdef USE_FLOAT8_BYVAL
	local_node->${var_name} = DatumGetInt64(DirectFunctionCall1(numeric_int8, NumericGetDatum(${value_name}val.numeric)));
#else
	local_node->${var_name} = DatumGetInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(${value_name}val.numeric)));
#endif
	%elif type_node["name"] in ["Oid", "Index", "uint32", "AclMode", "bits32"]:
		local_node->${var_name} = DatumGetUInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(${value_name}val.numeric)));
	%elif type_node["name"] in enum_likes_types:
		local_node->${var_name} = DatumGetInt16(DirectFunctionCall1(numeric_int2, NumericGetDatum(${value_name}val.numeric)));
	%elif type_node["name"] in ["double", "Cost", "Selectivity"]:
		local_node->${var_name} = DatumGetFloat8(DirectFunctionCall1(numeric_float8, NumericGetDatum(${value_name}val.numeric)));
	%elif type_node["name"] in ["float"]:
		local_node->${var_name} = DatumGetFloat4(DirectFunctionCall1(numeric_float4, NumericGetDatum(${value_name}val.numeric)));
	%else:
		local_node->${var_name} = DatumGetInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(${value_name}val.numeric)));
	%endif
</%def>

<%def name="deser_direct_node(var_name, type_node)">
	%if elog:
		elog(WARNING, "Start deserailize direct node ${var_name}");
	%endif
	${type_node["name"]}_deser(container, (void *)&local_node->${var_name}, -1);
</%def>

<%def name="deser_bitmapset(var_name, type_node)">
	{
		Bitmapset  *result = NULL;
		JsonbValue v;
		int type;
		JsonbIterator *it = JsonbIteratorInit(var_value->val.binary.data);
		%if elog:
			elog(WARNING, "Start deserailize bitmapset ${var_name}");
		%endif
		while ((type = JsonbIteratorNext(&it, &v, true)) != WJB_DONE)
		{
			if (type == WJB_ELEM)
				result = bms_add_member(result,
										DatumGetUInt32(DirectFunctionCall1(numeric_int4,
																		   NumericGetDatum(v.val.numeric))));
		}
		local_node->${var_name} = result;
	}
	
</%def>

%for struct_name, struct in node_tree.items():
	static
	void *${struct_name}_deser(JsonbContainer *container, void *node_cast, int replace_type);
%endfor

static Datum
datum_deser(JsonbValue *var_value, bool typbyval)
{
	Datum res;
	char *s;
	int i = 0;
	int type;
	JsonbValue v;

	JsonbIterator *it = JsonbIteratorInit(var_value->val.binary.data);

	if (typbyval)
	{
		if (it->nElems > (Size) sizeof(Datum))
			elog(ERROR, "byval datum but length = %d", it->nElems);
		res = (Datum) 0;
		s = (char *) (&res);
		while ((type = JsonbIteratorNext(&it, &v, true)) != WJB_DONE)
		{
			if (type == WJB_ELEM)
			{
				if (i >= (Size) sizeof(Datum))
					break;

				s[i] = (char) DatumGetInt16(DirectFunctionCall1(numeric_int2, NumericGetDatum(v.val.numeric)));
				i++;
			}
		}
	}
	else if (it->nElems <= 0)
	{
		res = (Datum) NULL;
	}
	else
	{
		s = (char *) palloc(it->nElems);
		while ((type = JsonbIteratorNext(&it, &v, true)) != WJB_DONE)
		{
			if (type == WJB_ELEM)
			{
				if (i >= it->nElems)
					break;

				s[i] = (char) DatumGetInt16(DirectFunctionCall1(numeric_int2, NumericGetDatum(v.val.numeric)));
				i++;
			}
		}
		res = PointerGetDatum(s);
		
	}

	return res;
}


%for struct_name, struct in node_tree.items():
	static
	void *${struct_name}_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		${struct_name} *local_node;
		%if elog:
			elog(WARNING, "Start deserailize struct ${struct_name}");
		%endif
		if (node_cast != NULL)
			local_node = (${struct_name} *) node_cast;
		else
			local_node = makeNode(${struct_name});

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

		%for var_name, type_node in struct.items():
			%if var_name != "type":
			{
				%if not type_node["pointer"] and type_node["name"] in numeric_types:
					${make_key_value(var_name)}
					${deser_numeric(var_name, type_node)}
				%elif not type_node["pointer"] and type_node["name"] == "bool":
					${make_key_value(var_name)}
					${deser_bool(var_name, type_node)}
				%elif type_node["pointer"] and type_node["name"] == "List":
					${make_key_value(var_name)}
					${deser_list(var_name, type_node)}
				%elif type_node["pointer"] and type_node["name"] in node_types:
					${make_key_value(var_name)}
					${deser_node(var_name, type_node)}
				%elif type_node["pointer"] and type_node["name"] == "char":
					${make_key_value(var_name)}
					if (var_value->type == jbvNull)
						local_node->${var_name} = NULL;
					else
					{
						char *result = palloc(var_value->val.string.len + 1);
						memcpy(result, var_value->val.string.val, var_value->val.string.len);
						result[var_value->val.string.len] = '\0';
						local_node->${var_name} = result;
					}
				%elif not type_node["pointer"] and type_node["name"] in ["Plan", "Scan", "CreateStmt", "Join", "NodeTag", "Expr"]:
					${deser_direct_node(var_name, type_node)}
				%elif type_node["pointer"] and (type_node["name"] in numeric_types or type_node["name"] == "bool"):
					${make_key_value(var_name)}
					%if "numCols" in struct:
						${deser_array(var_name, type_node)}
					%elif camel_split(var_name)[0]+"NumCols" in struct:
						${deser_array(var_name, type_node, "local_node->%s" % camel_split(var_name)[0]+"NumCols")}
					%else:
						${deser_array(var_name, type_node, NULL)}
					%endif
				%elif type_node["pointer"] and type_node["name"] == "Bitmapset":
					${make_key_value(var_name)}
					if (var_value->type == jbvNull)
						local_node->${var_name} = NULL;
					else
						${deser_bitmapset(var_name, type_node)}
				%elif not type_node["pointer"] and type_node["name"] == "Value":
					${make_key_value(var_name)}
					if (var_value->type == jbvString) {
						char *result = palloc(var_value->val.string.len + 1);
						result[var_value->val.string.len] = '\0';
						memcpy(result, var_value->val.string.val, var_value->val.string.len);
						local_node->${var_name} = *makeString(result);
					} else if (var_value->type == jbvNumeric) {
						local_node->${var_name} = *makeInteger(DatumGetInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric))));
					}
					##${deser_value("(&node->%s)" % var_name, "WJB_VALUE")}
				%elif not type_node["pointer"] and struct_name == "Const" and type_node["name"] == "Datum":
					${make_key_value(var_name)}
					if (var_value->type == jbvNull)
						local_node->${var_name} = (Datum) NULL;
					else
					{
						JsonbValue *typbyval_value;
						${make_key("typbyval_key", "constbyval")}
						
						typbyval_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&typbyval_key);
						
						local_node->${var_name} = datum_deser(var_value, typbyval_value->val.boolean);
					}
				%else:
					/* NOT FOUND TYPE: ${"*" if type_node["pointer"] else ""}${type_node["name"]} */
				%endif
				##(JsonbContainer *) jbvp->val.binary.data
			}
			%endif
		%endfor
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
%endfor


static
void *jsonb_to_node(JsonbContainer *container)
{
	JsonbValue *node_type;
	int16 node_type_value;
	
	${make_key("node_type_key", "type")}

	if (container == NULL)
	{
		return NULL;
	}
	else if ((container->header & JB_CMASK) == 0)
	{
		return NULL;
	}
	else if ((container->header & (JB_FARRAY | JB_FOBJECT)) == JB_FARRAY)
	{
		return list_deser(container, false);
	}

	node_type = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&node_type_key);
	node_type_value = DatumGetInt16(DirectFunctionCall1(numeric_int2, NumericGetDatum(node_type->val.numeric)));
	##it = JsonbIteratorInit(&json->root);
	##numeric_int4

	switch (node_type_value)
	{
	%for struct_name, struct in node_tree.items():
		case T_${struct_name}:
			return ${struct_name}_deser(container, NULL, -1);
	%endfor
		case T_SeqScan:
			return Scan_deser(container, NULL, T_SeqScan);
##		case T_SampleScan:
##			return Scan_deser(container, NULL, T_SampleScan);
		case T_DistinctExpr:
			return OpExpr_deser(container, NULL, T_DistinctExpr);
		case T_NullIfExpr:
			return OpExpr_deser(container, NULL, T_NullIfExpr);
	}
	elog(WARNING, "could not read unrecognized node type:%d", node_type_value);
	return NULL;
}

void *jsonb_to_node_tree(Jsonb *json, void *(*hookPtr) (void *))
{
	void *node;
	hook = hookPtr;
	node = jsonb_to_node(&json->root);
	hook = NULL;
	return node;
}