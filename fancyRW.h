
#ifndef fancyRW_h
#define fancyRW_h

#include<iostream>
#include <errno.h>
#include <unistd.h>
template<typename T>
int READ(int fd, T* obj_ptr, int count)
{
  char* addr=(char*)obj_ptr;
  //loop. Read repeatedly until count bytes are read in
  int bytesRead = 0;
  int totalBytesRead = 0;
  while (count > 0)
  {
	bytesRead = read(fd, addr + totalBytesRead, count);
    //std::cout << "bytes read = " << bytesRead << std::endl;
    if (bytesRead == -1)
    {
      if (errno != EINTR)
      {
        return -1;
      }
    }
	else
	{
		totalBytesRead += bytesRead;
		count -= totalBytesRead;
	}
  }
  return totalBytesRead;
}

template<typename T>
int WRITE(int fd, T* obj_ptr, int count)
{
  char* addr=(char*)obj_ptr;
  //loop. Write repeatedly until count bytes are written out
  int bytesWritten = 0;
  int totalBytesWritten = 0;
  while (count > 0)
  {
    //std::cout << "bytes written = " << bytesWritten << std::endl;
	bytesWritten = write(fd, addr + totalBytesWritten, count);
    if (bytesWritten == -1)
    {
      if (errno != EINTR)
      {
        return -1;
      }
    }
	else
	{
		totalBytesWritten += bytesWritten;
		count -= totalBytesWritten;
	}
  }
  return totalBytesWritten;
}
#endif

//Example of a method for testing your functions shown here:
//opened/connected/etc to a socket with fd=7
//
/*int count=write(7, "Todd Gibson", 11);
cout << count << endl;//will display a number between 1 and 11
int count=write(7, "d Gibson", 8);*/
//
//How to test your READ template. Read will be: READ(7, ptr, 11);
/*write(7, "To", 2);
write(7, "DD ", 3);
write(7, "Gibso", 5);
write(7, "n", 1);*/
//
//
