import std;
import ZhouYi.GanZhi;
import ZhouYi.ZiWei.Constants;
import ZhouYi.ZiWei.Star;
import ZhouYi.ZiWei.Horoscope;
import ZhouYi.ZiWei.Palace;
import ZhouYi.ZiWei.SiHua;
import ZhouYi.ZiWei.Controller;
import ZhouYi.ZiWei;
import ZhouYi.tyme;
import nlohmann.json;

using namespace std;
using namespace ZhouYi::ZiWei;

int main() {
    const array<pair<WuXingJu, string>, 5> ju_list = {{
        {WuXingJu::ShuiErJu, "水二局"},
        {WuXingJu::MuSanJu,  "木三局"},
        {WuXingJu::JinSiJu,  "金四局"},
        {WuXingJu::TuWuJu,   "土五局"},
        {WuXingJu::HuoLiuJu, "火六局"}
    }};

    const array<string, 12> gong_names = {
        "寅", "卯", "辰", "巳", "午", "未",
        "申", "酉", "戌", "亥", "子", "丑"
    };

    cout << "===== 紫微星定位测试表 =====\n";

    for (const auto& [ju, ju_name] : ju_list) {
        cout << "\n[" << ju_name << "]\n";

        for (int day = 1; day <= 30; ++day) {
            int idx = get_zi_wei_index(day, ju);

            cout << day << "日 -> "
                 << gong_names[idx]
                 << "宫";

            if (day % 5 == 0) {
                cout << '\n';
            } else {
                cout << "    ";
            }
        }

        cout << '\n';
    }


    cout << "\n===== 小限回归测试 =====\n";

    int failed = 0;

    auto check_xiao_xian = [&](string_view name,
                               int age,
                               bool is_male,
                               ZhouYi::GanZhi::DiZhi year_zhi,
                               int expected_index) {
        auto result = get_xiao_xian(age, is_male, year_zhi);

        bool ok = (result.gong_index == expected_index);

        cout << (ok ? "[PASS] " : "[FAIL] ")
             << name
             << "：虚岁" << age
             << " -> " << gong_names[result.gong_index]
             << "宫，预期 " << gong_names[expected_index] << "宫\n";

        if (!ok) {
            ++failed;
        }
    };

    // 寅午戌年：1岁辰起；男顺
    check_xiao_xian("寅年男", 1, true, ZhouYi::GanZhi::DiZhi::Yin, 2);  // 辰
    check_xiao_xian("寅年男", 2, true, ZhouYi::GanZhi::DiZhi::Yin, 3);  // 巳
    check_xiao_xian("寅年男", 13, true, ZhouYi::GanZhi::DiZhi::Yin, 2); // 辰

    // 申子辰年：1岁戌起；女逆
    check_xiao_xian("辰年女", 1, false, ZhouYi::GanZhi::DiZhi::Chen, 8);  // 戌
    check_xiao_xian("辰年女", 2, false, ZhouYi::GanZhi::DiZhi::Chen, 7);  // 酉
    check_xiao_xian("辰年女", 13, false, ZhouYi::GanZhi::DiZhi::Chen, 8); // 戌

    // 巳酉丑年：1岁未起
    check_xiao_xian("酉年男", 1, true, ZhouYi::GanZhi::DiZhi::You, 5);   // 未

    // 亥卯未年：1岁丑起
    check_xiao_xian("卯年女", 1, false, ZhouYi::GanZhi::DiZhi::Mao, 11); // 丑

    if (failed != 0) {
        cerr << "\n❌ 小限测试失败数量: " << failed << '\n';
        return 1;
    }

    cout << "✅ 小限回归测试全部通过\n";

    cout << "\n===== 流月斗君回归测试 =====\n";

    auto check_liu_yue = [&](string_view name,
                             int lunar_month,
                             int birth_month,
                             ZhouYi::GanZhi::DiZhi birth_hour_zhi,
                             ZhouYi::GanZhi::DiZhi year_zhi,
                             int expected_index) {
        auto result = get_liu_yue(
            lunar_month,
            birth_month,
            birth_hour_zhi,
            ZhouYi::GanZhi::TianGan::Jia,
            year_zhi,
            LiuYueGanZhiPolicy::LunarMonthWuHuDun,
            ZhouYi::GanZhi::TianGan::Jia,
            ZhouYi::GanZhi::DiZhi::Yin
        );

        bool ok = (result.gong_index == expected_index);

        cout << (ok ? "[PASS] " : "[FAIL] ")
             << name
             << " -> " << gong_names[result.gong_index]
             << "宫，预期 " << gong_names[expected_index] << "宫\n";

        if (!ok) {
            ++failed;
        }
    };

    // 子年太岁宫 index 10，出生3月，午时 index 6
    // 斗君 = 10 - 2 + 6 = 14 -> index 2（辰）
    check_liu_yue(
        "子年・生3月午时・正月",
        1,
        3,
        ZhouYi::GanZhi::DiZhi::Wu,
        ZhouYi::GanZhi::DiZhi::Zi,
        2
    );

    check_liu_yue(
        "子年・生3月午时・二月",
        2,
        3,
        ZhouYi::GanZhi::DiZhi::Wu,
        ZhouYi::GanZhi::DiZhi::Zi,
        3
    );

    check_liu_yue(
        "子年・生3月午时・三月",
        3,
        3,
        ZhouYi::GanZhi::DiZhi::Wu,
        ZhouYi::GanZhi::DiZhi::Zi,
        4
    );

    if (failed != 0) {
        cerr << "\n❌ 运限核心测试失败数量: " << failed << '\n';
        return 1;
    }

    cout << "✅ 流月斗君回归测试全部通过\n";

    cout << "\n===== 闰月策略回归测试 =====\n";

    auto check_leap_month = [&](string_view name,
                                int raw_month,
                                int day,
                                LeapMonthPolicy policy,
                                int expected_month) {
        int actual = resolve_ziwei_month(
            raw_month,
            day,
            policy
        );

        bool ok = (actual == expected_month);

        cout << (ok ? "[PASS] " : "[FAIL] ")
             << name
             << " -> " << actual
             << "月，预期 " << expected_month << "月\n";

        if (!ok) {
            ++failed;
        }
    };

    // 普通月份：任何策略都不能改变月份
    check_leap_month(
        "普通五月",
        5, 20,
        LeapMonthPolicy::SplitAtDay15,
        5
    );

    // 闰五月按同名月
    check_leap_month(
        "闰五月二十・同月策略",
        -5, 20,
        LeapMonthPolicy::SameAsRegularMonth,
        5
    );

    // 闰五月整体按下月
    check_leap_month(
        "闰五月初十・下月策略",
        -5, 10,
        LeapMonthPolicy::NextMonth,
        6
    );

    // 分月策略：十五以前仍属本月
    check_leap_month(
        "闰五月十五・分月策略",
        -5, 15,
        LeapMonthPolicy::SplitAtDay15,
        5
    );

    // 分月策略：十六日起归下月
    check_leap_month(
        "闰五月十六・分月策略",
        -5, 16,
        LeapMonthPolicy::SplitAtDay15,
        6
    );

    // 十二月跨年边界
    check_leap_month(
        "闰十二月二十・下月策略",
        -12, 20,
        LeapMonthPolicy::NextMonth,
        1
    );

    check_leap_month(
        "闰十二月二十・分月策略",
        -12, 20,
        LeapMonthPolicy::SplitAtDay15,
        1
    );

    if (failed != 0) {
        cerr << "\n❌ 核心回归测试失败数量: " << failed << '\n';
        return 1;
    }

    cout << "✅ 闰月策略回归测试全部通过\n";

    cout << "\n===== 闰月端到端排盘测试 =====\n";

    int leap_year = 0;
    int leap_month = 0;

    for (int year = 2020; year <= 2030 && leap_year == 0; ++year) {
        for (int month = 1; month <= 12; ++month) {
            try {
                auto probe = pai_pan_lunar(
                    year,
                    month,
                    20,
                    12,
                    true,
                    true,
                    LeapMonthPolicy::SameAsRegularMonth
                );

                if (probe.is_leap_month &&
                    probe.raw_lunar_month == -month) {
                    leap_year = year;
                    leap_month = month;
                    break;
                }
            } catch (...) {
            }
        }
    }

    if (leap_year == 0) {
        cerr << "[FAIL] 2020~2030 未找到可用真实闰月 fixture\n";
        return 1;
    }

    cout << "[PASS] 找到真实闰月 fixture："
         << leap_year << "年闰" << leap_month << "月\n";

    auto same = pai_pan_lunar(
        leap_year,
        leap_month,
        20,
        12,
        true,
        true,
        LeapMonthPolicy::SameAsRegularMonth
    );

    auto next = pai_pan_lunar(
        leap_year,
        leap_month,
        20,
        12,
        true,
        true,
        LeapMonthPolicy::NextMonth
    );

    auto split = pai_pan_lunar(
        leap_year,
        leap_month,
        20,
        12,
        true,
        true,
        LeapMonthPolicy::SplitAtDay15
    );

    int expected_next_month = (leap_month % 12) + 1;

    auto check_e2e = [&](string_view name, bool ok) {
        cout << (ok ? "[PASS] " : "[FAIL] ") << name << '\n';
        if (!ok) {
            ++failed;
        }
    };

    check_e2e(
        "真实闰月标记",
        same.is_leap_month &&
        same.raw_lunar_month == -leap_month
    );

    check_e2e(
        "SameAsRegularMonth 保存并解析正确",
        same.leap_month_policy == LeapMonthPolicy::SameAsRegularMonth &&
        same.resolved_lunar_month == leap_month
    );

    check_e2e(
        "NextMonth 保存并解析正确",
        next.leap_month_policy == LeapMonthPolicy::NextMonth &&
        next.resolved_lunar_month == expected_next_month
    );

    check_e2e(
        "SplitAtDay15 十六日后解析正确",
        split.leap_month_policy == LeapMonthPolicy::SplitAtDay15 &&
        split.resolved_lunar_month == expected_next_month
    );

    check_e2e(
        "闰月策略实际影响本命命宫",
        same.ming_gong_index != next.ming_gong_index
    );

    check_e2e(
        "NextMonth 与 SplitAtDay15(20日)本命一致",
        next.ming_gong_index == split.ming_gong_index
    );

    if (failed != 0) {
        cerr << "\n❌ 端到端核心测试失败数量: "
             << failed << '\n';
        return 1;
    }

    cout << "✅ 闰月端到端排盘测试全部通过\n";

    cout << "\n===== 小限缓存一致性测试 =====\n";

    // 使用普通日期生成真实 ZiWeiResult，
    // 对比动态 get_xiao_xian() 与 palaces[].xiao_xian_ages。
    auto cache_chart = pai_pan_solar(
        2004, 5, 31, 13, false
    );

    auto birth_zhi = cache_chart.year_pillar.zhi;

    for (int age = 1; age <= 60; ++age) {
        auto xiao = get_xiao_xian(
            age,
            cache_chart.is_male,
            birth_zhi
        );

        bool found_in_cache = false;

        for (const auto& cached_age :
             cache_chart.palaces[xiao.gong_index].xiao_xian_ages) {
            if (cached_age == age) {
                found_in_cache = true;
                break;
            }
        }

        if (!found_in_cache) {
            cout << "[FAIL] 虚岁" << age
                 << " 动态小限宫与缓存不一致\n";
            ++failed;
        }
    }

    if (failed != 0) {
        cerr << "\n❌ 小限缓存一致性测试失败数量: "
             << failed << '\n';
        return 1;
    }

    cout << "✅ 小限动态算法与 palaces[].xiao_xian_ages 一致\n";

    cout << "\n===== 流年缓存一致性测试 =====\n";

    int birth_solar_year = cache_chart.solar_day.get_year();

    for (int age = 1; age <= 60; ++age) {
        int target_year = birth_solar_year + age - 1;

        auto target_solar_day = tyme::SolarDay::from_ymd(
            target_year, 7, 1
        );
        auto target_cycle_day =
            target_solar_day.get_sixty_cycle_day();
        auto target_year_cycle =
            target_cycle_day.get_year();

        TianGan target_year_gan =
            static_cast<TianGan>(
                target_year_cycle.get_heaven_stem().get_index()
            );

        DiZhi target_year_zhi =
            static_cast<DiZhi>(
                target_year_cycle.get_earth_branch().get_index()
            );

        auto liu_nian = get_liu_nian(
            target_year,
            target_year_gan,
            target_year_zhi,
            cache_chart.ming_gong_index
        );

        bool found_in_cache = false;

        for (const auto& cached_age :
             cache_chart.palaces[liu_nian.gong_index].liu_nian_ages) {
            if (cached_age == age) {
                found_in_cache = true;
                break;
            }
        }

        if (!found_in_cache) {
            cout << "[FAIL] 虚岁" << age
                 << " 流年宫与缓存不一致"
                 << "，target_year=" << target_year
                 << '\n';
            ++failed;
        }
    }

    if (failed != 0) {
        cerr << "\n❌ 流年缓存一致性测试失败数量: "
             << failed << '\n';
        return 1;
    }

    cout << "✅ 流年动态算法与 palaces[].liu_nian_ages 一致\n";

    cout << "\n===== 大限核心回归测试 =====\n";

    vector<GongWeiData> da_xian_palaces;
    da_xian_palaces.reserve(12);
    for (const auto& palace : cache_chart.palaces) {
        da_xian_palaces.push_back(palace.gong_data);
    }

    auto check_da_xian_set =
        [&](string_view name,
            WuXingJu ju,
            bool is_male,
            DiZhi year_zhi,
            bool expected_shun) {

        auto data = arrange_da_xian(
            cache_chart.ming_gong_index,
            ju,
            is_male,
            year_zhi,
            da_xian_palaces
        );

        int qi_yun_age = static_cast<int>(ju);
        bool ok = true;

        array<bool, 12> seen_gong{};

        for (int step = 0; step < 12; ++step) {
            int expected_gong = expected_shun
                ? (cache_chart.ming_gong_index + step) % 12
                : (cache_chart.ming_gong_index - step + 120) % 12;

            int expected_start = qi_yun_age + step * 10;
            int expected_end = expected_start + 9;

            // da_xian_data 是按宫位索引存储
            const auto& dx = data[expected_gong];

            if (dx.gong_index != expected_gong ||
                dx.start_age != expected_start ||
                dx.end_age != expected_end) {
                cout << "[FAIL] " << name
                     << " 第" << (step + 1) << "限"
                     << " 宫位/年龄错误\n";
                ok = false;
            }

            // 大限干支必须等于所行本命宫干支
            const auto& natal_palace = da_xian_palaces[expected_gong];

            if (dx.tian_gan != natal_palace.tian_gan ||
                dx.di_zhi != natal_palace.di_zhi) {
                cout << "[FAIL] " << name
                     << " 第" << (step + 1)
                     << "限干支与本命宫不一致\n";
                ok = false;
            }

            // 大限四化必须由该大限宫天干决定，且顺序为禄权科忌
            auto expected_si_hua =
                get_si_hua_star_names(natal_palace.tian_gan);

            if (dx.si_hua != expected_si_hua) {
                cout << "[FAIL] " << name
                     << " 第" << (step + 1)
                     << "限四化错误\n";
                ok = false;
            }

            if (seen_gong[expected_gong]) {
                cout << "[FAIL] " << name
                     << " 出现重复大限宫\n";
                ok = false;
            }
            seen_gong[expected_gong] = true;
        }

        for (bool seen : seen_gong) {
            if (!seen) {
                cout << "[FAIL] " << name
                     << " 未覆盖全部12宫\n";
                ok = false;
                break;
            }
        }

        // 第一大限必须从命宫开始
        const auto& first = data[cache_chart.ming_gong_index];
        if (first.start_age != qi_yun_age ||
            first.end_age != qi_yun_age + 9) {
            cout << "[FAIL] " << name
                 << " 第一大限没有从命宫起\n";
            ok = false;
        }

        cout << (ok ? "[PASS] " : "[FAIL] ")
             << name << '\n';

        if (!ok) {
            ++failed;
        }
    };

    // 申支 enum index=8，为阳支：
    // 男命顺、女命逆。
    // 同时覆盖五行局不同起限年龄。
    check_da_xian_set(
        "水二局・申年男・顺行",
        WuXingJu::ShuiErJu,
        true,
        DiZhi::Shen,
        true
    );

    check_da_xian_set(
        "木三局・申年女・逆行",
        WuXingJu::MuSanJu,
        false,
        DiZhi::Shen,
        false
    );

    // 酉支 enum index=9，为阴支：
    // 男命逆、女命顺。
    check_da_xian_set(
        "金四局・酉年男・逆行",
        WuXingJu::JinSiJu,
        true,
        DiZhi::You,
        false
    );

    check_da_xian_set(
        "土五局・酉年女・顺行",
        WuXingJu::TuWuJu,
        false,
        DiZhi::You,
        true
    );

    check_da_xian_set(
        "火六局・申年男・顺行",
        WuXingJu::HuoLiuJu,
        true,
        DiZhi::Shen,
        true
    );

    if (failed != 0) {
        cerr << "\n❌ 大限核心测试失败数量: "
             << failed << '\n';
        return 1;
    }

    cout << "✅ 大限核心回归测试全部通过\n";

    cout << "\n===== 流日核心回归测试 =====\n";

    {
        auto check_liu_ri =
            [&](int lunar_day,
                TianGan day_gan,
                DiZhi day_zhi,
                int liu_yue_index,
                int expected_gong,
                string_view name) {

            auto data = get_liu_ri(
                lunar_day,
                day_gan,
                day_zhi,
                liu_yue_index
            );

            bool ok = true;

            if (data.gong_index != expected_gong) {
                cout << "[FAIL] " << name
                     << " 宫位=" << data.gong_index
                     << "，预期=" << expected_gong << '\n';
                ok = false;
            }

            if (data.day != lunar_day ||
                data.tian_gan != day_gan ||
                data.di_zhi != day_zhi) {
                cout << "[FAIL] " << name
                     << " 输入干支/日期没有正确保留\n";
                ok = false;
            }

            auto expected_si_hua =
                get_si_hua_star_names(day_gan);

            if (data.si_hua != expected_si_hua) {
                cout << "[FAIL] " << name
                     << " 流日四化错误\n";
                ok = false;
            }

            cout << (ok ? "[PASS] " : "[FAIL] ")
                 << name << '\n';

            if (!ok) {
                ++failed;
            }
        };

        // 假设流月在辰宫(index 2)
        check_liu_ri(
            1,
            TianGan::Jia,
            DiZhi::Zi,
            2,
            2,
            "流月辰宫・初一 -> 辰宫"
        );

        check_liu_ri(
            2,
            TianGan::Yi,
            DiZhi::Chou,
            2,
            3,
            "流月辰宫・初二 -> 巳宫"
        );

        check_liu_ri(
            13,
            TianGan::Bing,
            DiZhi::Yin,
            2,
            2,
            "流月辰宫・十三 -> 辰宫"
        );
    }

    if (failed != 0) {
        cerr << "\n❌ 流日核心测试失败数量: "
             << failed << '\n';
        return 1;
    }

    cout << "✅ 流日核心回归测试全部通过\n";

    cout << "\n===== 流时核心回归测试 =====\n";

    {
        auto check_liu_shi =
            [&](DiZhi hour_zhi,
                TianGan hour_gan,
                int liu_ri_index,
                int expected_gong,
                string_view name) {

            auto data = get_liu_shi(
                hour_zhi,
                hour_gan,
                liu_ri_index
            );

            bool ok = true;

            if (data.gong_index != expected_gong) {
                cout << "[FAIL] " << name
                     << " 宫位=" << data.gong_index
                     << "，预期=" << expected_gong << '\n';
                ok = false;
            }

            if (data.shi_chen != hour_zhi ||
                data.tian_gan != hour_gan ||
                data.di_zhi != hour_zhi) {
                cout << "[FAIL] " << name
                     << " 输入干支没有正确保留\n";
                ok = false;
            }

            auto expected_si_hua =
                get_si_hua_star_names(hour_gan);

            if (data.si_hua != expected_si_hua) {
                cout << "[FAIL] " << name
                     << " 流时四化错误\n";
                ok = false;
            }

            cout << (ok ? "[PASS] " : "[FAIL] ")
                 << name << '\n';

            if (!ok) {
                ++failed;
            }
        };

        // 假设流日在午宫(index 4)
        check_liu_shi(
            DiZhi::Zi,
            TianGan::Jia,
            4,
            4,
            "流日午宫・子时 -> 午宫"
        );

        check_liu_shi(
            DiZhi::Chou,
            TianGan::Yi,
            4,
            5,
            "流日午宫・丑时 -> 未宫"
        );

        check_liu_shi(
            DiZhi::Hai,
            TianGan::Bing,
            4,
            3,
            "流日午宫・亥时 -> 巳宫"
        );
    }

    if (failed != 0) {
        cerr << "\n❌ 流时核心测试失败数量: "
             << failed << '\n';
        return 1;
    }

    cout << "✅ 流时核心回归测试全部通过\n";

    cout << "\n===== 年份干支基元回归测试 =====\n";

    {
        struct YearCase {
            int year;
            ZhouYi::GanZhi::TianGan gan;
            ZhouYi::GanZhi::DiZhi zhi;
            string_view name;
        };

        const array<YearCase, 4> cases = {{
            {1984, ZhouYi::GanZhi::TianGan::Jia,
                   ZhouYi::GanZhi::DiZhi::Zi,
                   "1984 -> 甲子"},
            {2020, ZhouYi::GanZhi::TianGan::Geng,
                   ZhouYi::GanZhi::DiZhi::Zi,
                   "2020 -> 庚子"},
            {2024, ZhouYi::GanZhi::TianGan::Jia,
                   ZhouYi::GanZhi::DiZhi::Chen,
                   "2024 -> 甲辰"},
            {2026, ZhouYi::GanZhi::TianGan::Bing,
                   ZhouYi::GanZhi::DiZhi::Wu,
                   "2026 -> 丙午"}
        }};

        for (const auto& tc : cases) {
            auto [gan, zhi] = get_year_gan_zhi_from_year(tc.year);

            bool ok = gan == tc.gan && zhi == tc.zhi;

            cout << (ok ? "[PASS] " : "[FAIL] ")
                 << tc.name << '\n';

            if (!ok) {
                ++failed;
            }
        }
    }

    cout << "\n===== 流年换年边界策略回归测试 =====\n";

    {
        struct BoundaryCase {
            int month;
            int day;
            int expected_lunar_year;
            int expected_li_chun_year;
            string_view name;
        };

        const array<BoundaryCase, 3> cases = {{
            {2, 3,  2025, 2025, "立春前：两策略均属2025"},
            {2, 5,  2025, 2026, "立春后春节前：两策略分歧"},
            {2, 17, 2026, 2026, "春节后：两策略均属2026"}
        }};

        for (const auto& tc : cases) {
            auto solar =
                tyme::SolarDay::from_ymd(2026, tc.month, tc.day);

            int lunar_year =
                solar.get_lunar_day().get_year();

            int li_chun_year =
                solar.get_sixty_cycle_day()
                     .get_sixty_cycle_month()
                     .get_sixty_cycle_year()
                     .get_year();

            int by_lunar = resolve_liu_nian_year(
                lunar_year,
                li_chun_year,
                LiuNianYearBoundaryPolicy::LunarNewYear
            );

            int by_li_chun = resolve_liu_nian_year(
                lunar_year,
                li_chun_year,
                LiuNianYearBoundaryPolicy::LiChun
            );

            bool ok =
                lunar_year == tc.expected_lunar_year &&
                li_chun_year == tc.expected_li_chun_year &&
                by_lunar == tc.expected_lunar_year &&
                by_li_chun == tc.expected_li_chun_year;

            cout << (ok ? "[PASS] " : "[FAIL] ")
                 << tc.name << '\n';

            if (!ok) {
                ++failed;
            }
        }

        auto [lunar_gan, lunar_zhi] =
            get_year_gan_zhi_from_year(2025);
        auto [li_chun_gan, li_chun_zhi] =
            get_year_gan_zhi_from_year(2026);

        bool ganzhi_diff =
            lunar_gan != li_chun_gan ||
            lunar_zhi != li_chun_zhi;

        cout << (ganzhi_diff ? "[PASS] " : "[FAIL] ")
             << "错位区间两策略产生不同流年干支\n";

        if (!ganzhi_diff) {
            ++failed;
        }
    }

    cout << "\n===== 农历流月五虎遁回归测试 =====\n";

    {
        struct MonthGanZhiCase {
            TianGan year_gan;
            int month;
            TianGan expected_gan;
            DiZhi expected_zhi;
            string_view name;
        };

        const array<MonthGanZhiCase, 10> cases = {{
            {TianGan::Jia, 1,  TianGan::Bing, DiZhi::Yin,  "甲年正月 -> 丙寅"},
            {TianGan::Ji,  1,  TianGan::Bing, DiZhi::Yin,  "己年正月 -> 丙寅"},
            {TianGan::Yi,  1,  TianGan::Wu,   DiZhi::Yin,  "乙年正月 -> 戊寅"},
            {TianGan::Geng,1,  TianGan::Wu,   DiZhi::Yin,  "庚年正月 -> 戊寅"},
            {TianGan::Bing,1,  TianGan::Geng, DiZhi::Yin,  "丙年正月 -> 庚寅"},
            {TianGan::Xin, 1,  TianGan::Geng, DiZhi::Yin,  "辛年正月 -> 庚寅"},
            {TianGan::Ding,1,  TianGan::Ren,  DiZhi::Yin,  "丁年正月 -> 壬寅"},
            {TianGan::Ren, 1,  TianGan::Ren,  DiZhi::Yin,  "壬年正月 -> 壬寅"},
            {TianGan::Wu,  1,  TianGan::Jia,  DiZhi::Yin,  "戊年正月 -> 甲寅"},
            {TianGan::Gui,12,  TianGan::Yi,   DiZhi::Chou, "癸年十二月 -> 乙丑"}
        }};

        for (const auto& tc : cases) {
            auto [gan, zhi] =
                get_lunar_month_gan_zhi(tc.year_gan, tc.month);

            bool ok =
                gan == tc.expected_gan &&
                zhi == tc.expected_zhi;

            cout << (ok ? "[PASS] " : "[FAIL] ")
                 << tc.name << '\n';

            if (!ok) {
                ++failed;
            }
        }

        // 同一个农历月不能因为公历节气跨界而改变五虎遁结果。
        auto [gan_before, zhi_before] =
            get_lunar_month_gan_zhi(TianGan::Bing, 6);
        auto [gan_after, zhi_after] =
            get_lunar_month_gan_zhi(TianGan::Bing, 6);

        bool stable =
            gan_before == gan_after &&
            zhi_before == zhi_after;

        cout << (stable ? "[PASS] " : "[FAIL] ")
             << "同一农历六月干支不受节令换月影响\n";

        if (!stable) {
            ++failed;
        }
    }

    if (failed != 0) {
        cerr << "\n❌ 农历流月五虎遁测试失败数量: "
             << failed << '\n';
        return 1;
    }

    cout << "✅ 农历流月五虎遁回归测试全部通过\n";

    cout << "\n===== 流月干支策略对照测试 =====\n";

    {
        // 同一斗君输入：宫位必须相同。
        // 农历策略：甲年六月 -> 辛未
        // 节令策略：这里人为传入壬申，用来验证策略确实切换来源。
        auto lunar_policy = get_liu_yue(
            6,
            3,
            ZhouYi::GanZhi::DiZhi::Wu,
            ZhouYi::GanZhi::TianGan::Jia,
            ZhouYi::GanZhi::DiZhi::Zi,
            LiuYueGanZhiPolicy::LunarMonthWuHuDun,
            ZhouYi::GanZhi::TianGan::Ren,
            ZhouYi::GanZhi::DiZhi::Shen
        );

        auto solar_term_policy = get_liu_yue(
            6,
            3,
            ZhouYi::GanZhi::DiZhi::Wu,
            ZhouYi::GanZhi::TianGan::Jia,
            ZhouYi::GanZhi::DiZhi::Zi,
            LiuYueGanZhiPolicy::SolarTermMonthPillar,
            ZhouYi::GanZhi::TianGan::Ren,
            ZhouYi::GanZhi::DiZhi::Shen
        );

        bool same_gong =
            lunar_policy.gong_index == solar_term_policy.gong_index;

        cout << (same_gong ? "[PASS] " : "[FAIL] ")
             << "两种流月干支策略不改变斗君宫位\n";

        if (!same_gong) {
            ++failed;
        }

        bool lunar_ok =
            lunar_policy.tian_gan == ZhouYi::GanZhi::TianGan::Xin &&
            lunar_policy.di_zhi == ZhouYi::GanZhi::DiZhi::Wei;

        cout << (lunar_ok ? "[PASS] " : "[FAIL] ")
             << "农历策略：甲年六月 -> 辛未\n";

        if (!lunar_ok) {
            ++failed;
        }

        bool solar_ok =
            solar_term_policy.tian_gan == ZhouYi::GanZhi::TianGan::Ren &&
            solar_term_policy.di_zhi == ZhouYi::GanZhi::DiZhi::Shen;

        cout << (solar_ok ? "[PASS] " : "[FAIL] ")
             << "节令策略：使用显式节令月柱\n";

        if (!solar_ok) {
            ++failed;
        }

        bool si_hua_ok =
            lunar_policy.si_hua ==
                get_si_hua_star_names(ZhouYi::GanZhi::TianGan::Xin) &&
            solar_term_policy.si_hua ==
                get_si_hua_star_names(ZhouYi::GanZhi::TianGan::Ren);

        cout << (si_hua_ok ? "[PASS] " : "[FAIL] ")
             << "流月四化严格跟最终月干\n";

        if (!si_hua_ok) {
            ++failed;
        }
    }

    if (failed != 0) {
        cerr << "\n❌ 流月干支策略测试失败数量: "
             << failed << '\n';
        return 1;
    }

    cout << "✅ 流月干支策略对照测试全部通过\n";


    cout << "\n===== 晚子时换日策略回归测试 =====\n";

    {
        constexpr int year = 2024;
        constexpr int month = 2;
        constexpr int day = 9;

        // 2024-02-09 的次日为春节正月初一，可同时覆盖农历跨月边界。
        auto original_lunar =
            tyme::SolarDay::from_ymd(year, month, day)
                .get_lunar_day();

        auto next_lunar =
            tyme::SolarDay::from_ymd(year, month, day)
                .next(1)
                .get_lunar_day();

        auto late_zi_22 = pai_pan_solar(
            year,
            month,
            day,
            22,
            true,
            LeapMonthPolicy::SameAsRegularMonth,
            ZiHourDayBoundaryPolicy::LateZi
        );

        auto midnight_23 = pai_pan_solar(
            year,
            month,
            day,
            23,
            true,
            LeapMonthPolicy::SameAsRegularMonth,
            ZiHourDayBoundaryPolicy::Midnight
        );

        auto late_zi_23 = pai_pan_solar(
            year,
            month,
            day,
            23,
            true,
            LeapMonthPolicy::SameAsRegularMonth,
            ZiHourDayBoundaryPolicy::LateZi
        );

        bool hour22_ok =
            !late_zi_22.zi_hour_shifted_to_next_day &&
            late_zi_22.zi_hour_day_boundary_policy ==
                ZiHourDayBoundaryPolicy::LateZi &&
            late_zi_22.raw_lunar_month ==
                original_lunar.get_month() &&
            late_zi_22.resolved_lunar_day ==
                original_lunar.get_day();

        cout << (hour22_ok ? "[PASS] " : "[FAIL] ")
             << "22:00 + LateZi 不换日\n";

        if (!hour22_ok) {
            ++failed;
        }

        bool midnight23_ok =
            !midnight_23.zi_hour_shifted_to_next_day &&
            midnight_23.zi_hour_day_boundary_policy ==
                ZiHourDayBoundaryPolicy::Midnight &&
            midnight_23.raw_lunar_month ==
                original_lunar.get_month() &&
            midnight_23.resolved_lunar_day ==
                original_lunar.get_day();

        cout << (midnight23_ok ? "[PASS] " : "[FAIL] ")
             << "23:00 + Midnight 仍使用当天农历日期\n";

        if (!midnight23_ok) {
            ++failed;
        }

        bool late23_ok =
            late_zi_23.zi_hour_shifted_to_next_day &&
            late_zi_23.zi_hour_day_boundary_policy ==
                ZiHourDayBoundaryPolicy::LateZi &&
            late_zi_23.raw_lunar_month ==
                next_lunar.get_month() &&
            late_zi_23.resolved_lunar_day ==
                next_lunar.get_day();

        cout << (late23_ok ? "[PASS] " : "[FAIL] ")
             << "23:00 + LateZi 使用真实次日农历日期\n";

        if (!late23_ok) {
            ++failed;
        }

        bool cross_month_ok =
            original_lunar.get_month() != next_lunar.get_month() &&
            late_zi_23.raw_lunar_month !=
                midnight_23.raw_lunar_month &&
            late_zi_23.resolved_lunar_month ==
                resolve_ziwei_month(
                    next_lunar.get_month(),
                    next_lunar.get_day(),
                    LeapMonthPolicy::SameAsRegularMonth
                );

        cout << (cross_month_ok ? "[PASS] " : "[FAIL] ")
             << "晚子换日正确跨越农历月边界\n";

        if (!cross_month_ok) {
            ++failed;
        }
    }

    if (failed != 0) {
        cerr << "\n❌ 晚子时换日策略测试失败数量: "
             << failed << '\n';
        return 1;
    }

    cout << "✅ 晚子时换日策略回归测试全部通过\n";


    cout << "\n===== Horoscope 六层聚合回归测试 =====\n";

    {
        auto chart = pai_pan_solar(
            1990, 6, 15, 10, true,
            LeapMonthPolicy::SameAsRegularMonth,
            ZiHourDayBoundaryPolicy::Midnight
        );

        auto horoscope = chart.get_horoscope(
            2026, 8, 21, 14, 37,
            LiuNianYearBoundaryPolicy::LunarNewYear,
            LiuYueGanZhiPolicy::LunarMonthWuHuDun,
            ZiHourDayBoundaryPolicy::Midnight
        );

        bool da_xian_ok =
            horoscope.da_xian.has_value() &&
            37 >= horoscope.da_xian->start_age &&
            37 <= horoscope.da_xian->end_age;

        cout << (da_xian_ok ? "[PASS] " : "[FAIL] ")
             << "正常虚岁可取得当前大限\n";

        if (!da_xian_ok) {
            ++failed;
        }

        bool six_layers_ok =
            horoscope.xiao_xian.age == 37 &&
            horoscope.liu_nian.gong_index >= 0 &&
            horoscope.liu_nian.gong_index < 12 &&
            horoscope.liu_yue.gong_index >= 0 &&
            horoscope.liu_yue.gong_index < 12 &&
            horoscope.liu_ri.gong_index >= 0 &&
            horoscope.liu_ri.gong_index < 12 &&
            horoscope.liu_shi.gong_index >= 0 &&
            horoscope.liu_shi.gong_index < 12;

        cout << (six_layers_ok ? "[PASS] " : "[FAIL] ")
             << "六层运限均生成有效宫位\n";

        if (!six_layers_ok) {
            ++failed;
        }
    }

    {
        auto chart = pai_pan_solar(
            1990, 6, 15, 10, true,
            LeapMonthPolicy::SameAsRegularMonth,
            ZiHourDayBoundaryPolicy::Midnight
        );

        int first_start_age = 999;

        for (const auto& dx : chart.da_xian_data) {
            first_start_age =
                std::min(first_start_age, dx.start_age);
        }

        int pre_start_age = first_start_age - 1;

        if (pre_start_age > 0) {
            auto horoscope = chart.get_horoscope(
                1990, 6, 15, 10, pre_start_age,
                LiuNianYearBoundaryPolicy::LunarNewYear,
                LiuYueGanZhiPolicy::LunarMonthWuHuDun,
                ZiHourDayBoundaryPolicy::Midnight
            );

            bool da_xian_stars_empty = true;
            for (const auto& gong : horoscope.da_xian_stars) {
                if (!gong.stars.empty()) {
                    da_xian_stars_empty = false;
                    break;
                }
            }

            bool pre_start_ok =
                !horoscope.da_xian.has_value() &&
                da_xian_stars_empty &&
                horoscope.xiao_xian.age == pre_start_age;

            cout << (pre_start_ok ? "[PASS] " : "[FAIL] ")
                 << "起运前大限与大限动态运耀均为空，其余运限仍可计算\n";

            if (!pre_start_ok) {
                ++failed;
            }
        } else {
            cout << "[PASS] 起运年龄为1，无起运前正虚岁可测\n";
        }
    }

    {
        // 2024-02-09 23:00：
        // Midnight 仍属腊月末，LateZi 按 2024-02-10 正月初一。
        auto chart = pai_pan_solar(
            1990, 6, 15, 10, true,
            LeapMonthPolicy::SameAsRegularMonth,
            ZiHourDayBoundaryPolicy::Midnight
        );

        auto midnight = chart.get_horoscope(
            2024, 2, 9, 23, 35,
            LiuNianYearBoundaryPolicy::LunarNewYear,
            LiuYueGanZhiPolicy::LunarMonthWuHuDun,
            ZiHourDayBoundaryPolicy::Midnight
        );

        auto late_zi = chart.get_horoscope(
            2024, 2, 9, 23, 35,
            LiuNianYearBoundaryPolicy::LunarNewYear,
            LiuYueGanZhiPolicy::LunarMonthWuHuDun,
            ZiHourDayBoundaryPolicy::LateZi
        );

        bool boundary_diff =
            midnight.liu_ri.day != late_zi.liu_ri.day ||
            midnight.liu_yue.gong_index != late_zi.liu_yue.gong_index ||
            midnight.liu_nian.year != late_zi.liu_nian.year;

        cout << (boundary_diff ? "[PASS] " : "[FAIL] ")
             << "23:00 Midnight / LateZi 在跨春节边界产生聚合差异\n";

        if (!boundary_diff) {
            ++failed;
        }

        bool late_zi_hour_ok =
            late_zi.liu_shi.di_zhi ==
                ZhouYi::GanZhi::DiZhi::Zi;

        cout << (late_zi_hour_ok ? "[PASS] " : "[FAIL] ")
             << "23:00 LateZi 仍保持子时地支\n";

        if (!late_zi_hour_ok) {
            ++failed;
        }
    }

    if (failed != 0) {
        cerr << "\n❌ Horoscope 六层聚合测试失败数量: "
             << failed << '\n';
        return 1;
    }

    cout << "✅ Horoscope 六层聚合回归测试全部通过\n";


    cout << "\n===== 流年动态流耀回归测试 =====\n";

    {
        auto yearly = get_horoscope_stars(
            ZhouYi::GanZhi::TianGan::Jia,
            ZhouYi::GanZhi::DiZhi::Zi,
            Scope::Yearly
        );

        auto has_star = [&](int gong_index, const string& name) {
            const auto& stars = yearly[gong_index].stars;
            return std::find(stars.begin(), stars.end(), name)
                != stars.end();
        };

        bool yearly_ok =
            has_star(11, "流魁") &&   // 丑
            has_star(5,  "流钺") &&   // 未
            has_star(0,  "流禄") &&   // 寅
            has_star(1,  "流羊") &&   // 卯
            has_star(11, "流陀") &&   // 丑
            has_star(0,  "流马") &&   // 寅
            has_star(1,  "流鸾") &&   // 卯
            has_star(7,  "流喜") &&   // 酉
            has_star(8,  "年解");     // 戌

        cout << (yearly_ok ? "[PASS] " : "[FAIL] ")
             << "甲子流年动态流耀位置符合现有年干/年支公式\n";

        if (!yearly_ok) {
            ++failed;
        }

        bool gong_index_ok = true;

        for (int i = 0; i < 12; ++i) {
            if (yearly[i].gong_index != i) {
                gong_index_ok = false;
                break;
            }
        }

        cout << (gong_index_ok ? "[PASS] " : "[FAIL] ")
             << "流耀结果宫位索引完整为0~11\n";

        if (!gong_index_ok) {
            ++failed;
        }
    }

    {
        auto no_unverified_stars = [](Scope scope) {
            auto data = get_horoscope_stars(
                ZhouYi::GanZhi::TianGan::Jia,
                ZhouYi::GanZhi::DiZhi::Zi,
                scope
            );

            for (const auto& gong : data) {
                if (!gong.stars.empty()) {
                    return false;
                }
            }

            return true;
        };

        auto decadal = get_horoscope_stars(
            ZhouYi::GanZhi::TianGan::Jia,
            ZhouYi::GanZhi::DiZhi::Zi,
            Scope::Decadal
        );

        auto has_decadal_star = [&](int gong_index, const string& name) {
            const auto& stars = decadal[gong_index].stars;
            return std::find(stars.begin(), stars.end(), name)
                != stars.end();
        };

        bool decadal_ok =
            has_decadal_star(11, "运魁") &&
            has_decadal_star(5,  "运钺") &&
            has_decadal_star(0,  "运禄") &&
            has_decadal_star(1,  "运羊") &&
            has_decadal_star(11, "运陀") &&
            has_decadal_star(0,  "运马") &&
            has_decadal_star(1,  "运鸾") &&
            has_decadal_star(7,  "运喜");

        cout << (decadal_ok ? "[PASS] " : "[FAIL] ")
             << "甲子大限动态运耀位置符合现有干支公式\n";

        if (!decadal_ok) {
            ++failed;
        }

        auto monthly_stars =
            get_horoscope_stars(
                ZhouYi::GanZhi::TianGan::Jia,
                ZhouYi::GanZhi::DiZhi::Zi,
                Scope::Monthly
            );

        auto has_monthly_star =
            [&](int gong_index, const string& star_name) {
                const auto& stars =
                    monthly_stars.at(gong_index).stars;

                return std::find(
                    stars.begin(),
                    stars.end(),
                    star_name
                ) != stars.end();
            };

        bool monthly_ok =
            has_monthly_star(11, "月魁") &&
            has_monthly_star(5,  "月钺") &&
            has_monthly_star(0,  "月禄") &&
            has_monthly_star(1,  "月羊") &&
            has_monthly_star(11, "月陀") &&
            has_monthly_star(1,  "月鸾") &&
            has_monthly_star(7,  "月喜");

        cout << (monthly_ok ? "[PASS] " : "[FAIL] ")
             << "甲子流月动态流耀：魁钺禄羊陀鸾喜位置正确\n";

        if (!monthly_ok) {
            ++failed;
        }

        bool suppressed_ok =
            no_unverified_stars(Scope::Daily) &&
            no_unverified_stars(Scope::Hourly);

        cout << (suppressed_ok ? "[PASS] " : "[FAIL] ")
             << "未审定的流日/流时动态流耀保持为空\n";

        if (!suppressed_ok) {
            ++failed;
        }
    }

    if (failed != 0) {
        cerr << "\n❌ 流年动态流耀测试失败数量: "
             << failed << '\n';
        return 1;
    }

    cout << "✅ 流年动态流耀回归测试全部通过\n";

    cout << "\n===== 宫干自化/飞化回归测试 =====\n";

    {
        array<pair<ZhouYi::GanZhi::TianGan, ZhouYi::GanZhi::DiZhi>, 12>
            gong_gan_zhi{};

        for (int i = 0; i < 12; ++i) {
            gong_gan_zhi[i] = {
                ZhouYi::GanZhi::TianGan::Jia,
                ZhouYi::GanZhi::DiZhi::Zi
            };
        }

        array<vector<string>, 12> stars_in_gong{};
        stars_in_gong[0].push_back("廉贞");
        stars_in_gong[1].push_back("破军");
        stars_in_gong[2].push_back("武曲");
        stars_in_gong[3].push_back("太阳");

        SiHuaSystem si_hua_system(
            gong_gan_zhi,
            stars_in_gong
        );

        const auto& gong0 =
            si_hua_system.get_gong_gan_si_hua(0);

        bool gong_gan_table_ok =
            gong0.gong_gan == ZhouYi::GanZhi::TianGan::Jia &&
            gong0.si_hua_list[0].star_name == "廉贞" &&
            gong0.si_hua_list[0].gong_index == 0 &&
            gong0.si_hua_list[1].star_name == "破军" &&
            gong0.si_hua_list[1].gong_index == 1 &&
            gong0.si_hua_list[2].star_name == "武曲" &&
            gong0.si_hua_list[2].gong_index == 2 &&
            gong0.si_hua_list[3].star_name == "太阳" &&
            gong0.si_hua_list[3].gong_index == 3;

        cout << (gong_gan_table_ok ? "[PASS] " : "[FAIL] ")
             << "甲干宫干四化：廉贞禄、破军权、武曲科、太阳忌\n";

        if (!gong_gan_table_ok) {
            ++failed;
        }

        bool zi_hua_ok =
            si_hua_system.has_zi_hua(0) &&
            si_hua_system.has_zi_hua_type(
                0,
                static_cast<SiHua>(0)
            );

        cout << (zi_hua_ok ? "[PASS] " : "[FAIL] ")
             << "甲干0宫廉贞在本宫，正确判定自化禄\n";

        if (!zi_hua_ok) {
            ++failed;
        }

        bool flies_ok =
            si_hua_system.flies_to(
                0, 0, static_cast<SiHua>(0)
            ) &&
            si_hua_system.flies_to(
                0, 1, static_cast<SiHua>(1)
            ) &&
            si_hua_system.flies_to(
                0, 2, static_cast<SiHua>(2)
            ) &&
            si_hua_system.flies_to(
                0, 3, static_cast<SiHua>(3)
            ) &&
            !si_hua_system.flies_to(
                0, 3, static_cast<SiHua>(0)
            );

        cout << (flies_ok ? "[PASS] " : "[FAIL] ")
             << "飞化 from/to 宫位与禄权科忌方向正确\n";

        if (!flies_ok) {
            ++failed;
        }

        auto from0 =
            si_hua_system.get_fei_hua_from(0);

        bool from_ok = from0.size() == 4;

        if (from_ok) {
            for (int type = 0; type < 4; ++type) {
                bool found = false;

                for (const auto& relation : from0) {
                    if (
                        relation.from_gong == 0 &&
                        relation.from_gan ==
                            ZhouYi::GanZhi::TianGan::Jia &&
                        relation.to_gong == type &&
                        relation.si_hua_type ==
                            static_cast<SiHua>(type)
                    ) {
                        found = true;
                        break;
                    }
                }

                if (!found) {
                    from_ok = false;
                    break;
                }
            }
        }

        cout << (from_ok ? "[PASS] " : "[FAIL] ")
             << "get_fei_hua_from 保持来源宫、目标宫和四化类型一致\n";

        if (!from_ok) {
            ++failed;
        }

        auto to3 =
            si_hua_system.get_fei_hua_to(3);

        bool to_ok = false;

        for (const auto& relation : to3) {
            if (
                relation.from_gong == 0 &&
                relation.to_gong == 3 &&
                relation.si_hua_type ==
                    static_cast<SiHua>(3) &&
                relation.star_name == "太阳"
            ) {
                to_ok = true;
                break;
            }
        }

        cout << (to_ok ? "[PASS] " : "[FAIL] ")
             << "get_fei_hua_to 正确识别0宫太阳化忌飞入3宫\n";

        if (!to_ok) {
            ++failed;
        }
    }

    if (failed != 0) {
        cerr << "\n❌ 宫干自化/飞化测试失败数量: "
             << failed << '\n';
        return 1;
    }

    cout << "✅ 宫干自化/飞化回归测试全部通过\n";

    cout << "\n===== 多层飞化链回归测试 =====\n";

    {
        array<pair<ZhouYi::GanZhi::TianGan, ZhouYi::GanZhi::DiZhi>, 12>
            gong_gan_zhi{};

        for (int i = 0; i < 12; ++i) {
            gong_gan_zhi[i] = {
                ZhouYi::GanZhi::TianGan::Gui,
                ZhouYi::GanZhi::DiZhi::Zi
            };
        }

        gong_gan_zhi[0].first = ZhouYi::GanZhi::TianGan::Jia;
        gong_gan_zhi[1].first = ZhouYi::GanZhi::TianGan::Yi;
        gong_gan_zhi[2].first = ZhouYi::GanZhi::TianGan::Bing;

        array<vector<string>, 12> stars_in_gong{};

        // 甲干化禄廉贞 -> 1宫
        stars_in_gong[1].push_back("廉贞");

        // 乙干化禄天机 -> 2宫
        stars_in_gong[2].push_back("天机");

        // 丙干化禄天同 -> 0宫
        stars_in_gong[0].push_back("天同");

        SiHuaSystem chain_system(
            gong_gan_zhi,
            stars_in_gong
        );

        auto chains_depth3 =
            chain_system.get_fei_hua_chains(
                0,
                static_cast<SiHua>(0),
                3
            );

        bool has_len1 = false;
        bool has_len2 = false;
        bool has_hui_ben_len3 = false;

        for (const auto& chain : chains_depth3) {
            if (chain.chain.size() == 1) {
                const auto& r0 = chain.chain[0];
                if (
                    r0.from_gong == 0 &&
                    r0.to_gong == 1 &&
                    !chain.is_hui_ben
                ) {
                    has_len1 = true;
                }
            }

            if (chain.chain.size() == 2) {
                const auto& r0 = chain.chain[0];
                const auto& r1 = chain.chain[1];

                if (
                    r0.from_gong == 0 &&
                    r0.to_gong == 1 &&
                    r1.from_gong == 1 &&
                    r1.to_gong == 2 &&
                    !chain.is_hui_ben
                ) {
                    has_len2 = true;
                }
            }

            if (chain.chain.size() == 3) {
                const auto& r0 = chain.chain[0];
                const auto& r1 = chain.chain[1];
                const auto& r2 = chain.chain[2];

                if (
                    r0.from_gong == 0 &&
                    r0.to_gong == 1 &&
                    r1.from_gong == 1 &&
                    r1.to_gong == 2 &&
                    r2.from_gong == 2 &&
                    r2.to_gong == 0 &&
                    chain.is_hui_ben
                ) {
                    has_hui_ben_len3 = true;
                }
            }
        }

        bool chain_ok =
            has_len1 &&
            has_len2 &&
            has_hui_ben_len3;

        cout << (chain_ok ? "[PASS] " : "[FAIL] ")
             << "化禄链 0→1→2→0 正确生成1/2/3层前缀与回本链\n";

        if (!chain_ok) {
            ++failed;
        }

        auto chains_depth2 =
            chain_system.get_fei_hua_chains(
                0,
                static_cast<SiHua>(0),
                2
            );

        bool no_hui_ben_at_depth2 = true;

        for (const auto& chain : chains_depth2) {
            if (chain.is_hui_ben || chain.chain.size() > 2) {
                no_hui_ben_at_depth2 = false;
                break;
            }
        }

        cout << (no_hui_ben_at_depth2 ? "[PASS] " : "[FAIL] ")
             << "max_depth=2 不会越界生成第3层回本链\n";

        if (!no_hui_ben_at_depth2) {
            ++failed;
        }

        auto hui_ben =
            chain_system.find_hui_ben_chains(0);

        bool find_hui_ben_ok = false;

        for (const auto& chain : hui_ben) {
            if (
                chain.is_hui_ben &&
                chain.chain.size() == 3 &&
                chain.chain[0].si_hua_type ==
                    static_cast<SiHua>(0) &&
                chain.chain[0].from_gong == 0 &&
                chain.chain[2].to_gong == 0
            ) {
                find_hui_ben_ok = true;
                break;
            }
        }

        cout << (find_hui_ben_ok ? "[PASS] " : "[FAIL] ")
             << "find_hui_ben_chains 正确找到三层化禄回本链\n";

        if (!find_hui_ben_ok) {
            ++failed;
        }

        bool invalid_depth_ok = false;

        try {
            (void)chain_system.get_fei_hua_chains(
                0,
                static_cast<SiHua>(0),
                0
            );
        } catch (const invalid_argument&) {
            invalid_depth_ok = true;
        }

        cout << (invalid_depth_ok ? "[PASS] " : "[FAIL] ")
             << "max_depth=0 正确拒绝非法参数\n";

        if (!invalid_depth_ok) {
            ++failed;
        }
    }

    if (failed != 0) {
        cerr << "\n❌ 多层飞化链测试失败数量: "
             << failed << '\n';
        return 1;
    }

    cout << "✅ 多层飞化链回归测试全部通过\n";

    cout << "\n===== 虚岁换岁边界回归测试 =====\n";

    {
        constexpr int birth_lunar_year = 1990;

        auto before_lny =
            tyme::SolarDay::from_ymd(2026, 2, 16)
                .get_lunar_day();

        auto after_lny =
            tyme::SolarDay::from_ymd(2026, 2, 17)
                .get_lunar_day();

        int age_before = calculate_virtual_age(
            birth_lunar_year,
            before_lny.get_year(),
            VirtualAgeBoundaryPolicy::LunarNewYear
        );

        int age_after = calculate_virtual_age(
            birth_lunar_year,
            after_lny.get_year(),
            VirtualAgeBoundaryPolicy::LunarNewYear
        );

        bool virtual_age_ok =
            age_after == age_before + 1;

        cout << (virtual_age_ok ? "[PASS] " : "[FAIL] ")
             << "农历正月初一正确增加一岁虚岁\n";

        if (!virtual_age_ok) {
            ++failed;
        }
    }

    if (failed != 0) {
        cerr << "\n❌ 虚岁换岁边界测试失败数量: "
             << failed << '\n';
        return 1;
    }

    cout << "✅ 虚岁换岁边界回归测试全部通过\n";

    cout << "\n===== 全时域分析数据集结构回归测试 =====\n";

    {
        auto dataset_result = pai_pan_solar(
            1990, 6, 15, 10, true
        );

        AnalysisDatasetOptions options;
        options.years_before = 0;
        options.years_after = 0;
        options.include_hours = true;
        options.year_boundary_policy =
            LiuNianYearBoundaryPolicy::LunarNewYear;
        options.month_gan_zhi_policy =
            LiuYueGanZhiPolicy::LunarMonthWuHuDun;
        options.day_boundary_policy =
            ZiHourDayBoundaryPolicy::Midnight;
        options.virtual_age_boundary_policy =
            VirtualAgeBoundaryPolicy::LunarNewYear;

        auto text = export_analysis_dataset(
            dataset_result,
            2026,
            options
        );

        auto j = nlohmann::json::parse(text);

        bool top_level_ok =
            j.contains("schema") &&
            j.contains("range") &&
            j.contains("rule_metadata") &&
            j.contains("natal") &&
            j.contains("timeline") &&
            j["range"]["year_count"] == 1 &&
            j["timeline"].is_array() &&
            j["timeline"].size() == 1;

        cout << (top_level_ok ? "[PASS] " : "[FAIL] ")
             << "analysis dataset 顶层结构完整\n";

        if (!top_level_ok) {
            ++failed;
        }

        bool year_ok =
            j["timeline"][0]["solar_year"] == 2026 &&
            j["timeline"][0]["days"].is_array() &&
            (
                j["timeline"][0]["days"].size() == 365 ||
                j["timeline"][0]["days"].size() == 366
            );

        cout << (year_ok ? "[PASS] " : "[FAIL] ")
             << "单年时间轴包含完整公历日\n";

        if (!year_ok) {
            ++failed;
        }

        bool day_ok = false;
        bool hours_ok = false;
        bool dynamic_ok = false;

        for (const auto& day : j["timeline"][0]["days"]) {
            if (
                day.value("status", "") == "before_birth"
            ) {
                continue;
            }

            day_ok =
                day.contains("da_xian") &&
                day.contains("xiao_xian") &&
                day.contains("liu_nian") &&
                day.contains("liu_yue") &&
                day.contains("liu_ri");

            hours_ok =
                day.contains("hours") &&
                day["hours"].is_array() &&
                day["hours"].size() == 24;

            dynamic_ok =
                day.contains("dynamic_stars") &&
                day["dynamic_stars"].contains("da_xian") &&
                day["dynamic_stars"].contains("liu_nian") &&
                day["dynamic_stars"].contains("liu_yue") &&
                day["dynamic_stars"].contains("liu_ri") &&
                day["dynamic_stars"].contains("liu_shi");

            break;
        }

        cout << (day_ok ? "[PASS] " : "[FAIL] ")
             << "每日六层上下文结构存在\n";
        if (!day_ok) {
            ++failed;
        }

        cout << (hours_ok ? "[PASS] " : "[FAIL] ")
             << "每日保留24个公历小时节点\n";
        if (!hours_ok) {
            ++failed;
        }

        cout << (dynamic_ok ? "[PASS] " : "[FAIL] ")
             << "大限/流年动态流耀与未审定层状态结构存在\n";
        if (!dynamic_ok) {
            ++failed;
        }

        bool metadata_ok =
            j["rule_metadata"]
                .contains("leap_month_policy") &&
            j["rule_metadata"]
                .contains("natal_zi_hour_day_boundary_policy") &&
            j["rule_metadata"]
                .contains("timeline_day_boundary_policy") &&
            j["rule_metadata"]
                .contains("virtual_age_boundary_policy") &&
            j["rule_metadata"]
                .contains("liu_nian_year_boundary_policy") &&
            j["rule_metadata"]
                .contains("liu_yue_gan_zhi_policy") &&
            j["rule_metadata"]
                .contains("huo_ling_policy") &&
            j["rule_metadata"]
                .contains("main_star_brightness_policy") &&
            j["rule_metadata"]
                .contains("dynamic_stars");

        cout << (metadata_ok ? "[PASS] " : "[FAIL] ")
             << "规则策略与未审定状态元数据完整\n";

        if (!metadata_ok) {
            ++failed;
        }
    }

    if (failed != 0) {
        cerr << "\n❌ 全时域分析数据集测试失败数量: "
             << failed << '\n';
        return 1;
    }

    cout << "✅ 全时域分析数据集结构回归测试全部通过\n";

    cout << "\n===== 最终本命 JSON 完整性回归测试 =====\n";

    {
        auto natal_result = pai_pan_solar(
            1990, 6, 15, 10, true
        );

        auto text = export_to_json_full(
            natal_result
        );

        auto j = nlohmann::json::parse(text);

        bool palace_count_ok =
            j.contains("palaces") &&
            j["palaces"].is_array() &&
            j["palaces"].size() == 12;

        cout << (palace_count_ok ? "[PASS] " : "[FAIL] ")
             << "本命十二宫完整输出\n";

        if (!palace_count_ok) {
            ++failed;
        }

        bool palace_structure_ok = true;

        if (palace_count_ok) {
            for (const auto& p : j["palaces"]) {
                if (
                    !p.contains("stars") ||
                    !p["stars"].contains("zhu_xing") ||
                    !p["stars"].contains("fu_xing") ||
                    !p["stars"].contains("sha_xing") ||
                    !p["stars"].contains("za_yao") ||
                    !p.contains("twelve_gods") ||
                    !p.contains("limits")
                ) {
                    palace_structure_ok = false;
                    break;
                }
            }
        } else {
            palace_structure_ok = false;
        }

        cout << (palace_structure_ok ? "[PASS] " : "[FAIL] ")
             << "每宫四类星曜、十二神与运限缓存结构完整\n";

        if (!palace_structure_ok) {
            ++failed;
        }

        bool brightness_null_semantics_ok = true;

        if (palace_count_ok) {
            for (const auto& p : j["palaces"]) {
                for (const char* category : {
                    "fu_xing", "sha_xing", "za_yao"
                }) {
                    for (const auto& star :
                         p["stars"][category]) {

                        if (
                            !star.contains("liang_du") ||
                            !star["liang_du"].is_null()
                        ) {
                            brightness_null_semantics_ok =
                                false;
                            break;
                        }
                    }

                    if (!brightness_null_semantics_ok) {
                        break;
                    }
                }

                if (!brightness_null_semantics_ok) {
                    break;
                }
            }
        }

        cout << (
            brightness_null_semantics_ok
                ? "[PASS] "
                : "[FAIL] "
        )
             << "非主星亮度保持 null，不伪造平旺陷\n";

        if (!brightness_null_semantics_ok) {
            ++failed;
        }

        bool si_hua_ok =
            j.contains("si_hua") &&
            j["si_hua"].contains("natal") &&
            j["si_hua"].contains("gong_gan_si_hua") &&
            j["si_hua"].contains("zi_hua") &&
            j["si_hua"].contains("fei_hua_relations") &&
            j["si_hua"].contains("fei_hua_chains") &&
            j["si_hua"].contains("hui_ben_chains");

        cout << (si_hua_ok ? "[PASS] " : "[FAIL] ")
             << "本命四化、自化、直接飞化与多层飞化链结构完整\n";

        if (!si_hua_ok) {
            ++failed;
        }

        bool san_fang_ok =
            j.contains("san_fang_si_zheng") &&
            j["san_fang_si_zheng"].is_array() &&
            j["san_fang_si_zheng"].size() == 12;

        cout << (san_fang_ok ? "[PASS] " : "[FAIL] ")
             << "十二宫三方四正完整输出\n";

        if (!san_fang_ok) {
            ++failed;
        }

        bool da_xian_ok =
            j.contains("da_xian") &&
            j["da_xian"].is_array() &&
            j["da_xian"].size() == 12;

        cout << (da_xian_ok ? "[PASS] " : "[FAIL] ")
             << "十二大限完整输出\n";

        if (!da_xian_ok) {
            ++failed;
        }

        bool ge_ju_ok =
            j.contains("ge_ju") &&
            j["ge_ju"].contains("ji_ge") &&
            j["ge_ju"].contains("xiong_ge") &&
            j["ge_ju"].contains("total_score");

        cout << (ge_ju_ok ? "[PASS] " : "[FAIL] ")
             << "格局分析结构完整\n";

        if (!ge_ju_ok) {
            ++failed;
        }

        bool kong_gong_ok =
            j.contains("kong_gong") &&
            j["kong_gong"].is_array();

        cout << (kong_gong_ok ? "[PASS] " : "[FAIL] ")
             << "空宫借星结构存在\n";

        if (!kong_gong_ok) {
            ++failed;
        }
    }

    if (failed != 0) {
        cerr << "\n❌ 最终本命 JSON 完整性测试失败数量: "
             << failed << '\n';
        return 1;
    }

    cout << "✅ 最终本命 JSON 完整性回归测试全部通过\n";

    return 0;
}
