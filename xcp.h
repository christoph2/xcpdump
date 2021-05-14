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
#ifndef __XCP_H
#define __XCP_H

#include <stdbool.h>

#include <linux/can.h>

/*
 * Defines
 */
#define TRUE    (1)
#define FALSE   (0)

/*
 * Types
 */

typedef struct tagCanIdType {
    canid_t src;
    canid_t dst;
} CanIdType;

typedef struct tagXcpMessage {
    canid_t src;
    canid_t dst;
    struct canfd_frame * frame;
} XcpMessage;

/*
 * Global Functions
 *
 */
void print_xcp_message(XcpMessage const * const msg, bool dtos);
void setIdentifiers(CanIdType const * const ids);

/*
 * Standard Events.
 */
#define XCP_EV_RESUME_MODE              (0x00)
#define XCP_EV_CLEAR_DAQ                (0x01)
#define XCP_EV_STORE_DAQ                (0x02)
#define XCP_EV_STORE_CAL                (0x03)
#define XCP_EV_CMD_PENDING              (0x05)
#define XCP_EV_DAQ_OVERLOAD             (0x06)
#define XCP_EV_SESSION_TERMINATED       (0x07)
#define XCP_EV_TIME_SYNC                (0x08)
#define XCP_EV_STIM_TIMEOUT             (0x09)
#define XCP_EV_SLEEP                    (0x0A)
#define XCP_EV_WAKE_UP                  (0x0B)
#define XCP_EV_USER                     (0xFE)
#define XCP_EV_TRANSPORT                (0xFF)

/*
 * Standard Service Requests.
 */
#define XCP_SERV_RESET                  (0x00)
#define XCP_SERV_TEXT                   (0x01)

/*
 * Resources.
 */
#define XCP_RESOURCE_PGM                (16)
#define XCP_RESOURCE_STIM               (8)
#define XCP_RESOURCE_DAQ                (4)
#define XCP_RESOURCE_CAL_PAG            (1)

/*
 * Comm Mode Basic.
 */
#define XCP_OPTIONAL_COMM_MODE          ((uint8_t)0x80)
#define XCP_SLAVE_BLOCK_MODE            ((uint8_t)0x40)
#define XCP_ADDRESS_GRANULARITY_0       ((uint8_t)2)
#define XCP_ADDRESS_GRANULARITY_1       ((uint8_t)4)
#define XCP_ADDRESS_GRANULARITY_BYTE    ((uint8_t)0)
#define XCP_ADDRESS_GRANULARITY_WORD    (XCP_ADDRESS_GRANULARITY_0)
#define XCP_ADDRESS_GRANULARITY_DWORD   (XCP_ADDRESS_GRANULARITY_1)
#define XCP_BYTE_ORDER_INTEL            ((uint8_t)0)
#define XCP_BYTE_ORDER_MOTOROLA         ((uint8_t)1)

/*
 * Current Session Status.
 */
#define RESUME                          (0x80)
#define DAQ_RUNNING                     (0x40)
#define CLEAR_DAQ_REQ                   (0x08)
#define STORE_DAQ_REQ                   (0x04)
#define STORE_CAL_REQ                   (0x01)


/*
 * Comm Mode Optional
 */
#define XCP_MASTER_BLOCK_MODE           (1)
#define XCP_INTERLEAVED_MODE            (2)

/*
 * GetID Mode.
 */
#define XCP_COMPRESSED_ENCRYPTED        (2)
#define XCP_TRANSFER_MODE               (1)

/*
 * SetRequest Mode.
 */
#define XCP_CLEAR_DAQ_REQ               (8)
#define XCP_STORE_DAQ_REQ_RESUME        (4)
#define XCP_STORE_DAQ_REQ_NO_RESUME     (2)
#define XCP_STORE_CAL_REQ               (1)


/*
 * Checksum Methods.
 */
#define XCP_CHECKSUM_METHOD_XCP_ADD_11      (1)
#define XCP_CHECKSUM_METHOD_XCP_ADD_12      (2)
#define XCP_CHECKSUM_METHOD_XCP_ADD_14      (3)
#define XCP_CHECKSUM_METHOD_XCP_ADD_22      (4)
#define XCP_CHECKSUM_METHOD_XCP_ADD_24      (5)
#define XCP_CHECKSUM_METHOD_XCP_ADD_44      (6)
#define XCP_CHECKSUM_METHOD_XCP_CRC_16      (7)
#define XCP_CHECKSUM_METHOD_XCP_CRC_16_CITT (8)
#define XCP_CHECKSUM_METHOD_XCP_CRC_32      (9)
#define XCP_CHECKSUM_METHOD_XCP_USER_DEFINED    (0xff)

