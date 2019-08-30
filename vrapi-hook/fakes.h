#include "main.h"
#include <vector>
#include <set>


class Fake
{
    public:
        Fake();
        virtual ~Fake();
        static bool IsFake(void *);
    protected:
        void trackFake(void *);
        void untrackFake(void *);
    private:
        static std::set<void *> fakePtrs;
};

class FakeMessage : Fake
{
    public:
        ovrMessageType MessageType;
        bool IsError = false;
        bool IsReady = false;
};

class FakeUser : Fake
{
    public: 
        const char *OculusID;
        const char *Presence;
        uint64_t UserID;
        const char *ImageUrl;
        const char *InviteToken;
        const char *SmallImageUrl;
        ovrUserPresenceStatus PresenceStatus;
};

class FakeLeaderboardEntry : Fake
{
    public:
        long long Score;
        int Rank;
        unsigned long long Timestamp;
        FakeUser User;
};

class FakeLeaderboardEntryArrayMessage : FakeMessage
{
    public:
        FakeLeaderboardEntryArrayMessage();
        std::vector<FakeLeaderboardEntry> Entries;    
};


