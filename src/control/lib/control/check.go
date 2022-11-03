//
// (C) Copyright 2022 Intel Corporation.
//
// SPDX-License-Identifier: BSD-2-Clause-Patent
//

package control

import (
	"context"
	"fmt"
	"sort"
	"strings"
	"time"

	"github.com/pkg/errors"
	"google.golang.org/grpc"
	"google.golang.org/protobuf/proto"

	chkpb "github.com/daos-stack/daos/src/control/common/proto/chk"
	mgmtpb "github.com/daos-stack/daos/src/control/common/proto/mgmt"
)

type SystemCheckEnableReq struct {
	unaryRequest
	msRequest

	mgmtpb.CheckEnableReq
}

// SystemCheckEnable enables the system checker.
func SystemCheckEnable(ctx context.Context, rpcClient UnaryInvoker, req *SystemCheckEnableReq) error {
	if req == nil {
		return errors.Errorf("nil %T", req)
	}

	req.CheckEnableReq.Sys = req.getSystem(rpcClient)
	req.setRPC(func(ctx context.Context, conn *grpc.ClientConn) (proto.Message, error) {
		return mgmtpb.NewMgmtSvcClient(conn).SystemCheckEnable(ctx, &req.CheckEnableReq)
	})
	rpcClient.Debugf("DAOS system checker enable request: %+v", req)

	ur, err := rpcClient.InvokeUnaryRPC(ctx, req)
	if err != nil {
		return err
	}
	ms, err := ur.getMSResponse()
	if err != nil {
		return err
	}
	rpcClient.Debugf("DAOS system checker enable response: %+v", ms)

	return nil
}

type SystemCheckDisableReq struct {
	unaryRequest
	msRequest

	mgmtpb.CheckDisableReq
}

// SystemCheckDisable disables the system checker.
func SystemCheckDisable(ctx context.Context, rpcClient UnaryInvoker, req *SystemCheckDisableReq) error {
	if req == nil {
		return errors.Errorf("nil %T", req)
	}

	req.CheckDisableReq.Sys = req.getSystem(rpcClient)
	req.setRPC(func(ctx context.Context, conn *grpc.ClientConn) (proto.Message, error) {
		return mgmtpb.NewMgmtSvcClient(conn).SystemCheckDisable(ctx, &req.CheckDisableReq)
	})
	rpcClient.Debugf("DAOS system checker disable request: %+v", req)

	ur, err := rpcClient.InvokeUnaryRPC(ctx, req)
	if err != nil {
		return err
	}
	ms, err := ur.getMSResponse()
	if err != nil {
		return err
	}
	rpcClient.Debugf("DAOS system checker disable response: %+v", ms)

	return nil
}

const (
	SystemCheckFlagDryRun  = SystemCheckFlags(chkpb.CheckFlag_CF_DRYRUN)
	SystemCheckFlagReset   = SystemCheckFlags(chkpb.CheckFlag_CF_RESET)
	SystemCheckFlagFailout = SystemCheckFlags(chkpb.CheckFlag_CF_FAILOUT)
	SystemCheckFlagAuto    = SystemCheckFlags(chkpb.CheckFlag_CF_AUTO)

	incClassPrefix     = "CIC_"
	incActionPrefix    = "CIA_"
	incStatusPrefix    = "CIS_"
	incScanPhasePrefix = "CSP_"
)

type SystemCheckFlags uint32

func (f SystemCheckFlags) String() string {
	var flags []string
	if f&SystemCheckFlagDryRun != 0 {
		flags = append(flags, "dry-run")
	}
	if f&SystemCheckFlagReset != 0 {
		flags = append(flags, "reset")
	}
	if f&SystemCheckFlagFailout != 0 {
		flags = append(flags, "failout")
	}
	if f&SystemCheckFlagAuto != 0 {
		flags = append(flags, "auto")
	}
	if len(flags) == 0 {
		return "none"
	}
	return strings.Join(flags, ",")
}

func (f SystemCheckFlags) MarshalJSON() ([]byte, error) {
	return []byte(`"` + f.String() + `"`), nil
}

type SystemCheckFindingClass chkpb.CheckInconsistClass

func (c SystemCheckFindingClass) String() string {
	return strings.TrimPrefix(chkpb.CheckInconsistClass(c).String(), incClassPrefix)
}

