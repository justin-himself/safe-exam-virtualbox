/*-
 * Copyright (c) 2004, 2005,
 * 	Bosko Milekic <bmilekic@FreeBSD.org>.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice unmodified, this list of conditions and the following
 *    disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef VPOX
#include <sys/cdefs.h>
__FBSDID("$FreeBSD: src/sys/kern/kern_mbuf.c,v 1.32.2.5.2.1 2009/04/15 03:14:26 kensmith Exp $");

#include "opt_mac.h"
#include "opt_param.h"

#include <sys/param.h>
#include <sys/malloc.h>
#include <sys/systm.h>
#include <sys/mbuf.h>
#include <sys/domain.h>
#include <sys/eventhandler.h>
#include <sys/kernel.h>
#include <sys/protosw.h>
#include <sys/smp.h>
#include <sys/sysctl.h>

#include <security/mac/mac_framework.h>

#include <vm/vm.h>
#include <vm/vm_page.h>
#include <vm/uma.h>
#include <vm/uma_int.h>
#include <vm/uma_dbg.h>
#else
# include <iprt/param.h>
# include <slirp.h>
# define IN_BSD
# include "ext.h"
#endif

/*
 * In FreeBSD, Mbufs and Mbuf Clusters are allocated from UMA
 * Zones.
 *
 * Mbuf Clusters (2K, contiguous) are allocated from the Cluster
 * Zone.  The Zone can be capped at kern.ipc.nmbclusters, if the
 * administrator so desires.
 *
 * Mbufs are allocated from a UMA Master Zone called the Mbuf
 * Zone.
 *
 * Additionally, FreeBSD provides a Packet Zone, which it
 * configures as a Secondary Zone to the Mbuf Master Zone,
 * thus sharing backend Slab kegs with the Mbuf Master Zone.
 *
 * Thus common-case allocations and locking are simplified:
 *
 *  m_clget()                m_getcl()
 *    |                         |
 *    |   .------------>[(Packet Cache)]    m_get(), m_gethdr()
 *    |   |             [     Packet   ]            |
 *  [(Cluster Cache)]   [    Secondary ]   [ (Mbuf Cache)     ]
 *  [ Cluster Zone  ]   [     Zone     ]   [ Mbuf Master Zone ]
 *        |                       \________         |
 *  [ Cluster Keg   ]                      \       /
 *        |    	                         [ Mbuf Keg   ]
 *  [ Cluster Slabs ]                         |
 *        |                              [ Mbuf Slabs ]
 *         \____________(VM)_________________/
 *
 *
 * Whenever an object is allocated with uma_zalloc() out of
 * one of the Zones its _ctor_ function is executed.  The same
 * for any deallocation through uma_zfree() the _dtor_ function
 * is executed.
 *
 * Caches are per-CPU and are filled from the Master Zone.
 *
 * Whenever an object is allocated from the underlying global
 * memory pool it gets pre-initialized with the _zinit_ functions.
 * When the Keg's are overfull objects get decomissioned with
 * _zfini_ functions and free'd back to the global memory pool.
 *
 */

#ifndef VPOX
int nmbclusters;		/* limits number of mbuf clusters */
int nmbjumbop;			/* limits number of page size jumbo clusters */
int nmbjumbo9;			/* limits number of 9k jumbo clusters */
int nmbjumbo16;			/* limits number of 16k jumbo clusters */
struct mbstat mbstat;
#endif

/*
 * tunable_mbinit() has to be run before init_maxsockets() thus
 * the SYSINIT order below is SI_ORDER_MIDDLE while init_maxsockets()
 * runs at SI_ORDER_ANY.
 */
static void
tunable_mbinit(void *dummy)
{
#ifdef VPOX
        PNATState pData = (PNATState)dummy;
#endif
	/* This has to be done before VM init. */
	nmbclusters = 1024 + maxusers * 64;
	nmbjumbop = nmbclusters / 2;
	nmbjumbo9 = nmbjumbop / 2;
	nmbjumbo16 = nmbjumbo9 / 2;
	TUNABLE_INT_FETCH("kern.ipc.nmbclusters", &nmbclusters);
}
SYSINIT(tunable_mbinit, SI_SUB_TUNABLES, SI_ORDER_MIDDLE, tunable_mbinit, NULL);

