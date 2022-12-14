/* $Id: */
/** @file
 * VPoxWatchdogUtils - Misc. utility functions for modules.
 */

/*
 * Copyright (C) 2011-2020 Oracle Corporation
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
#include <VPox/com/array.h>
#include "VPoxWatchdogInternal.h"


/**
 * Adds a group / a set of groups to the specified map.
 * If a group in the group map exists there will be no action.
 *
 * @return  IPRT status code.
 * @param   groups                  Map to add group(s) to.
 * @param   pszGroupsToAdd          Comma-separated string of one or more groups to add.
 * @param   fFlags                  Flags to set to the groups added.
 */
int groupAdd(mapGroups &groups, const char *pszGroupsToAdd, uint32_t fFlags)
{
    AssertPtrReturn(pszGroupsToAdd, VERR_INVALID_POINTER);

    try
    {
        std::istringstream strGroups(pszGroupsToAdd);
        for(std::string strToken; getline(strGroups, strToken, ','); )
        {
            strToken.erase(remove_if(strToken.begin(), strToken.end(), isspace), strToken.end());

            Utf8Str strTokenUtf8(strToken.c_str());
            mapGroupsIterConst it = groups.find(strTokenUtf8);

            if (it == groups.end())
                groups.insert(std::make_pair(strTokenUtf8, fFlags));
        }
    }
    catch (...)
    {
        AssertFailed();
    }

    return VINF_SUCCESS;
}

/**
 * Retrieves a metric from a specified machine.
 *
 * @return  IPRT status code.
 * @param   pMachine                Pointer to the machine's internal structure.
 * @param   strName                 Name of metric to retrieve.
 * @param   pulData                 Pointer to value to retrieve the actual metric value.
 */