func (c *SystemCheckFindingClass) FromString(in string) error {
	if !strings.HasPrefix(in, incClassPrefix) {
		in = incClassPrefix + in
	}
	if cls, ok := chkpb.CheckInconsistClass_value[in]; ok {
		*c = SystemCheckFindingClass(cls)
		return nil
	}
	return errors.Errorf("invalid inconsistency class %q", in)
}

type SystemCheckRepairAction chkpb.CheckInconsistAction

func (a SystemCheckRepairAction) String() string {
	return strings.TrimPrefix(chkpb.CheckInconsistAction(a).String(), incActionPrefix)
}

func (a *SystemCheckRepairAction) FromString(in string) error {
	if !strings.HasPrefix(in, incActionPrefix) {
		in = incActionPrefix + in
	}
	if act, ok := chkpb.CheckInconsistAction_value[in]; ok {
		*a = SystemCheckRepairAction(act)
		return nil
	}
	return errors.Errorf("invalid inconsistency action %q", in)
}

type SystemCheckPolicy struct {
	FindingClass SystemCheckFindingClass
	RepairAction SystemCheckRepairAction
}

func NewSystemCheckPolicy(cls, act string) (*SystemCheckPolicy, error) {
	p := &SystemCheckPolicy{}
	if err := p.FindingClass.FromString(cls); err != nil {
		return nil, err
	}
	if err := p.RepairAction.FromString(act); err != nil {
		return nil, err
	}
	return p, nil
}

func (p *SystemCheckPolicy) String() string {
	return p.FindingClass.String() + ":" + p.RepairAction.String()
}

func (p *SystemCheckPolicy) MarshalJSON() ([]byte, error) {
	return []byte(`"` + p.String() + `"`), nil
}

func (p *SystemCheckPolicy) UnmarshalJSON(in []byte) error {
	parts := strings.Split(strings.Trim(string(in), `"`), ":")
	if len(parts) != 2 {
		return errors.Errorf("invalid policy %q", in)
	}
	pol, err := NewSystemCheckPolicy(parts[0], parts[1])
	if err != nil {
		return err
	}
	*p = *pol

	return nil
}

func (p *SystemCheckPolicy) toPB() *mgmtpb.CheckInconsistPolicy {
	return &mgmtpb.CheckInconsistPolicy{
		InconsistCas: chkpb.CheckInconsistClass(p.FindingClass),
		InconsistAct: chkpb.CheckInconsistAction(p.RepairAction),
	}
}

func policyFromPB(pb *mgmtpb.CheckInconsistPolicy) *SystemCheckPolicy {
	return &SystemCheckPolicy{
		FindingClass: SystemCheckFindingClass(pb.InconsistCas),
		RepairAction: SystemCheckRepairAction(pb.InconsistAct),
	}
}

func CheckerPolicyClasses() []SystemCheckFindingClass {
	classes := make([]SystemCheckFindingClass, 0, len(chkpb.CheckInconsistClass_value))
	for _, val := range chkpb.CheckInconsistClass_value {
		classes = append(classes, SystemCheckFindingClass(val))
	}
	return classes
}

func CheckerPolicyActions() []SystemCheckRepairAction {
	actions := make([]SystemCheckRepairAction, 0, len(chkpb.CheckInconsistAction_value))
	for _, val := range chkpb.CheckInconsistAction_value {
		actions = append(actions, SystemCheckRepairAction(val))
	}
	return actions
}

type SystemCheckStartReq struct {
	unaryRequest
	msRequest

	Policies []*SystemCheckPolicy
	mgmtpb.CheckStartReq
}

// SystemCheckStart starts the system checker.
func SystemCheckStart(ctx context.Context, rpcClient UnaryInvoker, req *SystemCheckStartReq) error {
	if req == nil {
		return errors.Errorf("nil %T", req)
	}

	req.CheckStartReq.Sys = req.getSystem(rpcClient)
	for _, p := range req.Policies {
		req.CheckStartReq.Policies = append(req.CheckStartReq.Policies, p.toPB())
	}
	req.setRPC(func(ctx context.Context, conn *grpc.ClientConn) (proto.Message, error) {
		return mgmtpb.NewMgmtSvcClient(conn).SystemCheckStart(ctx, &req.CheckStartReq)
	})
	rpcClient.Debugf("DAOS system check start request: %+v", req)

	ur, err := rpcClient.InvokeUnaryRPC(ctx, req)
	if err != nil {
		return err
	}
	ms, err := ur.getMSResponse()
	if err != nil {
		return err
	}
	rpcClient.Debugf("DAOS system check start response: %+v", ms)

	return nil
}

