#include "sr_plan.h"

static
JsonbValue *node_to_jsonb(const void *obj, JsonbParseState *state);

static Oid remove_fake_func = 0;
static bool skip_location = false;

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

<%def name="ser_key(var_name)">
	key.type = jbvString;
	key.val.string.len = strlen("${var_name}");
	key.val.string.val = strdup("${var_name}");
	pushJsonbValue(&state, WJB_KEY, &key);
</%def>

<%def name="ser_numeric(var_name, type_node, wjb_type='WJB_VALUE')">
	%if elog:
		elog(WARNING, "Start serailize numeric ${var_name}");
	%endif
	val.type = jbvNumeric;
	%if type_node["name"] == "long":
#ifdef USE_FLOAT8_BYVAL
	val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int8_numeric, Int64GetDatum(node->${var_name})));
#else
	val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, Int32GetDatum(node->${var_name})));
#endif
	%elif type_node["name"] in ["Oid", "Index", "uint32", "AclMode", "bits32"]:
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, UInt32GetDatum(node->${var_name})));
	%elif type_node["name"] in enum_likes_types:
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->${var_name})));
	%elif type_node["name"] in ["double", "Cost", "Selectivity"]:
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(float8_numeric, Float8GetDatum(node->${var_name})));
	%elif type_node["name"] in ["float"]:
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(float4_numeric, Float4GetDatum(node->${var_name})));
	%else:
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, Int32GetDatum(node->${var_name})));
	%endif
	pushJsonbValue(&state, ${wjb_type}, &val);
</%def>

<%def name="ser_node(var_name, type_node)">
	%if elog:
		elog(WARNING, "Start serailize node ${var_name}");
	%endif
	node_to_jsonb(node->${var_name}, state);
</%def>

<%def name="ser_direct_node(var_name, type_node)">
	%if elog:
		elog(WARNING, "Start serailize direct node ${var_name}");
	%endif
	${ser_key(var_name)}
	${type_node["name"]}_ser(&node->${var_name}, state, false);
</%def>

<%def name="ser_bool(var_name, type_node, wjb_type='WJB_VALUE')">
	%if elog:
		elog(WARNING, "Start serailize boolean ${var_name}");
	%endif
	val.type = jbvBool;
	val.val.boolean = node->${var_name};
	pushJsonbValue(&state, ${wjb_type}, &val);
</%def>

<%def name="ser_string(var_name, type_node, wjb_type='WJB_VALUE')">
	%if elog:
		elog(WARNING, "Start serailize boolean ${var_name}");
	%endif
	if (node->${var_name} == NULL)
	{
		val.type = jbvNull;
		pushJsonbValue(&state, ${wjb_type}, &val);
	}
	else
	{
		val.type = jbvString;
		val.val.string.len = strlen(node->${var_name});
		val.val.string.val = (char *)node->${var_name};
		pushJsonbValue(&state, ${wjb_type}, &val);
	}
</%def>

<%def name="ser_array(var_name, type_node, num_col='node->numCols')">
	%if elog:
		elog(WARNING, "Start serailize array ${var_name}");
	%endif
	${ser_key(var_name)}
	pushJsonbValue(&state, WJB_BEGIN_ARRAY, NULL);
	for (i = 0; i < ${num_col}; i++)
	{
		%if type_node["name"] == "bool":
			${ser_bool(var_name+"[i]", type_node, "WJB_ELEM")}
		%else:
			${ser_numeric(var_name+"[i]", type_node, "WJB_ELEM")}
		%endif
	}
	pushJsonbValue(&state, WJB_END_ARRAY, NULL);
</%def>

<%def name="ser_bitmapset(var_name, type_node)">
	%if elog:
		elog(WARNING, "Start serailize bitmapset ${var_name}");
	%endif
	${ser_key(var_name)}
	pushJsonbValue(&state, WJB_KEY, &key);
	
	if (node->${var_name} == NULL)
	{
		val.type = jbvNull;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}
	else
	{
		int x = -1;
		pushJsonbValue(&state, WJB_BEGIN_ARRAY, NULL);
		while ((x = bms_next_member(node->${var_name}, x)) >= 0)
		{
			val.type = jbvNumeric;
			val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, Int32GetDatum(x)));
			pushJsonbValue(&state, WJB_ELEM, &val);
		}
		pushJsonbValue(&state, WJB_END_ARRAY, NULL);
	}
</%def>