#ifndef VPOX
/* XXX: These should be tuneables. Can't change UMA limits on the fly. */
static int
sysctl_nmbclusters(SYSCTL_HANDLER_ARGS)
{
	int error, newnmbclusters;

	newnmbclusters = nmbclusters;
	error = sysctl_handle_int(oidp, &newnmbclusters, 0, req);
	if (error == 0 && req->newptr) {
		if (newnmbclusters > nmbclusters) {
			nmbclusters = newnmbclusters;
			uma_zone_set_max(zone_clust, nmbclusters);
			EVENTHANDLER_INVOKE(nmbclusters_change);
		} else
			error = EINVAL;
	}
	return (error);
}
SYSCTL_PROC(_kern_ipc, OID_AUTO, nmbclusters, CTLTYPE_INT|CTLFLAG_RW,
&nmbclusters, 0, sysctl_nmbclusters, "IU",
    "Maximum number of mbuf clusters allowed");

static int
sysctl_nmbjumbop(SYSCTL_HANDLER_ARGS)
{
	int error, newnmbjumbop;

	newnmbjumbop = nmbjumbop;
	error = sysctl_handle_int(oidp, &newnmbjumbop, 0, req);
	if (error == 0 && req->newptr) {
		if (newnmbjumbop> nmbjumbop) {
			nmbjumbop = newnmbjumbop;
			uma_zone_set_max(zone_jumbop, nmbjumbop);
		} else
			error = EINVAL;
	}
	return (error);
}
SYSCTL_PROC(_kern_ipc, OID_AUTO, nmbjumbop, CTLTYPE_INT|CTLFLAG_RW,
&nmbjumbop, 0, sysctl_nmbjumbop, "IU",
	 "Maximum number of mbuf page size jumbo clusters allowed");


static int
sysctl_nmbjumbo9(SYSCTL_HANDLER_ARGS)
{
	int error, newnmbjumbo9;

	newnmbjumbo9 = nmbjumbo9;
	error = sysctl_handle_int(oidp, &newnmbjumbo9, 0, req);
	if (error == 0 && req->newptr) {
		if (newnmbjumbo9> nmbjumbo9) {
			nmbjumbo9 = newnmbjumbo9;
			uma_zone_set_max(zone_jumbo9, nmbjumbo9);
		} else
			error = EINVAL;
	}
	return (error);
}
SYSCTL_PROC(_kern_ipc, OID_AUTO, nmbjumbo9, CTLTYPE_INT|CTLFLAG_RW,
&nmbjumbo9, 0, sysctl_nmbjumbo9, "IU",
	"Maximum number of mbuf 9k jumbo clusters allowed");

static int
sysctl_nmbjumbo16(SYSCTL_HANDLER_ARGS)
{
	int error, newnmbjumbo16;

	newnmbjumbo16 = nmbjumbo16;
	error = sysctl_handle_int(oidp, &newnmbjumbo16, 0, req);
	if (error == 0 && req->newptr) {
		if (newnmbjumbo16> nmbjumbo16) {
			nmbjumbo16 = newnmbjumbo16;
			uma_zone_set_max(zone_jumbo16, nmbjumbo16);
		} else
			error = EINVAL;
	}
	return (error);
}
SYSCTL_PROC(_kern_ipc, OID_AUTO, nmbjumbo16, CTLTYPE_INT|CTLFLAG_RW,
&nmbjumbo16, 0, sysctl_nmbjumbo16, "IU",
    "Maximum number of mbuf 16k jumbo clusters allowed");



SYSCTL_STRUCT(_kern_ipc, OID_AUTO, mbstat, CTLFLAG_RD, &mbstat, mbstat,
    "Mbuf general information and statistics");

/*
 * Zones from which we allocate.
 */
uma_zone_t	zone_mbuf;
uma_zone_t	zone_clust;
uma_zone_t	zone_pack;
uma_zone_t	zone_jumbop;
uma_zone_t	zone_jumbo9;
uma_zone_t	zone_jumbo16;
uma_zone_t	zone_ext_refcnt;

/*
 * Local prototypes.
 */
