/* $Id: EMR3Nem.cpp $ */
/** @file
 * EM - Execution Monitor / Manager - NEM interface.
 */

/*
 * Copyright (C) 2006-2020 Oracle Corporation
 *
 * This file is part of VirtualPox Open Source Edition (OSE), as
 * available from http://www.virtualpox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualPox OSE distribution. VirtualPox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#define LOG_GROUP LOG_GROUP_EM
#define VMCPU_INCL_CPUM_GST_CTX
#include <VPox/vmm/em.h>
#include <VPox/vmm/vmm.h>
#include <VPox/vmm/selm.h>
#include <VPox/vmm/trpm.h>
#include <VPox/vmm/iem.h>
#include <VPox/vmm/iom.h>
#include <VPox/vmm/nem.h>
#include <VPox/vmm/dbgf.h>
#include <VPox/vmm/pgm.h>
#include <VPox/vmm/tm.h>
#include <VPox/vmm/mm.h>
#include <VPox/vmm/ssm.h>
#include <VPox/vmm/pdmapi.h>
#include <VPox/vmm/pdmcritsect.h>
#include <VPox/vmm/pdmqueue.h>
#include "EMInternal.h"
#include <VPox/vmm/vm.h>
#include <VPox/vmm/gim.h>
#include <VPox/vmm/cpumdis.h>
#include <VPox/dis.h>
#include <VPox/disopcode.h>
#include <VPox/err.h>
#include <VPox/vmm/dbgf.h>
#include "VMMTracing.h"

#include <iprt/asm.h>


/*********************************************************************************************************************************
*   Internal Functions                                                                                                           *
*********************************************************************************************************************************/
static int      emR3NemHandleRC(PVM pVM, PVMCPU pVCpu, int rc);
DECLINLINE(int) emR3NemExecuteInstruction(PVM pVM, PVMCPU pVCpu, const char *pszPrefix, int rcGC = VINF_SUCCESS);
static int      emR3NemExecuteIOInstruction(PVM pVM, PVMCPU pVCpu);
static int      emR3NemForcedActions(PVM pVM, PVMCPU pVCpu);

#define EMHANDLERC_WITH_NEM
#define emR3ExecuteInstruction   emR3NemExecuteInstruction
#define emR3ExecuteIOInstruction emR3NemExecuteIOInstruction
#include "EMHandleRCTmpl.h"


/**
 * Executes instruction in NEM mode if we can.
 *
 * This is somewhat comparable to REMR3EmulateInstruction.
 *
 * @returns VPox strict status code.
 * @retval  VINF_EM_DBG_STEPPED on success.
 * @retval  VERR_EM_CANNOT_EXEC_GUEST if we cannot execute guest instructions in
 *          HM right now.
 *
 * @param   pVM     The cross context VM structure.
 * @param   pVCpu   The cross context virtual CPU structure for the calling EMT.
 * @param   fFlags  Combinations of EM_ONE_INS_FLAGS_XXX.
 * @thread  EMT.
 */