<%def name="ser_value(value_name, value_type)">
	if (${value_name} == NULL)
	{
		JsonbValue	val;
		val.type = jbvNull;
		pushJsonbValue(&state, ${value_type}, &val);
	}
	else if (IsA(${value_name}, String) || IsA(${value_name}, BitString) || IsA(${value_name}, Float) )
	{
		JsonbValue	val;
		val.type = jbvString;
		val.val.string.len = strlen(((Value *)${value_name})->val.str);
		val.val.string.val = ((Value *)${value_name})->val.str;
		pushJsonbValue(&state, ${value_type}, &val);
	}
	else if (IsA(${value_name}, Integer))
	{
		JsonbValue	val;
		val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, Int32GetDatum(((Value *)${value_name})->val.ival)));
		pushJsonbValue(&state, ${value_type}, &val);
	}
	else if (IsA(${value_name}, Null))
	{
		JsonbValue	val;
		val.type = jbvNull;
		pushJsonbValue(&state, ${value_type}, &val);
	}
</%def>

%for struct_name, struct in node_tree.items():
	static
	JsonbValue *${struct_name}_ser(const ${struct_name} *node, JsonbParseState *state, bool sub_object);
%endfor


static
JsonbValue *datum_ser(JsonbParseState *state, Datum value, int typlen, bool typbyval)
{
	Size		length,
				i;
	char	   *s;

	length = datumGetSize(value, typbyval, typlen);
	if (typbyval)
	{
		JsonbValue	val;
		s = (char *) (&value);
		pushJsonbValue(&state, WJB_BEGIN_ARRAY, NULL);
		for (i = 0; i < (Size) sizeof(Datum); i++)
		{
			val.type = jbvNumeric;
			val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum((int) (s[i]))));
			pushJsonbValue(&state, WJB_ELEM, &val);
		}
		return pushJsonbValue(&state, WJB_END_ARRAY, NULL);
	}
	else
	{
		JsonbValue	val;
		s = (char *) DatumGetPointer(value);
		if (!PointerIsValid(s))
		{
			val.type = jbvNull;
			return pushJsonbValue(&state, WJB_VALUE, &val);
		}
		else
		{
			pushJsonbValue(&state, WJB_BEGIN_ARRAY, NULL);
			for (i = 0; i < length; i++)
			{
				val.type = jbvNumeric;
				val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum((int) (s[i]))));
				pushJsonbValue(&state, WJB_ELEM, &val);
			}
			return pushJsonbValue(&state, WJB_END_ARRAY, NULL);
		}
	}
	
}

%for struct_name, struct in node_tree.items():
	static
	JsonbValue *${struct_name}_ser(const ${struct_name} *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		%if elog:
			elog(WARNING, "Start serailize struct ${struct_name}");
		%endif
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}

		%if write_type_node:
			{
				JsonbValue val;
				${ser_key("node_type")}
				val.type = jbvString;
				val.val.string.len = strlen("${struct_name}");
				val.val.string.val = strdup("${struct_name}");
				pushJsonbValue(&state, WJB_VALUE, &val);
			}
		%endif

		%for var_name, type_node in sorted(struct.items()):
			%if not type_node["pointer"] and type_node["name"] in numeric_types:
				%if var_name == "location":
					if(!skip_location)
				%endif
				{
					JsonbValue val;
					${ser_key(var_name)}
					${ser_numeric(var_name, type_node)}
				}
			%elif not type_node["pointer"] and type_node["name"] == "bool":
				{
					JsonbValue val;
					${ser_key(var_name)}
					${ser_bool(var_name, type_node)}
				}
			%elif type_node["pointer"] and type_node["name"] in node_types:
				%if struct_name == "FuncExpr" and var_name == "args":
					if (!remove_fake_func && remove_fake_func != ((FuncExpr *)node)->funcid) {
						${ser_key(var_name)}
						${ser_node(var_name, type_node)}
					}
				%else:
					${ser_key(var_name)}
					${ser_node(var_name, type_node)}
				%endif
			%elif not type_node["pointer"] and type_node["name"] in ["Plan", "Scan", "CreateStmt", "Join", "NodeTag", "Expr"]:
				${ser_direct_node(var_name, type_node)}
			%elif type_node["pointer"] and type_node["name"] == "char":
				{
					JsonbValue val;
					${ser_key(var_name)}
					${ser_string(var_name, type_node)}
				}
			%elif type_node["pointer"] and (type_node["name"] in numeric_types or type_node["name"] == "bool"):
				%if "numCols" in struct:
					{
						int i;
						JsonbValue val;
						${ser_array(var_name, type_node)}
					}
				%elif camel_split(var_name)[0]+"NumCols" in struct:
					{
						int i;
						JsonbValue val;
						${ser_array(var_name, type_node, "node->%s" % camel_split(var_name)[0]+"NumCols")}
					}
				%elif len([t for v, t in struct.items() if t["name"] == "List"]) == 1:
					{
						int i;
						JsonbValue val;
						${ser_array(
							var_name,
							type_node,
							"list_length(node->%s)" % [v for v, t in struct.items() if t["name"] == "List"][0]
						)}
					}
				%else:
					/* CAN'T SER ARRAY ${var_name} */
				%endif
				
			%elif type_node["pointer"] and type_node["name"] == "Bitmapset":
				{
					JsonbValue val;
					${ser_bitmapset(var_name, type_node)}
				}
			%elif not type_node["pointer"] and type_node["name"] == "Value":
				%if elog:
					elog(WARNING, "Start serailize Value ${var_name}");
				%endif
				${ser_key(var_name)}
				${ser_value("(&node->%s)" % var_name, "WJB_VALUE")}
			%elif not type_node["pointer"] and struct_name == "Const" and type_node["name"] == "Datum":
				%if elog:
					elog(WARNING, "Start serailize Datum ${var_name}");
				%endif
				${ser_key(var_name)}
				if (node->constisnull)
				{
					JsonbValue val;
					val.type = jbvNull;
					pushJsonbValue(&state, WJB_VALUE, &val);
				}
				else
					datum_ser(state, node->${var_name}, node->constlen, node->constbyval);
			%else:
				/* NOT FOUND TYPE: ${"*" if type_node["pointer"] else ""}${type_node["name"]} */
			%endif
		%endfor
		%if elog:
			elog(WARNING, "End serailize struct ${struct_name}");
		%endif
		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

