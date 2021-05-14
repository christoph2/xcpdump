/* SPDX-License-Identifier: (GPL-2.0-only OR BSD-3-Clause) */
/*
 * xcpdump.c - dump and explain ASAM MC-1 XCP protocol CAN frames
 *
 * Copyright (c) 2021 Christoph Schueler
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
 * 3. Neither the name of Volkswagen nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * Alternatively, provided that this notice is retained in full, this
 * software may be distributed under the terms of the GNU General
 * Public License ("GPL") version 2, in which case the provisions of the
 * GPL apply INSTEAD OF those given above.
 *
 * The provided data structures and external interfaces from this code
 * are not restricted to be used by modules with a GPL compatible license.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 *
 * Send feedback to <cpu12.gems@googlemail.com>
 *
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include <linux/can.h>

#include "xcp.h"

/*
 *
 * Local Function-like Macros.
 *
 */

/*
 * Get n-th byte from CAN frame.
 *
 * Note: By convention, this function-like macro requires a variable called `msg` of type `XcpMessage`.
 */
#define MSG_BYTE(n)             (msg->frame->data[(n)])

#define MSG_FRAME_LEN()         (msg->frame->len)

#define MSG_BOOL(idx, mask)     ((MSG_BYTE((idx)) & (mask)) ? "TRUE" : "FALSE")

#define MSG_WORD(idx)           (XCP_MAKEWORD(MSG_BYTE((idx) + 1), MSG_BYTE((idx))))

#define MSG_DWORD(idx)           (XCP_MAKEDWORD(                        \
                XCP_MAKEWORD(MSG_BYTE((idx) + 3), MSG_BYTE((idx) + 2)), \
                XCP_MAKEWORD(MSG_BYTE((idx) + 1), MSG_BYTE((idx)))      \
            ))

#if !defined(XCP_LOBYTE)
#define XCP_LOBYTE(w)   ((uint8_t)((w) & (uint8_t)0xff))            /**< Get the low-byte from a word. */
#endif

#if !defined(XCP_HIBYTE)
#define XCP_HIBYTE(w)   ((uint8_t)(((w)  & (uint16_t)0xff00) >> 8)) /**< Get the high-byte from a word. */
#endif

#if !defined(XCP_LOWORD)
#define XCP_LOWORD(w)   ((uint16_t)((w) & (uint16_t)0xffff))        /**< Get the low-word from a double-word. */
#endif

#if !defined(XCP_HIWORD)
#define XCP_HIWORD(w)   ((uint16_t)(((w)  & (uint32_t)0xffff0000) >> 16))   /**< Get the high-word from a doubleword. */
#endif

#if !defined(XCP_MAKEWORD)
#define XCP_MAKEWORD(h, l) ((((uint16_t)((h) & ((uint8_t)0xff))) <<  (uint16_t)8) | ((uint16_t)((l) & ((uint8_t)0xff)))) /**< Make word from high and low bytes. */
#endif

#if !defined(XCP_MAKEDWORD)
#define XCP_MAKEDWORD(h, l)  ((((uint32_t)((h) & ((uint16_t)0xffffu))) << (uint32_t)16) | ((uint32_t)((l) & ((uint16_t)0xffffu)))) /**< Make double-word from high and low words. */
#endif


/*
 *
 * Local Types.
 *
 */
typedef struct tagGetSegmentInfoRequestType {
   uint8_t mode;
   uint8_t segmentInfo;
   uint8_t mappingIndex;
} GetSegmentInfoRequestType;


/*
 *
 * Local Variables.
 *
 */
static GetSegmentInfoRequestType getSegmentInfoRequest;
static uint8_t get_sector_info_mode;
static bool include_dtos = FALSE;
static CanIdType CanIds;

/*
 * To dissect positive responses we need to know what was requested.
 */
static uint8_t service_request = 0;

/*
 *
 * Local Functions.
 *
 */
static void hexdump_xcp_message(XcpMessage const * const msg, uint16_t offset);
#if 0
static void hexdump(uint8_t * data, size_t size);
#endif
static void print_xcp_request(XcpMessage const * const msg);
static void print_xcp_response(XcpMessage const * const msg);
static void print_xcp_positive_response(XcpMessage const * const msg);
static uint16_t print_requested_service(XcpMessage const * const msg);
static void print_resources(uint8_t res, uint8_t variant);
static void print_comm_mode_basic(uint8_t mode);
static void print_current_session_status(uint8_t status);
static void print_set_cal_page_mode(uint8_t mode);
static void print_checksum_method(uint8_t meth);
static void print_get_pag_processor_info(uint8_t properties);
static void print_get_page_info(uint8_t properties);
static void print_segment_mode(uint8_t mode);
static void print_daq_list_mode(uint8_t mode);
static void print_daq_properties(uint8_t properties);
static void print_daq_key_byte(uint8_t key);
static void print_daq_timestamp_mode(uint8_t mode);
static void print_daq_get_list_mode(uint8_t mode);
static void print_daq_event_properties(uint8_t properties);
static void print_event_channel_time_unit(uint8_t unit);
static void print_daq_list_properties(uint8_t properties);
static void print_pgm_comm_mode(uint8_t mode);
static void print_pgm_properties(uint8_t properties);
static void print_event(XcpMessage const * const msg);



void setIdentifiers(CanIdType const * const ids)
{
    memcpy(&CanIds, ids, sizeof(CanIdType));
}

#if 0
/*
 * Plain HexDumper
 *
 */
static void hexdump(uint8_t * data, size_t size)
{
    for (int i = 0; i < size; i++) {
        printf("0x%02x ", data[i]);
    }
}
#endif

/*
 * Main entry point of this module.
 *
 * Just differentiate between requests and responses.
 *
 */
void print_xcp_message(XcpMessage const * const msg, bool dtos)
{
    include_dtos = dtos;

    if (msg->frame->can_id == CanIds.src) {
        print_xcp_request(msg);
    } else if (msg->frame->can_id == CanIds.dst) {
        print_xcp_response(msg);
    } else {
        hexdump_xcp_message(msg, 0);
    }
}

/*
 *  Print message content as HEX bytes.
 *
 */
static void hexdump_xcp_message(XcpMessage const * const msg, uint16_t offset)
{
    unsigned idx;

    if ((msg->frame->len - offset) == 0) {
        return;
    }

    printf("[ ");
    for (idx = offset; idx < msg->frame->len; ++idx) {
            printf("%02X ", MSG_BYTE(idx));
    }
    printf("]");
}

static void print_xcp_request(XcpMessage const * const msg)
{
    uint16_t idx;

    service_request = MSG_BYTE(0);
    idx = print_requested_service(msg);
    hexdump_xcp_message(msg, idx);
    printf(")");
}

