// main.cpp — 세계 뼈대 동작 확인용 데모
// 나라 몇 개 세우고 시간을 흘려본다. 딱 그것만.

#include "World.h"

#include <iomanip>
#include <iostream>

int main() {
    ww::World world;   // BC 2000 시작

    world.found("Tennin",  ww::Ideology::Monarchy);
    world.found("Zevring", ww::Ideology::Democracy);
    world.found("Cocine",  ww::Ideology::Fascism);

    // 20초 = 1년. 100번 * 20초 = 100년
    for (int i = 0; i < 100; ++i) {
        world.advance(20.0);

        if (i % 25 != 24) continue;   // 25년마다 출력

        std::cout << "===== " << ww::formatYear(world.year()) << " =====\n";
        for (auto id : world.countries()) {
            auto c = world.look(id);
            std::cout << "  " << std::left << std::setw(9) << c.name
                      << " | " << ww::ideologyName(c.ideology)
                      << " | 단계 " << std::fixed << std::setprecision(2) << c.stageShown
                      << " | 인구 "  << std::setprecision(0) << c.population
                      << " | 지지도 " << std::setprecision(1) << c.supportRate << "%"
                      << "\n";
        }
        std::cout << "\n";
    }

    return 0;
}
