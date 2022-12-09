/* Generated by the protocol buffer compiler.  DO NOT EDIT! */
/* Generated from: chk.proto */

#ifndef PROTOBUF_C_chk_2eproto__INCLUDED
#define PROTOBUF_C_chk_2eproto__INCLUDED

#include <protobuf-c/protobuf-c.h>

PROTOBUF_C__BEGIN_DECLS

#if PROTOBUF_C_VERSION_NUMBER < 1003000
# error This file was generated by a newer version of protoc-c which is incompatible with your libprotobuf-c headers. Please update your headers.
#elif 1003000 < PROTOBUF_C_MIN_COMPILER_VERSION
# error This file was generated by an older version of protoc-c which is incompatible with your libprotobuf-c headers. Please regenerate this file with a newer version of protoc-c.
#endif


typedef struct _Chk__CheckReport Chk__CheckReport;


/* --- enums --- */

/*
 * Kinds of DAOS global inconsistency.
 */
typedef enum _Chk__CheckInconsistClass {
  /*
   * Consistent cases.
   */
  CHK__CHECK_INCONSIST_CLASS__CIC_NONE = 0,
  /*
   * Only a subset of the pool services are present but we will have a quorum.
   * Default action: CIA_IGNORE.
   */
  CHK__CHECK_INCONSIST_CLASS__CIC_POOL_LESS_SVC_WITH_QUORUM = 1,
  /*
   * Only a subset of the pool services are present, and we don't have a quorum.
   * Default action: CIA_INTERACT.
   */
  CHK__CHECK_INCONSIST_CLASS__CIC_POOL_LESS_SVC_WITHOUT_QUORUM = 2,
  /*
   * More members are reported than the pool service was created with.
   * Default action: CIA_DISCARD. Remove unrecognized pool service.
   */
  CHK__CHECK_INCONSIST_CLASS__CIC_POOL_MORE_SVC = 3,
  /*
   * Engine(s) claim the pool which is not registered to MS.
   * Default action: CIA_READD. Register the pool to the MS.
   */
  CHK__CHECK_INCONSIST_CLASS__CIC_POOL_NONEXIST_ON_MS = 4,
  /*
   * Pool is registered to MS but not claimed by any engine.
   * Default action: CIA_DISCARD. De-register pool from MS.
   */
  CHK__CHECK_INCONSIST_CLASS__CIC_POOL_NONEXIST_ON_ENGINE = 5,
  /*
   * Svcl list stored in MS does not match the actual PS membership.
   * Default action: CIA_TRUST_PS. Refresh svcl list in MS DB.
   */
  CHK__CHECK_INCONSIST_CLASS__CIC_POOL_BAD_SVCL = 6,
  /*
   * The pool label recorded by MS does not match the pool label property from PS.
   * Default action: CIA_TRUST_PS. Refresh label in MS DB.
   */
  CHK__CHECK_INCONSIST_CLASS__CIC_POOL_BAD_LABEL = 7,
  /*
   * An engine has some allocated storage but does not appear in pool map.
   * Default action: CIA_DISCARD. Associated files and blobs will be deleted from the engine.
   */
  CHK__CHECK_INCONSIST_CLASS__CIC_ENGINE_NONEXIST_IN_MAP = 8,
  /*
   * An engine has some allocated storage and is marked as down/downout in pool map.
   * Default action: CIA_IGNORE. It can be reintegrated after CR scan.
   */
  CHK__CHECK_INCONSIST_CLASS__CIC_ENGINE_DOWN_IN_MAP = 9,
  /*
   * An engine is referenced in pool map, but no storage is actually allocated on this engine.
   * Default action: CIA_DISCARD. Evict the rank from pool map, give left things to rebuild.
   */
  CHK__CHECK_INCONSIST_CLASS__CIC_ENGINE_HAS_NO_STORAGE = 10,
  /*
   * Containers that have storage allocated on engine but does not exist in the PS.
   * Default action: CIA_DISCARD. Destrory the unrecognized container.
   */
  CHK__CHECK_INCONSIST_CLASS__CIC_CONT_NONEXIST_ON_PS = 11,
  /*
   * The container label recorded by PS does not match the container label property.
   * Default action: CIA_TRUST_PS. Refresh label property on related target(s).
   */
  CHK__CHECK_INCONSIST_CLASS__CIC_CONT_BAD_LABEL = 12,
  /*
   * The DTX is corrupted, some participant RDG(s) may be lost.
   * Default action: CIA_INTERACT.
   */
  CHK__CHECK_INCONSIST_CLASS__CIC_DTX_CORRUPTED = 13,
  /*
   * The DTX entry on leader does not exist, then not sure the status.
   * Default action: CIA_DISCARD. It is equal to abort the DTX and may lost data on related
   * shard, then we may found data inconsistency in subseqeunt CR scan phase, at that time,
   * such data inconsistency will be fixed.
   */
  CHK__CHECK_INCONSIST_CLASS__CIC_DTX_ORPHAN = 14,
  /*
   * The checksum information is lost.
   * Default action: CIA_READD. We have to trust the data and recalculate the checksum. If
   * data is corrupted, then we may hit data inconsistency in subseqeunt CR scan phase, at
   * that time, such data inconsistency will be fixed.
   */
  CHK__CHECK_INCONSIST_CLASS__CIC_CSUM_LOST = 15,
  /*
   * Checksum related inconsistency or data corruption.
   * Default action: CIA_DISCARD. Then we will hit data lost in subseqeunt CR scan phase,
   * at that time, such data inconsistency will be fixed.
   */
  CHK__CHECK_INCONSIST_CLASS__CIC_CSUM_FAILURE = 16,
  /*
   * Replicated object lost some replica(s).
   * Default action: CIA_READD. Copy from another valid replica.
   */
  CHK__CHECK_INCONSIST_CLASS__CIC_OBJ_LOST_REP = 17,
  /*
   * EC object lost parity or data shard(s).
   * Default action: CIA_READD. Trust other available shards and recalculate the lost one(s).
   */
  CHK__CHECK_INCONSIST_CLASS__CIC_OBJ_LOST_EC_SHARD = 18,
  /*
   * EC object lost too many shards that exceeds its redundancy.
   * Default action: CIA_INTERACT. Ask the admin to decide whether keep or remove the object.
   */
  CHK__CHECK_INCONSIST_CLASS__CIC_OBJ_LOST_EC_DATA = 19,
  /*
   * Data inconsistency among replicas
   * Default action: CIA_TRUST_LATEST. Try to keep the latest data. If all have the same epoch,
   * then ask the admin (CIA_INTERACT) to decide which one will be trusted.
   */
  CHK__CHECK_INCONSIST_CLASS__CIC_OBJ_DATA_INCONSIST = 20,
  /*
   * Unknown inconsistency.
   * Default action: CIA_IGNORE.
   */
  CHK__CHECK_INCONSIST_CLASS__CIC_UNKNOWN = 100
    PROTOBUF_C__FORCE_ENUM_TO_BE_INT_SIZE(CHK__CHECK_INCONSIST_CLASS)
} Chk__CheckInconsistClass;
/*
 * Actions for how to handle kinds of inconsistency.
 */