static void print_xcp_response(XcpMessage const * const msg)
{
    uint8_t code = MSG_BYTE(0);
    uint16_t idx;

    printf("<- ");
    switch (code) {
        case 0xff:  /* Positive Response    */
            printf("OK");
            print_xcp_positive_response(msg);
            break;
        case 0xfe:  /* Error                */
            idx = 2;
            printf("ERROR(");
            switch (MSG_BYTE(1)) {
                case ERR_CMD_SYNCH:
                    printf("CMD_SYNCH");
                    break;
                case ERR_CMD_BUSY:
                    printf("CMD_BUSY");
                    break;
                case ERR_DAQ_ACTIVE:
                    printf("DAQ_ACTIVE");
                    break;
                case ERR_PGM_ACTIVE:
                    printf("PGM_ACTIVE");
                    break;
                case ERR_CMD_UNKNOWN:
                    printf("CMD_UNKNOWN");
                    break;
                case ERR_CMD_SYNTAX:
                    printf("CMD_SYNTAX");
                    break;
                case ERR_OUT_OF_RANGE:
                    printf("OUT_OF_RANGE");
                    break;
                case ERR_WRITE_PROTECTED:
                    printf("WRITE_PROTECTED");
                    break;
                case ERR_ACCESS_DENIED:
                    printf("ACCESS_DENIED");
                    break;
                case ERR_ACCESS_LOCKED:
                    printf("ACCESS_LOCKED");
                    break;
                case ERR_PAGE_NOT_VALID:
                    printf("PAGE_NOT_VALID");
                    break;
                case ERR_MODE_NOT_VALID:
                    printf("MODE_NOT_VALID");
                    break;
                case ERR_SEGMENT_NOT_VALID:
                    printf("SEGMENT_NOT_VALID");
                    break;
                case ERR_SEQUENCE:
                    printf("SEQUENCE");
                    break;
                case ERR_DAQ_CONFIG:
                    printf("DAQ_CONFIG");
                    break;
                case ERR_MEMORY_OVERFLOW:
                    printf("MEMORY_OVERFLOW");
                    break;
                case ERR_GENERIC:
                    printf("GENERIC");
                    break;
                case ERR_VERIFY:
                    printf("VERIFY");
                    break;
                case ERR_RESOURCE_TEMPORARY_NOT_ACCESSIBLE:
                    printf("RESOURCE_TEMPORARY_NOT_ACCESSIBLE");
                    break;
                case ERR_SUCCESS:
                    printf("SUCCESS");
                    break;
                default:
                    idx = 1;
                    break;
            }
            hexdump_xcp_message(msg, idx);
            printf(")");
            break;
        case 0xfd:  /* Event                */
            print_event(msg);
            break;
        case 0xfc:  /* Service Request      */
            printf("SERVICE REQ");
            /* TODO: service_request-request-code @pos #1 */
            hexdump_xcp_message(msg, 1);
            break;
        default:
            printf("DTO(pid = %u, ", code); /* we assume absolute ODT number in case of CAN. */
            hexdump_xcp_message(msg, 1);
            printf(")");
            break;
    }
    service_request = 0;
}


static void print_event(XcpMessage const * const msg)
{
    uint8_t event_id = MSG_BYTE(1);
    uint16_t idx = 2;
    uint8_t event_type = MSG_BYTE(2);
    uint16_t event_channel = MSG_WORD(4);
    uint16_t session_id = MSG_WORD(2);
    uint32_t timestamp = MSG_DWORD(4);

    printf("EVENT(id = ");
    switch (event_id) {
        case XCP_EV_RESUME_MODE:
            printf("EV_RESUME_MODE, sessionConfigurationId = %u, timestamp = %u", session_id, timestamp);
            idx = 8;
            break;
        case XCP_EV_CLEAR_DAQ:
            printf("EV_CLEAR_DAQ");
            break;
        case XCP_EV_STORE_DAQ:
            printf("EV_STORE_DAQ");
            break;
        case XCP_EV_STORE_CAL:
            printf("EV_STORE_CAL");
            break;
        case XCP_EV_CMD_PENDING:
            printf("EV_CMD_PENDING");
            break;
        case XCP_EV_DAQ_OVERLOAD:
            printf("EV_DAQ_OVERLOAD");
            break;
        case XCP_EV_SESSION_TERMINATED:
            printf("EV_SESSION_TERMINATED");
            break;
        case XCP_EV_TIME_SYNC:
            printf("EV_TIME_SYNC, timestamp = %u", timestamp);
            break;
        case XCP_EV_STIM_TIMEOUT:
            printf("EV_STIM_TIMEOUT, eventType = %s, eventChannel =%u",
                   event_type == 0 ? "EVENT_CHANNEL_NUMBER" : (event_type == 1 ? "DAQ LIST NUMBER" : "INVALID"),
                   event_channel
            );
            idx = 6;
            break;
        case XCP_EV_SLEEP:
            printf("EV_SLEEP");
            break;
        case XCP_EV_WAKE_UP:
            printf("EV_WAKE_UP");
            break;
        case XCP_EV_USER:
            printf("EV_USER");
            break;
        case XCP_EV_TRANSPORT:
            printf("EV_TRANSPORT");
            break;
        default:
            printf("0x%0x ", event_id);
            break;
    }
    hexdump_xcp_message(msg, idx);
    printf(")");
}

