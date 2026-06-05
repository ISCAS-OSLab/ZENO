// SPDX-License-Identifier: Mulan PSL v2
/*
 * Copyright (c) 2021 - 2026 The HEDB Project.
 * Copyright (c) 2026 The ZENO Project.
 */

#include "delete_queue.hpp"
#include <extension.hpp>
#include <interface.hpp>

using namespace std;

#ifdef __cplusplus
extern "C" {
#endif
#include <access/xact.h>
#include <utils/rel.h>
#ifdef USE_LOCAL_STORE_CLEAR
#include <executor/executor.h>
#endif
#include <commands/trigger.h>

PG_FUNCTION_INFO_V1(kv_xact_init);
PG_FUNCTION_INFO_V1(kv_replay_log);
PG_FUNCTION_INFO_V1(generic_delete_trigger);
#ifdef USE_LOCAL_STORE_CLEAR
void _PG_init(void);
void _PG_fini(void);
#endif

#ifdef __cplusplus
}
#endif

#ifdef USE_KVMAP_PARTITION
static RID relation_partition_id(Relation rel)
{
    RID partition = (RID)RelationGetRelid(rel) & ((1ULL << KVMAP_PARTITION_BITS) - 1ULL);
    if (partition == RID_LOCAL_PARTITION) {
        elog(ERROR, "relation partition conflicts with local RID partition: relid=%u partition=%llu",
             RelationGetRelid(rel), (unsigned long long)partition);
    }
    return partition;
}

static bool rid_in_partition(RID id, RID partition)
{
    return !islocal(id) && rid_partition(id) == partition;
}
#endif

static bool init = false;
static Oid kv_int4_oid;
static Oid kv_float4_oid;
static Oid kv_text_oid;
static Oid kv_timestamp_oid;
#ifdef USE_LOCAL_STORE_CLEAR
static ExecutorStart_hook_type prev_ExecutorStart = NULL;
static ExecutorEnd_hook_type prev_ExecutorEnd = NULL;
static int executor_depth = 0;
static bool clear_outer_executor = false;
static bool xact_callback_registered = false;
static void kv_xact_commit_hook(XactEvent event, void *arg);
#endif
#ifdef USE_LAZY_DELETE
DeleteQueue delete_set_int4;
DeleteQueue delete_set_float4;
DeleteQueue delete_set_text;
DeleteQueue delete_set_timestamp;
#endif

#ifdef USE_LOCAL_STORE_CLEAR
static void send_local_clear_req()
{
    auto req = SingleArgRequest(CMD_LOCAL_CLEAR);
    TEEInvoker* invoker = TEEInvoker::getInstance();
    int error = invoker->sendRequest(&req);
    if (error)
        print_error("%s %d", __func__, error);
}

static void kvdb_ExecutorStart(QueryDesc *queryDesc, int eflags)
{
    if (executor_depth == 0) {
        clear_outer_executor = queryDesc && queryDesc->operation == CMD_SELECT;
    }

    executor_depth++;
    if (prev_ExecutorStart)
        prev_ExecutorStart(queryDesc, eflags);
    else
        standard_ExecutorStart(queryDesc, eflags);
}

static void kvdb_ExecutorEnd(QueryDesc *queryDesc)
{
    if (prev_ExecutorEnd)
        prev_ExecutorEnd(queryDesc);
    else
        standard_ExecutorEnd(queryDesc);

    executor_depth--;
    if (executor_depth == 0 && clear_outer_executor) {
        send_local_clear_req();
        clear_outer_executor = false;
    }
}

static void register_xact_callback_once()
{
    if (!xact_callback_registered) {
        RegisterXactCallback(kv_xact_commit_hook, NULL);
        xact_callback_registered = true;
    }
}

void _PG_init(void)
{
    register_xact_callback_once();

    prev_ExecutorStart = ExecutorStart_hook;
    ExecutorStart_hook = kvdb_ExecutorStart;

    prev_ExecutorEnd = ExecutorEnd_hook;
    ExecutorEnd_hook = kvdb_ExecutorEnd;
}

void _PG_fini(void)
{
    ExecutorStart_hook = prev_ExecutorStart;
    ExecutorEnd_hook = prev_ExecutorEnd;
}
#endif

void init_oids() {
	if (likely(init))	return;

    kv_int4_oid = TypenameGetTypid("kv_int4");
    if (!OidIsValid(kv_int4_oid))
        elog(ERROR, "Type kv_int4_oid does not exist");

    kv_float4_oid = TypenameGetTypid("kv_float4");
    if (!OidIsValid(kv_float4_oid))
        elog(ERROR, "Type kv_float4_oid does not exist");

    kv_text_oid = TypenameGetTypid("kv_text");
    if (!OidIsValid(kv_text_oid))
        elog(ERROR, "Type kv_text_oid does not exist");

    kv_timestamp_oid = TypenameGetTypid("kv_timestamp");
    if (!OidIsValid(kv_timestamp_oid))
        elog(ERROR, "Type kv_timestamp_oid does not exist");

    init = true;
}

