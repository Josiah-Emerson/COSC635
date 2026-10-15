#pragma once
#include <cstdint>

// Types
enum class Msg_Type{
   START,
   DATA,
   END,
   UNKNOWN
};

struct PDU_Header{
   std::uint8_t   type_version;
   std::uint8_t   transfer_id[3];
   std::uint8_t   sequence_num[8]; // NOTE: using std::uint64_t adds padding so that the header size is no longer 16 bytes
   std::uint16_t  payload_len;     // NOTE: The max value of this shold be 1024
   std::uint16_t  reserved;
};

static_assert(sizeof(PDU_Header) == 16, "PDU_Header size is not what it should be");

struct PDU_START_Header{
   std::uint8_t fileSize_Bytes[10];    // NOTE: while this can hold a max value of 2^80 - 1, the max allowed file size is (2^64 - 1) * 1024
   std::uint8_t tot_DATA_PDU_Cnt[8];   // Number of PDU DATA to be sent
   std::uint8_t filename_size;         // size of the file name
   std::uint8_t extension_size;        // size of the extension name
};

static_assert(sizeof(PDU_START_Header) == 20, "PDU_START_Header size is not what it should be");

// Helper funcs

// stores u64 value in bytes 
// bytes is expcted to be an array of 8 std::uint8_t
void U64ToBytes(std::uint64_t num, std::uint8_t* bytes);

// returns the value stored in bytes as a uint64
std::uint64_t bytesToU64(const std::uint8_t* bytes);

// Header Encoding Functions
// returns the std::uint8_t corresponding to the type_version field of PDU_Header
// version must be between 0 and 63
std::uint8_t encodeTypeVersionField(int version, Msg_Type msgType);

// Expects a pointer to an array of 3 bytes to put the transfer ID in 
// The tranfer ID is currently a randomly generated 24 bit value
void createTransferID(std::uint8_t* t_idField);

// encodes the value num into the seqNum field. 
// seqNumField should be apointer to an array of 8 std::uint8_t 
void encodeSequenceNumber(std::uint64_t num, std::uint8_t* seqNumField);

// fileSize is size of the file in bytes 
// fileSizeField is a pointer to the first element of an array of 10 std::uint8_t 
void encodeFileSize(std::uintmax_t fileSize, std::uint8_t* fileSizeField);

// filesize is filesize in bytes
// pduCntField is a pointer to the tot_DATA_PDU_Cnt field and expected to be an array of 8 std::uint8_t
void encodeDATAPDUCnt(std::uintmax_t filesize, std::uint8_t* pduCntField);

// Header Decoding Functions 

// returns the PDU version specified by typeVersion
int decodeVersion(std::uint8_t typeVersion);

// returns the PDU msgType specified by typeVersion
Msg_Type decodeMsgType(std::uint8_t typeVersion);

// converts the 3 byte transfer ID field to a 32 bit uint 
// t_idField is expected to point to the first byte of the 3 byte field 
std::uint32_t decodeTransferID(std::uint8_t* t_idField);

// decodes the sequene number field
// argument seq_field is expected to be a pointer to the first element of 
// an array of 8 std::uint8_t values
std::uint64_t decodeSequenceNum(std::uint8_t* seq_field);

// Expects fileSizeField to be a pointer to 10 contiguous std::uint8_t
std::uintmax_t decodeFileSize(std::uint8_t* fileSizeField);

std::uint64_t decodeDATAPDUCnt();
