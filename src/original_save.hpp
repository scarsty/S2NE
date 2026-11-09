#pragma once
#include <algorithm>
#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>

namespace s2ne {
// Raw on-disk state, not an OriginalEvents::Unit snapshot. See save-evidence.txt.
class OriginalSave {
public:
    static constexpr unsigned save_address=0x4362B4, load_address=0x4367CB;
    static constexpr size_t unit_size=0x16A, maximum_units=50, fixed_size=646;
    static constexpr std::array<uint8_t,9> signature{0xA5,0x5F,0xA4,0xE8,0xB1,0x4B,0xA8,0xCF,0};
    using UnitRecord=std::array<uint8_t,unit_size>;
    struct Group {
        std::vector<UnitRecord> units;
        // Runtime +CC (4), +DE (4), +D6 (4), +D4 (2), +E2 (4), in that order.
        std::array<uint8_t,18> metadata{};
        std::vector<uint8_t> inventory; // ITEMSP.BIN's signed 16-bit entry count, not its file size.
        std::array<uint8_t,33> status{}; // runtime +286
    };
    struct State {
        std::array<uint8_t,50> description{};
        std::array<uint8_t,255> attribution{}; // Preserve original stack-buffer padding verbatim.
        std::array<uint8_t,80> flags_4f7d10{};
        std::array<uint8_t,31> settings_4f7cf0{};
        std::array<uint8_t,29> globals_4f7cd0{};
        std::array<uint8_t,4> value_4f705c{};
        std::array<uint8_t,160> words_4f6fbc{};
        std::array<uint8_t,28> stage_4fb478{};
        std::array<Group,2> groups;
        std::vector<uint16_t> event_states;
        bool battle_context()const{return u32(globals_4f7cd0,4)==0;}
    };
private:
    static uint32_t u32(std::span<const uint8_t> b,size_t p){
        return uint32_t(b[p])|(uint32_t(b[p+1])<<8)|(uint32_t(b[p+2])<<16)|(uint32_t(b[p+3])<<24);
    }
    static void fail(){throw std::runtime_error("Invalid or unsupported original S2NE save");}
    static void check_items(size_t count){if(count>32767)fail();}
    class Reader {
        std::span<const uint8_t> bytes_;
        size_t position_=0;
    public:
        explicit Reader(std::span<const uint8_t> bytes):bytes_(bytes){}
        void take(std::span<uint8_t> dest){
            if(dest.size()>bytes_.size()-position_)fail();
            std::copy_n(bytes_.begin()+position_,dest.size(),dest.begin());position_+=dest.size();
        }
        uint32_t dword(){std::array<uint8_t,4> b{};take(b);return u32(b,0);}
        uint16_t word(){std::array<uint8_t,2> b{};take(b);return uint16_t(b[0]|(unsigned(b[1])<<8));}
        size_t remaining()const{return bytes_.size()-position_;}
    };
    static void append(std::vector<uint8_t>& out,std::span<const uint8_t> b){out.insert(out.end(),b.begin(),b.end());}
    static void number(std::vector<uint8_t>& out,uint32_t value,unsigned width){
        for(unsigned i=0;i<width;++i)out.push_back(uint8_t(value>>(i*8)));
    }
    static void validate(const State& s,size_t items){
        check_items(items);
        for(const auto& g:s.groups)if(g.units.size()>maximum_units||g.inventory.size()!=items)fail();
        if(s.event_states.size()>32767||(!s.battle_context()&&!s.event_states.empty()))fail();
    }
public:
    static size_t item_count(std::span<const uint8_t> itemsp){
        if(itemsp.size()<2)fail();
        size_t count=unsigned(itemsp[0])|(unsigned(itemsp[1])<<8);
        check_items(count);
        if(itemsp.size()<2+count*38)fail();
        return count;
    }
    // Validates framing only; unit pointer fixups/derived runtime fields are deliberately not applied.
    static State decode(std::span<const uint8_t> bytes,size_t items){
        check_items(items);Reader r(bytes);State s;
        std::array<uint8_t,9> magic{};r.take(magic);if(magic!=signature)fail();
        r.take(s.description);r.take(s.attribution);r.take(s.flags_4f7d10);r.take(s.settings_4f7cf0);
        r.take(s.globals_4f7cd0);r.take(s.value_4f705c);r.take(s.words_4f6fbc);r.take(s.stage_4fb478);
        for(auto& g:s.groups){
            auto count=r.dword();if(count>maximum_units)fail();
            if(size_t(count)*unit_size+18+items+33>r.remaining())fail();
            g.units.resize(count);for(auto& unit:g.units)r.take(unit);
            r.take(g.metadata);g.inventory.resize(items);r.take(g.inventory);r.take(g.status);
        }
        if(s.battle_context()){
            unsigned count=r.word();if(count>32767||size_t(count)*2!=r.remaining())fail();
            s.event_states.resize(count);for(auto& state:s.event_states)state=r.word();
        }
        if(r.remaining())fail();
        validate(s,items);return s;
    }
    static std::vector<uint8_t> encode(const State& s,size_t items){
        validate(s,items);std::vector<uint8_t> out;
        append(out,signature);append(out,s.description);append(out,s.attribution);append(out,s.flags_4f7d10);
        append(out,s.settings_4f7cf0);append(out,s.globals_4f7cd0);append(out,s.value_4f705c);
        append(out,s.words_4f6fbc);append(out,s.stage_4fb478);
        for(const auto& g:s.groups){
            number(out,uint32_t(g.units.size()),4);for(const auto& unit:g.units)append(out,unit);
            append(out,g.metadata);append(out,g.inventory);append(out,g.status);
        }
        if(s.battle_context()){
            number(out,uint32_t(s.event_states.size()),2);for(auto state:s.event_states)number(out,state,2);
        }
        return out;
    }
    static State load(const std::filesystem::path& path,size_t items){
        std::ifstream file(path,std::ios::binary);if(!file)throw std::runtime_error("Cannot open original save");
        std::vector<uint8_t> bytes{std::istreambuf_iterator<char>(file),{}};
        if(file.bad())throw std::runtime_error("Cannot read original save");
        return decode(bytes,items);
    }
    static std::filesystem::path slot_path(const std::filesystem::path& root,unsigned slot){
        if(slot<1||slot>53)fail();
        return root/"SAVE"/("UJ"+std::string(slot<10?"0":"")+std::to_string(slot)+".SAV");
    }
};
}
