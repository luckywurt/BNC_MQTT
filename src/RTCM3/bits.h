// Part of BNC, a utility for retrieving decoding and
// converting GNSS data streams from NTRIP broadcasters.
//
// Copyright (C) 2015
// German Federal Agency for Cartography and Geodesy (BKG)
// http://www.bkg.bund.de
// Alberding GmbH
// http://www.alberding.eu
//
// Email: euref-ip@bkg.bund.de
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation, version 2.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

#ifndef BITS_H
#define BITS_H

#define LOADBITS(a) { \
  while((a) > numbits) { \
    if(!size--) return false; \
    bitfield = (bitfield<<8)|*(data++); \
    numbits += 8; \
  } \
}

/* extract bits from data stream
   b = variable to store result, a = number of bits */
#define GETBITS64(b, a) { \
  if(((a) > 56) && ((a)-56) > numbits) { \
    uint64_t x; \
    GETBITS(x, 56) \
    LOADBITS((a)-56) \
    b = ((x<<((a)-56)) | (bitfield<<(sizeof(bitfield)*8-numbits)) \
    >>(sizeof(bitfield)*8-((a)-56))); \
    numbits -= ((a)-56); \
  } \
  else { \
    GETBITS(b, a) \
  } \
}

/* extract bits from data stream
   b = variable to store result, a = number of bits */
#define GETBITS(b, a) { \
  LOADBITS(a) \
  b = (bitfield<<(64-numbits))>>(64-(a)); \
  numbits -= (a); \
}

/* extract bits from data stream
   b = variable to store result, a = number of bits */
#define GETBITSFACTOR(b, a, c) { \
  LOADBITS(a) \
  b = ((bitfield<<(sizeof(bitfield)*8-numbits))>>(sizeof(bitfield)*8-(a)))*(c); \
  numbits -= (a); \
}

/* extract floating value from data stream
   b = variable to store result, a = number of bits */
#define GETFLOAT(b, a, c) { \
  LOADBITS(a) \
  b = ((double)((bitfield<<(64-numbits))>>(64-(a))))*(c); \
  numbits -= (a); \
}

/* extract signed floating value from data stream
   b = variable to store result, a = number of bits */
#define GETFLOATSIGN(b, a, c) { \
  LOADBITS(a) \
  b = ((double)(((int64_t)(bitfield<<(64-numbits)))>>(64-(a))))*(c); \
  numbits -= (a); \
}

/* extract bits from data stream
   b = variable to store result, a = number of bits */
#define GETBITSSIGN(b, a) { \
  LOADBITS(a) \
  b = ((int64_t)(bitfield<<(64-numbits)))>>(64-(a)); \
  numbits -= (a); \
}

#define GETFLOATSIGNM(b, a, c) { \
  int l; \
  LOADBITS(a) \
  l = (bitfield<<(64-numbits))>>(64-1); \
  b = ((double)(((bitfield<<(64-(numbits-1))))>>(64-(a-1))))*(c); \
  numbits -= (a); \
  if(l) b *= -1.0; \
}

#define SKIPBITS(b) { LOADBITS(b) numbits -= (b); }

/* extract byte-aligned byte from data stream,
   b = variable to store size, s = variable to store string pointer */
#define GETSTRING(b, s) { \
  b = *(data++); \
  s = (char *) data; \
  data += b; \
  size -= b+1; \
}

#define STARTDATA \
  size_t ressize=0; \
  char *blockstart=0; \
  int numbits=0; \
  uint64_t bitbuffer=0;


#define STOREBITS \
  while(numbits >= 8) { \
    if(!size) return 0; \
    *(buffer++) = bitbuffer>>(numbits-8); \
    numbits -= 8; \
    ++ressize; \
    --size; \
  }

#define ADDBITS(a, b) { \
    bitbuffer = (bitbuffer<<(a))|((b)&((1<<a)-1)); \
    numbits += (a); \
    STOREBITS \
  }


#define INITBLOCK \
  numbits = 0; \
  blockstart = buffer; \
  ADDBITS(8, 0xD3) \
  ADDBITS(6, 0) \
  ADDBITS(10, 0)

#define ENDBLOCK \
  if(numbits) { ADDBITS((8-numbits), 0) } { \
    int len = buffer-blockstart-3; \
    blockstart[1] |= len>>8; \
    blockstart[2] = len; \
    if(len > 1023) \
      return 0; \
    len = CRC24(len+3, (const unsigned char *) blockstart); \
    ADDBITS(24, len) \
  }

