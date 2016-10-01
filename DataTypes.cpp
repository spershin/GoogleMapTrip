#include "stdafx.h"
#include "DataTypes.h"
#include "FindFile.h"
#include "Stats.h"
#include <algorithm>

#include "NodeGPSFixesCanadaTrip.h"

const double INVALID_GEO_COORD = 999.0;

static const size_t bufSize = 66000;
static unsigned char buf[bufSize + 10];
static unsigned char* end;

double LatLon::Distance(const LatLon& other) const
{
  double dx = lat - other.lat;
  double dy = lon - other.lon;
  return sqrt((dx * dx) + (dy * dy));
}

bool LatLon::IsValid() const
{
  return -90.0 <= lat && lat <= 90.0 && -180.0 <= lon && lon <= 180.0;
}

void ProcessImageFile(const FileInfo& fi, ImageDataMap& mapImageData)
{
  FILE* f = nullptr;
  fopen_s(&f, fi.path.c_str(), "rb");
  if(f == nullptr)
  {
    printf("ERROR: failed to open file: %s\n", fi.name.c_str());
    return;
  }

  // Should have more than 12 bytes to have any data
  const size_t read = fread(buf, 1, bufSize, f);
  fclose(f);
  if(read <= 12)
  {
    printf("ERROR: file size is too small: %s\n", fi.name.c_str());
    return;
  }

  ImageData imgData;
  strcpy_s(imgData.fileName, sizeof(imgData.fileName), fi.name.c_str());

  // jpg's 'start of the image' and exif's start
  if(buf[0] != 0xFF || buf[1] != 0xD8 || buf[2] != 0xFF || buf[3] != 0xE1)
  {
    printf("WARNING: file is not jpg with exif: %s\n", fi.name.c_str());
    // We assume it is mp4 and try to get data from it

    // Generate date/time from the name for now
    // fileName	"20160827_082123.mp4"
    // dateTime	"2016:08:27 08:21:23"
    memcpy(imgData.dateTime, imgData.fileName, 4);
    imgData.dateTime[4] = '.';
    memcpy(imgData.dateTime + 5, imgData.fileName + 4, 2);
    imgData.dateTime[7] = '.';
    memcpy(imgData.dateTime + 8, imgData.fileName + 6, 2);
    imgData.dateTime[10] = ' ';
    memcpy(imgData.dateTime + 11, imgData.fileName + 9, 2);
    imgData.dateTime[13] = ':';
    memcpy(imgData.dateTime + 14, imgData.fileName + 11, 2);
    imgData.dateTime[16] = ':';
    memcpy(imgData.dateTime + 17, imgData.fileName + 13, 2);
    imgData.dateTime[19] = '\0';

    // Lat/Lon are invalid for now
    imgData.latLon.lat = imgData.latLon.lon = INVALID_GEO_COORD;
  }
  else // jpg image with exif
  {
    // Swap bytes in the exif's size
    buf[0] = buf[4];
    buf[4] = buf[5];
    buf[5] = buf[0];
    unsigned short exifSize = *(unsigned short*)(buf + 4);
        
    // Example: Size of lake louise exif is 0x3D7D, means that exif ends at 0x3D7D - 2 + 4 = 0x00003D7F
    // That is where 0xFFD9 marker is (end of image), Then 0xFFE5 follows (for whatever reason)
    end = buf + exifSize - 2 + 4;

    // Next 6 bytes should be this:
    static const char EXIF_ID[] = { 'E', 'x', 'i', 'f', '\0', '\0' };
    if(memcmp(EXIF_ID, buf + 6, sizeof(EXIF_ID)) != 0)
    {
      printf("ERROR: invalid exif header: %s\n", fi.name.c_str());
      return;
    }

    // TIFF header
    unsigned char* tiff = buf + 12;

    // Byte align: 'II' - intel, 'MM' - motorolla
    exifSetIsIntel(tiff[0] == 'I' && tiff[1] == 'I');
    // Next is 0x2A00 or 0x002A
    // Next is 1st Image File Directory Offset 
    unsigned char* pIFD = tiff + exifGetUInt(tiff + 4);
    unsigned char* pIFDExif = nullptr;
    unsigned char* pIFDGpsInfo = nullptr;

    int foundExif = 0;
    int gotExif = false;

    // Read IFDs until the next's offset is zero and it points to TIFF header
    while(pIFD != tiff)
    {
      const unsigned short entryCount = exifGetUShort(pIFD);
      ExifDirEntry *de = (ExifDirEntry*)(pIFD + 2);
      ExifDirEntry *deEnd = de + entryCount;
      for(; de != deEnd; ++de)
      {
        de->intelize();
        switch(de->tag)
        {
          case 0x8769: pIFDExif = tiff + de->data;    break;
          case 0x8825: pIFDGpsInfo = tiff + de->data; break;
        }
      }
      pIFD = tiff + exifGetUInt(deEnd);
    }

    if(!FillImageData(imgData, fi, tiff, pIFDExif, pIFDGpsInfo))
      return;
  }

  // Apply fixes
  for(int i = 0; i < sizeof(nodeGPSFixes)/sizeof(nodeGPSFixes[0]); ++i)
  {
    if(strcmp(nodeGPSFixes[i].fileName, imgData.fileName) == 0)
    {
      imgData.latLon.lat = nodeGPSFixes[i].lat;
      imgData.latLon.lon = nodeGPSFixes[i].lon;
    }
  }
  std::string key(imgData.dateTime, 10);
  mapImageData[key].emplace_back(imgData);
}

