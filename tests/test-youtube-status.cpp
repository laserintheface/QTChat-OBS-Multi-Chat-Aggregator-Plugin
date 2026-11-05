#include "youtube-status.h"
#include <cassert>
#include <string>
using YouTubeStatus::parse;
int main() {
    for (auto text : {"0", "0,0", "0, 0", " 0 ,\t0 ", "00,0"}) {
        auto p=parse(text,true); assert(p.valid && p.code==0 && p.error==-1);
    }
    for (int i=0;i<=16;++i) {
        const auto s=std::to_string(i);
        auto p=parse(s+", "+s,true); assert(p.valid && p.code==i);
        assert(p.error==(i?i:-1));
    }
    for (auto text : {"", " ", "0,", ",0", "0,,0", "+0", "-0", "17", "999999999999999999999999999999999", "0,garbage", "0,17", "0\r\n", "0;0"})
        assert(!parse(text,true).valid);
    assert(!parse("0",false).valid);
    auto conflict=parse("0,8",true);
    assert(!conflict.valid && conflict.conflict && conflict.error==8);
    auto invalid=parse("0,garbage,8",true);
    assert(!invalid.valid && invalid.error==8);
    assert(!parse(std::string(1025,'0'),true).valid);
    std::string many="0"; for(int i=0;i<64;++i) many+=",0";
    assert(!parse(many,true).valid);
}