type SystemCheckStopReq struct {
	unaryRequest
	msRequest

	mgmtpb.CheckStopReq
}

// SystemCheckStop stops the system checker.
func SystemCheckStop(ctx context.Context, rpcClient UnaryInvoker, req *SystemCheckStopReq) error {
	if req == nil {
		return errors.Errorf("nil %T", req)
	}

	req.CheckStopReq.Sys = req.getSystem(rpcClient)
	req.setRPC(func(ctx context.Context, conn *grpc.ClientConn) (proto.Message, error) {
		return mgmtpb.NewMgmtSvcClient(conn).SystemCheckStop(ctx, &req.CheckStopReq)
	})
	rpcClient.Debugf("DAOS system check stop request: %+v", req)

	ur, err := rpcClient.InvokeUnaryRPC(ctx, req)
	if err != nil {
		return err
	}
	ms, err := ur.getMSResponse()
	if err != nil {
		return err
	}
	rpcClient.Debugf("DAOS system check stop response: %+v", ms)

	return nil
}

type SystemCheckQueryReq struct {
	unaryRequest
	msRequest

	mgmtpb.CheckQueryReq
}

type SystemCheckStatus chkpb.CheckInstStatus

func (s SystemCheckStatus) String() string {
	return strings.TrimPrefix(chkpb.CheckInstStatus(s).String(), incStatusPrefix)
}

func (p SystemCheckStatus) MarshalJSON() ([]byte, error) {
	return []byte(`"` + p.String() + `"`), nil
}

const (
	SystemCheckStatusInit       = SystemCheckStatus(chkpb.CheckInstStatus_CIS_INIT)
	SystemCheckStatusRunning    = SystemCheckStatus(chkpb.CheckInstStatus_CIS_RUNNING)
	SystemCheckStatusCompleted  = SystemCheckStatus(chkpb.CheckInstStatus_CIS_COMPLETED)
	SystemCheckStatusStopped    = SystemCheckStatus(chkpb.CheckInstStatus_CIS_STOPPED)
	SystemCheckStatusFailed     = SystemCheckStatus(chkpb.CheckInstStatus_CIS_FAILED)
	SystemCheckStatusPaused     = SystemCheckStatus(chkpb.CheckInstStatus_CIS_PAUSED)
	SystemCheckStatusImplicated = SystemCheckStatus(chkpb.CheckInstStatus_CIS_IMPLICATED)
)

type SystemCheckScanPhase chkpb.CheckScanPhase

func (p SystemCheckScanPhase) String() string {
	return strings.TrimPrefix(chkpb.CheckScanPhase(p).String(), incScanPhasePrefix)
}

func (p SystemCheckScanPhase) Description() string {
	switch p {
	case SystemCheckScanPhasePrepare:
		return "Preparing check engine"
	case SystemCheckScanPhasePoolList:
		return "Comparing pool list on MS and storage nodes"
	case SystemCheckScanPhasePoolMembership:
		return "Comparing pool membership on MS and storage nodes"
	case SystemCheckScanPhasePoolCleanup:
		return "Cleaning up pool entries"
	case SystemCheckScanPhaseContainerList:
		return "Comparing container list on PS and storage nodes"
	case SystemCheckScanPhaseContainerCleanup:
		return "Cleaning up container entries"
	case SystemCheckScanPhaseDtxResync:
		return "DTX resync and cleanup"
	case SystemCheckScanPhaseObjectScrub:
		return "Scrubbing objects"
	case SystemCheckScanPhaseObjectRebuild:
		return "Rebuilding objects"
	case SystemCheckScanPhaseAggregation:
		return "EC and VOS aggregation"
	case SystemCheckScanPhaseDone:
		return "Check completed"
	default:
		return fmt.Sprintf("Unknown (%s)", p)
	}
}

func (p SystemCheckScanPhase) MarshalJSON() ([]byte, error) {
	return []byte(`"` + p.String() + `"`), nil
}