bool FillImageData(ImageData &imgData, const FileInfo& fi, unsigned char* tiff, unsigned char* pIFDExif, unsigned char* pIFDGpsInfo)
{
  if(pIFDExif == nullptr || pIFDGpsInfo == nullptr)
  {
    printf("ERROR: No GPS or Exif data: %s\n", fi.name.c_str());
    return false;
  }

  while(pIFDExif != tiff)
  {
    const unsigned short entryCount = exifGetUShort(pIFDExif);
    ExifDirEntry *de = (ExifDirEntry*)(pIFDExif + 2);
    ExifDirEntry *deEnd = de + entryCount;
    for(; de != deEnd; ++de)
    {
      de->intelize();
      switch(de->tag)
      {
        //case 0x9003: // Date/time original
        case 0x9004: // Date/time digital generation
          strcpy_s(imgData.dateTime, sizeof(imgData.dateTime), (char*)(tiff + de->data));
          imgData.dateTime[4] = imgData.dateTime[7] = '.';
          break;
      }
    }

    pIFDExif = tiff + exifGetUInt(deEnd);
  }

  // Process GPS Block
  int gotGPSData = 0;
  GeoCoord  lat;
  GeoCoord  lon;
  while(pIFDGpsInfo != tiff)
  {
    const unsigned short entryCount = exifGetUShort(pIFDGpsInfo);
    ExifDirEntry *de = (ExifDirEntry*)(pIFDGpsInfo + 2);
    ExifDirEntry *deEnd = de + entryCount;
    for(; de != deEnd; ++de)
    {
      de->intelize();
      switch(de->tag)
      {
        case 0x0001: // North or South Latitude
          lat.ns_ew = *(char*)&de->data;  
          gotGPSData |= 1;
          break;

        case 0x0003: // East or West Longitude
          lon.ns_ew = *(char*)&de->data;
          gotGPSData |= 2;
          break;

        case 0x0002: // Latitude
          exifGetGeoCoord(lat, tiff + de->data);
          gotGPSData |= 4;
          break;

        case 0x0004: // Longitude
          exifGetGeoCoord(lon, tiff + de->data);
          gotGPSData |= 8;
          break;

        case 0x0006: // Altitude
          imgData.alt  = exifGetURational(tiff + de->data);
          break;
      }
    }

    pIFDGpsInfo = tiff + exifGetUInt(deEnd);
  }
  if(gotGPSData != (1|2|4|8))
  {
    printf("WARNING: GPS data is incomplete: %s\n", fi.name.c_str());
    imgData.latLon.lat = imgData.latLon.lon = INVALID_GEO_COORD;
  }
  else
  {
    imgData.latLon.lat = lat.calculate();
    imgData.latLon.lon = lon.calculate();
  }

  // We remove files with "(X)" and "_YYY" suffices.
  // Exception is "_001" which we rename to canonical name
  if(imgData.fileName[15] != '.')
  {
    if(memcmp(imgData.fileName + 15, "_001", sizeof(char) * 4) == 0)
    {
      strcpy_s(imgData.fileName + 15, sizeof(imgData.fileName) - 15, imgData.fileName + 19);
    }
    else
    {
      printf("ERROR: name is not canonical and not '*_001.*' - ignoring: %s\n", fi.name.c_str());
      return false;
    }
  }

  // Check if date/time corresponds to the name!
  // fileName	"20160827_082123.jpg"
  // dateTime	"2016:08:27 08:21:23"
  if(memcmp(imgData.fileName, imgData.dateTime, 4) != 0 ||
     memcmp(imgData.fileName + 4, imgData.dateTime + 5, 2) != 0 ||
     memcmp(imgData.fileName + 6, imgData.dateTime + 8, 2) != 0)
  {
    printf("ERROR: date and name's date are different: %s != %s\n", imgData.dateTime, fi.name.c_str());
    return false;
  }
  // Don't check time - seems like a lot of files have 1 second difference
  //if(memcmp(imgData.fileName + 9, imgData.dateTime + 11, 2) != 0 ||
  //   memcmp(imgData.fileName + 11, imgData.dateTime + 14, 2) != 0 ||
  //   memcmp(imgData.fileName + 13, imgData.dateTime + 17, 2) != 0)
  //{
  //  printf("time and name's time are different: %s != %s\n", imgData.dateTime, fi.name.c_str());
  //  return false;
  //}

  return true;
}

