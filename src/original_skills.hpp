#pragma once
#include "original_events.hpp"
#include "original_menu.hpp"
#include "original_combat.hpp"
#include "original_promotion_transaction.hpp"
#include <functional>
#include <optional>

namespace s2ne {
// 464C0C/464C83 -> 469F04. Selection requests are not execution results.
class OriginalSkills {
public:
    enum class Page { list, requested, closed };
    enum class Failure { none, mp, st, action_points, hp, partner_unresolved, world_unresolved, unsupported_execution, stale_unit };
    struct Record {
        unsigned id{},group{},bit{};
        std::array<uint8_t,37> raw{};
        int word(size_t offset)const{return int(int16_t(unsigned(raw.at(offset))|(unsigned(raw.at(offset+1))<<8)));}
        std::vector<uint8_t> name()const{std::vector<uint8_t> out;for(size_t i=2;i<13&&raw[i];++i)out.push_back(raw[i]);return out;}
        unsigned effect()const{return raw[19];}
    };
    struct Group { unsigned index{};std::vector<uint8_t> name;std::vector<Record> records; };
    struct Context {
        bool second{},suppress_first{},suppress_second{},army_da_nonzero{};
        bool status3d_40{}; // Original full runtime status unavailable in Unit.status3c byte.
        bool debug{};
    };
    struct Request {unsigned unit_id{},appearance{},skill_id{},group{},bit{};bool second{};};
    struct Result {bool applied{};Failure failure{Failure::unsupported_execution};int healing{},experience_award{},proficiency_award{};};
    struct ExecutionContext {
        unsigned difficulty{3};uint32_t seconds{};
        uint32_t status3c{}; // Full runtime +3C DWORD, not Unit.status3c's low byte.
        unsigned skill_payment_state{}; // Live 505C1C[skill-1], initially BIN10 table2.
        bool friendly_army{true},conditions_mode{};
    };
    // Apply proficiency/learning (group), then 458B03/459309 XP/level-up to
    // the staged unit/RNG only. Return false when the native bridge is absent.
    using AwardAdapter=std::function<bool(OriginalEvents::Unit&,original_combat::Random&,int,int,unsigned)>;
private:
    OriginalMenu font_;
    std::vector<uint8_t> bin_,pal_,cel_;
    std::vector<Group> groups_;
    size_t header_{};
    int ap_base_{},ap_repeat_{};
    std::array<int,256> config_{};
    unsigned owner_id_{},owner_appearance_{};
    Context context_;
    Page page_{Page::closed};
    size_t group_{},skill_{};
    std::optional<Request> request_;
    static std::vector<uint8_t> read(const std::filesystem::path& p){std::ifstream f(p,std::ios::binary);if(!f)throw std::runtime_error("Missing original skill resource");return {std::istreambuf_iterator<char>(f),{}};}
    static unsigned word(const auto& b,size_t p){return b.at(p)|(unsigned(b.at(p+1))<<8);}
    static unsigned dword(const auto& b,size_t p){return word(b,p)|(word(b,p+2)<<16);}
    size_t entry(unsigned id)const{return dword(pal_,848+(id-1)*8)-11-(id-1);}
    std::vector<uint8_t> string(size_t p)const{std::vector<uint8_t> out;while(bin_.at(p))out.push_back(bin_.at(p++));return out;}
    void sprite(std::vector<uint8_t>& image,unsigned index,int left,int top)const{
        size_t g=dword(pal_,816)-57,p=g+dword(cel_,g+2+index*8);unsigned w=word(cel_,p)+1,h=word(cel_,p+2)+1;p+=4;
        for(unsigned y=0;y<h;++y)for(unsigned x=0;x<w;){unsigned op=cel_.at(p++),n=op?op&127:1,c=0;if(!n||n>w-x)throw std::runtime_error("Invalid skill sprite");if(op&&!(op&128))c=cel_.at(p++);
            while(n--){unsigned ink=op&128?cel_.at(p++):c;int dx=left+int(x++),dy=top+int(y);if(!ink||dx<0||dx>=640||dy<0||dy>=480)continue;size_t dst=(size_t(dy)*640+dx)*4;for(unsigned k=0;k<3;++k)image.at(dst+k)=uint8_t(unsigned(pal_.at(ink*3+k))*255/63);image.at(dst+3)=255;}}
    }
public:
    explicit OriginalSkills(const std::filesystem::path& root,bool small_font=false):font_(root),bin_(read(root/"SUPERSP.BIN")),pal_(read(root/"SUPERSP.PAL")),cel_(read(root/"SUPERSP.CEL")){
        font_.small_font=small_font;header_=entry(2);size_t config=entry(10);config+=2+4*word(bin_,config);config+=2+word(bin_,config);config+=2;
        for(size_t i=0;i<config_.size();++i)config_[i]=int(int8_t(bin_.at(config+i)));
        ap_base_=config_[0xEF];ap_repeat_=config_[0xF3];
        if(bin_.at(header_+1)>11||bin_.at(header_+2)+bin_.at(header_+3)>11)throw std::runtime_error("Invalid original skill groups");
        if(config_[0x8F]<=0||config_[0x91]<=0||config_[13]+3<=0||config_[0xE3]<=0)throw std::runtime_error("Invalid original self-heal configuration");
    }
    int action_cost(const OriginalEvents::Unit& unit)const{return ap_base_+((unit.status34&12)?ap_repeat_:0);}
    bool entry_available(const OriginalEvents::Unit& unit,Context context)const{
        unsigned begin=context.second?bin_.at(header_+2):0,end=context.second?begin+bin_.at(header_+3):bin_.at(header_+2);
        if(context.debug||context.status3d_40||((unit.action_status_runtime?unit.action_status_runtime->at(0x3D):unit.status3d)&0x40)||(context.second?context.suppress_second:context.suppress_first)||unit.action_points<action_cost(unit))return false;
        for(unsigned g=begin;g<end;++g)if(unit.learned.at(g))return true;return false;
    }
    void open(const OriginalEvents::Unit& unit,Context context){
        context_=context;owner_id_=unit.id;owner_appearance_=unit.appearance;groups_.clear();request_.reset();group_=skill_=0;page_=Page::closed;
        if(!entry_available(unit,context))return;
        unsigned count=bin_.at(header_+1),begin=context.second?bin_.at(header_+2):0,end=context.second?begin+bin_.at(header_+3):bin_.at(header_+2);
        size_t records=header_+9+count*2,names=records+bin_.at(header_+7)*37,name_data=names+2*(count+bin_.at(header_+8));
        for(unsigned g=begin;g<end;++g){if(!unit.learned.at(g))continue;Group group{g,string(name_data+word(bin_,names+g*2)),{}};
            unsigned first=bin_.at(header_+9+g*2),length=bin_.at(header_+10+g*2);
            for(unsigned bit=0;bit<length;++bit){if(bit>=16||!(unit.learned[g]&(1u<<bit)))continue;Record r{first+bit+1,g,bit,{}};if(r.id>bin_.at(header_+7))throw std::runtime_error("Invalid original skill ID");for(size_t n=0;n<37;++n)r.raw[n]=bin_.at(records+(r.id-1)*37+n);group.records.push_back(r);}
            groups_.push_back(std::move(group));
        }
        if(!groups_.empty()&&!groups_[0].records.empty())page_=Page::list;
    }
    const std::vector<Group>& groups()const{return groups_;}
    Page page()const{return page_;}size_t selected_group()const{return group_;}size_t selected_skill()const{return skill_;}
    const Record& selected()const{return groups_.at(group_).records.at(skill_);}
    Failure resource_failure(const OriginalEvents::Unit& unit,const Record& record)const{
        int cost=record.word(13);unsigned flags=record.raw[33];
        if((flags&1)&&unit.mp<((unit.abilities&0x20)?cost/2:cost))return Failure::mp;
        if((flags&2)&&unit.st<cost)return Failure::st;
        if((flags&4)&&unit.hp<=cost)return Failure::hp;
        if(unit.action_points<action_cost(unit))return Failure::action_points;return Failure::none;
    }
    Failure select(const OriginalEvents::Unit& unit){
        if(page_!=Page::list)return Failure::world_unresolved;
        if(unit.id!=owner_id_||unit.appearance!=owner_appearance_||!(unit.learned.at(selected().group)&(1u<<selected().bit)))return Failure::stale_unit;
        if(!entry_available(unit,context_))return Failure::world_unresolved;
        if(auto failure=resource_failure(unit,selected());failure!=Failure::none)return failure;
        if(selected().raw[32]&7)return Failure::partner_unresolved;
        // 21..31 require 44CB0E world-state checks before leaving the list.
        if(selected().effect()>=21&&selected().effect()<=31)return Failure::world_unresolved;
        request_=Request{unit.id,unit.appearance,selected().id,selected().group,selected().bit,context_.second};page_=Page::requested;return Failure::none;
    }
    const std::optional<Request>& request()const{return request_;}
    unsigned initial_payment_state(unsigned id)const{
        size_t p=entry(10);if(!id||id>word(bin_,p))throw std::out_of_range("Skill payment state");
        return bin_.at(p+2+word(bin_,p)+id-1);
    }
    bool effect_supported(const Record& record)const{
        // Narrow to the installed self-heal, not every effect18/formula41 record.
        static constexpr std::array<uint8_t,24> tail{25,0,90,0,0,1,18,41,120,0,100,0,0,0,0,0,0,0,0,0,66,0,0,0};
        return record.id==88&&record.group==8&&record.bit==5&&std::equal(tail.begin(),tail.end(),record.raw.begin()+13);
    }
    bool executable_available(const OriginalEvents::Unit& unit,Context context,bool award_bridge_ready)const{
        return award_bridge_ready&&context.second&&entry_available(unit,context)&&((unit.learned[8]&(1u<<5))||(unit.learned[9]&1));
    }
    Result execute(OriginalEvents::Unit&)const{return {false,Failure::unsupported_execution};}
    Result execute(OriginalEvents::Unit& unit,const OriginalEvents::Unit& target,const OriginalEvents& events,
        original_combat::Random& random,ExecutionContext execution,const AwardAdapter& award){
        if(page_!=Page::requested||!request_||!effect_supported(selected())||!award)return {false,Failure::unsupported_execution};
        const auto request=*request_;
        if(unit.id!=request.unit_id||unit.appearance!=request.appearance||!(unit.learned.at(request.group)&(1u<<request.bit)))return {false,Failure::stale_unit};
        // Range0 origin + targetmode1; accept the actual actor reference only.
        if(&unit!=&target||unit.hp<=0||unit.max_hp<unit.hp||execution.difficulty<1||execution.difficulty>5||!execution.friendly_army||execution.conditions_mode||
            (execution.status3c&255)!=unit.status3c||!entry_available(unit,context_))return {false,Failure::world_unresolved};
        if(auto failure=resource_failure(unit,selected());failure!=Failure::none)return {false,failure};
        auto staged=unit;auto rng=random;const auto& record=selected();int amount=0;
        if(!(execution.status3c&0x10000)){
            int fencing=std::max(1,unit.fencing);
            amount=fencing*config_[0x8E]/config_[0x8F]-fencing*config_[0x90]/config_[0x91]+record.word(21);
            if(events.equipment_special(unit,19))amount*=2;
            amount=std::max(1,amount*(config_[12]+rng.bounded(config_[13]+3,execution.seconds))/10);
        }
        int healed=0,experience=0;
        if(unit.hp<unit.max_hp){
            amount=std::min(amount,unit.max_hp-unit.hp);
            if(rng.percent(execution.seconds)<=record.word(23))healed=amount;
            staged.hp+=healed;experience=std::max(1,std::min(60,healed/config_[0xE3]));
        }
        int proficiency=(6+(healed>0?1:0)-int(execution.difficulty))*5+10;
        // The bridge includes native proficiency/learning, XP multipliers, level-up
        // RNG and stat reinitialization. Rejection discards every staged mutation.
        if(!award(staged,rng,experience,proficiency,request.group))return {false,Failure::world_unresolved};
        unsigned mask=request.second?8:4;
        staged.action_points-=ap_base_+((staged.status34&mask)?ap_repeat_:0);
        staged.status34|=uint8_t(mask);staged.carry_points=std::min(staged.carry_points,staged.action_points);
        if(execution.skill_payment_state!=2)staged.st-=record.word(13);
        unit=std::move(staged);random=rng;request_.reset();page_=Page::closed;
        return {true,Failure::none,healed,experience,proficiency};
    }
    struct HostileResult {bool applied{},hit{},immune{};unsigned removed{},slot{};int chance{},roll{};Failure failure{Failure::none};};
    bool sword94_supported()const{return page_!=Page::closed&&selected().id==94&&selected().group==9&&selected().bit==0&&selected().effect()==1&&selected().raw[20]==44&&selected().raw[33]==194;}
    bool hostile_supported()const{return page_!=Page::closed&&selected().id==32&&selected().group==2&&selected().bit==11&&selected().effect()==34;}
    // 401146 builds concentric diamond rings; record17 is the radius, not AP.
    bool hostile_in_range(const OriginalEvents::Unit& actor,const OriginalEvents::Unit& target)const{
        if((!hostile_supported()&&!sword94_supported())||actor.hostile()==target.hostile()||target.hp<=0||!(target.status_flags&0x80)||(target.status_flags&0x20))return false;
        int dx=2*(target.x-actor.x)+int(target.y&1)-int(actor.y&1),dy=target.y-actor.y;
        return std::max(std::abs(dx),std::abs(dy))<=int(selected().raw[17])&&(&actor!=&target);
    }
    HostileResult execute_hostile(OriginalEvents::Unit& actor,OriginalEvents::Unit& target,
        original_combat::Random& random,ExecutionContext context,const OriginalPromotionTransaction& equipment,const AwardAdapter& award){
        HostileResult result;
        if(page_!=Page::requested||!request_||!hostile_supported()){result.failure=Failure::unsupported_execution;return result;}
        if(actor.id!=request_->unit_id||actor.appearance!=request_->appearance||!(actor.learned[2]&0x800)){result.failure=Failure::stale_unit;return result;}
        if(context.difficulty<1||context.difficulty>5||!hostile_in_range(actor,target)||!entry_available(actor,context_)||!award){result.failure=Failure::world_unresolved;return result;}
        if(auto failure=resource_failure(actor,selected());failure!=Failure::none){result.failure=failure;return result;}
        auto stagedActor=actor,stagedTarget=target;auto rng=random;
        std::vector<unsigned> slots;for(unsigned i=1;i<6;++i)if(target.equipment[i])slots.push_back(i);
        int defense=0;
        if(!slots.empty()){result.slot=slots[size_t(rng.bounded(int(slots.size()),context.seconds))];defense=equipment.item_defense(target.equipment[result.slot]);}
        // 46D874 ->407E2D modifies resistance first;46D496 adds its minimum5.
        int resistance=(actor.st-defense)/2;
        resistance=resistance*(config_[10]+rng.bounded(config_[11]+3,context.seconds))/10;
        if(context.friendly_army&&!context.conditions_mode)resistance=resistance*(100+config_[0xF3+context.difficulty])/100;
        result.chance=std::clamp(5+std::clamp(resistance,1,99),1,99);
        result.roll=rng.percent(context.seconds);result.hit=result.roll<=result.chance;
        unsigned status3e=target.action_status_runtime?target.action_status_runtime->at(0x3E):target.status3e;
        result.immune=(status3e&0x10)||(target.abilities&0x4000);
        if(result.hit){
            stagedTarget.status3e=uint8_t(status3e&~2u);
            if(stagedTarget.action_status_runtime)(*stagedTarget.action_status_runtime)[0x3E]&=uint8_t(~2u);
            if(!result.immune&&!slots.empty()){
                OriginalPromotionTransaction::State state{stagedTarget,std::vector<uint8_t>(equipment.item_count()),OriginalRandom({1,0})};
                result.removed=target.equipment[result.slot];equipment.remove_equipment(state,result.slot,true);stagedTarget=std::move(state.unit);
            }
        }
        int proficiency=(selected().word(13)+6-int(context.difficulty))*5/2+10;
        if(!award(stagedActor,rng,5,proficiency,2)){result.failure=Failure::world_unresolved;return result;}
        unsigned mask=4;stagedActor.action_points-=ap_base_+((stagedActor.status34&mask)?ap_repeat_:0);stagedActor.status34|=mask;
        stagedActor.carry_points=std::min(stagedActor.carry_points,stagedActor.action_points);
        if(context.skill_payment_state!=2)stagedActor.mp-=((actor.abilities&0x20)?selected().word(13)/2:selected().word(13));
        actor=std::move(stagedActor);target=std::move(stagedTarget);random=rng;result.applied=true;request_.reset();page_=Page::closed;return result;
    }
    void resume(){if(page_==Page::requested){request_.reset();page_=Page::list;}}
    void cancel(){request_.reset();page_=Page::closed;}
    void navigate(unsigned scan){if(page_!=Page::list)return;if(scan==0x4B||scan==0x4D){group_=scan==0x4B?(group_+groups_.size()-1)%groups_.size():(group_+1)%groups_.size();skill_=0;}else if(scan==0x48||scan==0x50){size_t n=groups_[group_].records.size();if(n)skill_=scan==0x48?(skill_+n-1)%n:(skill_+1)%n;}}
    bool mouse(int x,int y,bool click){if(page_!=Page::list)return false;if(x>=26&&x<=506&&y>=394&&y<=426){size_t index=size_t(x-26)/34;if(index<groups_[group_].records.size()){bool same=skill_==index;if(!click)skill_=index;return click&&same;}}for(size_t g=0;g<groups_.size();++g){int cx=508+int(g%3)*45,cy=394+int(g/3)*28;if(x>=cx-22&&x<=cx+22&&y>=cy&&y<cy+16){if(!click){group_=g;skill_=0;}return false;}}return false;}
    void draw(std::vector<uint8_t>& image)const{
        if(page_==Page::closed)return;if(image.size()!=640*480*4)throw std::invalid_argument("Skill menu requires 640x480 RGBA");sprite(image,98,0,380);
        for(size_t g=0;g<groups_.size();++g){const auto& name=groups_[g].name;font_.draw_small(image,name,508+int(g%3)*45-int(name.size())*4,394+int(g/3)*28,g==group_?60:105,true);}
        for(size_t s=0;s<groups_[group_].records.size();++s)sprite(image,121+2*(groups_[group_].records[s].id-1)+(s==skill_?1:0),26+int(s)*34,394);
        font_.draw_original_large(image,selected().name(),148,438,168);
    }
};
}
