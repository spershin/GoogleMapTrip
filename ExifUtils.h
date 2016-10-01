#pragma once

enum EFormat : unsigned short
{
  UnsignedByte = 1,
  AsciiStrings = 2,
  UnsignedShort = 3,
  UnsignedLong = 4,
  UnsignedRational = 5,
  SignedByte = 6,
  Undefined = 7,
  SignedShort = 8,
  SignedLong = 9,
  SignedRational = 10,
  Float = 11,
  Double = 12,
};

struct GeoCoord
{
  double  deg;
  double  min;
  double  sec;
  char    ns_ew; // 'N' or 'S' for Lat; 'E' or 'W' fr Lon

  double  calculate() const;
};

void            exifSetIsIntel(bool isIntel);
unsigned short  exifGetUShort(void* p);
unsigned int    exifGetUInt(void* p);
void            exifGetGeoCoord(GeoCoord& geoCoord, void* p);
double          exifGetURational(void* p);

#pragma pack(push, 1)
struct ExifDirEntry
{
  unsigned short    tag;
  EFormat           format;
  unsigned int      numComponents;
  unsigned int      data;

  void              intelize();
};
#pragma pack(pop)