int ImageDataComparator(const void *r1, const void *r2)
{
	return strcmp(((const ImageData *)r1)->dateTime, ((const ImageData *)r2)->dateTime);
}

int GetTimeDiffInSeconds(const char* dateTime, const char* dateTimeNext)
{
  const char* time = dateTime + 11;
  const char* timeNext = dateTimeNext + 11;
  int h, m, s, hNext, mNext, sNext;
  if(3 == sscanf_s(time, "%d:%d:%d", &h, &m, &s) &&
     3 == sscanf_s(timeNext, "%d:%d:%d", &hNext, &mNext, &sNext))
  {
    sNext -= s;
    mNext -= m;
    hNext -= h;
    if(sNext < 0)
    {
      sNext += 60;
      mNext -= 1;
    }
    if(mNext < 0)
    {
      mNext += 60;
      hNext -= 1;
    }
    _ASSERTE(hNext >= 0 && mNext >= 0 && sNext >= 0);
    return sNext + mNext * 60 + hNext * 3600;
  }
  else
  {
    _ASSERTE(0);
  }
  return 0;
}


void FixInvalidGeoCoords(ImageDataMap& mapImageData)
{
  // Sort files inside each day by their date/time
  for(ImageDataMap::iterator itDayImagesData = mapImageData.begin(); itDayImagesData != mapImageData.end(); ++itDayImagesData)
  {
    ImageDataVec& vec = itDayImagesData->second;
    
    std::qsort(&vec[0], vec.size(), sizeof(ImageData), ImageDataComparator);
    goodImageCount += vec.size();
    
    for(ImageDataVec::iterator it = vec.begin(); it != vec.end();)
    {
      if(!it->latLon.IsValid())
      {
        const char* dateTime = it->dateTime;
        int secondsToNext = 60 * 60 * 24;

        // Find the nearest next with valid latLon
        for(ImageDataVec::const_iterator itNext = it + 1; itNext != vec.end(); ++itNext)
        {
          if(itNext->latLon.IsValid())
          {
            secondsToNext = GetTimeDiffInSeconds(dateTime, itNext->dateTime);
            it->latLon = itNext->latLon;
            break;
          }
        }

        // Find the nearest prev with valid latLon
        if(it != vec.begin())
        {
          ImageDataVec::const_iterator itPrev = it;
          while(itPrev != vec.begin())
          {
            --itPrev;
            if(itPrev->latLon.IsValid())
            {
              if(GetTimeDiffInSeconds(itPrev->dateTime, dateTime) < secondsToNext)
              {
                it->latLon = itPrev->latLon;
              }
              break;
            }
          }
        }
      }

      // Still not valid - remove the image data
      if(!it->latLon.IsValid())
      {
        printf("ERROR: Couldn't find sibling image with GPS data for: %s\n", it->fileName);
        it = vec.erase(it);
      }
      else
      {
        ++it;
      }

    } // Image Data loop
  } // Days loop
}