static uint16_t print_requested_service(XcpMessage const * const msg)
{
    uint16_t idx = 1;
    uint8_t service = MSG_BYTE(0);

    //static uint32_t counter = 0;

    //printf("\t\tRequ: %u [%u]\n", service_request,counter );
    //counter++;

    printf("-> ");
    switch (service) {
        case CONNECT:
            printf("CONNECT(mode = ");
            if (MSG_BYTE(1) == 0x00) {
                printf("NORMAL");
            } else if (MSG_BYTE(1) == 0x01) {
                printf("USER_DEFINED");
            }
            idx = 2;
            break;
        case DISCONNECT:
            printf("DISCONNECT(");
            break;
        case GET_STATUS:
            printf("GET_STATUS(");
            break;
        case SYNCH:
            printf("SYNCH(");
            break;
        case GET_COMM_MODE_INFO:
            printf("GET_COMM_MODE_INFO(");
            break;
        case GET_ID:
            printf("GET_ID(");
            printf("requestedIdentificationType = %u", MSG_BYTE(1));
            idx = 2;
            break;
        case SET_REQUEST:
            printf("SET_REQUEST(");
            printf("mode = {");
            printf("clearDaqReq = %s", MSG_BOOL(1, XCP_CLEAR_DAQ_REQ));
            printf(", storeDaqReqResume = %s", MSG_BOOL(1, XCP_STORE_DAQ_REQ_RESUME));
            printf(", storeDaqReqNoResume = %s", MSG_BOOL(1, XCP_STORE_DAQ_REQ_NO_RESUME));
            printf(", storeCalReq = %s", MSG_BOOL(1, XCP_STORE_CAL_REQ));
            printf("}");
            printf(", sessionConfigurationId = %u", MSG_WORD(2));
            idx = 3;
            break;
        case GET_SEED:
            printf("GET_SEED(");
            printf("mode = \"%s\"", (MSG_BYTE(1) == 0) ? "first part of seed" : "remaining part of seed");
            printf(", \"%s\"", (MSG_BYTE(2) == 0) ? "Resource" : "Don’t care");
            idx = 3;
            break;
        case UNLOCK:
            printf("UNLOCK(");
            printf("length = %u, key: ", MSG_BYTE(1));
            idx = 2;
            break;
        case SET_MTA:
            printf("SET_MTA(");
            printf("address = 0x%08x", MSG_DWORD(4));
            printf(", addressExtension = 0x%02x", MSG_BYTE(3));
            idx = 8;
            break;
        case UPLOAD:
            printf("UPLOAD(");
            printf("numberOfDataElements = %u", MSG_BYTE(1));
            idx = 2;
            break;
        case SHORT_UPLOAD:
            printf("SHORT_UPLOAD(");
            printf("numberOfDataElements = %u", MSG_BYTE(1));
            printf("address = 0x%08x", MSG_DWORD(4));
            printf(", addressExtension = 0x%02x", MSG_BYTE(3));
            idx = 8;
            break;
        case BUILD_CHECKSUM:
            printf("BUILD_CHECKSUM(");
            printf("blockSize = 0x%08x ", MSG_DWORD(4));
            idx = 8;
            break;
        case TRANSPORT_LAYER_CMD:
            printf("TRANSPORT_LAYER_CMD(");
            printf("subCommandCode = %u", MSG_BYTE(1));
            printf("parameters: ");
            hexdump_xcp_message(msg, 2);
            idx = 8;
        case USER_CMD:
            printf("USER_CMD(");
            printf("subCommandCode = %u", MSG_BYTE(1));
            printf("parameters: ");
            hexdump_xcp_message(msg, 2);
            idx = 8;
        case DOWNLOAD:
            printf("DOWNLOAD(");
            printf("numberOfDataElements = %u", MSG_BYTE(1));
            printf(", elements: ");
            hexdump_xcp_message(msg, 2);
            idx = 8;
            break;
        case DOWNLOAD_NEXT:
            printf("DOWNLOAD_NEXT(");
            printf("numberOfDataElements = %u", MSG_BYTE(1));
            printf(", elements: ");
            hexdump_xcp_message(msg, 2);
            idx = 8;
            break;
        case DOWNLOAD_MAX:
            printf("DOWNLOAD_MAX(");
            printf("elements: ");
            hexdump_xcp_message(msg, 1);
            idx = 8;
            break;
        case SHORT_DOWNLOAD:
            printf("SHORT_DOWNLOAD(");
            printf("numberOfDataElements = %u", MSG_BYTE(1));
            printf(", address = 0x%08x", MSG_DWORD(4));
            printf(", addressExtension = 0x%02x", MSG_BYTE(3));
            if (MSG_FRAME_LEN()) {
                printf("elements: ");
                hexdump_xcp_message(msg, 8);    /* In case of CAN-FD */
            }
            idx = 8;
            break;
        case MODIFY_BITS:
            printf("MODIFY_BITS(");
            printf("shiftValue = %u", MSG_BYTE(1));
            printf(", andMask = 0x%04x", MSG_WORD(2));
            printf(", xorMask = 0x%04x", MSG_WORD(4));
            idx = 6;
            break;
        case SET_CAL_PAGE:
            printf("SET_CAL_PAGE(");
            print_set_cal_page_mode(MSG_BYTE(1));
            printf(", logicalDataSegmentNumber = %u", MSG_BYTE(2));
            printf(", logicalDataPageNumber = %u", MSG_BYTE(3));
            idx = 4;
            break;
        case GET_CAL_PAGE:
            printf("GET_CAL_PAGE(");
            printf(", logicalDataPageNumber = %u", MSG_BYTE(3));
            idx = 4;
            break;
        case GET_PAG_PROCESSOR_INFO:
            printf("GET_PAG_PROCESSOR_INFO(");
            break;
        case GET_SEGMENT_INFO:
            printf("GET_SEGMENT_INFO(");
            getSegmentInfoRequest.mode = MSG_BYTE(1);
            printf("mode = ");
            if (getSegmentInfoRequest.mode == 0) {
                printf("0 [\"get basic address info for this SEGMENT\"]");
            } else if (getSegmentInfoRequest.mode == 1) {
                printf("1 [\"get standard info for this SEGMENT\"]");
            } else if (getSegmentInfoRequest.mode == 2) {
                printf("2 [\"get address mapping info for this SEGMENT\"]");
            } else {
                printf("%u [\"*** INVALID ***\"]", getSegmentInfoRequest.mode);
            }
            printf(", segmentNumber = %u", MSG_BYTE(2));
            getSegmentInfoRequest.segmentInfo = MSG_BYTE(3);
            if (getSegmentInfoRequest.mode == 0) {
                printf(", segmentInfo = \"%s\"", (getSegmentInfoRequest.segmentInfo == 0) ? "address" : "length");
            } else if (getSegmentInfoRequest.mode == 2) {
                printf(", segmentInfo = \"%s\"", (getSegmentInfoRequest.segmentInfo == 0) ? "sourceAddress" :
                       (getSegmentInfoRequest.segmentInfo == 1) ? "destinationAddress" : "lengthAddress"
                );
            }
            if (getSegmentInfoRequest.mode == 2) {
                printf(", mappingIndex = %u [\"identifier for address mapping range that MAPPING_INFO belongs to\"]", MSG_BYTE(4));
            }
            idx = 5;
            break;
        case GET_PAGE_INFO:
            printf("GET_PAGE_INFO(");
            printf("segmentNumber = %u", MSG_BYTE(2));
            printf(", pageNumber = %u", MSG_BYTE(3));
            break;
        case SET_SEGMENT_MODE:
            printf("SET_SEGMENT_MODE(");
            print_segment_mode(MSG_BYTE(1));
            printf(", segmentNumber = %u", MSG_BYTE(2));
            break;
        case GET_SEGMENT_MODE:
            printf("GET_SEGMENT_MODE(");
            printf(", segmentNumber = %u", MSG_BYTE(2));
            break;
        case COPY_CAL_PAGE:
            printf("COPY_CAL_PAGE(");
            printf("logicalDataSegmentNumberSource= %u", MSG_BYTE(1));
            printf(", logicalDataPageNumberSource = %u", MSG_BYTE(2));
            printf(", logicalDataSegmentNumberDestination = %u", MSG_BYTE(3));
            printf(", logicalDataPageNumberDestination = %u", MSG_BYTE(4));
            break;
        case CLEAR_DAQ_LIST:
            printf("CLEAR_DAQ_LIST(");
            printf("daqListNumber = %u", MSG_WORD(2));
            break;
        case SET_DAQ_PTR:
            printf("SET_DAQ_PTR(");
            printf("daqListNumber = %u", MSG_WORD(2));
            printf(", odtNumber = %u", MSG_BYTE(4));
            printf(", odtEntryNumber = %u", MSG_BYTE(5));
            break;
        case WRITE_DAQ:
            printf("WRITE_DAQ(");
            printf("bitOffset = %u", MSG_BYTE(1));
            printf(", sizeofElement = %u", MSG_BYTE(2));
            printf(", addressExtension = %u", MSG_BYTE(3));
            printf(", adddress = 0x%08x", MSG_DWORD(4));
            break;
        case SET_DAQ_LIST_MODE:
            printf("SET_DAQ_LIST_MODE(");
            print_daq_list_mode(MSG_BYTE(1));
            printf(" , daqListNumber = %u", MSG_WORD(2));
            printf(" , eventChannelNumber = %u", MSG_WORD(4));
            printf(" , transmissionRatePrescaler = %u", MSG_WORD(6));
            printf(" , daqListPriority = %u", MSG_BYTE(7));
            break;
        case GET_DAQ_LIST_MODE:
            printf("GET_DAQ_LIST_MODE(");
            printf("daqListNumber = %u", MSG_WORD(2));
            idx = 4;
            break;
        case START_STOP_DAQ_LIST:
            printf("START_STOP_DAQ_LIST(");
            printf("mode = ");
            switch (MSG_BYTE(1)) {
                case 0:
                    printf("STOP");
                    break;
                case 1:
                    printf("START");
                    break;
                case 2:
                    printf("SELECT");
                    break;
                default:
                    break;
            }
            printf(", daqListNumber = %u", MSG_WORD(2));
            break;
        case START_STOP_SYNCH:
            printf("START_STOP_SYNCH(");
            printf("mode = ");
            switch (MSG_BYTE(1)) {
                case 0:
                    printf("STOP_ALL");
                    break;
                case 1:
                    printf("START_SELECTED");
                    break;
                case 2:
                    printf("STOP_SELECTED");
                    break;
                default:
                    break;
            }
            break;
        case GET_DAQ_CLOCK:
            printf("GET_DAQ_CLOCK(");
            break;
        case READ_DAQ:
            printf("READ_DAQ(");
            break;
        case GET_DAQ_PROCESSOR_INFO:
            printf("GET_DAQ_PROCESSOR_INFO(");
            break;
        case GET_DAQ_RESOLUTION_INFO:
            printf("GET_DAQ_RESOLUTION_INFO(");
            break;
        case GET_DAQ_LIST_INFO:
            printf("GET_DAQ_LIST_INFO(");
            printf("daqListNumber = %u", MSG_WORD(2));
            idx = 4;
            break;
        case GET_DAQ_EVENT_INFO:
            printf("GET_DAQ_EVENT_INFO(");
            printf("eventChannelNumber = %u", MSG_WORD(2));
            idx = 4;
            break;
        case FREE_DAQ:
            printf("FREE_DAQ(");
            break;
        case ALLOC_DAQ:
            printf("ALLOC_DAQ(");
            printf("daqCount = %u", MSG_WORD(2));
            idx = 4;
            break;
        case ALLOC_ODT:
            printf("ALLOC_ODT(");
            printf("daqListNumber = %u", MSG_WORD(2));
            printf(", odtCount = %u", MSG_WORD(4));
            idx = 6;
            break;
        case ALLOC_ODT_ENTRY:
            printf("ALLOC_ODT_ENTRY(");
            printf("daqListNumber = %u", MSG_WORD(2));
            printf(", odtNumber = %u", MSG_WORD(4));
            printf(", odtEntriesCount = %u", MSG_BYTE(5));
            idx = 7;
            break;
        case PROGRAM_START:
            printf("PROGRAM_START(");
            break;
        case PROGRAM_CLEAR:
            printf("PROGRAM_CLEAR(");
            printf("accessMode = ");
            switch (MSG_BYTE(1)) {
                case 0:
                    printf("ABSOLUTE");
                    break;
                case 1:
                    printf("FUNCTIONAL");
                    break;
                default:
                    printf("\"INVALID\"");
                    break;
            }
            printf(", clearRange = %u", MSG_DWORD(4));
            break;
        case PROGRAM:
            printf("PROGRAM(");
            printf("numberOfDataElements = %u", MSG_BYTE(1));
            printf(", elements: ");
            hexdump_xcp_message(msg, 2);
            idx = 8;
            break;
        case PROGRAM_RESET:
            printf("PROGRAM_RESET(");
            break;
        case GET_PGM_PROCESSOR_INFO:
            printf("GET_PGM_PROCESSOR_INFO(");
            break;
        case GET_SECTOR_INFO:
            printf("GET_SECTOR_INFO(");
            printf("mode = ");
            get_sector_info_mode = MSG_BYTE(1);
            switch (MSG_BYTE(1)) {
                case 0:
                    printf("\"get start address for this SECTOR\"");
                    break;
                case 1:
                    printf("\"get length of this SECTOR[bytes]\"");
                    break;
                case 2:
                    printf("\"get name length of this SECTOR\"");
                    break;
                default:
                    break;
            }
            printf(", sectorNumber = %u", MSG_BYTE(2));
            break;
        case PROGRAM_PREPARE:
            printf("PROGRAM_PREPARE(");
            printf("Codesize[AG] = %u", MSG_WORD(2));
            break;
        case PROGRAM_FORMAT:
            printf("PROGRAM_FORMAT(");
            printf("compressionMethod = %u", MSG_BYTE(1));
            printf(", encryptionMethod = %u", MSG_BYTE(2));
            printf(", programmingMethod = %u", MSG_BYTE(3));
            printf(", accessMethod = %u", MSG_BYTE(4));
            break;
        case PROGRAM_NEXT:
            printf("PROGRAM_NEXT(");
            printf("numberOfDataElements = %u", MSG_BYTE(1));
            printf(", elements: ");
            hexdump_xcp_message(msg, 2);
            idx = 8;
            break;
        case PROGRAM_MAX:
            printf("PROGRAM_MAX(");
            printf("elements: ");
            hexdump_xcp_message(msg, 1);
            idx = 8;
            break;
        case PROGRAM_VERIFY:
            printf("PROGRAM_VERIFY(");
            printf("verificationMode = \"");
            if (MSG_BYTE(1) == 0) {
                printf("request to start internal routine");
            } else if (MSG_BYTE(1) == 1) {
                printf("sending Verification Value");
            }
            printf("\"");
            printf(", verificationType = %u", MSG_WORD(2));
            printf(", verificationValue = %u", MSG_DWORD(4));
            break;
        case WRITE_DAQ_MULTIPLE:
            printf("WRITE_DAQ_MULTIPLE(");
            printf("elements = [");
            for (uint8_t idx = 0; idx < MSG_BYTE(1); ++idx) {
                printf("{");
                printf("bitOffset = %u", MSG_BYTE((idx * 8) + 2));
                printf(", sizeofElement = %u", MSG_BYTE((idx * 8) + 3));
                printf(", adddress = 0x%08x", MSG_DWORD((idx * 8) + 4));
                printf(", addressExtension = %u", MSG_BYTE((idx * 8) + 8));
                printf("}, ");
            }
            printf("]");
            break;
        case TIME_CORRELATION_PROPERTIES:
            printf("TIME_CORRELATION_PROPERTIES(");
            break;
        case DTO_CTR_PROPERTIES:
            printf("DTO_CTR_PROPERTIES(");
            break;
#if 0
        case 0xC002:
            printf("GET_DAQ_PACKED_MODE(");
            break;
        case 0xC001:
            printf("SET_DAQ_PACKED_MODE(");
            break;
        case 0xC000:
            printf("GET_VERSION(");
            break;
#endif
        default:
            break;
    }
    return idx;
}