typedef enum _Chk__CheckInconsistAction {
  /*
   * Default action, depends on the detailed inconsistency class.
   */
  CHK__CHECK_INCONSIST_ACTION__CIA_DEFAULT = 0,
  /*
   * Interact with administrator for further action.
   */
  CHK__CHECK_INCONSIST_ACTION__CIA_INTERACT = 1,
  /*
   * Ignore but log the inconsistency.
   */
  CHK__CHECK_INCONSIST_ACTION__CIA_IGNORE = 2,
  /*
   * Discard the unrecognized element: pool service, pool itself, container, and so on.
   */
  CHK__CHECK_INCONSIST_ACTION__CIA_DISCARD = 3,
  /*
   * Re-add the missing element: pool to MS, target to pool map, and so on.
   */
  CHK__CHECK_INCONSIST_ACTION__CIA_READD = 4,
  /*
   * Trust the information recorded in MS DB.
   */
  CHK__CHECK_INCONSIST_ACTION__CIA_TRUST_MS = 5,
  /*
   * Trust the information recorded in PS DB.
   */
  CHK__CHECK_INCONSIST_ACTION__CIA_TRUST_PS = 6,
  /*
   * Trust the information recorded by target(s).
   */
  CHK__CHECK_INCONSIST_ACTION__CIA_TRUST_TARGET = 7,
  /*
   * Trust the majority parts (if have).
   */
  CHK__CHECK_INCONSIST_ACTION__CIA_TRUST_MAJORITY = 8,
  /*
   * Trust the one with latest (pool map or epoch) information. Keep the latest data.
   */
  CHK__CHECK_INCONSIST_ACTION__CIA_TRUST_LATEST = 9,
  /*
   * Trust the one with oldest (pool map or epoch) information. Rollback to old version.
   */
  CHK__CHECK_INCONSIST_ACTION__CIA_TRUST_OLDEST = 10,
  /*
   * Trust EC parity shard.
   */
  CHK__CHECK_INCONSIST_ACTION__CIA_TRUST_EC_PARITY = 11,
  /*
   * Trust EC data shard.
   */
  CHK__CHECK_INCONSIST_ACTION__CIA_TRUST_EC_DATA = 12
    PROTOBUF_C__FORCE_ENUM_TO_BE_INT_SIZE(CHK__CHECK_INCONSIST_ACTION)
} Chk__CheckInconsistAction;
/*
 * The flags to control DAOS check general behavior, not related with any detailed inconsistency.
 */