/*
 * SetCalPage Mode.
 */
#define XCP_SET_CAL_PAGE_ALL                    (0x80)
#define XCP_SET_CAL_PAGE_XCP                    (0x02)
#define XCP_SET_CAL_PAGE_ECU                    (0x01)

/*
 * PAG Processor Properties.
 */
#define XCP_PAG_PROCESSOR_FREEZE_SUPPORTED      (1)

/*
 * Page Properties.
 */
#define XCP_WRITE_ACCESS_WITH_ECU           (32)
#define XCP_WRITE_ACCESS_WITHOUT_ECU        (16)
#define XCP_READ_ACCESS_WITH_ECU            (8)
#define XCP_READ_ACCESS_WITHOUT_ECU         (4)
#define ECU_ACCESS_WITH_XCP                 (2)
#define ECU_ACCESS_WITHOUT_XCP              (1)

/*
 * Segment Mode
 */
#define XCP_SEGMENT_MODE_FREEZE             (1)

/*
 * DAQ List Modes.
 */
#define XCP_DAQ_LIST_MODE_ALTERNATING       ((uint8_t)0x01)
#define XCP_DAQ_LIST_MODE_DIRECTION         ((uint8_t)0x02)
#define XCP_DAQ_LIST_MODE_TIMESTAMP         ((uint8_t)0x10)
#define XCP_DAQ_LIST_MODE_PID_OFF           ((uint8_t)0x20)
#define XCP_DAQ_LIST_MODE_SELECTED          ((uint8_t)0x40)
#define XCP_DAQ_LIST_MODE_STARTED           ((uint8_t)0x80)


/*
 * DAQ Properties
 */
#define XCP_DAQ_PROP_OVERLOAD_EVENT         ((uint8_t)0x80)
#define XCP_DAQ_PROP_OVERLOAD_MSB           ((uint8_t)0x40)
#define XCP_DAQ_PROP_PID_OFF_SUPPORTED      ((uint8_t)0x20)
#define XCP_DAQ_PROP_TIMESTAMP_SUPPORTED    ((uint8_t)0x10)
#define XCP_DAQ_PROP_BIT_STIM_SUPPORTED     ((uint8_t)0x08)
#define XCP_DAQ_PROP_RESUME_SUPPORTED       ((uint8_t)0x04)
#define XCP_DAQ_PROP_PRESCALER_SUPPORTED    ((uint8_t)0x02)
#define XCP_DAQ_PROP_DAQ_CONFIG_TYPE        ((uint8_t)0x01)

/*
 * DAQ Key Byte
 */
#define XCP_DAQ_KEY_IDENTIFICATION_FIELD_TYPE_1 ((uint8_t)0x80)
#define XCP_DAQ_KEY_IDENTIFICATION_FIELD_TYPE_0 ((uint8_t)0x40)
#define XCP_DAQ_KEY_ADDRESS_EXTENSION_DAQ       ((uint8_t)0x20)
#define XCP_DAQ_KEY_ADDRESS_EXTENSION_ODT       ((uint8_t)0x10)
#define XCP_DAQ_KEY_OPTIMISATION_TYPE_3         ((uint8_t)0x08)
#define XCP_DAQ_KEY_OPTIMISATION_TYPE_2         ((uint8_t)0x04)
#define XCP_DAQ_KEY_OPTIMISATION_TYPE_1         ((uint8_t)0x02)
#define XCP_DAQ_KEY_OPTIMISATION_TYPE_0         ((uint8_t)0x01)

/*
 * DAQ Timestamp Mode
 */
#define DAQ_TIME_STAMP_MODE_UNIT_3              (0x80)
#define DAQ_TIME_STAMP_MODE_UNIT_2              (0x40)
#define DAQ_TIME_STAMP_MODE_UNIT_1              (0x20)
#define DAQ_TIME_STAMP_MODE_UNIT_0              (0x10)
#define DAQ_TIME_STAMP_MODE_TIMESTAMP_FIXED     (0x08)
#define DAQ_TIME_STAMP_MODE_SIZE_2              (0x04)
#define DAQ_TIME_STAMP_MODE_SIZE_1              (0x02)
#define DAQ_TIME_STAMP_MODE_SIZE_0              (0x01)

/*
 * DAQ List Mode
 */