%endfor

%for list_type_name in list_types:
	static
	JsonbValue *${list_type_name}_ser(const void *node, JsonbParseState *state)
	{
		const ListCell *lc;
		%if elog:
			elog(WARNING, "Start serailize ${list_type_name}");
		%endif
		pushJsonbValue(&state, WJB_BEGIN_ARRAY, NULL);

		foreach(lc, node)
		{
			%if list_type_name == "List":
				${ser_value("lfirst(lc)", "WJB_ELEM")}
				else
					node_to_jsonb(lfirst(lc), state);
			%elif list_type_name == "IntList":
				JsonbValue	val;
				val.type = jbvNumeric;
				val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, Int32GetDatum(lfirst_int(lc))));
				pushJsonbValue(&state, WJB_ELEM, &val);
			%elif list_type_name == "OidList":
				JsonbValue	val;
				val.type = jbvNumeric;
				val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, UInt32GetDatum(lfirst_int(lc))));
				pushJsonbValue(&state, WJB_ELEM, &val);
			%endif
		}

		return pushJsonbValue(&state, WJB_END_ARRAY, NULL);
	}
%endfor

static
JsonbValue *node_to_jsonb(const void *obj, JsonbParseState *state)
{
	if (obj == NULL) {
		JsonbValue out;
		out.type = jbvNull;
		return pushJsonbValue(&state, WJB_VALUE, &out);
	}
	%for list_type_name in list_types:
		else if (IsA(obj, ${list_type_name}))
		{
			return ${list_type_name}_ser(obj, state);
		}
	%endfor

	switch (nodeTag(obj))
	{
	%for struct_name, struct in node_tree.items():
		case T_${struct_name}:
			return ${struct_name}_ser(obj, state, true);
	%endfor
		case T_SeqScan:
			return Scan_ser(obj, state, true);
##		case T_SampleScan:
##			return Scan_ser(obj, state, true);
		case T_DistinctExpr:
			return OpExpr_ser(obj, state, true);
		case T_NullIfExpr:
			return OpExpr_ser(obj, state, true);
		default:
			/*
			 * This should be an ERROR, but it's too useful to be able to
			 * dump structures that _outNode only understands part of.
			 */
			elog(WARNING, "could not dump unrecognized node type: %d",
				 (int) nodeTag(obj));
			break;
	}
	return NULL;
}

Jsonb *node_tree_to_jsonb(const void *obj, Oid fake_func, bool skip_location_from_node)
{
	Jsonb *tmp;
	remove_fake_func = fake_func;
	skip_location = skip_location_from_node;
	tmp = JsonbValueToJsonb(node_to_jsonb(obj, NULL));
	remove_fake_func = 0;
	return tmp;
}