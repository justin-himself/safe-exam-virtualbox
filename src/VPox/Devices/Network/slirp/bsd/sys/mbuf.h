/*-
 * Copyright (c) 1982, 1986, 1988, 1993
 *	The Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)mbuf.h	8.5 (Berkeley) 2/19/95
 * $FreeBSD: src/sys/sys/mbuf.h,v 1.217.2.3.4.1 2009/04/15 03:14:26 kensmith Exp $
 */

#ifndef _SYS_MBUF_H_
#define	_SYS_MBUF_H_

#ifndef VPOX
/* XXX: These includes suck. Sorry! */
#include <sys/queue.h>
#ifdef _KERNEL
#include <sys/systm.h>
#include <vm/uma.h>
#ifdef WITNESS
#include <sys/lock.h>
#endif
#endif
#else /* VPOX */
# include <iprt/param.h>
# include "misc.h"
# include "ext.h"

typedef const char *c_caddr_t;

DECLNORETURN(static void) panic (char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vpox_slirp_printV(fmt, args);
    va_end(args);
    AssertFatalFailed();
}
/* for non-gnu compilers */
# define __func__ RT_GCC_EXTENSION __FUNCTION__
# ifndef __inline
#  ifdef __GNUC__
#  define __inline __inline__
# else
#  define __inline
# endif
# endif

# define bzero(a1, len) memset((a1), 0, (len))

/* (vvl) some definitions from sys/param.h */
/*
 * Constants related to network buffer management.
 * MCLBYTES must be no larger than PAGE_SIZE.
 */
# ifndef	MSIZE
#  define MSIZE		256		/* size of an mbuf */
# endif	/* MSIZE */

# ifndef	MCLSHIFT
#  define MCLSHIFT	11		/* convert bytes to mbuf clusters */
# endif	/* MCLSHIFT */

# ifndef MCLBYTES
#  define MCLBYTES	(1 << MCLSHIFT)	/* size of an mbuf cluster */
# endif /*MCLBYTES*/

# define	MJUMPAGESIZE	PAGE_SIZE	/* jumbo cluster 4k */
# define	MJUM9BYTES	(9 * 1024)	/* jumbo cluster 9k */
# define	MJUM16BYTES	(16 * 1024)	/* jumbo cluster 16k */
#endif /* VPOX */

/*
 * Mbufs are of a single size, MSIZE (sys/param.h), which includes overhead.
 * An mbuf may add a single "mbuf cluster" of size MCLBYTES (also in
 * sys/param.h), which has no additional overhead and is used instead of the
 * internal data area; this is done when at least MINCLSIZE of data must be
 * stored.  Additionally, it is possible to allocate a separate buffer
 * externally and attach it to the mbuf in a way similar to that of mbuf
 * clusters.
 */
#define	MLEN		(MSIZE - sizeof(struct m_hdr))	/* normal data len */
#define	MHLEN		(MLEN - sizeof(struct pkthdr))	/* data len w/pkthdr */
#define	MINCLSIZE	(MHLEN + 1)	/* smallest amount to put in cluster */
#define	M_MAXCOMPRESS	(MHLEN / 2)	/* max amount to copy for compression */

#if defined(_KERNEL) || defined(VPOX)
/*-
 * Macros for type conversion:
 * mtod(m, t)	-- Convert mbuf pointer to data pointer of correct type.
 * dtom(x)	-- Convert data pointer within mbuf to mbuf pointer (XXX).
 */
#define	mtod(m, t)	((t)((m)->m_data))
#define	dtom(x)		((struct mbuf *)((intptr_t)(x) & ~(MSIZE-1)))

/*
 * Argument structure passed to UMA routines during mbuf and packet
 * allocations.
 */
struct mb_args {
	int	flags;	/* Flags for mbuf being allocated */
	short	type;	/* Type of mbuf being allocated */
};
#endif /* _KERNEL */

#if defined(__LP64__)
#define M_HDR_PAD    6
#else
#define M_HDR_PAD    2
#endif

/*
 * Header present at the beginning of every mbuf.
 */
struct m_hdr {
	struct mbuf	*mh_next;	/* next buffer in chain */
	struct mbuf	*mh_nextpkt;	/* next chain in queue/record */
	caddr_t		 mh_data;	/* location of data */
	int		 mh_len;	/* amount of data in this mbuf */
	int		 mh_flags;	/* flags; see below */
	short		 mh_type;	/* type of data in this mbuf */
#ifdef VPOX
        struct socket   *mh_so;         /*socket assotiated with mbuf*/
        TAILQ_ENTRY(mbuf) mh_ifq;
#endif
	uint8_t          pad[M_HDR_PAD];/* word align                  */
};

/*
 * Packet tag structure (see below for details).
 */
struct m_tag {
	SLIST_ENTRY(m_tag)	m_tag_link;	/* List of packet tags */
	u_int16_t		m_tag_id;	/* Tag ID */
	u_int16_t		m_tag_len;	/* Length of data */
	u_int32_t		m_tag_cookie;	/* ABI/Module ID */
	void			(*m_tag_free)(struct m_tag *);
};

/*
 * Record/packet header in first mbuf of chain; valid only if M_PKTHDR is set.
 */
struct pkthdr {
	struct ifnet	*rcvif;		/* rcv interface */
	/* variables for ip and tcp reassembly */
	void		*header;	/* pointer to packet header */
	int		 len;		/* total packet length */
	/* variables for hardware checksum */
	int		 csum_flags;	/* flags regarding checksum */
	int		 csum_data;	/* data field used by csum routines */
	u_int16_t	 tso_segsz;	/* TSO segment size */
	u_int16_t	 ether_vtag;	/* Ethernet 802.1p+q vlan tag */
	SLIST_HEAD(packet_tags, m_tag) tags; /* list of packet tags */
};

