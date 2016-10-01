#include "stdafx.h"
#include "FindFile.h"
#include <direct.h>
#include <windows.h>
#include <algorithm>

FileInfo::FileInfo(__int64 sz, const std::string& n) : size(sz), name(n)
{
  std::transform(name.begin(), name.end(), name.begin(), ::tolower);
}

bool FIComparator::operator()(const FileInfo& _Left, const FileInfo& _Right) const
{	// apply operator< to operands
  //if(_Left.name == _Right.name)
  //  return _Left.size < _Right.size;
  return _Left.name < _Right.name;
}

std::string wndGetCurrentDir()
{
	char szCurDir[MAX_PATH + 1];
	GetCurrentDirectory(MAX_PATH, szCurDir);
	return szCurDir;
}

void wndSearchFolderRecurse(const std::string& path, FileInfoSet &s)
{
	// Store current directory to return there later
	std::string strCurDir = wndGetCurrentDir();

	// Jump to given folder
  if(_chdir(path.c_str()) == -1)
		return;

	WIN32_FIND_DATA fd;
	HANDLE hFindFile = FindFirstFile("*.*", &fd);
	if(hFindFile != INVALID_HANDLE_VALUE)
	{
		do
		{
			if(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				// skip "." & ".." directories
				if(!strcmp(fd.cFileName, ".") || !strcmp(fd.cFileName, ".."))
					continue;

				std::string strNewFolder(path); strNewFolder += fd.cFileName; strNewFolder += "\\";
				wndSearchFolderRecurse(strNewFolder, s);
				continue;
			}

      const DWORD dwTemp = fd.nFileSizeHigh;
      fd.nFileSizeHigh = fd.nFileSizeLow;
      fd.nFileSizeLow = dwTemp;
      FileInfo fi(*(unsigned __int64*)&fd.nFileSizeHigh, fd.cFileName);

      if(fi.name == "thumbs.db")
      {
        continue;
      }

      fi.path = path; fi.path += fd.cFileName;

      FileInfoSet::const_iterator it = s.find(fi);
      if(it != s.end())
      {
        void *d = malloc((size_t)fi.size);
        void *d2 = malloc((size_t)fi.size);
        FILE *f; fopen_s(&f, fi.path.c_str(), "rb");
        fread(d, (size_t)fi.size, 1, f);
        fclose(f);
        FILE *f2; fopen_s(&f2, it->path.c_str(), "rb");
        fread(d2, (size_t)fi.size, 1, f2);
        fclose(f2);
        if(memcmp(d, d2, (size_t)fi.size) != 0)
        {
          if(fi.name == "thumbs.db")
          {
            free(d);
            free(d2);
            continue;
          }
          _ASSERTE(false);
        }

        free(d);
        free(d2);
        continue;
      }


		  s.insert(fi);
		}
		while(FindNextFile(hFindFile, &fd));
	}

	FindClose(hFindFile);
  _chdir(strCurDir.c_str());
}