typedef enum _Chk__CheckFlag {
  CHK__CHECK_FLAG__CF_NONE = 0,
  /*
   * Only scan without real repairing inconsistency.
   */
  CHK__CHECK_FLAG__CF_DRYRUN = 1,
  /*
   * Start DAOS check from the beginning.
   * Otherwise, resume the DAOS check from the latest checkpoint by default.
   */
  CHK__CHECK_FLAG__CF_RESET = 2,
  /*
   * Stop DAOS check if hit unknown inconsistency or fail to repair some inconsistency.
   * Otherwise, mark 'fail' on related component and continue to handle next one by default.
   */
  CHK__CHECK_FLAG__CF_FAILOUT = 4,
  /*
   * If the admin does not want to interact with engine during check scan, then CIA_INTERACT
   * will be converted to CIA_IGNORE. That will overwrite the CheckInconsistPolicy.
   */
  CHK__CHECK_FLAG__CF_AUTO = 8,
  /*
   * Handle orphan pool when start the check instance. If not specify the flag, some orphan
   * pool(s) may be not handled (by default) unless all pools are checked from the scratch.
   */
  CHK__CHECK_FLAG__CF_ORPHAN_POOL = 16,
  /*
   * Overwrite former set CF_FAILOUT flag, cannot be specified together with CF_FAILOUT.
   */
  CHK__CHECK_FLAG__CF_NO_FAILOUT = 32,
  /*
   * Overwrite former set CF_AUTO flag, cannot be specified together with CF_AUTO.
   */
  CHK__CHECK_FLAG__CF_NO_AUTO = 64
    PROTOBUF_C__FORCE_ENUM_TO_BE_INT_SIZE(CHK__CHECK_FLAG)
} Chk__CheckFlag;
/*
 * The status of DAOS check instance.
 */
typedef enum _Chk__CheckInstStatus {
  /*
   * DAOS check has never been run.
   */
  CHK__CHECK_INST_STATUS__CIS_INIT = 0,
  /*
   * DAOS check is still in process.
   */
  CHK__CHECK_INST_STATUS__CIS_RUNNING = 1,
  /*
   * All passes have been done for all required pools.
   */
  CHK__CHECK_INST_STATUS__CIS_COMPLETED = 2,
  /*
   * DAOS check has been explicitly stopped, do not allow to rejoin.
   */
  CHK__CHECK_INST_STATUS__CIS_STOPPED = 3,
  /*
   * DAOS check auto stopped for some unrecoverable failure, do not rejoin.
   */
  CHK__CHECK_INST_STATUS__CIS_FAILED = 4,
  /*
   * DAOS check has been paused because engine exit, allow to rejoin.
   */
  CHK__CHECK_INST_STATUS__CIS_PAUSED = 5,
  /*
   * Check on the engine exit for other engine failure, do not rejoin.
   */
  CHK__CHECK_INST_STATUS__CIS_IMPLICATED = 6
    PROTOBUF_C__FORCE_ENUM_TO_BE_INT_SIZE(CHK__CHECK_INST_STATUS)
} Chk__CheckInstStatus;
/*
 * The pool status for DAOS check.
 */
