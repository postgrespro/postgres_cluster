#include "sr_plan.h"

static
void *jsonb_to_node(JsonbContainer *container);

static List *
list_deser(JsonbContainer *container, bool oid);

typedef int (*myFuncDef)(int, int);
static void *(*hook) (void *);










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













	static
	void *RangeTableSample_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *CreateEnumStmt_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *SampleScan_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *SetOperationStmt_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *AlterTSDictionaryStmt_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *SortGroupClause_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *NamedArgExpr_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *CaseWhen_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *CommentStmt_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *VacuumStmt_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *AlterOwnerStmt_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *Unique_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *MinMaxExpr_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *TransactionStmt_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *CreateEventTrigStmt_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *ArrayRef_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *AlterExtensionStmt_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *CreateRoleStmt_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *AlterOpFamilyStmt_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *LockStmt_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *AlterTableStmt_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *CreateSchemaStmt_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *ClosePortalStmt_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *RelabelType_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *FunctionScan_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *AlterSeqStmt_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *SubPlan_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *ScalarArrayOpExpr_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *RoleSpec_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *InlineCodeBlock_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *VariableShowStmt_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *ImportForeignSchemaStmt_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *AlterForeignServerStmt_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *ModifyTable_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *CheckPointStmt_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *RangeVar_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *AlterExtensionContentsStmt_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *RangeTblRef_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *CreateOpClassStmt_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *LoadStmt_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *ColumnRef_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *CreateTrigStmt_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *ConstraintsSetStmt_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *Var_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *BitmapIndexScan_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *Plan_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *CreateStmt_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *InferClause_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *Param_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *ExecuteStmt_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *DropdbStmt_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *FetchStmt_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *HashJoin_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *ColumnDef_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *AlterRoleSetStmt_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *Result_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *PrepareStmt_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *AlterDomainStmt_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *CompositeTypeStmt_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *CustomScan_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *Agg_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *AlterObjectSchemaStmt_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *AlterEventTrigStmt_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *AlterDefaultPrivilegesStmt_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *PlannedStmt_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *Aggref_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *SubqueryScan_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *CreateFunctionStmt_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *CreateCastStmt_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *CteScan_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *IndexElem_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *CreateFdwStmt_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *NestLoop_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *TypeCast_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *CoerceToDomainValue_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *InsertStmt_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *SortBy_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *ReassignOwnedStmt_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *TypeName_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *Constraint_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *OnConflictClause_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *AlterPolicyStmt_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *GroupingFunc_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *SelectStmt_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *CopyStmt_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *ArrayExpr_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *InferenceElem_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *BitmapOr_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *FuncExpr_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *AlterUserMappingStmt_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *SetOp_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *AlterTSConfigurationStmt_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *AlterRoleStmt_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *Sort_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *DiscardStmt_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *CreateForeignServerStmt_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *CaseExpr_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *CreateExtensionStmt_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *CommonTableExpr_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *RenameStmt_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *A_Star_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *UnlistenStmt_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *GroupingSet_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *BoolExpr_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *BitmapAnd_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *RowExpr_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *UpdateStmt_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *CollateExpr_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *DropTableSpaceStmt_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *DeallocateStmt_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *CreatedbStmt_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *Hash_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *CurrentOfExpr_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *WindowDef_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *AlterFunctionStmt_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *ExplainStmt_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *AlterDatabaseStmt_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *ParamRef_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *CreateForeignTableStmt_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *BitmapHeapScan_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *Scan_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *AlterTableSpaceOptionsStmt_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *FuncCall_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *Join_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *ReindexStmt_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *WithClause_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *RangeSubselect_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *CreatePLangStmt_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *Const_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *XmlExpr_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *FuncWithArgs_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *CollateClause_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *CreateUserMappingStmt_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *ListenStmt_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *RefreshMatViewStmt_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *DeclareCursorStmt_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *CreateConversionStmt_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *Value_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *DropStmt_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *Limit_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *IntoClause_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *AlternativeSubPlan_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *ForeignScan_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *A_Const_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *AlterSystemStmt_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *ResTarget_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *RangeTblEntry_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *A_Indirection_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *Append_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *TableSampleClause_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *BooleanTest_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *ArrayCoerceExpr_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *WithCheckOption_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *RowCompareExpr_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *MultiAssignRef_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *TruncateStmt_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *CoerceToDomain_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *WindowClause_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *DefElem_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *DropUserMappingStmt_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *A_ArrayExpr_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *CreateTableAsStmt_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *OpExpr_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *FieldStore_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *CoerceViaIO_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *SubLink_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *WorkTableScan_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *A_Expr_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *DropOwnedStmt_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *AlterDatabaseSetStmt_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *CreateRangeStmt_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *DeleteStmt_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *CreateSeqStmt_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *RuleStmt_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *RangeTblFunction_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *IndexStmt_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *WindowAgg_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *ConvertRowtypeExpr_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *MergeAppend_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *AccessPriv_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *CreateOpFamilyStmt_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *CoalesceExpr_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *DoStmt_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *IndexOnlyScan_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *XmlSerialize_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *FunctionParameter_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *SetToDefault_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *JoinExpr_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *NullTest_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *NotifyStmt_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *AlterTableMoveAllStmt_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *LockingClause_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *DefineStmt_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *CreateOpClassItem_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *ClusterStmt_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *MergeJoin_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *SecLabelStmt_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *CaseTestExpr_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *TidScan_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *Query_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *ReplicaIdentityStmt_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *IndexScan_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *ValuesScan_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *FieldSelect_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *OnConflictExpr_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *VariableSetStmt_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *TargetEntry_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *LockRows_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *CreateDomainStmt_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *RowMarkClause_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *NestLoopParam_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *WindowFunc_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *TableLikeClause_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *Expr_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *RecursiveUnion_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *FromExpr_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *Alias_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *AlterEnumStmt_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *RangeFunction_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *CreateTransformStmt_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *AlterTableCmd_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *GrantStmt_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *CreateTableSpaceStmt_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *CreatePolicyStmt_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *GrantRoleStmt_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *DropRoleStmt_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *ViewStmt_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *A_Indices_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *PlanInvalItem_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *Group_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *PlanRowMark_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *AlterFdwStmt_deser(JsonbContainer *container, void *node_cast, int replace_type);
	static
	void *Material_deser(JsonbContainer *container, void *node_cast, int replace_type);

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


	static
	void *RangeTableSample_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		RangeTableSample *local_node;
		if (node_cast != NULL)
			local_node = (RangeTableSample *) node_cast;
		else
			local_node = makeNode(RangeTableSample);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("args");
	var_key.val.string.val = strdup("args");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->args = NULL;
	else
		local_node->args = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("method");
	var_key.val.string.val = strdup("method");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->method = NULL;
	else
		local_node->method = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("repeatable");
	var_key.val.string.val = strdup("repeatable");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->repeatable = NULL;
	} else {
		local_node->repeatable = (Node *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("relation");
	var_key.val.string.val = strdup("relation");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->relation = NULL;
	} else {
		local_node->relation = (Node *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("location");
	var_key.val.string.val = strdup("location");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->location = DatumGetInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *CreateEnumStmt_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		CreateEnumStmt *local_node;
		if (node_cast != NULL)
			local_node = (CreateEnumStmt *) node_cast;
		else
			local_node = makeNode(CreateEnumStmt);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("vals");
	var_key.val.string.val = strdup("vals");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->vals = NULL;
	else
		local_node->vals = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("typeName");
	var_key.val.string.val = strdup("typeName");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->typeName = NULL;
	else
		local_node->typeName = list_deser(var_value->val.binary.data, false);

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *SampleScan_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		SampleScan *local_node;
		if (node_cast != NULL)
			local_node = (SampleScan *) node_cast;
		else
			local_node = makeNode(SampleScan);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	Scan_deser(container, (void *)&local_node->scan, -1);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("tablesample");
	var_key.val.string.val = strdup("tablesample");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->tablesample = NULL;
	} else {
		local_node->tablesample = (TableSampleClause *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *SetOperationStmt_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		SetOperationStmt *local_node;
		if (node_cast != NULL)
			local_node = (SetOperationStmt *) node_cast;
		else
			local_node = makeNode(SetOperationStmt);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("rarg");
	var_key.val.string.val = strdup("rarg");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->rarg = NULL;
	} else {
		local_node->rarg = (Node *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("larg");
	var_key.val.string.val = strdup("larg");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->larg = NULL;
	} else {
		local_node->larg = (Node *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("colCollations");
	var_key.val.string.val = strdup("colCollations");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->colCollations = NULL;
	else
		local_node->colCollations = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("all");
	var_key.val.string.val = strdup("all");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	local_node->all = var_value->val.boolean;

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("op");
	var_key.val.string.val = strdup("op");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->op = DatumGetInt16(DirectFunctionCall1(numeric_int2, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("groupClauses");
	var_key.val.string.val = strdup("groupClauses");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->groupClauses = NULL;
	else
		local_node->groupClauses = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("colTypes");
	var_key.val.string.val = strdup("colTypes");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->colTypes = NULL;
	else
		local_node->colTypes = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("colTypmods");
	var_key.val.string.val = strdup("colTypmods");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->colTypmods = NULL;
	else
		local_node->colTypmods = list_deser(var_value->val.binary.data, false);

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *AlterTSDictionaryStmt_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		AlterTSDictionaryStmt *local_node;
		if (node_cast != NULL)
			local_node = (AlterTSDictionaryStmt *) node_cast;
		else
			local_node = makeNode(AlterTSDictionaryStmt);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("options");
	var_key.val.string.val = strdup("options");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->options = NULL;
	else
		local_node->options = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("dictname");
	var_key.val.string.val = strdup("dictname");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->dictname = NULL;
	else
		local_node->dictname = list_deser(var_value->val.binary.data, false);

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *SortGroupClause_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		SortGroupClause *local_node;
		if (node_cast != NULL)
			local_node = (SortGroupClause *) node_cast;
		else
			local_node = makeNode(SortGroupClause);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("sortop");
	var_key.val.string.val = strdup("sortop");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->sortop = DatumGetUInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("eqop");
	var_key.val.string.val = strdup("eqop");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->eqop = DatumGetUInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("hashable");
	var_key.val.string.val = strdup("hashable");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	local_node->hashable = var_value->val.boolean;

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("tleSortGroupRef");
	var_key.val.string.val = strdup("tleSortGroupRef");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->tleSortGroupRef = DatumGetUInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("nulls_first");
	var_key.val.string.val = strdup("nulls_first");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	local_node->nulls_first = var_value->val.boolean;

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *NamedArgExpr_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		NamedArgExpr *local_node;
		if (node_cast != NULL)
			local_node = (NamedArgExpr *) node_cast;
		else
			local_node = makeNode(NamedArgExpr);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("name");
	var_key.val.string.val = strdup("name");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					if (var_value->type == jbvNull)
						local_node->name = NULL;
					else
					{
						char *result = palloc(var_value->val.string.len + 1);
						memcpy(result, var_value->val.string.val, var_value->val.string.len);
						result[var_value->val.string.len] = '\0';
						local_node->name = result;
					}
			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("argnumber");
	var_key.val.string.val = strdup("argnumber");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->argnumber = DatumGetInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("location");
	var_key.val.string.val = strdup("location");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->location = DatumGetInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("arg");
	var_key.val.string.val = strdup("arg");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->arg = NULL;
	} else {
		local_node->arg = (Expr *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
			{
					
	Expr_deser(container, (void *)&local_node->xpr, -1);

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *CaseWhen_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		CaseWhen *local_node;
		if (node_cast != NULL)
			local_node = (CaseWhen *) node_cast;
		else
			local_node = makeNode(CaseWhen);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("location");
	var_key.val.string.val = strdup("location");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->location = DatumGetInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("result");
	var_key.val.string.val = strdup("result");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->result = NULL;
	} else {
		local_node->result = (Expr *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
			{
					
	Expr_deser(container, (void *)&local_node->xpr, -1);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("expr");
	var_key.val.string.val = strdup("expr");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->expr = NULL;
	} else {
		local_node->expr = (Expr *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *CommentStmt_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		CommentStmt *local_node;
		if (node_cast != NULL)
			local_node = (CommentStmt *) node_cast;
		else
			local_node = makeNode(CommentStmt);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("objtype");
	var_key.val.string.val = strdup("objtype");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->objtype = DatumGetInt16(DirectFunctionCall1(numeric_int2, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("comment");
	var_key.val.string.val = strdup("comment");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					if (var_value->type == jbvNull)
						local_node->comment = NULL;
					else
					{
						char *result = palloc(var_value->val.string.len + 1);
						memcpy(result, var_value->val.string.val, var_value->val.string.len);
						result[var_value->val.string.len] = '\0';
						local_node->comment = result;
					}
			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("objargs");
	var_key.val.string.val = strdup("objargs");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->objargs = NULL;
	else
		local_node->objargs = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("objname");
	var_key.val.string.val = strdup("objname");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->objname = NULL;
	else
		local_node->objname = list_deser(var_value->val.binary.data, false);

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *VacuumStmt_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		VacuumStmt *local_node;
		if (node_cast != NULL)
			local_node = (VacuumStmt *) node_cast;
		else
			local_node = makeNode(VacuumStmt);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("va_cols");
	var_key.val.string.val = strdup("va_cols");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->va_cols = NULL;
	else
		local_node->va_cols = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("relation");
	var_key.val.string.val = strdup("relation");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->relation = NULL;
	} else {
		local_node->relation = (RangeVar *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("options");
	var_key.val.string.val = strdup("options");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->options = DatumGetInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *AlterOwnerStmt_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		AlterOwnerStmt *local_node;
		if (node_cast != NULL)
			local_node = (AlterOwnerStmt *) node_cast;
		else
			local_node = makeNode(AlterOwnerStmt);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("newowner");
	var_key.val.string.val = strdup("newowner");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->newowner = NULL;
	} else {
		local_node->newowner = (Node *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("objarg");
	var_key.val.string.val = strdup("objarg");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->objarg = NULL;
	else
		local_node->objarg = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("objectType");
	var_key.val.string.val = strdup("objectType");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->objectType = DatumGetInt16(DirectFunctionCall1(numeric_int2, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("relation");
	var_key.val.string.val = strdup("relation");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->relation = NULL;
	} else {
		local_node->relation = (RangeVar *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("object");
	var_key.val.string.val = strdup("object");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->object = NULL;
	else
		local_node->object = list_deser(var_value->val.binary.data, false);

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *Unique_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		Unique *local_node;
		if (node_cast != NULL)
			local_node = (Unique *) node_cast;
		else
			local_node = makeNode(Unique);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	Plan_deser(container, (void *)&local_node->plan, -1);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("numCols");
	var_key.val.string.val = strdup("numCols");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->numCols = DatumGetInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("uniqOperators");
	var_key.val.string.val = strdup("uniqOperators");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

						
	{
		int type;
		int i = 0;
		JsonbValue v;
		JsonbIterator *it = JsonbIteratorInit(var_value->val.binary.data);
			local_node->numCols = it->nElems;
		local_node->uniqOperators = (Oid*) palloc(sizeof(Oid)*it->nElems);
		while ((type = JsonbIteratorNext(&it, &v, true)) != WJB_DONE)
		{
			if (type == WJB_ELEM)
			{
					
		local_node->uniqOperators[i] = DatumGetUInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(v.val.numeric)));

				i++;
			}
		}
	}

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("uniqColIdx");
	var_key.val.string.val = strdup("uniqColIdx");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

						
	{
		int type;
		int i = 0;
		JsonbValue v;
		JsonbIterator *it = JsonbIteratorInit(var_value->val.binary.data);
			local_node->numCols = it->nElems;
		local_node->uniqColIdx = (AttrNumber*) palloc(sizeof(AttrNumber)*it->nElems);
		while ((type = JsonbIteratorNext(&it, &v, true)) != WJB_DONE)
		{
			if (type == WJB_ELEM)
			{
					
		local_node->uniqColIdx[i] = DatumGetInt16(DirectFunctionCall1(numeric_int2, NumericGetDatum(v.val.numeric)));

				i++;
			}
		}
	}

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *MinMaxExpr_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		MinMaxExpr *local_node;
		if (node_cast != NULL)
			local_node = (MinMaxExpr *) node_cast;
		else
			local_node = makeNode(MinMaxExpr);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("minmaxcollid");
	var_key.val.string.val = strdup("minmaxcollid");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->minmaxcollid = DatumGetUInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("args");
	var_key.val.string.val = strdup("args");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->args = NULL;
	else
		local_node->args = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("minmaxtype");
	var_key.val.string.val = strdup("minmaxtype");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->minmaxtype = DatumGetUInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("inputcollid");
	var_key.val.string.val = strdup("inputcollid");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->inputcollid = DatumGetUInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("op");
	var_key.val.string.val = strdup("op");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->op = DatumGetInt16(DirectFunctionCall1(numeric_int2, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("location");
	var_key.val.string.val = strdup("location");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->location = DatumGetInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	Expr_deser(container, (void *)&local_node->xpr, -1);

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *TransactionStmt_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		TransactionStmt *local_node;
		if (node_cast != NULL)
			local_node = (TransactionStmt *) node_cast;
		else
			local_node = makeNode(TransactionStmt);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("options");
	var_key.val.string.val = strdup("options");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->options = NULL;
	else
		local_node->options = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("kind");
	var_key.val.string.val = strdup("kind");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->kind = DatumGetInt16(DirectFunctionCall1(numeric_int2, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("gid");
	var_key.val.string.val = strdup("gid");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					if (var_value->type == jbvNull)
						local_node->gid = NULL;
					else
					{
						char *result = palloc(var_value->val.string.len + 1);
						memcpy(result, var_value->val.string.val, var_value->val.string.len);
						result[var_value->val.string.len] = '\0';
						local_node->gid = result;
					}
			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *CreateEventTrigStmt_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		CreateEventTrigStmt *local_node;
		if (node_cast != NULL)
			local_node = (CreateEventTrigStmt *) node_cast;
		else
			local_node = makeNode(CreateEventTrigStmt);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("funcname");
	var_key.val.string.val = strdup("funcname");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->funcname = NULL;
	else
		local_node->funcname = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("whenclause");
	var_key.val.string.val = strdup("whenclause");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->whenclause = NULL;
	else
		local_node->whenclause = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("trigname");
	var_key.val.string.val = strdup("trigname");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					if (var_value->type == jbvNull)
						local_node->trigname = NULL;
					else
					{
						char *result = palloc(var_value->val.string.len + 1);
						memcpy(result, var_value->val.string.val, var_value->val.string.len);
						result[var_value->val.string.len] = '\0';
						local_node->trigname = result;
					}
			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("eventname");
	var_key.val.string.val = strdup("eventname");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					if (var_value->type == jbvNull)
						local_node->eventname = NULL;
					else
					{
						char *result = palloc(var_value->val.string.len + 1);
						memcpy(result, var_value->val.string.val, var_value->val.string.len);
						result[var_value->val.string.len] = '\0';
						local_node->eventname = result;
					}
			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *ArrayRef_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		ArrayRef *local_node;
		if (node_cast != NULL)
			local_node = (ArrayRef *) node_cast;
		else
			local_node = makeNode(ArrayRef);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("reflowerindexpr");
	var_key.val.string.val = strdup("reflowerindexpr");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->reflowerindexpr = NULL;
	else
		local_node->reflowerindexpr = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("refarraytype");
	var_key.val.string.val = strdup("refarraytype");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->refarraytype = DatumGetUInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	Expr_deser(container, (void *)&local_node->xpr, -1);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("refassgnexpr");
	var_key.val.string.val = strdup("refassgnexpr");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->refassgnexpr = NULL;
	} else {
		local_node->refassgnexpr = (Expr *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("refexpr");
	var_key.val.string.val = strdup("refexpr");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->refexpr = NULL;
	} else {
		local_node->refexpr = (Expr *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("reftypmod");
	var_key.val.string.val = strdup("reftypmod");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->reftypmod = DatumGetInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("refelemtype");
	var_key.val.string.val = strdup("refelemtype");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->refelemtype = DatumGetUInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("refcollid");
	var_key.val.string.val = strdup("refcollid");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->refcollid = DatumGetUInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("refupperindexpr");
	var_key.val.string.val = strdup("refupperindexpr");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->refupperindexpr = NULL;
	else
		local_node->refupperindexpr = list_deser(var_value->val.binary.data, false);

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *AlterExtensionStmt_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		AlterExtensionStmt *local_node;
		if (node_cast != NULL)
			local_node = (AlterExtensionStmt *) node_cast;
		else
			local_node = makeNode(AlterExtensionStmt);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("options");
	var_key.val.string.val = strdup("options");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->options = NULL;
	else
		local_node->options = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("extname");
	var_key.val.string.val = strdup("extname");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					if (var_value->type == jbvNull)
						local_node->extname = NULL;
					else
					{
						char *result = palloc(var_value->val.string.len + 1);
						memcpy(result, var_value->val.string.val, var_value->val.string.len);
						result[var_value->val.string.len] = '\0';
						local_node->extname = result;
					}
			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *CreateRoleStmt_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		CreateRoleStmt *local_node;
		if (node_cast != NULL)
			local_node = (CreateRoleStmt *) node_cast;
		else
			local_node = makeNode(CreateRoleStmt);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("options");
	var_key.val.string.val = strdup("options");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->options = NULL;
	else
		local_node->options = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("role");
	var_key.val.string.val = strdup("role");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					if (var_value->type == jbvNull)
						local_node->role = NULL;
					else
					{
						char *result = palloc(var_value->val.string.len + 1);
						memcpy(result, var_value->val.string.val, var_value->val.string.len);
						result[var_value->val.string.len] = '\0';
						local_node->role = result;
					}
			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("stmt_type");
	var_key.val.string.val = strdup("stmt_type");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->stmt_type = DatumGetInt16(DirectFunctionCall1(numeric_int2, NumericGetDatum(var_value->val.numeric)));

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *AlterOpFamilyStmt_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		AlterOpFamilyStmt *local_node;
		if (node_cast != NULL)
			local_node = (AlterOpFamilyStmt *) node_cast;
		else
			local_node = makeNode(AlterOpFamilyStmt);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("opfamilyname");
	var_key.val.string.val = strdup("opfamilyname");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->opfamilyname = NULL;
	else
		local_node->opfamilyname = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("items");
	var_key.val.string.val = strdup("items");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->items = NULL;
	else
		local_node->items = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("amname");
	var_key.val.string.val = strdup("amname");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					if (var_value->type == jbvNull)
						local_node->amname = NULL;
					else
					{
						char *result = palloc(var_value->val.string.len + 1);
						memcpy(result, var_value->val.string.val, var_value->val.string.len);
						result[var_value->val.string.len] = '\0';
						local_node->amname = result;
					}
			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("isDrop");
	var_key.val.string.val = strdup("isDrop");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	local_node->isDrop = var_value->val.boolean;

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *LockStmt_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		LockStmt *local_node;
		if (node_cast != NULL)
			local_node = (LockStmt *) node_cast;
		else
			local_node = makeNode(LockStmt);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("mode");
	var_key.val.string.val = strdup("mode");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->mode = DatumGetInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("relations");
	var_key.val.string.val = strdup("relations");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->relations = NULL;
	else
		local_node->relations = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("nowait");
	var_key.val.string.val = strdup("nowait");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	local_node->nowait = var_value->val.boolean;

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *AlterTableStmt_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		AlterTableStmt *local_node;
		if (node_cast != NULL)
			local_node = (AlterTableStmt *) node_cast;
		else
			local_node = makeNode(AlterTableStmt);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("relation");
	var_key.val.string.val = strdup("relation");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->relation = NULL;
	} else {
		local_node->relation = (RangeVar *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("cmds");
	var_key.val.string.val = strdup("cmds");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->cmds = NULL;
	else
		local_node->cmds = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("missing_ok");
	var_key.val.string.val = strdup("missing_ok");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	local_node->missing_ok = var_value->val.boolean;

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("relkind");
	var_key.val.string.val = strdup("relkind");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->relkind = DatumGetInt16(DirectFunctionCall1(numeric_int2, NumericGetDatum(var_value->val.numeric)));

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *CreateSchemaStmt_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		CreateSchemaStmt *local_node;
		if (node_cast != NULL)
			local_node = (CreateSchemaStmt *) node_cast;
		else
			local_node = makeNode(CreateSchemaStmt);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("schemaname");
	var_key.val.string.val = strdup("schemaname");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					if (var_value->type == jbvNull)
						local_node->schemaname = NULL;
					else
					{
						char *result = palloc(var_value->val.string.len + 1);
						memcpy(result, var_value->val.string.val, var_value->val.string.len);
						result[var_value->val.string.len] = '\0';
						local_node->schemaname = result;
					}
			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("if_not_exists");
	var_key.val.string.val = strdup("if_not_exists");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	local_node->if_not_exists = var_value->val.boolean;

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("schemaElts");
	var_key.val.string.val = strdup("schemaElts");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->schemaElts = NULL;
	else
		local_node->schemaElts = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("authrole");
	var_key.val.string.val = strdup("authrole");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->authrole = NULL;
	} else {
		local_node->authrole = (Node *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *ClosePortalStmt_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		ClosePortalStmt *local_node;
		if (node_cast != NULL)
			local_node = (ClosePortalStmt *) node_cast;
		else
			local_node = makeNode(ClosePortalStmt);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("portalname");
	var_key.val.string.val = strdup("portalname");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					if (var_value->type == jbvNull)
						local_node->portalname = NULL;
					else
					{
						char *result = palloc(var_value->val.string.len + 1);
						memcpy(result, var_value->val.string.val, var_value->val.string.len);
						result[var_value->val.string.len] = '\0';
						local_node->portalname = result;
					}
			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *RelabelType_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		RelabelType *local_node;
		if (node_cast != NULL)
			local_node = (RelabelType *) node_cast;
		else
			local_node = makeNode(RelabelType);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("location");
	var_key.val.string.val = strdup("location");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->location = DatumGetInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("arg");
	var_key.val.string.val = strdup("arg");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->arg = NULL;
	} else {
		local_node->arg = (Expr *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("resulttypmod");
	var_key.val.string.val = strdup("resulttypmod");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->resulttypmod = DatumGetInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("relabelformat");
	var_key.val.string.val = strdup("relabelformat");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->relabelformat = DatumGetInt16(DirectFunctionCall1(numeric_int2, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("resulttype");
	var_key.val.string.val = strdup("resulttype");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->resulttype = DatumGetUInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("resultcollid");
	var_key.val.string.val = strdup("resultcollid");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->resultcollid = DatumGetUInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	Expr_deser(container, (void *)&local_node->xpr, -1);

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *FunctionScan_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		FunctionScan *local_node;
		if (node_cast != NULL)
			local_node = (FunctionScan *) node_cast;
		else
			local_node = makeNode(FunctionScan);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("functions");
	var_key.val.string.val = strdup("functions");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->functions = NULL;
	else
		local_node->functions = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("funcordinality");
	var_key.val.string.val = strdup("funcordinality");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	local_node->funcordinality = var_value->val.boolean;

			}
			{
					
	Scan_deser(container, (void *)&local_node->scan, -1);

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *AlterSeqStmt_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		AlterSeqStmt *local_node;
		if (node_cast != NULL)
			local_node = (AlterSeqStmt *) node_cast;
		else
			local_node = makeNode(AlterSeqStmt);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("options");
	var_key.val.string.val = strdup("options");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->options = NULL;
	else
		local_node->options = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("sequence");
	var_key.val.string.val = strdup("sequence");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->sequence = NULL;
	} else {
		local_node->sequence = (RangeVar *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("missing_ok");
	var_key.val.string.val = strdup("missing_ok");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	local_node->missing_ok = var_value->val.boolean;

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *SubPlan_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		SubPlan *local_node;
		if (node_cast != NULL)
			local_node = (SubPlan *) node_cast;
		else
			local_node = makeNode(SubPlan);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("args");
	var_key.val.string.val = strdup("args");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->args = NULL;
	else
		local_node->args = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("paramIds");
	var_key.val.string.val = strdup("paramIds");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->paramIds = NULL;
	else
		local_node->paramIds = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("setParam");
	var_key.val.string.val = strdup("setParam");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->setParam = NULL;
	else
		local_node->setParam = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("unknownEqFalse");
	var_key.val.string.val = strdup("unknownEqFalse");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	local_node->unknownEqFalse = var_value->val.boolean;

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("firstColType");
	var_key.val.string.val = strdup("firstColType");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->firstColType = DatumGetUInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("per_call_cost");
	var_key.val.string.val = strdup("per_call_cost");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->per_call_cost = DatumGetFloat8(DirectFunctionCall1(numeric_float8, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("firstColTypmod");
	var_key.val.string.val = strdup("firstColTypmod");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->firstColTypmod = DatumGetInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	Expr_deser(container, (void *)&local_node->xpr, -1);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("parParam");
	var_key.val.string.val = strdup("parParam");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->parParam = NULL;
	else
		local_node->parParam = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("startup_cost");
	var_key.val.string.val = strdup("startup_cost");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->startup_cost = DatumGetFloat8(DirectFunctionCall1(numeric_float8, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("plan_id");
	var_key.val.string.val = strdup("plan_id");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->plan_id = DatumGetInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("plan_name");
	var_key.val.string.val = strdup("plan_name");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					if (var_value->type == jbvNull)
						local_node->plan_name = NULL;
					else
					{
						char *result = palloc(var_value->val.string.len + 1);
						memcpy(result, var_value->val.string.val, var_value->val.string.len);
						result[var_value->val.string.len] = '\0';
						local_node->plan_name = result;
					}
			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("firstColCollation");
	var_key.val.string.val = strdup("firstColCollation");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->firstColCollation = DatumGetUInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("testexpr");
	var_key.val.string.val = strdup("testexpr");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->testexpr = NULL;
	} else {
		local_node->testexpr = (Node *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("subLinkType");
	var_key.val.string.val = strdup("subLinkType");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->subLinkType = DatumGetInt16(DirectFunctionCall1(numeric_int2, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("useHashTable");
	var_key.val.string.val = strdup("useHashTable");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	local_node->useHashTable = var_value->val.boolean;

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *ScalarArrayOpExpr_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		ScalarArrayOpExpr *local_node;
		if (node_cast != NULL)
			local_node = (ScalarArrayOpExpr *) node_cast;
		else
			local_node = makeNode(ScalarArrayOpExpr);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("args");
	var_key.val.string.val = strdup("args");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->args = NULL;
	else
		local_node->args = list_deser(var_value->val.binary.data, false);

			}
			{
					
	Expr_deser(container, (void *)&local_node->xpr, -1);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("inputcollid");
	var_key.val.string.val = strdup("inputcollid");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->inputcollid = DatumGetUInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("opfuncid");
	var_key.val.string.val = strdup("opfuncid");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->opfuncid = DatumGetUInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("location");
	var_key.val.string.val = strdup("location");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->location = DatumGetInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("useOr");
	var_key.val.string.val = strdup("useOr");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	local_node->useOr = var_value->val.boolean;

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("opno");
	var_key.val.string.val = strdup("opno");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->opno = DatumGetUInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *RoleSpec_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		RoleSpec *local_node;
		if (node_cast != NULL)
			local_node = (RoleSpec *) node_cast;
		else
			local_node = makeNode(RoleSpec);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("rolename");
	var_key.val.string.val = strdup("rolename");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					if (var_value->type == jbvNull)
						local_node->rolename = NULL;
					else
					{
						char *result = palloc(var_value->val.string.len + 1);
						memcpy(result, var_value->val.string.val, var_value->val.string.len);
						result[var_value->val.string.len] = '\0';
						local_node->rolename = result;
					}
			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("location");
	var_key.val.string.val = strdup("location");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->location = DatumGetInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("roletype");
	var_key.val.string.val = strdup("roletype");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->roletype = DatumGetInt16(DirectFunctionCall1(numeric_int2, NumericGetDatum(var_value->val.numeric)));

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *InlineCodeBlock_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		InlineCodeBlock *local_node;
		if (node_cast != NULL)
			local_node = (InlineCodeBlock *) node_cast;
		else
			local_node = makeNode(InlineCodeBlock);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("langOid");
	var_key.val.string.val = strdup("langOid");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->langOid = DatumGetUInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("langIsTrusted");
	var_key.val.string.val = strdup("langIsTrusted");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	local_node->langIsTrusted = var_value->val.boolean;

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("source_text");
	var_key.val.string.val = strdup("source_text");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					if (var_value->type == jbvNull)
						local_node->source_text = NULL;
					else
					{
						char *result = palloc(var_value->val.string.len + 1);
						memcpy(result, var_value->val.string.val, var_value->val.string.len);
						result[var_value->val.string.len] = '\0';
						local_node->source_text = result;
					}
			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *VariableShowStmt_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		VariableShowStmt *local_node;
		if (node_cast != NULL)
			local_node = (VariableShowStmt *) node_cast;
		else
			local_node = makeNode(VariableShowStmt);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("name");
	var_key.val.string.val = strdup("name");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					if (var_value->type == jbvNull)
						local_node->name = NULL;
					else
					{
						char *result = palloc(var_value->val.string.len + 1);
						memcpy(result, var_value->val.string.val, var_value->val.string.len);
						result[var_value->val.string.len] = '\0';
						local_node->name = result;
					}
			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *ImportForeignSchemaStmt_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		ImportForeignSchemaStmt *local_node;
		if (node_cast != NULL)
			local_node = (ImportForeignSchemaStmt *) node_cast;
		else
			local_node = makeNode(ImportForeignSchemaStmt);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("remote_schema");
	var_key.val.string.val = strdup("remote_schema");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					if (var_value->type == jbvNull)
						local_node->remote_schema = NULL;
					else
					{
						char *result = palloc(var_value->val.string.len + 1);
						memcpy(result, var_value->val.string.val, var_value->val.string.len);
						result[var_value->val.string.len] = '\0';
						local_node->remote_schema = result;
					}
			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("options");
	var_key.val.string.val = strdup("options");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->options = NULL;
	else
		local_node->options = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("local_schema");
	var_key.val.string.val = strdup("local_schema");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					if (var_value->type == jbvNull)
						local_node->local_schema = NULL;
					else
					{
						char *result = palloc(var_value->val.string.len + 1);
						memcpy(result, var_value->val.string.val, var_value->val.string.len);
						result[var_value->val.string.len] = '\0';
						local_node->local_schema = result;
					}
			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("list_type");
	var_key.val.string.val = strdup("list_type");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->list_type = DatumGetInt16(DirectFunctionCall1(numeric_int2, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("server_name");
	var_key.val.string.val = strdup("server_name");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					if (var_value->type == jbvNull)
						local_node->server_name = NULL;
					else
					{
						char *result = palloc(var_value->val.string.len + 1);
						memcpy(result, var_value->val.string.val, var_value->val.string.len);
						result[var_value->val.string.len] = '\0';
						local_node->server_name = result;
					}
			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("table_list");
	var_key.val.string.val = strdup("table_list");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->table_list = NULL;
	else
		local_node->table_list = list_deser(var_value->val.binary.data, false);

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *AlterForeignServerStmt_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		AlterForeignServerStmt *local_node;
		if (node_cast != NULL)
			local_node = (AlterForeignServerStmt *) node_cast;
		else
			local_node = makeNode(AlterForeignServerStmt);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("options");
	var_key.val.string.val = strdup("options");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->options = NULL;
	else
		local_node->options = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("servername");
	var_key.val.string.val = strdup("servername");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					if (var_value->type == jbvNull)
						local_node->servername = NULL;
					else
					{
						char *result = palloc(var_value->val.string.len + 1);
						memcpy(result, var_value->val.string.val, var_value->val.string.len);
						result[var_value->val.string.len] = '\0';
						local_node->servername = result;
					}
			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("has_version");
	var_key.val.string.val = strdup("has_version");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	local_node->has_version = var_value->val.boolean;

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("version");
	var_key.val.string.val = strdup("version");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					if (var_value->type == jbvNull)
						local_node->version = NULL;
					else
					{
						char *result = palloc(var_value->val.string.len + 1);
						memcpy(result, var_value->val.string.val, var_value->val.string.len);
						result[var_value->val.string.len] = '\0';
						local_node->version = result;
					}
			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *ModifyTable_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		ModifyTable *local_node;
		if (node_cast != NULL)
			local_node = (ModifyTable *) node_cast;
		else
			local_node = makeNode(ModifyTable);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("withCheckOptionLists");
	var_key.val.string.val = strdup("withCheckOptionLists");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->withCheckOptionLists = NULL;
	else
		local_node->withCheckOptionLists = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("plans");
	var_key.val.string.val = strdup("plans");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->plans = NULL;
	else
		local_node->plans = list_deser(var_value->val.binary.data, false);

			}
			{
					
	Plan_deser(container, (void *)&local_node->plan, -1);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("nominalRelation");
	var_key.val.string.val = strdup("nominalRelation");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->nominalRelation = DatumGetUInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("onConflictSet");
	var_key.val.string.val = strdup("onConflictSet");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->onConflictSet = NULL;
	else
		local_node->onConflictSet = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("returningLists");
	var_key.val.string.val = strdup("returningLists");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->returningLists = NULL;
	else
		local_node->returningLists = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("arbiterIndexes");
	var_key.val.string.val = strdup("arbiterIndexes");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->arbiterIndexes = NULL;
	else
		local_node->arbiterIndexes = list_deser(var_value->val.binary.data, true);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("fdwPrivLists");
	var_key.val.string.val = strdup("fdwPrivLists");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->fdwPrivLists = NULL;
	else
		local_node->fdwPrivLists = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("canSetTag");
	var_key.val.string.val = strdup("canSetTag");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	local_node->canSetTag = var_value->val.boolean;

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("rowMarks");
	var_key.val.string.val = strdup("rowMarks");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->rowMarks = NULL;
	else
		local_node->rowMarks = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("epqParam");
	var_key.val.string.val = strdup("epqParam");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->epqParam = DatumGetInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("resultRelIndex");
	var_key.val.string.val = strdup("resultRelIndex");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->resultRelIndex = DatumGetInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("exclRelRTI");
	var_key.val.string.val = strdup("exclRelRTI");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->exclRelRTI = DatumGetUInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("resultRelations");
	var_key.val.string.val = strdup("resultRelations");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->resultRelations = NULL;
	else
		local_node->resultRelations = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("operation");
	var_key.val.string.val = strdup("operation");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->operation = DatumGetInt16(DirectFunctionCall1(numeric_int2, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("exclRelTlist");
	var_key.val.string.val = strdup("exclRelTlist");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->exclRelTlist = NULL;
	else
		local_node->exclRelTlist = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("onConflictAction");
	var_key.val.string.val = strdup("onConflictAction");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->onConflictAction = DatumGetInt16(DirectFunctionCall1(numeric_int2, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("onConflictWhere");
	var_key.val.string.val = strdup("onConflictWhere");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->onConflictWhere = NULL;
	} else {
		local_node->onConflictWhere = (Node *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *CheckPointStmt_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		CheckPointStmt *local_node;
		if (node_cast != NULL)
			local_node = (CheckPointStmt *) node_cast;
		else
			local_node = makeNode(CheckPointStmt);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *RangeVar_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		RangeVar *local_node;
		if (node_cast != NULL)
			local_node = (RangeVar *) node_cast;
		else
			local_node = makeNode(RangeVar);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("relname");
	var_key.val.string.val = strdup("relname");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					if (var_value->type == jbvNull)
						local_node->relname = NULL;
					else
					{
						char *result = palloc(var_value->val.string.len + 1);
						memcpy(result, var_value->val.string.val, var_value->val.string.len);
						result[var_value->val.string.len] = '\0';
						local_node->relname = result;
					}
			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("schemaname");
	var_key.val.string.val = strdup("schemaname");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					if (var_value->type == jbvNull)
						local_node->schemaname = NULL;
					else
					{
						char *result = palloc(var_value->val.string.len + 1);
						memcpy(result, var_value->val.string.val, var_value->val.string.len);
						result[var_value->val.string.len] = '\0';
						local_node->schemaname = result;
					}
			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("catalogname");
	var_key.val.string.val = strdup("catalogname");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					if (var_value->type == jbvNull)
						local_node->catalogname = NULL;
					else
					{
						char *result = palloc(var_value->val.string.len + 1);
						memcpy(result, var_value->val.string.val, var_value->val.string.len);
						result[var_value->val.string.len] = '\0';
						local_node->catalogname = result;
					}
			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("location");
	var_key.val.string.val = strdup("location");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->location = DatumGetInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("relpersistence");
	var_key.val.string.val = strdup("relpersistence");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->relpersistence = DatumGetInt16(DirectFunctionCall1(numeric_int2, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("inhOpt");
	var_key.val.string.val = strdup("inhOpt");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->inhOpt = DatumGetInt16(DirectFunctionCall1(numeric_int2, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("alias");
	var_key.val.string.val = strdup("alias");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->alias = NULL;
	} else {
		local_node->alias = (Alias *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *AlterExtensionContentsStmt_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		AlterExtensionContentsStmt *local_node;
		if (node_cast != NULL)
			local_node = (AlterExtensionContentsStmt *) node_cast;
		else
			local_node = makeNode(AlterExtensionContentsStmt);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("objtype");
	var_key.val.string.val = strdup("objtype");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->objtype = DatumGetInt16(DirectFunctionCall1(numeric_int2, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("objname");
	var_key.val.string.val = strdup("objname");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->objname = NULL;
	else
		local_node->objname = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("action");
	var_key.val.string.val = strdup("action");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->action = DatumGetInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("objargs");
	var_key.val.string.val = strdup("objargs");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->objargs = NULL;
	else
		local_node->objargs = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("extname");
	var_key.val.string.val = strdup("extname");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					if (var_value->type == jbvNull)
						local_node->extname = NULL;
					else
					{
						char *result = palloc(var_value->val.string.len + 1);
						memcpy(result, var_value->val.string.val, var_value->val.string.len);
						result[var_value->val.string.len] = '\0';
						local_node->extname = result;
					}
			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *RangeTblRef_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		RangeTblRef *local_node;
		if (node_cast != NULL)
			local_node = (RangeTblRef *) node_cast;
		else
			local_node = makeNode(RangeTblRef);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("rtindex");
	var_key.val.string.val = strdup("rtindex");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->rtindex = DatumGetInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *CreateOpClassStmt_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		CreateOpClassStmt *local_node;
		if (node_cast != NULL)
			local_node = (CreateOpClassStmt *) node_cast;
		else
			local_node = makeNode(CreateOpClassStmt);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("opfamilyname");
	var_key.val.string.val = strdup("opfamilyname");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->opfamilyname = NULL;
	else
		local_node->opfamilyname = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("datatype");
	var_key.val.string.val = strdup("datatype");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->datatype = NULL;
	} else {
		local_node->datatype = (TypeName *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("amname");
	var_key.val.string.val = strdup("amname");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					if (var_value->type == jbvNull)
						local_node->amname = NULL;
					else
					{
						char *result = palloc(var_value->val.string.len + 1);
						memcpy(result, var_value->val.string.val, var_value->val.string.len);
						result[var_value->val.string.len] = '\0';
						local_node->amname = result;
					}
			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("items");
	var_key.val.string.val = strdup("items");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->items = NULL;
	else
		local_node->items = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("opclassname");
	var_key.val.string.val = strdup("opclassname");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->opclassname = NULL;
	else
		local_node->opclassname = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("isDefault");
	var_key.val.string.val = strdup("isDefault");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	local_node->isDefault = var_value->val.boolean;

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *LoadStmt_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		LoadStmt *local_node;
		if (node_cast != NULL)
			local_node = (LoadStmt *) node_cast;
		else
			local_node = makeNode(LoadStmt);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("filename");
	var_key.val.string.val = strdup("filename");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					if (var_value->type == jbvNull)
						local_node->filename = NULL;
					else
					{
						char *result = palloc(var_value->val.string.len + 1);
						memcpy(result, var_value->val.string.val, var_value->val.string.len);
						result[var_value->val.string.len] = '\0';
						local_node->filename = result;
					}
			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *ColumnRef_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		ColumnRef *local_node;
		if (node_cast != NULL)
			local_node = (ColumnRef *) node_cast;
		else
			local_node = makeNode(ColumnRef);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("location");
	var_key.val.string.val = strdup("location");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->location = DatumGetInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("fields");
	var_key.val.string.val = strdup("fields");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->fields = NULL;
	else
		local_node->fields = list_deser(var_value->val.binary.data, false);

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *CreateTrigStmt_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		CreateTrigStmt *local_node;
		if (node_cast != NULL)
			local_node = (CreateTrigStmt *) node_cast;
		else
			local_node = makeNode(CreateTrigStmt);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("args");
	var_key.val.string.val = strdup("args");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->args = NULL;
	else
		local_node->args = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("timing");
	var_key.val.string.val = strdup("timing");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->timing = DatumGetInt16(DirectFunctionCall1(numeric_int2, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("events");
	var_key.val.string.val = strdup("events");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->events = DatumGetInt16(DirectFunctionCall1(numeric_int2, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("relation");
	var_key.val.string.val = strdup("relation");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->relation = NULL;
	} else {
		local_node->relation = (RangeVar *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("isconstraint");
	var_key.val.string.val = strdup("isconstraint");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	local_node->isconstraint = var_value->val.boolean;

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("funcname");
	var_key.val.string.val = strdup("funcname");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->funcname = NULL;
	else
		local_node->funcname = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("initdeferred");
	var_key.val.string.val = strdup("initdeferred");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	local_node->initdeferred = var_value->val.boolean;

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("columns");
	var_key.val.string.val = strdup("columns");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->columns = NULL;
	else
		local_node->columns = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("deferrable");
	var_key.val.string.val = strdup("deferrable");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	local_node->deferrable = var_value->val.boolean;

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("row");
	var_key.val.string.val = strdup("row");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	local_node->row = var_value->val.boolean;

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("constrrel");
	var_key.val.string.val = strdup("constrrel");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->constrrel = NULL;
	} else {
		local_node->constrrel = (RangeVar *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("trigname");
	var_key.val.string.val = strdup("trigname");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					if (var_value->type == jbvNull)
						local_node->trigname = NULL;
					else
					{
						char *result = palloc(var_value->val.string.len + 1);
						memcpy(result, var_value->val.string.val, var_value->val.string.len);
						result[var_value->val.string.len] = '\0';
						local_node->trigname = result;
					}
			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("whenClause");
	var_key.val.string.val = strdup("whenClause");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->whenClause = NULL;
	} else {
		local_node->whenClause = (Node *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *ConstraintsSetStmt_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		ConstraintsSetStmt *local_node;
		if (node_cast != NULL)
			local_node = (ConstraintsSetStmt *) node_cast;
		else
			local_node = makeNode(ConstraintsSetStmt);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("constraints");
	var_key.val.string.val = strdup("constraints");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->constraints = NULL;
	else
		local_node->constraints = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("deferred");
	var_key.val.string.val = strdup("deferred");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	local_node->deferred = var_value->val.boolean;

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *Var_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		Var *local_node;
		if (node_cast != NULL)
			local_node = (Var *) node_cast;
		else
			local_node = makeNode(Var);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("vartypmod");
	var_key.val.string.val = strdup("vartypmod");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->vartypmod = DatumGetInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("varoattno");
	var_key.val.string.val = strdup("varoattno");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->varoattno = DatumGetInt16(DirectFunctionCall1(numeric_int2, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("varno");
	var_key.val.string.val = strdup("varno");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->varno = DatumGetUInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("vartype");
	var_key.val.string.val = strdup("vartype");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->vartype = DatumGetUInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("varnoold");
	var_key.val.string.val = strdup("varnoold");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->varnoold = DatumGetUInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("varcollid");
	var_key.val.string.val = strdup("varcollid");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->varcollid = DatumGetUInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("varattno");
	var_key.val.string.val = strdup("varattno");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->varattno = DatumGetInt16(DirectFunctionCall1(numeric_int2, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("location");
	var_key.val.string.val = strdup("location");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->location = DatumGetInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	Expr_deser(container, (void *)&local_node->xpr, -1);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("varlevelsup");
	var_key.val.string.val = strdup("varlevelsup");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->varlevelsup = DatumGetUInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *BitmapIndexScan_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		BitmapIndexScan *local_node;
		if (node_cast != NULL)
			local_node = (BitmapIndexScan *) node_cast;
		else
			local_node = makeNode(BitmapIndexScan);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("indexid");
	var_key.val.string.val = strdup("indexid");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->indexid = DatumGetUInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("indexqualorig");
	var_key.val.string.val = strdup("indexqualorig");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->indexqualorig = NULL;
	else
		local_node->indexqualorig = list_deser(var_value->val.binary.data, false);

			}
			{
					
	Scan_deser(container, (void *)&local_node->scan, -1);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("indexqual");
	var_key.val.string.val = strdup("indexqual");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->indexqual = NULL;
	else
		local_node->indexqual = list_deser(var_value->val.binary.data, false);

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *Plan_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		Plan *local_node;
		if (node_cast != NULL)
			local_node = (Plan *) node_cast;
		else
			local_node = makeNode(Plan);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("total_cost");
	var_key.val.string.val = strdup("total_cost");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->total_cost = DatumGetFloat8(DirectFunctionCall1(numeric_float8, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("plan_rows");
	var_key.val.string.val = strdup("plan_rows");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->plan_rows = DatumGetFloat8(DirectFunctionCall1(numeric_float8, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("allParam");
	var_key.val.string.val = strdup("allParam");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					if (var_value->type == jbvNull)
						local_node->allParam = NULL;
					else
						
	{
		Bitmapset  *result = NULL;
		JsonbValue v;
		int type;
		JsonbIterator *it = JsonbIteratorInit(var_value->val.binary.data);
		while ((type = JsonbIteratorNext(&it, &v, true)) != WJB_DONE)
		{
			if (type == WJB_ELEM)
				result = bms_add_member(result,
										DatumGetUInt32(DirectFunctionCall1(numeric_int4,
																		   NumericGetDatum(v.val.numeric))));
		}
		local_node->allParam = result;
	}
	

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("initPlan");
	var_key.val.string.val = strdup("initPlan");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->initPlan = NULL;
	else
		local_node->initPlan = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("lefttree");
	var_key.val.string.val = strdup("lefttree");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->lefttree = NULL;
	} else {
		local_node->lefttree = (Plan *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("extParam");
	var_key.val.string.val = strdup("extParam");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					if (var_value->type == jbvNull)
						local_node->extParam = NULL;
					else
						
	{
		Bitmapset  *result = NULL;
		JsonbValue v;
		int type;
		JsonbIterator *it = JsonbIteratorInit(var_value->val.binary.data);
		while ((type = JsonbIteratorNext(&it, &v, true)) != WJB_DONE)
		{
			if (type == WJB_ELEM)
				result = bms_add_member(result,
										DatumGetUInt32(DirectFunctionCall1(numeric_int4,
																		   NumericGetDatum(v.val.numeric))));
		}
		local_node->extParam = result;
	}
	

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("righttree");
	var_key.val.string.val = strdup("righttree");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->righttree = NULL;
	} else {
		local_node->righttree = (Plan *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("plan_width");
	var_key.val.string.val = strdup("plan_width");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->plan_width = DatumGetInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("startup_cost");
	var_key.val.string.val = strdup("startup_cost");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->startup_cost = DatumGetFloat8(DirectFunctionCall1(numeric_float8, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("qual");
	var_key.val.string.val = strdup("qual");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->qual = NULL;
	else
		local_node->qual = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("targetlist");
	var_key.val.string.val = strdup("targetlist");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->targetlist = NULL;
	else
		local_node->targetlist = list_deser(var_value->val.binary.data, false);

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *CreateStmt_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		CreateStmt *local_node;
		if (node_cast != NULL)
			local_node = (CreateStmt *) node_cast;
		else
			local_node = makeNode(CreateStmt);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("oncommit");
	var_key.val.string.val = strdup("oncommit");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->oncommit = DatumGetInt16(DirectFunctionCall1(numeric_int2, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("constraints");
	var_key.val.string.val = strdup("constraints");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->constraints = NULL;
	else
		local_node->constraints = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("inhRelations");
	var_key.val.string.val = strdup("inhRelations");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->inhRelations = NULL;
	else
		local_node->inhRelations = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("ofTypename");
	var_key.val.string.val = strdup("ofTypename");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->ofTypename = NULL;
	} else {
		local_node->ofTypename = (TypeName *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("relation");
	var_key.val.string.val = strdup("relation");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->relation = NULL;
	} else {
		local_node->relation = (RangeVar *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("if_not_exists");
	var_key.val.string.val = strdup("if_not_exists");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	local_node->if_not_exists = var_value->val.boolean;

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("tablespacename");
	var_key.val.string.val = strdup("tablespacename");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					if (var_value->type == jbvNull)
						local_node->tablespacename = NULL;
					else
					{
						char *result = palloc(var_value->val.string.len + 1);
						memcpy(result, var_value->val.string.val, var_value->val.string.len);
						result[var_value->val.string.len] = '\0';
						local_node->tablespacename = result;
					}
			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("options");
	var_key.val.string.val = strdup("options");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->options = NULL;
	else
		local_node->options = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("tableElts");
	var_key.val.string.val = strdup("tableElts");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->tableElts = NULL;
	else
		local_node->tableElts = list_deser(var_value->val.binary.data, false);

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *InferClause_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		InferClause *local_node;
		if (node_cast != NULL)
			local_node = (InferClause *) node_cast;
		else
			local_node = makeNode(InferClause);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("conname");
	var_key.val.string.val = strdup("conname");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					if (var_value->type == jbvNull)
						local_node->conname = NULL;
					else
					{
						char *result = palloc(var_value->val.string.len + 1);
						memcpy(result, var_value->val.string.val, var_value->val.string.len);
						result[var_value->val.string.len] = '\0';
						local_node->conname = result;
					}
			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("location");
	var_key.val.string.val = strdup("location");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->location = DatumGetInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("whereClause");
	var_key.val.string.val = strdup("whereClause");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->whereClause = NULL;
	} else {
		local_node->whereClause = (Node *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("indexElems");
	var_key.val.string.val = strdup("indexElems");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->indexElems = NULL;
	else
		local_node->indexElems = list_deser(var_value->val.binary.data, false);

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *Param_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		Param *local_node;
		if (node_cast != NULL)
			local_node = (Param *) node_cast;
		else
			local_node = makeNode(Param);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("paramkind");
	var_key.val.string.val = strdup("paramkind");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->paramkind = DatumGetInt16(DirectFunctionCall1(numeric_int2, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("paramid");
	var_key.val.string.val = strdup("paramid");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->paramid = DatumGetInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("paramtype");
	var_key.val.string.val = strdup("paramtype");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->paramtype = DatumGetUInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("location");
	var_key.val.string.val = strdup("location");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->location = DatumGetInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("paramtypmod");
	var_key.val.string.val = strdup("paramtypmod");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->paramtypmod = DatumGetInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	Expr_deser(container, (void *)&local_node->xpr, -1);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("paramcollid");
	var_key.val.string.val = strdup("paramcollid");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->paramcollid = DatumGetUInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *ExecuteStmt_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		ExecuteStmt *local_node;
		if (node_cast != NULL)
			local_node = (ExecuteStmt *) node_cast;
		else
			local_node = makeNode(ExecuteStmt);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("name");
	var_key.val.string.val = strdup("name");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					if (var_value->type == jbvNull)
						local_node->name = NULL;
					else
					{
						char *result = palloc(var_value->val.string.len + 1);
						memcpy(result, var_value->val.string.val, var_value->val.string.len);
						result[var_value->val.string.len] = '\0';
						local_node->name = result;
					}
			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("params");
	var_key.val.string.val = strdup("params");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->params = NULL;
	else
		local_node->params = list_deser(var_value->val.binary.data, false);

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *DropdbStmt_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		DropdbStmt *local_node;
		if (node_cast != NULL)
			local_node = (DropdbStmt *) node_cast;
		else
			local_node = makeNode(DropdbStmt);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("dbname");
	var_key.val.string.val = strdup("dbname");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					if (var_value->type == jbvNull)
						local_node->dbname = NULL;
					else
					{
						char *result = palloc(var_value->val.string.len + 1);
						memcpy(result, var_value->val.string.val, var_value->val.string.len);
						result[var_value->val.string.len] = '\0';
						local_node->dbname = result;
					}
			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("missing_ok");
	var_key.val.string.val = strdup("missing_ok");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	local_node->missing_ok = var_value->val.boolean;

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *FetchStmt_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		FetchStmt *local_node;
		if (node_cast != NULL)
			local_node = (FetchStmt *) node_cast;
		else
			local_node = makeNode(FetchStmt);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("direction");
	var_key.val.string.val = strdup("direction");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->direction = DatumGetInt16(DirectFunctionCall1(numeric_int2, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("ismove");
	var_key.val.string.val = strdup("ismove");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	local_node->ismove = var_value->val.boolean;

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("howMany");
	var_key.val.string.val = strdup("howMany");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
#ifdef USE_FLOAT8_BYVAL
	local_node->howMany = DatumGetInt64(DirectFunctionCall1(numeric_int8, NumericGetDatum(var_value->val.numeric)));
#else
	local_node->howMany = DatumGetInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));
#endif

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("portalname");
	var_key.val.string.val = strdup("portalname");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					if (var_value->type == jbvNull)
						local_node->portalname = NULL;
					else
					{
						char *result = palloc(var_value->val.string.len + 1);
						memcpy(result, var_value->val.string.val, var_value->val.string.len);
						result[var_value->val.string.len] = '\0';
						local_node->portalname = result;
					}
			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *HashJoin_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		HashJoin *local_node;
		if (node_cast != NULL)
			local_node = (HashJoin *) node_cast;
		else
			local_node = makeNode(HashJoin);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("hashclauses");
	var_key.val.string.val = strdup("hashclauses");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->hashclauses = NULL;
	else
		local_node->hashclauses = list_deser(var_value->val.binary.data, false);

			}
			{
					
	Join_deser(container, (void *)&local_node->join, -1);

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *ColumnDef_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		ColumnDef *local_node;
		if (node_cast != NULL)
			local_node = (ColumnDef *) node_cast;
		else
			local_node = makeNode(ColumnDef);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("location");
	var_key.val.string.val = strdup("location");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->location = DatumGetInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("typeName");
	var_key.val.string.val = strdup("typeName");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->typeName = NULL;
	} else {
		local_node->typeName = (TypeName *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("inhcount");
	var_key.val.string.val = strdup("inhcount");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->inhcount = DatumGetInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("is_not_null");
	var_key.val.string.val = strdup("is_not_null");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	local_node->is_not_null = var_value->val.boolean;

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("collOid");
	var_key.val.string.val = strdup("collOid");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->collOid = DatumGetUInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("is_from_type");
	var_key.val.string.val = strdup("is_from_type");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	local_node->is_from_type = var_value->val.boolean;

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("fdwoptions");
	var_key.val.string.val = strdup("fdwoptions");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->fdwoptions = NULL;
	else
		local_node->fdwoptions = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("colname");
	var_key.val.string.val = strdup("colname");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					if (var_value->type == jbvNull)
						local_node->colname = NULL;
					else
					{
						char *result = palloc(var_value->val.string.len + 1);
						memcpy(result, var_value->val.string.val, var_value->val.string.len);
						result[var_value->val.string.len] = '\0';
						local_node->colname = result;
					}
			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("storage");
	var_key.val.string.val = strdup("storage");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->storage = DatumGetInt16(DirectFunctionCall1(numeric_int2, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("raw_default");
	var_key.val.string.val = strdup("raw_default");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->raw_default = NULL;
	} else {
		local_node->raw_default = (Node *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("constraints");
	var_key.val.string.val = strdup("constraints");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->constraints = NULL;
	else
		local_node->constraints = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("is_local");
	var_key.val.string.val = strdup("is_local");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	local_node->is_local = var_value->val.boolean;

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("collClause");
	var_key.val.string.val = strdup("collClause");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->collClause = NULL;
	} else {
		local_node->collClause = (CollateClause *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("cooked_default");
	var_key.val.string.val = strdup("cooked_default");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->cooked_default = NULL;
	} else {
		local_node->cooked_default = (Node *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *AlterRoleSetStmt_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		AlterRoleSetStmt *local_node;
		if (node_cast != NULL)
			local_node = (AlterRoleSetStmt *) node_cast;
		else
			local_node = makeNode(AlterRoleSetStmt);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("database");
	var_key.val.string.val = strdup("database");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					if (var_value->type == jbvNull)
						local_node->database = NULL;
					else
					{
						char *result = palloc(var_value->val.string.len + 1);
						memcpy(result, var_value->val.string.val, var_value->val.string.len);
						result[var_value->val.string.len] = '\0';
						local_node->database = result;
					}
			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("role");
	var_key.val.string.val = strdup("role");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->role = NULL;
	} else {
		local_node->role = (Node *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("setstmt");
	var_key.val.string.val = strdup("setstmt");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->setstmt = NULL;
	} else {
		local_node->setstmt = (VariableSetStmt *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *Result_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		Result *local_node;
		if (node_cast != NULL)
			local_node = (Result *) node_cast;
		else
			local_node = makeNode(Result);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("resconstantqual");
	var_key.val.string.val = strdup("resconstantqual");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->resconstantqual = NULL;
	} else {
		local_node->resconstantqual = (Node *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
			{
					
	Plan_deser(container, (void *)&local_node->plan, -1);

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *PrepareStmt_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		PrepareStmt *local_node;
		if (node_cast != NULL)
			local_node = (PrepareStmt *) node_cast;
		else
			local_node = makeNode(PrepareStmt);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("name");
	var_key.val.string.val = strdup("name");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					if (var_value->type == jbvNull)
						local_node->name = NULL;
					else
					{
						char *result = palloc(var_value->val.string.len + 1);
						memcpy(result, var_value->val.string.val, var_value->val.string.len);
						result[var_value->val.string.len] = '\0';
						local_node->name = result;
					}
			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("argtypes");
	var_key.val.string.val = strdup("argtypes");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->argtypes = NULL;
	else
		local_node->argtypes = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("query");
	var_key.val.string.val = strdup("query");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->query = NULL;
	} else {
		local_node->query = (Node *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *AlterDomainStmt_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		AlterDomainStmt *local_node;
		if (node_cast != NULL)
			local_node = (AlterDomainStmt *) node_cast;
		else
			local_node = makeNode(AlterDomainStmt);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("typeName");
	var_key.val.string.val = strdup("typeName");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->typeName = NULL;
	else
		local_node->typeName = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("def");
	var_key.val.string.val = strdup("def");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->def = NULL;
	} else {
		local_node->def = (Node *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("name");
	var_key.val.string.val = strdup("name");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					if (var_value->type == jbvNull)
						local_node->name = NULL;
					else
					{
						char *result = palloc(var_value->val.string.len + 1);
						memcpy(result, var_value->val.string.val, var_value->val.string.len);
						result[var_value->val.string.len] = '\0';
						local_node->name = result;
					}
			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("subtype");
	var_key.val.string.val = strdup("subtype");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->subtype = DatumGetInt16(DirectFunctionCall1(numeric_int2, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("behavior");
	var_key.val.string.val = strdup("behavior");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->behavior = DatumGetInt16(DirectFunctionCall1(numeric_int2, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("missing_ok");
	var_key.val.string.val = strdup("missing_ok");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	local_node->missing_ok = var_value->val.boolean;

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *CompositeTypeStmt_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		CompositeTypeStmt *local_node;
		if (node_cast != NULL)
			local_node = (CompositeTypeStmt *) node_cast;
		else
			local_node = makeNode(CompositeTypeStmt);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("coldeflist");
	var_key.val.string.val = strdup("coldeflist");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->coldeflist = NULL;
	else
		local_node->coldeflist = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("typevar");
	var_key.val.string.val = strdup("typevar");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->typevar = NULL;
	} else {
		local_node->typevar = (RangeVar *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *CustomScan_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		CustomScan *local_node;
		if (node_cast != NULL)
			local_node = (CustomScan *) node_cast;
		else
			local_node = makeNode(CustomScan);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("flags");
	var_key.val.string.val = strdup("flags");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->flags = DatumGetUInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	Scan_deser(container, (void *)&local_node->scan, -1);

			}
			{
					/* NOT FOUND TYPE: *CustomScanMethods */
			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("custom_exprs");
	var_key.val.string.val = strdup("custom_exprs");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->custom_exprs = NULL;
	else
		local_node->custom_exprs = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("custom_relids");
	var_key.val.string.val = strdup("custom_relids");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					if (var_value->type == jbvNull)
						local_node->custom_relids = NULL;
					else
						
	{
		Bitmapset  *result = NULL;
		JsonbValue v;
		int type;
		JsonbIterator *it = JsonbIteratorInit(var_value->val.binary.data);
		while ((type = JsonbIteratorNext(&it, &v, true)) != WJB_DONE)
		{
			if (type == WJB_ELEM)
				result = bms_add_member(result,
										DatumGetUInt32(DirectFunctionCall1(numeric_int4,
																		   NumericGetDatum(v.val.numeric))));
		}
		local_node->custom_relids = result;
	}
	

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("custom_scan_tlist");
	var_key.val.string.val = strdup("custom_scan_tlist");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->custom_scan_tlist = NULL;
	else
		local_node->custom_scan_tlist = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("custom_plans");
	var_key.val.string.val = strdup("custom_plans");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->custom_plans = NULL;
	else
		local_node->custom_plans = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("custom_private");
	var_key.val.string.val = strdup("custom_private");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->custom_private = NULL;
	else
		local_node->custom_private = list_deser(var_value->val.binary.data, false);

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *Agg_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		Agg *local_node;
		if (node_cast != NULL)
			local_node = (Agg *) node_cast;
		else
			local_node = makeNode(Agg);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("numGroups");
	var_key.val.string.val = strdup("numGroups");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
#ifdef USE_FLOAT8_BYVAL
	local_node->numGroups = DatumGetInt64(DirectFunctionCall1(numeric_int8, NumericGetDatum(var_value->val.numeric)));
#else
	local_node->numGroups = DatumGetInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));
#endif

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("grpColIdx");
	var_key.val.string.val = strdup("grpColIdx");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

						
	{
		int type;
		int i = 0;
		JsonbValue v;
		JsonbIterator *it = JsonbIteratorInit(var_value->val.binary.data);
			local_node->numCols = it->nElems;
		local_node->grpColIdx = (AttrNumber*) palloc(sizeof(AttrNumber)*it->nElems);
		while ((type = JsonbIteratorNext(&it, &v, true)) != WJB_DONE)
		{
			if (type == WJB_ELEM)
			{
					
		local_node->grpColIdx[i] = DatumGetInt16(DirectFunctionCall1(numeric_int2, NumericGetDatum(v.val.numeric)));

				i++;
			}
		}
	}

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("grpOperators");
	var_key.val.string.val = strdup("grpOperators");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

						
	{
		int type;
		int i = 0;
		JsonbValue v;
		JsonbIterator *it = JsonbIteratorInit(var_value->val.binary.data);
			local_node->numCols = it->nElems;
		local_node->grpOperators = (Oid*) palloc(sizeof(Oid)*it->nElems);
		while ((type = JsonbIteratorNext(&it, &v, true)) != WJB_DONE)
		{
			if (type == WJB_ELEM)
			{
					
		local_node->grpOperators[i] = DatumGetUInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(v.val.numeric)));

				i++;
			}
		}
	}

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("groupingSets");
	var_key.val.string.val = strdup("groupingSets");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->groupingSets = NULL;
	else
		local_node->groupingSets = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("aggstrategy");
	var_key.val.string.val = strdup("aggstrategy");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->aggstrategy = DatumGetInt16(DirectFunctionCall1(numeric_int2, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	Plan_deser(container, (void *)&local_node->plan, -1);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("numCols");
	var_key.val.string.val = strdup("numCols");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->numCols = DatumGetInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("chain");
	var_key.val.string.val = strdup("chain");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->chain = NULL;
	else
		local_node->chain = list_deser(var_value->val.binary.data, false);

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *AlterObjectSchemaStmt_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		AlterObjectSchemaStmt *local_node;
		if (node_cast != NULL)
			local_node = (AlterObjectSchemaStmt *) node_cast;
		else
			local_node = makeNode(AlterObjectSchemaStmt);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("missing_ok");
	var_key.val.string.val = strdup("missing_ok");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	local_node->missing_ok = var_value->val.boolean;

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("objarg");
	var_key.val.string.val = strdup("objarg");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->objarg = NULL;
	else
		local_node->objarg = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("objectType");
	var_key.val.string.val = strdup("objectType");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->objectType = DatumGetInt16(DirectFunctionCall1(numeric_int2, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("relation");
	var_key.val.string.val = strdup("relation");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->relation = NULL;
	} else {
		local_node->relation = (RangeVar *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("object");
	var_key.val.string.val = strdup("object");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->object = NULL;
	else
		local_node->object = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("newschema");
	var_key.val.string.val = strdup("newschema");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					if (var_value->type == jbvNull)
						local_node->newschema = NULL;
					else
					{
						char *result = palloc(var_value->val.string.len + 1);
						memcpy(result, var_value->val.string.val, var_value->val.string.len);
						result[var_value->val.string.len] = '\0';
						local_node->newschema = result;
					}
			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *AlterEventTrigStmt_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		AlterEventTrigStmt *local_node;
		if (node_cast != NULL)
			local_node = (AlterEventTrigStmt *) node_cast;
		else
			local_node = makeNode(AlterEventTrigStmt);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("trigname");
	var_key.val.string.val = strdup("trigname");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					if (var_value->type == jbvNull)
						local_node->trigname = NULL;
					else
					{
						char *result = palloc(var_value->val.string.len + 1);
						memcpy(result, var_value->val.string.val, var_value->val.string.len);
						result[var_value->val.string.len] = '\0';
						local_node->trigname = result;
					}
			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("tgenabled");
	var_key.val.string.val = strdup("tgenabled");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->tgenabled = DatumGetInt16(DirectFunctionCall1(numeric_int2, NumericGetDatum(var_value->val.numeric)));

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *AlterDefaultPrivilegesStmt_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		AlterDefaultPrivilegesStmt *local_node;
		if (node_cast != NULL)
			local_node = (AlterDefaultPrivilegesStmt *) node_cast;
		else
			local_node = makeNode(AlterDefaultPrivilegesStmt);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("action");
	var_key.val.string.val = strdup("action");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->action = NULL;
	} else {
		local_node->action = (GrantStmt *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("options");
	var_key.val.string.val = strdup("options");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->options = NULL;
	else
		local_node->options = list_deser(var_value->val.binary.data, false);

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *PlannedStmt_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		PlannedStmt *local_node;
		if (node_cast != NULL)
			local_node = (PlannedStmt *) node_cast;
		else
			local_node = makeNode(PlannedStmt);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("rewindPlanIDs");
	var_key.val.string.val = strdup("rewindPlanIDs");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					if (var_value->type == jbvNull)
						local_node->rewindPlanIDs = NULL;
					else
						
	{
		Bitmapset  *result = NULL;
		JsonbValue v;
		int type;
		JsonbIterator *it = JsonbIteratorInit(var_value->val.binary.data);
		while ((type = JsonbIteratorNext(&it, &v, true)) != WJB_DONE)
		{
			if (type == WJB_ELEM)
				result = bms_add_member(result,
										DatumGetUInt32(DirectFunctionCall1(numeric_int4,
																		   NumericGetDatum(v.val.numeric))));
		}
		local_node->rewindPlanIDs = result;
	}
	

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("relationOids");
	var_key.val.string.val = strdup("relationOids");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->relationOids = NULL;
	else
		local_node->relationOids = list_deser(var_value->val.binary.data, true);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("subplans");
	var_key.val.string.val = strdup("subplans");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->subplans = NULL;
	else
		local_node->subplans = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("queryId");
	var_key.val.string.val = strdup("queryId");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->queryId = DatumGetUInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("hasRowSecurity");
	var_key.val.string.val = strdup("hasRowSecurity");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	local_node->hasRowSecurity = var_value->val.boolean;

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("canSetTag");
	var_key.val.string.val = strdup("canSetTag");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	local_node->canSetTag = var_value->val.boolean;

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("nParamExec");
	var_key.val.string.val = strdup("nParamExec");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->nParamExec = DatumGetInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("commandType");
	var_key.val.string.val = strdup("commandType");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->commandType = DatumGetInt16(DirectFunctionCall1(numeric_int2, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("resultRelations");
	var_key.val.string.val = strdup("resultRelations");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->resultRelations = NULL;
	else
		local_node->resultRelations = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("rowMarks");
	var_key.val.string.val = strdup("rowMarks");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->rowMarks = NULL;
	else
		local_node->rowMarks = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("planTree");
	var_key.val.string.val = strdup("planTree");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->planTree = NULL;
	} else {
		local_node->planTree = (Plan *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("transientPlan");
	var_key.val.string.val = strdup("transientPlan");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	local_node->transientPlan = var_value->val.boolean;

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("hasModifyingCTE");
	var_key.val.string.val = strdup("hasModifyingCTE");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	local_node->hasModifyingCTE = var_value->val.boolean;

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("rtable");
	var_key.val.string.val = strdup("rtable");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->rtable = NULL;
	else
		local_node->rtable = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("hasReturning");
	var_key.val.string.val = strdup("hasReturning");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	local_node->hasReturning = var_value->val.boolean;

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("utilityStmt");
	var_key.val.string.val = strdup("utilityStmt");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->utilityStmt = NULL;
	} else {
		local_node->utilityStmt = (Node *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("invalItems");
	var_key.val.string.val = strdup("invalItems");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->invalItems = NULL;
	else
		local_node->invalItems = list_deser(var_value->val.binary.data, false);

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *Aggref_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		Aggref *local_node;
		if (node_cast != NULL)
			local_node = (Aggref *) node_cast;
		else
			local_node = makeNode(Aggref);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("args");
	var_key.val.string.val = strdup("args");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->args = NULL;
	else
		local_node->args = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("aggfnoid");
	var_key.val.string.val = strdup("aggfnoid");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->aggfnoid = DatumGetUInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("location");
	var_key.val.string.val = strdup("location");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->location = DatumGetInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("aggtype");
	var_key.val.string.val = strdup("aggtype");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->aggtype = DatumGetUInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("aggcollid");
	var_key.val.string.val = strdup("aggcollid");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->aggcollid = DatumGetUInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("inputcollid");
	var_key.val.string.val = strdup("inputcollid");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->inputcollid = DatumGetUInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("aggstar");
	var_key.val.string.val = strdup("aggstar");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	local_node->aggstar = var_value->val.boolean;

			}
			{
					
	Expr_deser(container, (void *)&local_node->xpr, -1);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("aggfilter");
	var_key.val.string.val = strdup("aggfilter");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->aggfilter = NULL;
	} else {
		local_node->aggfilter = (Expr *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("aggdirectargs");
	var_key.val.string.val = strdup("aggdirectargs");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->aggdirectargs = NULL;
	else
		local_node->aggdirectargs = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("aggorder");
	var_key.val.string.val = strdup("aggorder");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->aggorder = NULL;
	else
		local_node->aggorder = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("aggkind");
	var_key.val.string.val = strdup("aggkind");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->aggkind = DatumGetInt16(DirectFunctionCall1(numeric_int2, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("agglevelsup");
	var_key.val.string.val = strdup("agglevelsup");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->agglevelsup = DatumGetUInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("aggvariadic");
	var_key.val.string.val = strdup("aggvariadic");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	local_node->aggvariadic = var_value->val.boolean;

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("aggdistinct");
	var_key.val.string.val = strdup("aggdistinct");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->aggdistinct = NULL;
	else
		local_node->aggdistinct = list_deser(var_value->val.binary.data, false);

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *SubqueryScan_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		SubqueryScan *local_node;
		if (node_cast != NULL)
			local_node = (SubqueryScan *) node_cast;
		else
			local_node = makeNode(SubqueryScan);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("subplan");
	var_key.val.string.val = strdup("subplan");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->subplan = NULL;
	} else {
		local_node->subplan = (Plan *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
			{
					
	Scan_deser(container, (void *)&local_node->scan, -1);

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *CreateFunctionStmt_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		CreateFunctionStmt *local_node;
		if (node_cast != NULL)
			local_node = (CreateFunctionStmt *) node_cast;
		else
			local_node = makeNode(CreateFunctionStmt);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("funcname");
	var_key.val.string.val = strdup("funcname");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->funcname = NULL;
	else
		local_node->funcname = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("options");
	var_key.val.string.val = strdup("options");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->options = NULL;
	else
		local_node->options = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("parameters");
	var_key.val.string.val = strdup("parameters");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->parameters = NULL;
	else
		local_node->parameters = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("returnType");
	var_key.val.string.val = strdup("returnType");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->returnType = NULL;
	} else {
		local_node->returnType = (TypeName *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("replace");
	var_key.val.string.val = strdup("replace");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	local_node->replace = var_value->val.boolean;

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("withClause");
	var_key.val.string.val = strdup("withClause");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->withClause = NULL;
	else
		local_node->withClause = list_deser(var_value->val.binary.data, false);

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *CreateCastStmt_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		CreateCastStmt *local_node;
		if (node_cast != NULL)
			local_node = (CreateCastStmt *) node_cast;
		else
			local_node = makeNode(CreateCastStmt);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("targettype");
	var_key.val.string.val = strdup("targettype");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->targettype = NULL;
	} else {
		local_node->targettype = (TypeName *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("func");
	var_key.val.string.val = strdup("func");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->func = NULL;
	} else {
		local_node->func = (FuncWithArgs *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("inout");
	var_key.val.string.val = strdup("inout");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	local_node->inout = var_value->val.boolean;

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("sourcetype");
	var_key.val.string.val = strdup("sourcetype");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->sourcetype = NULL;
	} else {
		local_node->sourcetype = (TypeName *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("context");
	var_key.val.string.val = strdup("context");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->context = DatumGetInt16(DirectFunctionCall1(numeric_int2, NumericGetDatum(var_value->val.numeric)));

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *CteScan_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		CteScan *local_node;
		if (node_cast != NULL)
			local_node = (CteScan *) node_cast;
		else
			local_node = makeNode(CteScan);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("cteParam");
	var_key.val.string.val = strdup("cteParam");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->cteParam = DatumGetInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	Scan_deser(container, (void *)&local_node->scan, -1);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("ctePlanId");
	var_key.val.string.val = strdup("ctePlanId");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->ctePlanId = DatumGetInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *IndexElem_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		IndexElem *local_node;
		if (node_cast != NULL)
			local_node = (IndexElem *) node_cast;
		else
			local_node = makeNode(IndexElem);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("indexcolname");
	var_key.val.string.val = strdup("indexcolname");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					if (var_value->type == jbvNull)
						local_node->indexcolname = NULL;
					else
					{
						char *result = palloc(var_value->val.string.len + 1);
						memcpy(result, var_value->val.string.val, var_value->val.string.len);
						result[var_value->val.string.len] = '\0';
						local_node->indexcolname = result;
					}
			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("collation");
	var_key.val.string.val = strdup("collation");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->collation = NULL;
	else
		local_node->collation = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("ordering");
	var_key.val.string.val = strdup("ordering");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->ordering = DatumGetInt16(DirectFunctionCall1(numeric_int2, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("name");
	var_key.val.string.val = strdup("name");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					if (var_value->type == jbvNull)
						local_node->name = NULL;
					else
					{
						char *result = palloc(var_value->val.string.len + 1);
						memcpy(result, var_value->val.string.val, var_value->val.string.len);
						result[var_value->val.string.len] = '\0';
						local_node->name = result;
					}
			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("opclass");
	var_key.val.string.val = strdup("opclass");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->opclass = NULL;
	else
		local_node->opclass = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("nulls_ordering");
	var_key.val.string.val = strdup("nulls_ordering");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->nulls_ordering = DatumGetInt16(DirectFunctionCall1(numeric_int2, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("expr");
	var_key.val.string.val = strdup("expr");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->expr = NULL;
	} else {
		local_node->expr = (Node *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *CreateFdwStmt_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		CreateFdwStmt *local_node;
		if (node_cast != NULL)
			local_node = (CreateFdwStmt *) node_cast;
		else
			local_node = makeNode(CreateFdwStmt);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("options");
	var_key.val.string.val = strdup("options");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->options = NULL;
	else
		local_node->options = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("fdwname");
	var_key.val.string.val = strdup("fdwname");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					if (var_value->type == jbvNull)
						local_node->fdwname = NULL;
					else
					{
						char *result = palloc(var_value->val.string.len + 1);
						memcpy(result, var_value->val.string.val, var_value->val.string.len);
						result[var_value->val.string.len] = '\0';
						local_node->fdwname = result;
					}
			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("func_options");
	var_key.val.string.val = strdup("func_options");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->func_options = NULL;
	else
		local_node->func_options = list_deser(var_value->val.binary.data, false);

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *NestLoop_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		NestLoop *local_node;
		if (node_cast != NULL)
			local_node = (NestLoop *) node_cast;
		else
			local_node = makeNode(NestLoop);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("nestParams");
	var_key.val.string.val = strdup("nestParams");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->nestParams = NULL;
	else
		local_node->nestParams = list_deser(var_value->val.binary.data, false);

			}
			{
					
	Join_deser(container, (void *)&local_node->join, -1);

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *TypeCast_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		TypeCast *local_node;
		if (node_cast != NULL)
			local_node = (TypeCast *) node_cast;
		else
			local_node = makeNode(TypeCast);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("location");
	var_key.val.string.val = strdup("location");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->location = DatumGetInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("typeName");
	var_key.val.string.val = strdup("typeName");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->typeName = NULL;
	} else {
		local_node->typeName = (TypeName *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("arg");
	var_key.val.string.val = strdup("arg");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->arg = NULL;
	} else {
		local_node->arg = (Node *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *CoerceToDomainValue_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		CoerceToDomainValue *local_node;
		if (node_cast != NULL)
			local_node = (CoerceToDomainValue *) node_cast;
		else
			local_node = makeNode(CoerceToDomainValue);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("collation");
	var_key.val.string.val = strdup("collation");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->collation = DatumGetUInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("location");
	var_key.val.string.val = strdup("location");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->location = DatumGetInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("typeMod");
	var_key.val.string.val = strdup("typeMod");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->typeMod = DatumGetInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	Expr_deser(container, (void *)&local_node->xpr, -1);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("typeId");
	var_key.val.string.val = strdup("typeId");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->typeId = DatumGetUInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *InsertStmt_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		InsertStmt *local_node;
		if (node_cast != NULL)
			local_node = (InsertStmt *) node_cast;
		else
			local_node = makeNode(InsertStmt);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("onConflictClause");
	var_key.val.string.val = strdup("onConflictClause");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->onConflictClause = NULL;
	} else {
		local_node->onConflictClause = (OnConflictClause *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("cols");
	var_key.val.string.val = strdup("cols");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->cols = NULL;
	else
		local_node->cols = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("relation");
	var_key.val.string.val = strdup("relation");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->relation = NULL;
	} else {
		local_node->relation = (RangeVar *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("withClause");
	var_key.val.string.val = strdup("withClause");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->withClause = NULL;
	} else {
		local_node->withClause = (WithClause *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("selectStmt");
	var_key.val.string.val = strdup("selectStmt");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->selectStmt = NULL;
	} else {
		local_node->selectStmt = (Node *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("returningList");
	var_key.val.string.val = strdup("returningList");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->returningList = NULL;
	else
		local_node->returningList = list_deser(var_value->val.binary.data, false);

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *SortBy_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		SortBy *local_node;
		if (node_cast != NULL)
			local_node = (SortBy *) node_cast;
		else
			local_node = makeNode(SortBy);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("location");
	var_key.val.string.val = strdup("location");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->location = DatumGetInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("sortby_dir");
	var_key.val.string.val = strdup("sortby_dir");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->sortby_dir = DatumGetInt16(DirectFunctionCall1(numeric_int2, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("useOp");
	var_key.val.string.val = strdup("useOp");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->useOp = NULL;
	else
		local_node->useOp = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("sortby_nulls");
	var_key.val.string.val = strdup("sortby_nulls");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->sortby_nulls = DatumGetInt16(DirectFunctionCall1(numeric_int2, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("node");
	var_key.val.string.val = strdup("node");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->node = NULL;
	} else {
		local_node->node = (Node *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *ReassignOwnedStmt_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		ReassignOwnedStmt *local_node;
		if (node_cast != NULL)
			local_node = (ReassignOwnedStmt *) node_cast;
		else
			local_node = makeNode(ReassignOwnedStmt);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("newrole");
	var_key.val.string.val = strdup("newrole");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->newrole = NULL;
	} else {
		local_node->newrole = (Node *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("roles");
	var_key.val.string.val = strdup("roles");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->roles = NULL;
	else
		local_node->roles = list_deser(var_value->val.binary.data, false);

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *TypeName_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		TypeName *local_node;
		if (node_cast != NULL)
			local_node = (TypeName *) node_cast;
		else
			local_node = makeNode(TypeName);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("pct_type");
	var_key.val.string.val = strdup("pct_type");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	local_node->pct_type = var_value->val.boolean;

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("setof");
	var_key.val.string.val = strdup("setof");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	local_node->setof = var_value->val.boolean;

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("location");
	var_key.val.string.val = strdup("location");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->location = DatumGetInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("typeOid");
	var_key.val.string.val = strdup("typeOid");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->typeOid = DatumGetUInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("names");
	var_key.val.string.val = strdup("names");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->names = NULL;
	else
		local_node->names = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("typmods");
	var_key.val.string.val = strdup("typmods");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->typmods = NULL;
	else
		local_node->typmods = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("typemod");
	var_key.val.string.val = strdup("typemod");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->typemod = DatumGetInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("arrayBounds");
	var_key.val.string.val = strdup("arrayBounds");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->arrayBounds = NULL;
	else
		local_node->arrayBounds = list_deser(var_value->val.binary.data, false);

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *Constraint_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		Constraint *local_node;
		if (node_cast != NULL)
			local_node = (Constraint *) node_cast;
		else
			local_node = makeNode(Constraint);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("conname");
	var_key.val.string.val = strdup("conname");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					if (var_value->type == jbvNull)
						local_node->conname = NULL;
					else
					{
						char *result = palloc(var_value->val.string.len + 1);
						memcpy(result, var_value->val.string.val, var_value->val.string.len);
						result[var_value->val.string.len] = '\0';
						local_node->conname = result;
					}
			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("indexspace");
	var_key.val.string.val = strdup("indexspace");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					if (var_value->type == jbvNull)
						local_node->indexspace = NULL;
					else
					{
						char *result = palloc(var_value->val.string.len + 1);
						memcpy(result, var_value->val.string.val, var_value->val.string.len);
						result[var_value->val.string.len] = '\0';
						local_node->indexspace = result;
					}
			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("options");
	var_key.val.string.val = strdup("options");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->options = NULL;
	else
		local_node->options = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("cooked_expr");
	var_key.val.string.val = strdup("cooked_expr");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					if (var_value->type == jbvNull)
						local_node->cooked_expr = NULL;
					else
					{
						char *result = palloc(var_value->val.string.len + 1);
						memcpy(result, var_value->val.string.val, var_value->val.string.len);
						result[var_value->val.string.len] = '\0';
						local_node->cooked_expr = result;
					}
			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("fk_upd_action");
	var_key.val.string.val = strdup("fk_upd_action");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->fk_upd_action = DatumGetInt16(DirectFunctionCall1(numeric_int2, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("pktable");
	var_key.val.string.val = strdup("pktable");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->pktable = NULL;
	} else {
		local_node->pktable = (RangeVar *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("is_no_inherit");
	var_key.val.string.val = strdup("is_no_inherit");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	local_node->is_no_inherit = var_value->val.boolean;

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("fk_matchtype");
	var_key.val.string.val = strdup("fk_matchtype");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->fk_matchtype = DatumGetInt16(DirectFunctionCall1(numeric_int2, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("skip_validation");
	var_key.val.string.val = strdup("skip_validation");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	local_node->skip_validation = var_value->val.boolean;

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("indexname");
	var_key.val.string.val = strdup("indexname");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					if (var_value->type == jbvNull)
						local_node->indexname = NULL;
					else
					{
						char *result = palloc(var_value->val.string.len + 1);
						memcpy(result, var_value->val.string.val, var_value->val.string.len);
						result[var_value->val.string.len] = '\0';
						local_node->indexname = result;
					}
			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("location");
	var_key.val.string.val = strdup("location");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->location = DatumGetInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("where_clause");
	var_key.val.string.val = strdup("where_clause");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->where_clause = NULL;
	} else {
		local_node->where_clause = (Node *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("access_method");
	var_key.val.string.val = strdup("access_method");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					if (var_value->type == jbvNull)
						local_node->access_method = NULL;
					else
					{
						char *result = palloc(var_value->val.string.len + 1);
						memcpy(result, var_value->val.string.val, var_value->val.string.len);
						result[var_value->val.string.len] = '\0';
						local_node->access_method = result;
					}
			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("old_conpfeqop");
	var_key.val.string.val = strdup("old_conpfeqop");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->old_conpfeqop = NULL;
	else
		local_node->old_conpfeqop = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("initially_valid");
	var_key.val.string.val = strdup("initially_valid");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	local_node->initially_valid = var_value->val.boolean;

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("pk_attrs");
	var_key.val.string.val = strdup("pk_attrs");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->pk_attrs = NULL;
	else
		local_node->pk_attrs = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("old_pktable_oid");
	var_key.val.string.val = strdup("old_pktable_oid");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->old_pktable_oid = DatumGetUInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("fk_del_action");
	var_key.val.string.val = strdup("fk_del_action");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->fk_del_action = DatumGetInt16(DirectFunctionCall1(numeric_int2, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("initdeferred");
	var_key.val.string.val = strdup("initdeferred");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	local_node->initdeferred = var_value->val.boolean;

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("deferrable");
	var_key.val.string.val = strdup("deferrable");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	local_node->deferrable = var_value->val.boolean;

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("fk_attrs");
	var_key.val.string.val = strdup("fk_attrs");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->fk_attrs = NULL;
	else
		local_node->fk_attrs = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("contype");
	var_key.val.string.val = strdup("contype");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->contype = DatumGetInt16(DirectFunctionCall1(numeric_int2, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("exclusions");
	var_key.val.string.val = strdup("exclusions");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->exclusions = NULL;
	else
		local_node->exclusions = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("keys");
	var_key.val.string.val = strdup("keys");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->keys = NULL;
	else
		local_node->keys = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("raw_expr");
	var_key.val.string.val = strdup("raw_expr");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->raw_expr = NULL;
	} else {
		local_node->raw_expr = (Node *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *OnConflictClause_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		OnConflictClause *local_node;
		if (node_cast != NULL)
			local_node = (OnConflictClause *) node_cast;
		else
			local_node = makeNode(OnConflictClause);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("location");
	var_key.val.string.val = strdup("location");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->location = DatumGetInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("whereClause");
	var_key.val.string.val = strdup("whereClause");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->whereClause = NULL;
	} else {
		local_node->whereClause = (Node *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("targetList");
	var_key.val.string.val = strdup("targetList");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->targetList = NULL;
	else
		local_node->targetList = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("action");
	var_key.val.string.val = strdup("action");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->action = DatumGetInt16(DirectFunctionCall1(numeric_int2, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("infer");
	var_key.val.string.val = strdup("infer");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->infer = NULL;
	} else {
		local_node->infer = (InferClause *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *AlterPolicyStmt_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		AlterPolicyStmt *local_node;
		if (node_cast != NULL)
			local_node = (AlterPolicyStmt *) node_cast;
		else
			local_node = makeNode(AlterPolicyStmt);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("with_check");
	var_key.val.string.val = strdup("with_check");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->with_check = NULL;
	} else {
		local_node->with_check = (Node *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("table");
	var_key.val.string.val = strdup("table");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->table = NULL;
	} else {
		local_node->table = (RangeVar *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("policy_name");
	var_key.val.string.val = strdup("policy_name");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					if (var_value->type == jbvNull)
						local_node->policy_name = NULL;
					else
					{
						char *result = palloc(var_value->val.string.len + 1);
						memcpy(result, var_value->val.string.val, var_value->val.string.len);
						result[var_value->val.string.len] = '\0';
						local_node->policy_name = result;
					}
			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("qual");
	var_key.val.string.val = strdup("qual");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->qual = NULL;
	} else {
		local_node->qual = (Node *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("roles");
	var_key.val.string.val = strdup("roles");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->roles = NULL;
	else
		local_node->roles = list_deser(var_value->val.binary.data, false);

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *GroupingFunc_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		GroupingFunc *local_node;
		if (node_cast != NULL)
			local_node = (GroupingFunc *) node_cast;
		else
			local_node = makeNode(GroupingFunc);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("args");
	var_key.val.string.val = strdup("args");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->args = NULL;
	else
		local_node->args = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("refs");
	var_key.val.string.val = strdup("refs");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->refs = NULL;
	else
		local_node->refs = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("cols");
	var_key.val.string.val = strdup("cols");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->cols = NULL;
	else
		local_node->cols = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("location");
	var_key.val.string.val = strdup("location");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->location = DatumGetInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	Expr_deser(container, (void *)&local_node->xpr, -1);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("agglevelsup");
	var_key.val.string.val = strdup("agglevelsup");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->agglevelsup = DatumGetUInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *SelectStmt_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		SelectStmt *local_node;
		if (node_cast != NULL)
			local_node = (SelectStmt *) node_cast;
		else
			local_node = makeNode(SelectStmt);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("withClause");
	var_key.val.string.val = strdup("withClause");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->withClause = NULL;
	} else {
		local_node->withClause = (WithClause *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("larg");
	var_key.val.string.val = strdup("larg");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->larg = NULL;
	} else {
		local_node->larg = (SelectStmt *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("op");
	var_key.val.string.val = strdup("op");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->op = DatumGetInt16(DirectFunctionCall1(numeric_int2, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("intoClause");
	var_key.val.string.val = strdup("intoClause");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->intoClause = NULL;
	} else {
		local_node->intoClause = (IntoClause *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("all");
	var_key.val.string.val = strdup("all");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	local_node->all = var_value->val.boolean;

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("whereClause");
	var_key.val.string.val = strdup("whereClause");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->whereClause = NULL;
	} else {
		local_node->whereClause = (Node *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("sortClause");
	var_key.val.string.val = strdup("sortClause");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->sortClause = NULL;
	else
		local_node->sortClause = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("limitCount");
	var_key.val.string.val = strdup("limitCount");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->limitCount = NULL;
	} else {
		local_node->limitCount = (Node *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("valuesLists");
	var_key.val.string.val = strdup("valuesLists");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->valuesLists = NULL;
	else
		local_node->valuesLists = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("targetList");
	var_key.val.string.val = strdup("targetList");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->targetList = NULL;
	else
		local_node->targetList = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("groupClause");
	var_key.val.string.val = strdup("groupClause");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->groupClause = NULL;
	else
		local_node->groupClause = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("fromClause");
	var_key.val.string.val = strdup("fromClause");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->fromClause = NULL;
	else
		local_node->fromClause = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("lockingClause");
	var_key.val.string.val = strdup("lockingClause");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->lockingClause = NULL;
	else
		local_node->lockingClause = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("windowClause");
	var_key.val.string.val = strdup("windowClause");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->windowClause = NULL;
	else
		local_node->windowClause = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("distinctClause");
	var_key.val.string.val = strdup("distinctClause");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->distinctClause = NULL;
	else
		local_node->distinctClause = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("havingClause");
	var_key.val.string.val = strdup("havingClause");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->havingClause = NULL;
	} else {
		local_node->havingClause = (Node *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("limitOffset");
	var_key.val.string.val = strdup("limitOffset");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->limitOffset = NULL;
	} else {
		local_node->limitOffset = (Node *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("rarg");
	var_key.val.string.val = strdup("rarg");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->rarg = NULL;
	} else {
		local_node->rarg = (SelectStmt *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *CopyStmt_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		CopyStmt *local_node;
		if (node_cast != NULL)
			local_node = (CopyStmt *) node_cast;
		else
			local_node = makeNode(CopyStmt);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("is_from");
	var_key.val.string.val = strdup("is_from");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	local_node->is_from = var_value->val.boolean;

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("is_program");
	var_key.val.string.val = strdup("is_program");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	local_node->is_program = var_value->val.boolean;

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("options");
	var_key.val.string.val = strdup("options");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->options = NULL;
	else
		local_node->options = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("relation");
	var_key.val.string.val = strdup("relation");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->relation = NULL;
	} else {
		local_node->relation = (RangeVar *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("filename");
	var_key.val.string.val = strdup("filename");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					if (var_value->type == jbvNull)
						local_node->filename = NULL;
					else
					{
						char *result = palloc(var_value->val.string.len + 1);
						memcpy(result, var_value->val.string.val, var_value->val.string.len);
						result[var_value->val.string.len] = '\0';
						local_node->filename = result;
					}
			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("attlist");
	var_key.val.string.val = strdup("attlist");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->attlist = NULL;
	else
		local_node->attlist = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("query");
	var_key.val.string.val = strdup("query");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->query = NULL;
	} else {
		local_node->query = (Node *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *ArrayExpr_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		ArrayExpr *local_node;
		if (node_cast != NULL)
			local_node = (ArrayExpr *) node_cast;
		else
			local_node = makeNode(ArrayExpr);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("array_typeid");
	var_key.val.string.val = strdup("array_typeid");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->array_typeid = DatumGetUInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("location");
	var_key.val.string.val = strdup("location");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->location = DatumGetInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("multidims");
	var_key.val.string.val = strdup("multidims");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	local_node->multidims = var_value->val.boolean;

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("array_collid");
	var_key.val.string.val = strdup("array_collid");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->array_collid = DatumGetUInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("element_typeid");
	var_key.val.string.val = strdup("element_typeid");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->element_typeid = DatumGetUInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("elements");
	var_key.val.string.val = strdup("elements");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->elements = NULL;
	else
		local_node->elements = list_deser(var_value->val.binary.data, false);

			}
			{
					
	Expr_deser(container, (void *)&local_node->xpr, -1);

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *InferenceElem_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		InferenceElem *local_node;
		if (node_cast != NULL)
			local_node = (InferenceElem *) node_cast;
		else
			local_node = makeNode(InferenceElem);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("infercollid");
	var_key.val.string.val = strdup("infercollid");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->infercollid = DatumGetUInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("inferopclass");
	var_key.val.string.val = strdup("inferopclass");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->inferopclass = DatumGetUInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	Expr_deser(container, (void *)&local_node->xpr, -1);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("expr");
	var_key.val.string.val = strdup("expr");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->expr = NULL;
	} else {
		local_node->expr = (Node *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *BitmapOr_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		BitmapOr *local_node;
		if (node_cast != NULL)
			local_node = (BitmapOr *) node_cast;
		else
			local_node = makeNode(BitmapOr);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("bitmapplans");
	var_key.val.string.val = strdup("bitmapplans");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->bitmapplans = NULL;
	else
		local_node->bitmapplans = list_deser(var_value->val.binary.data, false);

			}
			{
					
	Plan_deser(container, (void *)&local_node->plan, -1);

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *FuncExpr_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		FuncExpr *local_node;
		if (node_cast != NULL)
			local_node = (FuncExpr *) node_cast;
		else
			local_node = makeNode(FuncExpr);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("args");
	var_key.val.string.val = strdup("args");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->args = NULL;
	else
		local_node->args = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("funcid");
	var_key.val.string.val = strdup("funcid");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->funcid = DatumGetUInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("location");
	var_key.val.string.val = strdup("location");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->location = DatumGetInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("funcformat");
	var_key.val.string.val = strdup("funcformat");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->funcformat = DatumGetInt16(DirectFunctionCall1(numeric_int2, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("funcvariadic");
	var_key.val.string.val = strdup("funcvariadic");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	local_node->funcvariadic = var_value->val.boolean;

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("inputcollid");
	var_key.val.string.val = strdup("inputcollid");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->inputcollid = DatumGetUInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("funcresulttype");
	var_key.val.string.val = strdup("funcresulttype");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->funcresulttype = DatumGetUInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("funcretset");
	var_key.val.string.val = strdup("funcretset");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	local_node->funcretset = var_value->val.boolean;

			}
			{
					
	Expr_deser(container, (void *)&local_node->xpr, -1);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("funccollid");
	var_key.val.string.val = strdup("funccollid");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->funccollid = DatumGetUInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *AlterUserMappingStmt_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		AlterUserMappingStmt *local_node;
		if (node_cast != NULL)
			local_node = (AlterUserMappingStmt *) node_cast;
		else
			local_node = makeNode(AlterUserMappingStmt);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("servername");
	var_key.val.string.val = strdup("servername");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					if (var_value->type == jbvNull)
						local_node->servername = NULL;
					else
					{
						char *result = palloc(var_value->val.string.len + 1);
						memcpy(result, var_value->val.string.val, var_value->val.string.len);
						result[var_value->val.string.len] = '\0';
						local_node->servername = result;
					}
			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("options");
	var_key.val.string.val = strdup("options");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->options = NULL;
	else
		local_node->options = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("user");
	var_key.val.string.val = strdup("user");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->user = NULL;
	} else {
		local_node->user = (Node *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *SetOp_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		SetOp *local_node;
		if (node_cast != NULL)
			local_node = (SetOp *) node_cast;
		else
			local_node = makeNode(SetOp);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("strategy");
	var_key.val.string.val = strdup("strategy");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->strategy = DatumGetInt16(DirectFunctionCall1(numeric_int2, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("dupColIdx");
	var_key.val.string.val = strdup("dupColIdx");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

						
	{
		int type;
		int i = 0;
		JsonbValue v;
		JsonbIterator *it = JsonbIteratorInit(var_value->val.binary.data);
			local_node->numCols = it->nElems;
		local_node->dupColIdx = (AttrNumber*) palloc(sizeof(AttrNumber)*it->nElems);
		while ((type = JsonbIteratorNext(&it, &v, true)) != WJB_DONE)
		{
			if (type == WJB_ELEM)
			{
					
		local_node->dupColIdx[i] = DatumGetInt16(DirectFunctionCall1(numeric_int2, NumericGetDatum(v.val.numeric)));

				i++;
			}
		}
	}

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("cmd");
	var_key.val.string.val = strdup("cmd");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->cmd = DatumGetInt16(DirectFunctionCall1(numeric_int2, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("dupOperators");
	var_key.val.string.val = strdup("dupOperators");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

						
	{
		int type;
		int i = 0;
		JsonbValue v;
		JsonbIterator *it = JsonbIteratorInit(var_value->val.binary.data);
			local_node->numCols = it->nElems;
		local_node->dupOperators = (Oid*) palloc(sizeof(Oid)*it->nElems);
		while ((type = JsonbIteratorNext(&it, &v, true)) != WJB_DONE)
		{
			if (type == WJB_ELEM)
			{
					
		local_node->dupOperators[i] = DatumGetUInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(v.val.numeric)));

				i++;
			}
		}
	}

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("numGroups");
	var_key.val.string.val = strdup("numGroups");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
#ifdef USE_FLOAT8_BYVAL
	local_node->numGroups = DatumGetInt64(DirectFunctionCall1(numeric_int8, NumericGetDatum(var_value->val.numeric)));
#else
	local_node->numGroups = DatumGetInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));
#endif

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("firstFlag");
	var_key.val.string.val = strdup("firstFlag");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->firstFlag = DatumGetInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	Plan_deser(container, (void *)&local_node->plan, -1);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("numCols");
	var_key.val.string.val = strdup("numCols");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->numCols = DatumGetInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("flagColIdx");
	var_key.val.string.val = strdup("flagColIdx");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->flagColIdx = DatumGetInt16(DirectFunctionCall1(numeric_int2, NumericGetDatum(var_value->val.numeric)));

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *AlterTSConfigurationStmt_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		AlterTSConfigurationStmt *local_node;
		if (node_cast != NULL)
			local_node = (AlterTSConfigurationStmt *) node_cast;
		else
			local_node = makeNode(AlterTSConfigurationStmt);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("missing_ok");
	var_key.val.string.val = strdup("missing_ok");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	local_node->missing_ok = var_value->val.boolean;

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("replace");
	var_key.val.string.val = strdup("replace");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	local_node->replace = var_value->val.boolean;

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("override");
	var_key.val.string.val = strdup("override");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	local_node->override = var_value->val.boolean;

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("dicts");
	var_key.val.string.val = strdup("dicts");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->dicts = NULL;
	else
		local_node->dicts = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("tokentype");
	var_key.val.string.val = strdup("tokentype");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->tokentype = NULL;
	else
		local_node->tokentype = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("cfgname");
	var_key.val.string.val = strdup("cfgname");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->cfgname = NULL;
	else
		local_node->cfgname = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("kind");
	var_key.val.string.val = strdup("kind");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->kind = DatumGetInt16(DirectFunctionCall1(numeric_int2, NumericGetDatum(var_value->val.numeric)));

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *AlterRoleStmt_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		AlterRoleStmt *local_node;
		if (node_cast != NULL)
			local_node = (AlterRoleStmt *) node_cast;
		else
			local_node = makeNode(AlterRoleStmt);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("action");
	var_key.val.string.val = strdup("action");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->action = DatumGetInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("options");
	var_key.val.string.val = strdup("options");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->options = NULL;
	else
		local_node->options = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("role");
	var_key.val.string.val = strdup("role");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->role = NULL;
	} else {
		local_node->role = (Node *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *Sort_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		Sort *local_node;
		if (node_cast != NULL)
			local_node = (Sort *) node_cast;
		else
			local_node = makeNode(Sort);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("collations");
	var_key.val.string.val = strdup("collations");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

						
	{
		int type;
		int i = 0;
		JsonbValue v;
		JsonbIterator *it = JsonbIteratorInit(var_value->val.binary.data);
			local_node->numCols = it->nElems;
		local_node->collations = (Oid*) palloc(sizeof(Oid)*it->nElems);
		while ((type = JsonbIteratorNext(&it, &v, true)) != WJB_DONE)
		{
			if (type == WJB_ELEM)
			{
					
		local_node->collations[i] = DatumGetUInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(v.val.numeric)));

				i++;
			}
		}
	}

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("sortColIdx");
	var_key.val.string.val = strdup("sortColIdx");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

						
	{
		int type;
		int i = 0;
		JsonbValue v;
		JsonbIterator *it = JsonbIteratorInit(var_value->val.binary.data);
			local_node->numCols = it->nElems;
		local_node->sortColIdx = (AttrNumber*) palloc(sizeof(AttrNumber)*it->nElems);
		while ((type = JsonbIteratorNext(&it, &v, true)) != WJB_DONE)
		{
			if (type == WJB_ELEM)
			{
					
		local_node->sortColIdx[i] = DatumGetInt16(DirectFunctionCall1(numeric_int2, NumericGetDatum(v.val.numeric)));

				i++;
			}
		}
	}

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("nullsFirst");
	var_key.val.string.val = strdup("nullsFirst");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

						
	{
		int type;
		int i = 0;
		JsonbValue v;
		JsonbIterator *it = JsonbIteratorInit(var_value->val.binary.data);
			local_node->numCols = it->nElems;
		local_node->nullsFirst = (bool*) palloc(sizeof(bool)*it->nElems);
		while ((type = JsonbIteratorNext(&it, &v, true)) != WJB_DONE)
		{
			if (type == WJB_ELEM)
			{
					
	local_node->nullsFirst[i] = v.val.boolean;

				i++;
			}
		}
	}

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("sortOperators");
	var_key.val.string.val = strdup("sortOperators");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

						
	{
		int type;
		int i = 0;
		JsonbValue v;
		JsonbIterator *it = JsonbIteratorInit(var_value->val.binary.data);
			local_node->numCols = it->nElems;
		local_node->sortOperators = (Oid*) palloc(sizeof(Oid)*it->nElems);
		while ((type = JsonbIteratorNext(&it, &v, true)) != WJB_DONE)
		{
			if (type == WJB_ELEM)
			{
					
		local_node->sortOperators[i] = DatumGetUInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(v.val.numeric)));

				i++;
			}
		}
	}

			}
			{
					
	Plan_deser(container, (void *)&local_node->plan, -1);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("numCols");
	var_key.val.string.val = strdup("numCols");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->numCols = DatumGetInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *DiscardStmt_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		DiscardStmt *local_node;
		if (node_cast != NULL)
			local_node = (DiscardStmt *) node_cast;
		else
			local_node = makeNode(DiscardStmt);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("target");
	var_key.val.string.val = strdup("target");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->target = DatumGetInt16(DirectFunctionCall1(numeric_int2, NumericGetDatum(var_value->val.numeric)));

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *CreateForeignServerStmt_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		CreateForeignServerStmt *local_node;
		if (node_cast != NULL)
			local_node = (CreateForeignServerStmt *) node_cast;
		else
			local_node = makeNode(CreateForeignServerStmt);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("options");
	var_key.val.string.val = strdup("options");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->options = NULL;
	else
		local_node->options = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("version");
	var_key.val.string.val = strdup("version");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					if (var_value->type == jbvNull)
						local_node->version = NULL;
					else
					{
						char *result = palloc(var_value->val.string.len + 1);
						memcpy(result, var_value->val.string.val, var_value->val.string.len);
						result[var_value->val.string.len] = '\0';
						local_node->version = result;
					}
			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("servername");
	var_key.val.string.val = strdup("servername");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					if (var_value->type == jbvNull)
						local_node->servername = NULL;
					else
					{
						char *result = palloc(var_value->val.string.len + 1);
						memcpy(result, var_value->val.string.val, var_value->val.string.len);
						result[var_value->val.string.len] = '\0';
						local_node->servername = result;
					}
			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("fdwname");
	var_key.val.string.val = strdup("fdwname");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					if (var_value->type == jbvNull)
						local_node->fdwname = NULL;
					else
					{
						char *result = palloc(var_value->val.string.len + 1);
						memcpy(result, var_value->val.string.val, var_value->val.string.len);
						result[var_value->val.string.len] = '\0';
						local_node->fdwname = result;
					}
			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("servertype");
	var_key.val.string.val = strdup("servertype");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					if (var_value->type == jbvNull)
						local_node->servertype = NULL;
					else
					{
						char *result = palloc(var_value->val.string.len + 1);
						memcpy(result, var_value->val.string.val, var_value->val.string.len);
						result[var_value->val.string.len] = '\0';
						local_node->servertype = result;
					}
			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *CaseExpr_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		CaseExpr *local_node;
		if (node_cast != NULL)
			local_node = (CaseExpr *) node_cast;
		else
			local_node = makeNode(CaseExpr);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("args");
	var_key.val.string.val = strdup("args");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->args = NULL;
	else
		local_node->args = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("defresult");
	var_key.val.string.val = strdup("defresult");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->defresult = NULL;
	} else {
		local_node->defresult = (Expr *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("arg");
	var_key.val.string.val = strdup("arg");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->arg = NULL;
	} else {
		local_node->arg = (Expr *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("casecollid");
	var_key.val.string.val = strdup("casecollid");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->casecollid = DatumGetUInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("casetype");
	var_key.val.string.val = strdup("casetype");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->casetype = DatumGetUInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("location");
	var_key.val.string.val = strdup("location");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->location = DatumGetInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	Expr_deser(container, (void *)&local_node->xpr, -1);

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *CreateExtensionStmt_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		CreateExtensionStmt *local_node;
		if (node_cast != NULL)
			local_node = (CreateExtensionStmt *) node_cast;
		else
			local_node = makeNode(CreateExtensionStmt);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("if_not_exists");
	var_key.val.string.val = strdup("if_not_exists");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	local_node->if_not_exists = var_value->val.boolean;

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("extname");
	var_key.val.string.val = strdup("extname");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					if (var_value->type == jbvNull)
						local_node->extname = NULL;
					else
					{
						char *result = palloc(var_value->val.string.len + 1);
						memcpy(result, var_value->val.string.val, var_value->val.string.len);
						result[var_value->val.string.len] = '\0';
						local_node->extname = result;
					}
			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("options");
	var_key.val.string.val = strdup("options");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->options = NULL;
	else
		local_node->options = list_deser(var_value->val.binary.data, false);

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *CommonTableExpr_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		CommonTableExpr *local_node;
		if (node_cast != NULL)
			local_node = (CommonTableExpr *) node_cast;
		else
			local_node = makeNode(CommonTableExpr);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("ctequery");
	var_key.val.string.val = strdup("ctequery");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->ctequery = NULL;
	} else {
		local_node->ctequery = (Node *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("ctecoltypmods");
	var_key.val.string.val = strdup("ctecoltypmods");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->ctecoltypmods = NULL;
	else
		local_node->ctecoltypmods = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("ctecolcollations");
	var_key.val.string.val = strdup("ctecolcollations");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->ctecolcollations = NULL;
	else
		local_node->ctecolcollations = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("ctecoltypes");
	var_key.val.string.val = strdup("ctecoltypes");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->ctecoltypes = NULL;
	else
		local_node->ctecoltypes = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("aliascolnames");
	var_key.val.string.val = strdup("aliascolnames");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->aliascolnames = NULL;
	else
		local_node->aliascolnames = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("ctecolnames");
	var_key.val.string.val = strdup("ctecolnames");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->ctecolnames = NULL;
	else
		local_node->ctecolnames = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("ctename");
	var_key.val.string.val = strdup("ctename");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					if (var_value->type == jbvNull)
						local_node->ctename = NULL;
					else
					{
						char *result = palloc(var_value->val.string.len + 1);
						memcpy(result, var_value->val.string.val, var_value->val.string.len);
						result[var_value->val.string.len] = '\0';
						local_node->ctename = result;
					}
			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("location");
	var_key.val.string.val = strdup("location");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->location = DatumGetInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("cterefcount");
	var_key.val.string.val = strdup("cterefcount");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->cterefcount = DatumGetInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("cterecursive");
	var_key.val.string.val = strdup("cterecursive");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	local_node->cterecursive = var_value->val.boolean;

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *RenameStmt_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		RenameStmt *local_node;
		if (node_cast != NULL)
			local_node = (RenameStmt *) node_cast;
		else
			local_node = makeNode(RenameStmt);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("renameType");
	var_key.val.string.val = strdup("renameType");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->renameType = DatumGetInt16(DirectFunctionCall1(numeric_int2, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("newname");
	var_key.val.string.val = strdup("newname");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					if (var_value->type == jbvNull)
						local_node->newname = NULL;
					else
					{
						char *result = palloc(var_value->val.string.len + 1);
						memcpy(result, var_value->val.string.val, var_value->val.string.len);
						result[var_value->val.string.len] = '\0';
						local_node->newname = result;
					}
			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("objarg");
	var_key.val.string.val = strdup("objarg");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->objarg = NULL;
	else
		local_node->objarg = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("relationType");
	var_key.val.string.val = strdup("relationType");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->relationType = DatumGetInt16(DirectFunctionCall1(numeric_int2, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("relation");
	var_key.val.string.val = strdup("relation");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->relation = NULL;
	} else {
		local_node->relation = (RangeVar *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("subname");
	var_key.val.string.val = strdup("subname");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					if (var_value->type == jbvNull)
						local_node->subname = NULL;
					else
					{
						char *result = palloc(var_value->val.string.len + 1);
						memcpy(result, var_value->val.string.val, var_value->val.string.len);
						result[var_value->val.string.len] = '\0';
						local_node->subname = result;
					}
			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("object");
	var_key.val.string.val = strdup("object");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->object = NULL;
	else
		local_node->object = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("behavior");
	var_key.val.string.val = strdup("behavior");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->behavior = DatumGetInt16(DirectFunctionCall1(numeric_int2, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("missing_ok");
	var_key.val.string.val = strdup("missing_ok");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	local_node->missing_ok = var_value->val.boolean;

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *A_Star_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		A_Star *local_node;
		if (node_cast != NULL)
			local_node = (A_Star *) node_cast;
		else
			local_node = makeNode(A_Star);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *UnlistenStmt_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		UnlistenStmt *local_node;
		if (node_cast != NULL)
			local_node = (UnlistenStmt *) node_cast;
		else
			local_node = makeNode(UnlistenStmt);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("conditionname");
	var_key.val.string.val = strdup("conditionname");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					if (var_value->type == jbvNull)
						local_node->conditionname = NULL;
					else
					{
						char *result = palloc(var_value->val.string.len + 1);
						memcpy(result, var_value->val.string.val, var_value->val.string.len);
						result[var_value->val.string.len] = '\0';
						local_node->conditionname = result;
					}
			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *GroupingSet_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		GroupingSet *local_node;
		if (node_cast != NULL)
			local_node = (GroupingSet *) node_cast;
		else
			local_node = makeNode(GroupingSet);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("location");
	var_key.val.string.val = strdup("location");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->location = DatumGetInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("kind");
	var_key.val.string.val = strdup("kind");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->kind = DatumGetInt16(DirectFunctionCall1(numeric_int2, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("content");
	var_key.val.string.val = strdup("content");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->content = NULL;
	else
		local_node->content = list_deser(var_value->val.binary.data, false);

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *BoolExpr_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		BoolExpr *local_node;
		if (node_cast != NULL)
			local_node = (BoolExpr *) node_cast;
		else
			local_node = makeNode(BoolExpr);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("args");
	var_key.val.string.val = strdup("args");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->args = NULL;
	else
		local_node->args = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("boolop");
	var_key.val.string.val = strdup("boolop");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->boolop = DatumGetInt16(DirectFunctionCall1(numeric_int2, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("location");
	var_key.val.string.val = strdup("location");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->location = DatumGetInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	Expr_deser(container, (void *)&local_node->xpr, -1);

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *BitmapAnd_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		BitmapAnd *local_node;
		if (node_cast != NULL)
			local_node = (BitmapAnd *) node_cast;
		else
			local_node = makeNode(BitmapAnd);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("bitmapplans");
	var_key.val.string.val = strdup("bitmapplans");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->bitmapplans = NULL;
	else
		local_node->bitmapplans = list_deser(var_value->val.binary.data, false);

			}
			{
					
	Plan_deser(container, (void *)&local_node->plan, -1);

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *RowExpr_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		RowExpr *local_node;
		if (node_cast != NULL)
			local_node = (RowExpr *) node_cast;
		else
			local_node = makeNode(RowExpr);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("args");
	var_key.val.string.val = strdup("args");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->args = NULL;
	else
		local_node->args = list_deser(var_value->val.binary.data, false);

			}
			{
					
	Expr_deser(container, (void *)&local_node->xpr, -1);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("row_format");
	var_key.val.string.val = strdup("row_format");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->row_format = DatumGetInt16(DirectFunctionCall1(numeric_int2, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("colnames");
	var_key.val.string.val = strdup("colnames");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->colnames = NULL;
	else
		local_node->colnames = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("location");
	var_key.val.string.val = strdup("location");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->location = DatumGetInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("row_typeid");
	var_key.val.string.val = strdup("row_typeid");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->row_typeid = DatumGetUInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *UpdateStmt_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		UpdateStmt *local_node;
		if (node_cast != NULL)
			local_node = (UpdateStmt *) node_cast;
		else
			local_node = makeNode(UpdateStmt);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("fromClause");
	var_key.val.string.val = strdup("fromClause");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->fromClause = NULL;
	else
		local_node->fromClause = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("whereClause");
	var_key.val.string.val = strdup("whereClause");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->whereClause = NULL;
	} else {
		local_node->whereClause = (Node *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("targetList");
	var_key.val.string.val = strdup("targetList");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->targetList = NULL;
	else
		local_node->targetList = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("relation");
	var_key.val.string.val = strdup("relation");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->relation = NULL;
	} else {
		local_node->relation = (RangeVar *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("withClause");
	var_key.val.string.val = strdup("withClause");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->withClause = NULL;
	} else {
		local_node->withClause = (WithClause *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("returningList");
	var_key.val.string.val = strdup("returningList");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->returningList = NULL;
	else
		local_node->returningList = list_deser(var_value->val.binary.data, false);

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *CollateExpr_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		CollateExpr *local_node;
		if (node_cast != NULL)
			local_node = (CollateExpr *) node_cast;
		else
			local_node = makeNode(CollateExpr);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("location");
	var_key.val.string.val = strdup("location");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->location = DatumGetInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("arg");
	var_key.val.string.val = strdup("arg");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->arg = NULL;
	} else {
		local_node->arg = (Expr *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("collOid");
	var_key.val.string.val = strdup("collOid");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->collOid = DatumGetUInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	Expr_deser(container, (void *)&local_node->xpr, -1);

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *DropTableSpaceStmt_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		DropTableSpaceStmt *local_node;
		if (node_cast != NULL)
			local_node = (DropTableSpaceStmt *) node_cast;
		else
			local_node = makeNode(DropTableSpaceStmt);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("tablespacename");
	var_key.val.string.val = strdup("tablespacename");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					if (var_value->type == jbvNull)
						local_node->tablespacename = NULL;
					else
					{
						char *result = palloc(var_value->val.string.len + 1);
						memcpy(result, var_value->val.string.val, var_value->val.string.len);
						result[var_value->val.string.len] = '\0';
						local_node->tablespacename = result;
					}
			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("missing_ok");
	var_key.val.string.val = strdup("missing_ok");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	local_node->missing_ok = var_value->val.boolean;

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *DeallocateStmt_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		DeallocateStmt *local_node;
		if (node_cast != NULL)
			local_node = (DeallocateStmt *) node_cast;
		else
			local_node = makeNode(DeallocateStmt);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("name");
	var_key.val.string.val = strdup("name");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					if (var_value->type == jbvNull)
						local_node->name = NULL;
					else
					{
						char *result = palloc(var_value->val.string.len + 1);
						memcpy(result, var_value->val.string.val, var_value->val.string.len);
						result[var_value->val.string.len] = '\0';
						local_node->name = result;
					}
			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *CreatedbStmt_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		CreatedbStmt *local_node;
		if (node_cast != NULL)
			local_node = (CreatedbStmt *) node_cast;
		else
			local_node = makeNode(CreatedbStmt);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("options");
	var_key.val.string.val = strdup("options");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->options = NULL;
	else
		local_node->options = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("dbname");
	var_key.val.string.val = strdup("dbname");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					if (var_value->type == jbvNull)
						local_node->dbname = NULL;
					else
					{
						char *result = palloc(var_value->val.string.len + 1);
						memcpy(result, var_value->val.string.val, var_value->val.string.len);
						result[var_value->val.string.len] = '\0';
						local_node->dbname = result;
					}
			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *Hash_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		Hash *local_node;
		if (node_cast != NULL)
			local_node = (Hash *) node_cast;
		else
			local_node = makeNode(Hash);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("skewColTypmod");
	var_key.val.string.val = strdup("skewColTypmod");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->skewColTypmod = DatumGetInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	Plan_deser(container, (void *)&local_node->plan, -1);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("skewInherit");
	var_key.val.string.val = strdup("skewInherit");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	local_node->skewInherit = var_value->val.boolean;

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("skewColType");
	var_key.val.string.val = strdup("skewColType");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->skewColType = DatumGetUInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("skewTable");
	var_key.val.string.val = strdup("skewTable");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->skewTable = DatumGetUInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("skewColumn");
	var_key.val.string.val = strdup("skewColumn");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->skewColumn = DatumGetInt16(DirectFunctionCall1(numeric_int2, NumericGetDatum(var_value->val.numeric)));

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *CurrentOfExpr_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		CurrentOfExpr *local_node;
		if (node_cast != NULL)
			local_node = (CurrentOfExpr *) node_cast;
		else
			local_node = makeNode(CurrentOfExpr);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("cursor_param");
	var_key.val.string.val = strdup("cursor_param");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->cursor_param = DatumGetInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("cursor_name");
	var_key.val.string.val = strdup("cursor_name");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					if (var_value->type == jbvNull)
						local_node->cursor_name = NULL;
					else
					{
						char *result = palloc(var_value->val.string.len + 1);
						memcpy(result, var_value->val.string.val, var_value->val.string.len);
						result[var_value->val.string.len] = '\0';
						local_node->cursor_name = result;
					}
			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("cvarno");
	var_key.val.string.val = strdup("cvarno");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->cvarno = DatumGetUInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	Expr_deser(container, (void *)&local_node->xpr, -1);

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *WindowDef_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		WindowDef *local_node;
		if (node_cast != NULL)
			local_node = (WindowDef *) node_cast;
		else
			local_node = makeNode(WindowDef);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("location");
	var_key.val.string.val = strdup("location");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->location = DatumGetInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("refname");
	var_key.val.string.val = strdup("refname");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					if (var_value->type == jbvNull)
						local_node->refname = NULL;
					else
					{
						char *result = palloc(var_value->val.string.len + 1);
						memcpy(result, var_value->val.string.val, var_value->val.string.len);
						result[var_value->val.string.len] = '\0';
						local_node->refname = result;
					}
			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("partitionClause");
	var_key.val.string.val = strdup("partitionClause");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->partitionClause = NULL;
	else
		local_node->partitionClause = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("startOffset");
	var_key.val.string.val = strdup("startOffset");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->startOffset = NULL;
	} else {
		local_node->startOffset = (Node *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("orderClause");
	var_key.val.string.val = strdup("orderClause");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->orderClause = NULL;
	else
		local_node->orderClause = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("name");
	var_key.val.string.val = strdup("name");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					if (var_value->type == jbvNull)
						local_node->name = NULL;
					else
					{
						char *result = palloc(var_value->val.string.len + 1);
						memcpy(result, var_value->val.string.val, var_value->val.string.len);
						result[var_value->val.string.len] = '\0';
						local_node->name = result;
					}
			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("frameOptions");
	var_key.val.string.val = strdup("frameOptions");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->frameOptions = DatumGetInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("endOffset");
	var_key.val.string.val = strdup("endOffset");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->endOffset = NULL;
	} else {
		local_node->endOffset = (Node *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *AlterFunctionStmt_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		AlterFunctionStmt *local_node;
		if (node_cast != NULL)
			local_node = (AlterFunctionStmt *) node_cast;
		else
			local_node = makeNode(AlterFunctionStmt);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("func");
	var_key.val.string.val = strdup("func");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->func = NULL;
	} else {
		local_node->func = (FuncWithArgs *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("actions");
	var_key.val.string.val = strdup("actions");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->actions = NULL;
	else
		local_node->actions = list_deser(var_value->val.binary.data, false);

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *ExplainStmt_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		ExplainStmt *local_node;
		if (node_cast != NULL)
			local_node = (ExplainStmt *) node_cast;
		else
			local_node = makeNode(ExplainStmt);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("options");
	var_key.val.string.val = strdup("options");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->options = NULL;
	else
		local_node->options = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("query");
	var_key.val.string.val = strdup("query");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->query = NULL;
	} else {
		local_node->query = (Node *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *AlterDatabaseStmt_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		AlterDatabaseStmt *local_node;
		if (node_cast != NULL)
			local_node = (AlterDatabaseStmt *) node_cast;
		else
			local_node = makeNode(AlterDatabaseStmt);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("options");
	var_key.val.string.val = strdup("options");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->options = NULL;
	else
		local_node->options = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("dbname");
	var_key.val.string.val = strdup("dbname");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					if (var_value->type == jbvNull)
						local_node->dbname = NULL;
					else
					{
						char *result = palloc(var_value->val.string.len + 1);
						memcpy(result, var_value->val.string.val, var_value->val.string.len);
						result[var_value->val.string.len] = '\0';
						local_node->dbname = result;
					}
			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *ParamRef_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		ParamRef *local_node;
		if (node_cast != NULL)
			local_node = (ParamRef *) node_cast;
		else
			local_node = makeNode(ParamRef);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("number");
	var_key.val.string.val = strdup("number");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->number = DatumGetInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("location");
	var_key.val.string.val = strdup("location");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->location = DatumGetInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *CreateForeignTableStmt_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		CreateForeignTableStmt *local_node;
		if (node_cast != NULL)
			local_node = (CreateForeignTableStmt *) node_cast;
		else
			local_node = makeNode(CreateForeignTableStmt);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("options");
	var_key.val.string.val = strdup("options");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->options = NULL;
	else
		local_node->options = list_deser(var_value->val.binary.data, false);

			}
			{
					
	CreateStmt_deser(container, (void *)&local_node->base, -1);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("servername");
	var_key.val.string.val = strdup("servername");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					if (var_value->type == jbvNull)
						local_node->servername = NULL;
					else
					{
						char *result = palloc(var_value->val.string.len + 1);
						memcpy(result, var_value->val.string.val, var_value->val.string.len);
						result[var_value->val.string.len] = '\0';
						local_node->servername = result;
					}
			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *BitmapHeapScan_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		BitmapHeapScan *local_node;
		if (node_cast != NULL)
			local_node = (BitmapHeapScan *) node_cast;
		else
			local_node = makeNode(BitmapHeapScan);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	Scan_deser(container, (void *)&local_node->scan, -1);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("bitmapqualorig");
	var_key.val.string.val = strdup("bitmapqualorig");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->bitmapqualorig = NULL;
	else
		local_node->bitmapqualorig = list_deser(var_value->val.binary.data, false);

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *Scan_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		Scan *local_node;
		if (node_cast != NULL)
			local_node = (Scan *) node_cast;
		else
			local_node = makeNode(Scan);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	Plan_deser(container, (void *)&local_node->plan, -1);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("scanrelid");
	var_key.val.string.val = strdup("scanrelid");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->scanrelid = DatumGetUInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *AlterTableSpaceOptionsStmt_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		AlterTableSpaceOptionsStmt *local_node;
		if (node_cast != NULL)
			local_node = (AlterTableSpaceOptionsStmt *) node_cast;
		else
			local_node = makeNode(AlterTableSpaceOptionsStmt);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("isReset");
	var_key.val.string.val = strdup("isReset");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	local_node->isReset = var_value->val.boolean;

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("options");
	var_key.val.string.val = strdup("options");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->options = NULL;
	else
		local_node->options = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("tablespacename");
	var_key.val.string.val = strdup("tablespacename");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					if (var_value->type == jbvNull)
						local_node->tablespacename = NULL;
					else
					{
						char *result = palloc(var_value->val.string.len + 1);
						memcpy(result, var_value->val.string.val, var_value->val.string.len);
						result[var_value->val.string.len] = '\0';
						local_node->tablespacename = result;
					}
			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *FuncCall_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		FuncCall *local_node;
		if (node_cast != NULL)
			local_node = (FuncCall *) node_cast;
		else
			local_node = makeNode(FuncCall);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("funcname");
	var_key.val.string.val = strdup("funcname");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->funcname = NULL;
	else
		local_node->funcname = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("args");
	var_key.val.string.val = strdup("args");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->args = NULL;
	else
		local_node->args = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("agg_distinct");
	var_key.val.string.val = strdup("agg_distinct");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	local_node->agg_distinct = var_value->val.boolean;

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("agg_filter");
	var_key.val.string.val = strdup("agg_filter");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->agg_filter = NULL;
	} else {
		local_node->agg_filter = (Node *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("agg_order");
	var_key.val.string.val = strdup("agg_order");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->agg_order = NULL;
	else
		local_node->agg_order = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("func_variadic");
	var_key.val.string.val = strdup("func_variadic");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	local_node->func_variadic = var_value->val.boolean;

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("over");
	var_key.val.string.val = strdup("over");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->over = NULL;
	} else {
		local_node->over = (WindowDef *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("agg_star");
	var_key.val.string.val = strdup("agg_star");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	local_node->agg_star = var_value->val.boolean;

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("agg_within_group");
	var_key.val.string.val = strdup("agg_within_group");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	local_node->agg_within_group = var_value->val.boolean;

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("location");
	var_key.val.string.val = strdup("location");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->location = DatumGetInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *Join_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		Join *local_node;
		if (node_cast != NULL)
			local_node = (Join *) node_cast;
		else
			local_node = makeNode(Join);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("jointype");
	var_key.val.string.val = strdup("jointype");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->jointype = DatumGetInt16(DirectFunctionCall1(numeric_int2, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	Plan_deser(container, (void *)&local_node->plan, -1);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("joinqual");
	var_key.val.string.val = strdup("joinqual");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->joinqual = NULL;
	else
		local_node->joinqual = list_deser(var_value->val.binary.data, false);

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *ReindexStmt_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		ReindexStmt *local_node;
		if (node_cast != NULL)
			local_node = (ReindexStmt *) node_cast;
		else
			local_node = makeNode(ReindexStmt);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("name");
	var_key.val.string.val = strdup("name");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					if (var_value->type == jbvNull)
						local_node->name = NULL;
					else
					{
						char *result = palloc(var_value->val.string.len + 1);
						memcpy(result, var_value->val.string.val, var_value->val.string.len);
						result[var_value->val.string.len] = '\0';
						local_node->name = result;
					}
			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("relation");
	var_key.val.string.val = strdup("relation");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->relation = NULL;
	} else {
		local_node->relation = (RangeVar *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("options");
	var_key.val.string.val = strdup("options");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->options = DatumGetInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("kind");
	var_key.val.string.val = strdup("kind");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->kind = DatumGetInt16(DirectFunctionCall1(numeric_int2, NumericGetDatum(var_value->val.numeric)));

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *WithClause_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		WithClause *local_node;
		if (node_cast != NULL)
			local_node = (WithClause *) node_cast;
		else
			local_node = makeNode(WithClause);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("location");
	var_key.val.string.val = strdup("location");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->location = DatumGetInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("ctes");
	var_key.val.string.val = strdup("ctes");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->ctes = NULL;
	else
		local_node->ctes = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("recursive");
	var_key.val.string.val = strdup("recursive");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	local_node->recursive = var_value->val.boolean;

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *RangeSubselect_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		RangeSubselect *local_node;
		if (node_cast != NULL)
			local_node = (RangeSubselect *) node_cast;
		else
			local_node = makeNode(RangeSubselect);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("subquery");
	var_key.val.string.val = strdup("subquery");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->subquery = NULL;
	} else {
		local_node->subquery = (Node *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("alias");
	var_key.val.string.val = strdup("alias");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->alias = NULL;
	} else {
		local_node->alias = (Alias *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("lateral");
	var_key.val.string.val = strdup("lateral");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	local_node->lateral = var_value->val.boolean;

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *CreatePLangStmt_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		CreatePLangStmt *local_node;
		if (node_cast != NULL)
			local_node = (CreatePLangStmt *) node_cast;
		else
			local_node = makeNode(CreatePLangStmt);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("plhandler");
	var_key.val.string.val = strdup("plhandler");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->plhandler = NULL;
	else
		local_node->plhandler = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("replace");
	var_key.val.string.val = strdup("replace");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	local_node->replace = var_value->val.boolean;

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("plinline");
	var_key.val.string.val = strdup("plinline");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->plinline = NULL;
	else
		local_node->plinline = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("plname");
	var_key.val.string.val = strdup("plname");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					if (var_value->type == jbvNull)
						local_node->plname = NULL;
					else
					{
						char *result = palloc(var_value->val.string.len + 1);
						memcpy(result, var_value->val.string.val, var_value->val.string.len);
						result[var_value->val.string.len] = '\0';
						local_node->plname = result;
					}
			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("plvalidator");
	var_key.val.string.val = strdup("plvalidator");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->plvalidator = NULL;
	else
		local_node->plvalidator = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("pltrusted");
	var_key.val.string.val = strdup("pltrusted");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	local_node->pltrusted = var_value->val.boolean;

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *Const_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		Const *local_node;
		if (node_cast != NULL)
			local_node = (Const *) node_cast;
		else
			local_node = makeNode(Const);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("constvalue");
	var_key.val.string.val = strdup("constvalue");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					if (var_value->type == jbvNull)
						local_node->constvalue = (Datum) NULL;
					else
					{
						JsonbValue *typbyval_value;
						
	JsonbValue typbyval_key;
	typbyval_key.type = jbvString;
	typbyval_key.val.string.len = strlen("constbyval");
	typbyval_key.val.string.val = strdup("constbyval");

						
						typbyval_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&typbyval_key);
						
						local_node->constvalue = datum_deser(var_value, typbyval_value->val.boolean);
					}
			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("consttype");
	var_key.val.string.val = strdup("consttype");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->consttype = DatumGetUInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("constlen");
	var_key.val.string.val = strdup("constlen");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->constlen = DatumGetInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("constbyval");
	var_key.val.string.val = strdup("constbyval");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	local_node->constbyval = var_value->val.boolean;

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("constisnull");
	var_key.val.string.val = strdup("constisnull");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	local_node->constisnull = var_value->val.boolean;

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("constcollid");
	var_key.val.string.val = strdup("constcollid");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->constcollid = DatumGetUInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("location");
	var_key.val.string.val = strdup("location");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->location = DatumGetInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("consttypmod");
	var_key.val.string.val = strdup("consttypmod");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->consttypmod = DatumGetInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	Expr_deser(container, (void *)&local_node->xpr, -1);

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *XmlExpr_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		XmlExpr *local_node;
		if (node_cast != NULL)
			local_node = (XmlExpr *) node_cast;
		else
			local_node = makeNode(XmlExpr);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("args");
	var_key.val.string.val = strdup("args");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->args = NULL;
	else
		local_node->args = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("xmloption");
	var_key.val.string.val = strdup("xmloption");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->xmloption = DatumGetInt16(DirectFunctionCall1(numeric_int2, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("typmod");
	var_key.val.string.val = strdup("typmod");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->typmod = DatumGetInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("named_args");
	var_key.val.string.val = strdup("named_args");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->named_args = NULL;
	else
		local_node->named_args = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("name");
	var_key.val.string.val = strdup("name");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					if (var_value->type == jbvNull)
						local_node->name = NULL;
					else
					{
						char *result = palloc(var_value->val.string.len + 1);
						memcpy(result, var_value->val.string.val, var_value->val.string.len);
						result[var_value->val.string.len] = '\0';
						local_node->name = result;
					}
			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("op");
	var_key.val.string.val = strdup("op");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->op = DatumGetInt16(DirectFunctionCall1(numeric_int2, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("location");
	var_key.val.string.val = strdup("location");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->location = DatumGetInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("arg_names");
	var_key.val.string.val = strdup("arg_names");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->arg_names = NULL;
	else
		local_node->arg_names = list_deser(var_value->val.binary.data, false);

			}
			{
					
	Expr_deser(container, (void *)&local_node->xpr, -1);

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *FuncWithArgs_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		FuncWithArgs *local_node;
		if (node_cast != NULL)
			local_node = (FuncWithArgs *) node_cast;
		else
			local_node = makeNode(FuncWithArgs);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("funcname");
	var_key.val.string.val = strdup("funcname");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->funcname = NULL;
	else
		local_node->funcname = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("funcargs");
	var_key.val.string.val = strdup("funcargs");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->funcargs = NULL;
	else
		local_node->funcargs = list_deser(var_value->val.binary.data, false);

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *CollateClause_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		CollateClause *local_node;
		if (node_cast != NULL)
			local_node = (CollateClause *) node_cast;
		else
			local_node = makeNode(CollateClause);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("location");
	var_key.val.string.val = strdup("location");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->location = DatumGetInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("collname");
	var_key.val.string.val = strdup("collname");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->collname = NULL;
	else
		local_node->collname = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("arg");
	var_key.val.string.val = strdup("arg");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->arg = NULL;
	} else {
		local_node->arg = (Node *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *CreateUserMappingStmt_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		CreateUserMappingStmt *local_node;
		if (node_cast != NULL)
			local_node = (CreateUserMappingStmt *) node_cast;
		else
			local_node = makeNode(CreateUserMappingStmt);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("servername");
	var_key.val.string.val = strdup("servername");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					if (var_value->type == jbvNull)
						local_node->servername = NULL;
					else
					{
						char *result = palloc(var_value->val.string.len + 1);
						memcpy(result, var_value->val.string.val, var_value->val.string.len);
						result[var_value->val.string.len] = '\0';
						local_node->servername = result;
					}
			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("options");
	var_key.val.string.val = strdup("options");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->options = NULL;
	else
		local_node->options = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("user");
	var_key.val.string.val = strdup("user");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->user = NULL;
	} else {
		local_node->user = (Node *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *ListenStmt_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		ListenStmt *local_node;
		if (node_cast != NULL)
			local_node = (ListenStmt *) node_cast;
		else
			local_node = makeNode(ListenStmt);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("conditionname");
	var_key.val.string.val = strdup("conditionname");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					if (var_value->type == jbvNull)
						local_node->conditionname = NULL;
					else
					{
						char *result = palloc(var_value->val.string.len + 1);
						memcpy(result, var_value->val.string.val, var_value->val.string.len);
						result[var_value->val.string.len] = '\0';
						local_node->conditionname = result;
					}
			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *RefreshMatViewStmt_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		RefreshMatViewStmt *local_node;
		if (node_cast != NULL)
			local_node = (RefreshMatViewStmt *) node_cast;
		else
			local_node = makeNode(RefreshMatViewStmt);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("relation");
	var_key.val.string.val = strdup("relation");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->relation = NULL;
	} else {
		local_node->relation = (RangeVar *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("skipData");
	var_key.val.string.val = strdup("skipData");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	local_node->skipData = var_value->val.boolean;

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("concurrent");
	var_key.val.string.val = strdup("concurrent");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	local_node->concurrent = var_value->val.boolean;

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *DeclareCursorStmt_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		DeclareCursorStmt *local_node;
		if (node_cast != NULL)
			local_node = (DeclareCursorStmt *) node_cast;
		else
			local_node = makeNode(DeclareCursorStmt);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("options");
	var_key.val.string.val = strdup("options");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->options = DatumGetInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("query");
	var_key.val.string.val = strdup("query");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->query = NULL;
	} else {
		local_node->query = (Node *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("portalname");
	var_key.val.string.val = strdup("portalname");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					if (var_value->type == jbvNull)
						local_node->portalname = NULL;
					else
					{
						char *result = palloc(var_value->val.string.len + 1);
						memcpy(result, var_value->val.string.val, var_value->val.string.len);
						result[var_value->val.string.len] = '\0';
						local_node->portalname = result;
					}
			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *CreateConversionStmt_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		CreateConversionStmt *local_node;
		if (node_cast != NULL)
			local_node = (CreateConversionStmt *) node_cast;
		else
			local_node = makeNode(CreateConversionStmt);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("func_name");
	var_key.val.string.val = strdup("func_name");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->func_name = NULL;
	else
		local_node->func_name = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("conversion_name");
	var_key.val.string.val = strdup("conversion_name");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->conversion_name = NULL;
	else
		local_node->conversion_name = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("to_encoding_name");
	var_key.val.string.val = strdup("to_encoding_name");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					if (var_value->type == jbvNull)
						local_node->to_encoding_name = NULL;
					else
					{
						char *result = palloc(var_value->val.string.len + 1);
						memcpy(result, var_value->val.string.val, var_value->val.string.len);
						result[var_value->val.string.len] = '\0';
						local_node->to_encoding_name = result;
					}
			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("def");
	var_key.val.string.val = strdup("def");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	local_node->def = var_value->val.boolean;

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("for_encoding_name");
	var_key.val.string.val = strdup("for_encoding_name");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					if (var_value->type == jbvNull)
						local_node->for_encoding_name = NULL;
					else
					{
						char *result = palloc(var_value->val.string.len + 1);
						memcpy(result, var_value->val.string.val, var_value->val.string.len);
						result[var_value->val.string.len] = '\0';
						local_node->for_encoding_name = result;
					}
			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *Value_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		Value *local_node;
		if (node_cast != NULL)
			local_node = (Value *) node_cast;
		else
			local_node = makeNode(Value);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					/* NOT FOUND TYPE: NotFound */
			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *DropStmt_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		DropStmt *local_node;
		if (node_cast != NULL)
			local_node = (DropStmt *) node_cast;
		else
			local_node = makeNode(DropStmt);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("objects");
	var_key.val.string.val = strdup("objects");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->objects = NULL;
	else
		local_node->objects = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("removeType");
	var_key.val.string.val = strdup("removeType");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->removeType = DatumGetInt16(DirectFunctionCall1(numeric_int2, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("arguments");
	var_key.val.string.val = strdup("arguments");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->arguments = NULL;
	else
		local_node->arguments = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("behavior");
	var_key.val.string.val = strdup("behavior");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->behavior = DatumGetInt16(DirectFunctionCall1(numeric_int2, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("concurrent");
	var_key.val.string.val = strdup("concurrent");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	local_node->concurrent = var_value->val.boolean;

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("missing_ok");
	var_key.val.string.val = strdup("missing_ok");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	local_node->missing_ok = var_value->val.boolean;

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *Limit_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		Limit *local_node;
		if (node_cast != NULL)
			local_node = (Limit *) node_cast;
		else
			local_node = makeNode(Limit);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("limitCount");
	var_key.val.string.val = strdup("limitCount");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->limitCount = NULL;
	} else {
		local_node->limitCount = (Node *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("limitOffset");
	var_key.val.string.val = strdup("limitOffset");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->limitOffset = NULL;
	} else {
		local_node->limitOffset = (Node *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
			{
					
	Plan_deser(container, (void *)&local_node->plan, -1);

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *IntoClause_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		IntoClause *local_node;
		if (node_cast != NULL)
			local_node = (IntoClause *) node_cast;
		else
			local_node = makeNode(IntoClause);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("rel");
	var_key.val.string.val = strdup("rel");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->rel = NULL;
	} else {
		local_node->rel = (RangeVar *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("options");
	var_key.val.string.val = strdup("options");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->options = NULL;
	else
		local_node->options = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("viewQuery");
	var_key.val.string.val = strdup("viewQuery");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->viewQuery = NULL;
	} else {
		local_node->viewQuery = (Node *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("skipData");
	var_key.val.string.val = strdup("skipData");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	local_node->skipData = var_value->val.boolean;

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("colNames");
	var_key.val.string.val = strdup("colNames");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->colNames = NULL;
	else
		local_node->colNames = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("onCommit");
	var_key.val.string.val = strdup("onCommit");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->onCommit = DatumGetInt16(DirectFunctionCall1(numeric_int2, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("tableSpaceName");
	var_key.val.string.val = strdup("tableSpaceName");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					if (var_value->type == jbvNull)
						local_node->tableSpaceName = NULL;
					else
					{
						char *result = palloc(var_value->val.string.len + 1);
						memcpy(result, var_value->val.string.val, var_value->val.string.len);
						result[var_value->val.string.len] = '\0';
						local_node->tableSpaceName = result;
					}
			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *AlternativeSubPlan_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		AlternativeSubPlan *local_node;
		if (node_cast != NULL)
			local_node = (AlternativeSubPlan *) node_cast;
		else
			local_node = makeNode(AlternativeSubPlan);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("subplans");
	var_key.val.string.val = strdup("subplans");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->subplans = NULL;
	else
		local_node->subplans = list_deser(var_value->val.binary.data, false);

			}
			{
					
	Expr_deser(container, (void *)&local_node->xpr, -1);

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *ForeignScan_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		ForeignScan *local_node;
		if (node_cast != NULL)
			local_node = (ForeignScan *) node_cast;
		else
			local_node = makeNode(ForeignScan);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("fdw_private");
	var_key.val.string.val = strdup("fdw_private");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->fdw_private = NULL;
	else
		local_node->fdw_private = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("fs_server");
	var_key.val.string.val = strdup("fs_server");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->fs_server = DatumGetUInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("fdw_exprs");
	var_key.val.string.val = strdup("fdw_exprs");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->fdw_exprs = NULL;
	else
		local_node->fdw_exprs = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("fdw_recheck_quals");
	var_key.val.string.val = strdup("fdw_recheck_quals");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->fdw_recheck_quals = NULL;
	else
		local_node->fdw_recheck_quals = list_deser(var_value->val.binary.data, false);

			}
			{
					
	Scan_deser(container, (void *)&local_node->scan, -1);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("fsSystemCol");
	var_key.val.string.val = strdup("fsSystemCol");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	local_node->fsSystemCol = var_value->val.boolean;

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("fdw_scan_tlist");
	var_key.val.string.val = strdup("fdw_scan_tlist");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->fdw_scan_tlist = NULL;
	else
		local_node->fdw_scan_tlist = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("fs_relids");
	var_key.val.string.val = strdup("fs_relids");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					if (var_value->type == jbvNull)
						local_node->fs_relids = NULL;
					else
						
	{
		Bitmapset  *result = NULL;
		JsonbValue v;
		int type;
		JsonbIterator *it = JsonbIteratorInit(var_value->val.binary.data);
		while ((type = JsonbIteratorNext(&it, &v, true)) != WJB_DONE)
		{
			if (type == WJB_ELEM)
				result = bms_add_member(result,
										DatumGetUInt32(DirectFunctionCall1(numeric_int4,
																		   NumericGetDatum(v.val.numeric))));
		}
		local_node->fs_relids = result;
	}
	

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *A_Const_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		A_Const *local_node;
		if (node_cast != NULL)
			local_node = (A_Const *) node_cast;
		else
			local_node = makeNode(A_Const);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("location");
	var_key.val.string.val = strdup("location");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->location = DatumGetInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("val");
	var_key.val.string.val = strdup("val");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					if (var_value->type == jbvString) {
						char *result = palloc(var_value->val.string.len + 1);
						result[var_value->val.string.len] = '\0';
						memcpy(result, var_value->val.string.val, var_value->val.string.len);
						local_node->val = *makeString(result);
					} else if (var_value->type == jbvNumeric) {
						local_node->val = *makeInteger(DatumGetInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric))));
					}
			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *AlterSystemStmt_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		AlterSystemStmt *local_node;
		if (node_cast != NULL)
			local_node = (AlterSystemStmt *) node_cast;
		else
			local_node = makeNode(AlterSystemStmt);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("setstmt");
	var_key.val.string.val = strdup("setstmt");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->setstmt = NULL;
	} else {
		local_node->setstmt = (VariableSetStmt *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *ResTarget_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		ResTarget *local_node;
		if (node_cast != NULL)
			local_node = (ResTarget *) node_cast;
		else
			local_node = makeNode(ResTarget);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("name");
	var_key.val.string.val = strdup("name");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					if (var_value->type == jbvNull)
						local_node->name = NULL;
					else
					{
						char *result = palloc(var_value->val.string.len + 1);
						memcpy(result, var_value->val.string.val, var_value->val.string.len);
						result[var_value->val.string.len] = '\0';
						local_node->name = result;
					}
			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("indirection");
	var_key.val.string.val = strdup("indirection");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->indirection = NULL;
	else
		local_node->indirection = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("location");
	var_key.val.string.val = strdup("location");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->location = DatumGetInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("val");
	var_key.val.string.val = strdup("val");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->val = NULL;
	} else {
		local_node->val = (Node *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *RangeTblEntry_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		RangeTblEntry *local_node;
		if (node_cast != NULL)
			local_node = (RangeTblEntry *) node_cast;
		else
			local_node = makeNode(RangeTblEntry);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("subquery");
	var_key.val.string.val = strdup("subquery");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->subquery = NULL;
	} else {
		local_node->subquery = (Query *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("ctecoltypmods");
	var_key.val.string.val = strdup("ctecoltypmods");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->ctecoltypmods = NULL;
	else
		local_node->ctecoltypmods = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("lateral");
	var_key.val.string.val = strdup("lateral");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	local_node->lateral = var_value->val.boolean;

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("inFromCl");
	var_key.val.string.val = strdup("inFromCl");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	local_node->inFromCl = var_value->val.boolean;

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("updatedCols");
	var_key.val.string.val = strdup("updatedCols");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					if (var_value->type == jbvNull)
						local_node->updatedCols = NULL;
					else
						
	{
		Bitmapset  *result = NULL;
		JsonbValue v;
		int type;
		JsonbIterator *it = JsonbIteratorInit(var_value->val.binary.data);
		while ((type = JsonbIteratorNext(&it, &v, true)) != WJB_DONE)
		{
			if (type == WJB_ELEM)
				result = bms_add_member(result,
										DatumGetUInt32(DirectFunctionCall1(numeric_int4,
																		   NumericGetDatum(v.val.numeric))));
		}
		local_node->updatedCols = result;
	}
	

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("tablesample");
	var_key.val.string.val = strdup("tablesample");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->tablesample = NULL;
	} else {
		local_node->tablesample = (TableSampleClause *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("checkAsUser");
	var_key.val.string.val = strdup("checkAsUser");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->checkAsUser = DatumGetUInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("values_lists");
	var_key.val.string.val = strdup("values_lists");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->values_lists = NULL;
	else
		local_node->values_lists = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("joinaliasvars");
	var_key.val.string.val = strdup("joinaliasvars");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->joinaliasvars = NULL;
	else
		local_node->joinaliasvars = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("securityQuals");
	var_key.val.string.val = strdup("securityQuals");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->securityQuals = NULL;
	else
		local_node->securityQuals = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("ctename");
	var_key.val.string.val = strdup("ctename");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					if (var_value->type == jbvNull)
						local_node->ctename = NULL;
					else
					{
						char *result = palloc(var_value->val.string.len + 1);
						memcpy(result, var_value->val.string.val, var_value->val.string.len);
						result[var_value->val.string.len] = '\0';
						local_node->ctename = result;
					}
			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("ctelevelsup");
	var_key.val.string.val = strdup("ctelevelsup");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->ctelevelsup = DatumGetUInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("jointype");
	var_key.val.string.val = strdup("jointype");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->jointype = DatumGetInt16(DirectFunctionCall1(numeric_int2, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("requiredPerms");
	var_key.val.string.val = strdup("requiredPerms");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->requiredPerms = DatumGetUInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("relkind");
	var_key.val.string.val = strdup("relkind");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->relkind = DatumGetInt16(DirectFunctionCall1(numeric_int2, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("functions");
	var_key.val.string.val = strdup("functions");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->functions = NULL;
	else
		local_node->functions = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("values_collations");
	var_key.val.string.val = strdup("values_collations");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->values_collations = NULL;
	else
		local_node->values_collations = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("self_reference");
	var_key.val.string.val = strdup("self_reference");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	local_node->self_reference = var_value->val.boolean;

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("alias");
	var_key.val.string.val = strdup("alias");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->alias = NULL;
	} else {
		local_node->alias = (Alias *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("rtekind");
	var_key.val.string.val = strdup("rtekind");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->rtekind = DatumGetInt16(DirectFunctionCall1(numeric_int2, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("insertedCols");
	var_key.val.string.val = strdup("insertedCols");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					if (var_value->type == jbvNull)
						local_node->insertedCols = NULL;
					else
						
	{
		Bitmapset  *result = NULL;
		JsonbValue v;
		int type;
		JsonbIterator *it = JsonbIteratorInit(var_value->val.binary.data);
		while ((type = JsonbIteratorNext(&it, &v, true)) != WJB_DONE)
		{
			if (type == WJB_ELEM)
				result = bms_add_member(result,
										DatumGetUInt32(DirectFunctionCall1(numeric_int4,
																		   NumericGetDatum(v.val.numeric))));
		}
		local_node->insertedCols = result;
	}
	

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("funcordinality");
	var_key.val.string.val = strdup("funcordinality");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	local_node->funcordinality = var_value->val.boolean;

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("ctecoltypes");
	var_key.val.string.val = strdup("ctecoltypes");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->ctecoltypes = NULL;
	else
		local_node->ctecoltypes = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("inh");
	var_key.val.string.val = strdup("inh");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	local_node->inh = var_value->val.boolean;

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("selectedCols");
	var_key.val.string.val = strdup("selectedCols");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					if (var_value->type == jbvNull)
						local_node->selectedCols = NULL;
					else
						
	{
		Bitmapset  *result = NULL;
		JsonbValue v;
		int type;
		JsonbIterator *it = JsonbIteratorInit(var_value->val.binary.data);
		while ((type = JsonbIteratorNext(&it, &v, true)) != WJB_DONE)
		{
			if (type == WJB_ELEM)
				result = bms_add_member(result,
										DatumGetUInt32(DirectFunctionCall1(numeric_int4,
																		   NumericGetDatum(v.val.numeric))));
		}
		local_node->selectedCols = result;
	}
	

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("relid");
	var_key.val.string.val = strdup("relid");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->relid = DatumGetUInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("security_barrier");
	var_key.val.string.val = strdup("security_barrier");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	local_node->security_barrier = var_value->val.boolean;

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("ctecolcollations");
	var_key.val.string.val = strdup("ctecolcollations");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->ctecolcollations = NULL;
	else
		local_node->ctecolcollations = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("eref");
	var_key.val.string.val = strdup("eref");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->eref = NULL;
	} else {
		local_node->eref = (Alias *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *A_Indirection_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		A_Indirection *local_node;
		if (node_cast != NULL)
			local_node = (A_Indirection *) node_cast;
		else
			local_node = makeNode(A_Indirection);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("indirection");
	var_key.val.string.val = strdup("indirection");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->indirection = NULL;
	else
		local_node->indirection = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("arg");
	var_key.val.string.val = strdup("arg");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->arg = NULL;
	} else {
		local_node->arg = (Node *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *Append_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		Append *local_node;
		if (node_cast != NULL)
			local_node = (Append *) node_cast;
		else
			local_node = makeNode(Append);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	Plan_deser(container, (void *)&local_node->plan, -1);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("appendplans");
	var_key.val.string.val = strdup("appendplans");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->appendplans = NULL;
	else
		local_node->appendplans = list_deser(var_value->val.binary.data, false);

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *TableSampleClause_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		TableSampleClause *local_node;
		if (node_cast != NULL)
			local_node = (TableSampleClause *) node_cast;
		else
			local_node = makeNode(TableSampleClause);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("args");
	var_key.val.string.val = strdup("args");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->args = NULL;
	else
		local_node->args = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("tsmhandler");
	var_key.val.string.val = strdup("tsmhandler");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->tsmhandler = DatumGetUInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("repeatable");
	var_key.val.string.val = strdup("repeatable");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->repeatable = NULL;
	} else {
		local_node->repeatable = (Expr *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *BooleanTest_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		BooleanTest *local_node;
		if (node_cast != NULL)
			local_node = (BooleanTest *) node_cast;
		else
			local_node = makeNode(BooleanTest);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("booltesttype");
	var_key.val.string.val = strdup("booltesttype");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->booltesttype = DatumGetInt16(DirectFunctionCall1(numeric_int2, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("location");
	var_key.val.string.val = strdup("location");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->location = DatumGetInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("arg");
	var_key.val.string.val = strdup("arg");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->arg = NULL;
	} else {
		local_node->arg = (Expr *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
			{
					
	Expr_deser(container, (void *)&local_node->xpr, -1);

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *ArrayCoerceExpr_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		ArrayCoerceExpr *local_node;
		if (node_cast != NULL)
			local_node = (ArrayCoerceExpr *) node_cast;
		else
			local_node = makeNode(ArrayCoerceExpr);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("coerceformat");
	var_key.val.string.val = strdup("coerceformat");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->coerceformat = DatumGetInt16(DirectFunctionCall1(numeric_int2, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("location");
	var_key.val.string.val = strdup("location");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->location = DatumGetInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("arg");
	var_key.val.string.val = strdup("arg");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->arg = NULL;
	} else {
		local_node->arg = (Expr *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("isExplicit");
	var_key.val.string.val = strdup("isExplicit");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	local_node->isExplicit = var_value->val.boolean;

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("resulttypmod");
	var_key.val.string.val = strdup("resulttypmod");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->resulttypmod = DatumGetInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("resulttype");
	var_key.val.string.val = strdup("resulttype");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->resulttype = DatumGetUInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("resultcollid");
	var_key.val.string.val = strdup("resultcollid");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->resultcollid = DatumGetUInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	Expr_deser(container, (void *)&local_node->xpr, -1);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("elemfuncid");
	var_key.val.string.val = strdup("elemfuncid");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->elemfuncid = DatumGetUInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *WithCheckOption_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		WithCheckOption *local_node;
		if (node_cast != NULL)
			local_node = (WithCheckOption *) node_cast;
		else
			local_node = makeNode(WithCheckOption);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("polname");
	var_key.val.string.val = strdup("polname");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					if (var_value->type == jbvNull)
						local_node->polname = NULL;
					else
					{
						char *result = palloc(var_value->val.string.len + 1);
						memcpy(result, var_value->val.string.val, var_value->val.string.len);
						result[var_value->val.string.len] = '\0';
						local_node->polname = result;
					}
			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("cascaded");
	var_key.val.string.val = strdup("cascaded");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	local_node->cascaded = var_value->val.boolean;

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("qual");
	var_key.val.string.val = strdup("qual");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->qual = NULL;
	} else {
		local_node->qual = (Node *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("relname");
	var_key.val.string.val = strdup("relname");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					if (var_value->type == jbvNull)
						local_node->relname = NULL;
					else
					{
						char *result = palloc(var_value->val.string.len + 1);
						memcpy(result, var_value->val.string.val, var_value->val.string.len);
						result[var_value->val.string.len] = '\0';
						local_node->relname = result;
					}
			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("kind");
	var_key.val.string.val = strdup("kind");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->kind = DatumGetInt16(DirectFunctionCall1(numeric_int2, NumericGetDatum(var_value->val.numeric)));

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *RowCompareExpr_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		RowCompareExpr *local_node;
		if (node_cast != NULL)
			local_node = (RowCompareExpr *) node_cast;
		else
			local_node = makeNode(RowCompareExpr);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("largs");
	var_key.val.string.val = strdup("largs");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->largs = NULL;
	else
		local_node->largs = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("inputcollids");
	var_key.val.string.val = strdup("inputcollids");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->inputcollids = NULL;
	else
		local_node->inputcollids = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("rctype");
	var_key.val.string.val = strdup("rctype");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->rctype = DatumGetInt16(DirectFunctionCall1(numeric_int2, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("opnos");
	var_key.val.string.val = strdup("opnos");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->opnos = NULL;
	else
		local_node->opnos = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("opfamilies");
	var_key.val.string.val = strdup("opfamilies");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->opfamilies = NULL;
	else
		local_node->opfamilies = list_deser(var_value->val.binary.data, false);

			}
			{
					
	Expr_deser(container, (void *)&local_node->xpr, -1);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("rargs");
	var_key.val.string.val = strdup("rargs");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->rargs = NULL;
	else
		local_node->rargs = list_deser(var_value->val.binary.data, false);

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *MultiAssignRef_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		MultiAssignRef *local_node;
		if (node_cast != NULL)
			local_node = (MultiAssignRef *) node_cast;
		else
			local_node = makeNode(MultiAssignRef);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("source");
	var_key.val.string.val = strdup("source");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->source = NULL;
	} else {
		local_node->source = (Node *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("colno");
	var_key.val.string.val = strdup("colno");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->colno = DatumGetInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("ncolumns");
	var_key.val.string.val = strdup("ncolumns");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->ncolumns = DatumGetInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *TruncateStmt_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		TruncateStmt *local_node;
		if (node_cast != NULL)
			local_node = (TruncateStmt *) node_cast;
		else
			local_node = makeNode(TruncateStmt);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("restart_seqs");
	var_key.val.string.val = strdup("restart_seqs");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	local_node->restart_seqs = var_value->val.boolean;

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("relations");
	var_key.val.string.val = strdup("relations");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->relations = NULL;
	else
		local_node->relations = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("behavior");
	var_key.val.string.val = strdup("behavior");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->behavior = DatumGetInt16(DirectFunctionCall1(numeric_int2, NumericGetDatum(var_value->val.numeric)));

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *CoerceToDomain_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		CoerceToDomain *local_node;
		if (node_cast != NULL)
			local_node = (CoerceToDomain *) node_cast;
		else
			local_node = makeNode(CoerceToDomain);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("location");
	var_key.val.string.val = strdup("location");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->location = DatumGetInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("arg");
	var_key.val.string.val = strdup("arg");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->arg = NULL;
	} else {
		local_node->arg = (Expr *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("coercionformat");
	var_key.val.string.val = strdup("coercionformat");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->coercionformat = DatumGetInt16(DirectFunctionCall1(numeric_int2, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("resulttypmod");
	var_key.val.string.val = strdup("resulttypmod");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->resulttypmod = DatumGetInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("resulttype");
	var_key.val.string.val = strdup("resulttype");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->resulttype = DatumGetUInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("resultcollid");
	var_key.val.string.val = strdup("resultcollid");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->resultcollid = DatumGetUInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	Expr_deser(container, (void *)&local_node->xpr, -1);

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *WindowClause_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		WindowClause *local_node;
		if (node_cast != NULL)
			local_node = (WindowClause *) node_cast;
		else
			local_node = makeNode(WindowClause);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("refname");
	var_key.val.string.val = strdup("refname");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					if (var_value->type == jbvNull)
						local_node->refname = NULL;
					else
					{
						char *result = palloc(var_value->val.string.len + 1);
						memcpy(result, var_value->val.string.val, var_value->val.string.len);
						result[var_value->val.string.len] = '\0';
						local_node->refname = result;
					}
			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("partitionClause");
	var_key.val.string.val = strdup("partitionClause");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->partitionClause = NULL;
	else
		local_node->partitionClause = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("startOffset");
	var_key.val.string.val = strdup("startOffset");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->startOffset = NULL;
	} else {
		local_node->startOffset = (Node *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("orderClause");
	var_key.val.string.val = strdup("orderClause");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->orderClause = NULL;
	else
		local_node->orderClause = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("name");
	var_key.val.string.val = strdup("name");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					if (var_value->type == jbvNull)
						local_node->name = NULL;
					else
					{
						char *result = palloc(var_value->val.string.len + 1);
						memcpy(result, var_value->val.string.val, var_value->val.string.len);
						result[var_value->val.string.len] = '\0';
						local_node->name = result;
					}
			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("winref");
	var_key.val.string.val = strdup("winref");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->winref = DatumGetUInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("copiedOrder");
	var_key.val.string.val = strdup("copiedOrder");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	local_node->copiedOrder = var_value->val.boolean;

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("frameOptions");
	var_key.val.string.val = strdup("frameOptions");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->frameOptions = DatumGetInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("endOffset");
	var_key.val.string.val = strdup("endOffset");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->endOffset = NULL;
	} else {
		local_node->endOffset = (Node *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *DefElem_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		DefElem *local_node;
		if (node_cast != NULL)
			local_node = (DefElem *) node_cast;
		else
			local_node = makeNode(DefElem);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("defaction");
	var_key.val.string.val = strdup("defaction");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->defaction = DatumGetInt16(DirectFunctionCall1(numeric_int2, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("arg");
	var_key.val.string.val = strdup("arg");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->arg = NULL;
	} else {
		local_node->arg = (Node *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("defname");
	var_key.val.string.val = strdup("defname");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					if (var_value->type == jbvNull)
						local_node->defname = NULL;
					else
					{
						char *result = palloc(var_value->val.string.len + 1);
						memcpy(result, var_value->val.string.val, var_value->val.string.len);
						result[var_value->val.string.len] = '\0';
						local_node->defname = result;
					}
			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("defnamespace");
	var_key.val.string.val = strdup("defnamespace");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					if (var_value->type == jbvNull)
						local_node->defnamespace = NULL;
					else
					{
						char *result = palloc(var_value->val.string.len + 1);
						memcpy(result, var_value->val.string.val, var_value->val.string.len);
						result[var_value->val.string.len] = '\0';
						local_node->defnamespace = result;
					}
			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *DropUserMappingStmt_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		DropUserMappingStmt *local_node;
		if (node_cast != NULL)
			local_node = (DropUserMappingStmt *) node_cast;
		else
			local_node = makeNode(DropUserMappingStmt);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("servername");
	var_key.val.string.val = strdup("servername");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					if (var_value->type == jbvNull)
						local_node->servername = NULL;
					else
					{
						char *result = palloc(var_value->val.string.len + 1);
						memcpy(result, var_value->val.string.val, var_value->val.string.len);
						result[var_value->val.string.len] = '\0';
						local_node->servername = result;
					}
			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("user");
	var_key.val.string.val = strdup("user");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->user = NULL;
	} else {
		local_node->user = (Node *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("missing_ok");
	var_key.val.string.val = strdup("missing_ok");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	local_node->missing_ok = var_value->val.boolean;

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *A_ArrayExpr_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		A_ArrayExpr *local_node;
		if (node_cast != NULL)
			local_node = (A_ArrayExpr *) node_cast;
		else
			local_node = makeNode(A_ArrayExpr);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("location");
	var_key.val.string.val = strdup("location");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->location = DatumGetInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("elements");
	var_key.val.string.val = strdup("elements");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->elements = NULL;
	else
		local_node->elements = list_deser(var_value->val.binary.data, false);

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *CreateTableAsStmt_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		CreateTableAsStmt *local_node;
		if (node_cast != NULL)
			local_node = (CreateTableAsStmt *) node_cast;
		else
			local_node = makeNode(CreateTableAsStmt);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("into");
	var_key.val.string.val = strdup("into");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->into = NULL;
	} else {
		local_node->into = (IntoClause *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("is_select_into");
	var_key.val.string.val = strdup("is_select_into");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	local_node->is_select_into = var_value->val.boolean;

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("if_not_exists");
	var_key.val.string.val = strdup("if_not_exists");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	local_node->if_not_exists = var_value->val.boolean;

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("query");
	var_key.val.string.val = strdup("query");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->query = NULL;
	} else {
		local_node->query = (Node *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("relkind");
	var_key.val.string.val = strdup("relkind");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->relkind = DatumGetInt16(DirectFunctionCall1(numeric_int2, NumericGetDatum(var_value->val.numeric)));

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *OpExpr_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		OpExpr *local_node;
		if (node_cast != NULL)
			local_node = (OpExpr *) node_cast;
		else
			local_node = makeNode(OpExpr);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("args");
	var_key.val.string.val = strdup("args");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->args = NULL;
	else
		local_node->args = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("location");
	var_key.val.string.val = strdup("location");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->location = DatumGetInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("opresulttype");
	var_key.val.string.val = strdup("opresulttype");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->opresulttype = DatumGetUInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	Expr_deser(container, (void *)&local_node->xpr, -1);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("inputcollid");
	var_key.val.string.val = strdup("inputcollid");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->inputcollid = DatumGetUInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("opfuncid");
	var_key.val.string.val = strdup("opfuncid");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->opfuncid = DatumGetUInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("opcollid");
	var_key.val.string.val = strdup("opcollid");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->opcollid = DatumGetUInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("opretset");
	var_key.val.string.val = strdup("opretset");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	local_node->opretset = var_value->val.boolean;

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("opno");
	var_key.val.string.val = strdup("opno");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->opno = DatumGetUInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *FieldStore_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		FieldStore *local_node;
		if (node_cast != NULL)
			local_node = (FieldStore *) node_cast;
		else
			local_node = makeNode(FieldStore);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("fieldnums");
	var_key.val.string.val = strdup("fieldnums");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->fieldnums = NULL;
	else
		local_node->fieldnums = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("newvals");
	var_key.val.string.val = strdup("newvals");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->newvals = NULL;
	else
		local_node->newvals = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("arg");
	var_key.val.string.val = strdup("arg");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->arg = NULL;
	} else {
		local_node->arg = (Expr *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("resulttype");
	var_key.val.string.val = strdup("resulttype");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->resulttype = DatumGetUInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	Expr_deser(container, (void *)&local_node->xpr, -1);

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *CoerceViaIO_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		CoerceViaIO *local_node;
		if (node_cast != NULL)
			local_node = (CoerceViaIO *) node_cast;
		else
			local_node = makeNode(CoerceViaIO);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("location");
	var_key.val.string.val = strdup("location");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->location = DatumGetInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("arg");
	var_key.val.string.val = strdup("arg");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->arg = NULL;
	} else {
		local_node->arg = (Expr *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("resulttype");
	var_key.val.string.val = strdup("resulttype");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->resulttype = DatumGetUInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("resultcollid");
	var_key.val.string.val = strdup("resultcollid");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->resultcollid = DatumGetUInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	Expr_deser(container, (void *)&local_node->xpr, -1);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("coerceformat");
	var_key.val.string.val = strdup("coerceformat");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->coerceformat = DatumGetInt16(DirectFunctionCall1(numeric_int2, NumericGetDatum(var_value->val.numeric)));

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *SubLink_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		SubLink *local_node;
		if (node_cast != NULL)
			local_node = (SubLink *) node_cast;
		else
			local_node = makeNode(SubLink);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("operName");
	var_key.val.string.val = strdup("operName");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->operName = NULL;
	else
		local_node->operName = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("location");
	var_key.val.string.val = strdup("location");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->location = DatumGetInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("subselect");
	var_key.val.string.val = strdup("subselect");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->subselect = NULL;
	} else {
		local_node->subselect = (Node *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("subLinkId");
	var_key.val.string.val = strdup("subLinkId");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->subLinkId = DatumGetInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("subLinkType");
	var_key.val.string.val = strdup("subLinkType");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->subLinkType = DatumGetInt16(DirectFunctionCall1(numeric_int2, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	Expr_deser(container, (void *)&local_node->xpr, -1);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("testexpr");
	var_key.val.string.val = strdup("testexpr");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->testexpr = NULL;
	} else {
		local_node->testexpr = (Node *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *WorkTableScan_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		WorkTableScan *local_node;
		if (node_cast != NULL)
			local_node = (WorkTableScan *) node_cast;
		else
			local_node = makeNode(WorkTableScan);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("wtParam");
	var_key.val.string.val = strdup("wtParam");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->wtParam = DatumGetInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	Scan_deser(container, (void *)&local_node->scan, -1);

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *A_Expr_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		A_Expr *local_node;
		if (node_cast != NULL)
			local_node = (A_Expr *) node_cast;
		else
			local_node = makeNode(A_Expr);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("lexpr");
	var_key.val.string.val = strdup("lexpr");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->lexpr = NULL;
	} else {
		local_node->lexpr = (Node *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("rexpr");
	var_key.val.string.val = strdup("rexpr");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->rexpr = NULL;
	} else {
		local_node->rexpr = (Node *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("name");
	var_key.val.string.val = strdup("name");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->name = NULL;
	else
		local_node->name = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("location");
	var_key.val.string.val = strdup("location");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->location = DatumGetInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("kind");
	var_key.val.string.val = strdup("kind");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->kind = DatumGetInt16(DirectFunctionCall1(numeric_int2, NumericGetDatum(var_value->val.numeric)));

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *DropOwnedStmt_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		DropOwnedStmt *local_node;
		if (node_cast != NULL)
			local_node = (DropOwnedStmt *) node_cast;
		else
			local_node = makeNode(DropOwnedStmt);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("behavior");
	var_key.val.string.val = strdup("behavior");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->behavior = DatumGetInt16(DirectFunctionCall1(numeric_int2, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("roles");
	var_key.val.string.val = strdup("roles");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->roles = NULL;
	else
		local_node->roles = list_deser(var_value->val.binary.data, false);

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *AlterDatabaseSetStmt_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		AlterDatabaseSetStmt *local_node;
		if (node_cast != NULL)
			local_node = (AlterDatabaseSetStmt *) node_cast;
		else
			local_node = makeNode(AlterDatabaseSetStmt);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("dbname");
	var_key.val.string.val = strdup("dbname");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					if (var_value->type == jbvNull)
						local_node->dbname = NULL;
					else
					{
						char *result = palloc(var_value->val.string.len + 1);
						memcpy(result, var_value->val.string.val, var_value->val.string.len);
						result[var_value->val.string.len] = '\0';
						local_node->dbname = result;
					}
			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("setstmt");
	var_key.val.string.val = strdup("setstmt");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->setstmt = NULL;
	} else {
		local_node->setstmt = (VariableSetStmt *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *CreateRangeStmt_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		CreateRangeStmt *local_node;
		if (node_cast != NULL)
			local_node = (CreateRangeStmt *) node_cast;
		else
			local_node = makeNode(CreateRangeStmt);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("params");
	var_key.val.string.val = strdup("params");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->params = NULL;
	else
		local_node->params = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("typeName");
	var_key.val.string.val = strdup("typeName");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->typeName = NULL;
	else
		local_node->typeName = list_deser(var_value->val.binary.data, false);

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *DeleteStmt_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		DeleteStmt *local_node;
		if (node_cast != NULL)
			local_node = (DeleteStmt *) node_cast;
		else
			local_node = makeNode(DeleteStmt);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("whereClause");
	var_key.val.string.val = strdup("whereClause");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->whereClause = NULL;
	} else {
		local_node->whereClause = (Node *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("relation");
	var_key.val.string.val = strdup("relation");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->relation = NULL;
	} else {
		local_node->relation = (RangeVar *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("withClause");
	var_key.val.string.val = strdup("withClause");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->withClause = NULL;
	} else {
		local_node->withClause = (WithClause *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("usingClause");
	var_key.val.string.val = strdup("usingClause");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->usingClause = NULL;
	else
		local_node->usingClause = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("returningList");
	var_key.val.string.val = strdup("returningList");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->returningList = NULL;
	else
		local_node->returningList = list_deser(var_value->val.binary.data, false);

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *CreateSeqStmt_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		CreateSeqStmt *local_node;
		if (node_cast != NULL)
			local_node = (CreateSeqStmt *) node_cast;
		else
			local_node = makeNode(CreateSeqStmt);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("ownerId");
	var_key.val.string.val = strdup("ownerId");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->ownerId = DatumGetUInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("options");
	var_key.val.string.val = strdup("options");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->options = NULL;
	else
		local_node->options = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("sequence");
	var_key.val.string.val = strdup("sequence");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->sequence = NULL;
	} else {
		local_node->sequence = (RangeVar *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("if_not_exists");
	var_key.val.string.val = strdup("if_not_exists");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	local_node->if_not_exists = var_value->val.boolean;

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *RuleStmt_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		RuleStmt *local_node;
		if (node_cast != NULL)
			local_node = (RuleStmt *) node_cast;
		else
			local_node = makeNode(RuleStmt);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("replace");
	var_key.val.string.val = strdup("replace");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	local_node->replace = var_value->val.boolean;

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("rulename");
	var_key.val.string.val = strdup("rulename");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					if (var_value->type == jbvNull)
						local_node->rulename = NULL;
					else
					{
						char *result = palloc(var_value->val.string.len + 1);
						memcpy(result, var_value->val.string.val, var_value->val.string.len);
						result[var_value->val.string.len] = '\0';
						local_node->rulename = result;
					}
			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("whereClause");
	var_key.val.string.val = strdup("whereClause");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->whereClause = NULL;
	} else {
		local_node->whereClause = (Node *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("event");
	var_key.val.string.val = strdup("event");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->event = DatumGetInt16(DirectFunctionCall1(numeric_int2, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("relation");
	var_key.val.string.val = strdup("relation");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->relation = NULL;
	} else {
		local_node->relation = (RangeVar *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("actions");
	var_key.val.string.val = strdup("actions");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->actions = NULL;
	else
		local_node->actions = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("instead");
	var_key.val.string.val = strdup("instead");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	local_node->instead = var_value->val.boolean;

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *RangeTblFunction_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		RangeTblFunction *local_node;
		if (node_cast != NULL)
			local_node = (RangeTblFunction *) node_cast;
		else
			local_node = makeNode(RangeTblFunction);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("funccolcollations");
	var_key.val.string.val = strdup("funccolcollations");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->funccolcollations = NULL;
	else
		local_node->funccolcollations = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("funccolnames");
	var_key.val.string.val = strdup("funccolnames");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->funccolnames = NULL;
	else
		local_node->funccolnames = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("funcexpr");
	var_key.val.string.val = strdup("funcexpr");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->funcexpr = NULL;
	} else {
		local_node->funcexpr = (Node *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("funcparams");
	var_key.val.string.val = strdup("funcparams");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					if (var_value->type == jbvNull)
						local_node->funcparams = NULL;
					else
						
	{
		Bitmapset  *result = NULL;
		JsonbValue v;
		int type;
		JsonbIterator *it = JsonbIteratorInit(var_value->val.binary.data);
		while ((type = JsonbIteratorNext(&it, &v, true)) != WJB_DONE)
		{
			if (type == WJB_ELEM)
				result = bms_add_member(result,
										DatumGetUInt32(DirectFunctionCall1(numeric_int4,
																		   NumericGetDatum(v.val.numeric))));
		}
		local_node->funcparams = result;
	}
	

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("funccoltypmods");
	var_key.val.string.val = strdup("funccoltypmods");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->funccoltypmods = NULL;
	else
		local_node->funccoltypmods = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("funccolcount");
	var_key.val.string.val = strdup("funccolcount");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->funccolcount = DatumGetInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("funccoltypes");
	var_key.val.string.val = strdup("funccoltypes");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->funccoltypes = NULL;
	else
		local_node->funccoltypes = list_deser(var_value->val.binary.data, false);

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *IndexStmt_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		IndexStmt *local_node;
		if (node_cast != NULL)
			local_node = (IndexStmt *) node_cast;
		else
			local_node = makeNode(IndexStmt);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("options");
	var_key.val.string.val = strdup("options");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->options = NULL;
	else
		local_node->options = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("whereClause");
	var_key.val.string.val = strdup("whereClause");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->whereClause = NULL;
	} else {
		local_node->whereClause = (Node *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("idxcomment");
	var_key.val.string.val = strdup("idxcomment");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					if (var_value->type == jbvNull)
						local_node->idxcomment = NULL;
					else
					{
						char *result = palloc(var_value->val.string.len + 1);
						memcpy(result, var_value->val.string.val, var_value->val.string.len);
						result[var_value->val.string.len] = '\0';
						local_node->idxcomment = result;
					}
			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("transformed");
	var_key.val.string.val = strdup("transformed");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	local_node->transformed = var_value->val.boolean;

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("relation");
	var_key.val.string.val = strdup("relation");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->relation = NULL;
	} else {
		local_node->relation = (RangeVar *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("oldNode");
	var_key.val.string.val = strdup("oldNode");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->oldNode = DatumGetUInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("indexParams");
	var_key.val.string.val = strdup("indexParams");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->indexParams = NULL;
	else
		local_node->indexParams = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("concurrent");
	var_key.val.string.val = strdup("concurrent");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	local_node->concurrent = var_value->val.boolean;

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("unique");
	var_key.val.string.val = strdup("unique");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	local_node->unique = var_value->val.boolean;

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("idxname");
	var_key.val.string.val = strdup("idxname");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					if (var_value->type == jbvNull)
						local_node->idxname = NULL;
					else
					{
						char *result = palloc(var_value->val.string.len + 1);
						memcpy(result, var_value->val.string.val, var_value->val.string.len);
						result[var_value->val.string.len] = '\0';
						local_node->idxname = result;
					}
			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("initdeferred");
	var_key.val.string.val = strdup("initdeferred");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	local_node->initdeferred = var_value->val.boolean;

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("accessMethod");
	var_key.val.string.val = strdup("accessMethod");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					if (var_value->type == jbvNull)
						local_node->accessMethod = NULL;
					else
					{
						char *result = palloc(var_value->val.string.len + 1);
						memcpy(result, var_value->val.string.val, var_value->val.string.len);
						result[var_value->val.string.len] = '\0';
						local_node->accessMethod = result;
					}
			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("deferrable");
	var_key.val.string.val = strdup("deferrable");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	local_node->deferrable = var_value->val.boolean;

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("isconstraint");
	var_key.val.string.val = strdup("isconstraint");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	local_node->isconstraint = var_value->val.boolean;

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("excludeOpNames");
	var_key.val.string.val = strdup("excludeOpNames");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->excludeOpNames = NULL;
	else
		local_node->excludeOpNames = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("tableSpace");
	var_key.val.string.val = strdup("tableSpace");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					if (var_value->type == jbvNull)
						local_node->tableSpace = NULL;
					else
					{
						char *result = palloc(var_value->val.string.len + 1);
						memcpy(result, var_value->val.string.val, var_value->val.string.len);
						result[var_value->val.string.len] = '\0';
						local_node->tableSpace = result;
					}
			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("primary");
	var_key.val.string.val = strdup("primary");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	local_node->primary = var_value->val.boolean;

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("if_not_exists");
	var_key.val.string.val = strdup("if_not_exists");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	local_node->if_not_exists = var_value->val.boolean;

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("indexOid");
	var_key.val.string.val = strdup("indexOid");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->indexOid = DatumGetUInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *WindowAgg_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		WindowAgg *local_node;
		if (node_cast != NULL)
			local_node = (WindowAgg *) node_cast;
		else
			local_node = makeNode(WindowAgg);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("winref");
	var_key.val.string.val = strdup("winref");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->winref = DatumGetUInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("frameOptions");
	var_key.val.string.val = strdup("frameOptions");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->frameOptions = DatumGetInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("startOffset");
	var_key.val.string.val = strdup("startOffset");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->startOffset = NULL;
	} else {
		local_node->startOffset = (Node *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("partColIdx");
	var_key.val.string.val = strdup("partColIdx");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

						
	{
		int type;
		int i = 0;
		JsonbValue v;
		JsonbIterator *it = JsonbIteratorInit(var_value->val.binary.data);
			local_node->partNumCols = it->nElems;
		local_node->partColIdx = (AttrNumber*) palloc(sizeof(AttrNumber)*it->nElems);
		while ((type = JsonbIteratorNext(&it, &v, true)) != WJB_DONE)
		{
			if (type == WJB_ELEM)
			{
					
		local_node->partColIdx[i] = DatumGetInt16(DirectFunctionCall1(numeric_int2, NumericGetDatum(v.val.numeric)));

				i++;
			}
		}
	}

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("ordColIdx");
	var_key.val.string.val = strdup("ordColIdx");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

						
	{
		int type;
		int i = 0;
		JsonbValue v;
		JsonbIterator *it = JsonbIteratorInit(var_value->val.binary.data);
			local_node->ordNumCols = it->nElems;
		local_node->ordColIdx = (AttrNumber*) palloc(sizeof(AttrNumber)*it->nElems);
		while ((type = JsonbIteratorNext(&it, &v, true)) != WJB_DONE)
		{
			if (type == WJB_ELEM)
			{
					
		local_node->ordColIdx[i] = DatumGetInt16(DirectFunctionCall1(numeric_int2, NumericGetDatum(v.val.numeric)));

				i++;
			}
		}
	}

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("partOperators");
	var_key.val.string.val = strdup("partOperators");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

						
	{
		int type;
		int i = 0;
		JsonbValue v;
		JsonbIterator *it = JsonbIteratorInit(var_value->val.binary.data);
			local_node->partNumCols = it->nElems;
		local_node->partOperators = (Oid*) palloc(sizeof(Oid)*it->nElems);
		while ((type = JsonbIteratorNext(&it, &v, true)) != WJB_DONE)
		{
			if (type == WJB_ELEM)
			{
					
		local_node->partOperators[i] = DatumGetUInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(v.val.numeric)));

				i++;
			}
		}
	}

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("ordNumCols");
	var_key.val.string.val = strdup("ordNumCols");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->ordNumCols = DatumGetInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	Plan_deser(container, (void *)&local_node->plan, -1);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("ordOperators");
	var_key.val.string.val = strdup("ordOperators");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

						
	{
		int type;
		int i = 0;
		JsonbValue v;
		JsonbIterator *it = JsonbIteratorInit(var_value->val.binary.data);
			local_node->ordNumCols = it->nElems;
		local_node->ordOperators = (Oid*) palloc(sizeof(Oid)*it->nElems);
		while ((type = JsonbIteratorNext(&it, &v, true)) != WJB_DONE)
		{
			if (type == WJB_ELEM)
			{
					
		local_node->ordOperators[i] = DatumGetUInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(v.val.numeric)));

				i++;
			}
		}
	}

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("partNumCols");
	var_key.val.string.val = strdup("partNumCols");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->partNumCols = DatumGetInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("endOffset");
	var_key.val.string.val = strdup("endOffset");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->endOffset = NULL;
	} else {
		local_node->endOffset = (Node *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *ConvertRowtypeExpr_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		ConvertRowtypeExpr *local_node;
		if (node_cast != NULL)
			local_node = (ConvertRowtypeExpr *) node_cast;
		else
			local_node = makeNode(ConvertRowtypeExpr);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("convertformat");
	var_key.val.string.val = strdup("convertformat");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->convertformat = DatumGetInt16(DirectFunctionCall1(numeric_int2, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("location");
	var_key.val.string.val = strdup("location");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->location = DatumGetInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("resulttype");
	var_key.val.string.val = strdup("resulttype");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->resulttype = DatumGetUInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("arg");
	var_key.val.string.val = strdup("arg");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->arg = NULL;
	} else {
		local_node->arg = (Expr *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
			{
					
	Expr_deser(container, (void *)&local_node->xpr, -1);

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *MergeAppend_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		MergeAppend *local_node;
		if (node_cast != NULL)
			local_node = (MergeAppend *) node_cast;
		else
			local_node = makeNode(MergeAppend);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("collations");
	var_key.val.string.val = strdup("collations");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

						
	{
		int type;
		int i = 0;
		JsonbValue v;
		JsonbIterator *it = JsonbIteratorInit(var_value->val.binary.data);
			local_node->numCols = it->nElems;
		local_node->collations = (Oid*) palloc(sizeof(Oid)*it->nElems);
		while ((type = JsonbIteratorNext(&it, &v, true)) != WJB_DONE)
		{
			if (type == WJB_ELEM)
			{
					
		local_node->collations[i] = DatumGetUInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(v.val.numeric)));

				i++;
			}
		}
	}

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("sortColIdx");
	var_key.val.string.val = strdup("sortColIdx");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

						
	{
		int type;
		int i = 0;
		JsonbValue v;
		JsonbIterator *it = JsonbIteratorInit(var_value->val.binary.data);
			local_node->numCols = it->nElems;
		local_node->sortColIdx = (AttrNumber*) palloc(sizeof(AttrNumber)*it->nElems);
		while ((type = JsonbIteratorNext(&it, &v, true)) != WJB_DONE)
		{
			if (type == WJB_ELEM)
			{
					
		local_node->sortColIdx[i] = DatumGetInt16(DirectFunctionCall1(numeric_int2, NumericGetDatum(v.val.numeric)));

				i++;
			}
		}
	}

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("nullsFirst");
	var_key.val.string.val = strdup("nullsFirst");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

						
	{
		int type;
		int i = 0;
		JsonbValue v;
		JsonbIterator *it = JsonbIteratorInit(var_value->val.binary.data);
			local_node->numCols = it->nElems;
		local_node->nullsFirst = (bool*) palloc(sizeof(bool)*it->nElems);
		while ((type = JsonbIteratorNext(&it, &v, true)) != WJB_DONE)
		{
			if (type == WJB_ELEM)
			{
					
	local_node->nullsFirst[i] = v.val.boolean;

				i++;
			}
		}
	}

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("sortOperators");
	var_key.val.string.val = strdup("sortOperators");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

						
	{
		int type;
		int i = 0;
		JsonbValue v;
		JsonbIterator *it = JsonbIteratorInit(var_value->val.binary.data);
			local_node->numCols = it->nElems;
		local_node->sortOperators = (Oid*) palloc(sizeof(Oid)*it->nElems);
		while ((type = JsonbIteratorNext(&it, &v, true)) != WJB_DONE)
		{
			if (type == WJB_ELEM)
			{
					
		local_node->sortOperators[i] = DatumGetUInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(v.val.numeric)));

				i++;
			}
		}
	}

			}
			{
					
	Plan_deser(container, (void *)&local_node->plan, -1);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("numCols");
	var_key.val.string.val = strdup("numCols");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->numCols = DatumGetInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("mergeplans");
	var_key.val.string.val = strdup("mergeplans");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->mergeplans = NULL;
	else
		local_node->mergeplans = list_deser(var_value->val.binary.data, false);

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *AccessPriv_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		AccessPriv *local_node;
		if (node_cast != NULL)
			local_node = (AccessPriv *) node_cast;
		else
			local_node = makeNode(AccessPriv);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("priv_name");
	var_key.val.string.val = strdup("priv_name");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					if (var_value->type == jbvNull)
						local_node->priv_name = NULL;
					else
					{
						char *result = palloc(var_value->val.string.len + 1);
						memcpy(result, var_value->val.string.val, var_value->val.string.len);
						result[var_value->val.string.len] = '\0';
						local_node->priv_name = result;
					}
			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("cols");
	var_key.val.string.val = strdup("cols");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->cols = NULL;
	else
		local_node->cols = list_deser(var_value->val.binary.data, false);

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *CreateOpFamilyStmt_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		CreateOpFamilyStmt *local_node;
		if (node_cast != NULL)
			local_node = (CreateOpFamilyStmt *) node_cast;
		else
			local_node = makeNode(CreateOpFamilyStmt);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("opfamilyname");
	var_key.val.string.val = strdup("opfamilyname");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->opfamilyname = NULL;
	else
		local_node->opfamilyname = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("amname");
	var_key.val.string.val = strdup("amname");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					if (var_value->type == jbvNull)
						local_node->amname = NULL;
					else
					{
						char *result = palloc(var_value->val.string.len + 1);
						memcpy(result, var_value->val.string.val, var_value->val.string.len);
						result[var_value->val.string.len] = '\0';
						local_node->amname = result;
					}
			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *CoalesceExpr_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		CoalesceExpr *local_node;
		if (node_cast != NULL)
			local_node = (CoalesceExpr *) node_cast;
		else
			local_node = makeNode(CoalesceExpr);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("args");
	var_key.val.string.val = strdup("args");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->args = NULL;
	else
		local_node->args = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("location");
	var_key.val.string.val = strdup("location");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->location = DatumGetInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("coalescecollid");
	var_key.val.string.val = strdup("coalescecollid");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->coalescecollid = DatumGetUInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("coalescetype");
	var_key.val.string.val = strdup("coalescetype");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->coalescetype = DatumGetUInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	Expr_deser(container, (void *)&local_node->xpr, -1);

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *DoStmt_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		DoStmt *local_node;
		if (node_cast != NULL)
			local_node = (DoStmt *) node_cast;
		else
			local_node = makeNode(DoStmt);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("args");
	var_key.val.string.val = strdup("args");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->args = NULL;
	else
		local_node->args = list_deser(var_value->val.binary.data, false);

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *IndexOnlyScan_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		IndexOnlyScan *local_node;
		if (node_cast != NULL)
			local_node = (IndexOnlyScan *) node_cast;
		else
			local_node = makeNode(IndexOnlyScan);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("indexid");
	var_key.val.string.val = strdup("indexid");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->indexid = DatumGetUInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("indextlist");
	var_key.val.string.val = strdup("indextlist");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->indextlist = NULL;
	else
		local_node->indextlist = list_deser(var_value->val.binary.data, false);

			}
			{
					
	Scan_deser(container, (void *)&local_node->scan, -1);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("indexqual");
	var_key.val.string.val = strdup("indexqual");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->indexqual = NULL;
	else
		local_node->indexqual = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("indexorderdir");
	var_key.val.string.val = strdup("indexorderdir");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->indexorderdir = DatumGetInt16(DirectFunctionCall1(numeric_int2, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("indexorderby");
	var_key.val.string.val = strdup("indexorderby");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->indexorderby = NULL;
	else
		local_node->indexorderby = list_deser(var_value->val.binary.data, false);

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *XmlSerialize_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		XmlSerialize *local_node;
		if (node_cast != NULL)
			local_node = (XmlSerialize *) node_cast;
		else
			local_node = makeNode(XmlSerialize);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("location");
	var_key.val.string.val = strdup("location");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->location = DatumGetInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("xmloption");
	var_key.val.string.val = strdup("xmloption");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->xmloption = DatumGetInt16(DirectFunctionCall1(numeric_int2, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("typeName");
	var_key.val.string.val = strdup("typeName");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->typeName = NULL;
	} else {
		local_node->typeName = (TypeName *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("expr");
	var_key.val.string.val = strdup("expr");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->expr = NULL;
	} else {
		local_node->expr = (Node *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *FunctionParameter_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		FunctionParameter *local_node;
		if (node_cast != NULL)
			local_node = (FunctionParameter *) node_cast;
		else
			local_node = makeNode(FunctionParameter);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("name");
	var_key.val.string.val = strdup("name");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					if (var_value->type == jbvNull)
						local_node->name = NULL;
					else
					{
						char *result = palloc(var_value->val.string.len + 1);
						memcpy(result, var_value->val.string.val, var_value->val.string.len);
						result[var_value->val.string.len] = '\0';
						local_node->name = result;
					}
			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("argType");
	var_key.val.string.val = strdup("argType");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->argType = NULL;
	} else {
		local_node->argType = (TypeName *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("mode");
	var_key.val.string.val = strdup("mode");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->mode = DatumGetInt16(DirectFunctionCall1(numeric_int2, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("defexpr");
	var_key.val.string.val = strdup("defexpr");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->defexpr = NULL;
	} else {
		local_node->defexpr = (Node *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *SetToDefault_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		SetToDefault *local_node;
		if (node_cast != NULL)
			local_node = (SetToDefault *) node_cast;
		else
			local_node = makeNode(SetToDefault);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("collation");
	var_key.val.string.val = strdup("collation");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->collation = DatumGetUInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("location");
	var_key.val.string.val = strdup("location");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->location = DatumGetInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("typeMod");
	var_key.val.string.val = strdup("typeMod");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->typeMod = DatumGetInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	Expr_deser(container, (void *)&local_node->xpr, -1);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("typeId");
	var_key.val.string.val = strdup("typeId");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->typeId = DatumGetUInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *JoinExpr_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		JoinExpr *local_node;
		if (node_cast != NULL)
			local_node = (JoinExpr *) node_cast;
		else
			local_node = makeNode(JoinExpr);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("rarg");
	var_key.val.string.val = strdup("rarg");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->rarg = NULL;
	} else {
		local_node->rarg = (Node *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("larg");
	var_key.val.string.val = strdup("larg");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->larg = NULL;
	} else {
		local_node->larg = (Node *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("quals");
	var_key.val.string.val = strdup("quals");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->quals = NULL;
	} else {
		local_node->quals = (Node *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("alias");
	var_key.val.string.val = strdup("alias");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->alias = NULL;
	} else {
		local_node->alias = (Alias *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("jointype");
	var_key.val.string.val = strdup("jointype");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->jointype = DatumGetInt16(DirectFunctionCall1(numeric_int2, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("rtindex");
	var_key.val.string.val = strdup("rtindex");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->rtindex = DatumGetInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("usingClause");
	var_key.val.string.val = strdup("usingClause");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->usingClause = NULL;
	else
		local_node->usingClause = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("isNatural");
	var_key.val.string.val = strdup("isNatural");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	local_node->isNatural = var_value->val.boolean;

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *NullTest_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		NullTest *local_node;
		if (node_cast != NULL)
			local_node = (NullTest *) node_cast;
		else
			local_node = makeNode(NullTest);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("nulltesttype");
	var_key.val.string.val = strdup("nulltesttype");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->nulltesttype = DatumGetInt16(DirectFunctionCall1(numeric_int2, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("argisrow");
	var_key.val.string.val = strdup("argisrow");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	local_node->argisrow = var_value->val.boolean;

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("location");
	var_key.val.string.val = strdup("location");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->location = DatumGetInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("arg");
	var_key.val.string.val = strdup("arg");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->arg = NULL;
	} else {
		local_node->arg = (Expr *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
			{
					
	Expr_deser(container, (void *)&local_node->xpr, -1);

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *NotifyStmt_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		NotifyStmt *local_node;
		if (node_cast != NULL)
			local_node = (NotifyStmt *) node_cast;
		else
			local_node = makeNode(NotifyStmt);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("payload");
	var_key.val.string.val = strdup("payload");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					if (var_value->type == jbvNull)
						local_node->payload = NULL;
					else
					{
						char *result = palloc(var_value->val.string.len + 1);
						memcpy(result, var_value->val.string.val, var_value->val.string.len);
						result[var_value->val.string.len] = '\0';
						local_node->payload = result;
					}
			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("conditionname");
	var_key.val.string.val = strdup("conditionname");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					if (var_value->type == jbvNull)
						local_node->conditionname = NULL;
					else
					{
						char *result = palloc(var_value->val.string.len + 1);
						memcpy(result, var_value->val.string.val, var_value->val.string.len);
						result[var_value->val.string.len] = '\0';
						local_node->conditionname = result;
					}
			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *AlterTableMoveAllStmt_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		AlterTableMoveAllStmt *local_node;
		if (node_cast != NULL)
			local_node = (AlterTableMoveAllStmt *) node_cast;
		else
			local_node = makeNode(AlterTableMoveAllStmt);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("objtype");
	var_key.val.string.val = strdup("objtype");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->objtype = DatumGetInt16(DirectFunctionCall1(numeric_int2, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("nowait");
	var_key.val.string.val = strdup("nowait");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	local_node->nowait = var_value->val.boolean;

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("new_tablespacename");
	var_key.val.string.val = strdup("new_tablespacename");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					if (var_value->type == jbvNull)
						local_node->new_tablespacename = NULL;
					else
					{
						char *result = palloc(var_value->val.string.len + 1);
						memcpy(result, var_value->val.string.val, var_value->val.string.len);
						result[var_value->val.string.len] = '\0';
						local_node->new_tablespacename = result;
					}
			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("orig_tablespacename");
	var_key.val.string.val = strdup("orig_tablespacename");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					if (var_value->type == jbvNull)
						local_node->orig_tablespacename = NULL;
					else
					{
						char *result = palloc(var_value->val.string.len + 1);
						memcpy(result, var_value->val.string.val, var_value->val.string.len);
						result[var_value->val.string.len] = '\0';
						local_node->orig_tablespacename = result;
					}
			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("roles");
	var_key.val.string.val = strdup("roles");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->roles = NULL;
	else
		local_node->roles = list_deser(var_value->val.binary.data, false);

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *LockingClause_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		LockingClause *local_node;
		if (node_cast != NULL)
			local_node = (LockingClause *) node_cast;
		else
			local_node = makeNode(LockingClause);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("waitPolicy");
	var_key.val.string.val = strdup("waitPolicy");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->waitPolicy = DatumGetInt16(DirectFunctionCall1(numeric_int2, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("strength");
	var_key.val.string.val = strdup("strength");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->strength = DatumGetInt16(DirectFunctionCall1(numeric_int2, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("lockedRels");
	var_key.val.string.val = strdup("lockedRels");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->lockedRels = NULL;
	else
		local_node->lockedRels = list_deser(var_value->val.binary.data, false);

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *DefineStmt_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		DefineStmt *local_node;
		if (node_cast != NULL)
			local_node = (DefineStmt *) node_cast;
		else
			local_node = makeNode(DefineStmt);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("args");
	var_key.val.string.val = strdup("args");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->args = NULL;
	else
		local_node->args = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("oldstyle");
	var_key.val.string.val = strdup("oldstyle");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	local_node->oldstyle = var_value->val.boolean;

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("definition");
	var_key.val.string.val = strdup("definition");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->definition = NULL;
	else
		local_node->definition = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("defnames");
	var_key.val.string.val = strdup("defnames");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->defnames = NULL;
	else
		local_node->defnames = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("kind");
	var_key.val.string.val = strdup("kind");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->kind = DatumGetInt16(DirectFunctionCall1(numeric_int2, NumericGetDatum(var_value->val.numeric)));

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *CreateOpClassItem_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		CreateOpClassItem *local_node;
		if (node_cast != NULL)
			local_node = (CreateOpClassItem *) node_cast;
		else
			local_node = makeNode(CreateOpClassItem);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("args");
	var_key.val.string.val = strdup("args");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->args = NULL;
	else
		local_node->args = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("number");
	var_key.val.string.val = strdup("number");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->number = DatumGetInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("itemtype");
	var_key.val.string.val = strdup("itemtype");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->itemtype = DatumGetInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("name");
	var_key.val.string.val = strdup("name");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->name = NULL;
	else
		local_node->name = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("storedtype");
	var_key.val.string.val = strdup("storedtype");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->storedtype = NULL;
	} else {
		local_node->storedtype = (TypeName *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("order_family");
	var_key.val.string.val = strdup("order_family");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->order_family = NULL;
	else
		local_node->order_family = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("class_args");
	var_key.val.string.val = strdup("class_args");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->class_args = NULL;
	else
		local_node->class_args = list_deser(var_value->val.binary.data, false);

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *ClusterStmt_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		ClusterStmt *local_node;
		if (node_cast != NULL)
			local_node = (ClusterStmt *) node_cast;
		else
			local_node = makeNode(ClusterStmt);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("verbose");
	var_key.val.string.val = strdup("verbose");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	local_node->verbose = var_value->val.boolean;

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("relation");
	var_key.val.string.val = strdup("relation");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->relation = NULL;
	} else {
		local_node->relation = (RangeVar *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("indexname");
	var_key.val.string.val = strdup("indexname");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					if (var_value->type == jbvNull)
						local_node->indexname = NULL;
					else
					{
						char *result = palloc(var_value->val.string.len + 1);
						memcpy(result, var_value->val.string.val, var_value->val.string.len);
						result[var_value->val.string.len] = '\0';
						local_node->indexname = result;
					}
			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *MergeJoin_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		MergeJoin *local_node;
		if (node_cast != NULL)
			local_node = (MergeJoin *) node_cast;
		else
			local_node = makeNode(MergeJoin);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("mergeNullsFirst");
	var_key.val.string.val = strdup("mergeNullsFirst");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

						
	{
		int type;
		int i = 0;
		JsonbValue v;
		JsonbIterator *it = JsonbIteratorInit(var_value->val.binary.data);
		local_node->mergeNullsFirst = (bool*) palloc(sizeof(bool)*it->nElems);
		while ((type = JsonbIteratorNext(&it, &v, true)) != WJB_DONE)
		{
			if (type == WJB_ELEM)
			{
					
	local_node->mergeNullsFirst[i] = v.val.boolean;

				i++;
			}
		}
	}

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("mergeFamilies");
	var_key.val.string.val = strdup("mergeFamilies");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

						
	{
		int type;
		int i = 0;
		JsonbValue v;
		JsonbIterator *it = JsonbIteratorInit(var_value->val.binary.data);
		local_node->mergeFamilies = (Oid*) palloc(sizeof(Oid)*it->nElems);
		while ((type = JsonbIteratorNext(&it, &v, true)) != WJB_DONE)
		{
			if (type == WJB_ELEM)
			{
					
		local_node->mergeFamilies[i] = DatumGetUInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(v.val.numeric)));

				i++;
			}
		}
	}

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("mergeclauses");
	var_key.val.string.val = strdup("mergeclauses");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->mergeclauses = NULL;
	else
		local_node->mergeclauses = list_deser(var_value->val.binary.data, false);

			}
			{
					
	Join_deser(container, (void *)&local_node->join, -1);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("mergeStrategies");
	var_key.val.string.val = strdup("mergeStrategies");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

						
	{
		int type;
		int i = 0;
		JsonbValue v;
		JsonbIterator *it = JsonbIteratorInit(var_value->val.binary.data);
		local_node->mergeStrategies = (int*) palloc(sizeof(int)*it->nElems);
		while ((type = JsonbIteratorNext(&it, &v, true)) != WJB_DONE)
		{
			if (type == WJB_ELEM)
			{
					
		local_node->mergeStrategies[i] = DatumGetInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(v.val.numeric)));

				i++;
			}
		}
	}

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("mergeCollations");
	var_key.val.string.val = strdup("mergeCollations");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

						
	{
		int type;
		int i = 0;
		JsonbValue v;
		JsonbIterator *it = JsonbIteratorInit(var_value->val.binary.data);
		local_node->mergeCollations = (Oid*) palloc(sizeof(Oid)*it->nElems);
		while ((type = JsonbIteratorNext(&it, &v, true)) != WJB_DONE)
		{
			if (type == WJB_ELEM)
			{
					
		local_node->mergeCollations[i] = DatumGetUInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(v.val.numeric)));

				i++;
			}
		}
	}

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *SecLabelStmt_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		SecLabelStmt *local_node;
		if (node_cast != NULL)
			local_node = (SecLabelStmt *) node_cast;
		else
			local_node = makeNode(SecLabelStmt);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("objtype");
	var_key.val.string.val = strdup("objtype");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->objtype = DatumGetInt16(DirectFunctionCall1(numeric_int2, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("provider");
	var_key.val.string.val = strdup("provider");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					if (var_value->type == jbvNull)
						local_node->provider = NULL;
					else
					{
						char *result = palloc(var_value->val.string.len + 1);
						memcpy(result, var_value->val.string.val, var_value->val.string.len);
						result[var_value->val.string.len] = '\0';
						local_node->provider = result;
					}
			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("label");
	var_key.val.string.val = strdup("label");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					if (var_value->type == jbvNull)
						local_node->label = NULL;
					else
					{
						char *result = palloc(var_value->val.string.len + 1);
						memcpy(result, var_value->val.string.val, var_value->val.string.len);
						result[var_value->val.string.len] = '\0';
						local_node->label = result;
					}
			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("objname");
	var_key.val.string.val = strdup("objname");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->objname = NULL;
	else
		local_node->objname = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("objargs");
	var_key.val.string.val = strdup("objargs");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->objargs = NULL;
	else
		local_node->objargs = list_deser(var_value->val.binary.data, false);

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *CaseTestExpr_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		CaseTestExpr *local_node;
		if (node_cast != NULL)
			local_node = (CaseTestExpr *) node_cast;
		else
			local_node = makeNode(CaseTestExpr);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("collation");
	var_key.val.string.val = strdup("collation");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->collation = DatumGetUInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("typeMod");
	var_key.val.string.val = strdup("typeMod");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->typeMod = DatumGetInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	Expr_deser(container, (void *)&local_node->xpr, -1);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("typeId");
	var_key.val.string.val = strdup("typeId");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->typeId = DatumGetUInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *TidScan_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		TidScan *local_node;
		if (node_cast != NULL)
			local_node = (TidScan *) node_cast;
		else
			local_node = makeNode(TidScan);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("tidquals");
	var_key.val.string.val = strdup("tidquals");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->tidquals = NULL;
	else
		local_node->tidquals = list_deser(var_value->val.binary.data, false);

			}
			{
					
	Scan_deser(container, (void *)&local_node->scan, -1);

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *Query_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		Query *local_node;
		if (node_cast != NULL)
			local_node = (Query *) node_cast;
		else
			local_node = makeNode(Query);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("windowClause");
	var_key.val.string.val = strdup("windowClause");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->windowClause = NULL;
	else
		local_node->windowClause = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("onConflict");
	var_key.val.string.val = strdup("onConflict");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->onConflict = NULL;
	} else {
		local_node->onConflict = (OnConflictExpr *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("jointree");
	var_key.val.string.val = strdup("jointree");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->jointree = NULL;
	} else {
		local_node->jointree = (FromExpr *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("queryId");
	var_key.val.string.val = strdup("queryId");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->queryId = DatumGetUInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("canSetTag");
	var_key.val.string.val = strdup("canSetTag");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	local_node->canSetTag = var_value->val.boolean;

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("hasRecursive");
	var_key.val.string.val = strdup("hasRecursive");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	local_node->hasRecursive = var_value->val.boolean;

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("querySource");
	var_key.val.string.val = strdup("querySource");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->querySource = DatumGetInt16(DirectFunctionCall1(numeric_int2, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("commandType");
	var_key.val.string.val = strdup("commandType");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->commandType = DatumGetInt16(DirectFunctionCall1(numeric_int2, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("hasDistinctOn");
	var_key.val.string.val = strdup("hasDistinctOn");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	local_node->hasDistinctOn = var_value->val.boolean;

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("sortClause");
	var_key.val.string.val = strdup("sortClause");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->sortClause = NULL;
	else
		local_node->sortClause = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("groupClause");
	var_key.val.string.val = strdup("groupClause");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->groupClause = NULL;
	else
		local_node->groupClause = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("hasModifyingCTE");
	var_key.val.string.val = strdup("hasModifyingCTE");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	local_node->hasModifyingCTE = var_value->val.boolean;

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("hasRowSecurity");
	var_key.val.string.val = strdup("hasRowSecurity");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	local_node->hasRowSecurity = var_value->val.boolean;

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("rtable");
	var_key.val.string.val = strdup("rtable");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->rtable = NULL;
	else
		local_node->rtable = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("hasAggs");
	var_key.val.string.val = strdup("hasAggs");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	local_node->hasAggs = var_value->val.boolean;

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("setOperations");
	var_key.val.string.val = strdup("setOperations");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->setOperations = NULL;
	} else {
		local_node->setOperations = (Node *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("withCheckOptions");
	var_key.val.string.val = strdup("withCheckOptions");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->withCheckOptions = NULL;
	else
		local_node->withCheckOptions = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("utilityStmt");
	var_key.val.string.val = strdup("utilityStmt");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->utilityStmt = NULL;
	} else {
		local_node->utilityStmt = (Node *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("resultRelation");
	var_key.val.string.val = strdup("resultRelation");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->resultRelation = DatumGetInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("targetList");
	var_key.val.string.val = strdup("targetList");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->targetList = NULL;
	else
		local_node->targetList = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("groupingSets");
	var_key.val.string.val = strdup("groupingSets");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->groupingSets = NULL;
	else
		local_node->groupingSets = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("limitOffset");
	var_key.val.string.val = strdup("limitOffset");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->limitOffset = NULL;
	} else {
		local_node->limitOffset = (Node *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("hasForUpdate");
	var_key.val.string.val = strdup("hasForUpdate");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	local_node->hasForUpdate = var_value->val.boolean;

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("rowMarks");
	var_key.val.string.val = strdup("rowMarks");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->rowMarks = NULL;
	else
		local_node->rowMarks = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("returningList");
	var_key.val.string.val = strdup("returningList");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->returningList = NULL;
	else
		local_node->returningList = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("hasWindowFuncs");
	var_key.val.string.val = strdup("hasWindowFuncs");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	local_node->hasWindowFuncs = var_value->val.boolean;

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("cteList");
	var_key.val.string.val = strdup("cteList");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->cteList = NULL;
	else
		local_node->cteList = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("distinctClause");
	var_key.val.string.val = strdup("distinctClause");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->distinctClause = NULL;
	else
		local_node->distinctClause = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("hasSubLinks");
	var_key.val.string.val = strdup("hasSubLinks");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	local_node->hasSubLinks = var_value->val.boolean;

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("havingQual");
	var_key.val.string.val = strdup("havingQual");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->havingQual = NULL;
	} else {
		local_node->havingQual = (Node *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("limitCount");
	var_key.val.string.val = strdup("limitCount");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->limitCount = NULL;
	} else {
		local_node->limitCount = (Node *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("constraintDeps");
	var_key.val.string.val = strdup("constraintDeps");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->constraintDeps = NULL;
	else
		local_node->constraintDeps = list_deser(var_value->val.binary.data, false);

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *ReplicaIdentityStmt_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		ReplicaIdentityStmt *local_node;
		if (node_cast != NULL)
			local_node = (ReplicaIdentityStmt *) node_cast;
		else
			local_node = makeNode(ReplicaIdentityStmt);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("name");
	var_key.val.string.val = strdup("name");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					if (var_value->type == jbvNull)
						local_node->name = NULL;
					else
					{
						char *result = palloc(var_value->val.string.len + 1);
						memcpy(result, var_value->val.string.val, var_value->val.string.len);
						result[var_value->val.string.len] = '\0';
						local_node->name = result;
					}
			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("identity_type");
	var_key.val.string.val = strdup("identity_type");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->identity_type = DatumGetInt16(DirectFunctionCall1(numeric_int2, NumericGetDatum(var_value->val.numeric)));

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *IndexScan_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		IndexScan *local_node;
		if (node_cast != NULL)
			local_node = (IndexScan *) node_cast;
		else
			local_node = makeNode(IndexScan);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("indexorderbyorig");
	var_key.val.string.val = strdup("indexorderbyorig");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->indexorderbyorig = NULL;
	else
		local_node->indexorderbyorig = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("indexid");
	var_key.val.string.val = strdup("indexid");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->indexid = DatumGetUInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("indexqualorig");
	var_key.val.string.val = strdup("indexqualorig");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->indexqualorig = NULL;
	else
		local_node->indexqualorig = list_deser(var_value->val.binary.data, false);

			}
			{
					
	Scan_deser(container, (void *)&local_node->scan, -1);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("indexqual");
	var_key.val.string.val = strdup("indexqual");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->indexqual = NULL;
	else
		local_node->indexqual = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("indexorderdir");
	var_key.val.string.val = strdup("indexorderdir");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->indexorderdir = DatumGetInt16(DirectFunctionCall1(numeric_int2, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("indexorderby");
	var_key.val.string.val = strdup("indexorderby");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->indexorderby = NULL;
	else
		local_node->indexorderby = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("indexorderbyops");
	var_key.val.string.val = strdup("indexorderbyops");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->indexorderbyops = NULL;
	else
		local_node->indexorderbyops = list_deser(var_value->val.binary.data, false);

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *ValuesScan_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		ValuesScan *local_node;
		if (node_cast != NULL)
			local_node = (ValuesScan *) node_cast;
		else
			local_node = makeNode(ValuesScan);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("values_lists");
	var_key.val.string.val = strdup("values_lists");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->values_lists = NULL;
	else
		local_node->values_lists = list_deser(var_value->val.binary.data, false);

			}
			{
					
	Scan_deser(container, (void *)&local_node->scan, -1);

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *FieldSelect_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		FieldSelect *local_node;
		if (node_cast != NULL)
			local_node = (FieldSelect *) node_cast;
		else
			local_node = makeNode(FieldSelect);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("resulttypmod");
	var_key.val.string.val = strdup("resulttypmod");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->resulttypmod = DatumGetInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("arg");
	var_key.val.string.val = strdup("arg");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->arg = NULL;
	} else {
		local_node->arg = (Expr *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("fieldnum");
	var_key.val.string.val = strdup("fieldnum");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->fieldnum = DatumGetInt16(DirectFunctionCall1(numeric_int2, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("resulttype");
	var_key.val.string.val = strdup("resulttype");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->resulttype = DatumGetUInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("resultcollid");
	var_key.val.string.val = strdup("resultcollid");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->resultcollid = DatumGetUInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	Expr_deser(container, (void *)&local_node->xpr, -1);

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *OnConflictExpr_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		OnConflictExpr *local_node;
		if (node_cast != NULL)
			local_node = (OnConflictExpr *) node_cast;
		else
			local_node = makeNode(OnConflictExpr);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("arbiterElems");
	var_key.val.string.val = strdup("arbiterElems");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->arbiterElems = NULL;
	else
		local_node->arbiterElems = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("arbiterWhere");
	var_key.val.string.val = strdup("arbiterWhere");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->arbiterWhere = NULL;
	} else {
		local_node->arbiterWhere = (Node *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("onConflictSet");
	var_key.val.string.val = strdup("onConflictSet");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->onConflictSet = NULL;
	else
		local_node->onConflictSet = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("action");
	var_key.val.string.val = strdup("action");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->action = DatumGetInt16(DirectFunctionCall1(numeric_int2, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("exclRelTlist");
	var_key.val.string.val = strdup("exclRelTlist");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->exclRelTlist = NULL;
	else
		local_node->exclRelTlist = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("exclRelIndex");
	var_key.val.string.val = strdup("exclRelIndex");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->exclRelIndex = DatumGetInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("onConflictWhere");
	var_key.val.string.val = strdup("onConflictWhere");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->onConflictWhere = NULL;
	} else {
		local_node->onConflictWhere = (Node *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("constraint");
	var_key.val.string.val = strdup("constraint");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->constraint = DatumGetUInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *VariableSetStmt_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		VariableSetStmt *local_node;
		if (node_cast != NULL)
			local_node = (VariableSetStmt *) node_cast;
		else
			local_node = makeNode(VariableSetStmt);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("name");
	var_key.val.string.val = strdup("name");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					if (var_value->type == jbvNull)
						local_node->name = NULL;
					else
					{
						char *result = palloc(var_value->val.string.len + 1);
						memcpy(result, var_value->val.string.val, var_value->val.string.len);
						result[var_value->val.string.len] = '\0';
						local_node->name = result;
					}
			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("args");
	var_key.val.string.val = strdup("args");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->args = NULL;
	else
		local_node->args = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("kind");
	var_key.val.string.val = strdup("kind");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->kind = DatumGetInt16(DirectFunctionCall1(numeric_int2, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("is_local");
	var_key.val.string.val = strdup("is_local");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	local_node->is_local = var_value->val.boolean;

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *TargetEntry_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		TargetEntry *local_node;
		if (node_cast != NULL)
			local_node = (TargetEntry *) node_cast;
		else
			local_node = makeNode(TargetEntry);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("resname");
	var_key.val.string.val = strdup("resname");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					if (var_value->type == jbvNull)
						local_node->resname = NULL;
					else
					{
						char *result = palloc(var_value->val.string.len + 1);
						memcpy(result, var_value->val.string.val, var_value->val.string.len);
						result[var_value->val.string.len] = '\0';
						local_node->resname = result;
					}
			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("resjunk");
	var_key.val.string.val = strdup("resjunk");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	local_node->resjunk = var_value->val.boolean;

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("resorigtbl");
	var_key.val.string.val = strdup("resorigtbl");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->resorigtbl = DatumGetUInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("ressortgroupref");
	var_key.val.string.val = strdup("ressortgroupref");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->ressortgroupref = DatumGetUInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("resorigcol");
	var_key.val.string.val = strdup("resorigcol");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->resorigcol = DatumGetInt16(DirectFunctionCall1(numeric_int2, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	Expr_deser(container, (void *)&local_node->xpr, -1);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("resno");
	var_key.val.string.val = strdup("resno");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->resno = DatumGetInt16(DirectFunctionCall1(numeric_int2, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("expr");
	var_key.val.string.val = strdup("expr");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->expr = NULL;
	} else {
		local_node->expr = (Expr *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *LockRows_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		LockRows *local_node;
		if (node_cast != NULL)
			local_node = (LockRows *) node_cast;
		else
			local_node = makeNode(LockRows);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("rowMarks");
	var_key.val.string.val = strdup("rowMarks");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->rowMarks = NULL;
	else
		local_node->rowMarks = list_deser(var_value->val.binary.data, false);

			}
			{
					
	Plan_deser(container, (void *)&local_node->plan, -1);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("epqParam");
	var_key.val.string.val = strdup("epqParam");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->epqParam = DatumGetInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *CreateDomainStmt_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		CreateDomainStmt *local_node;
		if (node_cast != NULL)
			local_node = (CreateDomainStmt *) node_cast;
		else
			local_node = makeNode(CreateDomainStmt);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("domainname");
	var_key.val.string.val = strdup("domainname");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->domainname = NULL;
	else
		local_node->domainname = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("constraints");
	var_key.val.string.val = strdup("constraints");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->constraints = NULL;
	else
		local_node->constraints = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("typeName");
	var_key.val.string.val = strdup("typeName");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->typeName = NULL;
	} else {
		local_node->typeName = (TypeName *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("collClause");
	var_key.val.string.val = strdup("collClause");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->collClause = NULL;
	} else {
		local_node->collClause = (CollateClause *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *RowMarkClause_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		RowMarkClause *local_node;
		if (node_cast != NULL)
			local_node = (RowMarkClause *) node_cast;
		else
			local_node = makeNode(RowMarkClause);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("waitPolicy");
	var_key.val.string.val = strdup("waitPolicy");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->waitPolicy = DatumGetInt16(DirectFunctionCall1(numeric_int2, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("rti");
	var_key.val.string.val = strdup("rti");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->rti = DatumGetUInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("pushedDown");
	var_key.val.string.val = strdup("pushedDown");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	local_node->pushedDown = var_value->val.boolean;

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("strength");
	var_key.val.string.val = strdup("strength");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->strength = DatumGetInt16(DirectFunctionCall1(numeric_int2, NumericGetDatum(var_value->val.numeric)));

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *NestLoopParam_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		NestLoopParam *local_node;
		if (node_cast != NULL)
			local_node = (NestLoopParam *) node_cast;
		else
			local_node = makeNode(NestLoopParam);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("paramno");
	var_key.val.string.val = strdup("paramno");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->paramno = DatumGetInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("paramval");
	var_key.val.string.val = strdup("paramval");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->paramval = NULL;
	} else {
		local_node->paramval = (Var *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *WindowFunc_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		WindowFunc *local_node;
		if (node_cast != NULL)
			local_node = (WindowFunc *) node_cast;
		else
			local_node = makeNode(WindowFunc);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("args");
	var_key.val.string.val = strdup("args");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->args = NULL;
	else
		local_node->args = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("aggfilter");
	var_key.val.string.val = strdup("aggfilter");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->aggfilter = NULL;
	} else {
		local_node->aggfilter = (Expr *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("winfnoid");
	var_key.val.string.val = strdup("winfnoid");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->winfnoid = DatumGetUInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("winagg");
	var_key.val.string.val = strdup("winagg");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	local_node->winagg = var_value->val.boolean;

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("wincollid");
	var_key.val.string.val = strdup("wincollid");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->wincollid = DatumGetUInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("winref");
	var_key.val.string.val = strdup("winref");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->winref = DatumGetUInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("inputcollid");
	var_key.val.string.val = strdup("inputcollid");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->inputcollid = DatumGetUInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("location");
	var_key.val.string.val = strdup("location");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->location = DatumGetInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("wintype");
	var_key.val.string.val = strdup("wintype");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->wintype = DatumGetUInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("winstar");
	var_key.val.string.val = strdup("winstar");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	local_node->winstar = var_value->val.boolean;

			}
			{
					
	Expr_deser(container, (void *)&local_node->xpr, -1);

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *TableLikeClause_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		TableLikeClause *local_node;
		if (node_cast != NULL)
			local_node = (TableLikeClause *) node_cast;
		else
			local_node = makeNode(TableLikeClause);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("relation");
	var_key.val.string.val = strdup("relation");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->relation = NULL;
	} else {
		local_node->relation = (RangeVar *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("options");
	var_key.val.string.val = strdup("options");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->options = DatumGetUInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *Expr_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		Expr *local_node;
		if (node_cast != NULL)
			local_node = (Expr *) node_cast;
		else
			local_node = makeNode(Expr);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *RecursiveUnion_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		RecursiveUnion *local_node;
		if (node_cast != NULL)
			local_node = (RecursiveUnion *) node_cast;
		else
			local_node = makeNode(RecursiveUnion);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("numGroups");
	var_key.val.string.val = strdup("numGroups");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
#ifdef USE_FLOAT8_BYVAL
	local_node->numGroups = DatumGetInt64(DirectFunctionCall1(numeric_int8, NumericGetDatum(var_value->val.numeric)));
#else
	local_node->numGroups = DatumGetInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));
#endif

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("dupColIdx");
	var_key.val.string.val = strdup("dupColIdx");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

						
	{
		int type;
		int i = 0;
		JsonbValue v;
		JsonbIterator *it = JsonbIteratorInit(var_value->val.binary.data);
			local_node->numCols = it->nElems;
		local_node->dupColIdx = (AttrNumber*) palloc(sizeof(AttrNumber)*it->nElems);
		while ((type = JsonbIteratorNext(&it, &v, true)) != WJB_DONE)
		{
			if (type == WJB_ELEM)
			{
					
		local_node->dupColIdx[i] = DatumGetInt16(DirectFunctionCall1(numeric_int2, NumericGetDatum(v.val.numeric)));

				i++;
			}
		}
	}

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("dupOperators");
	var_key.val.string.val = strdup("dupOperators");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

						
	{
		int type;
		int i = 0;
		JsonbValue v;
		JsonbIterator *it = JsonbIteratorInit(var_value->val.binary.data);
			local_node->numCols = it->nElems;
		local_node->dupOperators = (Oid*) palloc(sizeof(Oid)*it->nElems);
		while ((type = JsonbIteratorNext(&it, &v, true)) != WJB_DONE)
		{
			if (type == WJB_ELEM)
			{
					
		local_node->dupOperators[i] = DatumGetUInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(v.val.numeric)));

				i++;
			}
		}
	}

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("wtParam");
	var_key.val.string.val = strdup("wtParam");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->wtParam = DatumGetInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	Plan_deser(container, (void *)&local_node->plan, -1);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("numCols");
	var_key.val.string.val = strdup("numCols");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->numCols = DatumGetInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *FromExpr_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		FromExpr *local_node;
		if (node_cast != NULL)
			local_node = (FromExpr *) node_cast;
		else
			local_node = makeNode(FromExpr);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("fromlist");
	var_key.val.string.val = strdup("fromlist");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->fromlist = NULL;
	else
		local_node->fromlist = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("quals");
	var_key.val.string.val = strdup("quals");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->quals = NULL;
	} else {
		local_node->quals = (Node *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *Alias_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		Alias *local_node;
		if (node_cast != NULL)
			local_node = (Alias *) node_cast;
		else
			local_node = makeNode(Alias);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("colnames");
	var_key.val.string.val = strdup("colnames");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->colnames = NULL;
	else
		local_node->colnames = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("aliasname");
	var_key.val.string.val = strdup("aliasname");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					if (var_value->type == jbvNull)
						local_node->aliasname = NULL;
					else
					{
						char *result = palloc(var_value->val.string.len + 1);
						memcpy(result, var_value->val.string.val, var_value->val.string.len);
						result[var_value->val.string.len] = '\0';
						local_node->aliasname = result;
					}
			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *AlterEnumStmt_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		AlterEnumStmt *local_node;
		if (node_cast != NULL)
			local_node = (AlterEnumStmt *) node_cast;
		else
			local_node = makeNode(AlterEnumStmt);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("newValIsAfter");
	var_key.val.string.val = strdup("newValIsAfter");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	local_node->newValIsAfter = var_value->val.boolean;

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("typeName");
	var_key.val.string.val = strdup("typeName");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->typeName = NULL;
	else
		local_node->typeName = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("newValNeighbor");
	var_key.val.string.val = strdup("newValNeighbor");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					if (var_value->type == jbvNull)
						local_node->newValNeighbor = NULL;
					else
					{
						char *result = palloc(var_value->val.string.len + 1);
						memcpy(result, var_value->val.string.val, var_value->val.string.len);
						result[var_value->val.string.len] = '\0';
						local_node->newValNeighbor = result;
					}
			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("newVal");
	var_key.val.string.val = strdup("newVal");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					if (var_value->type == jbvNull)
						local_node->newVal = NULL;
					else
					{
						char *result = palloc(var_value->val.string.len + 1);
						memcpy(result, var_value->val.string.val, var_value->val.string.len);
						result[var_value->val.string.len] = '\0';
						local_node->newVal = result;
					}
			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("skipIfExists");
	var_key.val.string.val = strdup("skipIfExists");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	local_node->skipIfExists = var_value->val.boolean;

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *RangeFunction_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		RangeFunction *local_node;
		if (node_cast != NULL)
			local_node = (RangeFunction *) node_cast;
		else
			local_node = makeNode(RangeFunction);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("is_rowsfrom");
	var_key.val.string.val = strdup("is_rowsfrom");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	local_node->is_rowsfrom = var_value->val.boolean;

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("functions");
	var_key.val.string.val = strdup("functions");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->functions = NULL;
	else
		local_node->functions = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("alias");
	var_key.val.string.val = strdup("alias");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->alias = NULL;
	} else {
		local_node->alias = (Alias *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("lateral");
	var_key.val.string.val = strdup("lateral");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	local_node->lateral = var_value->val.boolean;

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("coldeflist");
	var_key.val.string.val = strdup("coldeflist");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->coldeflist = NULL;
	else
		local_node->coldeflist = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("ordinality");
	var_key.val.string.val = strdup("ordinality");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	local_node->ordinality = var_value->val.boolean;

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *CreateTransformStmt_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		CreateTransformStmt *local_node;
		if (node_cast != NULL)
			local_node = (CreateTransformStmt *) node_cast;
		else
			local_node = makeNode(CreateTransformStmt);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("lang");
	var_key.val.string.val = strdup("lang");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					if (var_value->type == jbvNull)
						local_node->lang = NULL;
					else
					{
						char *result = palloc(var_value->val.string.len + 1);
						memcpy(result, var_value->val.string.val, var_value->val.string.len);
						result[var_value->val.string.len] = '\0';
						local_node->lang = result;
					}
			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("replace");
	var_key.val.string.val = strdup("replace");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	local_node->replace = var_value->val.boolean;

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("fromsql");
	var_key.val.string.val = strdup("fromsql");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->fromsql = NULL;
	} else {
		local_node->fromsql = (FuncWithArgs *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("tosql");
	var_key.val.string.val = strdup("tosql");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->tosql = NULL;
	} else {
		local_node->tosql = (FuncWithArgs *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("type_name");
	var_key.val.string.val = strdup("type_name");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->type_name = NULL;
	} else {
		local_node->type_name = (TypeName *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *AlterTableCmd_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		AlterTableCmd *local_node;
		if (node_cast != NULL)
			local_node = (AlterTableCmd *) node_cast;
		else
			local_node = makeNode(AlterTableCmd);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("missing_ok");
	var_key.val.string.val = strdup("missing_ok");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	local_node->missing_ok = var_value->val.boolean;

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("newowner");
	var_key.val.string.val = strdup("newowner");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->newowner = NULL;
	} else {
		local_node->newowner = (Node *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("def");
	var_key.val.string.val = strdup("def");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->def = NULL;
	} else {
		local_node->def = (Node *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("name");
	var_key.val.string.val = strdup("name");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					if (var_value->type == jbvNull)
						local_node->name = NULL;
					else
					{
						char *result = palloc(var_value->val.string.len + 1);
						memcpy(result, var_value->val.string.val, var_value->val.string.len);
						result[var_value->val.string.len] = '\0';
						local_node->name = result;
					}
			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("subtype");
	var_key.val.string.val = strdup("subtype");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->subtype = DatumGetInt16(DirectFunctionCall1(numeric_int2, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("behavior");
	var_key.val.string.val = strdup("behavior");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->behavior = DatumGetInt16(DirectFunctionCall1(numeric_int2, NumericGetDatum(var_value->val.numeric)));

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *GrantStmt_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		GrantStmt *local_node;
		if (node_cast != NULL)
			local_node = (GrantStmt *) node_cast;
		else
			local_node = makeNode(GrantStmt);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("objtype");
	var_key.val.string.val = strdup("objtype");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->objtype = DatumGetInt16(DirectFunctionCall1(numeric_int2, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("objects");
	var_key.val.string.val = strdup("objects");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->objects = NULL;
	else
		local_node->objects = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("grant_option");
	var_key.val.string.val = strdup("grant_option");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	local_node->grant_option = var_value->val.boolean;

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("targtype");
	var_key.val.string.val = strdup("targtype");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->targtype = DatumGetInt16(DirectFunctionCall1(numeric_int2, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("is_grant");
	var_key.val.string.val = strdup("is_grant");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	local_node->is_grant = var_value->val.boolean;

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("behavior");
	var_key.val.string.val = strdup("behavior");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->behavior = DatumGetInt16(DirectFunctionCall1(numeric_int2, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("privileges");
	var_key.val.string.val = strdup("privileges");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->privileges = NULL;
	else
		local_node->privileges = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("grantees");
	var_key.val.string.val = strdup("grantees");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->grantees = NULL;
	else
		local_node->grantees = list_deser(var_value->val.binary.data, false);

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *CreateTableSpaceStmt_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		CreateTableSpaceStmt *local_node;
		if (node_cast != NULL)
			local_node = (CreateTableSpaceStmt *) node_cast;
		else
			local_node = makeNode(CreateTableSpaceStmt);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("location");
	var_key.val.string.val = strdup("location");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					if (var_value->type == jbvNull)
						local_node->location = NULL;
					else
					{
						char *result = palloc(var_value->val.string.len + 1);
						memcpy(result, var_value->val.string.val, var_value->val.string.len);
						result[var_value->val.string.len] = '\0';
						local_node->location = result;
					}
			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("owner");
	var_key.val.string.val = strdup("owner");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->owner = NULL;
	} else {
		local_node->owner = (Node *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("options");
	var_key.val.string.val = strdup("options");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->options = NULL;
	else
		local_node->options = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("tablespacename");
	var_key.val.string.val = strdup("tablespacename");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					if (var_value->type == jbvNull)
						local_node->tablespacename = NULL;
					else
					{
						char *result = palloc(var_value->val.string.len + 1);
						memcpy(result, var_value->val.string.val, var_value->val.string.len);
						result[var_value->val.string.len] = '\0';
						local_node->tablespacename = result;
					}
			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *CreatePolicyStmt_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		CreatePolicyStmt *local_node;
		if (node_cast != NULL)
			local_node = (CreatePolicyStmt *) node_cast;
		else
			local_node = makeNode(CreatePolicyStmt);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("cmd_name");
	var_key.val.string.val = strdup("cmd_name");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					if (var_value->type == jbvNull)
						local_node->cmd_name = NULL;
					else
					{
						char *result = palloc(var_value->val.string.len + 1);
						memcpy(result, var_value->val.string.val, var_value->val.string.len);
						result[var_value->val.string.len] = '\0';
						local_node->cmd_name = result;
					}
			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("with_check");
	var_key.val.string.val = strdup("with_check");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->with_check = NULL;
	} else {
		local_node->with_check = (Node *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("table");
	var_key.val.string.val = strdup("table");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->table = NULL;
	} else {
		local_node->table = (RangeVar *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("policy_name");
	var_key.val.string.val = strdup("policy_name");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					if (var_value->type == jbvNull)
						local_node->policy_name = NULL;
					else
					{
						char *result = palloc(var_value->val.string.len + 1);
						memcpy(result, var_value->val.string.val, var_value->val.string.len);
						result[var_value->val.string.len] = '\0';
						local_node->policy_name = result;
					}
			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("qual");
	var_key.val.string.val = strdup("qual");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->qual = NULL;
	} else {
		local_node->qual = (Node *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("roles");
	var_key.val.string.val = strdup("roles");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->roles = NULL;
	else
		local_node->roles = list_deser(var_value->val.binary.data, false);

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *GrantRoleStmt_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		GrantRoleStmt *local_node;
		if (node_cast != NULL)
			local_node = (GrantRoleStmt *) node_cast;
		else
			local_node = makeNode(GrantRoleStmt);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("granted_roles");
	var_key.val.string.val = strdup("granted_roles");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->granted_roles = NULL;
	else
		local_node->granted_roles = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("admin_opt");
	var_key.val.string.val = strdup("admin_opt");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	local_node->admin_opt = var_value->val.boolean;

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("is_grant");
	var_key.val.string.val = strdup("is_grant");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	local_node->is_grant = var_value->val.boolean;

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("grantee_roles");
	var_key.val.string.val = strdup("grantee_roles");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->grantee_roles = NULL;
	else
		local_node->grantee_roles = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("behavior");
	var_key.val.string.val = strdup("behavior");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->behavior = DatumGetInt16(DirectFunctionCall1(numeric_int2, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("grantor");
	var_key.val.string.val = strdup("grantor");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->grantor = NULL;
	} else {
		local_node->grantor = (Node *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *DropRoleStmt_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		DropRoleStmt *local_node;
		if (node_cast != NULL)
			local_node = (DropRoleStmt *) node_cast;
		else
			local_node = makeNode(DropRoleStmt);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("missing_ok");
	var_key.val.string.val = strdup("missing_ok");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	local_node->missing_ok = var_value->val.boolean;

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("roles");
	var_key.val.string.val = strdup("roles");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->roles = NULL;
	else
		local_node->roles = list_deser(var_value->val.binary.data, false);

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *ViewStmt_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		ViewStmt *local_node;
		if (node_cast != NULL)
			local_node = (ViewStmt *) node_cast;
		else
			local_node = makeNode(ViewStmt);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("options");
	var_key.val.string.val = strdup("options");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->options = NULL;
	else
		local_node->options = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("replace");
	var_key.val.string.val = strdup("replace");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	local_node->replace = var_value->val.boolean;

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("withCheckOption");
	var_key.val.string.val = strdup("withCheckOption");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->withCheckOption = DatumGetInt16(DirectFunctionCall1(numeric_int2, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("aliases");
	var_key.val.string.val = strdup("aliases");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->aliases = NULL;
	else
		local_node->aliases = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("view");
	var_key.val.string.val = strdup("view");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->view = NULL;
	} else {
		local_node->view = (RangeVar *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("query");
	var_key.val.string.val = strdup("query");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->query = NULL;
	} else {
		local_node->query = (Node *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *A_Indices_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		A_Indices *local_node;
		if (node_cast != NULL)
			local_node = (A_Indices *) node_cast;
		else
			local_node = makeNode(A_Indices);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("lidx");
	var_key.val.string.val = strdup("lidx");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->lidx = NULL;
	} else {
		local_node->lidx = (Node *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("uidx");
	var_key.val.string.val = strdup("uidx");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	if (var_value->type == jbvNull) 
	{
		local_node->uidx = NULL;
	} else {
		local_node->uidx = (Node *) jsonb_to_node(var_value->val.binary.data);
	}
	

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *PlanInvalItem_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		PlanInvalItem *local_node;
		if (node_cast != NULL)
			local_node = (PlanInvalItem *) node_cast;
		else
			local_node = makeNode(PlanInvalItem);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("cacheId");
	var_key.val.string.val = strdup("cacheId");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->cacheId = DatumGetInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("hashValue");
	var_key.val.string.val = strdup("hashValue");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->hashValue = DatumGetUInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *Group_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		Group *local_node;
		if (node_cast != NULL)
			local_node = (Group *) node_cast;
		else
			local_node = makeNode(Group);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	Plan_deser(container, (void *)&local_node->plan, -1);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("numCols");
	var_key.val.string.val = strdup("numCols");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->numCols = DatumGetInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("grpColIdx");
	var_key.val.string.val = strdup("grpColIdx");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

						
	{
		int type;
		int i = 0;
		JsonbValue v;
		JsonbIterator *it = JsonbIteratorInit(var_value->val.binary.data);
			local_node->numCols = it->nElems;
		local_node->grpColIdx = (AttrNumber*) palloc(sizeof(AttrNumber)*it->nElems);
		while ((type = JsonbIteratorNext(&it, &v, true)) != WJB_DONE)
		{
			if (type == WJB_ELEM)
			{
					
		local_node->grpColIdx[i] = DatumGetInt16(DirectFunctionCall1(numeric_int2, NumericGetDatum(v.val.numeric)));

				i++;
			}
		}
	}

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("grpOperators");
	var_key.val.string.val = strdup("grpOperators");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

						
	{
		int type;
		int i = 0;
		JsonbValue v;
		JsonbIterator *it = JsonbIteratorInit(var_value->val.binary.data);
			local_node->numCols = it->nElems;
		local_node->grpOperators = (Oid*) palloc(sizeof(Oid)*it->nElems);
		while ((type = JsonbIteratorNext(&it, &v, true)) != WJB_DONE)
		{
			if (type == WJB_ELEM)
			{
					
		local_node->grpOperators[i] = DatumGetUInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(v.val.numeric)));

				i++;
			}
		}
	}

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *PlanRowMark_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		PlanRowMark *local_node;
		if (node_cast != NULL)
			local_node = (PlanRowMark *) node_cast;
		else
			local_node = makeNode(PlanRowMark);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("markType");
	var_key.val.string.val = strdup("markType");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->markType = DatumGetInt16(DirectFunctionCall1(numeric_int2, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("rti");
	var_key.val.string.val = strdup("rti");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->rti = DatumGetUInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("strength");
	var_key.val.string.val = strdup("strength");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->strength = DatumGetInt16(DirectFunctionCall1(numeric_int2, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("waitPolicy");
	var_key.val.string.val = strdup("waitPolicy");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->waitPolicy = DatumGetInt16(DirectFunctionCall1(numeric_int2, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("rowmarkId");
	var_key.val.string.val = strdup("rowmarkId");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->rowmarkId = DatumGetUInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("prti");
	var_key.val.string.val = strdup("prti");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->prti = DatumGetUInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("isParent");
	var_key.val.string.val = strdup("isParent");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
	local_node->isParent = var_value->val.boolean;

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("allMarkTypes");
	var_key.val.string.val = strdup("allMarkTypes");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					
		local_node->allMarkTypes = DatumGetInt32(DirectFunctionCall1(numeric_int4, NumericGetDatum(var_value->val.numeric)));

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *AlterFdwStmt_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		AlterFdwStmt *local_node;
		if (node_cast != NULL)
			local_node = (AlterFdwStmt *) node_cast;
		else
			local_node = makeNode(AlterFdwStmt);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("options");
	var_key.val.string.val = strdup("options");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->options = NULL;
	else
		local_node->options = list_deser(var_value->val.binary.data, false);

			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("fdwname");
	var_key.val.string.val = strdup("fdwname");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					if (var_value->type == jbvNull)
						local_node->fdwname = NULL;
					else
					{
						char *result = palloc(var_value->val.string.len + 1);
						memcpy(result, var_value->val.string.val, var_value->val.string.len);
						result[var_value->val.string.len] = '\0';
						local_node->fdwname = result;
					}
			}
			{
					
	JsonbValue *var_value;
	
	JsonbValue var_key;
	var_key.type = jbvString;
	var_key.val.string.len = strlen("func_options");
	var_key.val.string.val = strdup("func_options");

	var_value = findJsonbValueFromContainer(container,
											JB_FOBJECT,
											&var_key);

					

	if (var_value == NULL || var_value->type == jbvNull)
		local_node->func_options = NULL;
	else
		local_node->func_options = list_deser(var_value->val.binary.data, false);

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}
	static
	void *Material_deser(JsonbContainer *container, void *node_cast, int replace_type)
	{
		Material *local_node;
		if (node_cast != NULL)
			local_node = (Material *) node_cast;
		else
			local_node = makeNode(Material);

		if (replace_type >= 0)
			((Node *)local_node)->type = replace_type;

			{
					
	Plan_deser(container, (void *)&local_node->plan, -1);

			}
		if (hook)
			return hook(local_node);
		else
			return local_node;
	}


static
void *jsonb_to_node(JsonbContainer *container)
{
	JsonbValue *node_type;
	int16 node_type_value;
	
	
	JsonbValue node_type_key;
	node_type_key.type = jbvString;
	node_type_key.val.string.len = strlen("type");
	node_type_key.val.string.val = strdup("type");


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

	switch (node_type_value)
	{
		case T_RangeTableSample:
			return RangeTableSample_deser(container, NULL, -1);
		case T_CreateEnumStmt:
			return CreateEnumStmt_deser(container, NULL, -1);
		case T_SampleScan:
			return SampleScan_deser(container, NULL, -1);
		case T_SetOperationStmt:
			return SetOperationStmt_deser(container, NULL, -1);
		case T_AlterTSDictionaryStmt:
			return AlterTSDictionaryStmt_deser(container, NULL, -1);
		case T_SortGroupClause:
			return SortGroupClause_deser(container, NULL, -1);
		case T_NamedArgExpr:
			return NamedArgExpr_deser(container, NULL, -1);
		case T_CaseWhen:
			return CaseWhen_deser(container, NULL, -1);
		case T_CommentStmt:
			return CommentStmt_deser(container, NULL, -1);
		case T_VacuumStmt:
			return VacuumStmt_deser(container, NULL, -1);
		case T_AlterOwnerStmt:
			return AlterOwnerStmt_deser(container, NULL, -1);
		case T_Unique:
			return Unique_deser(container, NULL, -1);
		case T_MinMaxExpr:
			return MinMaxExpr_deser(container, NULL, -1);
		case T_TransactionStmt:
			return TransactionStmt_deser(container, NULL, -1);
		case T_CreateEventTrigStmt:
			return CreateEventTrigStmt_deser(container, NULL, -1);
		case T_ArrayRef:
			return ArrayRef_deser(container, NULL, -1);
		case T_AlterExtensionStmt:
			return AlterExtensionStmt_deser(container, NULL, -1);
		case T_CreateRoleStmt:
			return CreateRoleStmt_deser(container, NULL, -1);
		case T_AlterOpFamilyStmt:
			return AlterOpFamilyStmt_deser(container, NULL, -1);
		case T_LockStmt:
			return LockStmt_deser(container, NULL, -1);
		case T_AlterTableStmt:
			return AlterTableStmt_deser(container, NULL, -1);
		case T_CreateSchemaStmt:
			return CreateSchemaStmt_deser(container, NULL, -1);
		case T_ClosePortalStmt:
			return ClosePortalStmt_deser(container, NULL, -1);
		case T_RelabelType:
			return RelabelType_deser(container, NULL, -1);
		case T_FunctionScan:
			return FunctionScan_deser(container, NULL, -1);
		case T_AlterSeqStmt:
			return AlterSeqStmt_deser(container, NULL, -1);
		case T_SubPlan:
			return SubPlan_deser(container, NULL, -1);
		case T_ScalarArrayOpExpr:
			return ScalarArrayOpExpr_deser(container, NULL, -1);
		case T_RoleSpec:
			return RoleSpec_deser(container, NULL, -1);
		case T_InlineCodeBlock:
			return InlineCodeBlock_deser(container, NULL, -1);
		case T_VariableShowStmt:
			return VariableShowStmt_deser(container, NULL, -1);
		case T_ImportForeignSchemaStmt:
			return ImportForeignSchemaStmt_deser(container, NULL, -1);
		case T_AlterForeignServerStmt:
			return AlterForeignServerStmt_deser(container, NULL, -1);
		case T_ModifyTable:
			return ModifyTable_deser(container, NULL, -1);
		case T_CheckPointStmt:
			return CheckPointStmt_deser(container, NULL, -1);
		case T_RangeVar:
			return RangeVar_deser(container, NULL, -1);
		case T_AlterExtensionContentsStmt:
			return AlterExtensionContentsStmt_deser(container, NULL, -1);
		case T_RangeTblRef:
			return RangeTblRef_deser(container, NULL, -1);
		case T_CreateOpClassStmt:
			return CreateOpClassStmt_deser(container, NULL, -1);
		case T_LoadStmt:
			return LoadStmt_deser(container, NULL, -1);
		case T_ColumnRef:
			return ColumnRef_deser(container, NULL, -1);
		case T_CreateTrigStmt:
			return CreateTrigStmt_deser(container, NULL, -1);
		case T_ConstraintsSetStmt:
			return ConstraintsSetStmt_deser(container, NULL, -1);
		case T_Var:
			return Var_deser(container, NULL, -1);
		case T_BitmapIndexScan:
			return BitmapIndexScan_deser(container, NULL, -1);
		case T_Plan:
			return Plan_deser(container, NULL, -1);
		case T_CreateStmt:
			return CreateStmt_deser(container, NULL, -1);
		case T_InferClause:
			return InferClause_deser(container, NULL, -1);
		case T_Param:
			return Param_deser(container, NULL, -1);
		case T_ExecuteStmt:
			return ExecuteStmt_deser(container, NULL, -1);
		case T_DropdbStmt:
			return DropdbStmt_deser(container, NULL, -1);
		case T_FetchStmt:
			return FetchStmt_deser(container, NULL, -1);
		case T_HashJoin:
			return HashJoin_deser(container, NULL, -1);
		case T_ColumnDef:
			return ColumnDef_deser(container, NULL, -1);
		case T_AlterRoleSetStmt:
			return AlterRoleSetStmt_deser(container, NULL, -1);
		case T_Result:
			return Result_deser(container, NULL, -1);
		case T_PrepareStmt:
			return PrepareStmt_deser(container, NULL, -1);
		case T_AlterDomainStmt:
			return AlterDomainStmt_deser(container, NULL, -1);
		case T_CompositeTypeStmt:
			return CompositeTypeStmt_deser(container, NULL, -1);
		case T_CustomScan:
			return CustomScan_deser(container, NULL, -1);
		case T_Agg:
			return Agg_deser(container, NULL, -1);
		case T_AlterObjectSchemaStmt:
			return AlterObjectSchemaStmt_deser(container, NULL, -1);
		case T_AlterEventTrigStmt:
			return AlterEventTrigStmt_deser(container, NULL, -1);
		case T_AlterDefaultPrivilegesStmt:
			return AlterDefaultPrivilegesStmt_deser(container, NULL, -1);
		case T_PlannedStmt:
			return PlannedStmt_deser(container, NULL, -1);
		case T_Aggref:
			return Aggref_deser(container, NULL, -1);
		case T_SubqueryScan:
			return SubqueryScan_deser(container, NULL, -1);
		case T_CreateFunctionStmt:
			return CreateFunctionStmt_deser(container, NULL, -1);
		case T_CreateCastStmt:
			return CreateCastStmt_deser(container, NULL, -1);
		case T_CteScan:
			return CteScan_deser(container, NULL, -1);
		case T_IndexElem:
			return IndexElem_deser(container, NULL, -1);
		case T_CreateFdwStmt:
			return CreateFdwStmt_deser(container, NULL, -1);
		case T_NestLoop:
			return NestLoop_deser(container, NULL, -1);
		case T_TypeCast:
			return TypeCast_deser(container, NULL, -1);
		case T_CoerceToDomainValue:
			return CoerceToDomainValue_deser(container, NULL, -1);
		case T_InsertStmt:
			return InsertStmt_deser(container, NULL, -1);
		case T_SortBy:
			return SortBy_deser(container, NULL, -1);
		case T_ReassignOwnedStmt:
			return ReassignOwnedStmt_deser(container, NULL, -1);
		case T_TypeName:
			return TypeName_deser(container, NULL, -1);
		case T_Constraint:
			return Constraint_deser(container, NULL, -1);
		case T_OnConflictClause:
			return OnConflictClause_deser(container, NULL, -1);
		case T_AlterPolicyStmt:
			return AlterPolicyStmt_deser(container, NULL, -1);
		case T_GroupingFunc:
			return GroupingFunc_deser(container, NULL, -1);
		case T_SelectStmt:
			return SelectStmt_deser(container, NULL, -1);
		case T_CopyStmt:
			return CopyStmt_deser(container, NULL, -1);
		case T_ArrayExpr:
			return ArrayExpr_deser(container, NULL, -1);
		case T_InferenceElem:
			return InferenceElem_deser(container, NULL, -1);
		case T_BitmapOr:
			return BitmapOr_deser(container, NULL, -1);
		case T_FuncExpr:
			return FuncExpr_deser(container, NULL, -1);
		case T_AlterUserMappingStmt:
			return AlterUserMappingStmt_deser(container, NULL, -1);
		case T_SetOp:
			return SetOp_deser(container, NULL, -1);
		case T_AlterTSConfigurationStmt:
			return AlterTSConfigurationStmt_deser(container, NULL, -1);
		case T_AlterRoleStmt:
			return AlterRoleStmt_deser(container, NULL, -1);
		case T_Sort:
			return Sort_deser(container, NULL, -1);
		case T_DiscardStmt:
			return DiscardStmt_deser(container, NULL, -1);
		case T_CreateForeignServerStmt:
			return CreateForeignServerStmt_deser(container, NULL, -1);
		case T_CaseExpr:
			return CaseExpr_deser(container, NULL, -1);
		case T_CreateExtensionStmt:
			return CreateExtensionStmt_deser(container, NULL, -1);
		case T_CommonTableExpr:
			return CommonTableExpr_deser(container, NULL, -1);
		case T_RenameStmt:
			return RenameStmt_deser(container, NULL, -1);
		case T_A_Star:
			return A_Star_deser(container, NULL, -1);
		case T_UnlistenStmt:
			return UnlistenStmt_deser(container, NULL, -1);
		case T_GroupingSet:
			return GroupingSet_deser(container, NULL, -1);
		case T_BoolExpr:
			return BoolExpr_deser(container, NULL, -1);
		case T_BitmapAnd:
			return BitmapAnd_deser(container, NULL, -1);
		case T_RowExpr:
			return RowExpr_deser(container, NULL, -1);
		case T_UpdateStmt:
			return UpdateStmt_deser(container, NULL, -1);
		case T_CollateExpr:
			return CollateExpr_deser(container, NULL, -1);
		case T_DropTableSpaceStmt:
			return DropTableSpaceStmt_deser(container, NULL, -1);
		case T_DeallocateStmt:
			return DeallocateStmt_deser(container, NULL, -1);
		case T_CreatedbStmt:
			return CreatedbStmt_deser(container, NULL, -1);
		case T_Hash:
			return Hash_deser(container, NULL, -1);
		case T_CurrentOfExpr:
			return CurrentOfExpr_deser(container, NULL, -1);
		case T_WindowDef:
			return WindowDef_deser(container, NULL, -1);
		case T_AlterFunctionStmt:
			return AlterFunctionStmt_deser(container, NULL, -1);
		case T_ExplainStmt:
			return ExplainStmt_deser(container, NULL, -1);
		case T_AlterDatabaseStmt:
			return AlterDatabaseStmt_deser(container, NULL, -1);
		case T_ParamRef:
			return ParamRef_deser(container, NULL, -1);
		case T_CreateForeignTableStmt:
			return CreateForeignTableStmt_deser(container, NULL, -1);
		case T_BitmapHeapScan:
			return BitmapHeapScan_deser(container, NULL, -1);
		case T_Scan:
			return Scan_deser(container, NULL, -1);
		case T_AlterTableSpaceOptionsStmt:
			return AlterTableSpaceOptionsStmt_deser(container, NULL, -1);
		case T_FuncCall:
			return FuncCall_deser(container, NULL, -1);
		case T_Join:
			return Join_deser(container, NULL, -1);
		case T_ReindexStmt:
			return ReindexStmt_deser(container, NULL, -1);
		case T_WithClause:
			return WithClause_deser(container, NULL, -1);
		case T_RangeSubselect:
			return RangeSubselect_deser(container, NULL, -1);
		case T_CreatePLangStmt:
			return CreatePLangStmt_deser(container, NULL, -1);
		case T_Const:
			return Const_deser(container, NULL, -1);
		case T_XmlExpr:
			return XmlExpr_deser(container, NULL, -1);
		case T_FuncWithArgs:
			return FuncWithArgs_deser(container, NULL, -1);
		case T_CollateClause:
			return CollateClause_deser(container, NULL, -1);
		case T_CreateUserMappingStmt:
			return CreateUserMappingStmt_deser(container, NULL, -1);
		case T_ListenStmt:
			return ListenStmt_deser(container, NULL, -1);
		case T_RefreshMatViewStmt:
			return RefreshMatViewStmt_deser(container, NULL, -1);
		case T_DeclareCursorStmt:
			return DeclareCursorStmt_deser(container, NULL, -1);
		case T_CreateConversionStmt:
			return CreateConversionStmt_deser(container, NULL, -1);
		case T_Value:
			return Value_deser(container, NULL, -1);
		case T_DropStmt:
			return DropStmt_deser(container, NULL, -1);
		case T_Limit:
			return Limit_deser(container, NULL, -1);
		case T_IntoClause:
			return IntoClause_deser(container, NULL, -1);
		case T_AlternativeSubPlan:
			return AlternativeSubPlan_deser(container, NULL, -1);
		case T_ForeignScan:
			return ForeignScan_deser(container, NULL, -1);
		case T_A_Const:
			return A_Const_deser(container, NULL, -1);
		case T_AlterSystemStmt:
			return AlterSystemStmt_deser(container, NULL, -1);
		case T_ResTarget:
			return ResTarget_deser(container, NULL, -1);
		case T_RangeTblEntry:
			return RangeTblEntry_deser(container, NULL, -1);
		case T_A_Indirection:
			return A_Indirection_deser(container, NULL, -1);
		case T_Append:
			return Append_deser(container, NULL, -1);
		case T_TableSampleClause:
			return TableSampleClause_deser(container, NULL, -1);
		case T_BooleanTest:
			return BooleanTest_deser(container, NULL, -1);
		case T_ArrayCoerceExpr:
			return ArrayCoerceExpr_deser(container, NULL, -1);
		case T_WithCheckOption:
			return WithCheckOption_deser(container, NULL, -1);
		case T_RowCompareExpr:
			return RowCompareExpr_deser(container, NULL, -1);
		case T_MultiAssignRef:
			return MultiAssignRef_deser(container, NULL, -1);
		case T_TruncateStmt:
			return TruncateStmt_deser(container, NULL, -1);
		case T_CoerceToDomain:
			return CoerceToDomain_deser(container, NULL, -1);
		case T_WindowClause:
			return WindowClause_deser(container, NULL, -1);
		case T_DefElem:
			return DefElem_deser(container, NULL, -1);
		case T_DropUserMappingStmt:
			return DropUserMappingStmt_deser(container, NULL, -1);
		case T_A_ArrayExpr:
			return A_ArrayExpr_deser(container, NULL, -1);
		case T_CreateTableAsStmt:
			return CreateTableAsStmt_deser(container, NULL, -1);
		case T_OpExpr:
			return OpExpr_deser(container, NULL, -1);
		case T_FieldStore:
			return FieldStore_deser(container, NULL, -1);
		case T_CoerceViaIO:
			return CoerceViaIO_deser(container, NULL, -1);
		case T_SubLink:
			return SubLink_deser(container, NULL, -1);
		case T_WorkTableScan:
			return WorkTableScan_deser(container, NULL, -1);
		case T_A_Expr:
			return A_Expr_deser(container, NULL, -1);
		case T_DropOwnedStmt:
			return DropOwnedStmt_deser(container, NULL, -1);
		case T_AlterDatabaseSetStmt:
			return AlterDatabaseSetStmt_deser(container, NULL, -1);
		case T_CreateRangeStmt:
			return CreateRangeStmt_deser(container, NULL, -1);
		case T_DeleteStmt:
			return DeleteStmt_deser(container, NULL, -1);
		case T_CreateSeqStmt:
			return CreateSeqStmt_deser(container, NULL, -1);
		case T_RuleStmt:
			return RuleStmt_deser(container, NULL, -1);
		case T_RangeTblFunction:
			return RangeTblFunction_deser(container, NULL, -1);
		case T_IndexStmt:
			return IndexStmt_deser(container, NULL, -1);
		case T_WindowAgg:
			return WindowAgg_deser(container, NULL, -1);
		case T_ConvertRowtypeExpr:
			return ConvertRowtypeExpr_deser(container, NULL, -1);
		case T_MergeAppend:
			return MergeAppend_deser(container, NULL, -1);
		case T_AccessPriv:
			return AccessPriv_deser(container, NULL, -1);
		case T_CreateOpFamilyStmt:
			return CreateOpFamilyStmt_deser(container, NULL, -1);
		case T_CoalesceExpr:
			return CoalesceExpr_deser(container, NULL, -1);
		case T_DoStmt:
			return DoStmt_deser(container, NULL, -1);
		case T_IndexOnlyScan:
			return IndexOnlyScan_deser(container, NULL, -1);
		case T_XmlSerialize:
			return XmlSerialize_deser(container, NULL, -1);
		case T_FunctionParameter:
			return FunctionParameter_deser(container, NULL, -1);
		case T_SetToDefault:
			return SetToDefault_deser(container, NULL, -1);
		case T_JoinExpr:
			return JoinExpr_deser(container, NULL, -1);
		case T_NullTest:
			return NullTest_deser(container, NULL, -1);
		case T_NotifyStmt:
			return NotifyStmt_deser(container, NULL, -1);
		case T_AlterTableMoveAllStmt:
			return AlterTableMoveAllStmt_deser(container, NULL, -1);
		case T_LockingClause:
			return LockingClause_deser(container, NULL, -1);
		case T_DefineStmt:
			return DefineStmt_deser(container, NULL, -1);
		case T_CreateOpClassItem:
			return CreateOpClassItem_deser(container, NULL, -1);
		case T_ClusterStmt:
			return ClusterStmt_deser(container, NULL, -1);
		case T_MergeJoin:
			return MergeJoin_deser(container, NULL, -1);
		case T_SecLabelStmt:
			return SecLabelStmt_deser(container, NULL, -1);
		case T_CaseTestExpr:
			return CaseTestExpr_deser(container, NULL, -1);
		case T_TidScan:
			return TidScan_deser(container, NULL, -1);
		case T_Query:
			return Query_deser(container, NULL, -1);
		case T_ReplicaIdentityStmt:
			return ReplicaIdentityStmt_deser(container, NULL, -1);
		case T_IndexScan:
			return IndexScan_deser(container, NULL, -1);
		case T_ValuesScan:
			return ValuesScan_deser(container, NULL, -1);
		case T_FieldSelect:
			return FieldSelect_deser(container, NULL, -1);
		case T_OnConflictExpr:
			return OnConflictExpr_deser(container, NULL, -1);
		case T_VariableSetStmt:
			return VariableSetStmt_deser(container, NULL, -1);
		case T_TargetEntry:
			return TargetEntry_deser(container, NULL, -1);
		case T_LockRows:
			return LockRows_deser(container, NULL, -1);
		case T_CreateDomainStmt:
			return CreateDomainStmt_deser(container, NULL, -1);
		case T_RowMarkClause:
			return RowMarkClause_deser(container, NULL, -1);
		case T_NestLoopParam:
			return NestLoopParam_deser(container, NULL, -1);
		case T_WindowFunc:
			return WindowFunc_deser(container, NULL, -1);
		case T_TableLikeClause:
			return TableLikeClause_deser(container, NULL, -1);
		case T_Expr:
			return Expr_deser(container, NULL, -1);
		case T_RecursiveUnion:
			return RecursiveUnion_deser(container, NULL, -1);
		case T_FromExpr:
			return FromExpr_deser(container, NULL, -1);
		case T_Alias:
			return Alias_deser(container, NULL, -1);
		case T_AlterEnumStmt:
			return AlterEnumStmt_deser(container, NULL, -1);
		case T_RangeFunction:
			return RangeFunction_deser(container, NULL, -1);
		case T_CreateTransformStmt:
			return CreateTransformStmt_deser(container, NULL, -1);
		case T_AlterTableCmd:
			return AlterTableCmd_deser(container, NULL, -1);
		case T_GrantStmt:
			return GrantStmt_deser(container, NULL, -1);
		case T_CreateTableSpaceStmt:
			return CreateTableSpaceStmt_deser(container, NULL, -1);
		case T_CreatePolicyStmt:
			return CreatePolicyStmt_deser(container, NULL, -1);
		case T_GrantRoleStmt:
			return GrantRoleStmt_deser(container, NULL, -1);
		case T_DropRoleStmt:
			return DropRoleStmt_deser(container, NULL, -1);
		case T_ViewStmt:
			return ViewStmt_deser(container, NULL, -1);
		case T_A_Indices:
			return A_Indices_deser(container, NULL, -1);
		case T_PlanInvalItem:
			return PlanInvalItem_deser(container, NULL, -1);
		case T_Group:
			return Group_deser(container, NULL, -1);
		case T_PlanRowMark:
			return PlanRowMark_deser(container, NULL, -1);
		case T_AlterFdwStmt:
			return AlterFdwStmt_deser(container, NULL, -1);
		case T_Material:
			return Material_deser(container, NULL, -1);
		case T_SeqScan:
			return Scan_deser(container, NULL, T_SeqScan);
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