#ifdef USE_LAZY_DELETE
void send_delete_req(Oid type, RID old)
{
#ifdef USE_KVMAP_PARTITION
    (void)type;
    (void)old;
    return;
#else
    if (islocal(old)) {
        return;
    }

    if (type == kv_float4_oid) {
        delete_set_float4.enqueue(old);
    } else if (type == kv_text_oid) {
        delete_set_text.enqueue(old);
    } else if (type == kv_timestamp_oid) {
        delete_set_timestamp.enqueue(old);
    } else {
        delete_set_int4.enqueue(old);
    }
#endif
}
#endif


RID send_promote_req(Oid type, RID local, RID partition = RID_DEFAULT_PARTITION)
{
    RID res;
    RID target = INVALID_RID;

    int req_type = CMD_PROMOTE_INT;
	if (type == kv_float4_oid) {
        req_type = CMD_PROMOTE_FLOAT;
#if defined(USE_LAZY_DELETE) && !defined(USE_KVMAP_PARTITION)
        target = delete_set_float4.dequeue();
#endif
    }
	else if (type == kv_text_oid) {
        req_type = CMD_PROMOTE_TEXT;
#if defined(USE_LAZY_DELETE) && !defined(USE_KVMAP_PARTITION)
        target = delete_set_text.dequeue();
#endif
    }
	else if (type == kv_timestamp_oid) {
        req_type = CMD_PROMOTE_TIMESTAMP;
#if defined(USE_LAZY_DELETE) && !defined(USE_KVMAP_PARTITION)
        target = delete_set_timestamp.dequeue();
#endif
    }
	else {
        assert(type == kv_int4_oid);
#if defined(USE_LAZY_DELETE) && !defined(USE_KVMAP_PARTITION)
        target = delete_set_int4.dequeue();
#endif
    }

    auto req = PromoteRequest(req_type, local, target, partition, &res);
    TEEInvoker* invoker = TEEInvoker::getInstance();
    int error = invoker->sendRequest(&req);
    if (error)
        print_error("%s %d", __func__, error);
    return res;
}

void kv_xact_commit_hook(XactEvent event, void *arg)
{
#ifdef USE_LAZY_DELETE
    if (event == XACT_EVENT_COMMIT) {
        delete_set_int4.set_reuse();
        delete_set_float4.set_reuse();
        delete_set_text.set_reuse();
        delete_set_timestamp.set_reuse();
    } else if (event == XACT_EVENT_ABORT) {
        delete_set_int4.discard_pending();
        delete_set_float4.discard_pending();
        delete_set_text.discard_pending();
        delete_set_timestamp.discard_pending();
    }
#endif
#ifdef USE_LOCAL_STORE_CLEAR
    if (event == XACT_EVENT_ABORT) {
        executor_depth = 0;
        clear_outer_executor = false;
        send_local_clear_req();
    }
#endif
}

Datum kv_xact_init(PG_FUNCTION_ARGS)
{
#ifdef USE_LOCAL_STORE_CLEAR
    register_xact_callback_once();
#else
    RegisterXactCallback(kv_xact_commit_hook, NULL);
#endif
    PG_RETURN_VOID();
}

Datum kv_replay_log(PG_FUNCTION_ARGS)
{
    PG_RETURN_DATUM(0);
}

