#pragma once

#include <string>

class LockFile
{
  public:
    LockFile(const std::string& tty);
    ~LockFile();

    inline bool created() const { return m_created; }

    bool present() const;
    void create();

  private:
    bool m_created;
    std::string m_name;
};
