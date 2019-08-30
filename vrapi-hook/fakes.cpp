#include "fakes.h"


std::set<void *> Fake::fakePtrs;
Fake::Fake()
{
    trackFake(this);
}

Fake::~Fake()
{
    untrackFake(this);
}

void Fake::trackFake(void* handle)
{
    Fake::fakePtrs.insert(handle);
}

void Fake::untrackFake(void* handle)
{
    Fake::fakePtrs.erase(handle);
}

bool Fake::IsFake(void* handle)
{
    return Fake::fakePtrs.find(handle) != fakePtrs.end();
}

FakeLeaderboardEntryArrayMessage::FakeLeaderboardEntryArrayMessage()
{
    MessageType = ovrMessageType::ovrMessage_Leaderboard_GetEntriesAfterRank;
}