static void print_xcp_positive_response(XcpMessage const * const msg)
{
    switch (service_request) {
        case CONNECT:
            printf("(");
            print_resources(MSG_BYTE(1), 0);
            printf(", ");
            print_comm_mode_basic(MSG_BYTE(2));
            printf(", maxCto = %d", MSG_BYTE(3));
            printf(", maxDto = %d", XCP_MAKEWORD(MSG_BYTE(5), MSG_BYTE(4)));
            printf(", protocolLayerVersion = %d.%d", MSG_BYTE(6), MSG_BYTE(7));
            printf(")");
            break;
        case GET_STATUS:
            printf("(");
            print_current_session_status(MSG_BYTE(1));
            printf(", ");
            print_resources(MSG_BYTE(2), 1);
            printf(", ");
            printf("sessionConfigurationId = %d", XCP_MAKEWORD(MSG_BYTE(5), MSG_BYTE(4)));
            printf(")");
            break;
        case GET_COMM_MODE_INFO:
            printf("(");
            printf("commModeOptional = {");
            printf("masterBlockMode = %s", MSG_BOOL(2, XCP_MASTER_BLOCK_MODE));
            printf(", interleavedMode = %s", MSG_BOOL(2, XCP_INTERLEAVED_MODE));
            printf("}");
            printf(", maxBs = %u", MSG_BYTE(4));
            printf(", minSt = %u", MSG_BYTE(5));
            printf(", queueSize = %u", MSG_BYTE(6));
            printf(", XCPDriverVersion = %u.%u", (MSG_BYTE(7) & 0xf0) >> 8, MSG_BYTE(7) & 0x0f);
            printf(")");
            break;
        case GET_ID:
            printf("(");
            printf("mode = {");
            printf("compressedEncrypted = %s", MSG_BOOL(1, XCP_COMPRESSED_ENCRYPTED));
            printf(", transferMode = %s", MSG_BOOL(1, XCP_TRANSFER_MODE));
            printf("}");
            printf(", length = %u", XCP_MAKEDWORD(
                XCP_MAKEWORD(MSG_BYTE(7), MSG_BYTE(6)),
                XCP_MAKEWORD(MSG_BYTE(5), MSG_BYTE(4))
            ));
            if (MSG_FRAME_LEN()) {
                hexdump_xcp_message(msg, 8);    /* In case of CAN-FD */
            }
            printf(")");
            break;
        case GET_SEED:
            printf("(");
            printf("length = %u", MSG_BYTE(1));
            printf(", seed: ");
            hexdump_xcp_message(msg, 2);
            printf(")");
            break;
        case UNLOCK:
            printf("(");
            print_resources(MSG_BYTE(1), 1);
            printf(")");
            break;
        case UPLOAD:
            printf("(");
            printf("elements: ");
            hexdump_xcp_message(msg, 1);
            printf(")");
            break;
        case BUILD_CHECKSUM:
            printf("(");
            print_checksum_method(MSG_BYTE(1));
            printf(", checksum = 0x%08x", MSG_DWORD(4));
            printf(")");
            break;
        case TRANSPORT_LAYER_CMD:
            printf("(");
            hexdump_xcp_message(msg, 1);
            printf(")");
            break;
        case GET_PAG_PROCESSOR_INFO:
            printf("(");
            printf("maxSegment = %u, ", MSG_BYTE(1));
            print_get_pag_processor_info(MSG_BYTE(2));
            printf(")");
        case GET_SEGMENT_INFO:
            printf("(");
            if (getSegmentInfoRequest.mode == 0) {
                if (getSegmentInfoRequest.segmentInfo == 0) {
                    printf("address = 0x%08x", MSG_DWORD(4));
                } else if (getSegmentInfoRequest.segmentInfo == 1) {
                    printf("length = %u", MSG_DWORD(4));
                }
            } else if (getSegmentInfoRequest.mode == 1) {
                printf("maxPages = %u", MSG_BYTE(1));
                printf(", addressExtension = %u", MSG_BYTE(2));
                printf(", maxMapping = %u", MSG_BYTE(3));
                printf(", compressionMethod = %u", MSG_BYTE(4));
                printf(", encryptionMethod = %u", MSG_BYTE(5));
            } else if (getSegmentInfoRequest.mode == 2) {
                if (getSegmentInfoRequest.segmentInfo == 0) {
                    printf(", sourceAddress = 0x%08x", MSG_DWORD(4));
                } else if (getSegmentInfoRequest.segmentInfo == 1) {
                    printf(", destinationAddress = 0x%08x", MSG_DWORD(4));
                } else if (getSegmentInfoRequest.segmentInfo == 2) {
                    printf(", length = %u", MSG_DWORD(4));
                }
            }
            printf(")");
        case GET_PAGE_INFO:
            printf("(");
            print_get_page_info(MSG_BYTE(1));
            printf(", initSegment = %u", MSG_BYTE(2));
            printf(")");
        case GET_SEGMENT_MODE:
            printf("(");
            print_segment_mode(MSG_BYTE(2));
            printf(")");
            break;
        case START_STOP_DAQ_LIST:
            printf("(");
            printf("firstPID = %u", MSG_BYTE(1));
            printf(")");
            break;
        case GET_DAQ_CLOCK:
            printf("(");
            printf("timestamp = %u", MSG_WORD(4));
            printf(")");
            break;
        case GET_DAQ_PROCESSOR_INFO:
            printf("(");
            print_daq_properties(MSG_BYTE(1));
            printf(", minDaq = %u", MSG_BYTE(6));
            printf(", maxDaq = %u", MSG_WORD(2));
            printf(", maxEventChannel = %u", MSG_WORD(4));
            printf(", ");
            print_daq_key_byte(MSG_BYTE(7));
            printf(")");
            break;
        case GET_DAQ_RESOLUTION_INFO:
            printf("(");
            printf("granularityOdtEntrySizeDaq = %u", MSG_BYTE(1));
            printf(", maxOdtEntrySizeDaq = %u", MSG_BYTE(2));
            printf(", granularityOdtEntrySizeStim = %u", MSG_BYTE(3));
            printf(", maxOdtEntrySizeStim = %u", MSG_BYTE(4));
            printf(", ");
            print_daq_timestamp_mode(MSG_BYTE(5));
            printf(", timestampTicks = %u", MSG_BYTE(6));
            printf(")");
            break;
        case GET_DAQ_LIST_MODE:
            printf("(");
            print_daq_get_list_mode(MSG_BYTE(1));
            printf(", currentEventChannelNumber = %u", MSG_WORD(4));
            printf(", currentPrescaler = %u", MSG_BYTE(6));
            printf(", currentDaqListPriority = %u", MSG_BYTE(7));
            printf(")");
            break;
        case GET_DAQ_EVENT_INFO:
            printf("(");
            print_daq_event_properties(MSG_BYTE(1));
            printf(", maxDaqList = %u", MSG_BYTE(2));
            printf(", channelNameLength = %u", MSG_BYTE(3));
            printf(", channelTimeCycle = ");
            if (MSG_BYTE(4) == 0) {
                printf("\"not cyclic\"");
                printf(", channelTimeUnit = \"N/A\"");
            } else {
                printf("%u", MSG_BYTE(4));
            }
            if (MSG_BYTE(4) != 0) {
                printf(", ");
                print_event_channel_time_unit(MSG_BYTE(5));
            }
            printf(", channelPriority = %u", MSG_BYTE(6));
            printf(")");
            break;
        case GET_DAQ_LIST_INFO:
            printf("(");
            print_daq_list_properties(MSG_BYTE(1));
            printf(", maxOdt = %u", MSG_BYTE(2));
            printf(", maxOdtEntries = %u", MSG_BYTE(3));
            printf(", fixedEvent = %u", MSG_WORD(4));
            printf(")");
            break;
        case READ_DAQ:
            printf("(");
            printf("bitOffset = %u", MSG_BYTE(1));
            printf(", elementSize = %u", MSG_BYTE(2));
            printf(", addressExtension = 0x%02x", MSG_BYTE(3));
            printf(", address = 0x%08x", MSG_DWORD(4));
            printf(")");
            break;
        case PROGRAM_START:
            printf("(");
            print_pgm_comm_mode(MSG_BYTE(2));
            printf(", maxCtoPgm = %u", MSG_BYTE(3));
            printf(", maxBsPgm = %u", MSG_BYTE(4));
            printf(", minStPgm = %u", MSG_BYTE(5));
            printf(", queueSizePgm = %u", MSG_BYTE(6));
            printf(")");
            break;
        case GET_PGM_PROCESSOR_INFO:
            printf("(");
            print_pgm_properties(MSG_BYTE(1));
            printf(", maxSector = %u", MSG_BYTE(2));
            printf(")");
            break;
        case GET_SECTOR_INFO:
            printf("(");
            switch (get_sector_info_mode) {
                case 0:
                case 1:
                    printf("clearSequenceNumber = %u", MSG_BYTE(1));
                    printf(", programSequenceNumber = %u", MSG_BYTE(2));
                    printf(", programmingMethod = %u", MSG_BYTE(3));
                    if (get_sector_info_mode == 0) {
                        printf(", startAddress = 0x%08x", MSG_DWORD(4));
                    } else if (get_sector_info_mode == 0) {
                        printf(", length = %u", MSG_DWORD(4));
                    }
                    break;
                case 2:
                    printf("nameLength = %u", MSG_BYTE(1));
                    break;
                default:
                    break;
            }
            printf(")");
            break;
        default:
            printf("(");
            hexdump_xcp_message(msg, 1);
            printf(")");
            break;
    }
}

