// World.cpp — 깊은 구현부
//
// 여기 있는 것은 전부 바깥에서 안 보인다:
//   나라의 실제 저장 구조, 초→연도 변환, 인구·수입 성장 공식,
//   지지도 변동, 나라 단계 계산, 붕괴 판정.
//
// 공식들은 전부 임시값이다. 밸런싱하면서 바꿀 자리.

#include "World.h"

#include <algorithm>
#include <cmath>
#include <unordered_map>

namespace ww {

namespace {

// --- 튜닝 상수 (전부 임시) ---------------------------------------------
constexpr double kSecondsPerYear = 20.0;   // 기획서 결정 1-5

constexpr double kStartPopulation = 10000.0;
constexpr double kStartMoney      = 100000000.0;
constexpr double kStartSupport    = 30.0;

// 붕괴 임계치 (기획서 4.1)
double collapseThreshold(Ideology i) {
    switch (i) {
        case Ideology::Monarchy:  return 1.0;    // 반란
        case Ideology::Democracy: return 10.0;   // 탄핵
        case Ideology::Communism: return 5.0;
        case Ideology::Fascism:   return 0.0;    // 지지도로는 안 무너진다 (다른 대가를 치른다)
    }
    return 1.0;
}

// 0.05 단위로 내림 — 단계 표시용 (기획서 1.7)
double quantizeStage(double v) {
    return std::floor(v * 20.0) / 20.0;
}

} // namespace

// ---------------------------------------------------------------------
// 내부 데이터
// ---------------------------------------------------------------------
struct World::Impl {
    // 진짜 나라 데이터. 바깥은 이 타입의 존재조차 모른다.
    struct Country {
        CountryId   id = kNoCountry;
        std::string name;
        Ideology    ideology = Ideology::Monarchy;

        double population  = kStartPopulation;
        double money       = kStartMoney;
        double supportRate = kStartSupport;

        // 외교권 — 봉우리(결정 1-0)의 핵심 지표. 단계 승급의 주 기준.
        // 지금은 자리만 잡아둔다. 외교 시스템 붙이면 여기서 계산.
        double diplomaticPower = 0.0;

        double foundedYear = 0.0;
        bool   alive = true;
    };

    double    yearExact = -2000.0;
    CountryId nextId    = 1;
    std::unordered_map<CountryId, Country> countries;

    // --- 한 나라의 1년치 갱신 -----------------------------------------
    void step(Country& c, double years) {
        if (!c.alive) return;

        // 인구: 아주 단순한 지수 성장 (임시)
        c.population *= std::pow(1.005, years);

        // 수입: 인구 기반 (도시 경제 붙이면 여기가 바뀐다 — 기획서 6)
        c.money += c.population * 0.5 * years;

        // 지지도: 파시즘은 높게 유지되지만 다른 대가를 치른다 (기획서 4.3)
        double drift = (c.ideology == Ideology::Fascism) ? +0.3 : -0.05;
        c.supportRate = std::clamp(c.supportRate + drift * years, 0.0, 100.0);

        // 붕괴 판정
        if (c.supportRate <= collapseThreshold(c.ideology)) {
            c.alive = false;
        }
    }

    // --- 나라 단계 계산 (기획서 1.7) ----------------------------------
    // 승급 기준: 외교권 위주 + 강함 조금.
    // 지금은 외교 시스템이 없으므로 국력만으로 임시 계산한다.
    double stageOf(const Country& c) const {
        // 강함: 인구·돈을 로그로 눌러서 완만하게.
        // 건국 시점(인구 1만·돈 1억)이 0이 되도록 기준선을 뺀다 → 건국은 항상 단계 1.00.
        auto logRatio = [](double now, double base) {
            return std::log10(std::max(now, 1.0) / base);
        };
        double strength = logRatio(c.population, kStartPopulation)
                        + logRatio(c.money,      kStartMoney) * 0.5;

        // 외교권(주 기준) — 아직 0. 외교 붙이면 가중치가 여기로 실린다.
        double influence = c.diplomaticPower;

        // 강함은 보조(×0.5), 외교권이 주 기준(×1.0) — 기획서 1.7
        double raw = 1.0 + strength * 0.5 + influence * 1.0;
        return std::clamp(raw, 1.0, 4.0);
    }
};

// ---------------------------------------------------------------------
// 인터페이스 구현
// ---------------------------------------------------------------------
World::World(int startYear) : impl_(new Impl) {
    impl_->yearExact = static_cast<double>(startYear);
}

World::~World() { delete impl_; }

void World::advance(double realSeconds) {
    if (realSeconds <= 0.0) return;

    const double years = realSeconds / kSecondsPerYear;
    impl_->yearExact += years;

    for (auto& kv : impl_->countries) {
        impl_->step(kv.second, years);
    }
}

int    World::year()      const { return static_cast<int>(std::floor(impl_->yearExact)); }
double World::yearExact() const { return impl_->yearExact; }

CountryId World::found(const std::string& name, Ideology ideology) {
    if (name.empty()) return kNoCountry;

    Impl::Country c;
    c.id          = impl_->nextId++;
    c.name        = name;
    c.ideology    = ideology;
    c.foundedYear = impl_->yearExact;

    CountryId id = c.id;
    impl_->countries.emplace(id, std::move(c));
    return id;
}

CountryView World::look(CountryId id) const {
    CountryView v;
    auto it = impl_->countries.find(id);
    if (it == impl_->countries.end()) return v;  // id == kNoCountry 로 남는다

    const auto& c = it->second;
    v.id          = c.id;
    v.name        = c.name;
    v.ideology    = c.ideology;
    v.population  = c.population;
    v.money       = c.money;
    v.supportRate = c.supportRate;
    v.alive       = c.alive;

    double s      = impl_->stageOf(c);
    v.stage       = static_cast<int>(std::floor(s));   // 기능이 열리는 정수 단계
    v.stageShown  = quantizeStage(s);                  // 화면 표시용 0.05 단위
    return v;
}

std::vector<CountryId> World::countries() const {
    std::vector<CountryId> out;
    out.reserve(impl_->countries.size());
    for (const auto& kv : impl_->countries) {
        if (kv.second.alive) out.push_back(kv.first);
    }
    std::sort(out.begin(), out.end());   // 순서 안정화
    return out;
}

void World::destroy(CountryId id) {
    auto it = impl_->countries.find(id);
    if (it != impl_->countries.end()) it->second.alive = false;
}

// ---------------------------------------------------------------------
std::string formatYear(int year) {
    if (year < 0) return "BC " + std::to_string(-year);
    return "AD " + std::to_string(year);
}

const char* ideologyName(Ideology i) {
    switch (i) {
        case Ideology::Monarchy:  return "군주주의";
        case Ideology::Democracy: return "민주주의";
        case Ideology::Communism: return "공산주의";
        case Ideology::Fascism:   return "파시즘";
    }
    return "?";
}

} // namespace ww