static int	mb_ctor_mbuf(void *, int, void *, int);
static int	mb_ctor_clust(void *, int, void *, int);
static int	mb_ctor_pack(void *, int, void *, int);
static void	mb_dtor_mbuf(void *, int, void *);
static void	mb_dtor_clust(void *, int, void *);
static void	mb_dtor_pack(void *, int, void *);
static int	mb_zinit_pack(void *, int, int);
static void	mb_zfini_pack(void *, int);
#else
/*
 * Local prototypes.
 */
static int	mb_ctor_mbuf(PNATState, void *, int, void *, int);
static int	mb_ctor_clust(PNATState, void *, int, void *, int);
static int	mb_ctor_pack(PNATState, void *, int, void *, int);
static void	mb_dtor_mbuf(PNATState, void *, int, void *);
static void	mb_dtor_clust(PNATState, void *, int, void *);
static void	mb_dtor_pack(PNATState, void *, int, void *);
static int	mb_zinit_pack(PNATState, void *, int, int);
static void	mb_zfini_pack(PNATState, void *, int);
#endif

/*static void	mb_reclaim(void *); - unused */
#ifndef VPOX
static void	mbuf_init(void *);
static void    *mbuf_jumbo_alloc(uma_zone_t, int, u_int8_t *, int);
static void	mbuf_jumbo_free(void *, int, u_int8_t);
#endif

#ifndef VPOX
static MALLOC_DEFINE(M_JUMBOFRAME, "jumboframes", "mbuf jumbo frame buffers");

/* Ensure that MSIZE doesn't break dtom() - it must be a power of 2 */
CTASSERT((((MSIZE - 1) ^ MSIZE) + 1) >> 1 == MSIZE);
#else
#define uma_zcreate(a0, a1, a2, a3, a4, a5, a6, a7) \
    uma_zcreate(pData, a0, a1, a2, a3, a4, a5, a6, a7)
#endif

/*
 * Initialize FreeBSD Network buffer allocation.
 */