static void print_resources(uint8_t res, uint8_t variant)
{
    printf("%s = { ", (variant == 0) ? "resources" : "protected");
    if (res & XCP_RESOURCE_CAL_PAG) {
        printf("CAL_PAG ");
    }
    if (res & XCP_RESOURCE_DAQ) {
        printf("DAQ ");
    }
    if (res & XCP_RESOURCE_STIM) {
        printf("STIM ");
    }
    if (res & XCP_RESOURCE_PGM) {
        printf("PGM ");
    }
    printf("}");
}

static void print_comm_mode_basic(uint8_t mode)
{
    printf("commModeBasic = {");
    printf("byteOrder = ");
    if (mode & XCP_BYTE_ORDER_MOTOROLA) {
        printf("MOTOROLA");
    } else {
        printf("INTEL");
    }
    printf(", AG = ");
    if (mode & XCP_ADDRESS_GRANULARITY_WORD) {
        printf("WORD");
    } else if (mode & XCP_ADDRESS_GRANULARITY_DWORD) {
        printf("DWORD");
    } else {
        printf("BYTE");
    }
    printf(", slaveBlockMode = %s", (mode & XCP_SLAVE_BLOCK_MODE) ? "TRUE" : "FALSE");
    printf(", optional = %s", (mode & XCP_OPTIONAL_COMM_MODE) ? "TRUE" : "FALSE");
    printf("}");
}