VPOXSTRICTRC emR3NemSingleInstruction(PVM pVM, PVMCPU pVCpu, uint32_t fFlags)
{
    Assert(!(fFlags & ~EM_ONE_INS_FLAGS_MASK));

    if (!NEMR3CanExecuteGuest(pVM, pVCpu))
        return VINF_EM_RESCHEDULE;

    uint64_t const uOldRip = pVCpu->cpum.GstCtx.rip;
    for (;;)
    {
        /*
         * Service necessary FFs before going into HM.
         */
        if (   VM_FF_IS_ANY_SET(pVM, VM_FF_HIGH_PRIORITY_PRE_RAW_MASK)
            || VMCPU_FF_IS_ANY_SET(pVCpu, VMCPU_FF_HIGH_PRIORITY_PRE_RAW_MASK))
        {
            VPOXSTRICTRC rcStrict = emR3NemForcedActions(pVM, pVCpu);
            if (rcStrict != VINF_SUCCESS)
            {
                Log(("emR3NemSingleInstruction: FFs before -> %Rrc\n", VPOXSTRICTRC_VAL(rcStrict)));
                return rcStrict;
            }
        }

        /*
         * Go execute it.
         */
        bool fOld = NEMR3SetSingleInstruction(pVM, pVCpu, true);
        VPOXSTRICTRC rcStrict = NEMR3RunGC(pVM, pVCpu);
        NEMR3SetSingleInstruction(pVM, pVCpu, fOld);
        LogFlow(("emR3NemSingleInstruction: %Rrc\n", VPOXSTRICTRC_VAL(rcStrict)));

        /*
         * Handle high priority FFs and informational status codes.  We don't do
         * normal FF processing the caller or the next call can deal with them.
         */
        VMCPU_FF_CLEAR_MASK(pVCpu, VMCPU_FF_RESUME_GUEST_MASK);
        if (   VM_FF_IS_ANY_SET(pVM, VM_FF_HIGH_PRIORITY_POST_MASK)
            || VMCPU_FF_IS_ANY_SET(pVCpu, VMCPU_FF_HIGH_PRIORITY_POST_MASK))
        {
            rcStrict = emR3HighPriorityPostForcedActions(pVM, pVCpu, rcStrict);
            LogFlow(("emR3NemSingleInstruction: FFs after -> %Rrc\n", VPOXSTRICTRC_VAL(rcStrict)));
        }

        if (rcStrict != VINF_SUCCESS && (rcStrict < VINF_EM_FIRST || rcStrict > VINF_EM_LAST))
        {
            rcStrict = emR3NemHandleRC(pVM, pVCpu, VPOXSTRICTRC_TODO(rcStrict));
            Log(("emR3NemSingleInstruction: emR3NemHandleRC -> %Rrc\n", VPOXSTRICTRC_VAL(rcStrict)));
        }

        /*
         * Done?
         */
        CPUM_ASSERT_NOT_EXTRN(pVCpu, CPUMCTX_EXTRN_RIP);
        if (   (rcStrict != VINF_SUCCESS && rcStrict != VINF_EM_DBG_STEPPED)
            || !(fFlags & EM_ONE_INS_FLAGS_RIP_CHANGE)
            || pVCpu->cpum.GstCtx.rip != uOldRip)
        {
            if (rcStrict == VINF_SUCCESS && pVCpu->cpum.GstCtx.rip != uOldRip)
                rcStrict = VINF_EM_DBG_STEPPED;
            Log(("emR3NemSingleInstruction: returns %Rrc (rip %llx -> %llx)\n",
                 VPOXSTRICTRC_VAL(rcStrict), uOldRip, pVCpu->cpum.GstCtx.rip));
            CPUM_IMPORT_EXTRN_RET(pVCpu, ~CPUMCTX_EXTRN_KEEPER_MASK);
            return rcStrict;
        }
    }
}


/**
 * Executes one (or perhaps a few more) instruction(s).
 *
 * @returns VPox status code suitable for EM.
 *
 * @param   pVM         The cross context VM structure.
 * @param   pVCpu       The cross context virtual CPU structure.
 * @param   rcRC        Return code from RC.
 * @param   pszPrefix   Disassembly prefix. If not NULL we'll disassemble the
 *                      instruction and prefix the log output with this text.
 */
#if defined(LOG_ENABLED) || defined(DOXYGEN_RUNNING)
static int emR3NemExecuteInstructionWorker(PVM pVM, PVMCPU pVCpu, int rcRC, const char *pszPrefix)
#else
static int emR3NemExecuteInstructionWorker(PVM pVM, PVMCPU pVCpu, int rcRC)
#endif
{
    NOREF(rcRC);

#ifdef LOG_ENABLED
    /*
     * Log it.
     */
    Log(("EMINS: %04x:%RGv RSP=%RGv\n", pVCpu->cpum.GstCtx.cs.Sel, (RTGCPTR)pVCpu->cpum.GstCtx.rip, (RTGCPTR)pVCpu->cpum.GstCtx.rsp));
    if (pszPrefix)
    {
        DBGFR3_INFO_LOG(pVM, pVCpu, "cpumguest", pszPrefix);
        DBGFR3_DISAS_INSTR_CUR_LOG(pVCpu, pszPrefix);
    }
#endif

    /*
     * Use IEM and fallback on REM if the functionality is missing.
     * Once IEM gets mature enough, nothing should ever fall back.
     */
    STAM_PROFILE_START(&pVCpu->em.s.StatIEMEmu, a);

    VPOXSTRICTRC rcStrict;
    uint32_t     idxContinueExitRec = pVCpu->em.s.idxContinueExitRec;
    RT_UNTRUSTED_NONVOLATILE_COPY_FENCE();
    if (idxContinueExitRec >= RT_ELEMENTS(pVCpu->em.s.aExitRecords))
    {
        CPUM_IMPORT_EXTRN_RET(pVCpu, IEM_CPUMCTX_EXTRN_MUST_MASK);
        rcStrict = IEMExecOne(pVCpu);
    }
    else
    {
        RT_UNTRUSTED_VALIDATED_FENCE();
        rcStrict = EMHistoryExec(pVCpu, &pVCpu->em.s.aExitRecords[idxContinueExitRec], 0);
        LogFlow(("emR3NemExecuteInstruction: %Rrc (EMHistoryExec)\n", VPOXSTRICTRC_VAL(rcStrict)));
    }

    STAM_PROFILE_STOP(&pVCpu->em.s.StatIEMEmu, a);

    NOREF(pVM);
    return VPOXSTRICTRC_TODO(rcStrict);
}


