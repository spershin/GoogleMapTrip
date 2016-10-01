// GoogleMapTrip.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "GoogleMapTrip.h"
#include "FindFile.h"
#include "ExifUtils.h"
#include "JSGen.h"
#include "DataTypes.h"
#include "Stats.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

void ProcessFolder(const char* path, const char* pathProcessed)
{
  // Get all files from the unprocessed folder
  FileInfoSet set;
  wndSearchFolderRecurse(path, set);

  // Get all files from the processed folder (where unwanted images are removed)
  FileInfoSet setProcessed;
  if(pathProcessed != nullptr)
  {
    wndSearchFolderRecurse(pathProcessed, setProcessed);
  }
  selectedImageCount = setProcessed.size();

  // Collect image data (name, GPS, date/time, etc.).
  // Here some filtering is done and bad images are culled
  ImageDataMap mapImageData;
  for(FileInfoSet::iterator it = set.begin(); it != set.end(); ++it)
  {
    ProcessImageFile(*it, mapImageData);
  }

  FixInvalidGeoCoords(mapImageData);
  
  std::vector<Day> vecDays;
  GenerateDays(vecDays, mapImageData, setProcessed);

  printf("%u images are found in unprocessed folder\n", set.size());
  printf("%u images are found in processed folder\n", selectedImageCount);
  printf("%u images are good\n", goodImageCount);
  printf("%u images are bad\n", set.size() - goodImageCount);
  printf("Initial paths' vertices: %u\n", initialVertices);
  printf("Skipped paths' vertices: %u\n", pathVerticesSkipped);
  printf("Used paths' vertices: %u\n", pathVerticesUsed);
  printf("Initial nodes: %u\n", goodImageCount);
  printf("Merged nodes: %u\n", pathNodesMerged);
  printf("Skipped nodes: %u\n", pathNodesSkipped);
  printf("Used nodes: %u\n", pathNodesFinal);
  printf("Images available in nodes: %u\n", imagesInNodes);

  // Print files from the processed set which didn't end up on the map!
  printf("Images didn't get to the map: %u\n", setProcessed.size());
  for(FileInfoSet::const_iterator it = setProcessed.cbegin(); it != setProcessed.cend(); ++it)
  {
    printf("\t%s:\n", it->name.c_str());
  }

  WriteDays(vecDays);
}

int PrintHelp()
{
  printf("Google Map Trip tool, copyright, mad god.\n");
  printf("\t-h\tshow this help.\n");
  printf("\t-e\tedit mode (nodes are draggable and aren't merged).\n");
  printf("\t-i <initialFolder>\tInitial folder - where all photos will be opened and GPS info taken.\n");
  printf("\t-r <revisedFolder>\tRevised folder - an image won't be in a node if it is not in this folder. The image will still be used to build the path.\n");
  return 0;
}

int main(int argc, char* argv[])
{
  if(argc == 1)
    return PrintHelp();

  char* initialFolder = nullptr;
  char* revisedFolder = nullptr;

  for(int argIndex = 1; argIndex < argc; ++argIndex)
  {
    if(strcmp(argv[argIndex], "-h") == 0)
    {
      return PrintHelp();
    }

    if(strcmp(argv[argIndex], "-e") == 0)
    {
      editMode = true;
      continue;
    }

    if(strcmp(argv[argIndex], "-i") == 0)
    {
      ++argIndex;
      if(argIndex >= argc)
      {
        return PrintHelp();
      }
      initialFolder = argv[argIndex];
    }

    if(strcmp(argv[argIndex], "-r") == 0)
    {
      ++argIndex;
      if(argIndex >= argc)
      {
        return PrintHelp();
      }
      revisedFolder = argv[argIndex];
    }
  }

  //const char* path = "C:/Projects/Photos/2016+ USA/2016.08.27-09.11 Canada Trip/";
  //const char* path = "C:/Projects/Photos-NEW3/Canada All/";
  //const char* pathProcessed = "C:/Projects/Photos/2016+ USA/2016.08.27-09.11 Canada Trip/";
  ProcessFolder(initialFolder, revisedFolder);

	return 0;
}