static void print_current_session_status(uint8_t status)
{
    printf("sessionStatus = {");
    printf("storeCalReq = %s, ", (status & STORE_CAL_REQ) ? "SET" : "RESET");
    printf("storeDaqReq = %s, ", (status & STORE_DAQ_REQ) ? "SET" : "RESET");
    printf("clearCalReq = %s, ", (status & CLEAR_DAQ_REQ) ? "SET" : "RESET");
    printf("daqRunning = %s, ",  (status & DAQ_RUNNING) ? "TRUE" : "FALSE");
    printf("resume = %s",  (status & RESUME) ? "TRUE" : "FALSE");
    printf("}");
}

static void print_checksum_method(uint8_t meth)
{
    printf("checksumMethod = { ");
    switch (meth) {
        case XCP_CHECKSUM_METHOD_XCP_ADD_11:
            printf("XCP_ADD_11");
            break;
        case XCP_CHECKSUM_METHOD_XCP_ADD_12:
            printf("XCP_ADD_12");
            break;
        case XCP_CHECKSUM_METHOD_XCP_ADD_14:
            printf("XCP_ADD_14");
            break;
        case XCP_CHECKSUM_METHOD_XCP_ADD_22:
            printf("XCP_ADD_22");
            break;
        case XCP_CHECKSUM_METHOD_XCP_ADD_24:
            printf("XCP_ADD_24");
            break;
        case XCP_CHECKSUM_METHOD_XCP_ADD_44:
            printf("XCP_ADD_44");
            break;
        case XCP_CHECKSUM_METHOD_XCP_CRC_16:
            printf("XCP_CRC_16");
            break;
        case XCP_CHECKSUM_METHOD_XCP_CRC_16_CITT:
            printf("XCP_CRC_16_CITT");
            break;
        case XCP_CHECKSUM_METHOD_XCP_CRC_32:
            printf("XCP_CRC_32");
            break;
        case XCP_CHECKSUM_METHOD_XCP_USER_DEFINED:
            printf("USER_DEFINED");
            break;
        default:
            printf("%u", meth);
    }
    printf(" }");
}

static void print_set_cal_page_mode(uint8_t mode)
{
    printf("mode = {");
    if (mode & XCP_SET_CAL_PAGE_ALL) {
        printf(" ALL");
    }
    if (mode & XCP_SET_CAL_PAGE_XCP) {
        printf(" XCP");
    }
    if (mode & XCP_SET_CAL_PAGE_ECU) {
        printf(" ECU");
    }
    printf(" }");
}

static void print_get_pag_processor_info(uint8_t properties)
{
    printf("properties = { ");
    if (properties & XCP_PAG_PROCESSOR_FREEZE_SUPPORTED) {
        printf("FREEZE_SUPPORTED");
    }
    printf(" }");
}

static void print_get_page_info(uint8_t properties)
{
    printf("properties = { ");

    printf("ecuAccessType = ");
    switch (properties & 3) {
        case 0:
            printf("\"ECU access not allowed\"");
            break;
        case 1:
            printf("\"without XCP only\"");
            break;
        case 2:
            printf("\"with XCP only\"");
            break;
        case 3:
            break;
    }
    printf(", xcpReadAccessType = ");
    switch (properties & 12) {
        case 0:
            printf("\"XCP READ access not allowed\"");
            break;
        case 4:
            printf("\"without ECU only\"");
            break;
        case 8:
            printf("\"with ECU only\"");
            break;
        case 12:
            break;
    }
    printf(", xcpWriteAccessType = ");
    switch (properties & 48) {
        case 0:
            printf("\"XCP WRITE access not allowed\"");
            break;
        case 16:
            printf("\"without ECU only\"");
            break;
        case 32:
            printf("\"with ECU only\"");
            break;
        case 48:
            break;
    }
    printf(" }");
}

static void print_segment_mode(uint8_t mode)
{
    printf("segmentMode = { ");
    printf("freeze = ");
    if (mode & XCP_SEGMENT_MODE_FREEZE) {
        printf("ENABLE");
    } else {
        printf("DISABLE");
    }
    printf(" }");
}

