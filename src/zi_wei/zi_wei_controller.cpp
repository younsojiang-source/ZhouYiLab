// 紫微斗数控制器实现
module ZhouYi.ZiWei.Controller;

import ZhouYi.GanZhi;
import ZhouYi.ZiWei;
import ZhouYi.ZiWei.Constants;
import ZhouYi.ZiWei.Star;
import ZhouYi.ZiWei.SiHua;
import ZhouYi.ZiWei.SanFang;
import ZhouYi.ZiWei.GeJu;
import ZhouYi.ZiWei.StarDocument;
import ZhouYi.ZiWei.Horoscope;
import ZhouYi.ZhMapper;
import ZhouYi.tyme;
import fmt;
import nlohmann.json;
import std;

using json = nlohmann::json;

namespace ZhouYi::ZiWei {
    using namespace std;
    using namespace ZhouYi::GanZhi;
    using namespace ZhouYi::Mapper;

    void pai_pan_and_print_solar(int year, int month, int day, int hour, bool is_male) {
        try {
            auto result = pai_pan_solar(year, month, day, hour, is_male);
            fmt::print("{}\n", result.to_string());
        } catch (const exception& e) {
            fmt::print("[错误] 排盘错误: {}\n", e.what());
        }
    }

    void pai_pan_and_print_lunar(int year, int month, int day, int hour, 
                                  bool is_male, bool is_leap_month) {
        try {
            auto result = pai_pan_lunar(year, month, day, hour, is_male, is_leap_month);
            fmt::print("{}\n", result.to_string());
        } catch (const exception& e) {
            fmt::print("[错误] 排盘错误: {}\n", e.what());
        }
    }

    void display_palace_detail(const ZiWeiResult& result, GongWei gong_wei) {
        try {
            const auto& palace = result.get_palace(gong_wei);
            fmt::print("\n{}\n", palace.to_string());
        } catch (const exception& e) {
            fmt::print("[错误] 获取宫位错误: {}\n", e.what());
        }
    }

    void display_ming_gong_san_fang_si_zheng(const ZiWeiResult& result) {
        fmt::print("\n\n");
        fmt::print("       命宫三方四正\n");
        fmt::print("\n\n");
        
        // 命宫
        fmt::print("【命宫】\n");
        display_palace_detail(result, GongWei::MingGong);
        
        // 财帛宫（三方）
        fmt::print("\n【财帛宫（三方）】\n");
        display_palace_detail(result, GongWei::CaiBoGong);
        
        // 官禄宫（三方）
        fmt::print("\n【官禄宫（三方）】\n");
        display_palace_detail(result, GongWei::GuanLuGong);
        
        // 迁移宫（对宫，四正）
        fmt::print("\n【迁移宫（对宫）】\n");
        display_palace_detail(result, GongWei::QianYiGong);
    }

    string export_to_json(const ZiWeiResult& result) {
        json j;

        auto star_to_json = [](const StarData& star) {
            json sj;

            sj["name"] = star.name;
            sj["gong_index"] = star.gong_index;

            if (star.liang_du.has_value()) {
                sj["liang_du"] =
                    string(to_zh(*star.liang_du));
            } else {
                sj["liang_du"] = nullptr;
            }

            if (star.si_hua.has_value()) {
                sj["si_hua"] =
                    string(to_zh(*star.si_hua));
            } else {
                sj["si_hua"] = nullptr;
            }

            return sj;
        };

        // ----------------------------------------------------
        // 基本历法与身份信息
        // ----------------------------------------------------
        j["solar_date"] = result.solar_day.to_string();
        j["lunar_date"] = result.lunar_day.to_string();
        j["lunar_hour"] = result.lunar_hour.to_string();
        j["gender"] = result.is_male ? "男" : "女";

        j["calendar_resolution"] = {
            {"raw_lunar_month", result.raw_lunar_month},
            {"resolved_lunar_month", result.resolved_lunar_month},
            {"resolved_lunar_day", result.resolved_lunar_day},
            {"is_leap_month", result.is_leap_month},
            {
                "zi_hour_shifted_to_next_day",
                result.zi_hour_shifted_to_next_day
            }
        };

        // ----------------------------------------------------
        // 四柱
        // ----------------------------------------------------
        j["si_zhu"]["year"] =
            result.year_pillar.to_string();

        j["si_zhu"]["month"] =
            result.month_pillar.to_string();

        j["si_zhu"]["day"] =
            result.day_pillar.to_string();

        j["si_zhu"]["hour"] =
            result.hour_pillar.to_string();

        // ----------------------------------------------------
        // 本命核心
        // ----------------------------------------------------
        j["wu_xing_ju"] =
            string(to_zh(result.wu_xing_ju));

        j["ming_gong_index"] =
            result.ming_gong_index;

        j["shen_gong_index"] =
            result.shen_gong_index;

        j["ming_zhu"] =
            result.ming_zhu_xing;

        j["shen_zhu"] =
            result.shen_zhu_xing;

        // ----------------------------------------------------
        // 十二宫完整数据
        // ----------------------------------------------------
        j["palaces"] = json::array();

        for (const auto& palace : result.palaces) {
            json pj;

            pj["index"] =
                palace.gong_data.index;

            pj["name"] =
                string(to_zh(
                    palace.gong_data.gong_wei
                ));

            pj["gan"] =
                string(
                    GanZhi::Mapper::to_zh(
                        palace.gong_data.tian_gan
                    )
                );

            pj["zhi"] =
                string(
                    GanZhi::Mapper::to_zh(
                        palace.gong_data.di_zhi
                    )
                );

            pj["gan_zhi"] =
                fmt::format(
                    "{}{}",
                    string(
                        GanZhi::Mapper::to_zh(
                            palace.gong_data.tian_gan
                        )
                    ),
                    string(
                        GanZhi::Mapper::to_zh(
                            palace.gong_data.di_zhi
                        )
                    )
                );

            pj["is_ming_palace"] =
                palace.gong_data.is_ming_palace;

            pj["is_body_palace"] =
                palace.gong_data.is_body_palace;

            pj["stars"] = {
                {"zhu_xing", json::array()},
                {"fu_xing", json::array()},
                {"sha_xing", json::array()},
                {"za_yao", json::array()}
            };

            for (const auto& star : palace.zhu_xing) {
                pj["stars"]["zhu_xing"]
                    .push_back(star_to_json(star));
            }

            for (const auto& star : palace.fu_xing) {
                pj["stars"]["fu_xing"]
                    .push_back(star_to_json(star));
            }

            for (const auto& star : palace.sha_xing) {
                pj["stars"]["sha_xing"]
                    .push_back(star_to_json(star));
            }

            for (const auto& star : palace.za_yao) {
                pj["stars"]["za_yao"]
                    .push_back(star_to_json(star));
            }

            // 为兼容旧消费者，继续保留原分类字段，
            // 但现在也统一为完整 StarData JSON。
            pj["zhu_xing"] =
                pj["stars"]["zhu_xing"];

            pj["fu_xing"] =
                pj["stars"]["fu_xing"];

            pj["sha_xing"] =
                pj["stars"]["sha_xing"];

            pj["za_yao"] =
                pj["stars"]["za_yao"];

            // ------------------------------------------------
            // 十二神
            // ------------------------------------------------
            pj["twelve_gods"] = json::object();

            if (palace.chang_sheng.has_value()) {
                pj["twelve_gods"]["chang_sheng"] =
                    string(to_zh(*palace.chang_sheng));
            } else {
                pj["twelve_gods"]["chang_sheng"] =
                    nullptr;
            }

            if (palace.bo_shi.has_value()) {
                pj["twelve_gods"]["bo_shi"] =
                    string(to_zh(*palace.bo_shi));
            } else {
                pj["twelve_gods"]["bo_shi"] =
                    nullptr;
            }

            if (palace.sui_qian.has_value()) {
                pj["twelve_gods"]["sui_qian"] =
                    string(to_zh(*palace.sui_qian));
            } else {
                pj["twelve_gods"]["sui_qian"] =
                    nullptr;
            }

            if (palace.jiang_qian.has_value()) {
                pj["twelve_gods"]["jiang_qian"] =
                    string(to_zh(*palace.jiang_qian));
            } else {
                pj["twelve_gods"]["jiang_qian"] =
                    nullptr;
            }

            // ------------------------------------------------
            // 运限缓存
            // ------------------------------------------------
            pj["limits"] = {
                {"da_xian_start_age", palace.da_xian_start},
                {"da_xian_end_age", palace.da_xian_end},
                {"xiao_xian_ages", palace.xiao_xian_ages},
                {"liu_nian_ages", palace.liu_nian_ages}
            };

            j["palaces"].push_back(pj);
        }

        return j.dump(2);
    }