/*
 * Description of external storage mapped into mbuf; valid only if M_EXT is
 * set.
 */
struct m_ext {
	caddr_t		 ext_buf;	/* start of buffer */
	void		(*ext_free)	/* free routine if not the usual */
			    (void *, void *);
	void		*ext_args;	/* optional argument pointer */
	u_int		 ext_size;	/* size of buffer, for ext_free */
#ifdef VPOX
	volatile uint32_t *ref_cnt;	/* pointer to ref count info */
#else
	volatile u_int	*ref_cnt;	/* pointer to ref count info */
#endif
	int		 ext_type;	/* type of external storage */
};

/*
 * The core of the mbuf object along with some shortcut defines for practical
 * purposes.
 */
struct mbuf {
	struct m_hdr	m_hdr;
	union {
		struct {
			struct pkthdr	MH_pkthdr;	/* M_PKTHDR set */
			union {
				struct m_ext	MH_ext;	/* M_EXT set */
				char		MH_databuf[MHLEN];
			} MH_dat;
		} MH;
		char	M_databuf[MLEN];		/* !M_PKTHDR, !M_EXT */
	} M_dat;
};
#define	m_next		m_hdr.mh_next
#define	m_len		m_hdr.mh_len
#define	m_data		m_hdr.mh_data
#define	m_type		m_hdr.mh_type
#define	m_flags		m_hdr.mh_flags
#define	m_nextpkt	m_hdr.mh_nextpkt
#define	m_act		m_nextpkt
#define	m_pkthdr	M_dat.MH.MH_pkthdr
#define	m_ext		M_dat.MH.MH_dat.MH_ext
#define	m_pktdat	M_dat.MH.MH_dat.MH_databuf
#define	m_dat		M_dat.M_databuf
#ifdef VPOX
# define m_so           m_hdr.mh_so
# define ifq_so         m_hdr.mh_so
# define m_ifq          m_hdr.mh_ifq
#endif

/*
 * mbuf flags.
 */
#define	M_EXT		0x00000001 /* has associated external storage */
#define	M_PKTHDR	0x00000002 /* start of record */
#define	M_EOR		0x00000004 /* end of record */
#define	M_RDONLY	0x00000008 /* associated data is marked read-only */
#define	M_PROTO1	0x00000010 /* protocol-specific */
#define	M_PROTO2	0x00000020 /* protocol-specific */
#define	M_PROTO3	0x00000040 /* protocol-specific */
#define	M_PROTO4	0x00000080 /* protocol-specific */
#define	M_PROTO5	0x00000100 /* protocol-specific */
#define	M_BCAST		0x00000200 /* send/received as link-level broadcast */
#define	M_MCAST		0x00000400 /* send/received as link-level multicast */
#define	M_FRAG		0x00000800 /* packet is a fragment of a larger packet */
#define	M_FIRSTFRAG	0x00001000 /* packet is first fragment */
#define	M_LASTFRAG	0x00002000 /* packet is last fragment */
#define	M_SKIP_FIREWALL	0x00004000 /* skip firewall processing */
#define	M_FREELIST	0x00008000 /* mbuf is on the free list */
#define	M_VLANTAG	0x00010000 /* ether_vtag is valid */
#define	M_PROMISC	0x00020000 /* packet was not for us */
#define	M_NOFREE	0x00040000 /* do not free mbuf, embedded in cluster */
#define	M_PROTO6	0x00080000 /* protocol-specific */
#define	M_PROTO7	0x00100000 /* protocol-specific */
#define	M_PROTO8	0x00200000 /* protocol-specific */
/*
 * For RELENG_{6,7} steal these flags for limited multiple routing table
 * support. In RELENG_8 and beyond, use just one flag and a tag.
 */
#define	M_FIB		0xF0000000 /* steal some bits to store fib number. */

#define	M_NOTIFICATION	M_PROTO5    /* SCTP notification */

/*
 * Flags to purge when crossing layers.
 */
#define	M_PROTOFLAGS \
    (M_PROTO1|M_PROTO2|M_PROTO3|M_PROTO4|M_PROTO5|M_PROTO6|M_PROTO7|M_PROTO8)

/*
 * Flags preserved when copying m_pkthdr.
 */
#define	M_COPYFLAGS \
    (M_PKTHDR|M_EOR|M_RDONLY|M_PROTOFLAGS|M_SKIP_FIREWALL|M_BCAST|M_MCAST|\
     M_FRAG|M_FIRSTFRAG|M_LASTFRAG|M_VLANTAG|M_PROMISC|M_FIB)

/*
 * External buffer types: identify ext_buf type.
 */
#define	EXT_CLUSTER	1	/* mbuf cluster */
#define	EXT_SFBUF	2	/* sendfile(2)'s sf_bufs */
#define	EXT_JUMBOP	3	/* jumbo cluster 4096 bytes */
#define	EXT_JUMBO9	4	/* jumbo cluster 9216 bytes */
#define	EXT_JUMBO16	5	/* jumbo cluster 16184 bytes */
#define	EXT_PACKET	6	/* mbuf+cluster from packet zone */
#define	EXT_MBUF	7	/* external mbuf reference (M_IOVEC) */
#define	EXT_NET_DRV	100	/* custom ext_buf provided by net driver(s) */
#define	EXT_MOD_TYPE	200	/* custom module's ext_buf type */
#define	EXT_DISPOSABLE	300	/* can throw this buffer away w/page flipping */
#define	EXT_EXTREF	400	/* has externally maintained ref_cnt ptr */

/*
 * Flags indicating hw checksum support and sw checksum requirements.  This
 * field can be directly tested against if_data.ifi_hwassist.
 */
