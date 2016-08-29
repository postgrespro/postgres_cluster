#include "sr_plan.h"

static
JsonbValue *node_to_jsonb(const void *obj, JsonbParseState *state);

static Oid remove_fake_func = 0;
static bool skip_location = false;





















	static
	JsonbValue *RangeTableSample_ser(const RangeTableSample *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *CreateEnumStmt_ser(const CreateEnumStmt *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *SampleScan_ser(const SampleScan *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *SetOperationStmt_ser(const SetOperationStmt *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *AlterTSDictionaryStmt_ser(const AlterTSDictionaryStmt *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *SortGroupClause_ser(const SortGroupClause *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *NamedArgExpr_ser(const NamedArgExpr *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *CaseWhen_ser(const CaseWhen *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *CommentStmt_ser(const CommentStmt *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *VacuumStmt_ser(const VacuumStmt *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *AlterOwnerStmt_ser(const AlterOwnerStmt *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *Unique_ser(const Unique *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *MinMaxExpr_ser(const MinMaxExpr *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *TransactionStmt_ser(const TransactionStmt *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *CreateEventTrigStmt_ser(const CreateEventTrigStmt *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *ArrayRef_ser(const ArrayRef *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *AlterExtensionStmt_ser(const AlterExtensionStmt *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *CreateRoleStmt_ser(const CreateRoleStmt *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *AlterOpFamilyStmt_ser(const AlterOpFamilyStmt *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *LockStmt_ser(const LockStmt *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *AlterTableStmt_ser(const AlterTableStmt *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *CreateSchemaStmt_ser(const CreateSchemaStmt *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *ClosePortalStmt_ser(const ClosePortalStmt *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *RelabelType_ser(const RelabelType *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *FunctionScan_ser(const FunctionScan *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *AlterSeqStmt_ser(const AlterSeqStmt *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *SubPlan_ser(const SubPlan *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *ScalarArrayOpExpr_ser(const ScalarArrayOpExpr *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *RoleSpec_ser(const RoleSpec *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *InlineCodeBlock_ser(const InlineCodeBlock *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *VariableShowStmt_ser(const VariableShowStmt *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *ImportForeignSchemaStmt_ser(const ImportForeignSchemaStmt *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *AlterForeignServerStmt_ser(const AlterForeignServerStmt *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *ModifyTable_ser(const ModifyTable *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *CheckPointStmt_ser(const CheckPointStmt *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *RangeVar_ser(const RangeVar *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *AlterExtensionContentsStmt_ser(const AlterExtensionContentsStmt *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *RangeTblRef_ser(const RangeTblRef *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *CreateOpClassStmt_ser(const CreateOpClassStmt *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *LoadStmt_ser(const LoadStmt *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *ColumnRef_ser(const ColumnRef *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *CreateTrigStmt_ser(const CreateTrigStmt *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *ConstraintsSetStmt_ser(const ConstraintsSetStmt *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *Var_ser(const Var *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *BitmapIndexScan_ser(const BitmapIndexScan *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *Plan_ser(const Plan *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *CreateStmt_ser(const CreateStmt *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *InferClause_ser(const InferClause *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *Param_ser(const Param *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *ExecuteStmt_ser(const ExecuteStmt *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *DropdbStmt_ser(const DropdbStmt *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *FetchStmt_ser(const FetchStmt *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *HashJoin_ser(const HashJoin *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *ColumnDef_ser(const ColumnDef *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *AlterRoleSetStmt_ser(const AlterRoleSetStmt *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *Result_ser(const Result *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *PrepareStmt_ser(const PrepareStmt *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *AlterDomainStmt_ser(const AlterDomainStmt *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *CompositeTypeStmt_ser(const CompositeTypeStmt *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *CustomScan_ser(const CustomScan *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *Agg_ser(const Agg *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *AlterObjectSchemaStmt_ser(const AlterObjectSchemaStmt *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *AlterEventTrigStmt_ser(const AlterEventTrigStmt *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *AlterDefaultPrivilegesStmt_ser(const AlterDefaultPrivilegesStmt *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *PlannedStmt_ser(const PlannedStmt *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *Aggref_ser(const Aggref *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *SubqueryScan_ser(const SubqueryScan *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *CreateFunctionStmt_ser(const CreateFunctionStmt *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *CreateCastStmt_ser(const CreateCastStmt *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *CteScan_ser(const CteScan *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *IndexElem_ser(const IndexElem *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *CreateFdwStmt_ser(const CreateFdwStmt *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *NestLoop_ser(const NestLoop *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *TypeCast_ser(const TypeCast *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *CoerceToDomainValue_ser(const CoerceToDomainValue *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *InsertStmt_ser(const InsertStmt *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *SortBy_ser(const SortBy *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *ReassignOwnedStmt_ser(const ReassignOwnedStmt *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *TypeName_ser(const TypeName *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *Constraint_ser(const Constraint *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *OnConflictClause_ser(const OnConflictClause *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *AlterPolicyStmt_ser(const AlterPolicyStmt *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *GroupingFunc_ser(const GroupingFunc *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *SelectStmt_ser(const SelectStmt *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *CopyStmt_ser(const CopyStmt *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *ArrayExpr_ser(const ArrayExpr *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *InferenceElem_ser(const InferenceElem *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *BitmapOr_ser(const BitmapOr *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *FuncExpr_ser(const FuncExpr *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *AlterUserMappingStmt_ser(const AlterUserMappingStmt *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *SetOp_ser(const SetOp *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *AlterTSConfigurationStmt_ser(const AlterTSConfigurationStmt *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *AlterRoleStmt_ser(const AlterRoleStmt *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *Sort_ser(const Sort *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *DiscardStmt_ser(const DiscardStmt *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *CreateForeignServerStmt_ser(const CreateForeignServerStmt *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *CaseExpr_ser(const CaseExpr *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *CreateExtensionStmt_ser(const CreateExtensionStmt *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *CommonTableExpr_ser(const CommonTableExpr *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *RenameStmt_ser(const RenameStmt *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *A_Star_ser(const A_Star *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *UnlistenStmt_ser(const UnlistenStmt *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *GroupingSet_ser(const GroupingSet *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *BoolExpr_ser(const BoolExpr *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *BitmapAnd_ser(const BitmapAnd *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *RowExpr_ser(const RowExpr *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *UpdateStmt_ser(const UpdateStmt *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *CollateExpr_ser(const CollateExpr *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *DropTableSpaceStmt_ser(const DropTableSpaceStmt *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *DeallocateStmt_ser(const DeallocateStmt *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *CreatedbStmt_ser(const CreatedbStmt *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *Hash_ser(const Hash *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *CurrentOfExpr_ser(const CurrentOfExpr *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *WindowDef_ser(const WindowDef *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *AlterFunctionStmt_ser(const AlterFunctionStmt *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *ExplainStmt_ser(const ExplainStmt *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *AlterDatabaseStmt_ser(const AlterDatabaseStmt *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *ParamRef_ser(const ParamRef *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *CreateForeignTableStmt_ser(const CreateForeignTableStmt *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *BitmapHeapScan_ser(const BitmapHeapScan *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *Scan_ser(const Scan *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *AlterTableSpaceOptionsStmt_ser(const AlterTableSpaceOptionsStmt *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *FuncCall_ser(const FuncCall *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *Join_ser(const Join *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *ReindexStmt_ser(const ReindexStmt *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *WithClause_ser(const WithClause *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *RangeSubselect_ser(const RangeSubselect *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *CreatePLangStmt_ser(const CreatePLangStmt *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *Const_ser(const Const *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *XmlExpr_ser(const XmlExpr *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *FuncWithArgs_ser(const FuncWithArgs *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *CollateClause_ser(const CollateClause *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *CreateUserMappingStmt_ser(const CreateUserMappingStmt *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *ListenStmt_ser(const ListenStmt *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *RefreshMatViewStmt_ser(const RefreshMatViewStmt *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *DeclareCursorStmt_ser(const DeclareCursorStmt *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *CreateConversionStmt_ser(const CreateConversionStmt *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *Value_ser(const Value *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *DropStmt_ser(const DropStmt *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *Limit_ser(const Limit *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *IntoClause_ser(const IntoClause *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *AlternativeSubPlan_ser(const AlternativeSubPlan *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *ForeignScan_ser(const ForeignScan *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *A_Const_ser(const A_Const *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *AlterSystemStmt_ser(const AlterSystemStmt *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *ResTarget_ser(const ResTarget *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *RangeTblEntry_ser(const RangeTblEntry *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *A_Indirection_ser(const A_Indirection *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *Append_ser(const Append *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *TableSampleClause_ser(const TableSampleClause *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *BooleanTest_ser(const BooleanTest *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *ArrayCoerceExpr_ser(const ArrayCoerceExpr *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *WithCheckOption_ser(const WithCheckOption *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *RowCompareExpr_ser(const RowCompareExpr *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *MultiAssignRef_ser(const MultiAssignRef *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *TruncateStmt_ser(const TruncateStmt *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *CoerceToDomain_ser(const CoerceToDomain *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *WindowClause_ser(const WindowClause *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *DefElem_ser(const DefElem *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *DropUserMappingStmt_ser(const DropUserMappingStmt *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *A_ArrayExpr_ser(const A_ArrayExpr *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *CreateTableAsStmt_ser(const CreateTableAsStmt *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *OpExpr_ser(const OpExpr *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *FieldStore_ser(const FieldStore *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *CoerceViaIO_ser(const CoerceViaIO *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *SubLink_ser(const SubLink *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *WorkTableScan_ser(const WorkTableScan *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *A_Expr_ser(const A_Expr *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *DropOwnedStmt_ser(const DropOwnedStmt *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *AlterDatabaseSetStmt_ser(const AlterDatabaseSetStmt *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *CreateRangeStmt_ser(const CreateRangeStmt *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *DeleteStmt_ser(const DeleteStmt *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *CreateSeqStmt_ser(const CreateSeqStmt *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *RuleStmt_ser(const RuleStmt *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *RangeTblFunction_ser(const RangeTblFunction *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *IndexStmt_ser(const IndexStmt *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *WindowAgg_ser(const WindowAgg *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *ConvertRowtypeExpr_ser(const ConvertRowtypeExpr *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *MergeAppend_ser(const MergeAppend *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *AccessPriv_ser(const AccessPriv *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *CreateOpFamilyStmt_ser(const CreateOpFamilyStmt *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *CoalesceExpr_ser(const CoalesceExpr *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *DoStmt_ser(const DoStmt *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *IndexOnlyScan_ser(const IndexOnlyScan *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *XmlSerialize_ser(const XmlSerialize *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *FunctionParameter_ser(const FunctionParameter *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *SetToDefault_ser(const SetToDefault *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *JoinExpr_ser(const JoinExpr *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *NullTest_ser(const NullTest *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *NotifyStmt_ser(const NotifyStmt *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *AlterTableMoveAllStmt_ser(const AlterTableMoveAllStmt *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *LockingClause_ser(const LockingClause *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *DefineStmt_ser(const DefineStmt *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *CreateOpClassItem_ser(const CreateOpClassItem *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *ClusterStmt_ser(const ClusterStmt *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *MergeJoin_ser(const MergeJoin *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *SecLabelStmt_ser(const SecLabelStmt *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *CaseTestExpr_ser(const CaseTestExpr *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *TidScan_ser(const TidScan *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *Query_ser(const Query *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *ReplicaIdentityStmt_ser(const ReplicaIdentityStmt *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *IndexScan_ser(const IndexScan *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *ValuesScan_ser(const ValuesScan *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *FieldSelect_ser(const FieldSelect *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *OnConflictExpr_ser(const OnConflictExpr *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *VariableSetStmt_ser(const VariableSetStmt *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *TargetEntry_ser(const TargetEntry *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *LockRows_ser(const LockRows *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *CreateDomainStmt_ser(const CreateDomainStmt *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *RowMarkClause_ser(const RowMarkClause *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *NestLoopParam_ser(const NestLoopParam *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *WindowFunc_ser(const WindowFunc *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *TableLikeClause_ser(const TableLikeClause *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *Expr_ser(const Expr *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *RecursiveUnion_ser(const RecursiveUnion *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *FromExpr_ser(const FromExpr *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *Alias_ser(const Alias *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *AlterEnumStmt_ser(const AlterEnumStmt *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *RangeFunction_ser(const RangeFunction *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *CreateTransformStmt_ser(const CreateTransformStmt *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *AlterTableCmd_ser(const AlterTableCmd *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *GrantStmt_ser(const GrantStmt *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *CreateTableSpaceStmt_ser(const CreateTableSpaceStmt *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *CreatePolicyStmt_ser(const CreatePolicyStmt *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *GrantRoleStmt_ser(const GrantRoleStmt *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *DropRoleStmt_ser(const DropRoleStmt *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *ViewStmt_ser(const ViewStmt *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *A_Indices_ser(const A_Indices *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *PlanInvalItem_ser(const PlanInvalItem *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *Group_ser(const Group *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *PlanRowMark_ser(const PlanRowMark *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *AlterFdwStmt_ser(const AlterFdwStmt *node, JsonbParseState *state, bool sub_object);
	static
	JsonbValue *Material_ser(const Material *node, JsonbParseState *state, bool sub_object);


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

	static
	JsonbValue *RangeTableSample_ser(const RangeTableSample *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


					
	key.type = jbvString;
	key.val.string.len = strlen("args");
	key.val.string.val = strdup("args");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->args, state);

					if(!skip_location)
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("location");
	key.val.string.val = strdup("location");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, Int32GetDatum(node->location)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("method");
	key.val.string.val = strdup("method");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->method, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("relation");
	key.val.string.val = strdup("relation");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->relation, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("repeatable");
	key.val.string.val = strdup("repeatable");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->repeatable, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *CreateEnumStmt_ser(const CreateEnumStmt *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("typeName");
	key.val.string.val = strdup("typeName");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->typeName, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("vals");
	key.val.string.val = strdup("vals");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->vals, state);

		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *SampleScan_ser(const SampleScan *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


				
	
	key.type = jbvString;
	key.val.string.len = strlen("scan");
	key.val.string.val = strdup("scan");
	pushJsonbValue(&state, WJB_KEY, &key);

	Scan_ser(&node->scan, state, false);

					
	key.type = jbvString;
	key.val.string.len = strlen("tablesample");
	key.val.string.val = strdup("tablesample");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->tablesample, state);

		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *SetOperationStmt_ser(const SetOperationStmt *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("all");
	key.val.string.val = strdup("all");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvBool;
	val.val.boolean = node->all;
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("colCollations");
	key.val.string.val = strdup("colCollations");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->colCollations, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("colTypes");
	key.val.string.val = strdup("colTypes");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->colTypes, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("colTypmods");
	key.val.string.val = strdup("colTypmods");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->colTypmods, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("groupClauses");
	key.val.string.val = strdup("groupClauses");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->groupClauses, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("larg");
	key.val.string.val = strdup("larg");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->larg, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("op");
	key.val.string.val = strdup("op");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->op)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("rarg");
	key.val.string.val = strdup("rarg");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->rarg, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *AlterTSDictionaryStmt_ser(const AlterTSDictionaryStmt *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


					
	key.type = jbvString;
	key.val.string.len = strlen("dictname");
	key.val.string.val = strdup("dictname");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->dictname, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("options");
	key.val.string.val = strdup("options");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->options, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *SortGroupClause_ser(const SortGroupClause *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("eqop");
	key.val.string.val = strdup("eqop");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, UInt32GetDatum(node->eqop)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("hashable");
	key.val.string.val = strdup("hashable");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvBool;
	val.val.boolean = node->hashable;
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("nulls_first");
	key.val.string.val = strdup("nulls_first");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvBool;
	val.val.boolean = node->nulls_first;
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("sortop");
	key.val.string.val = strdup("sortop");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, UInt32GetDatum(node->sortop)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("tleSortGroupRef");
	key.val.string.val = strdup("tleSortGroupRef");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, UInt32GetDatum(node->tleSortGroupRef)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *NamedArgExpr_ser(const NamedArgExpr *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


					
	key.type = jbvString;
	key.val.string.len = strlen("arg");
	key.val.string.val = strdup("arg");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->arg, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("argnumber");
	key.val.string.val = strdup("argnumber");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, Int32GetDatum(node->argnumber)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					if(!skip_location)
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("location");
	key.val.string.val = strdup("location");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, Int32GetDatum(node->location)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("name");
	key.val.string.val = strdup("name");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	if (node->name == NULL)
	{
		val.type = jbvNull;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}
	else
	{
		val.type = jbvString;
		val.val.string.len = strlen(node->name);
		val.val.string.val = (char *)node->name;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}

				}
				
	
	key.type = jbvString;
	key.val.string.len = strlen("xpr");
	key.val.string.val = strdup("xpr");
	pushJsonbValue(&state, WJB_KEY, &key);

	Expr_ser(&node->xpr, state, false);

		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *CaseWhen_ser(const CaseWhen *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


					
	key.type = jbvString;
	key.val.string.len = strlen("expr");
	key.val.string.val = strdup("expr");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->expr, state);

					if(!skip_location)
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("location");
	key.val.string.val = strdup("location");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, Int32GetDatum(node->location)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("result");
	key.val.string.val = strdup("result");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->result, state);

				
	
	key.type = jbvString;
	key.val.string.len = strlen("xpr");
	key.val.string.val = strdup("xpr");
	pushJsonbValue(&state, WJB_KEY, &key);

	Expr_ser(&node->xpr, state, false);

		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *CommentStmt_ser(const CommentStmt *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("comment");
	key.val.string.val = strdup("comment");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	if (node->comment == NULL)
	{
		val.type = jbvNull;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}
	else
	{
		val.type = jbvString;
		val.val.string.len = strlen(node->comment);
		val.val.string.val = (char *)node->comment;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("objargs");
	key.val.string.val = strdup("objargs");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->objargs, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("objname");
	key.val.string.val = strdup("objname");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->objname, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("objtype");
	key.val.string.val = strdup("objtype");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->objtype)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *VacuumStmt_ser(const VacuumStmt *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("options");
	key.val.string.val = strdup("options");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, Int32GetDatum(node->options)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("relation");
	key.val.string.val = strdup("relation");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->relation, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("va_cols");
	key.val.string.val = strdup("va_cols");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->va_cols, state);

		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *AlterOwnerStmt_ser(const AlterOwnerStmt *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


					
	key.type = jbvString;
	key.val.string.len = strlen("newowner");
	key.val.string.val = strdup("newowner");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->newowner, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("objarg");
	key.val.string.val = strdup("objarg");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->objarg, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("object");
	key.val.string.val = strdup("object");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->object, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("objectType");
	key.val.string.val = strdup("objectType");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->objectType)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("relation");
	key.val.string.val = strdup("relation");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->relation, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *Unique_ser(const Unique *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("numCols");
	key.val.string.val = strdup("numCols");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, Int32GetDatum(node->numCols)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				
	
	key.type = jbvString;
	key.val.string.len = strlen("plan");
	key.val.string.val = strdup("plan");
	pushJsonbValue(&state, WJB_KEY, &key);

	Plan_ser(&node->plan, state, false);

					{
						int i;
						JsonbValue val;
						
	
	key.type = jbvString;
	key.val.string.len = strlen("uniqColIdx");
	key.val.string.val = strdup("uniqColIdx");
	pushJsonbValue(&state, WJB_KEY, &key);

	pushJsonbValue(&state, WJB_BEGIN_ARRAY, NULL);
	for (i = 0; i < node->numCols; i++)
	{
			
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->uniqColIdx[i])));
	pushJsonbValue(&state, WJB_ELEM, &val);

	}
	pushJsonbValue(&state, WJB_END_ARRAY, NULL);

					}
				
					{
						int i;
						JsonbValue val;
						
	
	key.type = jbvString;
	key.val.string.len = strlen("uniqOperators");
	key.val.string.val = strdup("uniqOperators");
	pushJsonbValue(&state, WJB_KEY, &key);

	pushJsonbValue(&state, WJB_BEGIN_ARRAY, NULL);
	for (i = 0; i < node->numCols; i++)
	{
			
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, UInt32GetDatum(node->uniqOperators[i])));
	pushJsonbValue(&state, WJB_ELEM, &val);

	}
	pushJsonbValue(&state, WJB_END_ARRAY, NULL);

					}
				
		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *MinMaxExpr_ser(const MinMaxExpr *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


					
	key.type = jbvString;
	key.val.string.len = strlen("args");
	key.val.string.val = strdup("args");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->args, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("inputcollid");
	key.val.string.val = strdup("inputcollid");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, UInt32GetDatum(node->inputcollid)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					if(!skip_location)
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("location");
	key.val.string.val = strdup("location");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, Int32GetDatum(node->location)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("minmaxcollid");
	key.val.string.val = strdup("minmaxcollid");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, UInt32GetDatum(node->minmaxcollid)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("minmaxtype");
	key.val.string.val = strdup("minmaxtype");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, UInt32GetDatum(node->minmaxtype)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("op");
	key.val.string.val = strdup("op");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->op)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				
	
	key.type = jbvString;
	key.val.string.len = strlen("xpr");
	key.val.string.val = strdup("xpr");
	pushJsonbValue(&state, WJB_KEY, &key);

	Expr_ser(&node->xpr, state, false);

		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *TransactionStmt_ser(const TransactionStmt *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("gid");
	key.val.string.val = strdup("gid");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	if (node->gid == NULL)
	{
		val.type = jbvNull;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}
	else
	{
		val.type = jbvString;
		val.val.string.len = strlen(node->gid);
		val.val.string.val = (char *)node->gid;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("kind");
	key.val.string.val = strdup("kind");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->kind)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("options");
	key.val.string.val = strdup("options");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->options, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *CreateEventTrigStmt_ser(const CreateEventTrigStmt *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("eventname");
	key.val.string.val = strdup("eventname");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	if (node->eventname == NULL)
	{
		val.type = jbvNull;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}
	else
	{
		val.type = jbvString;
		val.val.string.len = strlen(node->eventname);
		val.val.string.val = (char *)node->eventname;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("funcname");
	key.val.string.val = strdup("funcname");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->funcname, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("trigname");
	key.val.string.val = strdup("trigname");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	if (node->trigname == NULL)
	{
		val.type = jbvNull;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}
	else
	{
		val.type = jbvString;
		val.val.string.len = strlen(node->trigname);
		val.val.string.val = (char *)node->trigname;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("whenclause");
	key.val.string.val = strdup("whenclause");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->whenclause, state);

		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *ArrayRef_ser(const ArrayRef *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("refarraytype");
	key.val.string.val = strdup("refarraytype");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, UInt32GetDatum(node->refarraytype)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("refassgnexpr");
	key.val.string.val = strdup("refassgnexpr");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->refassgnexpr, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("refcollid");
	key.val.string.val = strdup("refcollid");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, UInt32GetDatum(node->refcollid)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("refelemtype");
	key.val.string.val = strdup("refelemtype");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, UInt32GetDatum(node->refelemtype)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("refexpr");
	key.val.string.val = strdup("refexpr");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->refexpr, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("reflowerindexpr");
	key.val.string.val = strdup("reflowerindexpr");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->reflowerindexpr, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("reftypmod");
	key.val.string.val = strdup("reftypmod");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, Int32GetDatum(node->reftypmod)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("refupperindexpr");
	key.val.string.val = strdup("refupperindexpr");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->refupperindexpr, state);

				
	
	key.type = jbvString;
	key.val.string.len = strlen("xpr");
	key.val.string.val = strdup("xpr");
	pushJsonbValue(&state, WJB_KEY, &key);

	Expr_ser(&node->xpr, state, false);

		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *AlterExtensionStmt_ser(const AlterExtensionStmt *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("extname");
	key.val.string.val = strdup("extname");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	if (node->extname == NULL)
	{
		val.type = jbvNull;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}
	else
	{
		val.type = jbvString;
		val.val.string.len = strlen(node->extname);
		val.val.string.val = (char *)node->extname;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("options");
	key.val.string.val = strdup("options");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->options, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *CreateRoleStmt_ser(const CreateRoleStmt *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


					
	key.type = jbvString;
	key.val.string.len = strlen("options");
	key.val.string.val = strdup("options");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->options, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("role");
	key.val.string.val = strdup("role");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	if (node->role == NULL)
	{
		val.type = jbvNull;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}
	else
	{
		val.type = jbvString;
		val.val.string.len = strlen(node->role);
		val.val.string.val = (char *)node->role;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("stmt_type");
	key.val.string.val = strdup("stmt_type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->stmt_type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *AlterOpFamilyStmt_ser(const AlterOpFamilyStmt *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("amname");
	key.val.string.val = strdup("amname");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	if (node->amname == NULL)
	{
		val.type = jbvNull;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}
	else
	{
		val.type = jbvString;
		val.val.string.len = strlen(node->amname);
		val.val.string.val = (char *)node->amname;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("isDrop");
	key.val.string.val = strdup("isDrop");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvBool;
	val.val.boolean = node->isDrop;
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("items");
	key.val.string.val = strdup("items");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->items, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("opfamilyname");
	key.val.string.val = strdup("opfamilyname");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->opfamilyname, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *LockStmt_ser(const LockStmt *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("mode");
	key.val.string.val = strdup("mode");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, Int32GetDatum(node->mode)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("nowait");
	key.val.string.val = strdup("nowait");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvBool;
	val.val.boolean = node->nowait;
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("relations");
	key.val.string.val = strdup("relations");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->relations, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *AlterTableStmt_ser(const AlterTableStmt *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


					
	key.type = jbvString;
	key.val.string.len = strlen("cmds");
	key.val.string.val = strdup("cmds");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->cmds, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("missing_ok");
	key.val.string.val = strdup("missing_ok");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvBool;
	val.val.boolean = node->missing_ok;
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("relation");
	key.val.string.val = strdup("relation");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->relation, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("relkind");
	key.val.string.val = strdup("relkind");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->relkind)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *CreateSchemaStmt_ser(const CreateSchemaStmt *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


					
	key.type = jbvString;
	key.val.string.len = strlen("authrole");
	key.val.string.val = strdup("authrole");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->authrole, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("if_not_exists");
	key.val.string.val = strdup("if_not_exists");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvBool;
	val.val.boolean = node->if_not_exists;
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("schemaElts");
	key.val.string.val = strdup("schemaElts");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->schemaElts, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("schemaname");
	key.val.string.val = strdup("schemaname");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	if (node->schemaname == NULL)
	{
		val.type = jbvNull;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}
	else
	{
		val.type = jbvString;
		val.val.string.len = strlen(node->schemaname);
		val.val.string.val = (char *)node->schemaname;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *ClosePortalStmt_ser(const ClosePortalStmt *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("portalname");
	key.val.string.val = strdup("portalname");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	if (node->portalname == NULL)
	{
		val.type = jbvNull;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}
	else
	{
		val.type = jbvString;
		val.val.string.len = strlen(node->portalname);
		val.val.string.val = (char *)node->portalname;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *RelabelType_ser(const RelabelType *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


					
	key.type = jbvString;
	key.val.string.len = strlen("arg");
	key.val.string.val = strdup("arg");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->arg, state);

					if(!skip_location)
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("location");
	key.val.string.val = strdup("location");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, Int32GetDatum(node->location)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("relabelformat");
	key.val.string.val = strdup("relabelformat");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->relabelformat)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("resultcollid");
	key.val.string.val = strdup("resultcollid");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, UInt32GetDatum(node->resultcollid)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("resulttype");
	key.val.string.val = strdup("resulttype");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, UInt32GetDatum(node->resulttype)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("resulttypmod");
	key.val.string.val = strdup("resulttypmod");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, Int32GetDatum(node->resulttypmod)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				
	
	key.type = jbvString;
	key.val.string.len = strlen("xpr");
	key.val.string.val = strdup("xpr");
	pushJsonbValue(&state, WJB_KEY, &key);

	Expr_ser(&node->xpr, state, false);

		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *FunctionScan_ser(const FunctionScan *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("funcordinality");
	key.val.string.val = strdup("funcordinality");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvBool;
	val.val.boolean = node->funcordinality;
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("functions");
	key.val.string.val = strdup("functions");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->functions, state);

				
	
	key.type = jbvString;
	key.val.string.len = strlen("scan");
	key.val.string.val = strdup("scan");
	pushJsonbValue(&state, WJB_KEY, &key);

	Scan_ser(&node->scan, state, false);

		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *AlterSeqStmt_ser(const AlterSeqStmt *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("missing_ok");
	key.val.string.val = strdup("missing_ok");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvBool;
	val.val.boolean = node->missing_ok;
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("options");
	key.val.string.val = strdup("options");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->options, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("sequence");
	key.val.string.val = strdup("sequence");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->sequence, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *SubPlan_ser(const SubPlan *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


					
	key.type = jbvString;
	key.val.string.len = strlen("args");
	key.val.string.val = strdup("args");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->args, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("firstColCollation");
	key.val.string.val = strdup("firstColCollation");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, UInt32GetDatum(node->firstColCollation)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("firstColType");
	key.val.string.val = strdup("firstColType");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, UInt32GetDatum(node->firstColType)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("firstColTypmod");
	key.val.string.val = strdup("firstColTypmod");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, Int32GetDatum(node->firstColTypmod)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("parParam");
	key.val.string.val = strdup("parParam");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->parParam, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("paramIds");
	key.val.string.val = strdup("paramIds");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->paramIds, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("per_call_cost");
	key.val.string.val = strdup("per_call_cost");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(float8_numeric, Float8GetDatum(node->per_call_cost)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("plan_id");
	key.val.string.val = strdup("plan_id");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, Int32GetDatum(node->plan_id)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("plan_name");
	key.val.string.val = strdup("plan_name");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	if (node->plan_name == NULL)
	{
		val.type = jbvNull;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}
	else
	{
		val.type = jbvString;
		val.val.string.len = strlen(node->plan_name);
		val.val.string.val = (char *)node->plan_name;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("setParam");
	key.val.string.val = strdup("setParam");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->setParam, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("startup_cost");
	key.val.string.val = strdup("startup_cost");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(float8_numeric, Float8GetDatum(node->startup_cost)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("subLinkType");
	key.val.string.val = strdup("subLinkType");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->subLinkType)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("testexpr");
	key.val.string.val = strdup("testexpr");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->testexpr, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("unknownEqFalse");
	key.val.string.val = strdup("unknownEqFalse");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvBool;
	val.val.boolean = node->unknownEqFalse;
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("useHashTable");
	key.val.string.val = strdup("useHashTable");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvBool;
	val.val.boolean = node->useHashTable;
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				
	
	key.type = jbvString;
	key.val.string.len = strlen("xpr");
	key.val.string.val = strdup("xpr");
	pushJsonbValue(&state, WJB_KEY, &key);

	Expr_ser(&node->xpr, state, false);

		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *ScalarArrayOpExpr_ser(const ScalarArrayOpExpr *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


					
	key.type = jbvString;
	key.val.string.len = strlen("args");
	key.val.string.val = strdup("args");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->args, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("inputcollid");
	key.val.string.val = strdup("inputcollid");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, UInt32GetDatum(node->inputcollid)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					if(!skip_location)
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("location");
	key.val.string.val = strdup("location");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, Int32GetDatum(node->location)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("opfuncid");
	key.val.string.val = strdup("opfuncid");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, UInt32GetDatum(node->opfuncid)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("opno");
	key.val.string.val = strdup("opno");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, UInt32GetDatum(node->opno)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("useOr");
	key.val.string.val = strdup("useOr");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvBool;
	val.val.boolean = node->useOr;
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				
	
	key.type = jbvString;
	key.val.string.len = strlen("xpr");
	key.val.string.val = strdup("xpr");
	pushJsonbValue(&state, WJB_KEY, &key);

	Expr_ser(&node->xpr, state, false);

		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *RoleSpec_ser(const RoleSpec *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


					if(!skip_location)
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("location");
	key.val.string.val = strdup("location");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, Int32GetDatum(node->location)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("rolename");
	key.val.string.val = strdup("rolename");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	if (node->rolename == NULL)
	{
		val.type = jbvNull;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}
	else
	{
		val.type = jbvString;
		val.val.string.len = strlen(node->rolename);
		val.val.string.val = (char *)node->rolename;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("roletype");
	key.val.string.val = strdup("roletype");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->roletype)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *InlineCodeBlock_ser(const InlineCodeBlock *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("langIsTrusted");
	key.val.string.val = strdup("langIsTrusted");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvBool;
	val.val.boolean = node->langIsTrusted;
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("langOid");
	key.val.string.val = strdup("langOid");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, UInt32GetDatum(node->langOid)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("source_text");
	key.val.string.val = strdup("source_text");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	if (node->source_text == NULL)
	{
		val.type = jbvNull;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}
	else
	{
		val.type = jbvString;
		val.val.string.len = strlen(node->source_text);
		val.val.string.val = (char *)node->source_text;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *VariableShowStmt_ser(const VariableShowStmt *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("name");
	key.val.string.val = strdup("name");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	if (node->name == NULL)
	{
		val.type = jbvNull;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}
	else
	{
		val.type = jbvString;
		val.val.string.len = strlen(node->name);
		val.val.string.val = (char *)node->name;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *ImportForeignSchemaStmt_ser(const ImportForeignSchemaStmt *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("list_type");
	key.val.string.val = strdup("list_type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->list_type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("local_schema");
	key.val.string.val = strdup("local_schema");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	if (node->local_schema == NULL)
	{
		val.type = jbvNull;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}
	else
	{
		val.type = jbvString;
		val.val.string.len = strlen(node->local_schema);
		val.val.string.val = (char *)node->local_schema;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("options");
	key.val.string.val = strdup("options");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->options, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("remote_schema");
	key.val.string.val = strdup("remote_schema");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	if (node->remote_schema == NULL)
	{
		val.type = jbvNull;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}
	else
	{
		val.type = jbvString;
		val.val.string.len = strlen(node->remote_schema);
		val.val.string.val = (char *)node->remote_schema;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("server_name");
	key.val.string.val = strdup("server_name");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	if (node->server_name == NULL)
	{
		val.type = jbvNull;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}
	else
	{
		val.type = jbvString;
		val.val.string.len = strlen(node->server_name);
		val.val.string.val = (char *)node->server_name;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("table_list");
	key.val.string.val = strdup("table_list");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->table_list, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *AlterForeignServerStmt_ser(const AlterForeignServerStmt *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("has_version");
	key.val.string.val = strdup("has_version");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvBool;
	val.val.boolean = node->has_version;
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("options");
	key.val.string.val = strdup("options");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->options, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("servername");
	key.val.string.val = strdup("servername");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	if (node->servername == NULL)
	{
		val.type = jbvNull;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}
	else
	{
		val.type = jbvString;
		val.val.string.len = strlen(node->servername);
		val.val.string.val = (char *)node->servername;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("version");
	key.val.string.val = strdup("version");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	if (node->version == NULL)
	{
		val.type = jbvNull;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}
	else
	{
		val.type = jbvString;
		val.val.string.len = strlen(node->version);
		val.val.string.val = (char *)node->version;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}

				}
		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *ModifyTable_ser(const ModifyTable *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


					
	key.type = jbvString;
	key.val.string.len = strlen("arbiterIndexes");
	key.val.string.val = strdup("arbiterIndexes");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->arbiterIndexes, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("canSetTag");
	key.val.string.val = strdup("canSetTag");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvBool;
	val.val.boolean = node->canSetTag;
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("epqParam");
	key.val.string.val = strdup("epqParam");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, Int32GetDatum(node->epqParam)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("exclRelRTI");
	key.val.string.val = strdup("exclRelRTI");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, UInt32GetDatum(node->exclRelRTI)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("exclRelTlist");
	key.val.string.val = strdup("exclRelTlist");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->exclRelTlist, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("fdwPrivLists");
	key.val.string.val = strdup("fdwPrivLists");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->fdwPrivLists, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("nominalRelation");
	key.val.string.val = strdup("nominalRelation");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, UInt32GetDatum(node->nominalRelation)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("onConflictAction");
	key.val.string.val = strdup("onConflictAction");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->onConflictAction)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("onConflictSet");
	key.val.string.val = strdup("onConflictSet");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->onConflictSet, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("onConflictWhere");
	key.val.string.val = strdup("onConflictWhere");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->onConflictWhere, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("operation");
	key.val.string.val = strdup("operation");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->operation)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				
	
	key.type = jbvString;
	key.val.string.len = strlen("plan");
	key.val.string.val = strdup("plan");
	pushJsonbValue(&state, WJB_KEY, &key);

	Plan_ser(&node->plan, state, false);

					
	key.type = jbvString;
	key.val.string.len = strlen("plans");
	key.val.string.val = strdup("plans");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->plans, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("resultRelIndex");
	key.val.string.val = strdup("resultRelIndex");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, Int32GetDatum(node->resultRelIndex)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("resultRelations");
	key.val.string.val = strdup("resultRelations");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->resultRelations, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("returningLists");
	key.val.string.val = strdup("returningLists");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->returningLists, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("rowMarks");
	key.val.string.val = strdup("rowMarks");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->rowMarks, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("withCheckOptionLists");
	key.val.string.val = strdup("withCheckOptionLists");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->withCheckOptionLists, state);

		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *CheckPointStmt_ser(const CheckPointStmt *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *RangeVar_ser(const RangeVar *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


					
	key.type = jbvString;
	key.val.string.len = strlen("alias");
	key.val.string.val = strdup("alias");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->alias, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("catalogname");
	key.val.string.val = strdup("catalogname");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	if (node->catalogname == NULL)
	{
		val.type = jbvNull;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}
	else
	{
		val.type = jbvString;
		val.val.string.len = strlen(node->catalogname);
		val.val.string.val = (char *)node->catalogname;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("inhOpt");
	key.val.string.val = strdup("inhOpt");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->inhOpt)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					if(!skip_location)
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("location");
	key.val.string.val = strdup("location");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, Int32GetDatum(node->location)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("relname");
	key.val.string.val = strdup("relname");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	if (node->relname == NULL)
	{
		val.type = jbvNull;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}
	else
	{
		val.type = jbvString;
		val.val.string.len = strlen(node->relname);
		val.val.string.val = (char *)node->relname;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("relpersistence");
	key.val.string.val = strdup("relpersistence");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->relpersistence)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("schemaname");
	key.val.string.val = strdup("schemaname");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	if (node->schemaname == NULL)
	{
		val.type = jbvNull;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}
	else
	{
		val.type = jbvString;
		val.val.string.len = strlen(node->schemaname);
		val.val.string.val = (char *)node->schemaname;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *AlterExtensionContentsStmt_ser(const AlterExtensionContentsStmt *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("action");
	key.val.string.val = strdup("action");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, Int32GetDatum(node->action)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("extname");
	key.val.string.val = strdup("extname");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	if (node->extname == NULL)
	{
		val.type = jbvNull;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}
	else
	{
		val.type = jbvString;
		val.val.string.len = strlen(node->extname);
		val.val.string.val = (char *)node->extname;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("objargs");
	key.val.string.val = strdup("objargs");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->objargs, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("objname");
	key.val.string.val = strdup("objname");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->objname, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("objtype");
	key.val.string.val = strdup("objtype");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->objtype)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *RangeTblRef_ser(const RangeTblRef *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("rtindex");
	key.val.string.val = strdup("rtindex");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, Int32GetDatum(node->rtindex)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *CreateOpClassStmt_ser(const CreateOpClassStmt *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("amname");
	key.val.string.val = strdup("amname");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	if (node->amname == NULL)
	{
		val.type = jbvNull;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}
	else
	{
		val.type = jbvString;
		val.val.string.len = strlen(node->amname);
		val.val.string.val = (char *)node->amname;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("datatype");
	key.val.string.val = strdup("datatype");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->datatype, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("isDefault");
	key.val.string.val = strdup("isDefault");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvBool;
	val.val.boolean = node->isDefault;
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("items");
	key.val.string.val = strdup("items");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->items, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("opclassname");
	key.val.string.val = strdup("opclassname");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->opclassname, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("opfamilyname");
	key.val.string.val = strdup("opfamilyname");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->opfamilyname, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *LoadStmt_ser(const LoadStmt *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("filename");
	key.val.string.val = strdup("filename");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	if (node->filename == NULL)
	{
		val.type = jbvNull;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}
	else
	{
		val.type = jbvString;
		val.val.string.len = strlen(node->filename);
		val.val.string.val = (char *)node->filename;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *ColumnRef_ser(const ColumnRef *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


					
	key.type = jbvString;
	key.val.string.len = strlen("fields");
	key.val.string.val = strdup("fields");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->fields, state);

					if(!skip_location)
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("location");
	key.val.string.val = strdup("location");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, Int32GetDatum(node->location)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *CreateTrigStmt_ser(const CreateTrigStmt *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


					
	key.type = jbvString;
	key.val.string.len = strlen("args");
	key.val.string.val = strdup("args");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->args, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("columns");
	key.val.string.val = strdup("columns");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->columns, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("constrrel");
	key.val.string.val = strdup("constrrel");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->constrrel, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("deferrable");
	key.val.string.val = strdup("deferrable");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvBool;
	val.val.boolean = node->deferrable;
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("events");
	key.val.string.val = strdup("events");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->events)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("funcname");
	key.val.string.val = strdup("funcname");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->funcname, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("initdeferred");
	key.val.string.val = strdup("initdeferred");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvBool;
	val.val.boolean = node->initdeferred;
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("isconstraint");
	key.val.string.val = strdup("isconstraint");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvBool;
	val.val.boolean = node->isconstraint;
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("relation");
	key.val.string.val = strdup("relation");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->relation, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("row");
	key.val.string.val = strdup("row");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvBool;
	val.val.boolean = node->row;
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("timing");
	key.val.string.val = strdup("timing");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->timing)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("trigname");
	key.val.string.val = strdup("trigname");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	if (node->trigname == NULL)
	{
		val.type = jbvNull;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}
	else
	{
		val.type = jbvString;
		val.val.string.len = strlen(node->trigname);
		val.val.string.val = (char *)node->trigname;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("whenClause");
	key.val.string.val = strdup("whenClause");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->whenClause, state);

		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *ConstraintsSetStmt_ser(const ConstraintsSetStmt *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


					
	key.type = jbvString;
	key.val.string.len = strlen("constraints");
	key.val.string.val = strdup("constraints");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->constraints, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("deferred");
	key.val.string.val = strdup("deferred");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvBool;
	val.val.boolean = node->deferred;
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *Var_ser(const Var *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


					if(!skip_location)
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("location");
	key.val.string.val = strdup("location");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, Int32GetDatum(node->location)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("varattno");
	key.val.string.val = strdup("varattno");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->varattno)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("varcollid");
	key.val.string.val = strdup("varcollid");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, UInt32GetDatum(node->varcollid)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("varlevelsup");
	key.val.string.val = strdup("varlevelsup");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, UInt32GetDatum(node->varlevelsup)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("varno");
	key.val.string.val = strdup("varno");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, UInt32GetDatum(node->varno)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("varnoold");
	key.val.string.val = strdup("varnoold");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, UInt32GetDatum(node->varnoold)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("varoattno");
	key.val.string.val = strdup("varoattno");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->varoattno)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("vartype");
	key.val.string.val = strdup("vartype");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, UInt32GetDatum(node->vartype)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("vartypmod");
	key.val.string.val = strdup("vartypmod");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, Int32GetDatum(node->vartypmod)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				
	
	key.type = jbvString;
	key.val.string.len = strlen("xpr");
	key.val.string.val = strdup("xpr");
	pushJsonbValue(&state, WJB_KEY, &key);

	Expr_ser(&node->xpr, state, false);

		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *BitmapIndexScan_ser(const BitmapIndexScan *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("indexid");
	key.val.string.val = strdup("indexid");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, UInt32GetDatum(node->indexid)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("indexqual");
	key.val.string.val = strdup("indexqual");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->indexqual, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("indexqualorig");
	key.val.string.val = strdup("indexqualorig");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->indexqualorig, state);

				
	
	key.type = jbvString;
	key.val.string.len = strlen("scan");
	key.val.string.val = strdup("scan");
	pushJsonbValue(&state, WJB_KEY, &key);

	Scan_ser(&node->scan, state, false);

		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *Plan_ser(const Plan *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


				{
					JsonbValue val;
					
	
	key.type = jbvString;
	key.val.string.len = strlen("allParam");
	key.val.string.val = strdup("allParam");
	pushJsonbValue(&state, WJB_KEY, &key);

	pushJsonbValue(&state, WJB_KEY, &key);
	
	if (node->allParam == NULL)
	{
		val.type = jbvNull;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}
	else
	{
		int x = -1;
		pushJsonbValue(&state, WJB_BEGIN_ARRAY, NULL);
		while ((x = bms_next_member(node->allParam, x)) >= 0)
		{
			val.type = jbvNumeric;
			val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, Int32GetDatum(x)));
			pushJsonbValue(&state, WJB_ELEM, &val);
		}
		pushJsonbValue(&state, WJB_END_ARRAY, NULL);
	}

				}
				{
					JsonbValue val;
					
	
	key.type = jbvString;
	key.val.string.len = strlen("extParam");
	key.val.string.val = strdup("extParam");
	pushJsonbValue(&state, WJB_KEY, &key);

	pushJsonbValue(&state, WJB_KEY, &key);
	
	if (node->extParam == NULL)
	{
		val.type = jbvNull;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}
	else
	{
		int x = -1;
		pushJsonbValue(&state, WJB_BEGIN_ARRAY, NULL);
		while ((x = bms_next_member(node->extParam, x)) >= 0)
		{
			val.type = jbvNumeric;
			val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, Int32GetDatum(x)));
			pushJsonbValue(&state, WJB_ELEM, &val);
		}
		pushJsonbValue(&state, WJB_END_ARRAY, NULL);
	}

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("initPlan");
	key.val.string.val = strdup("initPlan");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->initPlan, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("lefttree");
	key.val.string.val = strdup("lefttree");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->lefttree, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("plan_rows");
	key.val.string.val = strdup("plan_rows");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(float8_numeric, Float8GetDatum(node->plan_rows)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("plan_width");
	key.val.string.val = strdup("plan_width");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, Int32GetDatum(node->plan_width)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("qual");
	key.val.string.val = strdup("qual");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->qual, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("righttree");
	key.val.string.val = strdup("righttree");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->righttree, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("startup_cost");
	key.val.string.val = strdup("startup_cost");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(float8_numeric, Float8GetDatum(node->startup_cost)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("targetlist");
	key.val.string.val = strdup("targetlist");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->targetlist, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("total_cost");
	key.val.string.val = strdup("total_cost");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(float8_numeric, Float8GetDatum(node->total_cost)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *CreateStmt_ser(const CreateStmt *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


					
	key.type = jbvString;
	key.val.string.len = strlen("constraints");
	key.val.string.val = strdup("constraints");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->constraints, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("if_not_exists");
	key.val.string.val = strdup("if_not_exists");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvBool;
	val.val.boolean = node->if_not_exists;
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("inhRelations");
	key.val.string.val = strdup("inhRelations");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->inhRelations, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("ofTypename");
	key.val.string.val = strdup("ofTypename");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->ofTypename, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("oncommit");
	key.val.string.val = strdup("oncommit");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->oncommit)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("options");
	key.val.string.val = strdup("options");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->options, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("relation");
	key.val.string.val = strdup("relation");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->relation, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("tableElts");
	key.val.string.val = strdup("tableElts");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->tableElts, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("tablespacename");
	key.val.string.val = strdup("tablespacename");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	if (node->tablespacename == NULL)
	{
		val.type = jbvNull;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}
	else
	{
		val.type = jbvString;
		val.val.string.len = strlen(node->tablespacename);
		val.val.string.val = (char *)node->tablespacename;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *InferClause_ser(const InferClause *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("conname");
	key.val.string.val = strdup("conname");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	if (node->conname == NULL)
	{
		val.type = jbvNull;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}
	else
	{
		val.type = jbvString;
		val.val.string.len = strlen(node->conname);
		val.val.string.val = (char *)node->conname;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("indexElems");
	key.val.string.val = strdup("indexElems");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->indexElems, state);

					if(!skip_location)
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("location");
	key.val.string.val = strdup("location");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, Int32GetDatum(node->location)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("whereClause");
	key.val.string.val = strdup("whereClause");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->whereClause, state);

		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *Param_ser(const Param *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


					if(!skip_location)
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("location");
	key.val.string.val = strdup("location");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, Int32GetDatum(node->location)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("paramcollid");
	key.val.string.val = strdup("paramcollid");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, UInt32GetDatum(node->paramcollid)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("paramid");
	key.val.string.val = strdup("paramid");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, Int32GetDatum(node->paramid)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("paramkind");
	key.val.string.val = strdup("paramkind");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->paramkind)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("paramtype");
	key.val.string.val = strdup("paramtype");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, UInt32GetDatum(node->paramtype)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("paramtypmod");
	key.val.string.val = strdup("paramtypmod");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, Int32GetDatum(node->paramtypmod)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				
	
	key.type = jbvString;
	key.val.string.len = strlen("xpr");
	key.val.string.val = strdup("xpr");
	pushJsonbValue(&state, WJB_KEY, &key);

	Expr_ser(&node->xpr, state, false);

		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *ExecuteStmt_ser(const ExecuteStmt *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("name");
	key.val.string.val = strdup("name");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	if (node->name == NULL)
	{
		val.type = jbvNull;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}
	else
	{
		val.type = jbvString;
		val.val.string.len = strlen(node->name);
		val.val.string.val = (char *)node->name;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("params");
	key.val.string.val = strdup("params");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->params, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *DropdbStmt_ser(const DropdbStmt *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("dbname");
	key.val.string.val = strdup("dbname");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	if (node->dbname == NULL)
	{
		val.type = jbvNull;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}
	else
	{
		val.type = jbvString;
		val.val.string.len = strlen(node->dbname);
		val.val.string.val = (char *)node->dbname;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("missing_ok");
	key.val.string.val = strdup("missing_ok");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvBool;
	val.val.boolean = node->missing_ok;
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *FetchStmt_ser(const FetchStmt *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("direction");
	key.val.string.val = strdup("direction");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->direction)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("howMany");
	key.val.string.val = strdup("howMany");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
#ifdef USE_FLOAT8_BYVAL
	val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int8_numeric, Int64GetDatum(node->howMany)));
#else
	val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, Int32GetDatum(node->howMany)));
#endif
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("ismove");
	key.val.string.val = strdup("ismove");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvBool;
	val.val.boolean = node->ismove;
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("portalname");
	key.val.string.val = strdup("portalname");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	if (node->portalname == NULL)
	{
		val.type = jbvNull;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}
	else
	{
		val.type = jbvString;
		val.val.string.len = strlen(node->portalname);
		val.val.string.val = (char *)node->portalname;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *HashJoin_ser(const HashJoin *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


					
	key.type = jbvString;
	key.val.string.len = strlen("hashclauses");
	key.val.string.val = strdup("hashclauses");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->hashclauses, state);

				
	
	key.type = jbvString;
	key.val.string.len = strlen("join");
	key.val.string.val = strdup("join");
	pushJsonbValue(&state, WJB_KEY, &key);

	Join_ser(&node->join, state, false);

		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *ColumnDef_ser(const ColumnDef *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


					
	key.type = jbvString;
	key.val.string.len = strlen("collClause");
	key.val.string.val = strdup("collClause");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->collClause, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("collOid");
	key.val.string.val = strdup("collOid");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, UInt32GetDatum(node->collOid)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("colname");
	key.val.string.val = strdup("colname");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	if (node->colname == NULL)
	{
		val.type = jbvNull;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}
	else
	{
		val.type = jbvString;
		val.val.string.len = strlen(node->colname);
		val.val.string.val = (char *)node->colname;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("constraints");
	key.val.string.val = strdup("constraints");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->constraints, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("cooked_default");
	key.val.string.val = strdup("cooked_default");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->cooked_default, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("fdwoptions");
	key.val.string.val = strdup("fdwoptions");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->fdwoptions, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("inhcount");
	key.val.string.val = strdup("inhcount");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, Int32GetDatum(node->inhcount)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("is_from_type");
	key.val.string.val = strdup("is_from_type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvBool;
	val.val.boolean = node->is_from_type;
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("is_local");
	key.val.string.val = strdup("is_local");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvBool;
	val.val.boolean = node->is_local;
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("is_not_null");
	key.val.string.val = strdup("is_not_null");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvBool;
	val.val.boolean = node->is_not_null;
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					if(!skip_location)
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("location");
	key.val.string.val = strdup("location");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, Int32GetDatum(node->location)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("raw_default");
	key.val.string.val = strdup("raw_default");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->raw_default, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("storage");
	key.val.string.val = strdup("storage");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->storage)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("typeName");
	key.val.string.val = strdup("typeName");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->typeName, state);

		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *AlterRoleSetStmt_ser(const AlterRoleSetStmt *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("database");
	key.val.string.val = strdup("database");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	if (node->database == NULL)
	{
		val.type = jbvNull;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}
	else
	{
		val.type = jbvString;
		val.val.string.len = strlen(node->database);
		val.val.string.val = (char *)node->database;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("role");
	key.val.string.val = strdup("role");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->role, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("setstmt");
	key.val.string.val = strdup("setstmt");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->setstmt, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *Result_ser(const Result *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


				
	
	key.type = jbvString;
	key.val.string.len = strlen("plan");
	key.val.string.val = strdup("plan");
	pushJsonbValue(&state, WJB_KEY, &key);

	Plan_ser(&node->plan, state, false);

					
	key.type = jbvString;
	key.val.string.len = strlen("resconstantqual");
	key.val.string.val = strdup("resconstantqual");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->resconstantqual, state);

		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *PrepareStmt_ser(const PrepareStmt *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


					
	key.type = jbvString;
	key.val.string.len = strlen("argtypes");
	key.val.string.val = strdup("argtypes");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->argtypes, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("name");
	key.val.string.val = strdup("name");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	if (node->name == NULL)
	{
		val.type = jbvNull;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}
	else
	{
		val.type = jbvString;
		val.val.string.len = strlen(node->name);
		val.val.string.val = (char *)node->name;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("query");
	key.val.string.val = strdup("query");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->query, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *AlterDomainStmt_ser(const AlterDomainStmt *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("behavior");
	key.val.string.val = strdup("behavior");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->behavior)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("def");
	key.val.string.val = strdup("def");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->def, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("missing_ok");
	key.val.string.val = strdup("missing_ok");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvBool;
	val.val.boolean = node->missing_ok;
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("name");
	key.val.string.val = strdup("name");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	if (node->name == NULL)
	{
		val.type = jbvNull;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}
	else
	{
		val.type = jbvString;
		val.val.string.len = strlen(node->name);
		val.val.string.val = (char *)node->name;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("subtype");
	key.val.string.val = strdup("subtype");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->subtype)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("typeName");
	key.val.string.val = strdup("typeName");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->typeName, state);

		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *CompositeTypeStmt_ser(const CompositeTypeStmt *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


					
	key.type = jbvString;
	key.val.string.len = strlen("coldeflist");
	key.val.string.val = strdup("coldeflist");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->coldeflist, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("typevar");
	key.val.string.val = strdup("typevar");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->typevar, state);

		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *CustomScan_ser(const CustomScan *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


					
	key.type = jbvString;
	key.val.string.len = strlen("custom_exprs");
	key.val.string.val = strdup("custom_exprs");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->custom_exprs, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("custom_plans");
	key.val.string.val = strdup("custom_plans");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->custom_plans, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("custom_private");
	key.val.string.val = strdup("custom_private");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->custom_private, state);

				{
					JsonbValue val;
					
	
	key.type = jbvString;
	key.val.string.len = strlen("custom_relids");
	key.val.string.val = strdup("custom_relids");
	pushJsonbValue(&state, WJB_KEY, &key);

	pushJsonbValue(&state, WJB_KEY, &key);
	
	if (node->custom_relids == NULL)
	{
		val.type = jbvNull;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}
	else
	{
		int x = -1;
		pushJsonbValue(&state, WJB_BEGIN_ARRAY, NULL);
		while ((x = bms_next_member(node->custom_relids, x)) >= 0)
		{
			val.type = jbvNumeric;
			val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, Int32GetDatum(x)));
			pushJsonbValue(&state, WJB_ELEM, &val);
		}
		pushJsonbValue(&state, WJB_END_ARRAY, NULL);
	}

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("custom_scan_tlist");
	key.val.string.val = strdup("custom_scan_tlist");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->custom_scan_tlist, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("flags");
	key.val.string.val = strdup("flags");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, UInt32GetDatum(node->flags)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				/* NOT FOUND TYPE: *CustomScanMethods */
				
	
	key.type = jbvString;
	key.val.string.len = strlen("scan");
	key.val.string.val = strdup("scan");
	pushJsonbValue(&state, WJB_KEY, &key);

	Scan_ser(&node->scan, state, false);

		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *Agg_ser(const Agg *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("aggstrategy");
	key.val.string.val = strdup("aggstrategy");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->aggstrategy)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("chain");
	key.val.string.val = strdup("chain");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->chain, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("groupingSets");
	key.val.string.val = strdup("groupingSets");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->groupingSets, state);

					{
						int i;
						JsonbValue val;
						
	
	key.type = jbvString;
	key.val.string.len = strlen("grpColIdx");
	key.val.string.val = strdup("grpColIdx");
	pushJsonbValue(&state, WJB_KEY, &key);

	pushJsonbValue(&state, WJB_BEGIN_ARRAY, NULL);
	for (i = 0; i < node->numCols; i++)
	{
			
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->grpColIdx[i])));
	pushJsonbValue(&state, WJB_ELEM, &val);

	}
	pushJsonbValue(&state, WJB_END_ARRAY, NULL);

					}
				
					{
						int i;
						JsonbValue val;
						
	
	key.type = jbvString;
	key.val.string.len = strlen("grpOperators");
	key.val.string.val = strdup("grpOperators");
	pushJsonbValue(&state, WJB_KEY, &key);

	pushJsonbValue(&state, WJB_BEGIN_ARRAY, NULL);
	for (i = 0; i < node->numCols; i++)
	{
			
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, UInt32GetDatum(node->grpOperators[i])));
	pushJsonbValue(&state, WJB_ELEM, &val);

	}
	pushJsonbValue(&state, WJB_END_ARRAY, NULL);

					}
				
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("numCols");
	key.val.string.val = strdup("numCols");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, Int32GetDatum(node->numCols)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("numGroups");
	key.val.string.val = strdup("numGroups");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
#ifdef USE_FLOAT8_BYVAL
	val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int8_numeric, Int64GetDatum(node->numGroups)));
#else
	val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, Int32GetDatum(node->numGroups)));
#endif
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				
	
	key.type = jbvString;
	key.val.string.len = strlen("plan");
	key.val.string.val = strdup("plan");
	pushJsonbValue(&state, WJB_KEY, &key);

	Plan_ser(&node->plan, state, false);

		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *AlterObjectSchemaStmt_ser(const AlterObjectSchemaStmt *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("missing_ok");
	key.val.string.val = strdup("missing_ok");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvBool;
	val.val.boolean = node->missing_ok;
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("newschema");
	key.val.string.val = strdup("newschema");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	if (node->newschema == NULL)
	{
		val.type = jbvNull;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}
	else
	{
		val.type = jbvString;
		val.val.string.len = strlen(node->newschema);
		val.val.string.val = (char *)node->newschema;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("objarg");
	key.val.string.val = strdup("objarg");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->objarg, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("object");
	key.val.string.val = strdup("object");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->object, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("objectType");
	key.val.string.val = strdup("objectType");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->objectType)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("relation");
	key.val.string.val = strdup("relation");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->relation, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *AlterEventTrigStmt_ser(const AlterEventTrigStmt *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("tgenabled");
	key.val.string.val = strdup("tgenabled");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->tgenabled)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("trigname");
	key.val.string.val = strdup("trigname");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	if (node->trigname == NULL)
	{
		val.type = jbvNull;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}
	else
	{
		val.type = jbvString;
		val.val.string.len = strlen(node->trigname);
		val.val.string.val = (char *)node->trigname;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *AlterDefaultPrivilegesStmt_ser(const AlterDefaultPrivilegesStmt *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


					
	key.type = jbvString;
	key.val.string.len = strlen("action");
	key.val.string.val = strdup("action");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->action, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("options");
	key.val.string.val = strdup("options");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->options, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *PlannedStmt_ser(const PlannedStmt *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("canSetTag");
	key.val.string.val = strdup("canSetTag");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvBool;
	val.val.boolean = node->canSetTag;
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("commandType");
	key.val.string.val = strdup("commandType");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->commandType)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("hasModifyingCTE");
	key.val.string.val = strdup("hasModifyingCTE");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvBool;
	val.val.boolean = node->hasModifyingCTE;
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("hasReturning");
	key.val.string.val = strdup("hasReturning");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvBool;
	val.val.boolean = node->hasReturning;
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("hasRowSecurity");
	key.val.string.val = strdup("hasRowSecurity");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvBool;
	val.val.boolean = node->hasRowSecurity;
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("invalItems");
	key.val.string.val = strdup("invalItems");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->invalItems, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("nParamExec");
	key.val.string.val = strdup("nParamExec");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, Int32GetDatum(node->nParamExec)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("planTree");
	key.val.string.val = strdup("planTree");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->planTree, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("queryId");
	key.val.string.val = strdup("queryId");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, UInt32GetDatum(node->queryId)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("relationOids");
	key.val.string.val = strdup("relationOids");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->relationOids, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("resultRelations");
	key.val.string.val = strdup("resultRelations");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->resultRelations, state);

				{
					JsonbValue val;
					
	
	key.type = jbvString;
	key.val.string.len = strlen("rewindPlanIDs");
	key.val.string.val = strdup("rewindPlanIDs");
	pushJsonbValue(&state, WJB_KEY, &key);

	pushJsonbValue(&state, WJB_KEY, &key);
	
	if (node->rewindPlanIDs == NULL)
	{
		val.type = jbvNull;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}
	else
	{
		int x = -1;
		pushJsonbValue(&state, WJB_BEGIN_ARRAY, NULL);
		while ((x = bms_next_member(node->rewindPlanIDs, x)) >= 0)
		{
			val.type = jbvNumeric;
			val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, Int32GetDatum(x)));
			pushJsonbValue(&state, WJB_ELEM, &val);
		}
		pushJsonbValue(&state, WJB_END_ARRAY, NULL);
	}

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("rowMarks");
	key.val.string.val = strdup("rowMarks");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->rowMarks, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("rtable");
	key.val.string.val = strdup("rtable");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->rtable, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("subplans");
	key.val.string.val = strdup("subplans");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->subplans, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("transientPlan");
	key.val.string.val = strdup("transientPlan");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvBool;
	val.val.boolean = node->transientPlan;
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("utilityStmt");
	key.val.string.val = strdup("utilityStmt");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->utilityStmt, state);

		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *Aggref_ser(const Aggref *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("aggcollid");
	key.val.string.val = strdup("aggcollid");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, UInt32GetDatum(node->aggcollid)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("aggdirectargs");
	key.val.string.val = strdup("aggdirectargs");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->aggdirectargs, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("aggdistinct");
	key.val.string.val = strdup("aggdistinct");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->aggdistinct, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("aggfilter");
	key.val.string.val = strdup("aggfilter");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->aggfilter, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("aggfnoid");
	key.val.string.val = strdup("aggfnoid");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, UInt32GetDatum(node->aggfnoid)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("aggkind");
	key.val.string.val = strdup("aggkind");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->aggkind)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("agglevelsup");
	key.val.string.val = strdup("agglevelsup");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, UInt32GetDatum(node->agglevelsup)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("aggorder");
	key.val.string.val = strdup("aggorder");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->aggorder, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("aggstar");
	key.val.string.val = strdup("aggstar");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvBool;
	val.val.boolean = node->aggstar;
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("aggtype");
	key.val.string.val = strdup("aggtype");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, UInt32GetDatum(node->aggtype)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("aggvariadic");
	key.val.string.val = strdup("aggvariadic");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvBool;
	val.val.boolean = node->aggvariadic;
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("args");
	key.val.string.val = strdup("args");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->args, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("inputcollid");
	key.val.string.val = strdup("inputcollid");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, UInt32GetDatum(node->inputcollid)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					if(!skip_location)
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("location");
	key.val.string.val = strdup("location");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, Int32GetDatum(node->location)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				
	
	key.type = jbvString;
	key.val.string.len = strlen("xpr");
	key.val.string.val = strdup("xpr");
	pushJsonbValue(&state, WJB_KEY, &key);

	Expr_ser(&node->xpr, state, false);

		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *SubqueryScan_ser(const SubqueryScan *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


				
	
	key.type = jbvString;
	key.val.string.len = strlen("scan");
	key.val.string.val = strdup("scan");
	pushJsonbValue(&state, WJB_KEY, &key);

	Scan_ser(&node->scan, state, false);

					
	key.type = jbvString;
	key.val.string.len = strlen("subplan");
	key.val.string.val = strdup("subplan");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->subplan, state);

		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *CreateFunctionStmt_ser(const CreateFunctionStmt *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


					
	key.type = jbvString;
	key.val.string.len = strlen("funcname");
	key.val.string.val = strdup("funcname");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->funcname, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("options");
	key.val.string.val = strdup("options");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->options, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("parameters");
	key.val.string.val = strdup("parameters");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->parameters, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("replace");
	key.val.string.val = strdup("replace");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvBool;
	val.val.boolean = node->replace;
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("returnType");
	key.val.string.val = strdup("returnType");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->returnType, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("withClause");
	key.val.string.val = strdup("withClause");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->withClause, state);

		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *CreateCastStmt_ser(const CreateCastStmt *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("context");
	key.val.string.val = strdup("context");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->context)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("func");
	key.val.string.val = strdup("func");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->func, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("inout");
	key.val.string.val = strdup("inout");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvBool;
	val.val.boolean = node->inout;
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("sourcetype");
	key.val.string.val = strdup("sourcetype");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->sourcetype, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("targettype");
	key.val.string.val = strdup("targettype");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->targettype, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *CteScan_ser(const CteScan *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("cteParam");
	key.val.string.val = strdup("cteParam");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, Int32GetDatum(node->cteParam)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("ctePlanId");
	key.val.string.val = strdup("ctePlanId");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, Int32GetDatum(node->ctePlanId)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				
	
	key.type = jbvString;
	key.val.string.len = strlen("scan");
	key.val.string.val = strdup("scan");
	pushJsonbValue(&state, WJB_KEY, &key);

	Scan_ser(&node->scan, state, false);

		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *IndexElem_ser(const IndexElem *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


					
	key.type = jbvString;
	key.val.string.len = strlen("collation");
	key.val.string.val = strdup("collation");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->collation, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("expr");
	key.val.string.val = strdup("expr");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->expr, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("indexcolname");
	key.val.string.val = strdup("indexcolname");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	if (node->indexcolname == NULL)
	{
		val.type = jbvNull;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}
	else
	{
		val.type = jbvString;
		val.val.string.len = strlen(node->indexcolname);
		val.val.string.val = (char *)node->indexcolname;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("name");
	key.val.string.val = strdup("name");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	if (node->name == NULL)
	{
		val.type = jbvNull;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}
	else
	{
		val.type = jbvString;
		val.val.string.len = strlen(node->name);
		val.val.string.val = (char *)node->name;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("nulls_ordering");
	key.val.string.val = strdup("nulls_ordering");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->nulls_ordering)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("opclass");
	key.val.string.val = strdup("opclass");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->opclass, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("ordering");
	key.val.string.val = strdup("ordering");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->ordering)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *CreateFdwStmt_ser(const CreateFdwStmt *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("fdwname");
	key.val.string.val = strdup("fdwname");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	if (node->fdwname == NULL)
	{
		val.type = jbvNull;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}
	else
	{
		val.type = jbvString;
		val.val.string.len = strlen(node->fdwname);
		val.val.string.val = (char *)node->fdwname;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("func_options");
	key.val.string.val = strdup("func_options");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->func_options, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("options");
	key.val.string.val = strdup("options");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->options, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *NestLoop_ser(const NestLoop *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


				
	
	key.type = jbvString;
	key.val.string.len = strlen("join");
	key.val.string.val = strdup("join");
	pushJsonbValue(&state, WJB_KEY, &key);

	Join_ser(&node->join, state, false);

					
	key.type = jbvString;
	key.val.string.len = strlen("nestParams");
	key.val.string.val = strdup("nestParams");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->nestParams, state);

		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *TypeCast_ser(const TypeCast *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


					
	key.type = jbvString;
	key.val.string.len = strlen("arg");
	key.val.string.val = strdup("arg");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->arg, state);

					if(!skip_location)
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("location");
	key.val.string.val = strdup("location");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, Int32GetDatum(node->location)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("typeName");
	key.val.string.val = strdup("typeName");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->typeName, state);

		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *CoerceToDomainValue_ser(const CoerceToDomainValue *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("collation");
	key.val.string.val = strdup("collation");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, UInt32GetDatum(node->collation)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					if(!skip_location)
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("location");
	key.val.string.val = strdup("location");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, Int32GetDatum(node->location)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("typeId");
	key.val.string.val = strdup("typeId");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, UInt32GetDatum(node->typeId)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("typeMod");
	key.val.string.val = strdup("typeMod");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, Int32GetDatum(node->typeMod)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				
	
	key.type = jbvString;
	key.val.string.len = strlen("xpr");
	key.val.string.val = strdup("xpr");
	pushJsonbValue(&state, WJB_KEY, &key);

	Expr_ser(&node->xpr, state, false);

		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *InsertStmt_ser(const InsertStmt *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


					
	key.type = jbvString;
	key.val.string.len = strlen("cols");
	key.val.string.val = strdup("cols");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->cols, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("onConflictClause");
	key.val.string.val = strdup("onConflictClause");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->onConflictClause, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("relation");
	key.val.string.val = strdup("relation");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->relation, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("returningList");
	key.val.string.val = strdup("returningList");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->returningList, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("selectStmt");
	key.val.string.val = strdup("selectStmt");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->selectStmt, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("withClause");
	key.val.string.val = strdup("withClause");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->withClause, state);

		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *SortBy_ser(const SortBy *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


					if(!skip_location)
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("location");
	key.val.string.val = strdup("location");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, Int32GetDatum(node->location)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("node");
	key.val.string.val = strdup("node");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->node, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("sortby_dir");
	key.val.string.val = strdup("sortby_dir");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->sortby_dir)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("sortby_nulls");
	key.val.string.val = strdup("sortby_nulls");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->sortby_nulls)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("useOp");
	key.val.string.val = strdup("useOp");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->useOp, state);

		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *ReassignOwnedStmt_ser(const ReassignOwnedStmt *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


					
	key.type = jbvString;
	key.val.string.len = strlen("newrole");
	key.val.string.val = strdup("newrole");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->newrole, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("roles");
	key.val.string.val = strdup("roles");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->roles, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *TypeName_ser(const TypeName *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


					
	key.type = jbvString;
	key.val.string.len = strlen("arrayBounds");
	key.val.string.val = strdup("arrayBounds");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->arrayBounds, state);

					if(!skip_location)
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("location");
	key.val.string.val = strdup("location");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, Int32GetDatum(node->location)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("names");
	key.val.string.val = strdup("names");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->names, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("pct_type");
	key.val.string.val = strdup("pct_type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvBool;
	val.val.boolean = node->pct_type;
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("setof");
	key.val.string.val = strdup("setof");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvBool;
	val.val.boolean = node->setof;
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("typeOid");
	key.val.string.val = strdup("typeOid");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, UInt32GetDatum(node->typeOid)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("typemod");
	key.val.string.val = strdup("typemod");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, Int32GetDatum(node->typemod)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("typmods");
	key.val.string.val = strdup("typmods");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->typmods, state);

		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *Constraint_ser(const Constraint *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("access_method");
	key.val.string.val = strdup("access_method");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	if (node->access_method == NULL)
	{
		val.type = jbvNull;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}
	else
	{
		val.type = jbvString;
		val.val.string.len = strlen(node->access_method);
		val.val.string.val = (char *)node->access_method;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("conname");
	key.val.string.val = strdup("conname");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	if (node->conname == NULL)
	{
		val.type = jbvNull;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}
	else
	{
		val.type = jbvString;
		val.val.string.len = strlen(node->conname);
		val.val.string.val = (char *)node->conname;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("contype");
	key.val.string.val = strdup("contype");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->contype)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("cooked_expr");
	key.val.string.val = strdup("cooked_expr");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	if (node->cooked_expr == NULL)
	{
		val.type = jbvNull;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}
	else
	{
		val.type = jbvString;
		val.val.string.len = strlen(node->cooked_expr);
		val.val.string.val = (char *)node->cooked_expr;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("deferrable");
	key.val.string.val = strdup("deferrable");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvBool;
	val.val.boolean = node->deferrable;
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("exclusions");
	key.val.string.val = strdup("exclusions");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->exclusions, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("fk_attrs");
	key.val.string.val = strdup("fk_attrs");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->fk_attrs, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("fk_del_action");
	key.val.string.val = strdup("fk_del_action");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->fk_del_action)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("fk_matchtype");
	key.val.string.val = strdup("fk_matchtype");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->fk_matchtype)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("fk_upd_action");
	key.val.string.val = strdup("fk_upd_action");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->fk_upd_action)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("indexname");
	key.val.string.val = strdup("indexname");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	if (node->indexname == NULL)
	{
		val.type = jbvNull;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}
	else
	{
		val.type = jbvString;
		val.val.string.len = strlen(node->indexname);
		val.val.string.val = (char *)node->indexname;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("indexspace");
	key.val.string.val = strdup("indexspace");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	if (node->indexspace == NULL)
	{
		val.type = jbvNull;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}
	else
	{
		val.type = jbvString;
		val.val.string.len = strlen(node->indexspace);
		val.val.string.val = (char *)node->indexspace;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("initdeferred");
	key.val.string.val = strdup("initdeferred");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvBool;
	val.val.boolean = node->initdeferred;
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("initially_valid");
	key.val.string.val = strdup("initially_valid");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvBool;
	val.val.boolean = node->initially_valid;
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("is_no_inherit");
	key.val.string.val = strdup("is_no_inherit");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvBool;
	val.val.boolean = node->is_no_inherit;
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("keys");
	key.val.string.val = strdup("keys");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->keys, state);

					if(!skip_location)
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("location");
	key.val.string.val = strdup("location");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, Int32GetDatum(node->location)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("old_conpfeqop");
	key.val.string.val = strdup("old_conpfeqop");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->old_conpfeqop, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("old_pktable_oid");
	key.val.string.val = strdup("old_pktable_oid");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, UInt32GetDatum(node->old_pktable_oid)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("options");
	key.val.string.val = strdup("options");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->options, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("pk_attrs");
	key.val.string.val = strdup("pk_attrs");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->pk_attrs, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("pktable");
	key.val.string.val = strdup("pktable");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->pktable, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("raw_expr");
	key.val.string.val = strdup("raw_expr");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->raw_expr, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("skip_validation");
	key.val.string.val = strdup("skip_validation");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvBool;
	val.val.boolean = node->skip_validation;
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("where_clause");
	key.val.string.val = strdup("where_clause");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->where_clause, state);

		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *OnConflictClause_ser(const OnConflictClause *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("action");
	key.val.string.val = strdup("action");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->action)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("infer");
	key.val.string.val = strdup("infer");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->infer, state);

					if(!skip_location)
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("location");
	key.val.string.val = strdup("location");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, Int32GetDatum(node->location)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("targetList");
	key.val.string.val = strdup("targetList");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->targetList, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("whereClause");
	key.val.string.val = strdup("whereClause");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->whereClause, state);

		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *AlterPolicyStmt_ser(const AlterPolicyStmt *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("policy_name");
	key.val.string.val = strdup("policy_name");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	if (node->policy_name == NULL)
	{
		val.type = jbvNull;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}
	else
	{
		val.type = jbvString;
		val.val.string.len = strlen(node->policy_name);
		val.val.string.val = (char *)node->policy_name;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("qual");
	key.val.string.val = strdup("qual");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->qual, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("roles");
	key.val.string.val = strdup("roles");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->roles, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("table");
	key.val.string.val = strdup("table");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->table, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("with_check");
	key.val.string.val = strdup("with_check");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->with_check, state);

		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *GroupingFunc_ser(const GroupingFunc *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("agglevelsup");
	key.val.string.val = strdup("agglevelsup");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, UInt32GetDatum(node->agglevelsup)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("args");
	key.val.string.val = strdup("args");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->args, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("cols");
	key.val.string.val = strdup("cols");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->cols, state);

					if(!skip_location)
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("location");
	key.val.string.val = strdup("location");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, Int32GetDatum(node->location)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("refs");
	key.val.string.val = strdup("refs");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->refs, state);

				
	
	key.type = jbvString;
	key.val.string.len = strlen("xpr");
	key.val.string.val = strdup("xpr");
	pushJsonbValue(&state, WJB_KEY, &key);

	Expr_ser(&node->xpr, state, false);

		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *SelectStmt_ser(const SelectStmt *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("all");
	key.val.string.val = strdup("all");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvBool;
	val.val.boolean = node->all;
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("distinctClause");
	key.val.string.val = strdup("distinctClause");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->distinctClause, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("fromClause");
	key.val.string.val = strdup("fromClause");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->fromClause, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("groupClause");
	key.val.string.val = strdup("groupClause");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->groupClause, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("havingClause");
	key.val.string.val = strdup("havingClause");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->havingClause, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("intoClause");
	key.val.string.val = strdup("intoClause");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->intoClause, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("larg");
	key.val.string.val = strdup("larg");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->larg, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("limitCount");
	key.val.string.val = strdup("limitCount");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->limitCount, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("limitOffset");
	key.val.string.val = strdup("limitOffset");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->limitOffset, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("lockingClause");
	key.val.string.val = strdup("lockingClause");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->lockingClause, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("op");
	key.val.string.val = strdup("op");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->op)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("rarg");
	key.val.string.val = strdup("rarg");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->rarg, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("sortClause");
	key.val.string.val = strdup("sortClause");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->sortClause, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("targetList");
	key.val.string.val = strdup("targetList");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->targetList, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("valuesLists");
	key.val.string.val = strdup("valuesLists");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->valuesLists, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("whereClause");
	key.val.string.val = strdup("whereClause");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->whereClause, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("windowClause");
	key.val.string.val = strdup("windowClause");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->windowClause, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("withClause");
	key.val.string.val = strdup("withClause");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->withClause, state);

		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *CopyStmt_ser(const CopyStmt *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


					
	key.type = jbvString;
	key.val.string.len = strlen("attlist");
	key.val.string.val = strdup("attlist");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->attlist, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("filename");
	key.val.string.val = strdup("filename");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	if (node->filename == NULL)
	{
		val.type = jbvNull;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}
	else
	{
		val.type = jbvString;
		val.val.string.len = strlen(node->filename);
		val.val.string.val = (char *)node->filename;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("is_from");
	key.val.string.val = strdup("is_from");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvBool;
	val.val.boolean = node->is_from;
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("is_program");
	key.val.string.val = strdup("is_program");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvBool;
	val.val.boolean = node->is_program;
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("options");
	key.val.string.val = strdup("options");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->options, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("query");
	key.val.string.val = strdup("query");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->query, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("relation");
	key.val.string.val = strdup("relation");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->relation, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *ArrayExpr_ser(const ArrayExpr *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("array_collid");
	key.val.string.val = strdup("array_collid");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, UInt32GetDatum(node->array_collid)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("array_typeid");
	key.val.string.val = strdup("array_typeid");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, UInt32GetDatum(node->array_typeid)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("element_typeid");
	key.val.string.val = strdup("element_typeid");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, UInt32GetDatum(node->element_typeid)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("elements");
	key.val.string.val = strdup("elements");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->elements, state);

					if(!skip_location)
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("location");
	key.val.string.val = strdup("location");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, Int32GetDatum(node->location)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("multidims");
	key.val.string.val = strdup("multidims");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvBool;
	val.val.boolean = node->multidims;
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				
	
	key.type = jbvString;
	key.val.string.len = strlen("xpr");
	key.val.string.val = strdup("xpr");
	pushJsonbValue(&state, WJB_KEY, &key);

	Expr_ser(&node->xpr, state, false);

		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *InferenceElem_ser(const InferenceElem *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


					
	key.type = jbvString;
	key.val.string.len = strlen("expr");
	key.val.string.val = strdup("expr");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->expr, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("infercollid");
	key.val.string.val = strdup("infercollid");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, UInt32GetDatum(node->infercollid)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("inferopclass");
	key.val.string.val = strdup("inferopclass");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, UInt32GetDatum(node->inferopclass)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				
	
	key.type = jbvString;
	key.val.string.len = strlen("xpr");
	key.val.string.val = strdup("xpr");
	pushJsonbValue(&state, WJB_KEY, &key);

	Expr_ser(&node->xpr, state, false);

		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *BitmapOr_ser(const BitmapOr *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


					
	key.type = jbvString;
	key.val.string.len = strlen("bitmapplans");
	key.val.string.val = strdup("bitmapplans");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->bitmapplans, state);

				
	
	key.type = jbvString;
	key.val.string.len = strlen("plan");
	key.val.string.val = strdup("plan");
	pushJsonbValue(&state, WJB_KEY, &key);

	Plan_ser(&node->plan, state, false);

		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *FuncExpr_ser(const FuncExpr *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


					if (!remove_fake_func && remove_fake_func != ((FuncExpr *)node)->funcid) {
						
	key.type = jbvString;
	key.val.string.len = strlen("args");
	key.val.string.val = strdup("args");
	pushJsonbValue(&state, WJB_KEY, &key);

						
	node_to_jsonb(node->args, state);

					}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("funccollid");
	key.val.string.val = strdup("funccollid");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, UInt32GetDatum(node->funccollid)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("funcformat");
	key.val.string.val = strdup("funcformat");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->funcformat)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("funcid");
	key.val.string.val = strdup("funcid");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, UInt32GetDatum(node->funcid)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("funcresulttype");
	key.val.string.val = strdup("funcresulttype");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, UInt32GetDatum(node->funcresulttype)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("funcretset");
	key.val.string.val = strdup("funcretset");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvBool;
	val.val.boolean = node->funcretset;
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("funcvariadic");
	key.val.string.val = strdup("funcvariadic");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvBool;
	val.val.boolean = node->funcvariadic;
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("inputcollid");
	key.val.string.val = strdup("inputcollid");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, UInt32GetDatum(node->inputcollid)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					if(!skip_location)
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("location");
	key.val.string.val = strdup("location");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, Int32GetDatum(node->location)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				
	
	key.type = jbvString;
	key.val.string.len = strlen("xpr");
	key.val.string.val = strdup("xpr");
	pushJsonbValue(&state, WJB_KEY, &key);

	Expr_ser(&node->xpr, state, false);

		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *AlterUserMappingStmt_ser(const AlterUserMappingStmt *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


					
	key.type = jbvString;
	key.val.string.len = strlen("options");
	key.val.string.val = strdup("options");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->options, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("servername");
	key.val.string.val = strdup("servername");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	if (node->servername == NULL)
	{
		val.type = jbvNull;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}
	else
	{
		val.type = jbvString;
		val.val.string.len = strlen(node->servername);
		val.val.string.val = (char *)node->servername;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("user");
	key.val.string.val = strdup("user");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->user, state);

		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *SetOp_ser(const SetOp *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("cmd");
	key.val.string.val = strdup("cmd");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->cmd)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					{
						int i;
						JsonbValue val;
						
	
	key.type = jbvString;
	key.val.string.len = strlen("dupColIdx");
	key.val.string.val = strdup("dupColIdx");
	pushJsonbValue(&state, WJB_KEY, &key);

	pushJsonbValue(&state, WJB_BEGIN_ARRAY, NULL);
	for (i = 0; i < node->numCols; i++)
	{
			
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->dupColIdx[i])));
	pushJsonbValue(&state, WJB_ELEM, &val);

	}
	pushJsonbValue(&state, WJB_END_ARRAY, NULL);

					}
				
					{
						int i;
						JsonbValue val;
						
	
	key.type = jbvString;
	key.val.string.len = strlen("dupOperators");
	key.val.string.val = strdup("dupOperators");
	pushJsonbValue(&state, WJB_KEY, &key);

	pushJsonbValue(&state, WJB_BEGIN_ARRAY, NULL);
	for (i = 0; i < node->numCols; i++)
	{
			
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, UInt32GetDatum(node->dupOperators[i])));
	pushJsonbValue(&state, WJB_ELEM, &val);

	}
	pushJsonbValue(&state, WJB_END_ARRAY, NULL);

					}
				
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("firstFlag");
	key.val.string.val = strdup("firstFlag");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, Int32GetDatum(node->firstFlag)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("flagColIdx");
	key.val.string.val = strdup("flagColIdx");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->flagColIdx)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("numCols");
	key.val.string.val = strdup("numCols");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, Int32GetDatum(node->numCols)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("numGroups");
	key.val.string.val = strdup("numGroups");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
#ifdef USE_FLOAT8_BYVAL
	val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int8_numeric, Int64GetDatum(node->numGroups)));
#else
	val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, Int32GetDatum(node->numGroups)));
#endif
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				
	
	key.type = jbvString;
	key.val.string.len = strlen("plan");
	key.val.string.val = strdup("plan");
	pushJsonbValue(&state, WJB_KEY, &key);

	Plan_ser(&node->plan, state, false);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("strategy");
	key.val.string.val = strdup("strategy");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->strategy)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *AlterTSConfigurationStmt_ser(const AlterTSConfigurationStmt *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


					
	key.type = jbvString;
	key.val.string.len = strlen("cfgname");
	key.val.string.val = strdup("cfgname");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->cfgname, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("dicts");
	key.val.string.val = strdup("dicts");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->dicts, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("kind");
	key.val.string.val = strdup("kind");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->kind)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("missing_ok");
	key.val.string.val = strdup("missing_ok");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvBool;
	val.val.boolean = node->missing_ok;
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("override");
	key.val.string.val = strdup("override");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvBool;
	val.val.boolean = node->override;
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("replace");
	key.val.string.val = strdup("replace");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvBool;
	val.val.boolean = node->replace;
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("tokentype");
	key.val.string.val = strdup("tokentype");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->tokentype, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *AlterRoleStmt_ser(const AlterRoleStmt *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("action");
	key.val.string.val = strdup("action");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, Int32GetDatum(node->action)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("options");
	key.val.string.val = strdup("options");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->options, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("role");
	key.val.string.val = strdup("role");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->role, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *Sort_ser(const Sort *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


					{
						int i;
						JsonbValue val;
						
	
	key.type = jbvString;
	key.val.string.len = strlen("collations");
	key.val.string.val = strdup("collations");
	pushJsonbValue(&state, WJB_KEY, &key);

	pushJsonbValue(&state, WJB_BEGIN_ARRAY, NULL);
	for (i = 0; i < node->numCols; i++)
	{
			
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, UInt32GetDatum(node->collations[i])));
	pushJsonbValue(&state, WJB_ELEM, &val);

	}
	pushJsonbValue(&state, WJB_END_ARRAY, NULL);

					}
				
					{
						int i;
						JsonbValue val;
						
	
	key.type = jbvString;
	key.val.string.len = strlen("nullsFirst");
	key.val.string.val = strdup("nullsFirst");
	pushJsonbValue(&state, WJB_KEY, &key);

	pushJsonbValue(&state, WJB_BEGIN_ARRAY, NULL);
	for (i = 0; i < node->numCols; i++)
	{
			
	val.type = jbvBool;
	val.val.boolean = node->nullsFirst[i];
	pushJsonbValue(&state, WJB_ELEM, &val);

	}
	pushJsonbValue(&state, WJB_END_ARRAY, NULL);

					}
				
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("numCols");
	key.val.string.val = strdup("numCols");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, Int32GetDatum(node->numCols)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				
	
	key.type = jbvString;
	key.val.string.len = strlen("plan");
	key.val.string.val = strdup("plan");
	pushJsonbValue(&state, WJB_KEY, &key);

	Plan_ser(&node->plan, state, false);

					{
						int i;
						JsonbValue val;
						
	
	key.type = jbvString;
	key.val.string.len = strlen("sortColIdx");
	key.val.string.val = strdup("sortColIdx");
	pushJsonbValue(&state, WJB_KEY, &key);

	pushJsonbValue(&state, WJB_BEGIN_ARRAY, NULL);
	for (i = 0; i < node->numCols; i++)
	{
			
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->sortColIdx[i])));
	pushJsonbValue(&state, WJB_ELEM, &val);

	}
	pushJsonbValue(&state, WJB_END_ARRAY, NULL);

					}
				
					{
						int i;
						JsonbValue val;
						
	
	key.type = jbvString;
	key.val.string.len = strlen("sortOperators");
	key.val.string.val = strdup("sortOperators");
	pushJsonbValue(&state, WJB_KEY, &key);

	pushJsonbValue(&state, WJB_BEGIN_ARRAY, NULL);
	for (i = 0; i < node->numCols; i++)
	{
			
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, UInt32GetDatum(node->sortOperators[i])));
	pushJsonbValue(&state, WJB_ELEM, &val);

	}
	pushJsonbValue(&state, WJB_END_ARRAY, NULL);

					}
				
		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *DiscardStmt_ser(const DiscardStmt *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("target");
	key.val.string.val = strdup("target");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->target)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *CreateForeignServerStmt_ser(const CreateForeignServerStmt *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("fdwname");
	key.val.string.val = strdup("fdwname");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	if (node->fdwname == NULL)
	{
		val.type = jbvNull;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}
	else
	{
		val.type = jbvString;
		val.val.string.len = strlen(node->fdwname);
		val.val.string.val = (char *)node->fdwname;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("options");
	key.val.string.val = strdup("options");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->options, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("servername");
	key.val.string.val = strdup("servername");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	if (node->servername == NULL)
	{
		val.type = jbvNull;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}
	else
	{
		val.type = jbvString;
		val.val.string.len = strlen(node->servername);
		val.val.string.val = (char *)node->servername;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("servertype");
	key.val.string.val = strdup("servertype");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	if (node->servertype == NULL)
	{
		val.type = jbvNull;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}
	else
	{
		val.type = jbvString;
		val.val.string.len = strlen(node->servertype);
		val.val.string.val = (char *)node->servertype;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("version");
	key.val.string.val = strdup("version");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	if (node->version == NULL)
	{
		val.type = jbvNull;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}
	else
	{
		val.type = jbvString;
		val.val.string.len = strlen(node->version);
		val.val.string.val = (char *)node->version;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}

				}
		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *CaseExpr_ser(const CaseExpr *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


					
	key.type = jbvString;
	key.val.string.len = strlen("arg");
	key.val.string.val = strdup("arg");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->arg, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("args");
	key.val.string.val = strdup("args");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->args, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("casecollid");
	key.val.string.val = strdup("casecollid");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, UInt32GetDatum(node->casecollid)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("casetype");
	key.val.string.val = strdup("casetype");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, UInt32GetDatum(node->casetype)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("defresult");
	key.val.string.val = strdup("defresult");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->defresult, state);

					if(!skip_location)
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("location");
	key.val.string.val = strdup("location");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, Int32GetDatum(node->location)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				
	
	key.type = jbvString;
	key.val.string.len = strlen("xpr");
	key.val.string.val = strdup("xpr");
	pushJsonbValue(&state, WJB_KEY, &key);

	Expr_ser(&node->xpr, state, false);

		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *CreateExtensionStmt_ser(const CreateExtensionStmt *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("extname");
	key.val.string.val = strdup("extname");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	if (node->extname == NULL)
	{
		val.type = jbvNull;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}
	else
	{
		val.type = jbvString;
		val.val.string.len = strlen(node->extname);
		val.val.string.val = (char *)node->extname;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("if_not_exists");
	key.val.string.val = strdup("if_not_exists");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvBool;
	val.val.boolean = node->if_not_exists;
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("options");
	key.val.string.val = strdup("options");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->options, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *CommonTableExpr_ser(const CommonTableExpr *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


					
	key.type = jbvString;
	key.val.string.len = strlen("aliascolnames");
	key.val.string.val = strdup("aliascolnames");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->aliascolnames, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("ctecolcollations");
	key.val.string.val = strdup("ctecolcollations");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->ctecolcollations, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("ctecolnames");
	key.val.string.val = strdup("ctecolnames");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->ctecolnames, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("ctecoltypes");
	key.val.string.val = strdup("ctecoltypes");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->ctecoltypes, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("ctecoltypmods");
	key.val.string.val = strdup("ctecoltypmods");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->ctecoltypmods, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("ctename");
	key.val.string.val = strdup("ctename");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	if (node->ctename == NULL)
	{
		val.type = jbvNull;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}
	else
	{
		val.type = jbvString;
		val.val.string.len = strlen(node->ctename);
		val.val.string.val = (char *)node->ctename;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("ctequery");
	key.val.string.val = strdup("ctequery");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->ctequery, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("cterecursive");
	key.val.string.val = strdup("cterecursive");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvBool;
	val.val.boolean = node->cterecursive;
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("cterefcount");
	key.val.string.val = strdup("cterefcount");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, Int32GetDatum(node->cterefcount)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					if(!skip_location)
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("location");
	key.val.string.val = strdup("location");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, Int32GetDatum(node->location)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *RenameStmt_ser(const RenameStmt *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("behavior");
	key.val.string.val = strdup("behavior");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->behavior)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("missing_ok");
	key.val.string.val = strdup("missing_ok");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvBool;
	val.val.boolean = node->missing_ok;
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("newname");
	key.val.string.val = strdup("newname");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	if (node->newname == NULL)
	{
		val.type = jbvNull;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}
	else
	{
		val.type = jbvString;
		val.val.string.len = strlen(node->newname);
		val.val.string.val = (char *)node->newname;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("objarg");
	key.val.string.val = strdup("objarg");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->objarg, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("object");
	key.val.string.val = strdup("object");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->object, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("relation");
	key.val.string.val = strdup("relation");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->relation, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("relationType");
	key.val.string.val = strdup("relationType");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->relationType)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("renameType");
	key.val.string.val = strdup("renameType");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->renameType)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("subname");
	key.val.string.val = strdup("subname");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	if (node->subname == NULL)
	{
		val.type = jbvNull;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}
	else
	{
		val.type = jbvString;
		val.val.string.len = strlen(node->subname);
		val.val.string.val = (char *)node->subname;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *A_Star_ser(const A_Star *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *UnlistenStmt_ser(const UnlistenStmt *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("conditionname");
	key.val.string.val = strdup("conditionname");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	if (node->conditionname == NULL)
	{
		val.type = jbvNull;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}
	else
	{
		val.type = jbvString;
		val.val.string.len = strlen(node->conditionname);
		val.val.string.val = (char *)node->conditionname;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *GroupingSet_ser(const GroupingSet *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


					
	key.type = jbvString;
	key.val.string.len = strlen("content");
	key.val.string.val = strdup("content");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->content, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("kind");
	key.val.string.val = strdup("kind");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->kind)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					if(!skip_location)
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("location");
	key.val.string.val = strdup("location");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, Int32GetDatum(node->location)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *BoolExpr_ser(const BoolExpr *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


					
	key.type = jbvString;
	key.val.string.len = strlen("args");
	key.val.string.val = strdup("args");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->args, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("boolop");
	key.val.string.val = strdup("boolop");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->boolop)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					if(!skip_location)
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("location");
	key.val.string.val = strdup("location");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, Int32GetDatum(node->location)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				
	
	key.type = jbvString;
	key.val.string.len = strlen("xpr");
	key.val.string.val = strdup("xpr");
	pushJsonbValue(&state, WJB_KEY, &key);

	Expr_ser(&node->xpr, state, false);

		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *BitmapAnd_ser(const BitmapAnd *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


					
	key.type = jbvString;
	key.val.string.len = strlen("bitmapplans");
	key.val.string.val = strdup("bitmapplans");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->bitmapplans, state);

				
	
	key.type = jbvString;
	key.val.string.len = strlen("plan");
	key.val.string.val = strdup("plan");
	pushJsonbValue(&state, WJB_KEY, &key);

	Plan_ser(&node->plan, state, false);

		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *RowExpr_ser(const RowExpr *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


					
	key.type = jbvString;
	key.val.string.len = strlen("args");
	key.val.string.val = strdup("args");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->args, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("colnames");
	key.val.string.val = strdup("colnames");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->colnames, state);

					if(!skip_location)
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("location");
	key.val.string.val = strdup("location");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, Int32GetDatum(node->location)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("row_format");
	key.val.string.val = strdup("row_format");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->row_format)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("row_typeid");
	key.val.string.val = strdup("row_typeid");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, UInt32GetDatum(node->row_typeid)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				
	
	key.type = jbvString;
	key.val.string.len = strlen("xpr");
	key.val.string.val = strdup("xpr");
	pushJsonbValue(&state, WJB_KEY, &key);

	Expr_ser(&node->xpr, state, false);

		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *UpdateStmt_ser(const UpdateStmt *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


					
	key.type = jbvString;
	key.val.string.len = strlen("fromClause");
	key.val.string.val = strdup("fromClause");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->fromClause, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("relation");
	key.val.string.val = strdup("relation");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->relation, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("returningList");
	key.val.string.val = strdup("returningList");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->returningList, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("targetList");
	key.val.string.val = strdup("targetList");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->targetList, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("whereClause");
	key.val.string.val = strdup("whereClause");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->whereClause, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("withClause");
	key.val.string.val = strdup("withClause");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->withClause, state);

		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *CollateExpr_ser(const CollateExpr *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


					
	key.type = jbvString;
	key.val.string.len = strlen("arg");
	key.val.string.val = strdup("arg");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->arg, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("collOid");
	key.val.string.val = strdup("collOid");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, UInt32GetDatum(node->collOid)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					if(!skip_location)
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("location");
	key.val.string.val = strdup("location");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, Int32GetDatum(node->location)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				
	
	key.type = jbvString;
	key.val.string.len = strlen("xpr");
	key.val.string.val = strdup("xpr");
	pushJsonbValue(&state, WJB_KEY, &key);

	Expr_ser(&node->xpr, state, false);

		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *DropTableSpaceStmt_ser(const DropTableSpaceStmt *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("missing_ok");
	key.val.string.val = strdup("missing_ok");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvBool;
	val.val.boolean = node->missing_ok;
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("tablespacename");
	key.val.string.val = strdup("tablespacename");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	if (node->tablespacename == NULL)
	{
		val.type = jbvNull;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}
	else
	{
		val.type = jbvString;
		val.val.string.len = strlen(node->tablespacename);
		val.val.string.val = (char *)node->tablespacename;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *DeallocateStmt_ser(const DeallocateStmt *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("name");
	key.val.string.val = strdup("name");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	if (node->name == NULL)
	{
		val.type = jbvNull;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}
	else
	{
		val.type = jbvString;
		val.val.string.len = strlen(node->name);
		val.val.string.val = (char *)node->name;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *CreatedbStmt_ser(const CreatedbStmt *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("dbname");
	key.val.string.val = strdup("dbname");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	if (node->dbname == NULL)
	{
		val.type = jbvNull;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}
	else
	{
		val.type = jbvString;
		val.val.string.len = strlen(node->dbname);
		val.val.string.val = (char *)node->dbname;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("options");
	key.val.string.val = strdup("options");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->options, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *Hash_ser(const Hash *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


				
	
	key.type = jbvString;
	key.val.string.len = strlen("plan");
	key.val.string.val = strdup("plan");
	pushJsonbValue(&state, WJB_KEY, &key);

	Plan_ser(&node->plan, state, false);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("skewColType");
	key.val.string.val = strdup("skewColType");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, UInt32GetDatum(node->skewColType)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("skewColTypmod");
	key.val.string.val = strdup("skewColTypmod");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, Int32GetDatum(node->skewColTypmod)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("skewColumn");
	key.val.string.val = strdup("skewColumn");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->skewColumn)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("skewInherit");
	key.val.string.val = strdup("skewInherit");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvBool;
	val.val.boolean = node->skewInherit;
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("skewTable");
	key.val.string.val = strdup("skewTable");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, UInt32GetDatum(node->skewTable)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *CurrentOfExpr_ser(const CurrentOfExpr *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("cursor_name");
	key.val.string.val = strdup("cursor_name");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	if (node->cursor_name == NULL)
	{
		val.type = jbvNull;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}
	else
	{
		val.type = jbvString;
		val.val.string.len = strlen(node->cursor_name);
		val.val.string.val = (char *)node->cursor_name;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("cursor_param");
	key.val.string.val = strdup("cursor_param");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, Int32GetDatum(node->cursor_param)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("cvarno");
	key.val.string.val = strdup("cvarno");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, UInt32GetDatum(node->cvarno)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				
	
	key.type = jbvString;
	key.val.string.len = strlen("xpr");
	key.val.string.val = strdup("xpr");
	pushJsonbValue(&state, WJB_KEY, &key);

	Expr_ser(&node->xpr, state, false);

		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *WindowDef_ser(const WindowDef *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


					
	key.type = jbvString;
	key.val.string.len = strlen("endOffset");
	key.val.string.val = strdup("endOffset");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->endOffset, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("frameOptions");
	key.val.string.val = strdup("frameOptions");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, Int32GetDatum(node->frameOptions)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					if(!skip_location)
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("location");
	key.val.string.val = strdup("location");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, Int32GetDatum(node->location)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("name");
	key.val.string.val = strdup("name");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	if (node->name == NULL)
	{
		val.type = jbvNull;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}
	else
	{
		val.type = jbvString;
		val.val.string.len = strlen(node->name);
		val.val.string.val = (char *)node->name;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("orderClause");
	key.val.string.val = strdup("orderClause");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->orderClause, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("partitionClause");
	key.val.string.val = strdup("partitionClause");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->partitionClause, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("refname");
	key.val.string.val = strdup("refname");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	if (node->refname == NULL)
	{
		val.type = jbvNull;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}
	else
	{
		val.type = jbvString;
		val.val.string.len = strlen(node->refname);
		val.val.string.val = (char *)node->refname;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("startOffset");
	key.val.string.val = strdup("startOffset");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->startOffset, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *AlterFunctionStmt_ser(const AlterFunctionStmt *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


					
	key.type = jbvString;
	key.val.string.len = strlen("actions");
	key.val.string.val = strdup("actions");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->actions, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("func");
	key.val.string.val = strdup("func");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->func, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *ExplainStmt_ser(const ExplainStmt *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


					
	key.type = jbvString;
	key.val.string.len = strlen("options");
	key.val.string.val = strdup("options");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->options, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("query");
	key.val.string.val = strdup("query");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->query, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *AlterDatabaseStmt_ser(const AlterDatabaseStmt *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("dbname");
	key.val.string.val = strdup("dbname");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	if (node->dbname == NULL)
	{
		val.type = jbvNull;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}
	else
	{
		val.type = jbvString;
		val.val.string.len = strlen(node->dbname);
		val.val.string.val = (char *)node->dbname;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("options");
	key.val.string.val = strdup("options");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->options, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *ParamRef_ser(const ParamRef *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


					if(!skip_location)
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("location");
	key.val.string.val = strdup("location");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, Int32GetDatum(node->location)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("number");
	key.val.string.val = strdup("number");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, Int32GetDatum(node->number)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *CreateForeignTableStmt_ser(const CreateForeignTableStmt *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


				
	
	key.type = jbvString;
	key.val.string.len = strlen("base");
	key.val.string.val = strdup("base");
	pushJsonbValue(&state, WJB_KEY, &key);

	CreateStmt_ser(&node->base, state, false);

					
	key.type = jbvString;
	key.val.string.len = strlen("options");
	key.val.string.val = strdup("options");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->options, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("servername");
	key.val.string.val = strdup("servername");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	if (node->servername == NULL)
	{
		val.type = jbvNull;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}
	else
	{
		val.type = jbvString;
		val.val.string.len = strlen(node->servername);
		val.val.string.val = (char *)node->servername;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}

				}
		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *BitmapHeapScan_ser(const BitmapHeapScan *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


					
	key.type = jbvString;
	key.val.string.len = strlen("bitmapqualorig");
	key.val.string.val = strdup("bitmapqualorig");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->bitmapqualorig, state);

				
	
	key.type = jbvString;
	key.val.string.len = strlen("scan");
	key.val.string.val = strdup("scan");
	pushJsonbValue(&state, WJB_KEY, &key);

	Scan_ser(&node->scan, state, false);

		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *Scan_ser(const Scan *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


				
	
	key.type = jbvString;
	key.val.string.len = strlen("plan");
	key.val.string.val = strdup("plan");
	pushJsonbValue(&state, WJB_KEY, &key);

	Plan_ser(&node->plan, state, false);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("scanrelid");
	key.val.string.val = strdup("scanrelid");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, UInt32GetDatum(node->scanrelid)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *AlterTableSpaceOptionsStmt_ser(const AlterTableSpaceOptionsStmt *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("isReset");
	key.val.string.val = strdup("isReset");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvBool;
	val.val.boolean = node->isReset;
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("options");
	key.val.string.val = strdup("options");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->options, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("tablespacename");
	key.val.string.val = strdup("tablespacename");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	if (node->tablespacename == NULL)
	{
		val.type = jbvNull;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}
	else
	{
		val.type = jbvString;
		val.val.string.len = strlen(node->tablespacename);
		val.val.string.val = (char *)node->tablespacename;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *FuncCall_ser(const FuncCall *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("agg_distinct");
	key.val.string.val = strdup("agg_distinct");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvBool;
	val.val.boolean = node->agg_distinct;
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("agg_filter");
	key.val.string.val = strdup("agg_filter");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->agg_filter, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("agg_order");
	key.val.string.val = strdup("agg_order");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->agg_order, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("agg_star");
	key.val.string.val = strdup("agg_star");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvBool;
	val.val.boolean = node->agg_star;
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("agg_within_group");
	key.val.string.val = strdup("agg_within_group");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvBool;
	val.val.boolean = node->agg_within_group;
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("args");
	key.val.string.val = strdup("args");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->args, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("func_variadic");
	key.val.string.val = strdup("func_variadic");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvBool;
	val.val.boolean = node->func_variadic;
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("funcname");
	key.val.string.val = strdup("funcname");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->funcname, state);

					if(!skip_location)
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("location");
	key.val.string.val = strdup("location");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, Int32GetDatum(node->location)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("over");
	key.val.string.val = strdup("over");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->over, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *Join_ser(const Join *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


					
	key.type = jbvString;
	key.val.string.len = strlen("joinqual");
	key.val.string.val = strdup("joinqual");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->joinqual, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("jointype");
	key.val.string.val = strdup("jointype");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->jointype)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				
	
	key.type = jbvString;
	key.val.string.len = strlen("plan");
	key.val.string.val = strdup("plan");
	pushJsonbValue(&state, WJB_KEY, &key);

	Plan_ser(&node->plan, state, false);

		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *ReindexStmt_ser(const ReindexStmt *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("kind");
	key.val.string.val = strdup("kind");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->kind)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("name");
	key.val.string.val = strdup("name");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	if (node->name == NULL)
	{
		val.type = jbvNull;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}
	else
	{
		val.type = jbvString;
		val.val.string.len = strlen(node->name);
		val.val.string.val = (char *)node->name;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("options");
	key.val.string.val = strdup("options");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, Int32GetDatum(node->options)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("relation");
	key.val.string.val = strdup("relation");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->relation, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *WithClause_ser(const WithClause *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


					
	key.type = jbvString;
	key.val.string.len = strlen("ctes");
	key.val.string.val = strdup("ctes");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->ctes, state);

					if(!skip_location)
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("location");
	key.val.string.val = strdup("location");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, Int32GetDatum(node->location)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("recursive");
	key.val.string.val = strdup("recursive");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvBool;
	val.val.boolean = node->recursive;
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *RangeSubselect_ser(const RangeSubselect *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


					
	key.type = jbvString;
	key.val.string.len = strlen("alias");
	key.val.string.val = strdup("alias");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->alias, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("lateral");
	key.val.string.val = strdup("lateral");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvBool;
	val.val.boolean = node->lateral;
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("subquery");
	key.val.string.val = strdup("subquery");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->subquery, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *CreatePLangStmt_ser(const CreatePLangStmt *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


					
	key.type = jbvString;
	key.val.string.len = strlen("plhandler");
	key.val.string.val = strdup("plhandler");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->plhandler, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("plinline");
	key.val.string.val = strdup("plinline");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->plinline, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("plname");
	key.val.string.val = strdup("plname");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	if (node->plname == NULL)
	{
		val.type = jbvNull;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}
	else
	{
		val.type = jbvString;
		val.val.string.len = strlen(node->plname);
		val.val.string.val = (char *)node->plname;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("pltrusted");
	key.val.string.val = strdup("pltrusted");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvBool;
	val.val.boolean = node->pltrusted;
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("plvalidator");
	key.val.string.val = strdup("plvalidator");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->plvalidator, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("replace");
	key.val.string.val = strdup("replace");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvBool;
	val.val.boolean = node->replace;
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *Const_ser(const Const *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("constbyval");
	key.val.string.val = strdup("constbyval");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvBool;
	val.val.boolean = node->constbyval;
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("constcollid");
	key.val.string.val = strdup("constcollid");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, UInt32GetDatum(node->constcollid)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("constisnull");
	key.val.string.val = strdup("constisnull");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvBool;
	val.val.boolean = node->constisnull;
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("constlen");
	key.val.string.val = strdup("constlen");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, Int32GetDatum(node->constlen)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("consttype");
	key.val.string.val = strdup("consttype");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, UInt32GetDatum(node->consttype)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("consttypmod");
	key.val.string.val = strdup("consttypmod");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, Int32GetDatum(node->consttypmod)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				
	key.type = jbvString;
	key.val.string.len = strlen("constvalue");
	key.val.string.val = strdup("constvalue");
	pushJsonbValue(&state, WJB_KEY, &key);

				if (node->constisnull)
				{
					JsonbValue val;
					val.type = jbvNull;
					pushJsonbValue(&state, WJB_VALUE, &val);
				}
				else
					datum_ser(state, node->constvalue, node->constlen, node->constbyval);
					if(!skip_location)
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("location");
	key.val.string.val = strdup("location");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, Int32GetDatum(node->location)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				
	
	key.type = jbvString;
	key.val.string.len = strlen("xpr");
	key.val.string.val = strdup("xpr");
	pushJsonbValue(&state, WJB_KEY, &key);

	Expr_ser(&node->xpr, state, false);

		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *XmlExpr_ser(const XmlExpr *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


					
	key.type = jbvString;
	key.val.string.len = strlen("arg_names");
	key.val.string.val = strdup("arg_names");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->arg_names, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("args");
	key.val.string.val = strdup("args");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->args, state);

					if(!skip_location)
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("location");
	key.val.string.val = strdup("location");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, Int32GetDatum(node->location)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("name");
	key.val.string.val = strdup("name");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	if (node->name == NULL)
	{
		val.type = jbvNull;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}
	else
	{
		val.type = jbvString;
		val.val.string.len = strlen(node->name);
		val.val.string.val = (char *)node->name;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("named_args");
	key.val.string.val = strdup("named_args");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->named_args, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("op");
	key.val.string.val = strdup("op");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->op)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, UInt32GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("typmod");
	key.val.string.val = strdup("typmod");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, Int32GetDatum(node->typmod)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("xmloption");
	key.val.string.val = strdup("xmloption");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->xmloption)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				
	
	key.type = jbvString;
	key.val.string.len = strlen("xpr");
	key.val.string.val = strdup("xpr");
	pushJsonbValue(&state, WJB_KEY, &key);

	Expr_ser(&node->xpr, state, false);

		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *FuncWithArgs_ser(const FuncWithArgs *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


					
	key.type = jbvString;
	key.val.string.len = strlen("funcargs");
	key.val.string.val = strdup("funcargs");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->funcargs, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("funcname");
	key.val.string.val = strdup("funcname");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->funcname, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *CollateClause_ser(const CollateClause *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


					
	key.type = jbvString;
	key.val.string.len = strlen("arg");
	key.val.string.val = strdup("arg");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->arg, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("collname");
	key.val.string.val = strdup("collname");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->collname, state);

					if(!skip_location)
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("location");
	key.val.string.val = strdup("location");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, Int32GetDatum(node->location)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *CreateUserMappingStmt_ser(const CreateUserMappingStmt *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


					
	key.type = jbvString;
	key.val.string.len = strlen("options");
	key.val.string.val = strdup("options");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->options, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("servername");
	key.val.string.val = strdup("servername");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	if (node->servername == NULL)
	{
		val.type = jbvNull;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}
	else
	{
		val.type = jbvString;
		val.val.string.len = strlen(node->servername);
		val.val.string.val = (char *)node->servername;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("user");
	key.val.string.val = strdup("user");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->user, state);

		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *ListenStmt_ser(const ListenStmt *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("conditionname");
	key.val.string.val = strdup("conditionname");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	if (node->conditionname == NULL)
	{
		val.type = jbvNull;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}
	else
	{
		val.type = jbvString;
		val.val.string.len = strlen(node->conditionname);
		val.val.string.val = (char *)node->conditionname;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *RefreshMatViewStmt_ser(const RefreshMatViewStmt *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("concurrent");
	key.val.string.val = strdup("concurrent");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvBool;
	val.val.boolean = node->concurrent;
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("relation");
	key.val.string.val = strdup("relation");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->relation, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("skipData");
	key.val.string.val = strdup("skipData");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvBool;
	val.val.boolean = node->skipData;
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *DeclareCursorStmt_ser(const DeclareCursorStmt *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("options");
	key.val.string.val = strdup("options");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, Int32GetDatum(node->options)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("portalname");
	key.val.string.val = strdup("portalname");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	if (node->portalname == NULL)
	{
		val.type = jbvNull;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}
	else
	{
		val.type = jbvString;
		val.val.string.len = strlen(node->portalname);
		val.val.string.val = (char *)node->portalname;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("query");
	key.val.string.val = strdup("query");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->query, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *CreateConversionStmt_ser(const CreateConversionStmt *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


					
	key.type = jbvString;
	key.val.string.len = strlen("conversion_name");
	key.val.string.val = strdup("conversion_name");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->conversion_name, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("def");
	key.val.string.val = strdup("def");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvBool;
	val.val.boolean = node->def;
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("for_encoding_name");
	key.val.string.val = strdup("for_encoding_name");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	if (node->for_encoding_name == NULL)
	{
		val.type = jbvNull;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}
	else
	{
		val.type = jbvString;
		val.val.string.len = strlen(node->for_encoding_name);
		val.val.string.val = (char *)node->for_encoding_name;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("func_name");
	key.val.string.val = strdup("func_name");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->func_name, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("to_encoding_name");
	key.val.string.val = strdup("to_encoding_name");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	if (node->to_encoding_name == NULL)
	{
		val.type = jbvNull;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}
	else
	{
		val.type = jbvString;
		val.val.string.len = strlen(node->to_encoding_name);
		val.val.string.val = (char *)node->to_encoding_name;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *Value_ser(const Value *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				/* NOT FOUND TYPE: NotFound */
		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *DropStmt_ser(const DropStmt *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


					
	key.type = jbvString;
	key.val.string.len = strlen("arguments");
	key.val.string.val = strdup("arguments");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->arguments, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("behavior");
	key.val.string.val = strdup("behavior");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->behavior)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("concurrent");
	key.val.string.val = strdup("concurrent");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvBool;
	val.val.boolean = node->concurrent;
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("missing_ok");
	key.val.string.val = strdup("missing_ok");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvBool;
	val.val.boolean = node->missing_ok;
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("objects");
	key.val.string.val = strdup("objects");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->objects, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("removeType");
	key.val.string.val = strdup("removeType");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->removeType)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *Limit_ser(const Limit *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


					
	key.type = jbvString;
	key.val.string.len = strlen("limitCount");
	key.val.string.val = strdup("limitCount");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->limitCount, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("limitOffset");
	key.val.string.val = strdup("limitOffset");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->limitOffset, state);

				
	
	key.type = jbvString;
	key.val.string.len = strlen("plan");
	key.val.string.val = strdup("plan");
	pushJsonbValue(&state, WJB_KEY, &key);

	Plan_ser(&node->plan, state, false);

		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *IntoClause_ser(const IntoClause *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


					
	key.type = jbvString;
	key.val.string.len = strlen("colNames");
	key.val.string.val = strdup("colNames");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->colNames, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("onCommit");
	key.val.string.val = strdup("onCommit");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->onCommit)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("options");
	key.val.string.val = strdup("options");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->options, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("rel");
	key.val.string.val = strdup("rel");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->rel, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("skipData");
	key.val.string.val = strdup("skipData");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvBool;
	val.val.boolean = node->skipData;
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("tableSpaceName");
	key.val.string.val = strdup("tableSpaceName");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	if (node->tableSpaceName == NULL)
	{
		val.type = jbvNull;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}
	else
	{
		val.type = jbvString;
		val.val.string.len = strlen(node->tableSpaceName);
		val.val.string.val = (char *)node->tableSpaceName;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("viewQuery");
	key.val.string.val = strdup("viewQuery");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->viewQuery, state);

		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *AlternativeSubPlan_ser(const AlternativeSubPlan *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


					
	key.type = jbvString;
	key.val.string.len = strlen("subplans");
	key.val.string.val = strdup("subplans");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->subplans, state);

				
	
	key.type = jbvString;
	key.val.string.len = strlen("xpr");
	key.val.string.val = strdup("xpr");
	pushJsonbValue(&state, WJB_KEY, &key);

	Expr_ser(&node->xpr, state, false);

		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *ForeignScan_ser(const ForeignScan *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


					
	key.type = jbvString;
	key.val.string.len = strlen("fdw_exprs");
	key.val.string.val = strdup("fdw_exprs");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->fdw_exprs, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("fdw_private");
	key.val.string.val = strdup("fdw_private");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->fdw_private, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("fdw_recheck_quals");
	key.val.string.val = strdup("fdw_recheck_quals");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->fdw_recheck_quals, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("fdw_scan_tlist");
	key.val.string.val = strdup("fdw_scan_tlist");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->fdw_scan_tlist, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("fsSystemCol");
	key.val.string.val = strdup("fsSystemCol");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvBool;
	val.val.boolean = node->fsSystemCol;
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	
	key.type = jbvString;
	key.val.string.len = strlen("fs_relids");
	key.val.string.val = strdup("fs_relids");
	pushJsonbValue(&state, WJB_KEY, &key);

	pushJsonbValue(&state, WJB_KEY, &key);
	
	if (node->fs_relids == NULL)
	{
		val.type = jbvNull;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}
	else
	{
		int x = -1;
		pushJsonbValue(&state, WJB_BEGIN_ARRAY, NULL);
		while ((x = bms_next_member(node->fs_relids, x)) >= 0)
		{
			val.type = jbvNumeric;
			val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, Int32GetDatum(x)));
			pushJsonbValue(&state, WJB_ELEM, &val);
		}
		pushJsonbValue(&state, WJB_END_ARRAY, NULL);
	}

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("fs_server");
	key.val.string.val = strdup("fs_server");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, UInt32GetDatum(node->fs_server)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				
	
	key.type = jbvString;
	key.val.string.len = strlen("scan");
	key.val.string.val = strdup("scan");
	pushJsonbValue(&state, WJB_KEY, &key);

	Scan_ser(&node->scan, state, false);

		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *A_Const_ser(const A_Const *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


					if(!skip_location)
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("location");
	key.val.string.val = strdup("location");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, Int32GetDatum(node->location)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				
	key.type = jbvString;
	key.val.string.len = strlen("val");
	key.val.string.val = strdup("val");
	pushJsonbValue(&state, WJB_KEY, &key);

				
	if ((&node->val) == NULL)
	{
		JsonbValue	val;
		val.type = jbvNull;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}
	else if (IsA((&node->val), String) || IsA((&node->val), BitString) || IsA((&node->val), Float) )
	{
		JsonbValue	val;
		val.type = jbvString;
		val.val.string.len = strlen(((Value *)(&node->val))->val.str);
		val.val.string.val = ((Value *)(&node->val))->val.str;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}
	else if (IsA((&node->val), Integer))
	{
		JsonbValue	val;
		val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, Int32GetDatum(((Value *)(&node->val))->val.ival)));
		pushJsonbValue(&state, WJB_VALUE, &val);
	}
	else if (IsA((&node->val), Null))
	{
		JsonbValue	val;
		val.type = jbvNull;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}

		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *AlterSystemStmt_ser(const AlterSystemStmt *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


					
	key.type = jbvString;
	key.val.string.len = strlen("setstmt");
	key.val.string.val = strdup("setstmt");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->setstmt, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *ResTarget_ser(const ResTarget *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


					
	key.type = jbvString;
	key.val.string.len = strlen("indirection");
	key.val.string.val = strdup("indirection");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->indirection, state);

					if(!skip_location)
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("location");
	key.val.string.val = strdup("location");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, Int32GetDatum(node->location)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("name");
	key.val.string.val = strdup("name");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	if (node->name == NULL)
	{
		val.type = jbvNull;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}
	else
	{
		val.type = jbvString;
		val.val.string.len = strlen(node->name);
		val.val.string.val = (char *)node->name;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("val");
	key.val.string.val = strdup("val");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->val, state);

		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *RangeTblEntry_ser(const RangeTblEntry *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


					
	key.type = jbvString;
	key.val.string.len = strlen("alias");
	key.val.string.val = strdup("alias");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->alias, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("checkAsUser");
	key.val.string.val = strdup("checkAsUser");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, UInt32GetDatum(node->checkAsUser)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("ctecolcollations");
	key.val.string.val = strdup("ctecolcollations");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->ctecolcollations, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("ctecoltypes");
	key.val.string.val = strdup("ctecoltypes");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->ctecoltypes, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("ctecoltypmods");
	key.val.string.val = strdup("ctecoltypmods");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->ctecoltypmods, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("ctelevelsup");
	key.val.string.val = strdup("ctelevelsup");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, UInt32GetDatum(node->ctelevelsup)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("ctename");
	key.val.string.val = strdup("ctename");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	if (node->ctename == NULL)
	{
		val.type = jbvNull;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}
	else
	{
		val.type = jbvString;
		val.val.string.len = strlen(node->ctename);
		val.val.string.val = (char *)node->ctename;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("eref");
	key.val.string.val = strdup("eref");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->eref, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("funcordinality");
	key.val.string.val = strdup("funcordinality");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvBool;
	val.val.boolean = node->funcordinality;
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("functions");
	key.val.string.val = strdup("functions");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->functions, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("inFromCl");
	key.val.string.val = strdup("inFromCl");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvBool;
	val.val.boolean = node->inFromCl;
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("inh");
	key.val.string.val = strdup("inh");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvBool;
	val.val.boolean = node->inh;
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	
	key.type = jbvString;
	key.val.string.len = strlen("insertedCols");
	key.val.string.val = strdup("insertedCols");
	pushJsonbValue(&state, WJB_KEY, &key);

	pushJsonbValue(&state, WJB_KEY, &key);
	
	if (node->insertedCols == NULL)
	{
		val.type = jbvNull;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}
	else
	{
		int x = -1;
		pushJsonbValue(&state, WJB_BEGIN_ARRAY, NULL);
		while ((x = bms_next_member(node->insertedCols, x)) >= 0)
		{
			val.type = jbvNumeric;
			val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, Int32GetDatum(x)));
			pushJsonbValue(&state, WJB_ELEM, &val);
		}
		pushJsonbValue(&state, WJB_END_ARRAY, NULL);
	}

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("joinaliasvars");
	key.val.string.val = strdup("joinaliasvars");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->joinaliasvars, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("jointype");
	key.val.string.val = strdup("jointype");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->jointype)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("lateral");
	key.val.string.val = strdup("lateral");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvBool;
	val.val.boolean = node->lateral;
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("relid");
	key.val.string.val = strdup("relid");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, UInt32GetDatum(node->relid)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("relkind");
	key.val.string.val = strdup("relkind");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->relkind)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("requiredPerms");
	key.val.string.val = strdup("requiredPerms");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, UInt32GetDatum(node->requiredPerms)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("rtekind");
	key.val.string.val = strdup("rtekind");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->rtekind)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("securityQuals");
	key.val.string.val = strdup("securityQuals");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->securityQuals, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("security_barrier");
	key.val.string.val = strdup("security_barrier");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvBool;
	val.val.boolean = node->security_barrier;
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	
	key.type = jbvString;
	key.val.string.len = strlen("selectedCols");
	key.val.string.val = strdup("selectedCols");
	pushJsonbValue(&state, WJB_KEY, &key);

	pushJsonbValue(&state, WJB_KEY, &key);
	
	if (node->selectedCols == NULL)
	{
		val.type = jbvNull;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}
	else
	{
		int x = -1;
		pushJsonbValue(&state, WJB_BEGIN_ARRAY, NULL);
		while ((x = bms_next_member(node->selectedCols, x)) >= 0)
		{
			val.type = jbvNumeric;
			val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, Int32GetDatum(x)));
			pushJsonbValue(&state, WJB_ELEM, &val);
		}
		pushJsonbValue(&state, WJB_END_ARRAY, NULL);
	}

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("self_reference");
	key.val.string.val = strdup("self_reference");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvBool;
	val.val.boolean = node->self_reference;
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("subquery");
	key.val.string.val = strdup("subquery");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->subquery, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("tablesample");
	key.val.string.val = strdup("tablesample");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->tablesample, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	
	key.type = jbvString;
	key.val.string.len = strlen("updatedCols");
	key.val.string.val = strdup("updatedCols");
	pushJsonbValue(&state, WJB_KEY, &key);

	pushJsonbValue(&state, WJB_KEY, &key);
	
	if (node->updatedCols == NULL)
	{
		val.type = jbvNull;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}
	else
	{
		int x = -1;
		pushJsonbValue(&state, WJB_BEGIN_ARRAY, NULL);
		while ((x = bms_next_member(node->updatedCols, x)) >= 0)
		{
			val.type = jbvNumeric;
			val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, Int32GetDatum(x)));
			pushJsonbValue(&state, WJB_ELEM, &val);
		}
		pushJsonbValue(&state, WJB_END_ARRAY, NULL);
	}

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("values_collations");
	key.val.string.val = strdup("values_collations");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->values_collations, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("values_lists");
	key.val.string.val = strdup("values_lists");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->values_lists, state);

		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *A_Indirection_ser(const A_Indirection *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


					
	key.type = jbvString;
	key.val.string.len = strlen("arg");
	key.val.string.val = strdup("arg");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->arg, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("indirection");
	key.val.string.val = strdup("indirection");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->indirection, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *Append_ser(const Append *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


					
	key.type = jbvString;
	key.val.string.len = strlen("appendplans");
	key.val.string.val = strdup("appendplans");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->appendplans, state);

				
	
	key.type = jbvString;
	key.val.string.len = strlen("plan");
	key.val.string.val = strdup("plan");
	pushJsonbValue(&state, WJB_KEY, &key);

	Plan_ser(&node->plan, state, false);

		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *TableSampleClause_ser(const TableSampleClause *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


					
	key.type = jbvString;
	key.val.string.len = strlen("args");
	key.val.string.val = strdup("args");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->args, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("repeatable");
	key.val.string.val = strdup("repeatable");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->repeatable, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("tsmhandler");
	key.val.string.val = strdup("tsmhandler");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, UInt32GetDatum(node->tsmhandler)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *BooleanTest_ser(const BooleanTest *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


					
	key.type = jbvString;
	key.val.string.len = strlen("arg");
	key.val.string.val = strdup("arg");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->arg, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("booltesttype");
	key.val.string.val = strdup("booltesttype");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->booltesttype)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					if(!skip_location)
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("location");
	key.val.string.val = strdup("location");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, Int32GetDatum(node->location)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				
	
	key.type = jbvString;
	key.val.string.len = strlen("xpr");
	key.val.string.val = strdup("xpr");
	pushJsonbValue(&state, WJB_KEY, &key);

	Expr_ser(&node->xpr, state, false);

		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *ArrayCoerceExpr_ser(const ArrayCoerceExpr *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


					
	key.type = jbvString;
	key.val.string.len = strlen("arg");
	key.val.string.val = strdup("arg");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->arg, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("coerceformat");
	key.val.string.val = strdup("coerceformat");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->coerceformat)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("elemfuncid");
	key.val.string.val = strdup("elemfuncid");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, UInt32GetDatum(node->elemfuncid)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("isExplicit");
	key.val.string.val = strdup("isExplicit");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvBool;
	val.val.boolean = node->isExplicit;
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					if(!skip_location)
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("location");
	key.val.string.val = strdup("location");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, Int32GetDatum(node->location)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("resultcollid");
	key.val.string.val = strdup("resultcollid");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, UInt32GetDatum(node->resultcollid)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("resulttype");
	key.val.string.val = strdup("resulttype");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, UInt32GetDatum(node->resulttype)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("resulttypmod");
	key.val.string.val = strdup("resulttypmod");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, Int32GetDatum(node->resulttypmod)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				
	
	key.type = jbvString;
	key.val.string.len = strlen("xpr");
	key.val.string.val = strdup("xpr");
	pushJsonbValue(&state, WJB_KEY, &key);

	Expr_ser(&node->xpr, state, false);

		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *WithCheckOption_ser(const WithCheckOption *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("cascaded");
	key.val.string.val = strdup("cascaded");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvBool;
	val.val.boolean = node->cascaded;
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("kind");
	key.val.string.val = strdup("kind");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->kind)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("polname");
	key.val.string.val = strdup("polname");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	if (node->polname == NULL)
	{
		val.type = jbvNull;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}
	else
	{
		val.type = jbvString;
		val.val.string.len = strlen(node->polname);
		val.val.string.val = (char *)node->polname;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("qual");
	key.val.string.val = strdup("qual");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->qual, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("relname");
	key.val.string.val = strdup("relname");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	if (node->relname == NULL)
	{
		val.type = jbvNull;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}
	else
	{
		val.type = jbvString;
		val.val.string.len = strlen(node->relname);
		val.val.string.val = (char *)node->relname;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *RowCompareExpr_ser(const RowCompareExpr *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


					
	key.type = jbvString;
	key.val.string.len = strlen("inputcollids");
	key.val.string.val = strdup("inputcollids");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->inputcollids, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("largs");
	key.val.string.val = strdup("largs");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->largs, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("opfamilies");
	key.val.string.val = strdup("opfamilies");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->opfamilies, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("opnos");
	key.val.string.val = strdup("opnos");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->opnos, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("rargs");
	key.val.string.val = strdup("rargs");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->rargs, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("rctype");
	key.val.string.val = strdup("rctype");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->rctype)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				
	
	key.type = jbvString;
	key.val.string.len = strlen("xpr");
	key.val.string.val = strdup("xpr");
	pushJsonbValue(&state, WJB_KEY, &key);

	Expr_ser(&node->xpr, state, false);

		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *MultiAssignRef_ser(const MultiAssignRef *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("colno");
	key.val.string.val = strdup("colno");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, Int32GetDatum(node->colno)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("ncolumns");
	key.val.string.val = strdup("ncolumns");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, Int32GetDatum(node->ncolumns)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("source");
	key.val.string.val = strdup("source");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->source, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *TruncateStmt_ser(const TruncateStmt *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("behavior");
	key.val.string.val = strdup("behavior");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->behavior)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("relations");
	key.val.string.val = strdup("relations");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->relations, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("restart_seqs");
	key.val.string.val = strdup("restart_seqs");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvBool;
	val.val.boolean = node->restart_seqs;
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *CoerceToDomain_ser(const CoerceToDomain *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


					
	key.type = jbvString;
	key.val.string.len = strlen("arg");
	key.val.string.val = strdup("arg");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->arg, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("coercionformat");
	key.val.string.val = strdup("coercionformat");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->coercionformat)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					if(!skip_location)
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("location");
	key.val.string.val = strdup("location");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, Int32GetDatum(node->location)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("resultcollid");
	key.val.string.val = strdup("resultcollid");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, UInt32GetDatum(node->resultcollid)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("resulttype");
	key.val.string.val = strdup("resulttype");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, UInt32GetDatum(node->resulttype)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("resulttypmod");
	key.val.string.val = strdup("resulttypmod");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, Int32GetDatum(node->resulttypmod)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				
	
	key.type = jbvString;
	key.val.string.len = strlen("xpr");
	key.val.string.val = strdup("xpr");
	pushJsonbValue(&state, WJB_KEY, &key);

	Expr_ser(&node->xpr, state, false);

		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *WindowClause_ser(const WindowClause *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("copiedOrder");
	key.val.string.val = strdup("copiedOrder");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvBool;
	val.val.boolean = node->copiedOrder;
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("endOffset");
	key.val.string.val = strdup("endOffset");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->endOffset, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("frameOptions");
	key.val.string.val = strdup("frameOptions");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, Int32GetDatum(node->frameOptions)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("name");
	key.val.string.val = strdup("name");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	if (node->name == NULL)
	{
		val.type = jbvNull;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}
	else
	{
		val.type = jbvString;
		val.val.string.len = strlen(node->name);
		val.val.string.val = (char *)node->name;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("orderClause");
	key.val.string.val = strdup("orderClause");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->orderClause, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("partitionClause");
	key.val.string.val = strdup("partitionClause");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->partitionClause, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("refname");
	key.val.string.val = strdup("refname");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	if (node->refname == NULL)
	{
		val.type = jbvNull;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}
	else
	{
		val.type = jbvString;
		val.val.string.len = strlen(node->refname);
		val.val.string.val = (char *)node->refname;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("startOffset");
	key.val.string.val = strdup("startOffset");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->startOffset, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("winref");
	key.val.string.val = strdup("winref");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, UInt32GetDatum(node->winref)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *DefElem_ser(const DefElem *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


					
	key.type = jbvString;
	key.val.string.len = strlen("arg");
	key.val.string.val = strdup("arg");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->arg, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("defaction");
	key.val.string.val = strdup("defaction");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->defaction)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("defname");
	key.val.string.val = strdup("defname");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	if (node->defname == NULL)
	{
		val.type = jbvNull;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}
	else
	{
		val.type = jbvString;
		val.val.string.len = strlen(node->defname);
		val.val.string.val = (char *)node->defname;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("defnamespace");
	key.val.string.val = strdup("defnamespace");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	if (node->defnamespace == NULL)
	{
		val.type = jbvNull;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}
	else
	{
		val.type = jbvString;
		val.val.string.len = strlen(node->defnamespace);
		val.val.string.val = (char *)node->defnamespace;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *DropUserMappingStmt_ser(const DropUserMappingStmt *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("missing_ok");
	key.val.string.val = strdup("missing_ok");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvBool;
	val.val.boolean = node->missing_ok;
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("servername");
	key.val.string.val = strdup("servername");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	if (node->servername == NULL)
	{
		val.type = jbvNull;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}
	else
	{
		val.type = jbvString;
		val.val.string.len = strlen(node->servername);
		val.val.string.val = (char *)node->servername;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("user");
	key.val.string.val = strdup("user");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->user, state);

		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *A_ArrayExpr_ser(const A_ArrayExpr *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


					
	key.type = jbvString;
	key.val.string.len = strlen("elements");
	key.val.string.val = strdup("elements");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->elements, state);

					if(!skip_location)
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("location");
	key.val.string.val = strdup("location");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, Int32GetDatum(node->location)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *CreateTableAsStmt_ser(const CreateTableAsStmt *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("if_not_exists");
	key.val.string.val = strdup("if_not_exists");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvBool;
	val.val.boolean = node->if_not_exists;
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("into");
	key.val.string.val = strdup("into");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->into, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("is_select_into");
	key.val.string.val = strdup("is_select_into");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvBool;
	val.val.boolean = node->is_select_into;
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("query");
	key.val.string.val = strdup("query");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->query, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("relkind");
	key.val.string.val = strdup("relkind");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->relkind)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *OpExpr_ser(const OpExpr *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


					
	key.type = jbvString;
	key.val.string.len = strlen("args");
	key.val.string.val = strdup("args");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->args, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("inputcollid");
	key.val.string.val = strdup("inputcollid");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, UInt32GetDatum(node->inputcollid)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					if(!skip_location)
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("location");
	key.val.string.val = strdup("location");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, Int32GetDatum(node->location)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("opcollid");
	key.val.string.val = strdup("opcollid");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, UInt32GetDatum(node->opcollid)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("opfuncid");
	key.val.string.val = strdup("opfuncid");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, UInt32GetDatum(node->opfuncid)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("opno");
	key.val.string.val = strdup("opno");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, UInt32GetDatum(node->opno)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("opresulttype");
	key.val.string.val = strdup("opresulttype");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, UInt32GetDatum(node->opresulttype)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("opretset");
	key.val.string.val = strdup("opretset");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvBool;
	val.val.boolean = node->opretset;
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				
	
	key.type = jbvString;
	key.val.string.len = strlen("xpr");
	key.val.string.val = strdup("xpr");
	pushJsonbValue(&state, WJB_KEY, &key);

	Expr_ser(&node->xpr, state, false);

		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *FieldStore_ser(const FieldStore *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


					
	key.type = jbvString;
	key.val.string.len = strlen("arg");
	key.val.string.val = strdup("arg");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->arg, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("fieldnums");
	key.val.string.val = strdup("fieldnums");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->fieldnums, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("newvals");
	key.val.string.val = strdup("newvals");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->newvals, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("resulttype");
	key.val.string.val = strdup("resulttype");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, UInt32GetDatum(node->resulttype)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				
	
	key.type = jbvString;
	key.val.string.len = strlen("xpr");
	key.val.string.val = strdup("xpr");
	pushJsonbValue(&state, WJB_KEY, &key);

	Expr_ser(&node->xpr, state, false);

		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *CoerceViaIO_ser(const CoerceViaIO *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


					
	key.type = jbvString;
	key.val.string.len = strlen("arg");
	key.val.string.val = strdup("arg");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->arg, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("coerceformat");
	key.val.string.val = strdup("coerceformat");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->coerceformat)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					if(!skip_location)
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("location");
	key.val.string.val = strdup("location");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, Int32GetDatum(node->location)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("resultcollid");
	key.val.string.val = strdup("resultcollid");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, UInt32GetDatum(node->resultcollid)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("resulttype");
	key.val.string.val = strdup("resulttype");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, UInt32GetDatum(node->resulttype)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				
	
	key.type = jbvString;
	key.val.string.len = strlen("xpr");
	key.val.string.val = strdup("xpr");
	pushJsonbValue(&state, WJB_KEY, &key);

	Expr_ser(&node->xpr, state, false);

		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *SubLink_ser(const SubLink *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


					if(!skip_location)
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("location");
	key.val.string.val = strdup("location");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, Int32GetDatum(node->location)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("operName");
	key.val.string.val = strdup("operName");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->operName, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("subLinkId");
	key.val.string.val = strdup("subLinkId");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, Int32GetDatum(node->subLinkId)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("subLinkType");
	key.val.string.val = strdup("subLinkType");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->subLinkType)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("subselect");
	key.val.string.val = strdup("subselect");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->subselect, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("testexpr");
	key.val.string.val = strdup("testexpr");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->testexpr, state);

				
	
	key.type = jbvString;
	key.val.string.len = strlen("xpr");
	key.val.string.val = strdup("xpr");
	pushJsonbValue(&state, WJB_KEY, &key);

	Expr_ser(&node->xpr, state, false);

		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *WorkTableScan_ser(const WorkTableScan *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


				
	
	key.type = jbvString;
	key.val.string.len = strlen("scan");
	key.val.string.val = strdup("scan");
	pushJsonbValue(&state, WJB_KEY, &key);

	Scan_ser(&node->scan, state, false);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("wtParam");
	key.val.string.val = strdup("wtParam");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, Int32GetDatum(node->wtParam)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *A_Expr_ser(const A_Expr *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("kind");
	key.val.string.val = strdup("kind");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->kind)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("lexpr");
	key.val.string.val = strdup("lexpr");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->lexpr, state);

					if(!skip_location)
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("location");
	key.val.string.val = strdup("location");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, Int32GetDatum(node->location)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("name");
	key.val.string.val = strdup("name");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->name, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("rexpr");
	key.val.string.val = strdup("rexpr");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->rexpr, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *DropOwnedStmt_ser(const DropOwnedStmt *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("behavior");
	key.val.string.val = strdup("behavior");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->behavior)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("roles");
	key.val.string.val = strdup("roles");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->roles, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *AlterDatabaseSetStmt_ser(const AlterDatabaseSetStmt *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("dbname");
	key.val.string.val = strdup("dbname");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	if (node->dbname == NULL)
	{
		val.type = jbvNull;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}
	else
	{
		val.type = jbvString;
		val.val.string.len = strlen(node->dbname);
		val.val.string.val = (char *)node->dbname;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("setstmt");
	key.val.string.val = strdup("setstmt");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->setstmt, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *CreateRangeStmt_ser(const CreateRangeStmt *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


					
	key.type = jbvString;
	key.val.string.len = strlen("params");
	key.val.string.val = strdup("params");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->params, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("typeName");
	key.val.string.val = strdup("typeName");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->typeName, state);

		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *DeleteStmt_ser(const DeleteStmt *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


					
	key.type = jbvString;
	key.val.string.len = strlen("relation");
	key.val.string.val = strdup("relation");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->relation, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("returningList");
	key.val.string.val = strdup("returningList");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->returningList, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("usingClause");
	key.val.string.val = strdup("usingClause");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->usingClause, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("whereClause");
	key.val.string.val = strdup("whereClause");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->whereClause, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("withClause");
	key.val.string.val = strdup("withClause");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->withClause, state);

		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *CreateSeqStmt_ser(const CreateSeqStmt *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("if_not_exists");
	key.val.string.val = strdup("if_not_exists");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvBool;
	val.val.boolean = node->if_not_exists;
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("options");
	key.val.string.val = strdup("options");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->options, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("ownerId");
	key.val.string.val = strdup("ownerId");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, UInt32GetDatum(node->ownerId)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("sequence");
	key.val.string.val = strdup("sequence");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->sequence, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *RuleStmt_ser(const RuleStmt *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


					
	key.type = jbvString;
	key.val.string.len = strlen("actions");
	key.val.string.val = strdup("actions");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->actions, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("event");
	key.val.string.val = strdup("event");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->event)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("instead");
	key.val.string.val = strdup("instead");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvBool;
	val.val.boolean = node->instead;
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("relation");
	key.val.string.val = strdup("relation");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->relation, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("replace");
	key.val.string.val = strdup("replace");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvBool;
	val.val.boolean = node->replace;
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("rulename");
	key.val.string.val = strdup("rulename");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	if (node->rulename == NULL)
	{
		val.type = jbvNull;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}
	else
	{
		val.type = jbvString;
		val.val.string.len = strlen(node->rulename);
		val.val.string.val = (char *)node->rulename;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("whereClause");
	key.val.string.val = strdup("whereClause");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->whereClause, state);

		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *RangeTblFunction_ser(const RangeTblFunction *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


					
	key.type = jbvString;
	key.val.string.len = strlen("funccolcollations");
	key.val.string.val = strdup("funccolcollations");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->funccolcollations, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("funccolcount");
	key.val.string.val = strdup("funccolcount");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, Int32GetDatum(node->funccolcount)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("funccolnames");
	key.val.string.val = strdup("funccolnames");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->funccolnames, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("funccoltypes");
	key.val.string.val = strdup("funccoltypes");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->funccoltypes, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("funccoltypmods");
	key.val.string.val = strdup("funccoltypmods");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->funccoltypmods, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("funcexpr");
	key.val.string.val = strdup("funcexpr");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->funcexpr, state);

				{
					JsonbValue val;
					
	
	key.type = jbvString;
	key.val.string.len = strlen("funcparams");
	key.val.string.val = strdup("funcparams");
	pushJsonbValue(&state, WJB_KEY, &key);

	pushJsonbValue(&state, WJB_KEY, &key);
	
	if (node->funcparams == NULL)
	{
		val.type = jbvNull;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}
	else
	{
		int x = -1;
		pushJsonbValue(&state, WJB_BEGIN_ARRAY, NULL);
		while ((x = bms_next_member(node->funcparams, x)) >= 0)
		{
			val.type = jbvNumeric;
			val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, Int32GetDatum(x)));
			pushJsonbValue(&state, WJB_ELEM, &val);
		}
		pushJsonbValue(&state, WJB_END_ARRAY, NULL);
	}

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *IndexStmt_ser(const IndexStmt *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("accessMethod");
	key.val.string.val = strdup("accessMethod");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	if (node->accessMethod == NULL)
	{
		val.type = jbvNull;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}
	else
	{
		val.type = jbvString;
		val.val.string.len = strlen(node->accessMethod);
		val.val.string.val = (char *)node->accessMethod;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("concurrent");
	key.val.string.val = strdup("concurrent");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvBool;
	val.val.boolean = node->concurrent;
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("deferrable");
	key.val.string.val = strdup("deferrable");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvBool;
	val.val.boolean = node->deferrable;
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("excludeOpNames");
	key.val.string.val = strdup("excludeOpNames");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->excludeOpNames, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("idxcomment");
	key.val.string.val = strdup("idxcomment");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	if (node->idxcomment == NULL)
	{
		val.type = jbvNull;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}
	else
	{
		val.type = jbvString;
		val.val.string.len = strlen(node->idxcomment);
		val.val.string.val = (char *)node->idxcomment;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("idxname");
	key.val.string.val = strdup("idxname");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	if (node->idxname == NULL)
	{
		val.type = jbvNull;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}
	else
	{
		val.type = jbvString;
		val.val.string.len = strlen(node->idxname);
		val.val.string.val = (char *)node->idxname;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("if_not_exists");
	key.val.string.val = strdup("if_not_exists");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvBool;
	val.val.boolean = node->if_not_exists;
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("indexOid");
	key.val.string.val = strdup("indexOid");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, UInt32GetDatum(node->indexOid)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("indexParams");
	key.val.string.val = strdup("indexParams");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->indexParams, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("initdeferred");
	key.val.string.val = strdup("initdeferred");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvBool;
	val.val.boolean = node->initdeferred;
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("isconstraint");
	key.val.string.val = strdup("isconstraint");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvBool;
	val.val.boolean = node->isconstraint;
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("oldNode");
	key.val.string.val = strdup("oldNode");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, UInt32GetDatum(node->oldNode)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("options");
	key.val.string.val = strdup("options");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->options, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("primary");
	key.val.string.val = strdup("primary");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvBool;
	val.val.boolean = node->primary;
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("relation");
	key.val.string.val = strdup("relation");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->relation, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("tableSpace");
	key.val.string.val = strdup("tableSpace");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	if (node->tableSpace == NULL)
	{
		val.type = jbvNull;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}
	else
	{
		val.type = jbvString;
		val.val.string.len = strlen(node->tableSpace);
		val.val.string.val = (char *)node->tableSpace;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("transformed");
	key.val.string.val = strdup("transformed");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvBool;
	val.val.boolean = node->transformed;
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("unique");
	key.val.string.val = strdup("unique");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvBool;
	val.val.boolean = node->unique;
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("whereClause");
	key.val.string.val = strdup("whereClause");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->whereClause, state);

		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *WindowAgg_ser(const WindowAgg *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


					
	key.type = jbvString;
	key.val.string.len = strlen("endOffset");
	key.val.string.val = strdup("endOffset");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->endOffset, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("frameOptions");
	key.val.string.val = strdup("frameOptions");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, Int32GetDatum(node->frameOptions)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					{
						int i;
						JsonbValue val;
						
	
	key.type = jbvString;
	key.val.string.len = strlen("ordColIdx");
	key.val.string.val = strdup("ordColIdx");
	pushJsonbValue(&state, WJB_KEY, &key);

	pushJsonbValue(&state, WJB_BEGIN_ARRAY, NULL);
	for (i = 0; i < node->ordNumCols; i++)
	{
			
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->ordColIdx[i])));
	pushJsonbValue(&state, WJB_ELEM, &val);

	}
	pushJsonbValue(&state, WJB_END_ARRAY, NULL);

					}
				
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("ordNumCols");
	key.val.string.val = strdup("ordNumCols");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, Int32GetDatum(node->ordNumCols)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					{
						int i;
						JsonbValue val;
						
	
	key.type = jbvString;
	key.val.string.len = strlen("ordOperators");
	key.val.string.val = strdup("ordOperators");
	pushJsonbValue(&state, WJB_KEY, &key);

	pushJsonbValue(&state, WJB_BEGIN_ARRAY, NULL);
	for (i = 0; i < node->ordNumCols; i++)
	{
			
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, UInt32GetDatum(node->ordOperators[i])));
	pushJsonbValue(&state, WJB_ELEM, &val);

	}
	pushJsonbValue(&state, WJB_END_ARRAY, NULL);

					}
				
					{
						int i;
						JsonbValue val;
						
	
	key.type = jbvString;
	key.val.string.len = strlen("partColIdx");
	key.val.string.val = strdup("partColIdx");
	pushJsonbValue(&state, WJB_KEY, &key);

	pushJsonbValue(&state, WJB_BEGIN_ARRAY, NULL);
	for (i = 0; i < node->partNumCols; i++)
	{
			
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->partColIdx[i])));
	pushJsonbValue(&state, WJB_ELEM, &val);

	}
	pushJsonbValue(&state, WJB_END_ARRAY, NULL);

					}
				
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("partNumCols");
	key.val.string.val = strdup("partNumCols");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, Int32GetDatum(node->partNumCols)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					{
						int i;
						JsonbValue val;
						
	
	key.type = jbvString;
	key.val.string.len = strlen("partOperators");
	key.val.string.val = strdup("partOperators");
	pushJsonbValue(&state, WJB_KEY, &key);

	pushJsonbValue(&state, WJB_BEGIN_ARRAY, NULL);
	for (i = 0; i < node->partNumCols; i++)
	{
			
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, UInt32GetDatum(node->partOperators[i])));
	pushJsonbValue(&state, WJB_ELEM, &val);

	}
	pushJsonbValue(&state, WJB_END_ARRAY, NULL);

					}
				
				
	
	key.type = jbvString;
	key.val.string.len = strlen("plan");
	key.val.string.val = strdup("plan");
	pushJsonbValue(&state, WJB_KEY, &key);

	Plan_ser(&node->plan, state, false);

					
	key.type = jbvString;
	key.val.string.len = strlen("startOffset");
	key.val.string.val = strdup("startOffset");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->startOffset, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("winref");
	key.val.string.val = strdup("winref");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, UInt32GetDatum(node->winref)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *ConvertRowtypeExpr_ser(const ConvertRowtypeExpr *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


					
	key.type = jbvString;
	key.val.string.len = strlen("arg");
	key.val.string.val = strdup("arg");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->arg, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("convertformat");
	key.val.string.val = strdup("convertformat");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->convertformat)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					if(!skip_location)
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("location");
	key.val.string.val = strdup("location");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, Int32GetDatum(node->location)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("resulttype");
	key.val.string.val = strdup("resulttype");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, UInt32GetDatum(node->resulttype)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				
	
	key.type = jbvString;
	key.val.string.len = strlen("xpr");
	key.val.string.val = strdup("xpr");
	pushJsonbValue(&state, WJB_KEY, &key);

	Expr_ser(&node->xpr, state, false);

		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *MergeAppend_ser(const MergeAppend *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


					{
						int i;
						JsonbValue val;
						
	
	key.type = jbvString;
	key.val.string.len = strlen("collations");
	key.val.string.val = strdup("collations");
	pushJsonbValue(&state, WJB_KEY, &key);

	pushJsonbValue(&state, WJB_BEGIN_ARRAY, NULL);
	for (i = 0; i < node->numCols; i++)
	{
			
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, UInt32GetDatum(node->collations[i])));
	pushJsonbValue(&state, WJB_ELEM, &val);

	}
	pushJsonbValue(&state, WJB_END_ARRAY, NULL);

					}
				
					
	key.type = jbvString;
	key.val.string.len = strlen("mergeplans");
	key.val.string.val = strdup("mergeplans");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->mergeplans, state);

					{
						int i;
						JsonbValue val;
						
	
	key.type = jbvString;
	key.val.string.len = strlen("nullsFirst");
	key.val.string.val = strdup("nullsFirst");
	pushJsonbValue(&state, WJB_KEY, &key);

	pushJsonbValue(&state, WJB_BEGIN_ARRAY, NULL);
	for (i = 0; i < node->numCols; i++)
	{
			
	val.type = jbvBool;
	val.val.boolean = node->nullsFirst[i];
	pushJsonbValue(&state, WJB_ELEM, &val);

	}
	pushJsonbValue(&state, WJB_END_ARRAY, NULL);

					}
				
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("numCols");
	key.val.string.val = strdup("numCols");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, Int32GetDatum(node->numCols)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				
	
	key.type = jbvString;
	key.val.string.len = strlen("plan");
	key.val.string.val = strdup("plan");
	pushJsonbValue(&state, WJB_KEY, &key);

	Plan_ser(&node->plan, state, false);

					{
						int i;
						JsonbValue val;
						
	
	key.type = jbvString;
	key.val.string.len = strlen("sortColIdx");
	key.val.string.val = strdup("sortColIdx");
	pushJsonbValue(&state, WJB_KEY, &key);

	pushJsonbValue(&state, WJB_BEGIN_ARRAY, NULL);
	for (i = 0; i < node->numCols; i++)
	{
			
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->sortColIdx[i])));
	pushJsonbValue(&state, WJB_ELEM, &val);

	}
	pushJsonbValue(&state, WJB_END_ARRAY, NULL);

					}
				
					{
						int i;
						JsonbValue val;
						
	
	key.type = jbvString;
	key.val.string.len = strlen("sortOperators");
	key.val.string.val = strdup("sortOperators");
	pushJsonbValue(&state, WJB_KEY, &key);

	pushJsonbValue(&state, WJB_BEGIN_ARRAY, NULL);
	for (i = 0; i < node->numCols; i++)
	{
			
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, UInt32GetDatum(node->sortOperators[i])));
	pushJsonbValue(&state, WJB_ELEM, &val);

	}
	pushJsonbValue(&state, WJB_END_ARRAY, NULL);

					}
				
		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *AccessPriv_ser(const AccessPriv *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


					
	key.type = jbvString;
	key.val.string.len = strlen("cols");
	key.val.string.val = strdup("cols");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->cols, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("priv_name");
	key.val.string.val = strdup("priv_name");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	if (node->priv_name == NULL)
	{
		val.type = jbvNull;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}
	else
	{
		val.type = jbvString;
		val.val.string.len = strlen(node->priv_name);
		val.val.string.val = (char *)node->priv_name;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *CreateOpFamilyStmt_ser(const CreateOpFamilyStmt *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("amname");
	key.val.string.val = strdup("amname");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	if (node->amname == NULL)
	{
		val.type = jbvNull;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}
	else
	{
		val.type = jbvString;
		val.val.string.len = strlen(node->amname);
		val.val.string.val = (char *)node->amname;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("opfamilyname");
	key.val.string.val = strdup("opfamilyname");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->opfamilyname, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *CoalesceExpr_ser(const CoalesceExpr *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


					
	key.type = jbvString;
	key.val.string.len = strlen("args");
	key.val.string.val = strdup("args");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->args, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("coalescecollid");
	key.val.string.val = strdup("coalescecollid");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, UInt32GetDatum(node->coalescecollid)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("coalescetype");
	key.val.string.val = strdup("coalescetype");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, UInt32GetDatum(node->coalescetype)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					if(!skip_location)
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("location");
	key.val.string.val = strdup("location");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, Int32GetDatum(node->location)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				
	
	key.type = jbvString;
	key.val.string.len = strlen("xpr");
	key.val.string.val = strdup("xpr");
	pushJsonbValue(&state, WJB_KEY, &key);

	Expr_ser(&node->xpr, state, false);

		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *DoStmt_ser(const DoStmt *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


					
	key.type = jbvString;
	key.val.string.len = strlen("args");
	key.val.string.val = strdup("args");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->args, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *IndexOnlyScan_ser(const IndexOnlyScan *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("indexid");
	key.val.string.val = strdup("indexid");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, UInt32GetDatum(node->indexid)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("indexorderby");
	key.val.string.val = strdup("indexorderby");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->indexorderby, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("indexorderdir");
	key.val.string.val = strdup("indexorderdir");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->indexorderdir)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("indexqual");
	key.val.string.val = strdup("indexqual");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->indexqual, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("indextlist");
	key.val.string.val = strdup("indextlist");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->indextlist, state);

				
	
	key.type = jbvString;
	key.val.string.len = strlen("scan");
	key.val.string.val = strdup("scan");
	pushJsonbValue(&state, WJB_KEY, &key);

	Scan_ser(&node->scan, state, false);

		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *XmlSerialize_ser(const XmlSerialize *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


					
	key.type = jbvString;
	key.val.string.len = strlen("expr");
	key.val.string.val = strdup("expr");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->expr, state);

					if(!skip_location)
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("location");
	key.val.string.val = strdup("location");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, Int32GetDatum(node->location)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("typeName");
	key.val.string.val = strdup("typeName");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->typeName, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("xmloption");
	key.val.string.val = strdup("xmloption");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->xmloption)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *FunctionParameter_ser(const FunctionParameter *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


					
	key.type = jbvString;
	key.val.string.len = strlen("argType");
	key.val.string.val = strdup("argType");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->argType, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("defexpr");
	key.val.string.val = strdup("defexpr");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->defexpr, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("mode");
	key.val.string.val = strdup("mode");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->mode)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("name");
	key.val.string.val = strdup("name");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	if (node->name == NULL)
	{
		val.type = jbvNull;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}
	else
	{
		val.type = jbvString;
		val.val.string.len = strlen(node->name);
		val.val.string.val = (char *)node->name;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *SetToDefault_ser(const SetToDefault *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("collation");
	key.val.string.val = strdup("collation");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, UInt32GetDatum(node->collation)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					if(!skip_location)
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("location");
	key.val.string.val = strdup("location");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, Int32GetDatum(node->location)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("typeId");
	key.val.string.val = strdup("typeId");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, UInt32GetDatum(node->typeId)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("typeMod");
	key.val.string.val = strdup("typeMod");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, Int32GetDatum(node->typeMod)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				
	
	key.type = jbvString;
	key.val.string.len = strlen("xpr");
	key.val.string.val = strdup("xpr");
	pushJsonbValue(&state, WJB_KEY, &key);

	Expr_ser(&node->xpr, state, false);

		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *JoinExpr_ser(const JoinExpr *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


					
	key.type = jbvString;
	key.val.string.len = strlen("alias");
	key.val.string.val = strdup("alias");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->alias, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("isNatural");
	key.val.string.val = strdup("isNatural");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvBool;
	val.val.boolean = node->isNatural;
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("jointype");
	key.val.string.val = strdup("jointype");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->jointype)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("larg");
	key.val.string.val = strdup("larg");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->larg, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("quals");
	key.val.string.val = strdup("quals");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->quals, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("rarg");
	key.val.string.val = strdup("rarg");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->rarg, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("rtindex");
	key.val.string.val = strdup("rtindex");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, Int32GetDatum(node->rtindex)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("usingClause");
	key.val.string.val = strdup("usingClause");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->usingClause, state);

		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *NullTest_ser(const NullTest *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


					
	key.type = jbvString;
	key.val.string.len = strlen("arg");
	key.val.string.val = strdup("arg");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->arg, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("argisrow");
	key.val.string.val = strdup("argisrow");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvBool;
	val.val.boolean = node->argisrow;
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					if(!skip_location)
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("location");
	key.val.string.val = strdup("location");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, Int32GetDatum(node->location)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("nulltesttype");
	key.val.string.val = strdup("nulltesttype");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->nulltesttype)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				
	
	key.type = jbvString;
	key.val.string.len = strlen("xpr");
	key.val.string.val = strdup("xpr");
	pushJsonbValue(&state, WJB_KEY, &key);

	Expr_ser(&node->xpr, state, false);

		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *NotifyStmt_ser(const NotifyStmt *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("conditionname");
	key.val.string.val = strdup("conditionname");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	if (node->conditionname == NULL)
	{
		val.type = jbvNull;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}
	else
	{
		val.type = jbvString;
		val.val.string.len = strlen(node->conditionname);
		val.val.string.val = (char *)node->conditionname;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("payload");
	key.val.string.val = strdup("payload");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	if (node->payload == NULL)
	{
		val.type = jbvNull;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}
	else
	{
		val.type = jbvString;
		val.val.string.len = strlen(node->payload);
		val.val.string.val = (char *)node->payload;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *AlterTableMoveAllStmt_ser(const AlterTableMoveAllStmt *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("new_tablespacename");
	key.val.string.val = strdup("new_tablespacename");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	if (node->new_tablespacename == NULL)
	{
		val.type = jbvNull;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}
	else
	{
		val.type = jbvString;
		val.val.string.len = strlen(node->new_tablespacename);
		val.val.string.val = (char *)node->new_tablespacename;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("nowait");
	key.val.string.val = strdup("nowait");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvBool;
	val.val.boolean = node->nowait;
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("objtype");
	key.val.string.val = strdup("objtype");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->objtype)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("orig_tablespacename");
	key.val.string.val = strdup("orig_tablespacename");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	if (node->orig_tablespacename == NULL)
	{
		val.type = jbvNull;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}
	else
	{
		val.type = jbvString;
		val.val.string.len = strlen(node->orig_tablespacename);
		val.val.string.val = (char *)node->orig_tablespacename;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("roles");
	key.val.string.val = strdup("roles");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->roles, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *LockingClause_ser(const LockingClause *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


					
	key.type = jbvString;
	key.val.string.len = strlen("lockedRels");
	key.val.string.val = strdup("lockedRels");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->lockedRels, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("strength");
	key.val.string.val = strdup("strength");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->strength)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("waitPolicy");
	key.val.string.val = strdup("waitPolicy");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->waitPolicy)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *DefineStmt_ser(const DefineStmt *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


					
	key.type = jbvString;
	key.val.string.len = strlen("args");
	key.val.string.val = strdup("args");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->args, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("definition");
	key.val.string.val = strdup("definition");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->definition, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("defnames");
	key.val.string.val = strdup("defnames");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->defnames, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("kind");
	key.val.string.val = strdup("kind");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->kind)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("oldstyle");
	key.val.string.val = strdup("oldstyle");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvBool;
	val.val.boolean = node->oldstyle;
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *CreateOpClassItem_ser(const CreateOpClassItem *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


					
	key.type = jbvString;
	key.val.string.len = strlen("args");
	key.val.string.val = strdup("args");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->args, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("class_args");
	key.val.string.val = strdup("class_args");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->class_args, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("itemtype");
	key.val.string.val = strdup("itemtype");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, Int32GetDatum(node->itemtype)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("name");
	key.val.string.val = strdup("name");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->name, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("number");
	key.val.string.val = strdup("number");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, Int32GetDatum(node->number)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("order_family");
	key.val.string.val = strdup("order_family");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->order_family, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("storedtype");
	key.val.string.val = strdup("storedtype");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->storedtype, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *ClusterStmt_ser(const ClusterStmt *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("indexname");
	key.val.string.val = strdup("indexname");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	if (node->indexname == NULL)
	{
		val.type = jbvNull;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}
	else
	{
		val.type = jbvString;
		val.val.string.len = strlen(node->indexname);
		val.val.string.val = (char *)node->indexname;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("relation");
	key.val.string.val = strdup("relation");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->relation, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("verbose");
	key.val.string.val = strdup("verbose");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvBool;
	val.val.boolean = node->verbose;
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *MergeJoin_ser(const MergeJoin *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


				
	
	key.type = jbvString;
	key.val.string.len = strlen("join");
	key.val.string.val = strdup("join");
	pushJsonbValue(&state, WJB_KEY, &key);

	Join_ser(&node->join, state, false);

					{
						int i;
						JsonbValue val;
						
	
	key.type = jbvString;
	key.val.string.len = strlen("mergeCollations");
	key.val.string.val = strdup("mergeCollations");
	pushJsonbValue(&state, WJB_KEY, &key);

	pushJsonbValue(&state, WJB_BEGIN_ARRAY, NULL);
	for (i = 0; i < list_length(node->mergeclauses); i++)
	{
			
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, UInt32GetDatum(node->mergeCollations[i])));
	pushJsonbValue(&state, WJB_ELEM, &val);

	}
	pushJsonbValue(&state, WJB_END_ARRAY, NULL);

					}
				
					{
						int i;
						JsonbValue val;
						
	
	key.type = jbvString;
	key.val.string.len = strlen("mergeFamilies");
	key.val.string.val = strdup("mergeFamilies");
	pushJsonbValue(&state, WJB_KEY, &key);

	pushJsonbValue(&state, WJB_BEGIN_ARRAY, NULL);
	for (i = 0; i < list_length(node->mergeclauses); i++)
	{
			
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, UInt32GetDatum(node->mergeFamilies[i])));
	pushJsonbValue(&state, WJB_ELEM, &val);

	}
	pushJsonbValue(&state, WJB_END_ARRAY, NULL);

					}
				
					{
						int i;
						JsonbValue val;
						
	
	key.type = jbvString;
	key.val.string.len = strlen("mergeNullsFirst");
	key.val.string.val = strdup("mergeNullsFirst");
	pushJsonbValue(&state, WJB_KEY, &key);

	pushJsonbValue(&state, WJB_BEGIN_ARRAY, NULL);
	for (i = 0; i < list_length(node->mergeclauses); i++)
	{
			
	val.type = jbvBool;
	val.val.boolean = node->mergeNullsFirst[i];
	pushJsonbValue(&state, WJB_ELEM, &val);

	}
	pushJsonbValue(&state, WJB_END_ARRAY, NULL);

					}
				
					{
						int i;
						JsonbValue val;
						
	
	key.type = jbvString;
	key.val.string.len = strlen("mergeStrategies");
	key.val.string.val = strdup("mergeStrategies");
	pushJsonbValue(&state, WJB_KEY, &key);

	pushJsonbValue(&state, WJB_BEGIN_ARRAY, NULL);
	for (i = 0; i < list_length(node->mergeclauses); i++)
	{
			
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, Int32GetDatum(node->mergeStrategies[i])));
	pushJsonbValue(&state, WJB_ELEM, &val);

	}
	pushJsonbValue(&state, WJB_END_ARRAY, NULL);

					}
				
					
	key.type = jbvString;
	key.val.string.len = strlen("mergeclauses");
	key.val.string.val = strdup("mergeclauses");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->mergeclauses, state);

		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *SecLabelStmt_ser(const SecLabelStmt *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("label");
	key.val.string.val = strdup("label");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	if (node->label == NULL)
	{
		val.type = jbvNull;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}
	else
	{
		val.type = jbvString;
		val.val.string.len = strlen(node->label);
		val.val.string.val = (char *)node->label;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("objargs");
	key.val.string.val = strdup("objargs");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->objargs, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("objname");
	key.val.string.val = strdup("objname");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->objname, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("objtype");
	key.val.string.val = strdup("objtype");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->objtype)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("provider");
	key.val.string.val = strdup("provider");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	if (node->provider == NULL)
	{
		val.type = jbvNull;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}
	else
	{
		val.type = jbvString;
		val.val.string.len = strlen(node->provider);
		val.val.string.val = (char *)node->provider;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *CaseTestExpr_ser(const CaseTestExpr *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("collation");
	key.val.string.val = strdup("collation");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, UInt32GetDatum(node->collation)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("typeId");
	key.val.string.val = strdup("typeId");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, UInt32GetDatum(node->typeId)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("typeMod");
	key.val.string.val = strdup("typeMod");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, Int32GetDatum(node->typeMod)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				
	
	key.type = jbvString;
	key.val.string.len = strlen("xpr");
	key.val.string.val = strdup("xpr");
	pushJsonbValue(&state, WJB_KEY, &key);

	Expr_ser(&node->xpr, state, false);

		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *TidScan_ser(const TidScan *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


				
	
	key.type = jbvString;
	key.val.string.len = strlen("scan");
	key.val.string.val = strdup("scan");
	pushJsonbValue(&state, WJB_KEY, &key);

	Scan_ser(&node->scan, state, false);

					
	key.type = jbvString;
	key.val.string.len = strlen("tidquals");
	key.val.string.val = strdup("tidquals");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->tidquals, state);

		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *Query_ser(const Query *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("canSetTag");
	key.val.string.val = strdup("canSetTag");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvBool;
	val.val.boolean = node->canSetTag;
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("commandType");
	key.val.string.val = strdup("commandType");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->commandType)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("constraintDeps");
	key.val.string.val = strdup("constraintDeps");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->constraintDeps, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("cteList");
	key.val.string.val = strdup("cteList");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->cteList, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("distinctClause");
	key.val.string.val = strdup("distinctClause");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->distinctClause, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("groupClause");
	key.val.string.val = strdup("groupClause");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->groupClause, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("groupingSets");
	key.val.string.val = strdup("groupingSets");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->groupingSets, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("hasAggs");
	key.val.string.val = strdup("hasAggs");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvBool;
	val.val.boolean = node->hasAggs;
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("hasDistinctOn");
	key.val.string.val = strdup("hasDistinctOn");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvBool;
	val.val.boolean = node->hasDistinctOn;
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("hasForUpdate");
	key.val.string.val = strdup("hasForUpdate");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvBool;
	val.val.boolean = node->hasForUpdate;
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("hasModifyingCTE");
	key.val.string.val = strdup("hasModifyingCTE");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvBool;
	val.val.boolean = node->hasModifyingCTE;
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("hasRecursive");
	key.val.string.val = strdup("hasRecursive");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvBool;
	val.val.boolean = node->hasRecursive;
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("hasRowSecurity");
	key.val.string.val = strdup("hasRowSecurity");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvBool;
	val.val.boolean = node->hasRowSecurity;
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("hasSubLinks");
	key.val.string.val = strdup("hasSubLinks");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvBool;
	val.val.boolean = node->hasSubLinks;
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("hasWindowFuncs");
	key.val.string.val = strdup("hasWindowFuncs");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvBool;
	val.val.boolean = node->hasWindowFuncs;
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("havingQual");
	key.val.string.val = strdup("havingQual");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->havingQual, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("jointree");
	key.val.string.val = strdup("jointree");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->jointree, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("limitCount");
	key.val.string.val = strdup("limitCount");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->limitCount, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("limitOffset");
	key.val.string.val = strdup("limitOffset");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->limitOffset, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("onConflict");
	key.val.string.val = strdup("onConflict");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->onConflict, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("queryId");
	key.val.string.val = strdup("queryId");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, UInt32GetDatum(node->queryId)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("querySource");
	key.val.string.val = strdup("querySource");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->querySource)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("resultRelation");
	key.val.string.val = strdup("resultRelation");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, Int32GetDatum(node->resultRelation)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("returningList");
	key.val.string.val = strdup("returningList");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->returningList, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("rowMarks");
	key.val.string.val = strdup("rowMarks");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->rowMarks, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("rtable");
	key.val.string.val = strdup("rtable");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->rtable, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("setOperations");
	key.val.string.val = strdup("setOperations");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->setOperations, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("sortClause");
	key.val.string.val = strdup("sortClause");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->sortClause, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("targetList");
	key.val.string.val = strdup("targetList");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->targetList, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("utilityStmt");
	key.val.string.val = strdup("utilityStmt");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->utilityStmt, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("windowClause");
	key.val.string.val = strdup("windowClause");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->windowClause, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("withCheckOptions");
	key.val.string.val = strdup("withCheckOptions");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->withCheckOptions, state);

		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *ReplicaIdentityStmt_ser(const ReplicaIdentityStmt *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("identity_type");
	key.val.string.val = strdup("identity_type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->identity_type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("name");
	key.val.string.val = strdup("name");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	if (node->name == NULL)
	{
		val.type = jbvNull;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}
	else
	{
		val.type = jbvString;
		val.val.string.len = strlen(node->name);
		val.val.string.val = (char *)node->name;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *IndexScan_ser(const IndexScan *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("indexid");
	key.val.string.val = strdup("indexid");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, UInt32GetDatum(node->indexid)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("indexorderby");
	key.val.string.val = strdup("indexorderby");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->indexorderby, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("indexorderbyops");
	key.val.string.val = strdup("indexorderbyops");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->indexorderbyops, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("indexorderbyorig");
	key.val.string.val = strdup("indexorderbyorig");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->indexorderbyorig, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("indexorderdir");
	key.val.string.val = strdup("indexorderdir");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->indexorderdir)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("indexqual");
	key.val.string.val = strdup("indexqual");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->indexqual, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("indexqualorig");
	key.val.string.val = strdup("indexqualorig");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->indexqualorig, state);

				
	
	key.type = jbvString;
	key.val.string.len = strlen("scan");
	key.val.string.val = strdup("scan");
	pushJsonbValue(&state, WJB_KEY, &key);

	Scan_ser(&node->scan, state, false);

		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *ValuesScan_ser(const ValuesScan *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


				
	
	key.type = jbvString;
	key.val.string.len = strlen("scan");
	key.val.string.val = strdup("scan");
	pushJsonbValue(&state, WJB_KEY, &key);

	Scan_ser(&node->scan, state, false);

					
	key.type = jbvString;
	key.val.string.len = strlen("values_lists");
	key.val.string.val = strdup("values_lists");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->values_lists, state);

		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *FieldSelect_ser(const FieldSelect *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


					
	key.type = jbvString;
	key.val.string.len = strlen("arg");
	key.val.string.val = strdup("arg");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->arg, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("fieldnum");
	key.val.string.val = strdup("fieldnum");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->fieldnum)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("resultcollid");
	key.val.string.val = strdup("resultcollid");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, UInt32GetDatum(node->resultcollid)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("resulttype");
	key.val.string.val = strdup("resulttype");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, UInt32GetDatum(node->resulttype)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("resulttypmod");
	key.val.string.val = strdup("resulttypmod");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, Int32GetDatum(node->resulttypmod)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				
	
	key.type = jbvString;
	key.val.string.len = strlen("xpr");
	key.val.string.val = strdup("xpr");
	pushJsonbValue(&state, WJB_KEY, &key);

	Expr_ser(&node->xpr, state, false);

		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *OnConflictExpr_ser(const OnConflictExpr *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("action");
	key.val.string.val = strdup("action");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->action)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("arbiterElems");
	key.val.string.val = strdup("arbiterElems");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->arbiterElems, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("arbiterWhere");
	key.val.string.val = strdup("arbiterWhere");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->arbiterWhere, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("constraint");
	key.val.string.val = strdup("constraint");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, UInt32GetDatum(node->constraint)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("exclRelIndex");
	key.val.string.val = strdup("exclRelIndex");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, Int32GetDatum(node->exclRelIndex)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("exclRelTlist");
	key.val.string.val = strdup("exclRelTlist");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->exclRelTlist, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("onConflictSet");
	key.val.string.val = strdup("onConflictSet");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->onConflictSet, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("onConflictWhere");
	key.val.string.val = strdup("onConflictWhere");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->onConflictWhere, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *VariableSetStmt_ser(const VariableSetStmt *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


					
	key.type = jbvString;
	key.val.string.len = strlen("args");
	key.val.string.val = strdup("args");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->args, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("is_local");
	key.val.string.val = strdup("is_local");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvBool;
	val.val.boolean = node->is_local;
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("kind");
	key.val.string.val = strdup("kind");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->kind)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("name");
	key.val.string.val = strdup("name");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	if (node->name == NULL)
	{
		val.type = jbvNull;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}
	else
	{
		val.type = jbvString;
		val.val.string.len = strlen(node->name);
		val.val.string.val = (char *)node->name;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *TargetEntry_ser(const TargetEntry *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


					
	key.type = jbvString;
	key.val.string.len = strlen("expr");
	key.val.string.val = strdup("expr");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->expr, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("resjunk");
	key.val.string.val = strdup("resjunk");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvBool;
	val.val.boolean = node->resjunk;
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("resname");
	key.val.string.val = strdup("resname");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	if (node->resname == NULL)
	{
		val.type = jbvNull;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}
	else
	{
		val.type = jbvString;
		val.val.string.len = strlen(node->resname);
		val.val.string.val = (char *)node->resname;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("resno");
	key.val.string.val = strdup("resno");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->resno)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("resorigcol");
	key.val.string.val = strdup("resorigcol");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->resorigcol)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("resorigtbl");
	key.val.string.val = strdup("resorigtbl");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, UInt32GetDatum(node->resorigtbl)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("ressortgroupref");
	key.val.string.val = strdup("ressortgroupref");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, UInt32GetDatum(node->ressortgroupref)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				
	
	key.type = jbvString;
	key.val.string.len = strlen("xpr");
	key.val.string.val = strdup("xpr");
	pushJsonbValue(&state, WJB_KEY, &key);

	Expr_ser(&node->xpr, state, false);

		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *LockRows_ser(const LockRows *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("epqParam");
	key.val.string.val = strdup("epqParam");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, Int32GetDatum(node->epqParam)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				
	
	key.type = jbvString;
	key.val.string.len = strlen("plan");
	key.val.string.val = strdup("plan");
	pushJsonbValue(&state, WJB_KEY, &key);

	Plan_ser(&node->plan, state, false);

					
	key.type = jbvString;
	key.val.string.len = strlen("rowMarks");
	key.val.string.val = strdup("rowMarks");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->rowMarks, state);

		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *CreateDomainStmt_ser(const CreateDomainStmt *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


					
	key.type = jbvString;
	key.val.string.len = strlen("collClause");
	key.val.string.val = strdup("collClause");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->collClause, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("constraints");
	key.val.string.val = strdup("constraints");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->constraints, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("domainname");
	key.val.string.val = strdup("domainname");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->domainname, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("typeName");
	key.val.string.val = strdup("typeName");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->typeName, state);

		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *RowMarkClause_ser(const RowMarkClause *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("pushedDown");
	key.val.string.val = strdup("pushedDown");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvBool;
	val.val.boolean = node->pushedDown;
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("rti");
	key.val.string.val = strdup("rti");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, UInt32GetDatum(node->rti)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("strength");
	key.val.string.val = strdup("strength");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->strength)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("waitPolicy");
	key.val.string.val = strdup("waitPolicy");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->waitPolicy)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *NestLoopParam_ser(const NestLoopParam *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("paramno");
	key.val.string.val = strdup("paramno");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, Int32GetDatum(node->paramno)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("paramval");
	key.val.string.val = strdup("paramval");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->paramval, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *WindowFunc_ser(const WindowFunc *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


					
	key.type = jbvString;
	key.val.string.len = strlen("aggfilter");
	key.val.string.val = strdup("aggfilter");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->aggfilter, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("args");
	key.val.string.val = strdup("args");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->args, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("inputcollid");
	key.val.string.val = strdup("inputcollid");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, UInt32GetDatum(node->inputcollid)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					if(!skip_location)
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("location");
	key.val.string.val = strdup("location");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, Int32GetDatum(node->location)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("winagg");
	key.val.string.val = strdup("winagg");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvBool;
	val.val.boolean = node->winagg;
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("wincollid");
	key.val.string.val = strdup("wincollid");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, UInt32GetDatum(node->wincollid)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("winfnoid");
	key.val.string.val = strdup("winfnoid");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, UInt32GetDatum(node->winfnoid)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("winref");
	key.val.string.val = strdup("winref");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, UInt32GetDatum(node->winref)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("winstar");
	key.val.string.val = strdup("winstar");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvBool;
	val.val.boolean = node->winstar;
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("wintype");
	key.val.string.val = strdup("wintype");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, UInt32GetDatum(node->wintype)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				
	
	key.type = jbvString;
	key.val.string.len = strlen("xpr");
	key.val.string.val = strdup("xpr");
	pushJsonbValue(&state, WJB_KEY, &key);

	Expr_ser(&node->xpr, state, false);

		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *TableLikeClause_ser(const TableLikeClause *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("options");
	key.val.string.val = strdup("options");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, UInt32GetDatum(node->options)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("relation");
	key.val.string.val = strdup("relation");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->relation, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *Expr_ser(const Expr *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *RecursiveUnion_ser(const RecursiveUnion *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


					{
						int i;
						JsonbValue val;
						
	
	key.type = jbvString;
	key.val.string.len = strlen("dupColIdx");
	key.val.string.val = strdup("dupColIdx");
	pushJsonbValue(&state, WJB_KEY, &key);

	pushJsonbValue(&state, WJB_BEGIN_ARRAY, NULL);
	for (i = 0; i < node->numCols; i++)
	{
			
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->dupColIdx[i])));
	pushJsonbValue(&state, WJB_ELEM, &val);

	}
	pushJsonbValue(&state, WJB_END_ARRAY, NULL);

					}
				
					{
						int i;
						JsonbValue val;
						
	
	key.type = jbvString;
	key.val.string.len = strlen("dupOperators");
	key.val.string.val = strdup("dupOperators");
	pushJsonbValue(&state, WJB_KEY, &key);

	pushJsonbValue(&state, WJB_BEGIN_ARRAY, NULL);
	for (i = 0; i < node->numCols; i++)
	{
			
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, UInt32GetDatum(node->dupOperators[i])));
	pushJsonbValue(&state, WJB_ELEM, &val);

	}
	pushJsonbValue(&state, WJB_END_ARRAY, NULL);

					}
				
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("numCols");
	key.val.string.val = strdup("numCols");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, Int32GetDatum(node->numCols)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("numGroups");
	key.val.string.val = strdup("numGroups");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
#ifdef USE_FLOAT8_BYVAL
	val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int8_numeric, Int64GetDatum(node->numGroups)));
#else
	val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, Int32GetDatum(node->numGroups)));
#endif
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				
	
	key.type = jbvString;
	key.val.string.len = strlen("plan");
	key.val.string.val = strdup("plan");
	pushJsonbValue(&state, WJB_KEY, &key);

	Plan_ser(&node->plan, state, false);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("wtParam");
	key.val.string.val = strdup("wtParam");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, Int32GetDatum(node->wtParam)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *FromExpr_ser(const FromExpr *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


					
	key.type = jbvString;
	key.val.string.len = strlen("fromlist");
	key.val.string.val = strdup("fromlist");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->fromlist, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("quals");
	key.val.string.val = strdup("quals");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->quals, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *Alias_ser(const Alias *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("aliasname");
	key.val.string.val = strdup("aliasname");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	if (node->aliasname == NULL)
	{
		val.type = jbvNull;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}
	else
	{
		val.type = jbvString;
		val.val.string.len = strlen(node->aliasname);
		val.val.string.val = (char *)node->aliasname;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("colnames");
	key.val.string.val = strdup("colnames");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->colnames, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *AlterEnumStmt_ser(const AlterEnumStmt *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("newVal");
	key.val.string.val = strdup("newVal");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	if (node->newVal == NULL)
	{
		val.type = jbvNull;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}
	else
	{
		val.type = jbvString;
		val.val.string.len = strlen(node->newVal);
		val.val.string.val = (char *)node->newVal;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("newValIsAfter");
	key.val.string.val = strdup("newValIsAfter");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvBool;
	val.val.boolean = node->newValIsAfter;
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("newValNeighbor");
	key.val.string.val = strdup("newValNeighbor");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	if (node->newValNeighbor == NULL)
	{
		val.type = jbvNull;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}
	else
	{
		val.type = jbvString;
		val.val.string.len = strlen(node->newValNeighbor);
		val.val.string.val = (char *)node->newValNeighbor;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("skipIfExists");
	key.val.string.val = strdup("skipIfExists");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvBool;
	val.val.boolean = node->skipIfExists;
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("typeName");
	key.val.string.val = strdup("typeName");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->typeName, state);

		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *RangeFunction_ser(const RangeFunction *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


					
	key.type = jbvString;
	key.val.string.len = strlen("alias");
	key.val.string.val = strdup("alias");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->alias, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("coldeflist");
	key.val.string.val = strdup("coldeflist");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->coldeflist, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("functions");
	key.val.string.val = strdup("functions");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->functions, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("is_rowsfrom");
	key.val.string.val = strdup("is_rowsfrom");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvBool;
	val.val.boolean = node->is_rowsfrom;
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("lateral");
	key.val.string.val = strdup("lateral");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvBool;
	val.val.boolean = node->lateral;
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("ordinality");
	key.val.string.val = strdup("ordinality");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvBool;
	val.val.boolean = node->ordinality;
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *CreateTransformStmt_ser(const CreateTransformStmt *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


					
	key.type = jbvString;
	key.val.string.len = strlen("fromsql");
	key.val.string.val = strdup("fromsql");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->fromsql, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("lang");
	key.val.string.val = strdup("lang");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	if (node->lang == NULL)
	{
		val.type = jbvNull;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}
	else
	{
		val.type = jbvString;
		val.val.string.len = strlen(node->lang);
		val.val.string.val = (char *)node->lang;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("replace");
	key.val.string.val = strdup("replace");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvBool;
	val.val.boolean = node->replace;
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("tosql");
	key.val.string.val = strdup("tosql");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->tosql, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("type_name");
	key.val.string.val = strdup("type_name");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->type_name, state);

		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *AlterTableCmd_ser(const AlterTableCmd *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("behavior");
	key.val.string.val = strdup("behavior");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->behavior)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("def");
	key.val.string.val = strdup("def");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->def, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("missing_ok");
	key.val.string.val = strdup("missing_ok");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvBool;
	val.val.boolean = node->missing_ok;
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("name");
	key.val.string.val = strdup("name");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	if (node->name == NULL)
	{
		val.type = jbvNull;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}
	else
	{
		val.type = jbvString;
		val.val.string.len = strlen(node->name);
		val.val.string.val = (char *)node->name;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("newowner");
	key.val.string.val = strdup("newowner");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->newowner, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("subtype");
	key.val.string.val = strdup("subtype");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->subtype)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *GrantStmt_ser(const GrantStmt *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("behavior");
	key.val.string.val = strdup("behavior");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->behavior)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("grant_option");
	key.val.string.val = strdup("grant_option");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvBool;
	val.val.boolean = node->grant_option;
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("grantees");
	key.val.string.val = strdup("grantees");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->grantees, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("is_grant");
	key.val.string.val = strdup("is_grant");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvBool;
	val.val.boolean = node->is_grant;
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("objects");
	key.val.string.val = strdup("objects");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->objects, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("objtype");
	key.val.string.val = strdup("objtype");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->objtype)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("privileges");
	key.val.string.val = strdup("privileges");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->privileges, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("targtype");
	key.val.string.val = strdup("targtype");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->targtype)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *CreateTableSpaceStmt_ser(const CreateTableSpaceStmt *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("location");
	key.val.string.val = strdup("location");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	if (node->location == NULL)
	{
		val.type = jbvNull;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}
	else
	{
		val.type = jbvString;
		val.val.string.len = strlen(node->location);
		val.val.string.val = (char *)node->location;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("options");
	key.val.string.val = strdup("options");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->options, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("owner");
	key.val.string.val = strdup("owner");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->owner, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("tablespacename");
	key.val.string.val = strdup("tablespacename");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	if (node->tablespacename == NULL)
	{
		val.type = jbvNull;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}
	else
	{
		val.type = jbvString;
		val.val.string.len = strlen(node->tablespacename);
		val.val.string.val = (char *)node->tablespacename;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *CreatePolicyStmt_ser(const CreatePolicyStmt *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("cmd_name");
	key.val.string.val = strdup("cmd_name");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	if (node->cmd_name == NULL)
	{
		val.type = jbvNull;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}
	else
	{
		val.type = jbvString;
		val.val.string.len = strlen(node->cmd_name);
		val.val.string.val = (char *)node->cmd_name;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("policy_name");
	key.val.string.val = strdup("policy_name");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	if (node->policy_name == NULL)
	{
		val.type = jbvNull;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}
	else
	{
		val.type = jbvString;
		val.val.string.len = strlen(node->policy_name);
		val.val.string.val = (char *)node->policy_name;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("qual");
	key.val.string.val = strdup("qual");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->qual, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("roles");
	key.val.string.val = strdup("roles");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->roles, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("table");
	key.val.string.val = strdup("table");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->table, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("with_check");
	key.val.string.val = strdup("with_check");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->with_check, state);

		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *GrantRoleStmt_ser(const GrantRoleStmt *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("admin_opt");
	key.val.string.val = strdup("admin_opt");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvBool;
	val.val.boolean = node->admin_opt;
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("behavior");
	key.val.string.val = strdup("behavior");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->behavior)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("granted_roles");
	key.val.string.val = strdup("granted_roles");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->granted_roles, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("grantee_roles");
	key.val.string.val = strdup("grantee_roles");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->grantee_roles, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("grantor");
	key.val.string.val = strdup("grantor");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->grantor, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("is_grant");
	key.val.string.val = strdup("is_grant");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvBool;
	val.val.boolean = node->is_grant;
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *DropRoleStmt_ser(const DropRoleStmt *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("missing_ok");
	key.val.string.val = strdup("missing_ok");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvBool;
	val.val.boolean = node->missing_ok;
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("roles");
	key.val.string.val = strdup("roles");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->roles, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *ViewStmt_ser(const ViewStmt *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


					
	key.type = jbvString;
	key.val.string.len = strlen("aliases");
	key.val.string.val = strdup("aliases");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->aliases, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("options");
	key.val.string.val = strdup("options");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->options, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("query");
	key.val.string.val = strdup("query");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->query, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("replace");
	key.val.string.val = strdup("replace");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvBool;
	val.val.boolean = node->replace;
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("view");
	key.val.string.val = strdup("view");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->view, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("withCheckOption");
	key.val.string.val = strdup("withCheckOption");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->withCheckOption)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *A_Indices_ser(const A_Indices *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


					
	key.type = jbvString;
	key.val.string.len = strlen("lidx");
	key.val.string.val = strdup("lidx");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->lidx, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("uidx");
	key.val.string.val = strdup("uidx");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->uidx, state);

		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *PlanInvalItem_ser(const PlanInvalItem *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("cacheId");
	key.val.string.val = strdup("cacheId");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, Int32GetDatum(node->cacheId)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("hashValue");
	key.val.string.val = strdup("hashValue");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, UInt32GetDatum(node->hashValue)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *Group_ser(const Group *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


					{
						int i;
						JsonbValue val;
						
	
	key.type = jbvString;
	key.val.string.len = strlen("grpColIdx");
	key.val.string.val = strdup("grpColIdx");
	pushJsonbValue(&state, WJB_KEY, &key);

	pushJsonbValue(&state, WJB_BEGIN_ARRAY, NULL);
	for (i = 0; i < node->numCols; i++)
	{
			
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->grpColIdx[i])));
	pushJsonbValue(&state, WJB_ELEM, &val);

	}
	pushJsonbValue(&state, WJB_END_ARRAY, NULL);

					}
				
					{
						int i;
						JsonbValue val;
						
	
	key.type = jbvString;
	key.val.string.len = strlen("grpOperators");
	key.val.string.val = strdup("grpOperators");
	pushJsonbValue(&state, WJB_KEY, &key);

	pushJsonbValue(&state, WJB_BEGIN_ARRAY, NULL);
	for (i = 0; i < node->numCols; i++)
	{
			
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, UInt32GetDatum(node->grpOperators[i])));
	pushJsonbValue(&state, WJB_ELEM, &val);

	}
	pushJsonbValue(&state, WJB_END_ARRAY, NULL);

					}
				
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("numCols");
	key.val.string.val = strdup("numCols");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, Int32GetDatum(node->numCols)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				
	
	key.type = jbvString;
	key.val.string.len = strlen("plan");
	key.val.string.val = strdup("plan");
	pushJsonbValue(&state, WJB_KEY, &key);

	Plan_ser(&node->plan, state, false);

		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *PlanRowMark_ser(const PlanRowMark *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("allMarkTypes");
	key.val.string.val = strdup("allMarkTypes");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, Int32GetDatum(node->allMarkTypes)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("isParent");
	key.val.string.val = strdup("isParent");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvBool;
	val.val.boolean = node->isParent;
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("markType");
	key.val.string.val = strdup("markType");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->markType)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("prti");
	key.val.string.val = strdup("prti");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, UInt32GetDatum(node->prti)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("rowmarkId");
	key.val.string.val = strdup("rowmarkId");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, UInt32GetDatum(node->rowmarkId)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("rti");
	key.val.string.val = strdup("rti");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, UInt32GetDatum(node->rti)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("strength");
	key.val.string.val = strdup("strength");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->strength)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("waitPolicy");
	key.val.string.val = strdup("waitPolicy");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->waitPolicy)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *AlterFdwStmt_ser(const AlterFdwStmt *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("fdwname");
	key.val.string.val = strdup("fdwname");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	if (node->fdwname == NULL)
	{
		val.type = jbvNull;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}
	else
	{
		val.type = jbvString;
		val.val.string.len = strlen(node->fdwname);
		val.val.string.val = (char *)node->fdwname;
		pushJsonbValue(&state, WJB_VALUE, &val);
	}

				}
					
	key.type = jbvString;
	key.val.string.len = strlen("func_options");
	key.val.string.val = strdup("func_options");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->func_options, state);

					
	key.type = jbvString;
	key.val.string.len = strlen("options");
	key.val.string.val = strdup("options");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	node_to_jsonb(node->options, state);

				{
					JsonbValue val;
					
	key.type = jbvString;
	key.val.string.len = strlen("type");
	key.val.string.val = strdup("type");
	pushJsonbValue(&state, WJB_KEY, &key);

					
	val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int2_numeric, Int16GetDatum(node->type)));
	pushJsonbValue(&state, WJB_VALUE, &val);

				}
		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}

	static
	JsonbValue *Material_ser(const Material *node, JsonbParseState *state, bool sub_object)
	{
		JsonbValue	key;
		if (sub_object)
		{
			pushJsonbValue(&state, WJB_BEGIN_OBJECT, NULL);
		}


				
	
	key.type = jbvString;
	key.val.string.len = strlen("plan");
	key.val.string.val = strdup("plan");
	pushJsonbValue(&state, WJB_KEY, &key);

	Plan_ser(&node->plan, state, false);

		if (sub_object)
		{
			return pushJsonbValue(&state, WJB_END_OBJECT, NULL);
		}
		else
		{
			return NULL;
		}
	}


	static
	JsonbValue *List_ser(const void *node, JsonbParseState *state)
	{
		const ListCell *lc;
		pushJsonbValue(&state, WJB_BEGIN_ARRAY, NULL);

		foreach(lc, node)
		{
				
	if (lfirst(lc) == NULL)
	{
		JsonbValue	val;
		val.type = jbvNull;
		pushJsonbValue(&state, WJB_ELEM, &val);
	}
	else if (IsA(lfirst(lc), String) || IsA(lfirst(lc), BitString) || IsA(lfirst(lc), Float) )
	{
		JsonbValue	val;
		val.type = jbvString;
		val.val.string.len = strlen(((Value *)lfirst(lc))->val.str);
		val.val.string.val = ((Value *)lfirst(lc))->val.str;
		pushJsonbValue(&state, WJB_ELEM, &val);
	}
	else if (IsA(lfirst(lc), Integer))
	{
		JsonbValue	val;
		val.type = jbvNumeric;
		val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, Int32GetDatum(((Value *)lfirst(lc))->val.ival)));
		pushJsonbValue(&state, WJB_ELEM, &val);
	}
	else if (IsA(lfirst(lc), Null))
	{
		JsonbValue	val;
		val.type = jbvNull;
		pushJsonbValue(&state, WJB_ELEM, &val);
	}

				else
					node_to_jsonb(lfirst(lc), state);
		}

		return pushJsonbValue(&state, WJB_END_ARRAY, NULL);
	}
	static
	JsonbValue *IntList_ser(const void *node, JsonbParseState *state)
	{
		const ListCell *lc;
		pushJsonbValue(&state, WJB_BEGIN_ARRAY, NULL);

		foreach(lc, node)
		{
				JsonbValue	val;
				val.type = jbvNumeric;
				val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, Int32GetDatum(lfirst_int(lc))));
				pushJsonbValue(&state, WJB_ELEM, &val);
		}

		return pushJsonbValue(&state, WJB_END_ARRAY, NULL);
	}
	static
	JsonbValue *OidList_ser(const void *node, JsonbParseState *state)
	{
		const ListCell *lc;
		pushJsonbValue(&state, WJB_BEGIN_ARRAY, NULL);

		foreach(lc, node)
		{
				JsonbValue	val;
				val.type = jbvNumeric;
				val.val.numeric = DatumGetNumeric(DirectFunctionCall1(int4_numeric, UInt32GetDatum(lfirst_int(lc))));
				pushJsonbValue(&state, WJB_ELEM, &val);
		}

		return pushJsonbValue(&state, WJB_END_ARRAY, NULL);
	}

static
JsonbValue *node_to_jsonb(const void *obj, JsonbParseState *state)
{
	if (obj == NULL) {
		JsonbValue out;
		out.type = jbvNull;
		return pushJsonbValue(&state, WJB_VALUE, &out);
	}
		else if (IsA(obj, List))
		{
			return List_ser(obj, state);
		}
		else if (IsA(obj, IntList))
		{
			return IntList_ser(obj, state);
		}
		else if (IsA(obj, OidList))
		{
			return OidList_ser(obj, state);
		}

	switch (nodeTag(obj))
	{
		case T_RangeTableSample:
			return RangeTableSample_ser(obj, state, true);
		case T_CreateEnumStmt:
			return CreateEnumStmt_ser(obj, state, true);
		case T_SampleScan:
			return SampleScan_ser(obj, state, true);
		case T_SetOperationStmt:
			return SetOperationStmt_ser(obj, state, true);
		case T_AlterTSDictionaryStmt:
			return AlterTSDictionaryStmt_ser(obj, state, true);
		case T_SortGroupClause:
			return SortGroupClause_ser(obj, state, true);
		case T_NamedArgExpr:
			return NamedArgExpr_ser(obj, state, true);
		case T_CaseWhen:
			return CaseWhen_ser(obj, state, true);
		case T_CommentStmt:
			return CommentStmt_ser(obj, state, true);
		case T_VacuumStmt:
			return VacuumStmt_ser(obj, state, true);
		case T_AlterOwnerStmt:
			return AlterOwnerStmt_ser(obj, state, true);
		case T_Unique:
			return Unique_ser(obj, state, true);
		case T_MinMaxExpr:
			return MinMaxExpr_ser(obj, state, true);
		case T_TransactionStmt:
			return TransactionStmt_ser(obj, state, true);
		case T_CreateEventTrigStmt:
			return CreateEventTrigStmt_ser(obj, state, true);
		case T_ArrayRef:
			return ArrayRef_ser(obj, state, true);
		case T_AlterExtensionStmt:
			return AlterExtensionStmt_ser(obj, state, true);
		case T_CreateRoleStmt:
			return CreateRoleStmt_ser(obj, state, true);
		case T_AlterOpFamilyStmt:
			return AlterOpFamilyStmt_ser(obj, state, true);
		case T_LockStmt:
			return LockStmt_ser(obj, state, true);
		case T_AlterTableStmt:
			return AlterTableStmt_ser(obj, state, true);
		case T_CreateSchemaStmt:
			return CreateSchemaStmt_ser(obj, state, true);
		case T_ClosePortalStmt:
			return ClosePortalStmt_ser(obj, state, true);
		case T_RelabelType:
			return RelabelType_ser(obj, state, true);
		case T_FunctionScan:
			return FunctionScan_ser(obj, state, true);
		case T_AlterSeqStmt:
			return AlterSeqStmt_ser(obj, state, true);
		case T_SubPlan:
			return SubPlan_ser(obj, state, true);
		case T_ScalarArrayOpExpr:
			return ScalarArrayOpExpr_ser(obj, state, true);
		case T_RoleSpec:
			return RoleSpec_ser(obj, state, true);
		case T_InlineCodeBlock:
			return InlineCodeBlock_ser(obj, state, true);
		case T_VariableShowStmt:
			return VariableShowStmt_ser(obj, state, true);
		case T_ImportForeignSchemaStmt:
			return ImportForeignSchemaStmt_ser(obj, state, true);
		case T_AlterForeignServerStmt:
			return AlterForeignServerStmt_ser(obj, state, true);
		case T_ModifyTable:
			return ModifyTable_ser(obj, state, true);
		case T_CheckPointStmt:
			return CheckPointStmt_ser(obj, state, true);
		case T_RangeVar:
			return RangeVar_ser(obj, state, true);
		case T_AlterExtensionContentsStmt:
			return AlterExtensionContentsStmt_ser(obj, state, true);
		case T_RangeTblRef:
			return RangeTblRef_ser(obj, state, true);
		case T_CreateOpClassStmt:
			return CreateOpClassStmt_ser(obj, state, true);
		case T_LoadStmt:
			return LoadStmt_ser(obj, state, true);
		case T_ColumnRef:
			return ColumnRef_ser(obj, state, true);
		case T_CreateTrigStmt:
			return CreateTrigStmt_ser(obj, state, true);
		case T_ConstraintsSetStmt:
			return ConstraintsSetStmt_ser(obj, state, true);
		case T_Var:
			return Var_ser(obj, state, true);
		case T_BitmapIndexScan:
			return BitmapIndexScan_ser(obj, state, true);
		case T_Plan:
			return Plan_ser(obj, state, true);
		case T_CreateStmt:
			return CreateStmt_ser(obj, state, true);
		case T_InferClause:
			return InferClause_ser(obj, state, true);
		case T_Param:
			return Param_ser(obj, state, true);
		case T_ExecuteStmt:
			return ExecuteStmt_ser(obj, state, true);
		case T_DropdbStmt:
			return DropdbStmt_ser(obj, state, true);
		case T_FetchStmt:
			return FetchStmt_ser(obj, state, true);
		case T_HashJoin:
			return HashJoin_ser(obj, state, true);
		case T_ColumnDef:
			return ColumnDef_ser(obj, state, true);
		case T_AlterRoleSetStmt:
			return AlterRoleSetStmt_ser(obj, state, true);
		case T_Result:
			return Result_ser(obj, state, true);
		case T_PrepareStmt:
			return PrepareStmt_ser(obj, state, true);
		case T_AlterDomainStmt:
			return AlterDomainStmt_ser(obj, state, true);
		case T_CompositeTypeStmt:
			return CompositeTypeStmt_ser(obj, state, true);
		case T_CustomScan:
			return CustomScan_ser(obj, state, true);
		case T_Agg:
			return Agg_ser(obj, state, true);
		case T_AlterObjectSchemaStmt:
			return AlterObjectSchemaStmt_ser(obj, state, true);
		case T_AlterEventTrigStmt:
			return AlterEventTrigStmt_ser(obj, state, true);
		case T_AlterDefaultPrivilegesStmt:
			return AlterDefaultPrivilegesStmt_ser(obj, state, true);
		case T_PlannedStmt:
			return PlannedStmt_ser(obj, state, true);
		case T_Aggref:
			return Aggref_ser(obj, state, true);
		case T_SubqueryScan:
			return SubqueryScan_ser(obj, state, true);
		case T_CreateFunctionStmt:
			return CreateFunctionStmt_ser(obj, state, true);
		case T_CreateCastStmt:
			return CreateCastStmt_ser(obj, state, true);
		case T_CteScan:
			return CteScan_ser(obj, state, true);
		case T_IndexElem:
			return IndexElem_ser(obj, state, true);
		case T_CreateFdwStmt:
			return CreateFdwStmt_ser(obj, state, true);
		case T_NestLoop:
			return NestLoop_ser(obj, state, true);
		case T_TypeCast:
			return TypeCast_ser(obj, state, true);
		case T_CoerceToDomainValue:
			return CoerceToDomainValue_ser(obj, state, true);
		case T_InsertStmt:
			return InsertStmt_ser(obj, state, true);
		case T_SortBy:
			return SortBy_ser(obj, state, true);
		case T_ReassignOwnedStmt:
			return ReassignOwnedStmt_ser(obj, state, true);
		case T_TypeName:
			return TypeName_ser(obj, state, true);
		case T_Constraint:
			return Constraint_ser(obj, state, true);
		case T_OnConflictClause:
			return OnConflictClause_ser(obj, state, true);
		case T_AlterPolicyStmt:
			return AlterPolicyStmt_ser(obj, state, true);
		case T_GroupingFunc:
			return GroupingFunc_ser(obj, state, true);
		case T_SelectStmt:
			return SelectStmt_ser(obj, state, true);
		case T_CopyStmt:
			return CopyStmt_ser(obj, state, true);
		case T_ArrayExpr:
			return ArrayExpr_ser(obj, state, true);
		case T_InferenceElem:
			return InferenceElem_ser(obj, state, true);
		case T_BitmapOr:
			return BitmapOr_ser(obj, state, true);
		case T_FuncExpr:
			return FuncExpr_ser(obj, state, true);
		case T_AlterUserMappingStmt:
			return AlterUserMappingStmt_ser(obj, state, true);
		case T_SetOp:
			return SetOp_ser(obj, state, true);
		case T_AlterTSConfigurationStmt:
			return AlterTSConfigurationStmt_ser(obj, state, true);
		case T_AlterRoleStmt:
			return AlterRoleStmt_ser(obj, state, true);
		case T_Sort:
			return Sort_ser(obj, state, true);
		case T_DiscardStmt:
			return DiscardStmt_ser(obj, state, true);
		case T_CreateForeignServerStmt:
			return CreateForeignServerStmt_ser(obj, state, true);
		case T_CaseExpr:
			return CaseExpr_ser(obj, state, true);
		case T_CreateExtensionStmt:
			return CreateExtensionStmt_ser(obj, state, true);
		case T_CommonTableExpr:
			return CommonTableExpr_ser(obj, state, true);
		case T_RenameStmt:
			return RenameStmt_ser(obj, state, true);
		case T_A_Star:
			return A_Star_ser(obj, state, true);
		case T_UnlistenStmt:
			return UnlistenStmt_ser(obj, state, true);
		case T_GroupingSet:
			return GroupingSet_ser(obj, state, true);
		case T_BoolExpr:
			return BoolExpr_ser(obj, state, true);
		case T_BitmapAnd:
			return BitmapAnd_ser(obj, state, true);
		case T_RowExpr:
			return RowExpr_ser(obj, state, true);
		case T_UpdateStmt:
			return UpdateStmt_ser(obj, state, true);
		case T_CollateExpr:
			return CollateExpr_ser(obj, state, true);
		case T_DropTableSpaceStmt:
			return DropTableSpaceStmt_ser(obj, state, true);
		case T_DeallocateStmt:
			return DeallocateStmt_ser(obj, state, true);
		case T_CreatedbStmt:
			return CreatedbStmt_ser(obj, state, true);
		case T_Hash:
			return Hash_ser(obj, state, true);
		case T_CurrentOfExpr:
			return CurrentOfExpr_ser(obj, state, true);
		case T_WindowDef:
			return WindowDef_ser(obj, state, true);
		case T_AlterFunctionStmt:
			return AlterFunctionStmt_ser(obj, state, true);
		case T_ExplainStmt:
			return ExplainStmt_ser(obj, state, true);
		case T_AlterDatabaseStmt:
			return AlterDatabaseStmt_ser(obj, state, true);
		case T_ParamRef:
			return ParamRef_ser(obj, state, true);
		case T_CreateForeignTableStmt:
			return CreateForeignTableStmt_ser(obj, state, true);
		case T_BitmapHeapScan:
			return BitmapHeapScan_ser(obj, state, true);
		case T_Scan:
			return Scan_ser(obj, state, true);
		case T_AlterTableSpaceOptionsStmt:
			return AlterTableSpaceOptionsStmt_ser(obj, state, true);
		case T_FuncCall:
			return FuncCall_ser(obj, state, true);
		case T_Join:
			return Join_ser(obj, state, true);
		case T_ReindexStmt:
			return ReindexStmt_ser(obj, state, true);
		case T_WithClause:
			return WithClause_ser(obj, state, true);
		case T_RangeSubselect:
			return RangeSubselect_ser(obj, state, true);
		case T_CreatePLangStmt:
			return CreatePLangStmt_ser(obj, state, true);
		case T_Const:
			return Const_ser(obj, state, true);
		case T_XmlExpr:
			return XmlExpr_ser(obj, state, true);
		case T_FuncWithArgs:
			return FuncWithArgs_ser(obj, state, true);
		case T_CollateClause:
			return CollateClause_ser(obj, state, true);
		case T_CreateUserMappingStmt:
			return CreateUserMappingStmt_ser(obj, state, true);
		case T_ListenStmt:
			return ListenStmt_ser(obj, state, true);
		case T_RefreshMatViewStmt:
			return RefreshMatViewStmt_ser(obj, state, true);
		case T_DeclareCursorStmt:
			return DeclareCursorStmt_ser(obj, state, true);
		case T_CreateConversionStmt:
			return CreateConversionStmt_ser(obj, state, true);
		case T_Value:
			return Value_ser(obj, state, true);
		case T_DropStmt:
			return DropStmt_ser(obj, state, true);
		case T_Limit:
			return Limit_ser(obj, state, true);
		case T_IntoClause:
			return IntoClause_ser(obj, state, true);
		case T_AlternativeSubPlan:
			return AlternativeSubPlan_ser(obj, state, true);
		case T_ForeignScan:
			return ForeignScan_ser(obj, state, true);
		case T_A_Const:
			return A_Const_ser(obj, state, true);
		case T_AlterSystemStmt:
			return AlterSystemStmt_ser(obj, state, true);
		case T_ResTarget:
			return ResTarget_ser(obj, state, true);
		case T_RangeTblEntry:
			return RangeTblEntry_ser(obj, state, true);
		case T_A_Indirection:
			return A_Indirection_ser(obj, state, true);
		case T_Append:
			return Append_ser(obj, state, true);
		case T_TableSampleClause:
			return TableSampleClause_ser(obj, state, true);
		case T_BooleanTest:
			return BooleanTest_ser(obj, state, true);
		case T_ArrayCoerceExpr:
			return ArrayCoerceExpr_ser(obj, state, true);
		case T_WithCheckOption:
			return WithCheckOption_ser(obj, state, true);
		case T_RowCompareExpr:
			return RowCompareExpr_ser(obj, state, true);
		case T_MultiAssignRef:
			return MultiAssignRef_ser(obj, state, true);
		case T_TruncateStmt:
			return TruncateStmt_ser(obj, state, true);
		case T_CoerceToDomain:
			return CoerceToDomain_ser(obj, state, true);
		case T_WindowClause:
			return WindowClause_ser(obj, state, true);
		case T_DefElem:
			return DefElem_ser(obj, state, true);
		case T_DropUserMappingStmt:
			return DropUserMappingStmt_ser(obj, state, true);
		case T_A_ArrayExpr:
			return A_ArrayExpr_ser(obj, state, true);
		case T_CreateTableAsStmt:
			return CreateTableAsStmt_ser(obj, state, true);
		case T_OpExpr:
			return OpExpr_ser(obj, state, true);
		case T_FieldStore:
			return FieldStore_ser(obj, state, true);
		case T_CoerceViaIO:
			return CoerceViaIO_ser(obj, state, true);
		case T_SubLink:
			return SubLink_ser(obj, state, true);
		case T_WorkTableScan:
			return WorkTableScan_ser(obj, state, true);
		case T_A_Expr:
			return A_Expr_ser(obj, state, true);
		case T_DropOwnedStmt:
			return DropOwnedStmt_ser(obj, state, true);
		case T_AlterDatabaseSetStmt:
			return AlterDatabaseSetStmt_ser(obj, state, true);
		case T_CreateRangeStmt:
			return CreateRangeStmt_ser(obj, state, true);
		case T_DeleteStmt:
			return DeleteStmt_ser(obj, state, true);
		case T_CreateSeqStmt:
			return CreateSeqStmt_ser(obj, state, true);
		case T_RuleStmt:
			return RuleStmt_ser(obj, state, true);
		case T_RangeTblFunction:
			return RangeTblFunction_ser(obj, state, true);
		case T_IndexStmt:
			return IndexStmt_ser(obj, state, true);
		case T_WindowAgg:
			return WindowAgg_ser(obj, state, true);
		case T_ConvertRowtypeExpr:
			return ConvertRowtypeExpr_ser(obj, state, true);
		case T_MergeAppend:
			return MergeAppend_ser(obj, state, true);
		case T_AccessPriv:
			return AccessPriv_ser(obj, state, true);
		case T_CreateOpFamilyStmt:
			return CreateOpFamilyStmt_ser(obj, state, true);
		case T_CoalesceExpr:
			return CoalesceExpr_ser(obj, state, true);
		case T_DoStmt:
			return DoStmt_ser(obj, state, true);
		case T_IndexOnlyScan:
			return IndexOnlyScan_ser(obj, state, true);
		case T_XmlSerialize:
			return XmlSerialize_ser(obj, state, true);
		case T_FunctionParameter:
			return FunctionParameter_ser(obj, state, true);
		case T_SetToDefault:
			return SetToDefault_ser(obj, state, true);
		case T_JoinExpr:
			return JoinExpr_ser(obj, state, true);
		case T_NullTest:
			return NullTest_ser(obj, state, true);
		case T_NotifyStmt:
			return NotifyStmt_ser(obj, state, true);
		case T_AlterTableMoveAllStmt:
			return AlterTableMoveAllStmt_ser(obj, state, true);
		case T_LockingClause:
			return LockingClause_ser(obj, state, true);
		case T_DefineStmt:
			return DefineStmt_ser(obj, state, true);
		case T_CreateOpClassItem:
			return CreateOpClassItem_ser(obj, state, true);
		case T_ClusterStmt:
			return ClusterStmt_ser(obj, state, true);
		case T_MergeJoin:
			return MergeJoin_ser(obj, state, true);
		case T_SecLabelStmt:
			return SecLabelStmt_ser(obj, state, true);
		case T_CaseTestExpr:
			return CaseTestExpr_ser(obj, state, true);
		case T_TidScan:
			return TidScan_ser(obj, state, true);
		case T_Query:
			return Query_ser(obj, state, true);
		case T_ReplicaIdentityStmt:
			return ReplicaIdentityStmt_ser(obj, state, true);
		case T_IndexScan:
			return IndexScan_ser(obj, state, true);
		case T_ValuesScan:
			return ValuesScan_ser(obj, state, true);
		case T_FieldSelect:
			return FieldSelect_ser(obj, state, true);
		case T_OnConflictExpr:
			return OnConflictExpr_ser(obj, state, true);
		case T_VariableSetStmt:
			return VariableSetStmt_ser(obj, state, true);
		case T_TargetEntry:
			return TargetEntry_ser(obj, state, true);
		case T_LockRows:
			return LockRows_ser(obj, state, true);
		case T_CreateDomainStmt:
			return CreateDomainStmt_ser(obj, state, true);
		case T_RowMarkClause:
			return RowMarkClause_ser(obj, state, true);
		case T_NestLoopParam:
			return NestLoopParam_ser(obj, state, true);
		case T_WindowFunc:
			return WindowFunc_ser(obj, state, true);
		case T_TableLikeClause:
			return TableLikeClause_ser(obj, state, true);
		case T_Expr:
			return Expr_ser(obj, state, true);
		case T_RecursiveUnion:
			return RecursiveUnion_ser(obj, state, true);
		case T_FromExpr:
			return FromExpr_ser(obj, state, true);
		case T_Alias:
			return Alias_ser(obj, state, true);
		case T_AlterEnumStmt:
			return AlterEnumStmt_ser(obj, state, true);
		case T_RangeFunction:
			return RangeFunction_ser(obj, state, true);
		case T_CreateTransformStmt:
			return CreateTransformStmt_ser(obj, state, true);
		case T_AlterTableCmd:
			return AlterTableCmd_ser(obj, state, true);
		case T_GrantStmt:
			return GrantStmt_ser(obj, state, true);
		case T_CreateTableSpaceStmt:
			return CreateTableSpaceStmt_ser(obj, state, true);
		case T_CreatePolicyStmt:
			return CreatePolicyStmt_ser(obj, state, true);
		case T_GrantRoleStmt:
			return GrantRoleStmt_ser(obj, state, true);
		case T_DropRoleStmt:
			return DropRoleStmt_ser(obj, state, true);
		case T_ViewStmt:
			return ViewStmt_ser(obj, state, true);
		case T_A_Indices:
			return A_Indices_ser(obj, state, true);
		case T_PlanInvalItem:
			return PlanInvalItem_ser(obj, state, true);
		case T_Group:
			return Group_ser(obj, state, true);
		case T_PlanRowMark:
			return PlanRowMark_ser(obj, state, true);
		case T_AlterFdwStmt:
			return AlterFdwStmt_ser(obj, state, true);
		case T_Material:
			return Material_ser(obj, state, true);
		case T_SeqScan:
			return Scan_ser(obj, state, true);
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