    // ============= 辅助函数：准备数据 =============
    
    namespace {
        // 准备宫干地支数据
        array<pair<TianGan, DiZhi>, 12> prepare_gong_gan_zhi(const ZiWeiResult& result) {
            array<pair<TianGan, DiZhi>, 12> gong_gan_zhi;
            for (int i = 0; i < 12; ++i) {
                gong_gan_zhi[i] = {
                    result.palaces[i].gong_data.tian_gan,
                    result.palaces[i].gong_data.di_zhi
                };
            }
            return gong_gan_zhi;
        }
        
        // 准备星耀列表
        array<vector<string>, 12> prepare_stars_in_gong(const ZiWeiResult& result) {
            array<vector<string>, 12> stars_in_gong;
            for (int i = 0; i < 12; ++i) {
                for (const auto& star : result.palaces[i].zhu_xing) {
                    stars_in_gong[i].push_back(star.name);
                }
                for (const auto& star : result.palaces[i].fu_xing) {
                    stars_in_gong[i].push_back(star.name);
                }
                for (const auto& star : result.palaces[i].sha_xing) {
                    stars_in_gong[i].push_back(star.name);
                }
                for (const auto& star : result.palaces[i].za_yao) {
                    stars_in_gong[i].push_back(star.name);
                }
            }
            return stars_in_gong;
        }
        
        // 准备地支列表
        array<DiZhi, 12> prepare_gong_di_zhi(const ZiWeiResult& result) {
            array<DiZhi, 12> gong_di_zhi;
            for (int i = 0; i < 12; ++i) {
                gong_di_zhi[i] = result.palaces[i].gong_data.di_zhi;
            }
            return gong_di_zhi;
        }
    }

    // ============= 四化分析功能实现 =============
    
    void display_gong_gan_si_hua(const ZiWeiResult& result) {
        fmt::print("\n\n");
        fmt::print("       宫干四化分析\n");
        fmt::print("\n\n");
        
        auto gong_gan_zhi = prepare_gong_gan_zhi(result);
        auto stars_in_gong = prepare_stars_in_gong(result);
        
        SiHuaSystem si_hua_system(gong_gan_zhi, stars_in_gong);
        
        for (const auto& gong_si_hua : si_hua_system.get_all_gong_gan_si_hua()) {
            fmt::print("{}\n", gong_si_hua.to_string());
        }
    }
    
    void display_zi_hua_analysis(const ZiWeiResult& result) {
        fmt::print("\n\n");
        fmt::print("       自化分析\n");
        fmt::print("\n\n");
        
        auto gong_gan_zhi = prepare_gong_gan_zhi(result);
        auto stars_in_gong = prepare_stars_in_gong(result);
        
        SiHuaSystem si_hua_system(gong_gan_zhi, stars_in_gong);
        auto zi_hua_list = si_hua_system.get_all_zi_hua();
        
        if (zi_hua_list.empty()) {
            fmt::print("无自化现象\n");
        } else {
            for (const auto& zi_hua : zi_hua_list) {
                fmt::print("{}\n", zi_hua.to_string());
            }
        }
    }
    
    void display_fei_hua_analysis(const ZiWeiResult& result, int from_gong, SiHua si_hua_type) {
        fmt::print("\n\n");
        fmt::print("       第{}宫 {} 飞化链分析\n", from_gong, string(to_zh(si_hua_type)));
        fmt::print("\n\n");
        
        auto gong_gan_zhi = prepare_gong_gan_zhi(result);
        auto stars_in_gong = prepare_stars_in_gong(result);
        
        SiHuaSystem si_hua_system(gong_gan_zhi, stars_in_gong);
        auto chains = si_hua_system.get_fei_hua_chains(from_gong, si_hua_type, 4);
        
        if (chains.empty()) {
            fmt::print("无飞化链\n");
        } else {
            for (const auto& chain : chains) {
                fmt::print("{}\n", chain.to_string());
            }
        }
    }

    // ============= 格局分析功能实现 =============
    
    void display_ge_ju_analysis(const ZiWeiResult& result) {
        fmt::print("\n\n");
        fmt::print("       格局分析\n");
        fmt::print("\n\n");
        
        auto stars_in_gong = prepare_stars_in_gong(result);
        auto gong_di_zhi = prepare_gong_di_zhi(result);
        
        GeJuAnalyzer analyzer(stars_in_gong, gong_di_zhi, result.ming_gong_index);
        
        auto ji_ge = analyzer.analyze_ji_ge();
        auto xiong_ge = analyzer.analyze_xiong_ge();
        int total_score = analyzer.get_total_score();
        
        fmt::print("【命盘总分】：{}\n\n", total_score);
        
        if (!ji_ge.empty()) {
            fmt::print("【吉格】（共{}个）\n", ji_ge.size());
            for (const auto& geju : ji_ge) {
                fmt::print("  {}\n", geju.to_string());
            }
            fmt::print("\n");
        }
        
        if (!xiong_ge.empty()) {
            fmt::print("【凶格】（共{}个）\n", xiong_ge.size());
            for (const auto& geju : xiong_ge) {
                fmt::print("  {}\n", geju.to_string());
            }
        }
    }
    
    void display_ji_ge(const ZiWeiResult& result) {
        auto stars_in_gong = prepare_stars_in_gong(result);
        auto gong_di_zhi = prepare_gong_di_zhi(result);
        
        GeJuAnalyzer analyzer(stars_in_gong, gong_di_zhi, result.ming_gong_index);
        auto ji_ge = analyzer.analyze_ji_ge();
        
        fmt::print("\n【吉格列表】（共{}个）\n", ji_ge.size());
        for (const auto& geju : ji_ge) {
            fmt::print("{}\n", geju.to_string());
        }
    }
    
    void display_xiong_ge(const ZiWeiResult& result) {
        auto stars_in_gong = prepare_stars_in_gong(result);
        auto gong_di_zhi = prepare_gong_di_zhi(result);
        
        GeJuAnalyzer analyzer(stars_in_gong, gong_di_zhi, result.ming_gong_index);
        auto xiong_ge = analyzer.analyze_xiong_ge();
        
        fmt::print("\n【凶格列表】（共{}个）\n", xiong_ge.size());
        for (const auto& geju : xiong_ge) {
            fmt::print("{}\n", geju.to_string());
        }
    }
    
    void display_shuang_xing_zu_he(const ZiWeiResult& result) {
        auto stars_in_gong = prepare_stars_in_gong(result);
        auto gong_di_zhi = prepare_gong_di_zhi(result);
        
        GeJuAnalyzer analyzer(stars_in_gong, gong_di_zhi, result.ming_gong_index);
        auto shuang_xing = analyzer.analyze_shuang_xing();
        
        fmt::print("\n【双星组合】（共{}组）\n", shuang_xing.size());
        for (const auto& zu_he : shuang_xing) {
            fmt::print("{}\n", zu_he.to_string());
        }
    }

    // ============= 三方四正分析功能实现 =============
    
    void display_san_fang_si_zheng(const ZiWeiResult& result, GongWei gong_wei) {
        int gong_index = result.get_palace(gong_wei).gong_data.index;
        auto san_fang = get_san_fang_si_zheng(gong_index);
        
        fmt::print("\n\n");
        fmt::print("       {} 三方四正\n", string(to_zh(gong_wei)));
        fmt::print("\n\n");
        
        fmt::print("{}\n\n", san_fang.to_string());
        
        for (int idx : san_fang.get_all_indices()) {
            const auto& palace = result.palaces[idx];
            fmt::print("{}\n\n", palace.to_string());
        }
    }
    
    void display_kong_gong_jie_xing(const ZiWeiResult& result) {
        auto stars_in_gong = prepare_stars_in_gong(result);
        
        SanFangAnalyzer analyzer(stars_in_gong);
        auto kong_gong_list = analyzer.get_all_kong_gong();
        
        fmt::print("\n【空宫借星】（共{}个）\n", kong_gong_list.size());
        if (kong_gong_list.empty()) {
            fmt::print("无空宫\n");
        } else {
            for (const auto& kong_gong : kong_gong_list) {
                fmt::print("{}\n", kong_gong.to_string());
            }
        }
    }

    // ============= 星耀特性查询功能实现 =============
    