#define	CSUM_IP			0x0001		/* will csum IP */
#define	CSUM_TCP		0x0002		/* will csum TCP */
#define	CSUM_UDP		0x0004		/* will csum UDP */
#define	CSUM_IP_FRAGS		0x0008		/* will csum IP fragments */
#define	CSUM_FRAGMENT		0x0010		/* will do IP fragmentation */
#define	CSUM_TSO		0x0020		/* will do TSO */

#define	CSUM_IP_CHECKED		0x0100		/* did csum IP */
#define	CSUM_IP_VALID		0x0200		/*   ... the csum is valid */
#define	CSUM_DATA_VALID		0x0400		/* csum_data field is valid */
#define	CSUM_PSEUDO_HDR		0x0800		/* csum_data has pseudo hdr */

#define	CSUM_DELAY_DATA		(CSUM_TCP | CSUM_UDP)
#define	CSUM_DELAY_IP		(CSUM_IP)	/* XXX add ipv6 here too? */

/*
 * mbuf types.
 */
#define	MT_NOTMBUF	0	/* USED INTERNALLY ONLY! Object is not mbuf */
#define	MT_DATA		1	/* dynamic (data) allocation */
#define	MT_HEADER	MT_DATA	/* packet header, use M_PKTHDR instead */
#define	MT_SONAME	8	/* socket name */
#define	MT_CONTROL	14	/* extra-data protocol message */
#define	MT_OOBDATA	15	/* expedited data  */
#define	MT_NTYPES	16	/* number of mbuf types for mbtypes[] */

#define	MT_NOINIT	255	/* Not a type but a flag to allocate
				   a non-initialized mbuf */

#define MB_NOTAGS	0x1UL	/* no tags attached to mbuf */

/*
 * General mbuf allocator statistics structure.
 *
 * Many of these statistics are no longer used; we instead track many
 * allocator statistics through UMA's built in statistics mechanism.
 */
struct mbstat {
	u_long	m_mbufs;	/* XXX */
	u_long	m_mclusts;	/* XXX */

	u_long	m_drain;	/* times drained protocols for space */
	u_long	m_mcfail;	/* XXX: times m_copym failed */
	u_long	m_mpfail;	/* XXX: times m_pullup failed */
	u_long	m_msize;	/* length of an mbuf */
	u_long	m_mclbytes;	/* length of an mbuf cluster */
	u_long	m_minclsize;	/* min length of data to allocate a cluster */
	u_long	m_mlen;		/* length of data in an mbuf */
	u_long	m_mhlen;	/* length of data in a header mbuf */

	/* Number of mbtypes (gives # elems in mbtypes[] array) */
	short	m_numtypes;

	/* XXX: Sendfile stats should eventually move to their own struct */
	u_long	sf_iocnt;	/* times sendfile had to do disk I/O */
	u_long	sf_allocfail;	/* times sfbuf allocation failed */
	u_long	sf_allocwait;	/* times sfbuf allocation had to wait */
};

/*
 * Flags specifying how an allocation should be made.
 *
 * The flag to use is as follows:
 * - M_DONTWAIT or M_NOWAIT from an interrupt handler to not block allocation.
 * - M_WAIT or M_WAITOK or M_TRYWAIT from wherever it is safe to block.
 *
 * M_DONTWAIT/M_NOWAIT means that we will not block the thread explicitly and
 * if we cannot allocate immediately we may return NULL, whereas
 * M_WAIT/M_WAITOK/M_TRYWAIT means that if we cannot allocate resources we
 * will block until they are available, and thus never return NULL.
 *
 * XXX Eventually just phase this out to use M_WAITOK/M_NOWAIT.
 */
#define	MBTOM(how)	(how)
#ifndef VPOX
#define	M_DONTWAIT	M_NOWAIT
#define	M_TRYWAIT	M_WAITOK
#define	M_WAIT		M_WAITOK
#else
/* @todo (r=vvl) not sure we can do it in NAT */
# define M_WAITOK  0
# define M_NOWAIT  0
# define M_DONTWAIT  0
# define M_TRYWAI    0
# define M_WAIT	    0
#endif

/*
 * String names of mbuf-related UMA(9) and malloc(9) types.  Exposed to
 * !_KERNEL so that monitoring tools can look up the zones with
 * libmemstat(3).
 */
#define	MBUF_MEM_NAME		"mbuf"
#define	MBUF_CLUSTER_MEM_NAME	"mbuf_cluster"
#define	MBUF_PACKET_MEM_NAME	"mbuf_packet"
#define	MBUF_JUMBOP_MEM_NAME	"mbuf_jumbo_pagesize"
#define	MBUF_JUMBO9_MEM_NAME	"mbuf_jumbo_9k"
#define	MBUF_JUMBO16_MEM_NAME	"mbuf_jumbo_16k"
#define	MBUF_TAG_MEM_NAME	"mbuf_tag"
#define	MBUF_EXTREFCNT_MEM_NAME	"mbuf_ext_refcnt"

#if defined(_KERNEL) || defined(VPOX)

#ifdef WITNESS
#define	MBUF_CHECKSLEEP(how) do {					\
	if (how == M_WAITOK)						\
		WITNESS_WARN(WARN_GIANTOK | WARN_SLEEPOK, NULL,		\
		    "Sleeping in \"%s\"", __func__);			\
} while (0)
#else
#define	MBUF_CHECKSLEEP(how)
#endif

/*
 * Network buffer allocation API
 *
 * The rest of it is defined in kern/kern_mbuf.c
 */

#ifndef VPOX
extern uma_zone_t	zone_mbuf;
extern uma_zone_t	zone_clust;
extern uma_zone_t	zone_pack;
extern uma_zone_t	zone_jumbop;
extern uma_zone_t	zone_jumbo9;
extern uma_zone_t	zone_jumbo16;
extern uma_zone_t	zone_ext_refcnt;
#endif

