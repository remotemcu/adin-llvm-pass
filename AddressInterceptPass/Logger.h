#ifndef LOGGER_H
#define LOGGER_H

#include <string>

#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"


namespace adin {

enum LevelDebug { _ERROR = 0, _WARNING, _INFO, _DEBUG, _ALL_LOG };

class Log {
    static LevelDebug gLevel;

    LevelDebug cLevel;

public:
    Log(const LevelDebug level, const std::string &fileName,
        const std::string &funcName, const int line) {
        cLevel = level;
        if (cLevel <= gLevel)
            llvm::dbgs() << fileName << ": " << funcName << ": " << line << "> ";
    }

    template <class T> Log &operator<<(const T &v) {
        if (cLevel <= gLevel)
            llvm::dbgs() << v;
        return *this;
    }

    ~Log() {
        if (cLevel <= gLevel)
            llvm::dbgs() << "\n";
    }

    static void setGLevel(const LevelDebug level) { gLevel = level; }

    static int loggerf (const LevelDebug level, const std::string &fileName,
                 const std::string &funcName, const int line,
                 const char *__restrict __format, ...);
};


#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

#define ADIN_LOG(LEVEL) Log((LEVEL), __FILENAME__, __FUNCTION__, __LINE__)

#define ADIN_PRINTF(LEVEL,F__,...) Log::loggerf((LEVEL), __FILENAME__, __FUNCTION__, __LINE__, F__, __VA_ARGS__)


} //namespace

#endif // LOGGER_H
