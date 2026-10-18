#pragma once
#include "PDU.h"
#include <limits>

/*
 *
 * Some common definitions
 *
 */

constexpr std::size_t MAX_PDU_PAYLOAD_SIZE { 1024 };
constexpr std::size_t MAX_PDU_SIZE {sizeof(PDU_Header) + MAX_PDU_PAYLOAD_SIZE };

constexpr std::size_t MAX_FILE_NAME_SIZE = std::numeric_limits<std::uint8_t>::max();
constexpr std::size_t MAX_EXTENSION_NAME_SIZE = std::numeric_limits<std::uint8_t>::max();

// NOTE: Add max file size (see NOTE on PDU_START_Header struct)
// Theoretical limits of the header > can be stored here
constexpr std::size_t MAX_FILE_SIZE = std::numeric_limits<std::uintmax_t>::max();
