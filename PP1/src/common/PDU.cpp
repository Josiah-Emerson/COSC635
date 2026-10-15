#include "PDU.h"
#include <cassert>
#include <random>

void U64ToBytes(std::uint64_t num, std::uint8_t* bytes){
   // bytes: 0 1 2 3 4 5 6 7

   int offset = 56; // beginning bit offset into val
   for(std::size_t i { 0 }; i < 8; ++i){
      bytes[i] = static_cast<std::uint8_t>(num >> offset);
      offset -= 8;
   }
}

std::uint64_t bytesToU64(const std::uint8_t* bytes){
   int offset = 56;
   std::uint64_t val { 0 };

   for(std::size_t i { 0 }; i < 8; ++i){
      val = val | (static_cast<std::uint64_t>(bytes[i]) << offset);
      offset -= 8;
   }

   return val;
}

// Header Encoding Functions
std::uint8_t encodeTypeVersionField(int version, Msg_Type msgType){
   assert(version < 64 && version >= 0 && "Invalid version number. Must be in range 0-63");
   std::uint8_t res { 0b00000000 };
   switch(msgType){
      case(Msg_Type::START):
         res = 0b00000000;
         break;
      case(Msg_Type::DATA):
         res = 0b00000001;
         break;
      case(Msg_Type::END):
         res = 0b00000011;
         break;
      default: 
         assert(false && "Unhandled");
         break;
   }

   // preserve 2 least significant bits
   // shift version num to 6 most significant bits and store in res
   res = (res & 0b00000011) | (version << 2);

   return res;
}

void createTransferID(std::uint8_t* t_idField){
   // NOTE: not meant to be super secure just low chance of a collision
   std::random_device rd;
   std::mt19937 generator(rd());
   std::uniform_int_distribution<unsigned int> dist(0, 255);

   for(std::size_t i { 0 }; i < 3; ++i){
      t_idField[i] = static_cast<std::uint8_t>(dist(generator));
   }
}

void encodeSequenceNumber(std::uint64_t num, std::uint8_t* seqNumField){
   U64ToBytes(num, seqNumField);
}

void encodeFileSize(std::uintmax_t filesize, std::uint8_t* fileSizeField){
   // NOTE: I think the on my machine std::uintmax_t is not big enough for this 
   // bytes: 0 1 2 3 4 5 6 7 8 9
   // 64bit: X X 0 1 2 3 4 5 6 7
   // while our offset is larger than NUM_BITS that byte is equal to 0?

   constexpr int NUM_BITS = sizeof(filesize) * 8; // Needed since the num of bytes in file size field is larger than std::uintmax_t
   int offset = 72;
   for(std::size_t i { 0 }; i < 10; ++i){
      fileSizeField[i] = offset >= NUM_BITS ? 0 : 
         static_cast<std::uint8_t>(filesize >> offset);
      offset -= 8;
   }
}

// Header Decoding Functions
int decodeVersion(std::uint8_t typeVersion){
   // TODO: probably don't need to bounds check, but oh well
   int version = typeVersion >> 2;
   assert(version >= 0 && version < 64 && "Somehow the decoded version was not in the bounds");

   return version;
}

Msg_Type decodeMsgType(std::uint8_t typeVersion){
   Msg_Type type = Msg_Type::UNKNOWN;
   switch(typeVersion & 0b00000011){
      case(0b00000000):
         type = Msg_Type::START;
         break;
      case(0b00000001):
         type = Msg_Type::DATA;
         break;
      case(0b00000011):
         type = Msg_Type::END;
         break;
   }

   return type;
}

std::uint32_t decodeTransferID(std::uint8_t* t_idField){
   return (static_cast<std::uint32_t>(t_idField[0]) << 16 |
           static_cast<std::uint32_t>(t_idField[1]) << 8 |
           static_cast<std::uint32_t>(t_idField[2]));
}

std::uint64_t decodeSequenceNum(std::uint8_t* seq_field){
   return bytesToU64(seq_field);
}

std::uintmax_t decodeFileSize(std::uint8_t* fileSizeField){
   constexpr int NUM_BITS = sizeof(std::uintmax_t) * 8;
   int offset = 72;
   std::uintmax_t val { 0 };

   for(std::size_t i { 0 }; i < 10; ++i){
      if(offset >= NUM_BITS){
         // TODO: More here for testing
         assert(fileSizeField[i] == 0 && "Expected this byte to be 0 but it is not");
      }else{
         val = val | (static_cast<std::uintmax_t>(fileSizeField[i]) << offset);
      }

      offset -= 8;
   }

   return val;
}