    void display_star_info(const string& star_name) {
        auto doc = get_star_document(star_name);
        if (doc.has_value()) {
            fmt::print("\n{}\n", doc->to_string());
        } else {
            fmt::print("\n未找到星曜「{}」的详细信息\n", star_name);
        }
    }
    
    void display_tao_hua_xing() {
        // 交际类杂耀包含桃花星
        auto tao_hua_list = StarDoc::get_za_yao_by_category(ZaYaoCategory::JiaoJi);
        fmt::print("\n【桃花星列表】：{}\n", fmt::join(tao_hua_list, "、"));
    }
    
    void display_cai_xing() {
        // 财星主要是禄存、天马（二助星）
        auto cai_xing_list = StarDoc::get_fu_xing_by_category(FuXingCategory::ErZhu);
        fmt::print("\n【财星列表】：{}\n", fmt::join(cai_xing_list, "、"));
    }

    // ============= 运限分析功能实现 =============
    
    void display_da_xian_analysis(const ZiWeiResult& result) {
        fmt::print("\n\n");
        fmt::print("       大限分析\n");
        fmt::print("\n\n");
        
        for (int i = 0; i < 12; ++i) {
            const auto& da_xian = result.da_xian_data[i];
            fmt::print("第{}限：{}岁-{}岁 - 大限宫位：第{}宫 {}\n",
                i + 1,
                da_xian.start_age,
                da_xian.end_age,
                da_xian.gong_index,
                string(to_zh(result.palaces[da_xian.gong_index].gong_data.gong_wei)));
        }
    }
    
    void display_xiao_xian_analysis(const ZiWeiResult& result, int current_age) {
        fmt::print("\n\n");
        fmt::print("       小限分析（{}岁）\n", current_age);
        fmt::print("\n\n");
        
        auto xiao_xian_data = get_xiao_xian(current_age, result.is_male, result.year_pillar.zhi);
        
        fmt::print("小限宫位：第{}宫 {}\n",
            xiao_xian_data.gong_index,
            string(to_zh(result.palaces[xiao_xian_data.gong_index].gong_data.gong_wei)));
        
        fmt::print("\n{}\n", result.palaces[xiao_xian_data.gong_index].to_string());
    }
    
    void display_liu_nian_analysis(
        const ZiWeiResult& result,
        int target_year,
        int current_age
    ) {
        fmt::print("\n\n");
        fmt::print(
            "       {}年流年分析（{}岁）\n",
            target_year,
            current_age
        );
        fmt::print("\n\n");

        // 整年流年查询直接使用目标流年本身，
        // 不再通过任意的 7 月 1 日间接取得干支。
        auto [tian_gan, di_zhi] =
            get_year_gan_zhi_from_year(target_year);

        auto liu_nian_data = get_liu_nian(
            target_year,
            tian_gan,
            di_zhi,
            result.ming_gong_index
        );

        fmt::print(
            "流年干支：{}{} ({}年)\n",
            string(GanZhi::Mapper::to_zh(liu_nian_data.tian_gan)),
            string(GanZhi::Mapper::to_zh(liu_nian_data.di_zhi)),
            target_year
        );
        fmt::print(
            "流年宫位：第{}宫 {}\n",
            liu_nian_data.gong_index,
            string(to_zh(
                result.palaces[liu_nian_data.gong_index]
                      .gong_data.gong_wei
            ))
        );

        fmt::print("\n流年四化：\n");
        for (int i = 0; i < 4; ++i) {
            if (!liu_nian_data.si_hua[i].empty()) {
                fmt::print(
                    "  {} - {}\n",
                    string(to_zh(static_cast<SiHua>(i))),
                    liu_nian_data.si_hua[i]
                );
            }
        }

        fmt::print("\n流年宫位详情：\n");
        fmt::print(
            "{}\n",
            result.palaces[liu_nian_data.gong_index].to_string()
        );
    }

    void display_liu_nian_analysis(
        const ZiWeiResult& result,
        int target_year,
        int target_month,
        int target_day,
        int current_age,
        LiuNianYearBoundaryPolicy policy
    ) {
        auto solar_day =
            tyme::SolarDay::from_ymd(
                target_year,
                target_month,
                target_day
            );

        int lunar_year =
            solar_day.get_lunar_day().get_year();

        int li_chun_year =
            solar_day
                .get_sixty_cycle_day()
                .get_sixty_cycle_month()
                .get_sixty_cycle_year()
                .get_year();

        int effective_year = resolve_liu_nian_year(
            lunar_year,
            li_chun_year,
            policy
        );

        auto [tian_gan, di_zhi] =
            get_year_gan_zhi_from_year(effective_year);

        auto liu_nian_data = get_liu_nian(
            effective_year,
            tian_gan,
            di_zhi,
            result.ming_gong_index
        );

        const char* policy_name =
            policy == LiuNianYearBoundaryPolicy::LunarNewYear
                ? "农历正月初一换年"
                : "立春换年";

        fmt::print("\n\n");
        fmt::print(
            "       {}年{}月{}日流年分析（{}岁）\n",
            target_year,
            target_month,
            target_day,
            current_age
        );
        fmt::print("\n\n");

        fmt::print(
            "流年边界：{}\n"
            "目标日期所属流年：{}\n"
            "流年干支：{}{}\n",
            policy_name,
            effective_year,
            string(GanZhi::Mapper::to_zh(liu_nian_data.tian_gan)),
            string(GanZhi::Mapper::to_zh(liu_nian_data.di_zhi))
        );

        fmt::print(
            "流年宫位：第{}宫 {}\n",
            liu_nian_data.gong_index,
            string(to_zh(
                result.palaces[liu_nian_data.gong_index]
                      .gong_data.gong_wei
            ))
        );

        fmt::print("\n流年四化：\n");
        for (int i = 0; i < 4; ++i) {
            if (!liu_nian_data.si_hua[i].empty()) {
                fmt::print(
                    "  {} - {}\n",
                    string(to_zh(static_cast<SiHua>(i))),
                    liu_nian_data.si_hua[i]
                );
            }
        }

        fmt::print("\n流年宫位详情：\n");
        fmt::print(
            "{}\n",
            result.palaces[liu_nian_data.gong_index].to_string()
        );
    }

    /**
     * @brief 计算某个公历日期实际所属的紫微流月
     *
     * 流月宫位采用斗君。
     * 农历月份继续遵守本命排盘所保存的闰月策略。
     * 流年干支的换年边界由 year_boundary_policy 显式决定，
     * 不再隐式固定为 tyme 的立春年。
     */
    static LiuYueData calculate_liu_yue_for_solar_day(
        const ZiWeiResult& result,
        const tyme::SolarDay& solar_day,
        LiuNianYearBoundaryPolicy year_boundary_policy,
        LiuYueGanZhiPolicy month_gan_zhi_policy
    ) {
        auto lunar_day = solar_day.get_lunar_day();

        int raw_lunar_month = lunar_day.get_month();
        int lunar_month = resolve_ziwei_month(
            raw_lunar_month,
            lunar_day.get_day(),
            result.leap_month_policy
        );

        auto sixty_cycle_day = solar_day.get_sixty_cycle_day();
        auto month_cycle = sixty_cycle_day.get_month();

        int lunar_year = lunar_day.get_year();

        int li_chun_year =
            sixty_cycle_day
                .get_sixty_cycle_month()
                .get_sixty_cycle_year()
                .get_year();

        int effective_year = resolve_liu_nian_year(
            lunar_year,
            li_chun_year,
            year_boundary_policy
        );

        auto [year_gan, year_zhi] =
            get_year_gan_zhi_from_year(effective_year);

        TianGan solar_term_month_gan =
            static_cast<TianGan>(
                month_cycle.get_heaven_stem().get_index()
            );

        DiZhi solar_term_month_zhi =
            static_cast<DiZhi>(
                month_cycle.get_earth_branch().get_index()
            );

        return get_liu_yue(
            lunar_month,
            result.resolved_lunar_month,
            result.hour_pillar.zhi,
            year_gan,
            year_zhi,
            month_gan_zhi_policy,
            solar_term_month_gan,
            solar_term_month_zhi
        );
    }