SYSINIT(mbuf, SI_SUB_MBUF, SI_ORDER_FIRST, mbuf_init, NULL);
#ifndef VPOX
static void
#else
void
#endif
mbuf_init(void *dummy)
{

	/*
	 * Configure UMA zones for Mbufs, Clusters, and Packets.
	 */
#ifndef VPOX
	zone_mbuf = uma_zcreate(MBUF_MEM_NAME, MSIZE,
	    mb_ctor_mbuf, mb_dtor_mbuf,
#ifdef INVARIANTS
	    trash_init, trash_fini,
#else
	    NULL, NULL,
#endif
	    MSIZE - 1, UMA_ZONE_MAXBUCKET);

	zone_clust = uma_zcreate(MBUF_CLUSTER_MEM_NAME, MCLBYTES,
	    mb_ctor_clust, mb_dtor_clust,
#ifdef INVARIANTS
	    trash_init, trash_fini,
#else
	    NULL, NULL,
#endif
	    UMA_ALIGN_PTR, UMA_ZONE_REFCNT);
#else /*!VPOX*/
        PNATState pData = (PNATState)dummy;
        tunable_mbinit(pData);
	zone_mbuf = uma_zcreate(MBUF_MEM_NAME, MSIZE,
	    mb_ctor_mbuf, mb_dtor_mbuf,
	    NULL, NULL,
	    MSIZE - 1, UMA_ZONE_MAXBUCKET);
	if (nmbclusters > 0)
		uma_zone_set_max(zone_mbuf, nmbclusters);

	zone_clust = uma_zcreate(MBUF_CLUSTER_MEM_NAME, MCLBYTES,
	    mb_ctor_clust, mb_dtor_clust,
	    NULL, NULL,
	    UMA_ALIGN_PTR, UMA_ZONE_REFCNT);
#endif /*VPOX*/
	if (nmbclusters > 0)
		uma_zone_set_max(zone_clust, nmbclusters);

	zone_pack = uma_zsecond_create(MBUF_PACKET_MEM_NAME, mb_ctor_pack,
	    mb_dtor_pack, mb_zinit_pack, mb_zfini_pack, zone_mbuf);

	/* Make jumbo frame zone too. Page size, 9k and 16k. */
#ifndef VPOX
	zone_jumbop = uma_zcreate(MBUF_JUMBOP_MEM_NAME, MJUMPAGESIZE,
	    mb_ctor_clust, mb_dtor_clust,
#ifdef INVARIANTS
	    trash_init, trash_fini,
#else
	    NULL, NULL,
#endif
	    UMA_ALIGN_PTR, UMA_ZONE_REFCNT);
	if (nmbjumbop > 0)
		uma_zone_set_max(zone_jumbop, nmbjumbop);

	zone_jumbo9 = uma_zcreate(MBUF_JUMBO9_MEM_NAME, MJUM9BYTES,
	    mb_ctor_clust, mb_dtor_clust,
#ifdef INVARIANTS
	    trash_init, trash_fini,
#else
	    NULL, NULL,
#endif
	    UMA_ALIGN_PTR, UMA_ZONE_REFCNT);
	if (nmbjumbo9 > 0)
		uma_zone_set_max(zone_jumbo9, nmbjumbo9);
	uma_zone_set_allocf(zone_jumbo9, mbuf_jumbo_alloc);
	uma_zone_set_freef(zone_jumbo9, mbuf_jumbo_free);

	zone_jumbo16 = uma_zcreate(MBUF_JUMBO16_MEM_NAME, MJUM16BYTES,
	    mb_ctor_clust, mb_dtor_clust,
#ifdef INVARIANTS
	    trash_init, trash_fini,
#else
	    NULL, NULL,
#endif
	    UMA_ALIGN_PTR, UMA_ZONE_REFCNT);
	if (nmbjumbo16 > 0)
		uma_zone_set_max(zone_jumbo16, nmbjumbo16);
#else /*!VPOX*/
	zone_jumbop = uma_zcreate(MBUF_JUMBOP_MEM_NAME, MJUMPAGESIZE,
	    mb_ctor_clust, mb_dtor_clust,
	    NULL, NULL,
	    UMA_ALIGN_PTR, UMA_ZONE_REFCNT);
	if (nmbjumbop > 0)
		uma_zone_set_max(zone_jumbop, nmbjumbop);

	zone_jumbo9 = uma_zcreate(MBUF_JUMBO9_MEM_NAME, MJUM9BYTES,
	    mb_ctor_clust, mb_dtor_clust,
	    NULL, NULL,
	    UMA_ALIGN_PTR, UMA_ZONE_REFCNT);
	if (nmbjumbo9 > 0)
		uma_zone_set_max(zone_jumbo9, nmbjumbo9);

	zone_jumbo16 = uma_zcreate(MBUF_JUMBO16_MEM_NAME, MJUM16BYTES,
	    mb_ctor_clust, mb_dtor_clust,
	    NULL, NULL,
	    UMA_ALIGN_PTR, UMA_ZONE_REFCNT);
	if (nmbjumbo16 > 0)
		uma_zone_set_max(zone_jumbo16, nmbjumbo16);
#endif /*VPOX*/

	zone_ext_refcnt = uma_zcreate(MBUF_EXTREFCNT_MEM_NAME, sizeof(u_int),
	    NULL, NULL,
	    NULL, NULL,
	    UMA_ALIGN_PTR, UMA_ZONE_ZINIT);

	/* uma_prealloc() goes here... */

	/*
	 * Hook event handler for low-memory situation, used to
	 * drain protocols and push data back to the caches (UMA
	 * later pushes it back to VM).
	 */
	EVENTHANDLER_REGISTER(vm_lowmem, mb_reclaim, NULL,
	    EVENTHANDLER_PRI_FIRST);

	/*
	 * [Re]set counters and local statistics knobs.
	 * XXX Some of these should go and be replaced, but UMA stat
	 * gathering needs to be revised.
	 */
	mbstat.m_mbufs = 0;
	mbstat.m_mclusts = 0;
	mbstat.m_drain = 0;
	mbstat.m_msize = MSIZE;
	mbstat.m_mclbytes = MCLBYTES;
	mbstat.m_minclsize = MINCLSIZE;
	mbstat.m_mlen = MLEN;
	mbstat.m_mhlen = MHLEN;
	mbstat.m_numtypes = MT_NTYPES;

	mbstat.m_mcfail = mbstat.m_mpfail = 0;
	mbstat.sf_iocnt = 0;
	mbstat.sf_allocwait = mbstat.sf_allocfail = 0;
}