#ifndef VPOX
static __inline struct mbuf	*m_getcl(int how, short type, int flags);
static __inline struct mbuf	*m_get(int how, short type);
static __inline struct mbuf	*m_gethdr(int how, short type);
static __inline struct mbuf	*m_getjcl(int how, short type, int flags,
				    int size);
static __inline struct mbuf	*m_getclr(int how, short type);	/* XXX */
static __inline struct mbuf	*m_free(struct mbuf *m);
static __inline void		 m_clget(struct mbuf *m, int how);
static __inline void		*m_cljget(struct mbuf *m, int how, int size);
void				 mb_free_ext(struct mbuf *);
#else
static __inline struct mbuf	*m_getcl(PNATState pData, int how, short type, int flags);
static __inline struct mbuf	*m_get(PNATState pData, int how, short type);
static __inline struct mbuf	*m_gethdr(PNATState pData, int how, short type);
static __inline struct mbuf	*m_getjcl(PNATState pData, int how,
                                    short type, int flags, int size);
static __inline struct mbuf	*m_getclr(PNATState pData, int how, short type);	/* XXX */
static __inline struct mbuf	*m_free(PNATState pData, struct mbuf *m);
static __inline void		 m_clget(PNATState pData, struct mbuf *m, int how);
static __inline void		*m_cljget(PNATState pData, struct mbuf *m, int how, int size);
void				 mb_free_ext(PNATState, struct mbuf *);
#endif
static __inline void		 m_chtype(struct mbuf *m, short new_type);
static __inline struct mbuf	*m_last(struct mbuf *m);

static __inline int
m_gettype(int size)
{
	int type;

	switch (size) {
	case MSIZE:
		type = EXT_MBUF;
		break;
	case MCLBYTES:
		type = EXT_CLUSTER;
		break;
#if MJUMPAGESIZE != MCLBYTES
	case MJUMPAGESIZE:
		type = EXT_JUMBOP;
		break;
#endif
	case MJUM9BYTES:
		type = EXT_JUMBO9;
		break;
	case MJUM16BYTES:
		type = EXT_JUMBO16;
		break;
	default:
		panic("%s: m_getjcl: invalid cluster size", __func__);
	}

	return (type);
}

static __inline uma_zone_t
#ifndef VPOX
m_getzone(int size)
#else
m_getzone(PNATState pData, int size)
#endif
{
	uma_zone_t zone;

	switch (size) {
	case MSIZE:
		zone = zone_mbuf;
		break;
	case MCLBYTES:
		zone = zone_clust;
		break;
#if MJUMPAGESIZE != MCLBYTES
	case MJUMPAGESIZE:
		zone = zone_jumbop;
		break;
#endif
	case MJUM9BYTES:
		zone = zone_jumbo9;
		break;
	case MJUM16BYTES:
		zone = zone_jumbo16;
		break;
	default:
		panic("%s: m_getjcl: invalid cluster type", __func__);
	}

	return (zone);
}

static __inline struct mbuf *
#ifndef VPOX
m_get(int how, short type)
#else
m_get(PNATState pData, int how, short type)
#endif
{
	struct mb_args args;

	args.flags = 0;
	args.type = type;
	return ((struct mbuf *)(uma_zalloc_arg(zone_mbuf, &args, how)));
}

/*
 * XXX This should be deprecated, very little use.
 */
static __inline struct mbuf *
#ifndef VPOX
m_getclr(int how, short type)
#else
m_getclr(PNATState pData, int how, short type)
#endif
{
	struct mbuf *m;
	struct mb_args args;

	args.flags = 0;
	args.type = type;
	m = uma_zalloc_arg(zone_mbuf, &args, how);
	if (m != NULL)
		bzero(m->m_data, MLEN);
	return (m);
}

static __inline struct mbuf *
#ifndef VPOX
m_gethdr(int how, short type)
#else
m_gethdr(PNATState pData, int how, short type)
#endif
{
	struct mb_args args;

	args.flags = M_PKTHDR;
	args.type = type;
	return ((struct mbuf *)(uma_zalloc_arg(zone_mbuf, &args, how)));
}

static __inline struct mbuf *
#ifndef VPOX
m_getcl(int how, short type, int flags)
#else
m_getcl(PNATState pData, int how, short type, int flags)
#endif
{
	struct mb_args args;

	args.flags = flags;
	args.type = type;
	return ((struct mbuf *)(uma_zalloc_arg(zone_pack, &args, how)));
}

/*
 * m_getjcl() returns an mbuf with a cluster of the specified size attached.
 * For size it takes MCLBYTES, MJUMPAGESIZE, MJUM9BYTES, MJUM16BYTES.
 *
 * XXX: This is rather large, should be real function maybe.
 */
static __inline struct mbuf *
#ifndef VPOX
m_getjcl(int how, short type, int flags, int size)
#else
m_getjcl(PNATState pData, int how, short type, int flags, int size)
#endif
{
	struct mb_args args;
	struct mbuf *m, *n;
	uma_zone_t zone;

	args.flags = flags;
	args.type = type;

	m = uma_zalloc_arg(zone_mbuf, &args, how);
	if (m == NULL)
		return (NULL);

#ifndef VPOX
	zone = m_getzone(size);
#else
	zone = m_getzone(pData, size);
#endif
	n = uma_zalloc_arg(zone, m, how);
	if (n == NULL) {
		uma_zfree(zone_mbuf, m);
		return (NULL);
	}
	return (m);
}

#ifndef VPOX
static __inline void
m_free_fast(struct mbuf *m)
{
	KASSERT(SLIST_EMPTY(&m->m_pkthdr.tags), ("doing fast free of mbuf with tags"));

	uma_zfree_arg(zone_mbuf, m, (void *)MB_NOTAGS);
}
#else
static __inline void
m_free_fast(PNATState pData, struct mbuf *m)
{
	AssertMsg(SLIST_EMPTY(&m->m_pkthdr.tags), ("doing fast free of mbuf with tags"));

	uma_zfree_arg(zone_mbuf, m, (void *)(uintptr_t)MB_NOTAGS);
}
#endif