/**
 * Executes one (or perhaps a few more) instruction(s).
 * This is just a wrapper for discarding pszPrefix in non-logging builds.
 *
 * @returns VPox status code suitable for EM.
 * @param   pVM         The cross context VM structure.
 * @param   pVCpu       The cross context virtual CPU structure.
 * @param   pszPrefix   Disassembly prefix. If not NULL we'll disassemble the
 *                      instruction and prefix the log output with this text.
 * @param   rcGC        GC return code
 */
DECLINLINE(int) emR3NemExecuteInstruction(PVM pVM, PVMCPU pVCpu, const char *pszPrefix, int rcGC)
{
#ifdef LOG_ENABLED
    return emR3NemExecuteInstructionWorker(pVM, pVCpu, rcGC, pszPrefix);
#else
    RT_NOREF_PV(pszPrefix);
    return emR3NemExecuteInstructionWorker(pVM, pVCpu, rcGC);
#endif
}

/**
 * Executes one (or perhaps a few more) IO instruction(s).
 *
 * @returns VPox status code suitable for EM.
 * @param   pVM         The cross context VM structure.
 * @param   pVCpu       The cross context virtual CPU structure.
 */
static int emR3NemExecuteIOInstruction(PVM pVM, PVMCPU pVCpu)
{
    RT_NOREF_PV(pVM);
    STAM_PROFILE_START(&pVCpu->em.s.StatIOEmu, a);

    /*
     * Hand it over to the interpreter.
     */
    CPUM_IMPORT_EXTRN_RET(pVCpu, IEM_CPUMCTX_EXTRN_MUST_MASK);
    VPOXSTRICTRC rcStrict;
    uint32_t     idxContinueExitRec = pVCpu->em.s.idxContinueExitRec;
    RT_UNTRUSTED_NONVOLATILE_COPY_FENCE();
    if (idxContinueExitRec >= RT_ELEMENTS(pVCpu->em.s.aExitRecords))
    {
        rcStrict = IEMExecOne(pVCpu);
        LogFlow(("emR3NemExecuteIOInstruction: %Rrc (IEMExecOne)\n", VPOXSTRICTRC_VAL(rcStrict)));
        STAM_COUNTER_INC(&pVCpu->em.s.CTX_SUFF(pStats)->StatIoIem);
    }
    else
    {
        RT_UNTRUSTED_VALIDATED_FENCE();
        rcStrict = EMHistoryExec(pVCpu, &pVCpu->em.s.aExitRecords[idxContinueExitRec], 0);
        LogFlow(("emR3NemExecuteIOInstruction: %Rrc (EMHistoryExec)\n", VPOXSTRICTRC_VAL(rcStrict)));
        STAM_COUNTER_INC(&pVCpu->em.s.CTX_SUFF(pStats)->StatIoRestarted);
    }

    STAM_PROFILE_STOP(&pVCpu->em.s.StatIOEmu, a);
    return VPOXSTRICTRC_TODO(rcStrict);
}


