#pragma once

#include <set>
#include <string>

struct FileInfo
{
  unsigned __int64  size;
  std::string       name;
  std::string       path;

  FileInfo(__int64 size, const std::string& name);
  FileInfo() : size(0) {}
};

struct FIComparator
{
  bool operator()(const FileInfo& _Left, const FileInfo& _Right) const;
};

typedef std::set<FileInfo, FIComparator> FileInfoSet;

void wndSearchFolderRecurse(const std::string& path, FileInfoSet &s);