static __inline struct mbuf *
#ifndef VPOX
m_free(struct mbuf *m)
#else
m_free(PNATState pData, struct mbuf *m)
#endif
{
	struct mbuf *n = m->m_next;

	if (m->m_flags & M_EXT)
#ifndef VPOX
		mb_free_ext(m);
#else
		mb_free_ext(pData, m);
#endif
	else if ((m->m_flags & M_NOFREE) == 0)
		uma_zfree(zone_mbuf, m);
	return (n);
}

static __inline void
#ifndef VPOX
m_clget(struct mbuf *m, int how)
#else
m_clget(PNATState pData, struct mbuf *m, int how)
#endif
{

	if (m->m_flags & M_EXT)
		printf("%s: %p mbuf already has cluster\n", __func__, m);
	m->m_ext.ext_buf = (char *)NULL;
	uma_zalloc_arg(zone_clust, m, how);
	/*
	 * On a cluster allocation failure, drain the packet zone and retry,
	 * we might be able to loosen a few clusters up on the drain.
	 */
	if ((how & M_NOWAIT) && (m->m_ext.ext_buf == NULL)) {
		zone_drain(zone_pack);
		uma_zalloc_arg(zone_clust, m, how);
	}
}

/*
 * m_cljget() is different from m_clget() as it can allocate clusters without
 * attaching them to an mbuf.  In that case the return value is the pointer
 * to the cluster of the requested size.  If an mbuf was specified, it gets
 * the cluster attached to it and the return value can be safely ignored.
 * For size it takes MCLBYTES, MJUMPAGESIZE, MJUM9BYTES, MJUM16BYTES.
 */
static __inline void *
#ifndef VPOX
m_cljget(struct mbuf *m, int how, int size)
#else
m_cljget(PNATState pData, struct mbuf *m, int how, int size)
#endif
{
	uma_zone_t zone;

	if (m && m->m_flags & M_EXT)
		printf("%s: %p mbuf already has cluster\n", __func__, m);
	if (m != NULL)
		m->m_ext.ext_buf = NULL;

#ifndef VPOX
	zone = m_getzone(size);
#else
	zone = m_getzone(pData, size);
#endif
	return (uma_zalloc_arg(zone, m, how));
}

static __inline void
#ifndef VPOX
m_cljset(struct mbuf *m, void *cl, int type)
#else
m_cljset(PNATState pData, struct mbuf *m, void *cl, int type)
#endif
{
	uma_zone_t zone;
	int size;

	switch (type) {
	case EXT_CLUSTER:
		size = MCLBYTES;
		zone = zone_clust;
		break;
#if MJUMPAGESIZE != MCLBYTES
	case EXT_JUMBOP:
		size = MJUMPAGESIZE;
		zone = zone_jumbop;
		break;
#endif
	case EXT_JUMBO9:
		size = MJUM9BYTES;
		zone = zone_jumbo9;
		break;
	case EXT_JUMBO16:
		size = MJUM16BYTES;
		zone = zone_jumbo16;
		break;
	default:
		panic("unknown cluster type");
		break;
	}

	m->m_data = m->m_ext.ext_buf = cl;
#ifdef VPOX
        m->m_ext.ext_free = (void (*)(void *, void *))0;
        m->m_ext.ext_args = NULL;
#else
	m->m_ext.ext_free = m->m_ext.ext_args = NULL;
#endif
	m->m_ext.ext_size = size;
	m->m_ext.ext_type = type;
	m->m_ext.ref_cnt = uma_find_refcnt(zone, cl);
	m->m_flags |= M_EXT;

}

static __inline void
m_chtype(struct mbuf *m, short new_type)
{

	m->m_type = new_type;
}

static __inline struct mbuf *
m_last(struct mbuf *m)
{

	while (m->m_next)
		m = m->m_next;
	return (m);
}

/*
 * mbuf, cluster, and external object allocation macros (for compatibility
 * purposes).
 */
#define	M_MOVE_PKTHDR(to, from)	m_move_pkthdr((to), (from))
#ifndef VPOX
#define	MGET(m, how, type)	((m) = m_get((how), (type)))
#define	MGETHDR(m, how, type)	((m) = m_gethdr((how), (type)))
#define	MCLGET(m, how)		m_clget((m), (how))
#define	MEXTADD(m, buf, size, free, args, flags, type) 			\
    m_extadd((m), (caddr_t)(buf), (size), (free), (args), (flags), (type))
#define	m_getm(m, len, how, type)					\
    m_getm2((m), (len), (how), (type), M_PKTHDR)
#else /*!VPOX*/
#define	MGET(m, how, type)	((m) = m_get(pData, (how), (type)))
#define	MGETHDR(m, how, type)	((m) = m_gethdr(pData, (how), (type)))
#define	MCLGET(m, how)		m_clget(pData, (m), (how))
#define	MEXTADD(m, buf, size, free, args, flags, type) 			\
    m_extadd(pData, (m), (caddr_t)(buf), (size), (free), (args), (flags), (type))
#define	m_getm(m, len, how, type)					\
    m_getm2(pData, (m), (len), (how), (type), M_PKTHDR)
#endif

/*
 * Evaluate TRUE if it's safe to write to the mbuf m's data region (this can
 * be both the local data payload, or an external buffer area, depending on
 * whether M_EXT is set).
 */
#define	M_WRITABLE(m)	(!((m)->m_flags & M_RDONLY) &&			\
			 (!(((m)->m_flags & M_EXT)) ||			\
			 (*((m)->m_ext.ref_cnt) == 1)) )		\

