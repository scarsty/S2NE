#pragma once
#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <span>
#include <stdexcept>
#include <vector>

namespace s2ne {
class OriginalEvents {
    std::vector<uint8_t> bin_, pal_, kinds_;
    size_t events_{};
    static std::vector<uint8_t> read(const std::filesystem::path& path) {
        std::ifstream file(path, std::ios::binary);
        if (!file) throw std::runtime_error("Missing original event resource");
        return {std::istreambuf_iterator<char>(file), {}};
    }
    unsigned word(size_t p) const { return bin_.at(p) | (unsigned(bin_.at(p+1)) << 8); }
    unsigned dword(size_t p) const { return word(p) | (word(p+2) << 16); }
    size_t entry(unsigned id) const {
        size_t p = 848 + (id-1)*8;
        unsigned value = 0;
        for (unsigned i=0;i<4;++i) value |= unsigned(pal_.at(p+i)) << (8*i);
        return value - 11 - (id-1);
    }
    size_t record(size_t event) const {
        if (event >= event_count()) throw std::runtime_error("Invalid original event index");
        return events_ + 4 + event*20;
    }
public:
    struct Line { int speaker; std::vector<uint8_t> name, text; };
    unsigned stage{}, first_stage{}, alternate_stage{};
    struct Unit {
        unsigned id, appearance; int x,y; std::vector<uint8_t> name; int first_turn{},last_turn{};
        int hp{},max_hp{},mp{},st{},sp{},strikeback{},attack{},defense{},fencing{},tactics{},behavior{};
        int action_points{},base_action_points{},carry_points{},movement_type{};
        // sub_4274B5 partitions stage units at the first behavior 2/3/8; behavior 1 is allied AI.
        bool hostile()const{return behavior==2||behavior==3||behavior==8||behavior==9||behavior==10;}
        bool controllable()const{return behavior==0;}
        bool present(unsigned turn=1)const{return hp>0&&first_turn<=int(turn)&&(last_turn==0||last_turn>=int(turn));}
    };
    unsigned map_id{};
    std::vector<Unit> units;
    std::vector<uint8_t> briefing,stage_name,victory_objective,defeat_objective;
    std::array<int,12> battle_config{};
    unsigned difficulty{};
    std::array<uint8_t,80> flags{};
    explicit OriginalEvents(const std::filesystem::path& root, unsigned level)
        : bin_(read(root/"SUPERSP.BIN")), pal_(read(root/"SUPERSP.PAL")), difficulty(level) {
        if (level < 1 || level > 5) throw std::runtime_error("Invalid new-game difficulty");
        // sub_428658: four count-byte tables, followed by two length-prefixed blocks.
        size_t config = entry(10);
        config += 2 + 4*word(config);
        config += 2 + word(config);
        config += 2;
        kinds_=read(root/"KINDSP.BIN");
        for(size_t i=0;i<battle_config.size();++i)battle_config[i]=int(int8_t(bin_.at(config+0xE8+i)));
        stage = bin_.at(config+0x100);
        first_stage = bin_.at(config+0xFE);
        alternate_stage = bin_.at(config+0xFF);
        load_stage(stage);
    }
    void load_stage(unsigned id) {
        size_t table = entry(1);
        if (!id || id > word(table)) throw std::runtime_error("Invalid original stage ID");
        size_t header = table + 2 + (id-1)*30;
        size_t data = table + dword(header);
        size_t end = data + dword(header+4);
        if (end > bin_.size() || end < data) throw std::runtime_error("Truncated original stage");
        map_id=bin_.at(header+9);
        units.clear();
        size_t characters=data+dword(header+26);
        for(unsigned i=0;i<bin_.at(header+8);++i){
            size_t p=characters+i*101;
            Unit unit{word(p),word(p+16),int(int16_t(word(p+23))),int(int16_t(word(p+25))),{}};
            // sub_4443B3 expands these signed words into runtime +124h..+136h.
            auto stat=[&](size_t offset){return int(int16_t(word(p+offset)));};
            unit.hp=unit.max_hp=stat(27);unit.mp=stat(29);unit.st=stat(31);unit.sp=stat(33);
            // Combat reads packed dwords with SAR 16: runtime +134/+136/+138.
                        unit.strikeback=stat(35);unit.attack=stat(37);unit.defense=stat(39);unit.fencing=stat(41);unit.tactics=stat(43);
            auto kw=[&](size_t q){return unsigned(kinds_.at(q))|(unsigned(kinds_.at(q+1))<<8);};
            size_t kind=kw(18)+(size_t(kw(20))<<16)+(unit.appearance-1)*37;
            unit.movement_type=int(int8_t(kinds_.at(kind+18)));
            unit.base_action_points=stat(47)>0?stat(47):int(int8_t(kinds_.at(kind+21)));
            unit.action_points=unit.base_action_points;
            unit.behavior=int(int8_t(bin_.at(p+49)));
            unit.first_turn=int(int8_t(bin_.at(p+51)));unit.last_turn=int(int8_t(bin_.at(p+52)));
            for(size_t j=p+2;j<p+16&&bin_.at(j);++j)unit.name.push_back(bin_.at(j));
            units.push_back(std::move(unit));
        }
        size_t sections = characters + bin_.at(header+8)*101;
        auto section_text=[&](size_t p,unsigned n){std::vector<uint8_t> result;for(unsigned i=0;i<n&&bin_.at(p+i);++i)result.push_back(bin_.at(p+i));return result;};
        stage_name=section_text(sections+14,word(sections));
        size_t victory=sections+14+word(sections)+word(sections+2);
        victory_objective=section_text(victory,word(sections+4));
        size_t defeat=victory+word(sections+4)+2*(word(sections+8)+word(sections+10));
        defeat_objective=section_text(defeat,word(sections+6));
        briefing.clear();
        size_t text=sections+14+word(sections);
        for(size_t i=0;i<word(sections+2)&&bin_.at(text+i);++i)briefing.push_back(bin_.at(text+i));
        events_ = sections + 14;
        for (unsigned i : {0,2,4,6}) events_ += word(sections+i);
        for (unsigned i : {8,10,12}) events_ += 2*word(sections+i);
        if (events_+4 > end || events_+4+event_count()*20 > end || events_+4+word(events_+2) > end)
            throw std::runtime_error("Invalid original stage event table");
        stage = id;
    }
    size_t event_count() const { return word(events_); }
    unsigned type(size_t event) const { return bin_.at(record(event)); }
    unsigned turn(size_t event) const { return word(record(event)+3); }
    unsigned flag(size_t event) const { return word(record(event)+3); }
    unsigned condition(size_t event) const { return word(record(event)+1); }
    bool flag_trigger(size_t event)const{unsigned f=flag(event);return type(event)==5&&f>=1&&f<=80&&flags[f-1]!=0;}
    void choose(size_t event,unsigned selected){
        if(!flag_trigger(event)||condition(event)!=0||selected<1||selected>=line_count(event))throw std::runtime_error("Invalid original event choice");
        flags[flag(event)-1]=uint8_t(selected);
        complete(event);
    }
    bool branch_trigger(size_t event)const{unsigned f=flag(event);return type(event)==5&&condition(event)>=1&&condition(event)<=4&&f>=1&&f<=80&&flags[f-1]==condition(event);}
    void consume_branch(size_t event){flags.at(flag(event)-1)=0;}
    bool flag_dialogue_trigger(size_t event)const{unsigned f=flag(event);return type(event)==9&&condition(event)==0&&f>=1&&f<=80&&flags[f-1]>0;}
    unsigned line_count(size_t event) const { return word(record(event)+18); }
    bool turn_trigger(size_t event, unsigned current_turn) const {
        size_t p = record(event);
        return type(event)==1 && word(p+1)==0 && turn(event)==current_turn;
    }
    Line line(size_t event, size_t index) const {
        if (index >= line_count(event)) throw std::runtime_error("Invalid original dialogue line");
        size_t start = events_ + dword(record(event)+12);
        size_t p = start + index*23;
        Line result{int(int16_t(word(p))), {}, {}};
        for (size_t i=p+6;i<p+19 && bin_.at(i);++i) result.name.push_back(bin_.at(i));
        size_t text = start + line_count(event)*23 + word(p+19);
        size_t length = word(p+21);
        if (length==0 || bin_.at(text+length-1)!=0 || text+length>events_+4+word(events_+2))
            throw std::runtime_error("Invalid original dialogue text range");
        result.text.assign(bin_.begin()+text, bin_.begin()+text+length-1);
        return result;
    }
    void complete(size_t event) {
        size_t p=record(event);
        bin_.at(p)=0;
        unsigned flag=bin_.at(p+11);
        if (flag>=1 && flag<=80) flags[flag-1]=1;
    }
};
}