/**
 * Process NEM specific forced actions.
 *
 * This function is called when any FFs in VM_FF_HIGH_PRIORITY_PRE_RAW_MASK
 * or/and VMCPU_FF_HIGH_PRIORITY_PRE_RAW_MASK are pending.
 *
 * @returns VPox status code. May return VINF_EM_NO_MEMORY but none of the other
 *          EM statuses.
 * @param   pVM         The cross context VM structure.
 * @param   pVCpu       The cross context virtual CPU structure.
 */
static int emR3NemForcedActions(PVM pVM, PVMCPU pVCpu)
{
    /*
     * Sync page directory should not happen in NEM mode.
     */
    if (VMCPU_FF_IS_ANY_SET(pVCpu, VMCPU_FF_PGM_SYNC_CR3 | VMCPU_FF_PGM_SYNC_CR3_NON_GLOBAL))
    {
        Log(("NEM: TODO: Make VMCPU_FF_PGM_SYNC_CR3 / VMCPU_FF_PGM_SYNC_CR3_NON_GLOBAL quiet! (%#RX64)\n", (uint64_t)pVCpu->fLocalForcedActions));
        VMCPU_FF_CLEAR_MASK(pVCpu, VMCPU_FF_PGM_SYNC_CR3 | VMCPU_FF_PGM_SYNC_CR3_NON_GLOBAL);
    }

    /*
     * Allocate handy pages (just in case the above actions have consumed some pages).
     */
    if (VM_FF_IS_PENDING_EXCEPT(pVM, VM_FF_PGM_NEED_HANDY_PAGES, VM_FF_PGM_NO_MEMORY))
    {
        int rc = PGMR3PhysAllocateHandyPages(pVM);
        if (RT_FAILURE(rc))
            return rc;
    }

    /*
     * Check whether we're out of memory now.
     *
     * This may stem from some of the above actions or operations that has been executed
     * since we ran FFs. The allocate handy pages must for instance always be followed by
     * this check.
     */
    if (VM_FF_IS_SET(pVM, VM_FF_PGM_NO_MEMORY))
        return VINF_EM_NO_MEMORY;

    return VINF_SUCCESS;
}


/**
 * Executes hardware accelerated raw code. (Intel VT-x & AMD-V)
 *
 * This function contains the raw-mode version of the inner
 * execution loop (the outer loop being in EMR3ExecuteVM()).
 *
 * @returns VPox status code. The most important ones are: VINF_EM_RESCHEDULE, VINF_EM_RESCHEDULE_RAW,
 *          VINF_EM_RESCHEDULE_REM, VINF_EM_SUSPEND, VINF_EM_RESET and VINF_EM_TERMINATE.
 *
 * @param   pVM         The cross context VM structure.
 * @param   pVCpu       The cross context virtual CPU structure.
 * @param   pfFFDone    Where to store an indicator telling whether or not
 *                      FFs were done before returning.
 */