/* Check if the supplied mbuf has a packet header, or else panic. */
#define	M_ASSERTPKTHDR(m)						\
	KASSERT(m != NULL && m->m_flags & M_PKTHDR,			\
	    ("%s: no mbuf packet header!", __func__))

/*
 * Ensure that the supplied mbuf is a valid, non-free mbuf.
 *
 * XXX: Broken at the moment.  Need some UMA magic to make it work again.
 */
#define	M_ASSERTVALID(m)						\
	KASSERT((((struct mbuf *)m)->m_flags & 0) == 0,			\
	    ("%s: attempted use of a free mbuf!", __func__))

/*
 * Set the m_data pointer of a newly-allocated mbuf (m_get/MGET) to place an
 * object of the specified size at the end of the mbuf, longword aligned.
 */
#define	M_ALIGN(m, len) do {						\
	KASSERT(!((m)->m_flags & (M_PKTHDR|M_EXT)),			\
		("%s: M_ALIGN not normal mbuf", __func__));		\
	KASSERT((m)->m_data == (m)->m_dat,				\
		("%s: M_ALIGN not a virgin mbuf", __func__));		\
	(m)->m_data += (MLEN - (len)) & ~(sizeof(long) - 1);		\
} while (0)

/*
 * As above, for mbufs allocated with m_gethdr/MGETHDR or initialized by
 * M_DUP/MOVE_PKTHDR.
 */
#define	MH_ALIGN(m, len) do {						\
	KASSERT((m)->m_flags & M_PKTHDR && !((m)->m_flags & M_EXT),	\
		("%s: MH_ALIGN not PKTHDR mbuf", __func__));		\
	KASSERT((m)->m_data == (m)->m_pktdat,				\
		("%s: MH_ALIGN not a virgin mbuf", __func__));		\
	(m)->m_data += (MHLEN - (len)) & ~(sizeof(long) - 1);		\
} while (0)

/*
 * Compute the amount of space available before the current start of data in
 * an mbuf.
 *
 * The M_WRITABLE() is a temporary, conservative safety measure: the burden
 * of checking writability of the mbuf data area rests solely with the caller.
 */
#define	M_LEADINGSPACE(m)						\
	((m)->m_flags & M_EXT ?						\
	    (M_WRITABLE(m) ? (m)->m_data - (m)->m_ext.ext_buf : 0):	\
	    (m)->m_flags & M_PKTHDR ? (m)->m_data - (m)->m_pktdat :	\
	    (m)->m_data - (m)->m_dat)

/*
 * Compute the amount of space available after the end of data in an mbuf.
 *
 * The M_WRITABLE() is a temporary, conservative safety measure: the burden
 * of checking writability of the mbuf data area rests solely with the caller.
 */
#define	M_TRAILINGSPACE(m)						\
	((m)->m_flags & M_EXT ?						\
	    (M_WRITABLE(m) ? (m)->m_ext.ext_buf + (m)->m_ext.ext_size	\
		- ((m)->m_data + (m)->m_len) : 0) :			\
	    &(m)->m_dat[MLEN] - ((m)->m_data + (m)->m_len))

/*
 * Arrange to prepend space of size plen to mbuf m.  If a new mbuf must be
 * allocated, how specifies whether to wait.  If the allocation fails, the
 * original mbuf chain is freed and m is set to NULL.
 */
#define	M_PREPEND(m, plen, how) do {					\
	struct mbuf **_mmp = &(m);					\
	struct mbuf *_mm = *_mmp;					\
	int _mplen = (plen);						\
	int __mhow = (how);						\
									\
	MBUF_CHECKSLEEP(how);						\
	if (M_LEADINGSPACE(_mm) >= _mplen) {				\
		_mm->m_data -= _mplen;					\
		_mm->m_len += _mplen;					\
	} else								\
		_mm = m_prepend(_mm, _mplen, __mhow);			\
	if (_mm != NULL && _mm->m_flags & M_PKTHDR)			\
		_mm->m_pkthdr.len += _mplen;				\
	*_mmp = _mm;							\
} while (0)

/*
 * Change mbuf to new type.  This is a relatively expensive operation and
 * should be avoided.
 */
#define	MCHTYPE(m, t)	m_chtype((m), (t))

/* Length to m_copy to copy all. */
#define	M_COPYALL	1000000000

/* Compatibility with 4.3. */
#define	m_copy(m, o, l)	m_copym((m), (o), (l), M_DONTWAIT)

extern int		max_datalen;	/* MHLEN - max_hdr */
extern int		max_hdr;	/* Largest link + protocol header */
extern int		max_linkhdr;	/* Largest link-level header */
extern int		max_protohdr;	/* Largest protocol header */
extern struct mbstat	mbstat;		/* General mbuf stats/infos */
extern int		nmbclusters;	/* Maximum number of clusters */

struct uio;

void		 m_align(struct mbuf *, int);
int		 m_apply(struct mbuf *, int, int,
		    int (*)(void *, void *, u_int), void *);
#ifndef VPOX
void		 m_adj(struct mbuf *, int);
int		 m_append(struct mbuf *, int, c_caddr_t);
struct mbuf	*m_defrag(struct mbuf *, int);
struct mbuf	*m_dup(struct mbuf *, int);
void		 m_cat(struct mbuf *, struct mbuf *);
struct mbuf	*m_collapse(struct mbuf *, int, int);
void		 m_copyback(struct mbuf *, int, int, c_caddr_t);
struct mbuf	*m_copym(struct mbuf *, int, int, int);
struct mbuf	*m_copymdata(struct mbuf *, struct mbuf *,
		    int, int, int, int);
struct mbuf	*m_copypacket(struct mbuf *, int);
struct mbuf	*m_copyup(struct mbuf *n, int len, int dstoff);
void		 m_extadd(struct mbuf *, caddr_t, u_int,
		    void (*)(void *, void *), void *, int, int);
