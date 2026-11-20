#include "Midi/MidiLearnRouter.h"
#include <atomic>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <new>
#include <stdexcept>
namespace
{
std::atomic<int> allocs {0};
thread_local bool watch = false;
}
void* operator new(std::size_t n)
{
    if (watch) ++allocs;
    if (void* p = std::malloc(n ? n : 1)) return p;
    throw std::bad_alloc();
}
void operator delete(void* p) noexcept { std::free(p); }
void operator delete(void* p, std::size_t) noexcept { std::free(p); }
void* operator new[](std::size_t n)
{
    if (watch) ++allocs;
    if (void* p = std::malloc(n ? n : 1)) return p;
    throw std::bad_alloc();
}
void operator delete[](void* p) noexcept { std::free(p); }
void operator delete[](void* p, std::size_t) noexcept { std::free(p); }
void need(bool yes,const char* why) { if(!yes) throw std::runtime_error(why); }

int main()
{
 try
 {
    biosphere::midi::LearnRouter router;
    need(router.assignment(0) == -1,"Default MIDI assignment must be empty");
    router.arm(3);
    need(router.learnTarget() == 3,"Arm target lost");
    watch = true;
    router.receive(2, 74, 105);
    watch = false;
    need(allocs.load()==0,"MIDI audio callback allocated");
    need(router.learnTarget()==-1,"Learning must end after one CC");
    need(router.assignment(3) == biosphere::midi::LearnRouter::encode(2,74),
         "CC/channel not learned");
    need(router.consume(3) == 105 && router.consume(3)==-1,"CC not queued once");
    auto notification=router.consumeLearnNotification();
    need(biosphere::midi::LearnRouter::learnedIndex(notification)==3
         && biosphere::midi::LearnRouter::learnedBinding(notification)==
         biosphere::midi::LearnRouter::encode(2,74),"Last learned status missing");
    need(router.consumeLearnNotification()==-1,"Learn status not cleared");

    router.receive(1,74,22);
    need(router.consume(3)==-1,"Wrong MIDI channel must be ignored");
    router.receive(2,74,0);
    router.receive(2,74,127);
    need(router.consume(3)==127,"Last CC of burst should win");

    router.assign(4,16,127);
    router.receive(16,127,64);
    need(router.consume(4)==64,"Channel 16 / CC127 not supported");
    router.clear(3);
    router.receive(2,74,11);
    need(router.consume(3)==-1,"Clear individual assignment failed");

    router.arm(2);
    router.cancel();
    router.receive(2,45,90);
    need(router.assignment(2)==-1,"Cancelled learn still mapped a CC");
    router.arm(2);
    router.receive(0,45,90);
    need(router.learnTarget()==2,"Invalid channel must not complete learning");
    router.receive(1,128,90);
    need(router.learnTarget()==2,"Invalid CC must not complete learning");
    router.receive(1,7,128);
    need(router.learnTarget()==2,"Invalid value must not complete learning");
    router.receive(1,7,100);
    need(router.assignment(2)==biosphere::midi::LearnRouter::encode(1,7),
         "Learn recovery failed");
    router.clearAll();
    for(int i=0;i<128;++i)need(router.assignment(i)==-1,"Clear all failed");
    need(router.learnTarget()==-1,"Learning not cancelled on clear all");
    need(router.consume(2)==-1,"CC event not cleared on clear all");

    router.assign(127,3,11);
    router.receive(3,11,42);
    need(router.consume(127)==42,"Highest parameter index should be reachable");
    router.assign(128,1,30);
    need(router.assignment(128)==-1,"Out-of-bounds parameter access");
    std::cout << "MIDI LEARN PASS: CC 0..127, 16 channels, learn/cancel/clear, "
                 "normalized values, no heap allocations in audio callback\n";
 }
 catch(const std::exception& e)
 {
    watch=false;
    std::cerr<<"MIDI LEARN FAIL: "<<e.what()<<'\n';
    return 1;
 }
 return 0;
}