#define SCALEADDBITS(a, b, c) ADDBITS(a, (int64_t)(c > 0 ? b*c+0.5 : b*c-0.5))


// RTCM3 CRS encoding
//////////////////////////////////////////////////////////
#define CRSTOINT(type, value) static_cast<type>(round(value))

#define CRSADDBITS(a, b) {bitbuffer = (bitbuffer<<(a)) \
                       |(CRSTOINT(long long,b)&((1ULL<<a)-1)); \
                       numbits += (a); \
                       while(numbits >= 8) { \
                       buffer[size++] = bitbuffer>>(numbits-8);numbits -= 8;}}

#define CRSADDBITSFLOAT(a,b,c) {long long i = CRSTOINT(long long,(b)/(c)); \
                             CRSADDBITS(a,i)};

// RTCM3 GPS EPH encoding
//////////////////////////////////////////////////////////
#define GPSTOINT(type, value) static_cast<type>(round(value))

#define GPSADDBITS(a, b) {bitbuffer = (bitbuffer<<(a)) \
                       |(GPSTOINT(long long,b)&((1ULL<<a)-1)); \
                       numbits += (a); \
                       while(numbits >= 8) { \
                       buffer[size++] = bitbuffer>>(numbits-8);numbits -= 8;}}

#define GPSADDBITSFLOAT(a,b,c) {long long i = GPSTOINT(long long,(b)/(c)); \
                             GPSADDBITS(a,i)};

// RTCM3 GLONASS EPH encoding
//////////////////////////////////////////////////////////
#define GLONASSTOINT(type, value) static_cast<type>(round(value))

#define GLONASSADDBITS(a, b) {bitbuffer = (bitbuffer<<(a)) \
                       |(GLONASSTOINT(long long,b)&((1ULL<<(a))-1)); \
                       numbits += (a); \
                       while(numbits >= 8) { \
                       buffer[size++] = bitbuffer>>(numbits-8);numbits -= 8;}}

#define GLONASSADDBITSFLOATM(a,b,c) {int s; long long i; \
                       if(b < 0.0) \
                       { \
                         s = 1; \
                         i = GLONASSTOINT(long long,(-b)/(c)); \
                         if(!i) s = 0; \
                       } \
                       else \
                       { \
                         s = 0; \
                         i = GLONASSTOINT(long long,(b)/(c)); \
                       } \
                       GLONASSADDBITS(1,s) \
                       GLONASSADDBITS(a-1,i)}

// RTCM3 Galileo EPH encoding
//////////////////////////////////////////////////////////
#define GALILEOTOINT(type, value) static_cast<type>(round(value))

#define GALILEOADDBITS(a, b) {bitbuffer = (bitbuffer<<(a)) \
                       |(GALILEOTOINT(long long,b)&((1LL<<a)-1)); \
                       numbits += (a); \
                       while(numbits >= 8) { \
                       buffer[size++] = bitbuffer>>(numbits-8);numbits -= 8;}}

#define GALILEOADDBITSFLOAT(a,b,c) {long long i = GALILEOTOINT(long long,(b)/(c)); \
                             GALILEOADDBITS(a,i)};

// RTCM3 BDS EPH encoding
//////////////////////////////////////////////////////////
#define BDSTOINT(type, value) static_cast<type>(round(value))

#define BDSADDBITS(a, b) {bitbuffer = (bitbuffer<<(a)) \
                       |(BDSTOINT(long long,b)&((1ULL<<a)-1)); \
                       numbits += (a); \
                       while(numbits >= 8) { \
                       buffer[size++] = bitbuffer>>(numbits-8);numbits -= 8;}}

#define BDSADDBITSFLOAT(a,b,c) {long long i = BDSTOINT(long long,(b)/(c)); \
                             BDSADDBITS(a,i)};

// RTCM3 SBAS EPH encoding
//////////////////////////////////////////////////////////
#define SBASTOINT(type, value) static_cast<type>(round(value))

#define SBASADDBITS(a, b) {bitbuffer = (bitbuffer<<(a)) \
                       |(SBASTOINT(long long,b)&((1ULL<<a)-1)); \
                       numbits += (a); \
                       while(numbits >= 8) { \
                       buffer[size++] = bitbuffer>>(numbits-8);numbits -= 8;}}

#define SBASADDBITSFLOAT(a,b,c) {long long i = SBASTOINT(long long,(b)/(c)); \
                             SBASADDBITS(a,i)};

#endif /* BITS_H */