    void display_liu_yue_analysis(
        const ZiWeiResult& result,
        int target_year,
        int target_month,
        int current_age
    ) {
        // 这是“公历月份概览”接口。
        // 明确以当月15日作为代表日，不再假装代表整个月。
        constexpr int reference_day = 15;

        auto solar_day = tyme::SolarDay::from_ymd(
            target_year,
            target_month,
            reference_day
        );

        auto lunar_day = solar_day.get_lunar_day();

        auto liu_yue_data = calculate_liu_yue_for_solar_day(
            result,
            solar_day,
            LiuNianYearBoundaryPolicy::LunarNewYear,
            LiuYueGanZhiPolicy::LunarMonthWuHuDun
        );

        fmt::print("\n\n");
        fmt::print(
            "       {}年{}月流月概览（{}岁）\n",
            target_year,
            target_month,
            current_age
        );
        fmt::print(
            "       参考日期：{}-{}-{}\n\n",
            target_year,
            target_month,
            reference_day
        );

        fmt::print(
            "流月干支：{}{}（农历{}）\n",
            string(GanZhi::Mapper::to_zh(
                liu_yue_data.tian_gan
            )),
            string(GanZhi::Mapper::to_zh(
                liu_yue_data.di_zhi
            )),
            lunar_day.get_lunar_month().get_name()
        );

        fmt::print(
            "流月宫位：第{}宫 {}\n",
            liu_yue_data.gong_index,
            string(to_zh(
                result.palaces[liu_yue_data.gong_index]
                      .gong_data.gong_wei
            ))
        );

        fmt::print("\n流月四化：\n");
        for (int i = 0; i < 4; ++i) {
            if (!liu_yue_data.si_hua[i].empty()) {
                fmt::print(
                    "  {} - {}\n",
                    string(to_zh(static_cast<SiHua>(i))),
                    liu_yue_data.si_hua[i]
                );
            }
        }

        fmt::print("\n流月宫位详情：\n");
        fmt::print(
            "{}\n",
            result.palaces[liu_yue_data.gong_index].to_string()
        );
    }

    void display_liu_yue_analysis(
        const ZiWeiResult& result,
        int target_year,
        int target_month,
        int target_day,
        int current_age,
        LiuNianYearBoundaryPolicy year_boundary_policy
    ) {
        auto solar_day = tyme::SolarDay::from_ymd(
            target_year,
            target_month,
            target_day
        );

        auto lunar_day = solar_day.get_lunar_day();

        auto liu_yue_data = calculate_liu_yue_for_solar_day(
            result,
            solar_day,
            year_boundary_policy,
            LiuYueGanZhiPolicy::LunarMonthWuHuDun
        );

        fmt::print("\n\n");
        fmt::print(
            "       {}年{}月{}日所属流月分析（{}岁）\n",
            target_year,
            target_month,
            target_day,
            current_age
        );
        fmt::print("\n");

        fmt::print(
            "流月干支：{}{}（农历{}）\n",
            string(GanZhi::Mapper::to_zh(
                liu_yue_data.tian_gan
            )),
            string(GanZhi::Mapper::to_zh(
                liu_yue_data.di_zhi
            )),
            lunar_day.get_lunar_month().get_name()
        );

        fmt::print(
            "流月宫位：第{}宫 {}\n",
            liu_yue_data.gong_index,
            string(to_zh(
                result.palaces[liu_yue_data.gong_index]
                      .gong_data.gong_wei
            ))
        );

        fmt::print("\n流月四化：\n");
        for (int i = 0; i < 4; ++i) {
            if (!liu_yue_data.si_hua[i].empty()) {
                fmt::print(
                    "  {} - {}\n",
                    string(to_zh(static_cast<SiHua>(i))),
                    liu_yue_data.si_hua[i]
                );
            }
        }

        fmt::print("\n流月宫位详情：\n");
        fmt::print(
            "{}\n",
            result.palaces[liu_yue_data.gong_index].to_string()
        );
    }

    void display_liu_ri_analysis(
        const ZiWeiResult& result,
        int target_year,
        int target_month,
        int target_day,
        int current_age,
        LiuNianYearBoundaryPolicy year_boundary_policy
    ) {
        fmt::print("\n\n");
        fmt::print("       {}年{}月{}日流日分析（{}岁）\n", target_year, target_month, target_day, current_age);
        fmt::print("\n\n");
        
        // 使用tyme库获取流日天干地支
        auto solar_day = tyme::SolarDay::from_ymd(target_year, target_month, target_day);
        auto lunar_day = solar_day.get_lunar_day();
        int lunar_day_num = lunar_day.get_day();
        
        auto sixty_cycle_day = solar_day.get_sixty_cycle_day();
        auto day_cycle = sixty_cycle_day.get_sixty_cycle();
        
        TianGan day_gan = static_cast<TianGan>(day_cycle.get_heaven_stem().get_index());
        DiZhi day_zhi = static_cast<DiZhi>(day_cycle.get_earth_branch().get_index());
        
        auto liu_yue_data = calculate_liu_yue_for_solar_day(
            result,
            solar_day,
            year_boundary_policy,
            LiuYueGanZhiPolicy::LunarMonthWuHuDun
        );
        
        // 调用horoscope模块获取流日数据
        auto liu_ri_data = get_liu_ri(lunar_day_num, day_gan, day_zhi, liu_yue_data.gong_index);
        
        fmt::print("流日干支：{}{}（农历{}{}日）\n",
            string(GanZhi::Mapper::to_zh(liu_ri_data.tian_gan)),
            string(GanZhi::Mapper::to_zh(liu_ri_data.di_zhi)),
            lunar_day.get_lunar_month().get_name(),
            liu_ri_data.day);
        fmt::print("流日宫位：第{}宫 {}\n",
            liu_ri_data.gong_index,
            string(to_zh(result.palaces[liu_ri_data.gong_index].gong_data.gong_wei)));
        
        // 显示流日四化
        fmt::print("\n流日四化：\n");
        for (int i = 0; i < 4; ++i) {
            if (!liu_ri_data.si_hua[i].empty()) {
                fmt::print("  {} - {}\n", 
                    string(to_zh(static_cast<SiHua>(i))),
                    liu_ri_data.si_hua[i]);
            }
        }
        
        fmt::print("\n流日宫位详情：\n");
        fmt::print("{}\n", result.palaces[liu_ri_data.gong_index].to_string());
    }
    
    void display_liu_shi_analysis(
        const ZiWeiResult& result,
        int target_year,
        int target_month,
        int target_day,
        DiZhi target_hour,
        int current_age,
        LiuNianYearBoundaryPolicy year_boundary_policy
    ) {
        fmt::print("\n\n");
        fmt::print("       {}年{}月{}日 {} 流时分析（{}岁）\n", 
            target_year, target_month, target_day, 
            string(to_zh(target_hour)), 
            current_age);
        fmt::print("\n\n");
        
        // 使用tyme库获取流时天干
        auto solar_day = tyme::SolarDay::from_ymd(target_year, target_month, target_day);
        auto sixty_cycle_day = solar_day.get_sixty_cycle_day();
        auto day_cycle = sixty_cycle_day.get_sixty_cycle();
        
        // 根据日干和时辰地支计算时干（五鼠遁日起时法）
        int day_gan_index = day_cycle.get_heaven_stem().get_index();
        int hour_zhi_index = static_cast<int>(target_hour);
        int hour_gan_index = (day_gan_index % 5 * 2 + hour_zhi_index) % 10;
        
        TianGan hour_gan = static_cast<TianGan>(hour_gan_index);
        
        // 获取流日宫位索引（需要先计算流日）
        auto lunar_day = solar_day.get_lunar_day();
        int lunar_day_num = lunar_day.get_day();
        TianGan day_gan = static_cast<TianGan>(day_cycle.get_heaven_stem().get_index());
        DiZhi day_zhi = static_cast<DiZhi>(day_cycle.get_earth_branch().get_index());
        
        auto liu_yue_data = calculate_liu_yue_for_solar_day(
            result,
            solar_day,
            year_boundary_policy,
            LiuYueGanZhiPolicy::LunarMonthWuHuDun
        );
        auto liu_ri_data = get_liu_ri(lunar_day_num, day_gan, day_zhi, liu_yue_data.gong_index);
        
        // 调用horoscope模块获取流时数据
        auto liu_shi_data = get_liu_shi(target_hour, hour_gan, liu_ri_data.gong_index);
        
        fmt::print("流时干支：{}{}\n", 
            string(GanZhi::Mapper::to_zh(liu_shi_data.tian_gan)),
            string(GanZhi::Mapper::to_zh(liu_shi_data.di_zhi)));
        fmt::print("流时宫位：第{}宫 {}\n",
            liu_shi_data.gong_index,
            string(to_zh(result.palaces[liu_shi_data.gong_index].gong_data.gong_wei)));
        
        // 显示流时四化
        fmt::print("\n流时四化：\n");
        for (int i = 0; i < 4; ++i) {
            if (!liu_shi_data.si_hua[i].empty()) {
                fmt::print("  {} - {}\n", 
                    string(to_zh(static_cast<SiHua>(i))),
                    liu_shi_data.si_hua[i]);
            }
        }
        
        fmt::print("\n流时宫位详情：\n");
        fmt::print("{}\n", result.palaces[liu_shi_data.gong_index].to_string());
    }
    
