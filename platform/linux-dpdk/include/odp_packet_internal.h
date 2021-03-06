/* Copyright (c) 2014, Linaro Limited
 * All rights reserved.
 *
 * SPDX-License-Identifier:     BSD-3-Clause
 */


/**
 * @file
 *
 * ODP packet descriptor - implementation internal
 */

#ifndef ODP_PACKET_INTERNAL_H_
#define ODP_PACKET_INTERNAL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <odp/align.h>
#include <odp_debug_internal.h>
#include <odp/debug.h>
#include <odp_buffer_internal.h>
#include <odp_buffer_inlines.h>
#include <odp_pool_internal.h>
#include <odp/packet.h>
#include <odp/packet_io.h>
#include <odp/crypto.h>
#include <odp_crypto_internal.h>

#include <rte_acl_osdep.h>

/**
 * Packet input & protocol flags
 */
typedef union {
	/* All input flags */
	uint32_t all;

	struct {
		uint32_t parsed_l2:1; /**< L2 parsed */
		uint32_t parsed_all:1;/**< Parsing complete */

		uint32_t l2:1;        /**< known L2 protocol present */
		uint32_t l3:1;        /**< known L3 protocol present */
		uint32_t l4:1;        /**< known L4 protocol present */

		uint32_t eth:1;       /**< Ethernet */
		uint32_t jumbo:1;     /**< Jumbo frame */
		uint32_t vlan:1;      /**< VLAN hdr found */
		uint32_t vlan_qinq:1; /**< Stacked VLAN found, QinQ */

		uint32_t snap:1;      /**< SNAP */
		uint32_t arp:1;       /**< ARP */

		uint32_t ipv4:1;      /**< IPv4 */
		uint32_t ipv6:1;      /**< IPv6 */
		uint32_t ipfrag:1;    /**< IP fragment */
		uint32_t ipopt:1;     /**< IP optional headers */
		uint32_t ipsec:1;     /**< IPSec decryption may be needed */

		uint32_t udp:1;       /**< UDP */
		uint32_t tcp:1;       /**< TCP */
		uint32_t tcpopt:1;    /**< TCP options present */
		uint32_t sctp:1;      /**< SCTP */
		uint32_t icmp:1;      /**< ICMP */
	};
} input_flags_t;

_ODP_STATIC_ASSERT(sizeof(input_flags_t) == sizeof(uint32_t),
		   "INPUT_FLAGS_SIZE_ERROR");

/**
 * Packet error flags
 */
typedef union {
	/* All error flags */
	uint32_t all;

	struct {
		/* Bitfield flags for each detected error */
		uint32_t app_error:1; /**< Error bit for application use */
		uint32_t frame_len:1; /**< Frame length error */
		uint32_t snap_len:1;  /**< Snap length error */
		uint32_t l2_chksum:1; /**< L2 checksum error, checks TBD */
		uint32_t ip_err:1;    /**< IP error,  checks TBD */
		uint32_t tcp_err:1;   /**< TCP error, checks TBD */
		uint32_t udp_err:1;   /**< UDP error, checks TBD */
	};
} error_flags_t;

_ODP_STATIC_ASSERT(sizeof(error_flags_t) == sizeof(uint32_t),
		   "ERROR_FLAGS_SIZE_ERROR");

/**
 * Packet output flags
 */
typedef union {
	/* All output flags */
	uint32_t all;

	struct {
		/* Bitfield flags for each output option */
		uint32_t l3_chksum_set:1; /**< L3 chksum bit is valid */
		uint32_t l3_chksum:1;     /**< L3 chksum override */
		uint32_t l4_chksum_set:1; /**< L3 chksum bit is valid */
		uint32_t l4_chksum:1;     /**< L4 chksum override  */
	};
} output_flags_t;

_ODP_STATIC_ASSERT(sizeof(output_flags_t) == sizeof(uint32_t),
		   "OUTPUT_FLAGS_SIZE_ERROR");

/**
 * Internal Packet header
 */
typedef struct {
	/* common buffer header */
	odp_buffer_hdr_t buf_hdr;

	odp_pktio_t input;       /**< Originating pktio */

	input_flags_t  input_flags;
	error_flags_t  error_flags;
	output_flags_t output_flags;

	uint32_t l2_offset; /**< offset to L2 hdr, e.g. Eth */
	uint32_t l3_offset; /**< offset to L3 hdr, e.g. IPv4, IPv6 */
	uint32_t l4_offset; /**< offset to L4 hdr (TCP, UDP, SCTP, also ICMP) */
	uint32_t payload_offset; /**< offset to payload */

	uint32_t vlan_s_tag;     /**< Parsed 1st VLAN header (S-TAG) */
	uint32_t vlan_c_tag;     /**< Parsed 2nd VLAN header (C-TAG) */
	uint32_t l3_protocol;    /**< Parsed L3 protocol */
	uint32_t l3_len;         /**< Layer 3 length */
	uint32_t l4_protocol;    /**< Parsed L4 protocol */
	uint32_t l4_len;         /**< Layer 4 length */
	uint32_t uarea_size;     /**< User metadata size, it's right after
				      odp_packet_hdr_t*/
	odp_crypto_generic_op_result_t op_result;  /**< Result for crypto */
} odp_packet_hdr_t __rte_cache_aligned;

