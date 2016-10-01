#include "stdafx.h"
#include "ExifUtils.h"

static bool isIntelExif = true; 

void exifSetIsIntel(bool isIntel)
{
  isIntelExif = isIntel;
}

unsigned short exifGetUShort(void* p)
{
  unsigned char* buf = (unsigned char*)p;
  if(!isIntelExif)
  {
    unsigned char temp = buf[0];
    buf[0] = buf[1];
    buf[1] = temp;
  }
  return *(unsigned short*)buf;
}

unsigned int exifGetUInt(void* p)
{
  unsigned char* buf = (unsigned char*)p;
  if(!isIntelExif)
  {
    unsigned char temp = buf[0];
    buf[0] = buf[3];
    buf[3] = temp;
    temp = buf[1];
    buf[1] = buf[2];
    buf[2] = temp;
  }
  return *(unsigned int*)buf;
}

void exifGetGeoCoord(GeoCoord& geoCoord, void* p)
{
  unsigned int* num = (unsigned int*)p;
  geoCoord.deg  = exifGetUInt(num + 0);
  geoCoord.deg /= exifGetUInt(num + 1);
  geoCoord.min  = exifGetUInt(num + 2);
  geoCoord.min /= exifGetUInt(num + 3);
  geoCoord.sec  = exifGetUInt(num + 4);
  geoCoord.sec /= exifGetUInt(num + 5);
}

double  GeoCoord::calculate() const
{
  double ret = deg + min / 60.0 + sec / 3600.0;
  if(ns_ew == 'S' || ns_ew == 'W')
    ret = -ret;
  return ret;
}

double exifGetURational(void* p)
{
  unsigned int* num = (unsigned int*)p;
  double ret = exifGetUInt(num + 0);
  return ret / exifGetUInt(num + 1);
}

void ExifDirEntry::intelize()
{
  if(!isIntelExif)
  {
    exifGetUShort(&tag);
    exifGetUShort(&format);
    exifGetUInt(&numComponents);
    unsigned int dataSize = numComponents;
    switch(format)
    {
      case UnsignedByte:
      case AsciiStrings:
      case Undefined:
      case SignedByte:  break;

      case UnsignedShort:
      case SignedShort: dataSize *= 2; break;

      case UnsignedLong:
      case SignedLong:
      case Float: dataSize *= 4; break;

      case UnsignedRational:
      case SignedRational: dataSize *= 8; break;
    }

    if(dataSize > 4 || (format != AsciiStrings && format != Undefined))
    {
      exifGetUInt(&data);
    }
  }
}
