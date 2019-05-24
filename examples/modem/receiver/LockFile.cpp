#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include "LockFile.hpp"

LockFile::LockFile(const std::string& tty):
  m_created(false),
  m_name("/var/lock/LCK..")
{
  std::string::size_type pos = tty.rfind('/');
  m_name.append((pos == std::string::npos) ? tty : tty.substr(pos + 1));
}

LockFile::~LockFile()
{
  if (m_created)
  {
    unlink(m_name.c_str());
    m_created = false;
  }
}

bool LockFile::present() const
{
  return access(m_name.c_str(), F_OK) == 0;
}

void LockFile::create()
{
  if (m_created) return;
  int fout = open(m_name.c_str(), O_CREAT | O_EXCL | O_WRONLY,
                  S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
  if (fout < 0) return;
  dprintf(fout, "%10d\n", getpid());
  close(fout);
  m_created = true;
}