#ifndef VPOX
/*
 * UMA backend page allocator for the jumbo frame zones.
 *
 * Allocates kernel virtual memory that is backed by contiguous physical
 * pages.
 */
static void *
mbuf_jumbo_alloc(uma_zone_t zone, int bytes, u_int8_t *flags, int fWait)
{

	/* Inform UMA that this allocator uses kernel_map/object. */
	*flags = UMA_SLAB_KERNEL;
	return (contigmalloc(bytes, M_JUMBOFRAME, fWait, (vm_paddr_t)0,
	    ~(vm_paddr_t)0, 1, 0));
}

/*
 * UMA backend page deallocator for the jumbo frame zones.
 */
static void
mbuf_jumbo_free(void *mem, int size, u_int8_t flags)
{

	contigfree(mem, size, M_JUMBOFRAME);
}
#endif

/*
 * Constructor for Mbuf master zone.
 *
 * The 'arg' pointer points to a mb_args structure which
 * contains call-specific information required to support the
 * mbuf allocation API.  See mbuf.h.
 */
static int
#ifndef VPOX
mb_ctor_mbuf(void *mem, int size, void *arg, int how)
#else
mb_ctor_mbuf(PNATState pData, void *mem, int size, void *arg, int how)
#endif
{
	struct mbuf *m;
	struct mb_args *args;
#ifdef MAC
	int error;
#endif
	int flags;
	short type;
#ifdef VPOX
    NOREF(pData);
#endif

#ifdef INVARIANTS
	trash_ctor(mem, size, arg, how);
#elif defined(VPOX)
    NOREF(size);
    NOREF(how);
#endif
	m = (struct mbuf *)mem;
	args = (struct mb_args *)arg;
	flags = args->flags;
	type = args->type;

	/*
	 * The mbuf is initialized later.  The caller has the
	 * responsibility to set up any MAC labels too.
	 */
	if (type == MT_NOINIT)
		return (0);

	m->m_next = NULL;
	m->m_nextpkt = NULL;
	m->m_len = 0;
	m->m_flags = flags;
	m->m_type = type;
	if (flags & M_PKTHDR) {
		m->m_data = m->m_pktdat;
		m->m_pkthdr.rcvif = NULL;
		m->m_pkthdr.len = 0;
		m->m_pkthdr.header = NULL;
		m->m_pkthdr.csum_flags = 0;
		m->m_pkthdr.csum_data = 0;
		m->m_pkthdr.tso_segsz = 0;
		m->m_pkthdr.ether_vtag = 0;
		SLIST_INIT(&m->m_pkthdr.tags);
#ifdef MAC
		/* If the label init fails, fail the alloc */
		error = mac_init_mbuf(m, how);
		if (error)
			return (error);
#endif
	} else
		m->m_data = m->m_dat;
	return (0);
}

/*
 * The Mbuf master zone destructor.
 */
static void
#ifndef VPOX
mb_dtor_mbuf(void *mem, int size, void *arg)
#else
mb_dtor_mbuf(PNATState pData, void *mem, int size, void *arg)
#endif
{
	struct mbuf *m;
	uintptr_t flags;
#ifdef VPOX
	NOREF(pData);
#endif

	m = (struct mbuf *)mem;
	flags = (uintptr_t)arg;

	if ((flags & MB_NOTAGS) == 0 && (m->m_flags & M_PKTHDR) != 0)
		m_tag_delete_chain(m, NULL);
	KASSERT((m->m_flags & M_EXT) == 0, ("%s: M_EXT set", __func__));
	KASSERT((m->m_flags & M_NOFREE) == 0, ("%s: M_NOFREE set", __func__));
#ifdef INVARIANTS
	trash_dtor(mem, size, arg);
#elif defined(VPOX)
	NOREF(size);
	NOREF(arg);
#endif
}

/*
 * The Mbuf Packet zone destructor.
 */
