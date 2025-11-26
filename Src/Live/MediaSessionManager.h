#pragma once
#include "MediaSession.h"
#include <map>
#include <string>

class MediaSessionManager
{
public:
    static MediaSessionManager* createNew();
    MediaSessionManager();
    ~MediaSessionManager();
public:
    bool addSession(MediaSession* session);
    bool removeSession(MediaSession* session);
    MediaSession* getSession(const std::string& name);
private:
    std::map<std::string, MediaSession*> mSessMap;
};