/**
 * Return the packet header
 */
static inline odp_packet_hdr_t *odp_packet_hdr(odp_packet_t pkt)
{
	return (odp_packet_hdr_t *)pkt;
}

/**
 * Parse packet and set internal metadata
 */
void odp_packet_parse(odp_packet_t pkt, size_t len, size_t l2_offset);

static inline void _odp_packet_reset_parse(odp_packet_t pkt)
{
	odp_packet_hdr_t *pkt_hdr = odp_packet_hdr(pkt);
	pkt_hdr->input_flags.parsed_l2 = 0;
	pkt_hdr->input_flags.parsed_all = 0;
}

static inline void copy_packet_parser_metadata(odp_packet_hdr_t *src_hdr,
					       odp_packet_hdr_t *dst_hdr)
{
	dst_hdr->input_flags    = src_hdr->input_flags;
	dst_hdr->error_flags    = src_hdr->error_flags;
	dst_hdr->output_flags   = src_hdr->output_flags;

	dst_hdr->l2_offset      = src_hdr->l2_offset;
	dst_hdr->l3_offset      = src_hdr->l3_offset;
	dst_hdr->l4_offset      = src_hdr->l4_offset;
	dst_hdr->payload_offset = src_hdr->payload_offset;

	dst_hdr->vlan_s_tag     = src_hdr->vlan_s_tag;
	dst_hdr->vlan_c_tag     = src_hdr->vlan_c_tag;
	dst_hdr->l3_protocol    = src_hdr->l3_protocol;
	dst_hdr->l3_len         = src_hdr->l3_len;
	dst_hdr->l4_protocol    = src_hdr->l4_protocol;
	dst_hdr->l4_len         = src_hdr->l4_len;
}

/* Forward declarations */
int _odp_packet_copy_to_packet(odp_packet_t srcpkt, uint32_t srcoffset,
			       odp_packet_t dstpkt, uint32_t dstoffset,
			       uint32_t len);

static inline int packet_parse_l2_not_done(odp_packet_hdr_t *pkt_hdr)
{
	return !pkt_hdr->input_flags.parsed_l2;
}

static inline int packet_parse_not_complete(odp_packet_hdr_t *pkt_hdr)
{
	return !pkt_hdr->input_flags.parsed_all;
}

int _odp_packet_parse(odp_packet_hdr_t *pkt_hdr);

/* Fill in parser metadata for L2 */
void packet_parse_l2(odp_packet_hdr_t *pkt_hdr);

/* Perform full packet parse */
static inline int packet_parse_full(odp_packet_hdr_t *pkt_hdr)
{
	return _odp_packet_parse(pkt_hdr);
}

static inline uint32_t packet_len(odp_packet_hdr_t* pkt_hdr)
{
	return rte_pktmbuf_pkt_len(&pkt_hdr->buf_hdr.mb);
}

/* Reset parser metadata for a new parse */
void packet_parse_reset(odp_packet_hdr_t *pkt_hdr);

void _odp_packet_copy_md_to_packet(odp_packet_t srcpkt, odp_packet_t dstpkt);

/* Convert a packet handle to a buffer handle */
odp_buffer_t _odp_packet_to_buffer(odp_packet_t pkt);

/* Convert a buffer handle to a packet handle */
odp_packet_t _odp_packet_from_buffer(odp_buffer_t buf);

static inline int odp_packet_hdr_has_l2(odp_packet_hdr_t *pkt_hdr)
{
	return pkt_hdr->input_flags.l2;
}

static inline void odp_packet_hdr_has_l2_set(odp_packet_hdr_t *pkt_hdr, int val)
{
	pkt_hdr->input_flags.l2 = val;
}

/* DPDK will reserve RTE_PKTMBUF_HEADROOM in any case */
_ODP_STATIC_ASSERT(ODP_CONFIG_PACKET_HEADROOM == RTE_PKTMBUF_HEADROOM,
		   "ERROR: Headroom has to be equal to RTE_PKTMBUF_HEADROOM");

/* We can't enforce tailroom reservation for received packets */
_ODP_STATIC_ASSERT(ODP_CONFIG_PACKET_TAILROOM == 0,
		   "ERROR: Tailroom has to be 0, DPDK doesn't support this");

#ifdef __cplusplus
}
#endif

#endif