#else
void		 m_adj(PNATState, struct mbuf *, int);
int		 m_append(PNATState pData, struct mbuf *, int, c_caddr_t);
struct mbuf	*m_defrag(PNATState, struct mbuf *, int);
struct mbuf	*m_dup(PNATState, struct mbuf *, int);
void		 m_cat(PNATState, struct mbuf *, struct mbuf *);
struct mbuf	*m_collapse(PNATState, struct mbuf *, int, int);
void		 m_copyback(PNATState, struct mbuf *, int, int, c_caddr_t);
struct mbuf	*m_copym(PNATState, struct mbuf *, int, int, int);
struct mbuf	*m_copymdata(PNATState, struct mbuf *, struct mbuf *,
		    int, int, int, int);
struct mbuf	*m_copypacket(PNATState, struct mbuf *, int);
struct mbuf	*m_copyup(PNATState, struct mbuf *n, int len, int dstoff);
void		 m_extadd(PNATState pData, struct mbuf *, caddr_t, u_int,
		    void (*)(void *, void *), void *, int, int);
#endif
void		 m_copydata(const struct mbuf *, int, int, caddr_t);
void		 m_copy_pkthdr(struct mbuf *, struct mbuf *);
void		 m_demote(struct mbuf *, int);
struct mbuf	*m_devget(char *, int, int, struct ifnet *,
		    void (*)(char *, caddr_t, u_int));
int		 m_dup_pkthdr(struct mbuf *, struct mbuf *, int);
u_int		 m_fixhdr(struct mbuf *);
struct mbuf	*m_fragment(struct mbuf *, int, int);
#ifndef VPOX
void		 m_freem(struct mbuf *);
struct mbuf	*m_getm2(struct mbuf *, int, int, short, int);
struct mbuf	*m_prepend(struct mbuf *, int, int);
struct mbuf	*m_pulldown(struct mbuf *, int, int, int *);
struct mbuf	*m_pullup(struct mbuf *, int);
int		m_sanity(struct mbuf *, int);
struct mbuf	*m_split(struct mbuf *, int, int);
struct mbuf	*m_unshare(struct mbuf *, int how);
#else
void		 m_freem(PNATState pData, struct mbuf *);
struct mbuf	*m_getm2(PNATState pData, struct mbuf *, int, int, short, int);
struct mbuf	*m_prepend(PNATState, struct mbuf *, int, int);
struct mbuf	*m_pulldown(PNATState, struct mbuf *, int, int, int *);
struct mbuf	*m_pullup(PNATState, struct mbuf *, int);
int		m_sanity(PNATState, struct mbuf *, int);
struct mbuf	*m_split(PNATState, struct mbuf *, int, int);
struct mbuf	*m_unshare(PNATState, struct mbuf *, int how);
#endif
struct mbuf	*m_getptr(struct mbuf *, int, int *);
u_int		 m_length(struct mbuf *, struct mbuf **);
void		 m_move_pkthdr(struct mbuf *, struct mbuf *);
void		 m_print(const struct mbuf *, int);
struct mbuf	*m_uiotombuf(struct uio *, int, int, int, int);

/*-
 * Network packets may have annotations attached by affixing a list of
 * "packet tags" to the pkthdr structure.  Packet tags are dynamically
 * allocated semi-opaque data structures that have a fixed header
 * (struct m_tag) that specifies the size of the memory block and a
 * <cookie,type> pair that identifies it.  The cookie is a 32-bit unique
 * unsigned value used to identify a module or ABI.  By convention this value
 * is chosen as the date+time that the module is created, expressed as the
 * number of seconds since the epoch (e.g., using date -u +'%s').  The type
 * value is an ABI/module-specific value that identifies a particular
 * annotation and is private to the module.  For compatibility with systems
 * like OpenBSD that define packet tags w/o an ABI/module cookie, the value
 * PACKET_ABI_COMPAT is used to implement m_tag_get and m_tag_find
 * compatibility shim functions and several tag types are defined below.
 * Users that do not require compatibility should use a private cookie value
 * so that packet tag-related definitions can be maintained privately.
 *
 * Note that the packet tag returned by m_tag_alloc has the default memory
 * alignment implemented by malloc.  To reference private data one can use a
 * construct like:
 *
 *	struct m_tag *mtag = m_tag_alloc(...);
 *	struct foo *p = (struct foo *)(mtag+1);
 *
 * if the alignment of struct m_tag is sufficient for referencing members of
 * struct foo.  Otherwise it is necessary to embed struct m_tag within the
 * private data structure to insure proper alignment; e.g.,
 *
 *	struct foo {
 *		struct m_tag	tag;
 *		...
 *	};
 *	struct foo *p = (struct foo *) m_tag_alloc(...);
 *	struct m_tag *mtag = &p->tag;
 */

/*
 * Persistent tags stay with an mbuf until the mbuf is reclaimed.  Otherwise
 * tags are expected to ``vanish'' when they pass through a network
 * interface.  For most interfaces this happens normally as the tags are
 * reclaimed when the mbuf is free'd.  However in some special cases
 * reclaiming must be done manually.  An example is packets that pass through
 * the loopback interface.  Also, one must be careful to do this when
 * ``turning around'' packets (e.g., icmp_reflect).
 *
 * To mark a tag persistent bit-or this flag in when defining the tag id.
 * The tag will then be treated as described above.
 */
#define	MTAG_PERSISTENT				0x800

#define	PACKET_TAG_NONE				0  /* Nadda */

