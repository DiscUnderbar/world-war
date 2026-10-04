// World.h — WorldWar MVP 세계 데이터 모듈 (C++ 프로토타입)
//
// 설계 원칙: Deep Module (좁은 인터페이스 + 깊은 구현)
//   바깥에 보이는 것은 World 하나뿐이다.
//   나라 데이터의 실제 구조, 시간 변환, 성장 공식, 단계 계산은
//   전부 World.cpp 안에 숨어 있다. 헤더만 봐서는 "무엇을 할 수 있나"만 보이고
//   "어떻게 하는가"는 안 보인다. 이것이 의도다.
//
// 숨기지 않는 것(정직한 Deep Module): 비용이 큰 동작은 이름으로 드러낸다.
//   advance()는 세계 전체를 갱신하므로 무겁다. look()은 스냅샷 복사다.

#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace ww {

// 나라 핸들. 바깥은 이 숫자만 들고 다닌다 (내부 구조를 모른다).
using CountryId = std::uint32_t;
constexpr CountryId kNoCountry = 0;

// 이념 — 붕괴 임계치가 다르다 (기획서 4.1)
enum class Ideology {
    Monarchy,   // 군주주의: 지지도 1% 이하 → 반란
    Democracy,  // 민주주의: 지지도 10% 이하 → 탄핵
    Communism,
    Fascism,    // 지지도는 높으나 대가를 치른다
};

// 조회용 읽기 전용 스냅샷.
// 바깥은 나라를 직접 만지지 못하고 이 사본만 본다 → 변경 경로가 World로 단일화된다.
struct CountryView {
    CountryId id = kNoCountry;
    std::string name;

    Ideology ideology = Ideology::Monarchy;
    double   population = 0.0;
    double   money = 0.0;
    double   supportRate = 0.0;   // 지지도 %

    // 나라 단계 (기획서 1.7)
    //   stage      : 기능이 열리는 정수 단계 (1~4)
    //   stageShown : 화면 표시용 0.05 단위 소수 (예: 1.35)
    int    stage = 1;
    double stageShown = 1.0;

    bool alive = true;
};

// 세계 하나 = 게임 한 판.
class World {
public:
    // 기본 시작: 기원전 2000년 (기획서 결정 1-2)
    explicit World(int startYear = -2000);
    ~World();

    World(const World&) = delete;
    World& operator=(const World&) = delete;

    // --- 시간 ---------------------------------------------------------
    // 실제 흐른 초를 넣으면 세계가 그만큼 나아간다.
    // 1 게임년 = 20 실시간초 (기획서 결정 1-5). 변환은 내부에 숨긴다.
    // 무겁다: 모든 나라를 갱신한다.
    void advance(double realSeconds);

    int    year()      const;  // 현재 연도 (음수 = 기원전)
    double yearExact() const;  // 소수 포함 연도

    // --- 나라 ---------------------------------------------------------
    // 건국. 실패하면 kNoCountry.
    CountryId found(const std::string& name, Ideology ideology = Ideology::Monarchy);

    // 조회 (스냅샷 복사본을 준다)
    CountryView look(CountryId id) const;
    std::vector<CountryId> countries() const;   // 살아있는 나라들

    // 멸망 처리
    void destroy(CountryId id);

private:
    struct Impl;          // 실제 데이터·공식은 전부 여기 (World.cpp)
    Impl* impl_;          // pimpl — 헤더에 내부 구조를 노출하지 않는다
};

// 표시용 헬퍼 (연도 문자열: -1999 → "BC 1999")
std::string formatYear(int year);
const char* ideologyName(Ideology i);

} // namespace ww
