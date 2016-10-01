#include "stdafx.h"
#include "JSGen.h"
#include "Stats.h"
#include <string>

void FillPlaceHolder(const char* srcFile, const char* tgtFile, const char* placeHolder, const std::string& str)
{
  FILE* f = nullptr;

  char * buf = new char[4000000 + 10];
  size_t read = 0;
  fopen_s(&f, srcFile, "rt");
  if(f != nullptr)
  {
    read = fread_s(buf, 4000000, 1, 4000000, f);
    buf[read] = '\0';
    fclose(f);
  }
  char *ph = strstr(buf, placeHolder);
  char *afterPlaceholder = ph + strlen(placeHolder);
  char *end = buf + read;
  
  fopen_s(&f, tgtFile, "wt");
  if(f != nullptr)
  {
    fwrite(buf, sizeof(char), ph - buf, f);
    fwrite(str.c_str(), sizeof(char), str.size(), f);
    fwrite(afterPlaceholder, sizeof(char), end - afterPlaceholder, f);
    fclose(f);
  }

  delete buf;
}

void WriteDays(const std::vector<Day>& vecDays)
{
  std::string strDays = "var days = [\n";

  size_t pathVerticesSkipped = 0;
  size_t pathVerticesUsed = 0;

  char buf[4096];
  for(size_t dayIndex = 0; dayIndex < vecDays.size(); ++dayIndex)
  {
    const Day& day = vecDays[dayIndex];

    if(day.nodes.empty() && day.path.empty())
    {
      continue;
    }

    // ================= Fill one day
    strDays += "  {\n";
    strDays += "    date: '"; strDays += day.date.c_str(); strDays += "',\n";
    strDays += "    line:null,\n";
    strDays += "    showNodes:true,\n";
    strDays += "    showPath:true,\n";
    // ================= Day's path
    strDays += "    path: [";
    for(auto itVertex = day.path.begin(); itVertex != day.path.end(); ++itVertex)
    {
      sprintf_s(buf, sizeof(buf), "  {lat: %f, lng: %f},\n", itVertex->lat, itVertex->lon);
      strDays += buf;
    }
    strDays += "    ],\n";

    // ================= Day's nodes
    strDays += "    nodes: [\n";
    for(auto itNode = day.nodes.begin(); itNode != day.nodes.end(); ++itNode)
    {
      sprintf_s(buf, sizeof(buf), "      { geo: {lat: %f, lng: %f},\n", itNode->latLon.lat, itNode->latLon.lon);
      strDays += buf;
      strDays += "        marker:null, alt:'-999', addr:'', places:[],\n";
      strDays += "        images: [\n";
      for(auto itImage = itNode->images.begin(); itImage != itNode->images.end(); ++itImage)
      {
        sprintf_s(buf, sizeof(buf), "          {name:'%s', dt:'%s', link:'', id:''},\n",
          itImage->fileName.c_str(), itImage->dateTime.c_str(), itImage->fileName.c_str());
        strDays += buf;
      }
      strDays += "      ]},\n";
    }
    strDays += "    ],\n";
    strDays += "  },\n";
  }

  strDays += "];";

  const char* src = "C:/Projects/GoogleMapTrip/Data/CanadaTrip.html";
  const char* final = "C:/Projects/GoogleMapTrip/Data/CanadaTripFinal.html";
  if(editMode)
  {
    FillPlaceHolder(src, final, "var editMode = false;", "var editMode = true;");
    src = final;
  }
#ifdef _DEBUG
  FillPlaceHolder(src, final, "var createDebugSpan = false;", "var createDebugSpan = true;");
  src = final;
#endif

  FillPlaceHolder(src, final, "var days;", strDays);
}