static void
#ifndef VPOX
mb_dtor_pack(void *mem, int size, void *arg)
#else
mb_dtor_pack(PNATState pData, void *mem, int size, void *arg)
#endif
{
	struct mbuf *m;

	m = (struct mbuf *)mem;
	if ((m->m_flags & M_PKTHDR) != 0)
		m_tag_delete_chain(m, NULL);

	/* Make sure we've got a clean cluster back. */
	KASSERT((m->m_flags & M_EXT) == M_EXT, ("%s: M_EXT not set", __func__));
	KASSERT(m->m_ext.ext_buf != NULL, ("%s: ext_buf == NULL", __func__));
	KASSERT(m->m_ext.ext_free == NULL, ("%s: ext_free != NULL", __func__));
	KASSERT(m->m_ext.ext_args == NULL, ("%s: ext_args != NULL", __func__));
	KASSERT(m->m_ext.ext_size == MCLBYTES, ("%s: ext_size != MCLBYTES", __func__));
	KASSERT(m->m_ext.ext_type == EXT_PACKET, ("%s: ext_type != EXT_PACKET", __func__));
	KASSERT(*m->m_ext.ref_cnt == 1, ("%s: ref_cnt != 1", __func__));
#ifdef INVARIANTS
	trash_dtor(m->m_ext.ext_buf, MCLBYTES, arg);
#elif defined(VPOX)
    NOREF(size);
    NOREF(arg);
#endif
	/*
	 * If there are processes blocked on zone_clust, waiting for pages to be freed up,
	 * cause them to be woken up by draining the packet zone. We are exposed to a race here
	 * (in the check for the UMA_ZFLAG_FULL) where we might miss the flag set, but that is
	 * deliberate. We don't want to acquire the zone lock for every mbuf free.
	 */
 	if (uma_zone_exhausted_nolock(zone_clust))
 		zone_drain(zone_pack);
}

/*
 * The Cluster and Jumbo[PAGESIZE|9|16] zone constructor.
 *
 * Here the 'arg' pointer points to the Mbuf which we
 * are configuring cluster storage for.  If 'arg' is
 * empty we allocate just the cluster without setting
 * the mbuf to it.  See mbuf.h.
 */
static int
#ifndef VPOX
mb_ctor_clust(void *mem, int size, void *arg, int how)
#else
mb_ctor_clust(PNATState pData, void *mem, int size, void *arg, int how)
#endif
{
	struct mbuf *m;
	u_int *refcnt;
	int type;
	uma_zone_t zone;
#ifdef VPOX
    NOREF(how);
#endif

#ifdef INVARIANTS
	trash_ctor(mem, size, arg, how);
#elif defined(VPOX)
    NOREF(how);
#endif
	switch (size) {
	case MCLBYTES:
		type = EXT_CLUSTER;
		zone = zone_clust;
		break;
#if MJUMPAGESIZE != MCLBYTES
	case MJUMPAGESIZE:
		type = EXT_JUMBOP;
		zone = zone_jumbop;
		break;
#endif
	case MJUM9BYTES:
		type = EXT_JUMBO9;
		zone = zone_jumbo9;
		break;
	case MJUM16BYTES:
		type = EXT_JUMBO16;
		zone = zone_jumbo16;
		break;
	default:
		panic("unknown cluster size");
		break;
	}

	m = (struct mbuf *)arg;
	refcnt = uma_find_refcnt(zone, mem);
	*refcnt = 1;
	if (m != NULL) {
		m->m_ext.ext_buf = (caddr_t)mem;
		m->m_data = m->m_ext.ext_buf;
		m->m_flags |= M_EXT;
		m->m_ext.ext_free = NULL;
		m->m_ext.ext_args = NULL;
		m->m_ext.ext_size = size;
		m->m_ext.ext_type = type;
		m->m_ext.ref_cnt = refcnt;
	}

	return (0);
}

/*
 * The Mbuf Cluster zone destructor.
 */
static void
#ifndef VPOX
mb_dtor_clust(void *mem, int size, void *arg)
#else
mb_dtor_clust(PNATState pData, void *mem, int size, void *arg)
#endif
{
#ifdef INVARIANTS
	uma_zone_t zone;

	zone = m_getzone(size);
	KASSERT(*(uma_find_refcnt(zone, mem)) <= 1,
		("%s: refcnt incorrect %u", __func__,
		 *(uma_find_refcnt(zone, mem))) );

	trash_dtor(mem, size, arg);
#elif defined(VPOX)
    NOREF(pData);
    NOREF(mem);
    NOREF(size);
    NOREF(arg);
#endif
}