const double PATH_VERTEX_MERGE_DISTANCE = 0.0001;
const double NODE_MERGE_DISTANCE = 0.0002;

void GenerateDays(std::vector<Day> &vecDays, const ImageDataMap& mapImageData, FileInfoSet &setProcessed)
{
  // Construct array of dates
  std::vector<std::string> vecDates;
  for(ImageDataMap::const_iterator it = mapImageData.begin(); it != mapImageData.end(); ++it)
  {
    vecDates.push_back(it->first);
  }
  initialVertices = goodImageCount;

  // Sort the array of days
  std::sort(vecDates.begin(), vecDates.end());

  FileInfo fiTemp;

  // Fill up the days
  vecDays.resize(vecDates.size());
  for(size_t dayIndex = 0; dayIndex < vecDays.size(); ++dayIndex)
  {
    Day& day = vecDays[dayIndex];

    day.date = vecDates[dayIndex];

    ImageDataMap::const_iterator itDayImagesData = mapImageData.find(day.date);

    // Generate the path with optimization (merge close vertices)
    LatLon prevLatLon = { 400.0, 400.0 };
    for(ImageDataVec::const_iterator it = itDayImagesData->second.begin(); it != itDayImagesData->second.end(); ++it)
    {
      const double dist = prevLatLon.Distance(it->latLon);
      if(dist <= PATH_VERTEX_MERGE_DISTANCE)
      {
        //printf("%s: Skipping latLon: {%f, %f}, dist: %f\n", it->fileName, it->latLon.lat, it->latLon.lon, dist);
        pathVerticesSkipped++;
        continue;
      }
      day.path.push_back(it->latLon);
      prevLatLon = it->latLon;
      pathVerticesUsed++;
    }

    // Generate nodes with optimization (merge close images)
    prevLatLon.lat = prevLatLon.lon = 400.0;
    for(ImageDataVec::const_iterator it = itDayImagesData->second.begin(); it != itDayImagesData->second.end(); ++it)
    {
      // Apply Exceptions
      bool isException = false;
      for(int i = 0; i < sizeof(imageExceptions)/sizeof(imageExceptions[0]); ++i)
      {
        if(strcmp(imageExceptions[i], it->fileName) == 0)
        {
          isException = true;
          break;
        }
      }
      if(isException)
        continue;

      if(!editMode)
      {
        // If not in revised images list then skip it (leave in edit mode)
        fiTemp.name = it->fileName;
        FileInfoSet::iterator itProcessedFile = setProcessed.find(fiTemp);
        if(itProcessedFile == setProcessed.end())
        {
          //printf("%s: Skipping node: {%f, %f}\n", it->fileName, it->latLon.lat, it->latLon.lon);
          pathNodesSkipped++;
          continue;
        }
        setProcessed.erase(itProcessedFile);
      }

      // If too close and not in edit mode then merge with the previous
      const double dist = prevLatLon.Distance(it->latLon);
      if(editMode || (dist > NODE_MERGE_DISTANCE))
      {
        day.nodes.push_back(Node());
        day.nodes.back().latLon = it->latLon;
        prevLatLon = it->latLon;
        pathNodesFinal++;
      }
      else
      {
        pathNodesMerged++;
      }

      day.nodes.back().images.push_back(Image(it->fileName, it->dateTime));
      imagesInNodes++;
    }
  }
}