const (
	SystemCheckScanPhasePrepare          = SystemCheckScanPhase(chkpb.CheckScanPhase_CSP_PREPARE)
	SystemCheckScanPhasePoolList         = SystemCheckScanPhase(chkpb.CheckScanPhase_CSP_POOL_LIST)
	SystemCheckScanPhasePoolMembership   = SystemCheckScanPhase(chkpb.CheckScanPhase_CSP_POOL_MBS)
	SystemCheckScanPhasePoolCleanup      = SystemCheckScanPhase(chkpb.CheckScanPhase_CSP_POOL_CLEANUP)
	SystemCheckScanPhaseContainerList    = SystemCheckScanPhase(chkpb.CheckScanPhase_CSP_CONT_LIST)
	SystemCheckScanPhaseContainerCleanup = SystemCheckScanPhase(chkpb.CheckScanPhase_CSP_CONT_CLEANUP)
	SystemCheckScanPhaseDtxResync        = SystemCheckScanPhase(chkpb.CheckScanPhase_CSP_DTX_RESYNC)
	SystemCheckScanPhaseObjectScrub      = SystemCheckScanPhase(chkpb.CheckScanPhase_CSP_OBJ_SCRUB)
	SystemCheckScanPhaseObjectRebuild    = SystemCheckScanPhase(chkpb.CheckScanPhase_CSP_REBUILD)
	SystemCheckScanPhaseAggregation      = SystemCheckScanPhase(chkpb.CheckScanPhase_OSP_AGGREGATION)
	SystemCheckScanPhaseDone             = SystemCheckScanPhase(chkpb.CheckScanPhase_DSP_DONE)
)

type SystemCheckRepairChoice struct {
	Action SystemCheckRepairAction
	Info   string
}

type SystemCheckReport struct {
	chkpb.CheckReport
}

func (r *SystemCheckReport) RepairChoices() []*SystemCheckRepairChoice {
	choices := make([]*SystemCheckRepairChoice, len(r.ActChoices))
	for i, c := range r.ActChoices {
		choices[i] = &SystemCheckRepairChoice{
			Action: SystemCheckRepairAction(c),
			Info:   r.ActMsgs[i],
		}
	}

	sort.Slice(choices, func(i, j int) bool {
		return choices[i].Action < choices[j].Action
	})
	return choices
}

func (r *SystemCheckReport) IsInteractive() bool {
	return r.Action == chkpb.CheckInconsistAction_CIA_INTERACT
}

func (r *SystemCheckReport) Resolution() string {
	msg := SystemCheckRepairAction(r.Action).String()
	if len(r.ActMsgs) == 1 {
		msg += ": " + r.ActMsgs[0]
	}
	return msg
}

type SystemCheckQueryResp struct {
	Status    SystemCheckStatus    `json:"status"`
	ScanPhase SystemCheckScanPhase `json:"scan_phase"`
	StartTime time.Time            `json:"start_time"`

	// FIXME: Don't use protobuf types in public API.
	Pools   []*mgmtpb.CheckQueryPool `json:"pools"`
	Reports []*SystemCheckReport     `json:"reports"`
}

// SystemCheckQuery queries the system checker status.
func SystemCheckQuery(ctx context.Context, rpcClient UnaryInvoker, req *SystemCheckQueryReq) (*SystemCheckQueryResp, error) {
	if req == nil {
		return nil, errors.Errorf("nil %T", req)
	}

	req.CheckQueryReq.Sys = req.getSystem(rpcClient)
	req.setRPC(func(ctx context.Context, conn *grpc.ClientConn) (proto.Message, error) {
		return mgmtpb.NewMgmtSvcClient(conn).SystemCheckQuery(ctx, &req.CheckQueryReq)
	})
	rpcClient.Debugf("DAOS system check query request: %+v", req)

	ur, err := rpcClient.InvokeUnaryRPC(ctx, req)
	if err != nil {
		return nil, err
	}
	pbResp := new(mgmtpb.CheckQueryResp)
	if err := convertMSResponse(ur, pbResp); err != nil {
		return nil, err
	}
	rpcClient.Debugf("DAOS system check query response: %+v", pbResp)

	resp := &SystemCheckQueryResp{
		Status:    SystemCheckStatus(pbResp.GetInsStatus()),
		ScanPhase: SystemCheckScanPhase(pbResp.GetInsPhase()),
		StartTime: time.Unix(int64(pbResp.GetTime().GetStartTime()), 0),
		Pools:     pbResp.GetPools(),
	}
	for _, pbReport := range pbResp.GetReports() {
		rpt := new(SystemCheckReport)
		proto.Merge(rpt, pbReport)
		resp.Reports = append(resp.Reports, rpt)
	}
	return resp, nil
}

type SystemCheckGetPolicyReq struct {
	unaryRequest
	msRequest

	mgmtpb.CheckGetPolicyReq
}

func (r *SystemCheckGetPolicyReq) SetClasses(classes []SystemCheckFindingClass) {
	for _, cls := range classes {
		r.CheckGetPolicyReq.Classes = append(r.CheckGetPolicyReq.Classes, chkpb.CheckInconsistClass(cls))
	}
}