/*
 * The Packet secondary zone's init routine, executed on the
 * object's transition from mbuf keg slab to zone cache.
 */
static int
#ifndef VPOX
mb_zinit_pack(void *mem, int size, int how)
#else
mb_zinit_pack(PNATState pData, void *mem, int size, int how)
#endif
{
	struct mbuf *m;

	m = (struct mbuf *)mem;		/* m is virgin. */
	if (uma_zalloc_arg(zone_clust, m, how) == NULL ||
	    m->m_ext.ext_buf == NULL)
		return (ENOMEM);
	m->m_ext.ext_type = EXT_PACKET;	/* Override. */
#ifdef INVARIANTS
	trash_init(m->m_ext.ext_buf, MCLBYTES, how);
#elif defined(VPOX)
    NOREF(size);
#endif
	return (0);
}

/*
 * The Packet secondary zone's fini routine, executed on the
 * object's transition from zone cache to keg slab.
 */
static void
#ifndef VPOX
mb_zfini_pack(void *mem, int size)
#else
mb_zfini_pack(PNATState pData, void *mem, int size)
#endif
{
	struct mbuf *m;

	m = (struct mbuf *)mem;
#ifdef INVARIANTS
	trash_fini(m->m_ext.ext_buf, MCLBYTES);
#endif
	uma_zfree_arg(zone_clust, m->m_ext.ext_buf, NULL);
#ifdef INVARIANTS
	trash_dtor(mem, size, NULL);
#elif defined(VPOX)
    NOREF(size);
#endif
}

/*
 * The "packet" keg constructor.
 */
static int
#ifndef VPOX
mb_ctor_pack(void *mem, int size, void *arg, int how)
#else
mb_ctor_pack(PNATState pData, void *mem, int size, void *arg, int how)
#endif
{
	struct mbuf *m;
	struct mb_args *args;
#ifdef MAC
	int error;
#endif
	int flags;
	short type;
#ifdef VPOX
    NOREF(pData);
    NOREF(size);
#endif

	m = (struct mbuf *)mem;
	args = (struct mb_args *)arg;
	flags = args->flags;
	type = args->type;

#ifdef INVARIANTS
	trash_ctor(m->m_ext.ext_buf, MCLBYTES, arg, how);
#elif defined(VPOX)
    NOREF(how);
#endif
	m->m_next = NULL;
	m->m_nextpkt = NULL;
	m->m_data = m->m_ext.ext_buf;
	m->m_len = 0;
	m->m_flags = (flags | M_EXT);
	m->m_type = type;

	if (flags & M_PKTHDR) {
		m->m_pkthdr.rcvif = NULL;
		m->m_pkthdr.len = 0;
		m->m_pkthdr.header = NULL;
		m->m_pkthdr.csum_flags = 0;
		m->m_pkthdr.csum_data = 0;
		m->m_pkthdr.tso_segsz = 0;
		m->m_pkthdr.ether_vtag = 0;
		SLIST_INIT(&m->m_pkthdr.tags);
#ifdef MAC
		/* If the label init fails, fail the alloc */
		error = mac_init_mbuf(m, how);
		if (error)
			return (error);
#endif
	}
	/* m_ext is already initialized. */

	return (0);
}

#if 0 /* unused */
/*
 * This is the protocol drain routine.
 *
 * No locks should be held when this is called.  The drain routines have to
 * presently acquire some locks which raises the possibility of lock order
 * reversal.
 */
static void
mb_reclaim(void *junk)
{
#ifndef VPOX
	struct domain *dp;
	struct protosw *pr;

	WITNESS_WARN(WARN_GIANTOK | WARN_SLEEPOK | WARN_PANIC, NULL,
	    "mb_reclaim()");

	for (dp = domains; dp != NULL; dp = dp->dom_next)
		for (pr = dp->dom_protosw; pr < dp->dom_protoswNPROTOSW; pr++)
			if (pr->pr_drain != NULL)
				(*pr->pr_drain)();
#else
    NOREF(junk);
#endif
}
#endif /* unused */