typedef enum _Chk__CheckPoolStatus {
  /*
   * DAOS check has not started against this pool.
   */
  CHK__CHECK_POOL_STATUS__CPS_UNCHECKED = 0,
  /*
   * The pool is being checked.
   */
  CHK__CHECK_POOL_STATUS__CPS_CHECKING = 1,
  /*
   * DAOS check has successfully completed all the passes on this pool.
   */
  CHK__CHECK_POOL_STATUS__CPS_CHECKED = 2,
  /*
   * DAOS check could not be completed due to some unrecoverable failure.
   */
  CHK__CHECK_POOL_STATUS__CPS_FAILED = 3,
  /*
   * Checking the pool has been paused because engine exit.
   */
  CHK__CHECK_POOL_STATUS__CPS_PAUSED = 4,
  /*
   * Waiting for the decision from the admin.
   */
  CHK__CHECK_POOL_STATUS__CPS_PENDING = 5,
  /*
   * DAOS check on the pool has been stopped explicitly.
   */
  CHK__CHECK_POOL_STATUS__CPS_STOPPED = 6,
  /*
   * Check on the pool is stopped because of other pool or engine failure.
   */
  CHK__CHECK_POOL_STATUS__CPS_IMPLICATED = 7
    PROTOBUF_C__FORCE_ENUM_TO_BE_INT_SIZE(CHK__CHECK_POOL_STATUS)
} Chk__CheckPoolStatus;
/*
 * DAOS check engine scan phases.
 */
typedef enum _Chk__CheckScanPhase {
  /*
   * Initial phase, prepare to start check on related engines.
   */
  CHK__CHECK_SCAN_PHASE__CSP_PREPARE = 0,
  /*
   * Pool list consolidation.
   */
  CHK__CHECK_SCAN_PHASE__CSP_POOL_LIST = 1,
  /*
   * Pool membership.
   */
  CHK__CHECK_SCAN_PHASE__CSP_POOL_MBS = 2,
  /*
   * Pool cleanup.
   */
  CHK__CHECK_SCAN_PHASE__CSP_POOL_CLEANUP = 3,
  /*
   * Container list consolidation.
   */
  CHK__CHECK_SCAN_PHASE__CSP_CONT_LIST = 4,
  /*
   * Container cleanup.
   */
  CHK__CHECK_SCAN_PHASE__CSP_CONT_CLEANUP = 5,
  /*
   * DTX resync and cleanup.
   */
  CHK__CHECK_SCAN_PHASE__CSP_DTX_RESYNC = 6,
  /*
   * RP/EC shards consistency verification with checksum scrub if have.
   */
  CHK__CHECK_SCAN_PHASE__CSP_OBJ_SCRUB = 7,
  /*
   * Object rebuild.
   */
  CHK__CHECK_SCAN_PHASE__CSP_REBUILD = 8,
  /*
   * EC aggregation & VOS aggregation.
   */
  CHK__CHECK_SCAN_PHASE__OSP_AGGREGATION = 9,
  /*
   * All done.
   */
  CHK__CHECK_SCAN_PHASE__DSP_DONE = 10
    PROTOBUF_C__FORCE_ENUM_TO_BE_INT_SIZE(CHK__CHECK_SCAN_PHASE)
} Chk__CheckScanPhase;

/* --- messages --- */

/*
 * DAOS check engine reports the found inconsistency and repair result to control plane.
 * If the repair action is CIA_INTERACT, then the control plane will reply current dRPC
 * firstly, and then interact with the admin for the repair decision in another section
 * and tell DAOS check engine via another DRPC_METHOD_MGMT_CHK_ACT dRPC call.
 * If the CheckReport::msg is not enough to help admin to make the decision, then we
 * may have to leverage DAOS debug tools to dump more information from related target.
 */