type SystemCheckGetPolicyResp struct {
	CheckerFlags SystemCheckFlags     `json:"checker_flags"`
	Policies     []*SystemCheckPolicy `json:"policies"`
}

// SystemCheckGetPolicy queries the system checker properties.
func SystemCheckGetPolicy(ctx context.Context, rpcClient UnaryInvoker, req *SystemCheckGetPolicyReq) (*SystemCheckGetPolicyResp, error) {
	if req == nil {
		return nil, errors.Errorf("nil %T", req)
	}

	req.CheckGetPolicyReq.Sys = req.getSystem(rpcClient)
	req.setRPC(func(ctx context.Context, conn *grpc.ClientConn) (proto.Message, error) {
		return mgmtpb.NewMgmtSvcClient(conn).SystemCheckGetPolicy(ctx, &req.CheckGetPolicyReq)
	})
	rpcClient.Debugf("DAOS system check get-policy request: %+v", req)

	ur, err := rpcClient.InvokeUnaryRPC(ctx, req)
	if err != nil {
		return nil, err
	}
	ms, err := ur.getMSResponse()
	if err != nil {
		return nil, err
	}
	rpcClient.Debugf("DAOS system check get-policy response: %+v", ms)

	resp := new(SystemCheckGetPolicyResp)
	if pbResp, ok := ms.(*mgmtpb.CheckGetPolicyResp); ok {
		resp.CheckerFlags = SystemCheckFlags(pbResp.Flags)
		for _, p := range pbResp.Policies {
			resp.Policies = append(resp.Policies, policyFromPB(p))
		}
	} else {
		return nil, errors.Errorf("unexpected response type %T", ms)
	}
	return resp, nil
}

type SystemCheckSetPolicyReq struct {
	unaryRequest
	msRequest

	Policies []*SystemCheckPolicy
	mgmtpb.CheckSetPolicyReq
}

// SystemCheckSetPolicy sets the system checker properties.
func SystemCheckSetPolicy(ctx context.Context, rpcClient UnaryInvoker, req *SystemCheckSetPolicyReq) error {
	if req == nil {
		return errors.Errorf("nil %T", req)
	}

	req.CheckSetPolicyReq.Sys = req.getSystem(rpcClient)
	for _, p := range req.Policies {
		req.CheckSetPolicyReq.Policies = append(req.CheckSetPolicyReq.Policies, p.toPB())
	}
	req.setRPC(func(ctx context.Context, conn *grpc.ClientConn) (proto.Message, error) {
		return mgmtpb.NewMgmtSvcClient(conn).SystemCheckSetPolicy(ctx, &req.CheckSetPolicyReq)
	})
	rpcClient.Debugf("DAOS system check set-policy request: %+v", req)

	ur, err := rpcClient.InvokeUnaryRPC(ctx, req)
	if err != nil {
		return err
	}
	ms, err := ur.getMSResponse()
	if err != nil {
		return err
	}
	rpcClient.Debugf("DAOS system check get-policy response: %+v", ms)

	return nil
}

type SystemCheckRepairReq struct {
	unaryRequest
	msRequest

	mgmtpb.CheckActReq
}

func (r *SystemCheckRepairReq) SetAction(action int32) error {
	if _, ok := chkpb.CheckInconsistAction_name[action]; !ok {
		return errors.Errorf("invalid action %d", action)
	}
	r.Act = chkpb.CheckInconsistAction(action)
	return nil
}

// SystemCheckRepair sends a request to the system checker to indicate
// what the desired repair action is for a reported inconsistency.
func SystemCheckRepair(ctx context.Context, rpcClient UnaryInvoker, req *SystemCheckRepairReq) error {
	if req == nil {
		return errors.Errorf("nil %T", req)
	}

	req.setRPC(func(ctx context.Context, conn *grpc.ClientConn) (proto.Message, error) {
		req.CheckActReq.Sys = req.getSystem(rpcClient)
		return mgmtpb.NewMgmtSvcClient(conn).SystemCheckRepair(ctx, &req.CheckActReq)
	})
	rpcClient.Debugf("DAOS system check repair request: %+v", req)

	ur, err := rpcClient.InvokeUnaryRPC(ctx, req)
	if err != nil {
		return err
	}
	msResp, err := ur.getMSResponse()
	if err != nil {
		return err
	}
	rpcClient.Debugf("DAOS system check repair response: %+v", msResp)

	return nil
}