    void display_yun_xian_full_analysis(
        const ZiWeiResult& result,
        int target_year,
        int target_month,
        int target_day,
        DiZhi target_hour,
        int current_age,
        LiuNianYearBoundaryPolicy year_boundary_policy
    ) {
        fmt::print("\n");
        fmt::print("\n");
        fmt::print("              完整运限分析报告                              \n");
        fmt::print("       {}年{}月{}日 {} （{}岁）            \n",
            target_year, target_month, target_day,
            string(GanZhi::Mapper::to_zh(target_hour)),
            current_age);
        fmt::print("\n");
        
        // 大限
        display_da_xian_analysis(result);
        
        // 小限
        display_xiao_xian_analysis(result, current_age);
        
        // 流年
        display_liu_nian_analysis(
            result,
            target_year,
            target_month,
            target_day,
            current_age,
            year_boundary_policy
        );

        // 流月：使用目标当天实际所属流月，不能再用公历15日代理
        display_liu_yue_analysis(
            result,
            target_year,
            target_month,
            target_day,
            current_age,
            year_boundary_policy
        );
        
        // 流日
        display_liu_ri_analysis(
            result,
            target_year,
            target_month,
            target_day,
            current_age,
            year_boundary_policy
        );
        
        // 流时
        display_liu_shi_analysis(
            result,
            target_year,
            target_month,
            target_day,
            target_hour,
            current_age,
            year_boundary_policy
        );
        
        fmt::print("\n");
        fmt::print("\n");
        fmt::print("       运限分析完成\n");
        fmt::print("\n");
    }

    // ============= 综合分析功能实现 =============
    
    void display_full_analysis(const ZiWeiResult& result) {
        fmt::print("\n");
        fmt::print("\n");
        fmt::print("              紫微斗数完整命盘分析报告                      \n");
        fmt::print("\n");
        
        // 基本信息
        fmt::print("\n【基本信息】\n");
        fmt::print("{}\n", result.to_string());
        
        // 格局分析
        display_ge_ju_analysis(result);
        
        // 四化分析
        display_gong_gan_si_hua(result);
        
        // 自化分析
        display_zi_hua_analysis(result);
        
        // 空宫分析
        display_kong_gong_jie_xing(result);
        
        // 大限分析
        display_da_xian_analysis(result);
    }
    
    string export_to_json_full(const ZiWeiResult& result) {
        json j = json::parse(export_to_json(result));
        
        auto stars_in_gong = prepare_stars_in_gong(result);
        auto gong_di_zhi = prepare_gong_di_zhi(result);
        auto gong_gan_zhi = prepare_gong_gan_zhi(result);
        
        // 添加格局信息
        GeJuAnalyzer ge_ju_analyzer(stars_in_gong, gong_di_zhi, result.ming_gong_index);
        auto ji_ge = ge_ju_analyzer.analyze_ji_ge();
        auto xiong_ge = ge_ju_analyzer.analyze_xiong_ge();
        
        j["ge_ju"]["ji_ge"] = json::array();
        for (const auto& geju : ji_ge) {
            json g;
            g["name"] = geju.name;
            g["description"] = geju.description;
            g["score"] = geju.score;
            j["ge_ju"]["ji_ge"].push_back(g);
        }
        
        j["ge_ju"]["xiong_ge"] = json::array();
        for (const auto& geju : xiong_ge) {
            json g;
            g["name"] = geju.name;
            g["description"] = geju.description;
            g["score"] = geju.score;
            j["ge_ju"]["xiong_ge"].push_back(g);
        }
        
        j["ge_ju"]["total_score"] = ge_ju_analyzer.get_total_score();
        
        // 添加四化信息
        SiHuaSystem si_hua_system(gong_gan_zhi, stars_in_gong);
        j["si_hua"]["gong_gan_si_hua"] = json::array();
        for (const auto& gong_si_hua : si_hua_system.get_all_gong_gan_si_hua()) {
            json sh;
            sh["gong_index"] = gong_si_hua.gong_index;
            sh["gong_gan"] = string(to_zh(gong_si_hua.gong_gan));
            sh["si_hua_list"] = json::array();
            for (const auto& si_hua_info : gong_si_hua.si_hua_list) {
                if (!si_hua_info.star_name.empty()) {
                    json info;
                    info["star"] = si_hua_info.star_name;
                    info["type"] = string(to_zh(si_hua_info.type));
                    info["to_gong"] = si_hua_info.gong_index;
                    sh["si_hua_list"].push_back(info);
                }
            }
            j["si_hua"]["gong_gan_si_hua"].push_back(sh);
        }
        
        auto zi_hua_list = si_hua_system.get_all_zi_hua();
        j["si_hua"]["zi_hua"] = json::array();
        for (const auto& zi_hua : zi_hua_list) {
            json zh;
            zh["gong_index"] = zi_hua.gong_index;
            zh["types"] = json::array();
            for (const auto& type : zi_hua.zi_hua_types) {
                zh["types"].push_back(string(to_zh(type)));
            }
            j["si_hua"]["zi_hua"].push_back(zh);
        }
        
        // ====================================================
        // 本命年干四化
        // ====================================================
        {
            auto natal_si_hua_names =
                get_si_hua_star_names(
                    result.year_pillar.gan
                );

            static constexpr const char* keys[4] = {
                "lu", "quan", "ke", "ji"
            };

            j["si_hua"]["natal"] = json::object();

            for (int i = 0; i < 4; ++i) {
                j["si_hua"]["natal"][keys[i]] = {
                    {
                        "type",
                        string(to_zh(
                            static_cast<SiHua>(i)
                        ))
                    },
                    {
                        "star",
                        natal_si_hua_names[i]
                    }
                };
            }
        }

        // ====================================================
        // 所有直接飞化关系
        //
        // 注意：
        // get_fei_hua_from() 也包含“飞回本宫”的自化，
        // 所以这里明确给出 is_zi_hua 字段。
        // ====================================================
        j["si_hua"]["fei_hua_relations"] =
            json::array();

        for (int from = 0; from < 12; ++from) {
            auto relations =
                si_hua_system.get_fei_hua_from(from);

            for (const auto& relation : relations) {
                j["si_hua"]["fei_hua_relations"]
                    .push_back({
                        {
                            "from_gong",
                            relation.from_gong
                        },
                        {
                            "from_gong_name",
                            string(to_zh(
                                result
                                    .palaces[
                                        relation.from_gong
                                    ]
                                    .gong_data
                                    .gong_wei
                            ))
                        },
                        {
                            "from_gan",
                            string(to_zh(
                                relation.from_gan
                            ))
                        },
                        {
                            "to_gong",
                            relation.to_gong
                        },
                        {
                            "to_gong_name",
                            string(to_zh(
                                result
                                    .palaces[
                                        relation.to_gong
                                    ]
                                    .gong_data
                                    .gong_wei
                            ))
                        },
                        {
                            "type",
                            string(to_zh(
                                relation.si_hua_type
                            ))
                        },
                        {
                            "star",
                            relation.star_name
                        },
                        {
                            "is_zi_hua",
                            relation.from_gong ==
                                relation.to_gong
                        }
                    });
            }
        }

        // ====================================================
        // 每宫 × 四化类型：最多4层飞化链
        // ====================================================
        j["si_hua"]["fei_hua_chains"] =
            json::array();

        for (int start_gong = 0;
             start_gong < 12;
             ++start_gong) {

            for (int type = 0; type < 4; ++type) {
                auto chains =
                    si_hua_system.get_fei_hua_chains(
                        start_gong,
                        static_cast<SiHua>(type),
                        4
                    );

                for (const auto& chain : chains) {
                    json cj;

                    cj["start_gong"] =
                        start_gong;

                    cj["start_gong_name"] =
                        string(to_zh(
                            result
                                .palaces[start_gong]
                                .gong_data
                                .gong_wei
                        ));

                    cj["type"] =
                        string(to_zh(
                            static_cast<SiHua>(type)
                        ));

                    cj["depth"] =
                        chain.chain.size();

                    cj["is_hui_ben"] =
                        chain.is_hui_ben;

                    cj["relations"] =
                        json::array();

                    for (const auto& r : chain.chain) {
                        cj["relations"].push_back({
                            {"from_gong", r.from_gong},
                            {"to_gong", r.to_gong},
                            {
                                "from_gan",
                                string(to_zh(r.from_gan))
                            },
                            {
                                "type",
                                string(to_zh(
                                    r.si_hua_type
                                ))
                            },
                            {"star", r.star_name}
                        });
                    }

                    j["si_hua"]["fei_hua_chains"]
                        .push_back(cj);
                }
            }
        }

        // ====================================================
        // 回本宫链：单独索引，便于分析程序直接使用
        // ====================================================
        j["si_hua"]["hui_ben_chains"] =
            json::array();

        for (int start_gong = 0;
             start_gong < 12;
             ++start_gong) {

            auto chains =
                si_hua_system.find_hui_ben_chains(
                    start_gong
                );

            for (const auto& chain : chains) {
                json cj;

                cj["start_gong"] =
                    start_gong;

                cj["start_gong_name"] =
                    string(to_zh(
                        result
                            .palaces[start_gong]
                            .gong_data
                            .gong_wei
                    ));

                cj["depth"] =
                    chain.chain.size();

                cj["relations"] =
                    json::array();

                for (const auto& r : chain.chain) {
                    cj["relations"].push_back({
                        {"from_gong", r.from_gong},
                        {"to_gong", r.to_gong},
                        {
                            "from_gan",
                            string(to_zh(r.from_gan))
                        },
                        {
                            "type",
                            string(to_zh(
                                r.si_hua_type
                            ))
                        },
                        {"star", r.star_name}
                    });
                }

                j["si_hua"]["hui_ben_chains"]
                    .push_back(cj);
            }
        }

        // ====================================================
        // 十二宫全部三方四正
        // ====================================================
        j["san_fang_si_zheng"] =
            json::array();

        for (int gong = 0; gong < 12; ++gong) {
            auto sf =
                get_san_fang_si_zheng(gong);

            json sfj = {
                {"gong_index", gong},
                {
                    "gong_name",
                    string(to_zh(
                        result.palaces[gong]
                            .gong_data.gong_wei
                    ))
                },
                {
                    "ben_gong_index",
                    sf.ben_gong_index
                },
                {
                    "dui_gong_index",
                    sf.dui_gong_index
                },
                {
                    "cai_bo_index",
                    sf.cai_bo_index
                },
                {
                    "guan_lu_index",
                    sf.guan_lu_index
                }
            };

            sfj["indices"] =
                sf.get_all_indices();

            sfj["palaces"] = {
                {
                    "ben_gong",
                    string(to_zh(
                        result
                            .palaces[
                                sf.ben_gong_index
                            ]
                            .gong_data
                            .gong_wei
                    ))
                },
                {
                    "dui_gong",
                    string(to_zh(
                        result
                            .palaces[
                                sf.dui_gong_index
                            ]
                            .gong_data
                            .gong_wei
                    ))
                },
                {
                    "cai_bo",
                    string(to_zh(
                        result
                            .palaces[
                                sf.cai_bo_index
                            ]
                            .gong_data
                            .gong_wei
                    ))
                },
                {
                    "guan_lu",
                    string(to_zh(
                        result
                            .palaces[
                                sf.guan_lu_index
                            ]
                            .gong_data
                            .gong_wei
                    ))
                }
            };

            j["san_fang_si_zheng"].push_back(
                sfj
            );
        }

        // ====================================================
        // 完整十二大限
        // ====================================================
        j["da_xian"] = json::array();

        static constexpr const char* si_hua_keys[4] = {
            "lu", "quan", "ke", "ji"
        };

        for (const auto& dx : result.da_xian_data) {
            json dxj;

            dxj["start_age"] = dx.start_age;
            dxj["end_age"] = dx.end_age;
            dxj["gong_index"] = dx.gong_index;

            dxj["gong_name"] =
                string(to_zh(
                    result.palaces[dx.gong_index]
                        .gong_data.gong_wei
                ));

            dxj["gan"] =
                string(
                    GanZhi::Mapper::to_zh(
                        dx.tian_gan
                    )
                );

            dxj["zhi"] =
                string(
                    GanZhi::Mapper::to_zh(
                        dx.di_zhi
                    )
                );

            dxj["si_hua"] = json::object();

            for (int i = 0; i < 4; ++i) {
                if (dx.si_hua[i].empty()) {
                    dxj["si_hua"][si_hua_keys[i]] =
                        nullptr;
                } else {
                    dxj["si_hua"][si_hua_keys[i]] =
                        dx.si_hua[i];
                }
            }

            j["da_xian"].push_back(dxj);
        }

        // ====================================================
        // 空宫与借星
        // ====================================================
        {
            SanFangAnalyzer san_fang_analyzer(
                stars_in_gong
            );

            auto kong_gong =
                san_fang_analyzer.get_all_kong_gong();

            j["kong_gong"] =
                json::array();

            for (const auto& kg : kong_gong) {
                j["kong_gong"].push_back({
                    {
                        "gong_index",
                        kg.gong_index
                    },
                    {
                        "gong_name",
                        string(to_zh(
                            result
                                .palaces[
                                    kg.gong_index
                                ]
                                .gong_data
                                .gong_wei
                        ))
                    },
                    {
                        "dui_gong_index",
                        kg.dui_gong_index
                    },
                    {
                        "jie_xing",
                        kg.jie_xing
                    }
                });
            }
        }

        return j.dump(2);
    }