struct  _Chk__CheckReport
{
  ProtobufCMessage base;
  /*
   * DAOS Check event sequence, unique for the instance.
   */
  uint64_t seq;
  /*
   * Inconsistency class
   */
  Chk__CheckInconsistClass class_;
  /*
   * The action taken to repair the inconsistency
   */
  Chk__CheckInconsistAction action;
  /*
   * Repair result: zero is for repaired successfully.
   *		  negative value if failed to repair.
   *		  positive value is for CIA_IGNORE or dryrun mode.
   * It is meaningless if the action is CIA_INTERACT.
   */
  int32_t result;
  /*
   * Inconsistency happened on which rank if applicable.
   */
  uint32_t rank;
  /*
   * Inconsistency happened on which target in the rank if applicable.
   */
  uint32_t target;
  /*
   * The consistency is in which pool if applicable.
   */
  char *pool_uuid;
  /*
   * The pool label, if available.
   */
  char *pool_label;
  /*
   * The consistency is in which container if applicable.
   */
  char *cont_uuid;
  /*
   * The container label, if available.
   */
  char *cont_label;
  /*
   * The consistency is in which object if applicable.
   */
  char *objid;
  /*
   * The consistency is in which dkey if applicable.
   */
  char *dkey;
  /*
   * The consistency is in which akey if applicable.
   */
  char *akey;
  /*
   * The time of report (and repair) the inconsistency.
   */
  char *timestamp;
  /*
   * Information to describe the inconsistency in detail.
   */
  char *msg;
  /*
   * Interactive mode options (first is suggested).
   */
  size_t n_act_choices;
  Chk__CheckInconsistAction *act_choices;
  /*
   * Details for each potential action (length should match actions).
   */
  size_t n_act_details;
  char **act_details;
  /*
   * Formatted messages containing details for each action choice.
   */
  size_t n_act_msgs;
  char **act_msgs;
};
#define CHK__CHECK_REPORT__INIT \
 { PROTOBUF_C_MESSAGE_INIT (&chk__check_report__descriptor) \
    , 0, CHK__CHECK_INCONSIST_CLASS__CIC_NONE, CHK__CHECK_INCONSIST_ACTION__CIA_DEFAULT, 0, 0, 0, (char *)protobuf_c_empty_string, (char *)protobuf_c_empty_string, (char *)protobuf_c_empty_string, (char *)protobuf_c_empty_string, (char *)protobuf_c_empty_string, (char *)protobuf_c_empty_string, (char *)protobuf_c_empty_string, (char *)protobuf_c_empty_string, (char *)protobuf_c_empty_string, 0,NULL, 0,NULL, 0,NULL }


/* Chk__CheckReport methods */
void   chk__check_report__init
                     (Chk__CheckReport         *message);
size_t chk__check_report__get_packed_size
                     (const Chk__CheckReport   *message);
size_t chk__check_report__pack
                     (const Chk__CheckReport   *message,
                      uint8_t             *out);
size_t chk__check_report__pack_to_buffer
                     (const Chk__CheckReport   *message,
                      ProtobufCBuffer     *buffer);
Chk__CheckReport *
       chk__check_report__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data);
void   chk__check_report__free_unpacked
                     (Chk__CheckReport *message,
                      ProtobufCAllocator *allocator);
/* --- per-message closures --- */

typedef void (*Chk__CheckReport_Closure)
                 (const Chk__CheckReport *message,
                  void *closure_data);

/* --- services --- */


/* --- descriptors --- */

extern const ProtobufCEnumDescriptor    chk__check_inconsist_class__descriptor;
extern const ProtobufCEnumDescriptor    chk__check_inconsist_action__descriptor;
extern const ProtobufCEnumDescriptor    chk__check_flag__descriptor;
extern const ProtobufCEnumDescriptor    chk__check_inst_status__descriptor;
extern const ProtobufCEnumDescriptor    chk__check_pool_status__descriptor;
extern const ProtobufCEnumDescriptor    chk__check_scan_phase__descriptor;
extern const ProtobufCMessageDescriptor chk__check_report__descriptor;

PROTOBUF_C__END_DECLS


#endif  /* PROTOBUF_C_chk_2eproto__INCLUDED */