#define DAQ_CURRENT_LIST_MODE_RESUME            (0x80)
#define DAQ_CURRENT_LIST_MODE_RUNNING           (0x40)
#define DAQ_CURRENT_LIST_MODE_PID_OFF           (0x20)
#define DAQ_CURRENT_LIST_MODE_TIMESTAMP         (0x10)
#define DAQ_CURRENT_LIST_MODE_DIRECTION         (0x02)
#define DAQ_CURRENT_LIST_MODE_SELECTED          (0x01)

/*
 * DAQ Event Channel Properties
 */
#define XCP_DAQ_EVENT_CHANNEL_TYPE_DAQ      ((uint8_t)0x04)
#define XCP_DAQ_EVENT_CHANNEL_TYPE_STIM     ((uint8_t)0x08)

/*
 * DAQ Consistency
 */
#define XCP_DAQ_CONSISTENCY_DAQ_LIST        ((uint8_t)0x40)
#define XCP_DAQ_CONSISTENCY_EVENT_CHANNEL   ((uint8_t)0x80)


/*
 * DAQ Time Units
 */
#define XCP_DAQ_EVENT_CHANNEL_TIME_UNIT_1NS    (0)
#define XCP_DAQ_EVENT_CHANNEL_TIME_UNIT_10NS   (1)
#define XCP_DAQ_EVENT_CHANNEL_TIME_UNIT_100NS  (2)
#define XCP_DAQ_EVENT_CHANNEL_TIME_UNIT_1US    (3)
#define XCP_DAQ_EVENT_CHANNEL_TIME_UNIT_10US   (4)
#define XCP_DAQ_EVENT_CHANNEL_TIME_UNIT_100US  (5)
#define XCP_DAQ_EVENT_CHANNEL_TIME_UNIT_1MS    (6)
#define XCP_DAQ_EVENT_CHANNEL_TIME_UNIT_10MS   (7)
#define XCP_DAQ_EVENT_CHANNEL_TIME_UNIT_100MS  (8)
#define XCP_DAQ_EVENT_CHANNEL_TIME_UNIT_1S     (9)
#define XCP_DAQ_EVENT_CHANNEL_TIME_UNIT_1PS    (10)
#define XCP_DAQ_EVENT_CHANNEL_TIME_UNIT_10PS   (11)
#define XCP_DAQ_EVENT_CHANNEL_TIME_UNIT_100PS  (12)

/*
 * DAQ list properties
 */
#define DAQ_LIST_PROPERTY_STIM                  (8)
#define DAQ_LIST_PROPERTY_DAQ                   (4)
#define DAQ_LIST_PROPERTY_EVENT_FIXED           (2)
#define DAQ_LIST_PROPERTY_PREDEFINED            (1)

/*
 * Commm Mode PGM
 */
#define XCP_PGM_COMM_MODE_SLAVE_BLOCK_MODE      (0x40)
#define XCP_PGM_COMM_MODE_INTERLEAVED_MODE      (0x02)
#define XCP_PGM_COMM_MODE_MASTER_BLOCK_MODE     (0x01)

/*
**  PGM Capabilities.
*/
#define XCP_PGM_NON_SEQ_PGM_REQUIRED            (0x80)
#define XCP_PGM_NON_SEQ_PGM_SUPPORTED           (0x40)
#define XCP_PGM_ENCRYPTION_REQUIRED             (0x20)
#define XCP_PGM_ENCRYPTION_SUPPORTED            (0x10)
#define XCP_PGM_COMPRESSION_REQUIRED            (8)
#define XCP_PGM_COMPRESSION_SUPPORTED           (4)
#define XCP_PGM_FUNCTIONAL_MODE                 (2)
#define XCP_PGM_ABSOLUTE_MODE                   (1)

/*
 * Service Codes.
 */