    string export_analysis_dataset(
        const ZiWeiResult& result,
        int center_year,
        const AnalysisDatasetOptions& options
    ) {
        if (options.years_before < 0 || options.years_after < 0) {
            throw invalid_argument(
                "years_before / years_after 不能为负数"
            );
        }

        json root;

        root["schema"] = {
            {"name", "zhouyilab-ziwei-analysis-dataset"},
            {"version", 1}
        };

        root["range"] = {
            {"center_year", center_year},
            {"years_before", options.years_before},
            {"years_after", options.years_after},
            {"start_year", center_year - options.years_before},
            {"end_year", center_year + options.years_after},
            {"year_count",
                options.years_before +
                options.years_after + 1}
        };

        // ----------------------------------------------------
        // 策略名称
        // ----------------------------------------------------
        auto leap_month_policy_name =
            [](LeapMonthPolicy policy) -> string {
                switch (policy) {
                    case LeapMonthPolicy::SameAsRegularMonth:
                        return "same_as_regular_month";
                    case LeapMonthPolicy::NextMonth:
                        return "next_month";
                    case LeapMonthPolicy::SplitAtDay15:
                        return "split_at_day_15";
                }
                return "unknown";
            };

        auto zi_hour_policy_name =
            [](ZiHourDayBoundaryPolicy policy) -> string {
                switch (policy) {
                    case ZiHourDayBoundaryPolicy::Midnight:
                        return "midnight";
                    case ZiHourDayBoundaryPolicy::LateZi:
                        return "late_zi_23_to_next_day";
                }
                return "unknown";
            };

        auto year_boundary_policy_name =
            [](LiuNianYearBoundaryPolicy policy) -> string {
                switch (policy) {
                    case LiuNianYearBoundaryPolicy::LunarNewYear:
                        return "lunar_new_year";
                    case LiuNianYearBoundaryPolicy::LiChun:
                        return "li_chun";
                }
                return "unknown";
            };

        auto month_policy_name =
            [](LiuYueGanZhiPolicy policy) -> string {
                switch (policy) {
                    case LiuYueGanZhiPolicy::LunarMonthWuHuDun:
                        return "lunar_month_wu_hu_dun";
                    case LiuYueGanZhiPolicy::SolarTermMonthPillar:
                        return "solar_term_month_pillar";
                }
                return "unknown";
            };

        auto virtual_age_policy_name =
            [](VirtualAgeBoundaryPolicy policy) -> string {
                switch (policy) {
                    case VirtualAgeBoundaryPolicy::LunarNewYear:
                        return "lunar_new_year";
                }
                return "unknown";
            };

        auto huo_ling_policy_name =
            [](HuoLingPolicy policy) -> string {
                switch (policy) {
                    case HuoLingPolicy::YearBranchStartBothForward:
                        return "year_branch_start_both_forward";
                }
                return "unknown";
            };

        auto brightness_policy_name =
            [](MainStarBrightnessPolicy policy) -> string {
                switch (policy) {
                    case MainStarBrightnessPolicy::ZhouYiLabBuiltinV1:
                        return "zhouyilab_builtin_v1";
                }
                return "unknown";
            };

        // ----------------------------------------------------
        // 规则及可信状态
        // ----------------------------------------------------
        root["rule_metadata"] = {
            {
                "leap_month_policy",
                leap_month_policy_name(
                    result.leap_month_policy
                )
            },
            {
                "natal_zi_hour_day_boundary_policy",
                zi_hour_policy_name(
                    result.zi_hour_day_boundary_policy
                )
            },
            {
                "timeline_day_boundary_policy",
                zi_hour_policy_name(
                    options.day_boundary_policy
                )
            },
            {
                "virtual_age_boundary_policy",
                virtual_age_policy_name(
                    options.virtual_age_boundary_policy
                )
            },
            {
                "liu_nian_year_boundary_policy",
                year_boundary_policy_name(
                    options.year_boundary_policy
                )
            },
            {
                "liu_yue_gan_zhi_policy",
                month_policy_name(
                    options.month_gan_zhi_policy
                )
            },
            {
                "huo_ling_policy",
                huo_ling_policy_name(
                    result.huo_ling_policy
                )
            },
            {
                "main_star_brightness_policy",
                brightness_policy_name(
                    result.main_star_brightness_policy
                )
            }
        };

        root["rule_metadata"]["main_star_brightness"] = {
            {"status", "enabled_builtin_table"},
            {
                "note",
                "仅十四主星使用当前内建亮度表；"
                "非主星无审定亮度时输出 null"
            }
        };

        root["rule_metadata"]["analysis_components"] = {
            {
                "natal_palaces",
                {
                    {"status", "verified_by_regression"},
                    {"note", "十二宫、主辅煞杂曜及十二神已纳入结构回归"}
                }
            },
            {
                "natal_si_hua",
                {
                    {"status", "verified_by_regression"},
                    {"note", "本命年干四化使用当前十干四化表"}
                }
            },
            {
                "gong_gan_si_hua",
                {
                    {"status", "verified_by_regression"},
                    {"note", "宫干四化、自化、飞入飞出方向已通过人工构造回归"}
                }
            },
            {
                "fei_hua_chains",
                {
                    {"status", "verified_by_regression"},
                    {"max_depth", 4},
                    {"note", "多层飞化及回本链递归方向已通过人工环链回归"}
                }
            },
            {
                "san_fang_si_zheng",
                {
                    {"status", "implemented_structurally"},
                    {"note", "十二宫均输出本宫、对宫及两组三合宫结构"}
                }
            },
            {
                "ge_ju",
                {
                    {"status", "implemented_not_fully_rule_audited"},
                    {"note", "格局结果可用于分析，但格局规则尚未逐条完成与核心排盘同等级审计"}
                }
            },
            {
                "da_xian",
                {
                    {"status", "verified_by_regression"},
                    {"note", "五行局、顺逆行及当前大限选择已有回归"}
                }
            },
            {
                "xiao_xian",
                {
                    {"status", "verified_by_regression"}
                }
            },
            {
                "liu_nian",
                {
                    {"status", "verified_by_regression"}
                }
            },
            {
                "liu_yue",
                {
                    {"status", "verified_by_regression"},
                    {"note", "流月宫位与干支策略显式记录；动态流耀除外"}
                }
            },
            {
                "liu_ri",
                {
                    {"status", "verified_by_regression"},
                    {"note", "流日宫位、干支、四化已验证；动态流耀未启用"}
                }
            },
            {
                "liu_shi",
                {
                    {"status", "verified_by_regression"},
                    {"note", "流时宫位、五鼠遁时干、四化及晚子边界已验证；动态流耀未启用"}
                }
            }
        };

        root["rule_metadata"]["data_semantics"] = {
            {
                "missing_brightness",
                "null 表示当前没有审定亮度规则，不等于平"
            },
            {
                "empty_dynamic_star_array",
                "必须结合 dynamic_stars.status 解读；unverified_not_enabled 不代表实际无星"
            },
            {
                "fei_hua_self_relation",
                "from_gong == to_gong 同时属于直接飞化关系与自化"
            },
            {
                "timeline_hour_resolution",
                "24个公历小时节点，用于保留23:00晚子换日边界"
            }
        };

        root["rule_metadata"]["dynamic_stars"] = {
            {
                "decadal",
                {
                    {"status", "enabled_verified_subset"},
                    {
                        "stars",
                        {
                            "运魁", "运钺",
                            "运禄", "运羊", "运陀",
                            "运马", "运鸾", "运喜"
                        }
                    }
                }
            },
            {
                "yearly",
                {
                    {"status", "enabled_verified_subset"},
                    {
                        "stars",
                        {
                            "流魁", "流钺",
                            "流禄", "流羊", "流陀",
                            "流马", "流鸾", "流喜",
                            "年解"
                        }
                    }
                }
            },
            {
                "monthly",
                {
                    {"status", "unverified_not_enabled"},
                    {
                        "note",
                        "尚未审定独立流月动态流耀规则"
                    }
                }
            },
            {
                "daily",
                {
                    {"status", "unverified_not_enabled"},
                    {
                        "note",
                        "尚未审定独立流日动态流耀规则"
                    }
                }
            },
            {
                "hourly",
                {
                    {"status", "unverified_not_enabled"},
                    {
                        "note",
                        "尚未审定独立流时动态流耀规则"
                    }
                }
            }
        };

        // ----------------------------------------------------
        // 本命：暂直接纳入现有完整版；
        // 下一阶段继续把本命 JSON 补至真正全字段。
        // ----------------------------------------------------
        root["natal"] =
            json::parse(export_to_json_full(result));

        root["natal"]["calendar_resolution"] = {
            {
                "raw_lunar_month",
                result.raw_lunar_month
            },
            {
                "resolved_lunar_month",
                result.resolved_lunar_month
            },
            {
                "resolved_lunar_day",
                result.resolved_lunar_day
            },
            {
                "is_leap_month",
                result.is_leap_month
            },
            {
                "zi_hour_shifted_to_next_day",
                result.zi_hour_shifted_to_next_day
            }
        };

        root["natal"]["ming_zhu"] =
            result.ming_zhu_xing;

        root["natal"]["shen_zhu"] =
            result.shen_zhu_xing;

        // ----------------------------------------------------
        // JSON 转换辅助函数
        // ----------------------------------------------------
        auto si_hua_to_json =
            [](const array<string, 4>& si_hua) {
                json j;
                static constexpr const char* names[4] = {
                    "lu", "quan", "ke", "ji"
                };

                for (int i = 0; i < 4; ++i) {
                    j[names[i]] =
                        si_hua[i].empty()
                            ? json(nullptr)
                            : json(si_hua[i]);
                }

                return j;
            };

        auto dynamic_stars_to_json =
            [](const array<HoroscopeStarData, 12>& data) {
                json a = json::array();

                for (const auto& gong : data) {
                    if (gong.stars.empty()) {
                        continue;
                    }

                    a.push_back({
                        {"gong_index", gong.gong_index},
                        {"stars", gong.stars}
                    });
                }

                return a;
            };

        auto da_xian_to_json =
            [&](const optional<DaXianData>& dx) -> json {
                if (!dx.has_value()) {
                    return nullptr;
                }

                return {
                    {"start_age", dx->start_age},
                    {"end_age", dx->end_age},
                    {"gong_index", dx->gong_index},
                    {
                        "gong_name",
                        string(to_zh(
                            result.palaces[dx->gong_index]
                                .gong_data.gong_wei
                        ))
                    },
                    {
                        "gan",
                        string(
                            GanZhi::Mapper::to_zh(
                                dx->tian_gan
                            )
                        )
                    },
                    {
                        "zhi",
                        string(
                            GanZhi::Mapper::to_zh(
                                dx->di_zhi
                            )
                        )
                    },
                    {"si_hua", si_hua_to_json(dx->si_hua)}
                };
            };

        auto xiao_xian_to_json =
            [&](const XiaoXianData& xx) {
                return json{
                    {"age", xx.age},
                    {"gong_index", xx.gong_index},
                    {
                        "gong_name",
                        string(to_zh(
                            result.palaces[xx.gong_index]
                                .gong_data.gong_wei
                        ))
                    }
                };
            };

        auto liu_nian_to_json =
            [&](const LiuNianData& ln) {
                return json{
                    {"year", ln.year},
                    {
                        "gan",
                        string(
                            GanZhi::Mapper::to_zh(
                                ln.tian_gan
                            )
                        )
                    },
                    {
                        "zhi",
                        string(
                            GanZhi::Mapper::to_zh(
                                ln.di_zhi
                            )
                        )
                    },
                    {"gong_index", ln.gong_index},
                    {
                        "gong_name",
                        string(to_zh(
                            result.palaces[ln.gong_index]
                                .gong_data.gong_wei
                        ))
                    },
                    {"si_hua", si_hua_to_json(ln.si_hua)}
                };
            };

        auto liu_yue_to_json =
            [&](const LiuYueData& ly) {
                return json{
                    {"lunar_month", ly.month},
                    {
                        "gan",
                        string(
                            GanZhi::Mapper::to_zh(
                                ly.tian_gan
                            )
                        )
                    },
                    {
                        "zhi",
                        string(
                            GanZhi::Mapper::to_zh(
                                ly.di_zhi
                            )
                        )
                    },
                    {"gong_index", ly.gong_index},
                    {
                        "gong_name",
                        string(to_zh(
                            result.palaces[ly.gong_index]
                                .gong_data.gong_wei
                        ))
                    },
                    {"si_hua", si_hua_to_json(ly.si_hua)}
                };
            };

        auto liu_ri_to_json =
            [&](const LiuRiData& lr) {
                return json{
                    {"lunar_day", lr.day},
                    {
                        "gan",
                        string(
                            GanZhi::Mapper::to_zh(
                                lr.tian_gan
                            )
                        )
                    },
                    {
                        "zhi",
                        string(
                            GanZhi::Mapper::to_zh(
                                lr.di_zhi
                            )
                        )
                    },
                    {"gong_index", lr.gong_index},
                    {
                        "gong_name",
                        string(to_zh(
                            result.palaces[lr.gong_index]
                                .gong_data.gong_wei
                        ))
                    },
                    {"si_hua", si_hua_to_json(lr.si_hua)}
                };
            };

        auto liu_shi_to_json =
            [&](const LiuShiData& ls) {
                return json{
                    {
                        "shi_chen",
                        string(
                            GanZhi::Mapper::to_zh(
                                ls.shi_chen
                            )
                        )
                    },
                    {
                        "gan",
                        string(
                            GanZhi::Mapper::to_zh(
                                ls.tian_gan
                            )
                        )
                    },
                    {
                        "zhi",
                        string(
                            GanZhi::Mapper::to_zh(
                                ls.di_zhi
                            )
                        )
                    },
                    {"gong_index", ls.gong_index},
                    {
                        "gong_name",
                        string(to_zh(
                            result.palaces[ls.gong_index]
                                .gong_data.gong_wei
                        ))
                    },
                    {"si_hua", si_hua_to_json(ls.si_hua)}
                };
            };

        auto is_gregorian_leap_year =
            [](int year) {
                return
                    (year % 4 == 0 && year % 100 != 0) ||
                    (year % 400 == 0);
            };

        auto days_in_month =
            [&](int year, int month) {
                static constexpr int normal_days[12] = {
                    31, 28, 31, 30, 31, 30,
                    31, 31, 30, 31, 30, 31
                };

                if (
                    month == 2 &&
                    is_gregorian_leap_year(year)
                ) {
                    return 29;
                }

                return normal_days[month - 1];
            };

        // ----------------------------------------------------
        // 时间轴
        // ----------------------------------------------------
        root["timeline"] = json::array();

        const int birth_lunar_year =
            result.lunar_day.get_year();

        const int start_year =
            center_year - options.years_before;

        const int end_year =
            center_year + options.years_after;

        for (
            int year = start_year;
            year <= end_year;
            ++year
        ) {
            json year_node;
            year_node["solar_year"] = year;
            year_node["days"] = json::array();

            for (int month = 1; month <= 12; ++month) {
                const int dim =
                    days_in_month(year, month);

                for (int day = 1; day <= dim; ++day) {
                    auto solar_day =
                        tyme::SolarDay::from_ymd(
                            year,
                            month,
                            day
                        );

                    auto lunar_day =
                        solar_day.get_lunar_day();

                    int virtual_age =
                        calculate_virtual_age(
                            birth_lunar_year,
                            lunar_day.get_year(),
                            options.virtual_age_boundary_policy
                        );

                    json day_node = {
                        {
                            "solar_date",
                            solar_day.to_string()
                        },
                        {
                            "lunar_date",
                            lunar_day.to_string()
                        },
                        {
                            "virtual_age_at_00",
                            virtual_age
                        }
                    };

                    if (virtual_age <= 0) {
                        day_node["status"] =
                            "before_birth";

                        year_node["days"].push_back(
                            day_node
                        );

                        continue;
                    }

                    // 00:00 作为该公历日的基础六层上下文。
                    auto base =
                        result.get_horoscope(
                            year,
                            month,
                            day,
                            0,
                            virtual_age,
                            options.year_boundary_policy,
                            options.month_gan_zhi_policy,
                            options.day_boundary_policy
                        );

                    day_node["da_xian"] =
                        da_xian_to_json(
                            base.da_xian
                        );

                    day_node["xiao_xian"] =
                        xiao_xian_to_json(
                            base.xiao_xian
                        );

                    day_node["liu_nian"] =
                        liu_nian_to_json(
                            base.liu_nian
                        );

                    day_node["liu_yue"] =
                        liu_yue_to_json(
                            base.liu_yue
                        );

                    day_node["liu_ri"] =
                        liu_ri_to_json(
                            base.liu_ri
                        );

                    day_node["dynamic_stars"] = {
                        {
                            "da_xian",
                            dynamic_stars_to_json(
                                base.da_xian_stars
                            )
                        },
                        {
                            "liu_nian",
                            dynamic_stars_to_json(
                                base.liu_nian_stars
                            )
                        },
                        {
                            "liu_yue",
                            json::array()
                        },
                        {
                            "liu_ri",
                            json::array()
                        },
                        {
                            "liu_shi",
                            json::array()
                        }
                    };

                    if (options.include_hours) {
                        day_node["hours"] =
                            json::array();

                        // 按24个公历小时保留完整边界语义。
                        for (
                            int hour = 0;
                            hour < 24;
                            ++hour
                        ) {
                            auto effective_day =
                                (
                                    options.day_boundary_policy ==
                                        ZiHourDayBoundaryPolicy::LateZi &&
                                    hour == 23
                                )
                                    ? solar_day.next(1)
                                    : solar_day;

                            int hour_virtual_age =
                                calculate_virtual_age(
                                    birth_lunar_year,
                                    effective_day
                                        .get_lunar_day()
                                        .get_year(),
                                    options
                                        .virtual_age_boundary_policy
                                );

                            if (hour_virtual_age <= 0) {
                                continue;
                            }

                            auto hs =
                                result.get_horoscope(
                                    year,
                                    month,
                                    day,
                                    hour,
                                    hour_virtual_age,
                                    options
                                        .year_boundary_policy,
                                    options
                                        .month_gan_zhi_policy,
                                    options
                                        .day_boundary_policy
                                );

                            json hour_node = {
                                {"hour", hour},
                                {
                                    "effective_solar_date",
                                    effective_day.to_string()
                                },
                                {
                                    "virtual_age",
                                    hour_virtual_age
                                },
                                {
                                    "liu_shi",
                                    liu_shi_to_json(
                                        hs.liu_shi
                                    )
                                }
                            };

                            // 23:00 晚子可能跨越流年/月/日边界。
                            // 只在发生真实换日时额外保存完整上下文，
                            // 避免其余23小时重复大量数据。
                            if (
                                options.day_boundary_policy ==
                                    ZiHourDayBoundaryPolicy::LateZi &&
                                hour == 23
                            ) {
                                hour_node["late_zi_context"] = {
                                    {
                                        "liu_nian",
                                        liu_nian_to_json(
                                            hs.liu_nian
                                        )
                                    },
                                    {
                                        "liu_yue",
                                        liu_yue_to_json(
                                            hs.liu_yue
                                        )
                                    },
                                    {
                                        "liu_ri",
                                        liu_ri_to_json(
                                            hs.liu_ri
                                        )
                                    }
                                };
                            }

                            day_node["hours"].push_back(
                                hour_node
                            );
                        }
                    }

                    year_node["days"].push_back(
                        day_node
                    );
                }
            }

            root["timeline"].push_back(
                year_node
            );
        }

        return root.dump(options.json_indent);
    }

} // namespace ZhouYi::ZiWei