static void print_daq_list_mode(uint8_t mode)
{
    printf(" mode = { ");
    if (mode & XCP_DAQ_LIST_MODE_ALTERNATING) {
        printf(" ALTERNATING");
    }
    if (mode & XCP_DAQ_LIST_MODE_DIRECTION) {
        printf(" DIRECTION");
    }
    if (mode & XCP_DAQ_LIST_MODE_TIMESTAMP) {
        printf(" TIMESTAMP");
    }
    if (mode & XCP_DAQ_LIST_MODE_PID_OFF) {
        printf(" PID_OFF");
    }
}

static void print_daq_properties(uint8_t properties)
{
    uint8_t ovl;

    printf("daqProperties = {");

    printf("daqConfigType = ");
    if (properties & XCP_DAQ_PROP_DAQ_CONFIG_TYPE) {
        printf("DYNAMIC");
    } else {
        printf("STATIC");
    }
    printf(", prescalerSupported = ");
    if (properties & XCP_DAQ_PROP_PRESCALER_SUPPORTED) {
        printf("TRUE");
    } else {
        printf("FALSE");
    }
    printf(", resumeSupported = ");
    if (properties & XCP_DAQ_PROP_RESUME_SUPPORTED) {
        printf("TRUE");
    } else {
        printf("FALSE");
    }
    printf(", bitStimSupported = ");
    if (properties & XCP_DAQ_PROP_BIT_STIM_SUPPORTED) {
        printf("TRUE");
    } else {
        printf("FALSE");
    }
    printf(", timestampSupported = ");
    if (properties & XCP_DAQ_PROP_TIMESTAMP_SUPPORTED) {
        printf("TRUE");
    } else {
        printf("FALSE");
    }
    printf(", pidOffSupported = ");
    if (properties & XCP_DAQ_PROP_PID_OFF_SUPPORTED) {
        printf("TRUE");
    } else {
        printf("FALSE");
    }
    printf(", overloadIndicationType = ");
    ovl = (properties & (XCP_DAQ_PROP_OVERLOAD_EVENT | XCP_DAQ_PROP_OVERLOAD_MSB)) >> 6;
    switch (ovl) {
        case 0:
            printf("\"no overload indication\"");
            break;
        case 1:
            printf("\"overload indication in MSB of PID\"");
            break;
        case 2:
            printf("\"overload indication by Event Packet\"");
            break;
        case 3:
            printf("\"not allowed\"");
            break;
        default:
            break;
    }
    printf("}");
}

static void print_daq_key_byte(uint8_t key)
{
    uint8_t ext;
    uint8_t idf;

    printf("keyByte = {");
    printf("optimisationType = ");
    switch (key & (XCP_DAQ_KEY_OPTIMISATION_TYPE_3 | XCP_DAQ_KEY_OPTIMISATION_TYPE_2 |
                   XCP_DAQ_KEY_OPTIMISATION_TYPE_1 | XCP_DAQ_KEY_OPTIMISATION_TYPE_0)) {
        case 0:
            printf("OM_DEFAULT");
            break;
        case 1:
            printf("OM_ODT_TYPE_16");
            break;
        case 2:
            printf("OM_ODT_TYPE_32");
            break;
        case 3:
            printf("OM_ODT_TYPE_64");
            break;
        case 4:
            printf("OM_ODT_TYPE_ALIGNMENT");
            break;
        case 5:
            printf("OM_MAX_ENTRY_SIZE");
            break;
        default:
            printf("\"INVALID\"");
            break;
    }
    printf(", addressExtensionType = ");
    ext = (key & (XCP_DAQ_KEY_ADDRESS_EXTENSION_DAQ | XCP_DAQ_KEY_ADDRESS_EXTENSION_ODT)) >> 4;
    switch (ext) {
        case 0:
            printf("\"address extension can be different within one and the same ODT\"");
            break;
        case 1:
            printf("\"address extension to be the same for all entries within one ODT\"");
            break;
        case 2:
            printf("\"Not allowed\"");
            break;
        case 3:
            printf("\"address extension to be the same for all entries within one DAQ\"");
            break;
        default:
            break;
    }
    printf(", identificationFieldType = ");
    idf = (key & (XCP_DAQ_KEY_IDENTIFICATION_FIELD_TYPE_1 | XCP_DAQ_KEY_IDENTIFICATION_FIELD_TYPE_0)) >> 6;
    switch (idf) {
        case 0:
            printf("\"Absolute ODT number\"");
            break;
        case 1:
            printf("\"Relative ODT number, absolute DAQ list number (BYTE)\"");
            break;

        case 2:
            printf("\"Relative ODT number, absolute DAQ list number (WORD)\"");
            break;

        case 3:
            printf("\"Relative ODT number, absolute DAQ list number (WORD, aligned)\"");
            break;
        default:
            break;
    }
    printf("}");
}

static void print_daq_timestamp_mode(uint8_t mode)
{
    uint8_t unit;

    printf("timestampMode = {");
    printf("size = ");
    switch (mode & (DAQ_TIME_STAMP_MODE_SIZE_2 | DAQ_TIME_STAMP_MODE_SIZE_1 | DAQ_TIME_STAMP_MODE_SIZE_0)) {
        case 0:
            printf("\"no timestamp\"");
            break;
        case 1:
            printf("1");
            break;
        case 2:
            printf("2");
            break;
        case 3:
            printf("\"not allowed\"");
            break;
        case 4:
            printf("4");
            break;
        default:
            printf("\"INVALID\"");
            break;
    }
    printf(", unit = ");
    unit = (mode & (DAQ_TIME_STAMP_MODE_UNIT_3 | DAQ_TIME_STAMP_MODE_UNIT_2 |
                    DAQ_TIME_STAMP_MODE_UNIT_1 | DAQ_TIME_STAMP_MODE_UNIT_0)) >> 4;
    switch (unit) {
        case 0:
            printf("1ns");
            break;
        case 1:
            printf("10ns");
            break;
        case 2:
            printf("100ns");
            break;
        case 3:
            printf("1us");
            break;
        case 4:
            printf("10us");
            break;
        case 5:
            printf("100us");
            break;
        case 6:
            printf("1ms");
            break;
        case 7:
            printf("10ms");
            break;
        case 8:
            printf("100ms");
            break;
        case 9:
            printf("1s");
            break;
        case 10:
            printf("1ps");
            break;
        case 11:
            printf("10ps");
            break;
        case 12:
            printf("100ps");
            break;
        default:
            printf("\"INVALID\"");
            break;
    }
    printf(", fixed = %s", (mode & DAQ_TIME_STAMP_MODE_TIMESTAMP_FIXED) ? "TRUE" : "FALSE");
    printf("}");
}