VPOXSTRICTRC emR3NemExecute(PVM pVM, PVMCPU pVCpu, bool *pfFFDone)
{
    VPOXSTRICTRC rcStrict = VERR_IPE_UNINITIALIZED_STATUS;

    LogFlow(("emR3NemExecute%d: (cs:eip=%04x:%RGv)\n", pVCpu->idCpu, pVCpu->cpum.GstCtx.cs.Sel, (RTGCPTR)pVCpu->cpum.GstCtx.rip));
    *pfFFDone = false;

    STAM_REL_COUNTER_INC(&pVCpu->em.s.StatNEMExecuteCalled);

    /*
     * Spin till we get a forced action which returns anything but VINF_SUCCESS.
     */
    for (;;)
    {
        STAM_PROFILE_ADV_START(&pVCpu->em.s.StatNEMEntry, a);

        /*
         * Check that we can execute in NEM mode.
         */
        if (NEMR3CanExecuteGuest(pVM, pVCpu))
        { /* likely */ }
        else
        {
            rcStrict = VINF_EM_RESCHEDULE_REM;
            break;
        }

        /*
         * Process high priority pre-execution raw-mode FFs.
         */
        if (   VM_FF_IS_ANY_SET(pVM, VM_FF_HIGH_PRIORITY_PRE_RAW_MASK)
            || VMCPU_FF_IS_ANY_SET(pVCpu, VMCPU_FF_HIGH_PRIORITY_PRE_RAW_MASK))
        {
            rcStrict = emR3NemForcedActions(pVM, pVCpu);
            if (rcStrict != VINF_SUCCESS)
                break;
        }

#ifdef LOG_ENABLED
        /*
         * Log important stuff before entering GC.
         */
        if (TRPMHasTrap(pVCpu))
            Log(("CPU%d: Pending hardware interrupt=0x%x cs:rip=%04X:%RGv\n", pVCpu->idCpu, TRPMGetTrapNo(pVCpu), pVCpu->cpum.GstCtx.cs.Sel, (RTGCPTR)pVCpu->cpum.GstCtx.rip));

        if (!(pVCpu->cpum.GstCtx.fExtrn & (  CPUMCTX_EXTRN_RIP | CPUMCTX_EXTRN_CS  | CPUMCTX_EXTRN_RFLAGS | CPUMCTX_EXTRN_SS
                                           | CPUMCTX_EXTRN_RSP | CPUMCTX_EXTRN_CR0 | CPUMCTX_EXTRN_CR4    | CPUMCTX_EXTRN_EFER)))
        {
            uint32_t cpl = CPUMGetGuestCPL(pVCpu);
            if (pVM->cCpus == 1)
            {
                if (pVCpu->cpum.GstCtx.eflags.Bits.u1VM)
                    Log(("NEMV86: %08x IF=%d\n", pVCpu->cpum.GstCtx.eip, pVCpu->cpum.GstCtx.eflags.Bits.u1IF));
                else if (CPUMIsGuestIn64BitCodeEx(&pVCpu->cpum.GstCtx))
                    Log(("NEMR%d: %04x:%RGv ESP=%RGv IF=%d IOPL=%d CR0=%x CR4=%x EFER=%x\n", cpl, pVCpu->cpum.GstCtx.cs.Sel, (RTGCPTR)pVCpu->cpum.GstCtx.rip, pVCpu->cpum.GstCtx.rsp, pVCpu->cpum.GstCtx.eflags.Bits.u1IF, pVCpu->cpum.GstCtx.eflags.Bits.u2IOPL, (uint32_t)pVCpu->cpum.GstCtx.cr0, (uint32_t)pVCpu->cpum.GstCtx.cr4, (uint32_t)pVCpu->cpum.GstCtx.msrEFER));
                else
                    Log(("NEMR%d: %04x:%08x ESP=%08X IF=%d IOPL=%d CR0=%x CR4=%x EFER=%x\n", cpl, pVCpu->cpum.GstCtx.cs.Sel,          pVCpu->cpum.GstCtx.eip, pVCpu->cpum.GstCtx.esp, pVCpu->cpum.GstCtx.eflags.Bits.u1IF, pVCpu->cpum.GstCtx.eflags.Bits.u2IOPL, (uint32_t)pVCpu->cpum.GstCtx.cr0, (uint32_t)pVCpu->cpum.GstCtx.cr4, (uint32_t)pVCpu->cpum.GstCtx.msrEFER));
            }
            else
            {
                if (pVCpu->cpum.GstCtx.eflags.Bits.u1VM)
                    Log(("NEMV86-CPU%d: %08x IF=%d\n", pVCpu->idCpu, pVCpu->cpum.GstCtx.eip, pVCpu->cpum.GstCtx.eflags.Bits.u1IF));
                else if (CPUMIsGuestIn64BitCodeEx(&pVCpu->cpum.GstCtx))
                    Log(("NEMR%d-CPU%d: %04x:%RGv ESP=%RGv IF=%d IOPL=%d CR0=%x CR4=%x EFER=%x\n", cpl, pVCpu->idCpu, pVCpu->cpum.GstCtx.cs.Sel, (RTGCPTR)pVCpu->cpum.GstCtx.rip, pVCpu->cpum.GstCtx.rsp, pVCpu->cpum.GstCtx.eflags.Bits.u1IF, pVCpu->cpum.GstCtx.eflags.Bits.u2IOPL, (uint32_t)pVCpu->cpum.GstCtx.cr0, (uint32_t)pVCpu->cpum.GstCtx.cr4, (uint32_t)pVCpu->cpum.GstCtx.msrEFER));
                else
                    Log(("NEMR%d-CPU%d: %04x:%08x ESP=%08X IF=%d IOPL=%d CR0=%x CR4=%x EFER=%x\n", cpl, pVCpu->idCpu, pVCpu->cpum.GstCtx.cs.Sel,          pVCpu->cpum.GstCtx.eip, pVCpu->cpum.GstCtx.esp, pVCpu->cpum.GstCtx.eflags.Bits.u1IF, pVCpu->cpum.GstCtx.eflags.Bits.u2IOPL, (uint32_t)pVCpu->cpum.GstCtx.cr0, (uint32_t)pVCpu->cpum.GstCtx.cr4, (uint32_t)pVCpu->cpum.GstCtx.msrEFER));
            }
        }
        else if (pVM->cCpus == 1)
            Log(("NEMRx: -> NEMR3RunGC\n"));
        else
            Log(("NEMRx-CPU%u: -> NEMR3RunGC\n",  pVCpu->idCpu));
#endif /* LOG_ENABLED */

        /*
         * Execute the code.
         */
        if (RT_LIKELY(emR3IsExecutionAllowed(pVM, pVCpu)))
        {
            STAM_PROFILE_ADV_STOP(&pVCpu->em.s.StatNEMEntry, a);
            STAM_REL_PROFILE_START(&pVCpu->em.s.StatNEMExec, x);
            rcStrict = NEMR3RunGC(pVM, pVCpu);
            STAM_REL_PROFILE_STOP(&pVCpu->em.s.StatNEMExec, x);
        }
        else
        {
            /* Give up this time slice; virtual time continues */
            STAM_PROFILE_ADV_STOP(&pVCpu->em.s.StatNEMEntry, a);
            STAM_REL_PROFILE_ADV_START(&pVCpu->em.s.StatCapped, u);
            RTThreadSleep(5);
            STAM_REL_PROFILE_ADV_STOP(&pVCpu->em.s.StatCapped, u);
            rcStrict = VINF_SUCCESS;
        }


        /*
         * Deal with high priority post execution FFs before doing anything else.
         */
        VMCPU_FF_CLEAR_MASK(pVCpu, VMCPU_FF_RESUME_GUEST_MASK);
        if (   VM_FF_IS_ANY_SET(pVM, VM_FF_HIGH_PRIORITY_POST_MASK)
            || VMCPU_FF_IS_ANY_SET(pVCpu, VMCPU_FF_HIGH_PRIORITY_POST_MASK))
            rcStrict = emR3HighPriorityPostForcedActions(pVM, pVCpu, rcStrict);

        /*
         * Process the returned status code.
         */
        if (rcStrict >= VINF_EM_FIRST && rcStrict <= VINF_EM_LAST)
            break;

        rcStrict = emR3NemHandleRC(pVM, pVCpu, VPOXSTRICTRC_TODO(rcStrict));
        if (rcStrict != VINF_SUCCESS)
            break;

        /*
         * Check and execute forced actions.
         */
#ifdef VPOX_HIGH_RES_TIMERS_HACK
        TMTimerPollVoid(pVM, pVCpu);
#endif
        if (   VM_FF_IS_ANY_SET(pVM, VM_FF_ALL_MASK)
            || VMCPU_FF_IS_ANY_SET(pVCpu, VMCPU_FF_ALL_MASK))
        {
            rcStrict = emR3ForcedActions(pVM, pVCpu, VPOXSTRICTRC_TODO(rcStrict));
            VPOXVMM_EM_FF_ALL_RET(pVCpu, VPOXSTRICTRC_VAL(rcStrict));
            if (   rcStrict != VINF_SUCCESS
                && rcStrict != VINF_EM_RESCHEDULE_HM)
            {
                *pfFFDone = true;
                break;
            }
        }
    }

    /*
     * Return to outer loop, making sure the fetch all state as we leave.
     *
     * Note! Not using CPUM_IMPORT_EXTRN_RET here, to prioritize an rcStrict error
     *       status over import errors.
     */
    if (pVCpu->cpum.GstCtx.fExtrn)
    {
        int rcImport = NEMImportStateOnDemand(pVCpu, pVCpu->cpum.GstCtx.fExtrn);
        AssertReturn(RT_SUCCESS(rcImport) || RT_FAILURE_NP(rcStrict), rcImport);
    }
#if defined(LOG_ENABLED) && defined(DEBUG)
    RTLogFlush(NULL);
#endif
    return rcStrict;
}

