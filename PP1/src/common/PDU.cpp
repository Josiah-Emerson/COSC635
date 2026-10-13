#include "PDU.h"
#include <cassert>
#include <random>

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
   // bytes: 0 1 2 3 4 5 6 7
   // as laid out in the seq number field

   int offset = 56; // Beginning bit offset into num
   for(std::size_t i { 0 }; i < 8; ++i){
      seqNumField[i] = static_cast<std::uint8_t>(num >> offset);
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
   int offset = 56;
   std::uint64_t val { 0 };

   for(std::size_t i { 0 }; i < 8; ++i){
      val = val | (static_cast<std::uint64_t>(seq_field[i]) << offset);
      offset -= 8;
   }

   return val;
}