int getMetric(PVPOXWATCHDOG_MACHINE pMachine, const Bstr& strName, LONG *pulData)
{
    AssertPtrReturn(pMachine, VERR_INVALID_POINTER);
    AssertPtrReturn(pulData, VERR_INVALID_POINTER);

    /* Input. */
    com::SafeArray<BSTR> metricNames(1);
    com::SafeIfaceArray<IUnknown> metricObjects(1);
    pMachine->machine.queryInterfaceTo(&metricObjects[0]);

    /* Output. */
    com::SafeArray<BSTR>          retNames;
    com::SafeIfaceArray<IUnknown> retObjects;
    com::SafeArray<BSTR>          retUnits;
    com::SafeArray<ULONG>         retScales;
    com::SafeArray<ULONG>         retSequenceNumbers;
    com::SafeArray<ULONG>         retIndices;
    com::SafeArray<ULONG>         retLengths;
    com::SafeArray<LONG>          retData;

    /* Query current memory free. */
    strName.cloneTo(&metricNames[0]);
#ifdef VPOX_WATCHDOG_GLOBAL_PERFCOL
    Assert(!g_pPerfCollector.isNull());
    HRESULT hrc = g_pPerfCollector->QueryMetricsData(
#else
    Assert(!pMachine->collector.isNull());
    HRESULT hrc = pMachine->collector->QueryMetricsData(
#endif
                                                ComSafeArrayAsInParam(metricNames),
                                                ComSafeArrayAsInParam(metricObjects),
                                                ComSafeArrayAsOutParam(retNames),
                                                ComSafeArrayAsOutParam(retObjects),
                                                ComSafeArrayAsOutParam(retUnits),
                                                ComSafeArrayAsOutParam(retScales),
                                                ComSafeArrayAsOutParam(retSequenceNumbers),
                                                ComSafeArrayAsOutParam(retIndices),
                                                ComSafeArrayAsOutParam(retLengths),
                                                ComSafeArrayAsOutParam(retData));
#if 0
    /* Useful for metrics debugging. */
    for (unsigned j = 0; j < retNames.size(); j++)
    {
        Bstr metricUnit(retUnits[j]);
        Bstr metricName(retNames[j]);
        RTPrintf("%-20ls ", metricName.raw());
        const char *separator = "";
        for (unsigned k = 0; k < retLengths[j]; k++)
        {
            if (retScales[j] == 1)
                RTPrintf("%s%d %ls", separator, retData[retIndices[j] + k], metricUnit.raw());
            else
                RTPrintf("%s%d.%02d%ls", separator, retData[retIndices[j] + k] / retScales[j],
                         (retData[retIndices[j] + k] * 100 / retScales[j]) % 100, metricUnit.raw());
            separator = ", ";
        }
        RTPrintf("\n");
    }
#endif

    if (SUCCEEDED(hrc))
        *pulData = retData.size() ? retData[retIndices[0]] : 0;

    return SUCCEEDED(hrc) ? VINF_SUCCESS : VINF_NOT_SUPPORTED;
}

/**
 * Returns the payload of a machine.
 *
 * @return  void*                   Pointer to payload data. Mutable!
 * @param   pMachine                Machine to get payload for.
 * @param   pszModule               Module name to get payload from.
 */
void* payloadFrom(PVPOXWATCHDOG_MACHINE pMachine, const char *pszModule)
{
    AssertPtrReturn(pMachine, NULL);
    AssertPtrReturn(pszModule, NULL);
    mapPayloadIter it = pMachine->payload.find(pszModule);
    if (it == pMachine->payload.end())
        return NULL;
    Assert(it->second.cbData);
    return it->second.pvData;
}

int payloadAlloc(PVPOXWATCHDOG_MACHINE pMachine, const char *pszModule,
                 size_t cbSize, void **ppszPayload)
{
    AssertPtrReturn(pMachine, VERR_INVALID_POINTER);
    AssertPtrReturn(pszModule, VERR_INVALID_POINTER);
    AssertReturn(cbSize, VERR_INVALID_PARAMETER);

    void *pvData = RTMemAllocZ(cbSize);
    AssertPtrReturn(pvData, VERR_NO_MEMORY);

    mapPayloadIter it = pMachine->payload.find(pszModule);
    AssertReturn(it == pMachine->payload.end(), VERR_INVALID_PARAMETER);

    VPOXWATCHDOG_MODULE_PAYLOAD p;
    p.pvData = pvData;
    p.cbData = cbSize;

    if (ppszPayload)
        *ppszPayload = p.pvData;

    pMachine->payload.insert(std::make_pair(pszModule, p));

    return VINF_SUCCESS;
}

void payloadFree(PVPOXWATCHDOG_MACHINE pMachine, const char *pszModule)
{
    AssertPtrReturnVoid(pMachine);
    AssertPtrReturnVoid(pszModule);

    mapPayloadIter it = pMachine->payload.find(pszModule);
    if (it != pMachine->payload.end())
    {
        RTMemFree(it->second.pvData);
        pMachine->payload.erase(it);
    }
}

PVPOXWATCHDOG_MACHINE getMachine(const Bstr& strUuid)
{
    mapVMIter it = g_mapVM.find(strUuid);
    if (it != g_mapVM.end())
        return &it->second;
    return NULL;
}

MachineState_T getMachineState(const PVPOXWATCHDOG_MACHINE pMachine)
{
    AssertPtrReturn(pMachine, MachineState_Null);
    MachineState_T machineState;
    Assert(!pMachine->machine.isNull());
    HRESULT rc = pMachine->machine->COMGETTER(State)(&machineState);
    if (SUCCEEDED(rc))
        return machineState;
    return MachineState_Null;
}

int cfgGetValueStr(const ComPtr<IVirtualPox> &rptrVPox, const ComPtr<IMachine> &rptrMachine,
                   const char *pszGlobal, const char *pszVM, Utf8Str &strValue, Utf8Str strDefault)
{
    AssertReturn(!rptrVPox.isNull(), VERR_INVALID_POINTER);


    /* Try per-VM approach. */
    Bstr strTemp;
    HRESULT hr;
    if (!rptrMachine.isNull())
    {
        AssertPtr(pszVM);
        hr = rptrMachine->GetExtraData(Bstr(pszVM).raw(),
                                       strTemp.asOutParam());
        if (   SUCCEEDED(hr)
            && !strTemp.isEmpty())
        {
            strValue = Utf8Str(strTemp);
        }
    }

    if (strValue.isEmpty()) /* Not set by per-VM value? */
    {
        AssertPtr(pszGlobal);

        /* Try global approach. */
        hr = rptrVPox->GetExtraData(Bstr(pszGlobal).raw(),
                                    strTemp.asOutParam());
        if (   SUCCEEDED(hr)
            && !strTemp.isEmpty())
        {
            strValue = Utf8Str(strTemp);
        }
    }

    if (strValue.isEmpty())
    {
        strValue = strDefault;
        return VERR_NOT_FOUND;
    }

    return VINF_SUCCESS;
}

int cfgGetValueU32(const ComPtr<IVirtualPox> &rptrVPox, const ComPtr<IMachine> &rptrMachine,
                   const char *pszGlobal, const char *pszVM, uint32_t *puValue, uint32_t uDefault)
{
    Utf8Str strValue;
    int rc = cfgGetValueStr(rptrVPox, rptrMachine, pszGlobal, pszVM, strValue, "" /* Default */);
    if (RT_SUCCESS(rc))
        *puValue = strValue.toUInt32();
    else
        *puValue = uDefault;
    return rc;
}