/* Packet tags for use with PACKET_ABI_COMPAT. */
#define	PACKET_TAG_IPSEC_IN_DONE		1  /* IPsec applied, in */
#define	PACKET_TAG_IPSEC_OUT_DONE		2  /* IPsec applied, out */
#define	PACKET_TAG_IPSEC_IN_CRYPTO_DONE		3  /* NIC IPsec crypto done */
#define	PACKET_TAG_IPSEC_OUT_CRYPTO_NEEDED	4  /* NIC IPsec crypto req'ed */
#define	PACKET_TAG_IPSEC_IN_COULD_DO_CRYPTO	5  /* NIC notifies IPsec */
#define	PACKET_TAG_IPSEC_PENDING_TDB		6  /* Reminder to do IPsec */
#define	PACKET_TAG_BRIDGE			7  /* Bridge processing done */
#define	PACKET_TAG_GIF				8  /* GIF processing done */
#define	PACKET_TAG_GRE				9  /* GRE processing done */
#define	PACKET_TAG_IN_PACKET_CHECKSUM		10 /* NIC checksumming done */
#define	PACKET_TAG_ENCAP			11 /* Encap.  processing */
#define	PACKET_TAG_IPSEC_SOCKET			12 /* IPSEC socket ref */
#define	PACKET_TAG_IPSEC_HISTORY		13 /* IPSEC history */
#define	PACKET_TAG_IPV6_INPUT			14 /* IPV6 input processing */
#define	PACKET_TAG_DUMMYNET			15 /* dummynet info */
#define	PACKET_TAG_DIVERT			17 /* divert info */
#define	PACKET_TAG_IPFORWARD			18 /* ipforward info */
#define	PACKET_TAG_MACLABEL	(19 | MTAG_PERSISTENT) /* MAC label */
#define	PACKET_TAG_PF				21 /* PF + ALTQ information */
#define	PACKET_TAG_RTSOCKFAM			25 /* rtsock sa family */
#define	PACKET_TAG_IPOPTIONS			27 /* Saved IP options */
#define	PACKET_TAG_CARP                         28 /* CARP info */
#ifdef VPOX
# define	PACKET_TAG_ALIAS                        0xab01
# define	PACKET_TAG_ETHER                        0xab02
# define        PACKET_SERVICE                          0xab03
#endif

/* Specific cookies and tags. */

/* Packet tag routines. */
struct m_tag	*m_tag_alloc(u_int32_t, int, int, int);
void		 m_tag_delete(struct mbuf *, struct m_tag *);
void		 m_tag_delete_chain(struct mbuf *, struct m_tag *);
void		 m_tag_free_default(struct m_tag *);
struct m_tag	*m_tag_locate(struct mbuf *, u_int32_t, int, struct m_tag *);
struct m_tag	*m_tag_copy(struct m_tag *, int);
int		 m_tag_copy_chain(struct mbuf *, struct mbuf *, int);
void		 m_tag_delete_nonpersistent(struct mbuf *);

/*
 * Initialize the list of tags associated with an mbuf.
 */
static __inline void
m_tag_init(struct mbuf *m)
{

	SLIST_INIT(&m->m_pkthdr.tags);
}

/*
 * Set up the contents of a tag.  Note that this does not fill in the free
 * method; the caller is expected to do that.
 *
 * XXX probably should be called m_tag_init, but that was already taken.
 */
static __inline void
m_tag_setup(struct m_tag *t, u_int32_t cookie, int type, int len)
{

	t->m_tag_id = type;
	t->m_tag_len = len;
	t->m_tag_cookie = cookie;
}

/*
 * Reclaim resources associated with a tag.
 */
static __inline void
m_tag_free(struct m_tag *t)
{

	(*t->m_tag_free)(t);
}

/*
 * Return the first tag associated with an mbuf.
 */
static __inline struct m_tag *
m_tag_first(struct mbuf *m)
{

	return (SLIST_FIRST(&m->m_pkthdr.tags));
}

/*
 * Return the next tag in the list of tags associated with an mbuf.
 */
static __inline struct m_tag *
m_tag_next(struct mbuf *m, struct m_tag *t)
{
        NOREF(m);
	return (SLIST_NEXT(t, m_tag_link));
}

/*
 * Prepend a tag to the list of tags associated with an mbuf.
 */
static __inline void
m_tag_prepend(struct mbuf *m, struct m_tag *t)
{

	SLIST_INSERT_HEAD(&m->m_pkthdr.tags, t, m_tag_link);
}

/*
 * Unlink a tag from the list of tags associated with an mbuf.
 */
static __inline void
m_tag_unlink(struct mbuf *m, struct m_tag *t)
{

	SLIST_REMOVE(&m->m_pkthdr.tags, t, m_tag, m_tag_link);
}

/* These are for OpenBSD compatibility. */
#define	MTAG_ABI_COMPAT		0		/* compatibility ABI */

static __inline struct m_tag *
m_tag_get(int type, int length, int fWait)
{
	return (m_tag_alloc(MTAG_ABI_COMPAT, type, length, fWait));
}

static __inline struct m_tag *
m_tag_find(struct mbuf *m, int type, struct m_tag *start)
{
	return (SLIST_EMPTY(&m->m_pkthdr.tags) ? (struct m_tag *)NULL :
	    m_tag_locate(m, MTAG_ABI_COMPAT, type, start));
}

/* XXX temporary FIB methods probably eventually use tags.*/
#define M_FIBSHIFT    28
#define M_FIBMASK	0x0F

/* get the fib from an mbuf and if it is not set, return the default */
#define M_GETFIB(_m) \
    ((((_m)->m_flags & M_FIB) >> M_FIBSHIFT) & M_FIBMASK)

#define M_SETFIB(_m, _fib) do {						\
	_m->m_flags &= ~M_FIB;					   	\
	_m->m_flags |= (((_fib) << M_FIBSHIFT) & M_FIB);  \
} while (0)

#endif /* _KERNEL */

#endif /* !_SYS_MBUF_H_ */