#define GET_DAQ_PACKED_MODE             (0xC002)
#define SET_DAQ_PACKED_MODE             (0xC001)
#define GET_VERSION                     (0xC000)
#define CONNECT                         (0xFF)
#define DISCONNECT                      (0xFE)
#define GET_STATUS                      (0xFD)
#define SYNCH                           (0xFC)
#define GET_COMM_MODE_INFO              (0xFB)
#define GET_ID                          (0xFA)
#define SET_REQUEST                     (0xF9)
#define GET_SEED                        (0xF8)
#define UNLOCK                          (0xF7)
#define SET_MTA                         (0xF6)
#define UPLOAD                          (0xF5)
#define SHORT_UPLOAD                    (0xF4)
#define BUILD_CHECKSUM                  (0xF3)
#define TRANSPORT_LAYER_CMD             (0xF2)
#define USER_CMD                        (0xF1)
#define DOWNLOAD                        (0xF0)
#define DOWNLOAD_NEXT                   (0xEF)
#define DOWNLOAD_MAX                    (0xEE)
#define SHORT_DOWNLOAD                  (0xED)
#define MODIFY_BITS                     (0xEC)
#define SET_CAL_PAGE                    (0xEB)
#define GET_CAL_PAGE                    (0xEA)
#define GET_PAG_PROCESSOR_INFO          (0xE9)
#define GET_SEGMENT_INFO                (0xE8)
#define GET_PAGE_INFO                   (0xE7)
#define SET_SEGMENT_MODE                (0xE6)
#define GET_SEGMENT_MODE                (0xE5)
#define COPY_CAL_PAGE                   (0xE4)
#define CLEAR_DAQ_LIST                  (0xE3)
#define SET_DAQ_PTR                     (0xE2)
#define WRITE_DAQ                       (0xE1)
#define SET_DAQ_LIST_MODE               (0xE0)
#define GET_DAQ_LIST_MODE               (0xDF)
#define START_STOP_DAQ_LIST             (0xDE)
#define START_STOP_SYNCH                (0xDD)
#define GET_DAQ_CLOCK                   (0xDC)
#define READ_DAQ                        (0xDB)
#define GET_DAQ_PROCESSOR_INFO          (0xDA)
#define GET_DAQ_RESOLUTION_INFO         (0xD9)
#define GET_DAQ_LIST_INFO               (0xD8)
#define GET_DAQ_EVENT_INFO              (0xD7)
#define FREE_DAQ                        (0xD6)
#define ALLOC_DAQ                       (0xD5)
#define ALLOC_ODT                       (0xD4)
#define ALLOC_ODT_ENTRY                 (0xD3)
#define PROGRAM_START                   (0xD2)
#define PROGRAM_CLEAR                   (0xD1)
#define PROGRAM                         (0xD0)
#define PROGRAM_RESET                   (0xCF)
#define GET_PGM_PROCESSOR_INFO          (0xCE)
#define GET_SECTOR_INFO                 (0xCD)
#define PROGRAM_PREPARE                 (0xCC)
#define PROGRAM_FORMAT                  (0xCB)
#define PROGRAM_NEXT                    (0xCA)
#define PROGRAM_MAX                     (0xC9)
#define PROGRAM_VERIFY                  (0xC8)
#define WRITE_DAQ_MULTIPLE              (0xC7)
#define TIME_CORRELATION_PROPERTIES     (0xC6)
#define DTO_CTR_PROPERTIES              (0xC5)


/*
 * Error Codes.
 */
#define ERR_CMD_SYNCH           (0x00)   /* Command processor synchronization.                            */
#define ERR_CMD_BUSY            (0x10)   /* Command was not executed.                                     */
#define ERR_DAQ_ACTIVE          (0x11)   /* Command rejected because DAQ is running.                      */
#define ERR_PGM_ACTIVE          (0x12)   /* Command rejected because PGM is running.                      */
#define ERR_CMD_UNKNOWN         (0x20)   /* Unknown command or not implemented optional command.          */
#define ERR_CMD_SYNTAX          (0x21)   /* Command syntax invalid                                        */
#define ERR_OUT_OF_RANGE        (0x22)   /* Command syntax valid but command parameter(s) out of range.   */
#define ERR_WRITE_PROTECTED     (0x23)   /* The memory location is write protected.                       */
#define ERR_ACCESS_DENIED       (0x24)   /* The memory location is not accessible.                        */
#define ERR_ACCESS_LOCKED       (0x25)   /* Access denied, Seed & Key is required                         */
#define ERR_PAGE_NOT_VALID      (0x26)   /* Selected page not available                                   */
#define ERR_MODE_NOT_VALID      (0x27)   /* Selected page mode not available                              */
#define ERR_SEGMENT_NOT_VALID   (0x28)   /* Selected segment not valid                                    */
#define ERR_SEQUENCE            (0x29)   /* Sequence error                                                */
#define ERR_DAQ_CONFIG          (0x2A)   /* DAQ configuration not valid                                   */
#define ERR_MEMORY_OVERFLOW     (0x30)   /* Memory overflow error                                         */
#define ERR_GENERIC             (0x31)   /* Generic error.                                                */
#define ERR_VERIFY              (0x32)   /* The slave internal program verify routine detects an error.   */
#define ERR_RESOURCE_TEMPORARY_NOT_ACCESSIBLE (0x33)  /* Access to the requested resource is temporary not possible. */
#define ERR_SUCCESS             (0xff)



#endif /* __XCP_H */

