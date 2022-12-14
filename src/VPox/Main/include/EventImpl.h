/* $Id: EventImpl.h $ */
/** @file
 * VirtualPox COM IEvent implementation
 */

/*
 * Copyright (C) 2010-2020 Oracle Corporation
 *
 * This file is part of VirtualPox Open Source Edition (OSE), as
 * available from http://www.virtualpox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualPox OSE distribution. VirtualPox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef MAIN_INCLUDED_EventImpl_h
#define MAIN_INCLUDED_EventImpl_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include "EventWrap.h"
#include "EventSourceWrap.h"
#include "VetoEventWrap.h"


class ATL_NO_VTABLE VPoxEvent :
    public EventWrap
{
public:
    DECLARE_EMPTY_CTOR_DTOR(VPoxEvent)

    HRESULT FinalConstruct();
    void FinalRelease();

    // public initializer/uninitializer for internal purposes only
    HRESULT init(IEventSource *aSource, VPoxEventType_T aType, BOOL aWaitable);
    void uninit();

private:
    // wrapped IEvent properties
    HRESULT getType(VPoxEventType_T *aType);
    HRESULT getSource(ComPtr<IEventSource> &aSource);
    HRESULT getWaitable(BOOL *aWaitable);

    // wrapped IEvent methods
    HRESULT setProcessed();
    HRESULT waitProcessed(LONG aTimeout, BOOL *aResult);

    struct Data;
    Data* m;
};


class ATL_NO_VTABLE VPoxVetoEvent :
    public VetoEventWrap
{
public:
    DECLARE_EMPTY_CTOR_DTOR(VPoxVetoEvent)

    HRESULT FinalConstruct();
    void FinalRelease();

    // public initializer/uninitializer for internal purposes only
    HRESULT init(IEventSource *aSource, VPoxEventType_T aType);
    void uninit();

private:
    // wrapped IEvent properties
    HRESULT getType(VPoxEventType_T *aType);
    HRESULT getSource(ComPtr<IEventSource> &aSource);
    HRESULT getWaitable(BOOL *aWaitable);

    // wrapped IEvent methods
    HRESULT setProcessed();
    HRESULT waitProcessed(LONG aTimeout, BOOL *aResult);

    // wrapped IVetoEvent methods
    HRESULT addVeto(const com::Utf8Str &aReason);
    HRESULT isVetoed(BOOL *aResult);
    HRESULT getVetos(std::vector<com::Utf8Str> &aResult);
    HRESULT addApproval(const com::Utf8Str &aReason);
    HRESULT isApproved(BOOL *aResult);
    HRESULT getApprovals(std::vector<com::Utf8Str> &aResult);

    struct Data;
    Data* m;
};

class ATL_NO_VTABLE EventSource :
    public EventSourceWrap
{
public:
    DECLARE_EMPTY_CTOR_DTOR(EventSource)

    HRESULT FinalConstruct();
    void FinalRelease();

    // public initializer/uninitializer for internal purposes only
    HRESULT init();
    void uninit();

private:
    // wrapped IEventSource methods
    HRESULT createListener(ComPtr<IEventListener> &aListener);
    HRESULT createAggregator(const std::vector<ComPtr<IEventSource> > &aSubordinates,
                             ComPtr<IEventSource> &aResult);
    HRESULT registerListener(const ComPtr<IEventListener> &aListener,
                             const std::vector<VPoxEventType_T> &aInteresting,
                             BOOL aActive);
    HRESULT unregisterListener(const ComPtr<IEventListener> &aListener);
    HRESULT fireEvent(const ComPtr<IEvent> &aEvent,
                      LONG aTimeout,
                      BOOL *aResult);
    HRESULT getEvent(const ComPtr<IEventListener> &aListener,
                     LONG aTimeout,
                     ComPtr<IEvent> &aEvent);
    HRESULT eventProcessed(const ComPtr<IEventListener> &aListener,
                           const ComPtr<IEvent> &aEvent);


    struct Data;
    Data* m;

    friend class ListenerRecord;
};

class VPoxEventDesc
{
public:
    VPoxEventDesc() : mEvent(0), mEventSource(0)
    {}

    ~VPoxEventDesc()
    {}

    /**
     * This function to be used with some care, as arguments order must match
     * attribute declaration order event class and its superclasses up to
     * IEvent. If unsure, consult implementation in generated VPoxEvents.cpp.
     */
    HRESULT init(IEventSource* aSource, VPoxEventType_T aType, ...);

    /**
    * Function similar to the above, but assumes that init() for this type
    * already called once, so no need to allocate memory, and only reinit
    * fields. Assumes event is subtype of IReusableEvent, asserts otherwise.
    */
    HRESULT reinit(VPoxEventType_T aType, ...);

    void uninit()
    {
        mEvent.setNull();
        mEventSource.setNull();
    }

    void getEvent(IEvent **aEvent)
    {
        mEvent.queryInterfaceTo(aEvent);
    }

    BOOL fire(LONG aTimeout)
    {
        if (mEventSource && mEvent)
        {
            BOOL fDelivered = FALSE;
            int rc = mEventSource->FireEvent(mEvent, aTimeout, &fDelivered);
            AssertRCReturn(rc, FALSE);
            return fDelivered;
        }
        return FALSE;
    }

private:
    ComPtr<IEvent>          mEvent;
    ComPtr<IEventSource>    mEventSource;
};


#endif /* !MAIN_INCLUDED_EventImpl_h */
