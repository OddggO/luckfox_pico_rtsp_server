#include "MediaSessionManager.h"

MediaSessionManager* MediaSessionManager::createNew()
{
    return new MediaSessionManager();
}

MediaSessionManager::MediaSessionManager()
{
    mSessMap.clear();
}

MediaSessionManager::~MediaSessionManager()
{
    // for(auto it = mSessMap.begin(); it != mSessMap.end(); it++)
    // {
    //     if (it->second != nullptr) {
    //         delete it->second;
    //         it->second = nullptr;
    //     }
    // }
    // mSessMap.clear();
}

bool MediaSessionManager::addSession(MediaSession* session)
{
    if (!session) {
        return false;
    }
    auto it = mSessMap.find(session->name());
    if (it != mSessMap.end()) {
        return false;
    }
    mSessMap.insert(std::make_pair(session->name(), session));
    return true;
}

bool MediaSessionManager::removeSession(MediaSession* session)
{
    if (!session) {
        return false;
    }
    auto it = mSessMap.find(session->name());
    if (it == mSessMap.end()) {
        return false;
    }
    mSessMap.erase(it);
    return true;
}

MediaSession* MediaSessionManager::getSession(const std::string& name)
{
    auto it = mSessMap.find(name);
    if (it == mSessMap.end()) {
        return nullptr;
    }
    return it->second;
}

