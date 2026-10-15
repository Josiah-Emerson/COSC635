#pragma once
#include "PDU.h"

/*
 *
 * Some common definitions
 *
 */

constexpr std::size_t MAX_PDU_PAYLOAD_SIZE { 1024 };
constexpr std::size_t MAX_PDU_SIZE {sizeof(PDU_Header) + MAX_PDU_PAYLOAD_SIZE };

// TODO: Add max file size (see NOTE on PDU_START_Header struct)
