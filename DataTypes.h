#pragma once

#include "ExifUtils.h"
#include <unordered_map>
#include <vector>
#include <string>

extern const double INVALID_GEO_COORD;

// Just represents a latitude/longitude data
struct LatLon
{
  double              lat;
  double              lon;

  double              Distance(const LatLon& other) const;
  bool                IsValid() const;
};

// Image data collected from the image file
struct ImageData
{
  char                fileName[36];
  char                dateTime[36];
  LatLon              latLon;
  double              alt;
};
typedef std::vector<ImageData>                        ImageDataVec;
// Map of date string to the vector of image datas
typedef std::unordered_map<std::string, ImageDataVec> ImageDataMap;

// Just the image info we need in the array of images for a node
struct Image
{
  std::string         fileName;
  std::string         dateTime;
  Image(const char* fn, const char* dt) : fileName(fn), dateTime(dt) {}
};

// A location with an array of images taken in the vicinity
struct Node {
  LatLon              latLon;
  std::vector<Image>  images;
};

struct Day
{
  std::string         date;
  std::vector<LatLon> path;
  std::vector<Node>   nodes;
};


#include "FindFile.h"

void ProcessImageFile(const FileInfo& fi, ImageDataMap& mapImageData);
bool FillImageData(ImageData &imgData, const FileInfo& fi, unsigned char* tiff, unsigned char* pIFDExif, unsigned char* pIFDGpsInfo);
void FixInvalidGeoCoords(ImageDataMap& mapImageData);
void GenerateDays(std::vector<Day> &vecDays, const ImageDataMap& mapImageData, FileInfoSet &setProcessed);