Datum generic_delete_trigger(PG_FUNCTION_ARGS)
{
    if (!CALLED_AS_TRIGGER(fcinfo))
        elog(ERROR, "generic_delete_trigger: not fired by trigger manager");

    init_oids();
    TriggerData *trigdata = (TriggerData *) fcinfo->context;
    Relation rel = trigdata->tg_relation;
    TupleDesc tupleDesc = RelationGetDescr(rel);
#ifdef USE_KVMAP_PARTITION
    RID partition = relation_partition_id(rel);
#endif
#ifdef USE_LAZY_DELETE
    HeapTuple oldtup = NULL;
#endif
    HeapTuple newtup = NULL;
    HeapTuple rettuple;

    if (TRIGGER_FIRED_BY_DELETE(trigdata->tg_event))
    {
        assert(TRIGGER_FIRED_AFTER(trigdata->tg_event));
        rettuple = NULL;
#ifdef USE_LAZY_DELETE
        oldtup = trigdata->tg_trigtuple;

        for (int i = 0; i < tupleDesc->natts; i++)
        {
            if (tupleDesc->attrs[i].attisdropped)
                continue;

            Oid atttype = tupleDesc->attrs[i].atttypid;
            if (atttype != kv_int4_oid && atttype != kv_float4_oid && atttype != kv_text_oid && atttype != kv_timestamp_oid)	continue;

            bool old_isnull;
            Datum old_datum = heap_getattr(oldtup, i + 1, tupleDesc, &old_isnull);

            if (old_isnull) continue;
            assert(TRIGGER_FIRED_BY_DELETE(trigdata->tg_event));
            uint64 old_value = DatumGetUInt64(old_datum);
            send_delete_req(atttype, old_value);
        }
#endif
        return PointerGetDatum(rettuple);
    }
    else if (TRIGGER_FIRED_BY_INSERT(trigdata->tg_event))
    {
        assert(TRIGGER_FIRED_BEFORE(trigdata->tg_event));
        rettuple = trigdata->tg_trigtuple;
        newtup = trigdata->tg_trigtuple;

        Datum repl_values[tupleDesc->natts];
        bool repl_isnull[tupleDesc->natts];
        bool repl_cols[tupleDesc->natts];

        for (int i = 0; i < tupleDesc->natts; i++) {
            repl_cols[i] = false;
            repl_isnull[i] = false;
            Oid atttype = tupleDesc->attrs[i].atttypid;
            if (atttype != kv_int4_oid && atttype != kv_float4_oid && atttype != kv_text_oid && atttype != kv_timestamp_oid) {
                continue;
            }

            Datum new_datum = heap_getattr(newtup, i + 1, tupleDesc, &(repl_isnull[i]));
            auto local = DatumGetUInt64(new_datum);
            if (repl_isnull[i]) {
                continue;
            }
#ifdef USE_KVMAP_PARTITION
            if (rid_in_partition(local, partition)) continue;
            auto global = send_promote_req(atttype, local, partition);
#else
            if (!islocal(local))    continue;
            auto global = send_promote_req(atttype, local);
#endif
            Datum new_value = UInt64GetDatum(global);

            repl_values[i] = new_value;
            repl_cols[i] = true;
        }
        newtup = heap_modify_tuple(newtup, tupleDesc, repl_values, repl_isnull, repl_cols);

        return PointerGetDatum(newtup);
    }
    else if (TRIGGER_FIRED_BY_UPDATE(trigdata->tg_event))
    {
        assert(TRIGGER_FIRED_BEFORE(trigdata->tg_event));
        rettuple = trigdata->tg_newtuple;
#ifdef USE_LAZY_DELETE
        oldtup = trigdata->tg_trigtuple;
#endif
        newtup = trigdata->tg_newtuple;
        Datum repl_values[tupleDesc->natts];
        bool repl_isnull[tupleDesc->natts];
        bool repl_cols[tupleDesc->natts];

        bool modified = false;

        for (int i = 0; i < tupleDesc->natts; i++) {
            repl_cols[i] = false;
            repl_isnull[i] = false;
            if (tupleDesc->attrs[i].attisdropped)
                continue;

            Oid atttype = tupleDesc->attrs[i].atttypid;
            if (atttype != kv_int4_oid && atttype != kv_float4_oid && atttype != kv_text_oid && atttype != kv_timestamp_oid)	continue;
#ifdef USE_LAZY_DELETE
            bool old_isnull;
            Datum old_datum = heap_getattr(oldtup, i + 1, tupleDesc, &old_isnull);
#endif
            Datum new_datum = (newtup != NULL) ? heap_getattr(newtup, i + 1, tupleDesc, &(repl_isnull[i])) : (Datum) 0;
            if (repl_isnull[i]) {
                continue;
            }

            auto local = DatumGetUInt64(new_datum);
#ifdef USE_KVMAP_PARTITION
            if (rid_in_partition(local, partition)) continue;
            auto global = send_promote_req(atttype, local, partition);
#else
            if (!(islocal(local)))  continue;

            auto global = send_promote_req(atttype, local);
#endif
            Datum new_value = UInt64GetDatum(global);

            repl_values[i] = new_value;
            repl_cols[i] = true;
            modified = true;

#ifdef USE_LAZY_DELETE
            if (old_isnull) continue;
            if (repl_isnull[i] || DatumGetUInt64(old_datum) != global)
            {
                uint64 old_value = DatumGetUInt64(old_datum);
                send_delete_req(atttype, old_value);
            }
#endif
        }
        if (modified)
            newtup = heap_modify_tuple(newtup, tupleDesc, repl_values, repl_isnull, repl_cols);
        return PointerGetDatum(newtup);
    }
    else
    {
        elog(ERROR, "Unsupported trigger event type");
        PG_RETURN_NULL();
    }
}