static void print_daq_get_list_mode(uint8_t mode)
{
    printf("mode = {");
    printf("resume = \"%s\"", (mode & DAQ_CURRENT_LIST_MODE_RESUME) ?
           "list is part of a RESUME configuration" : "list is NOT part of a RESUME configuration"
    );
    printf(", running = \"%s\"", (mode & DAQ_CURRENT_LIST_MODE_RUNNING) ?
           "DAQ list is active" : "DAQ list is inactive"
    );
    printf(", packetIdentifierTransmitted = %s", (mode & DAQ_CURRENT_LIST_MODE_PID_OFF) ? "FALSE" : "TRUE");
    printf(", timestamp = %s", (mode & DAQ_CURRENT_LIST_MODE_TIMESTAMP) ? "TRUE" : "FALSE");
    printf(", direction = %s", (mode & DAQ_CURRENT_LIST_MODE_DIRECTION) ? "STIM" : "DAQ");
    printf(", selected = %s", (mode & DAQ_CURRENT_LIST_MODE_SELECTED) ? "TRUE" : "FALSE");
    printf(" }");
}

static void print_daq_event_properties(uint8_t properties)
{
    uint8_t eventChannelType;
    uint8_t consistency;

    printf("eventProperties = {");

    eventChannelType = (properties & (XCP_DAQ_EVENT_CHANNEL_TYPE_DAQ | XCP_DAQ_EVENT_CHANNEL_TYPE_STIM)) >> 2;
    consistency = (properties & (XCP_DAQ_CONSISTENCY_EVENT_CHANNEL | XCP_DAQ_CONSISTENCY_DAQ_LIST)) >> 6;
    printf("eventChannelType = ");
    switch (eventChannelType) {
        case 0:
            printf("\"not allowed\"");
            break;
        case 1:
            printf("\"DIRECTION = DAQ only\"");
            break;
        case 2:
            printf("\"DIRECTION = STIM only\"");
            break;
        case 3:
            printf("\"DIRECTION DAQ and STIM\"");
            break;
        default:
            break;
    }
    printf(", consistency = ");
    switch (consistency) {
        case 0:
            printf("\"ODT level consistency\"");
            break;
        case 1:
            printf("\"DAQ list level consistency\"");
            break;
        case 2:
            printf("\"Event Channel level consistency\"");
            break;
        default:
            break;
    }
    printf(" }");
}


static void print_event_channel_time_unit(uint8_t unit)
{
    printf("unit = ");
    switch (unit) {
        case XCP_DAQ_EVENT_CHANNEL_TIME_UNIT_1NS:
            printf("1ns");
            break;
        case XCP_DAQ_EVENT_CHANNEL_TIME_UNIT_10NS:
            printf("10ns");
            break;
        case XCP_DAQ_EVENT_CHANNEL_TIME_UNIT_100NS:
            printf("100ns");
            break;
        case XCP_DAQ_EVENT_CHANNEL_TIME_UNIT_1US:
            printf("1us");
            break;
        case XCP_DAQ_EVENT_CHANNEL_TIME_UNIT_10US:
            printf("10us");
            break;
        case XCP_DAQ_EVENT_CHANNEL_TIME_UNIT_100US:
            printf("100us");
            break;
        case XCP_DAQ_EVENT_CHANNEL_TIME_UNIT_1MS:
            printf("1ms");
            break;
        case XCP_DAQ_EVENT_CHANNEL_TIME_UNIT_10MS:
            printf("10ms");
            break;
        case XCP_DAQ_EVENT_CHANNEL_TIME_UNIT_100MS:
            printf("100ms");
            break;
        case XCP_DAQ_EVENT_CHANNEL_TIME_UNIT_1S:
            printf("1s");
            break;
        case XCP_DAQ_EVENT_CHANNEL_TIME_UNIT_1PS:
            printf("1ps");
            break;
        case XCP_DAQ_EVENT_CHANNEL_TIME_UNIT_10PS:
            printf("10ps");
            break;
        case XCP_DAQ_EVENT_CHANNEL_TIME_UNIT_100PS:
            printf("100ps");
            break;

    }
}

static void print_daq_list_properties(uint8_t properties)
{
    uint8_t daqListType;
    printf("properties = {");

    printf("configurationType = ");
    if (properties & DAQ_LIST_PROPERTY_PREDEFINED) {
        printf("PREDEFINED");
    } else {
        printf("CHANGEABLE");
    }
    printf(", eventChannelAssignment = ");
    if (properties & DAQ_LIST_PROPERTY_EVENT_FIXED) {
        printf("FIXED");
    } else {
        printf("CHANGEABLE");
    }
    daqListType = (properties & (DAQ_LIST_PROPERTY_STIM | DAQ_LIST_PROPERTY_DAQ)) >> 2;
    printf(", daqListType = ");
    switch (daqListType) {
        case 0:
            printf("\"Not allowed\"");
            break;
        case 1:
            printf("\"DIRECTION = DAQ only\"");
            break;
        case 2:
            printf("\"DIRECTION = STIM only\"");
            break;
        case 3:
            printf("\"DIRECTION DAQ or STIM\"");
            break;
        default:
            break;
    }

    printf("}");
}

static void print_pgm_comm_mode(uint8_t mode)
{
    printf("mode = {");
    printf("interleavedMode = %s", (mode & XCP_PGM_COMM_MODE_INTERLEAVED_MODE) ? "TRUE" : "FALSE");
    printf(", masterBlockmode = %s", (mode & XCP_PGM_COMM_MODE_MASTER_BLOCK_MODE) ? "TRUE" : "FALSE");
    printf(", slaveBlockmode = %s", (mode & XCP_PGM_COMM_MODE_SLAVE_BLOCK_MODE) ? "TRUE" : "FALSE");
    printf("}");
}

static void print_pgm_properties(uint8_t properties)
{
    uint8_t mode;
    uint8_t compression;
    uint8_t encryption;
    uint8_t non_sequential_programming;

    mode = (properties & (XCP_PGM_FUNCTIONAL_MODE | XCP_PGM_ABSOLUTE_MODE));
    compression = (properties & (XCP_PGM_COMPRESSION_REQUIRED | XCP_PGM_COMPRESSION_SUPPORTED)) >> 2;
    encryption = (properties & (XCP_PGM_ENCRYPTION_REQUIRED | XCP_PGM_ENCRYPTION_SUPPORTED)) >> 4;
    non_sequential_programming = (properties & (XCP_PGM_NON_SEQ_PGM_REQUIRED | XCP_PGM_NON_SEQ_PGM_SUPPORTED)) >> 6;

    printf("properties = {");
    printf("clearPprogrammingMode = ");
    switch (mode) {
        case 0:
            printf("\"Not allowed\"");
            break;
        case 1:
            printf("\"Only ABSOLUTE\"");
            break;
        case 2:
            printf("\"Only FUNCTIONAL\"");
            break;
        case 3:
            printf("\"ABSOLUTE and FUNCTIONAL \"");
            break;
    }
    printf(", compression = ");
    switch (compression) {
        case 0:
            printf("\"not supported\"");
            break;
        case 1:
            printf("\"supported\"");
            break;
        case 2:
        case 3:
            printf("\"supported and required\"");
            break;
    }
    printf(", encryption = ");
    switch (encryption) {
        case 0:
            printf("\"not supported\"");
            break;
        case 1:
            printf("\"supported\"");
            break;
        case 2:
        case 3:
            printf("\"supported and required\"");
            break;
    }
    printf(", nonSequentialProgramming = ");
    switch (non_sequential_programming) {
        case 0:
            printf("\"not supported\"");
            break;
        case 1:
            printf("\"supported\"");
            break;
        case 2:
        case 3:
            printf("\"supported and required\"");
            break;
    }
    printf("}");
}

