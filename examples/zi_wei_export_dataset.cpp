import std;
import ZhouYi.ZiWei;
import ZhouYi.ZiWei.Controller;
import ZhouYi.ZiWei.Constants;

using namespace std;
using namespace ZhouYi::ZiWei;

namespace {

bool parse_gender(const string& value) {
    if (value == "male" || value == "m" || value == "男") {
        return true;
    }

    if (value == "female" || value == "f" || value == "女") {
        return false;
    }

    throw invalid_argument(
        "性别必须是 male/m/男 或 female/f/女"
    );
}

int parse_int(
    const string& text,
    const string& field_name
) {
    size_t used = 0;
    int value = stoi(text, &used);

    if (used != text.size()) {
        throw invalid_argument(
            field_name + " 不是有效整数: " + text
        );
    }

    return value;
}

LeapMonthPolicy parse_leap_policy(
    const string& value
) {
    if (value == "same") {
        return LeapMonthPolicy::SameAsRegularMonth;
    }

    if (value == "next") {
        return LeapMonthPolicy::NextMonth;
    }

    if (value == "split15") {
        return LeapMonthPolicy::SplitAtDay15;
    }

    throw invalid_argument(
        "--leap-policy 必须是 same / next / split15"
    );
}

ZiHourDayBoundaryPolicy parse_day_boundary(
    const string& value,
    const string& option_name
) {
    if (value == "midnight") {
        return ZiHourDayBoundaryPolicy::Midnight;
    }

    if (value == "latezi") {
        return ZiHourDayBoundaryPolicy::LateZi;
    }

    throw invalid_argument(
        option_name +
        " 必须是 midnight 或 latezi"
    );
}

void print_usage(const char* program) {
    cerr
        << "用法:\n\n"

        << "阳历出生：\n  "
        << program
        << " solar <年> <月> <日> <小时0-23>"
        << " <male|female> <中心年> <输出JSON> [选项]\n\n"

        << "农历出生：\n  "
        << program
        << " lunar <年> <月> <日> <小时0-23>"
        << " <male|female> <中心年> <输出JSON> [选项]\n\n"

        << "通用选项：\n"
        << "  --before N\n"
        << "      中心年前 N 年，默认 5\n"
        << "  --after N\n"
        << "      中心年后 N 年，默认 8\n"
        << "  --year-boundary lny|lichun\n"
        << "      流年换年边界\n"
        << "  --month-policy lunar|solar-term\n"
        << "      流月干支来源\n"
        << "  --day-boundary midnight|latezi\n"
        << "      时间轴流日换日策略\n"
        << "  --natal-day-boundary midnight|latezi\n"
        << "      出生本命晚子换日策略\n"
        << "  --compact\n"
        << "      紧凑 JSON\n\n"

        << "农历出生专用选项：\n"
        << "  --leap\n"
        << "      出生月份为闰月\n"
        << "  --leap-policy same|next|split15\n"
        << "      same    : 闰月按本月\n"
        << "      next    : 闰月按下月\n"
        << "      split15 : 初一至十五按本月，十六起按下月\n\n"

        << "示例：\n\n"

        << "  阳历：\n  "
        << program
        << " solar 1990 6 15 10 male 2026 out.json"
        << " --day-boundary latezi --compact\n\n"

        << "  农历闰月：\n  "
        << program
        << " lunar 2020 4 20 23 female 2026 out.json"
        << " --leap --leap-policy split15"
        << " --natal-day-boundary latezi"
        << " --day-boundary latezi --compact\n";
}

} // namespace

int main(int argc, char* argv[]) {
    try {
        if (argc < 9) {
            print_usage(argv[0]);
            return 2;
        }

        const string calendar_type = argv[1];

        if (
            calendar_type != "solar" &&
            calendar_type != "lunar"
        ) {
            throw invalid_argument(
                "第一个参数必须是 solar 或 lunar"
            );
        }

        const int birth_year =
            parse_int(argv[2], "出生年");

        const int birth_month =
            parse_int(argv[3], "出生月");

        const int birth_day =
            parse_int(argv[4], "出生日");

        const int birth_hour =
            parse_int(argv[5], "出生小时");

        const bool is_male =
            parse_gender(argv[6]);

        const int center_year =
            parse_int(argv[7], "中心年");

        const filesystem::path output_path =
            argv[8];

        if (birth_hour < 0 || birth_hour > 23) {
            throw invalid_argument(
                "出生小时必须在 0~23 之间"
            );
        }

        AnalysisDatasetOptions options;

        bool is_leap_month = false;

        LeapMonthPolicy leap_month_policy =
            LeapMonthPolicy::SameAsRegularMonth;

        ZiHourDayBoundaryPolicy natal_day_boundary =
            ZiHourDayBoundaryPolicy::Midnight;

        for (int i = 9; i < argc; ++i) {
            string arg = argv[i];

            auto require_value =
                [&](const string& option) -> string {
                    if (i + 1 >= argc) {
                        throw invalid_argument(
                            option + " 缺少参数"
                        );
                    }

                    return argv[++i];
                };

            if (arg == "--before") {
                options.years_before =
                    parse_int(
                        require_value(arg),
                        "--before"
                    );

            } else if (arg == "--after") {
                options.years_after =
                    parse_int(
                        require_value(arg),
                        "--after"
                    );

            } else if (arg == "--year-boundary") {
                string value = require_value(arg);

                if (value == "lny") {
                    options.year_boundary_policy =
                        LiuNianYearBoundaryPolicy::
                            LunarNewYear;

                } else if (value == "lichun") {
                    options.year_boundary_policy =
                        LiuNianYearBoundaryPolicy::
                            LiChun;

                } else {
                    throw invalid_argument(
                        "--year-boundary 必须是 "
                        "lny 或 lichun"
                    );
                }

            } else if (arg == "--month-policy") {
                string value = require_value(arg);

                if (value == "lunar") {
                    options.month_gan_zhi_policy =
                        LiuYueGanZhiPolicy::
                            LunarMonthWuHuDun;

                } else if (value == "solar-term") {
                    options.month_gan_zhi_policy =
                        LiuYueGanZhiPolicy::
                            SolarTermMonthPillar;

                } else {
                    throw invalid_argument(
                        "--month-policy 必须是 "
                        "lunar 或 solar-term"
                    );
                }

            } else if (arg == "--day-boundary") {
                options.day_boundary_policy =
                    parse_day_boundary(
                        require_value(arg),
                        "--day-boundary"
                    );

            } else if (arg == "--natal-day-boundary") {
                natal_day_boundary =
                    parse_day_boundary(
                        require_value(arg),
                        "--natal-day-boundary"
                    );

            } else if (arg == "--leap") {
                is_leap_month = true;

            } else if (arg == "--leap-policy") {
                leap_month_policy =
                    parse_leap_policy(
                        require_value(arg)
                    );

            } else if (arg == "--compact") {
                options.json_indent = -1;

            } else {
                throw invalid_argument(
                    "未知参数: " + arg
                );
            }
        }

        if (
            options.years_before < 0 ||
            options.years_after < 0
        ) {
            throw invalid_argument(
                "--before / --after 不能为负数"
            );
        }

        if (
            calendar_type == "solar" &&
            is_leap_month
        ) {
            throw invalid_argument(
                "--leap 只能用于 lunar 农历出生"
            );
        }

        if (
            calendar_type == "solar" &&
            leap_month_policy !=
                LeapMonthPolicy::SameAsRegularMonth
        ) {
            throw invalid_argument(
                "--leap-policy 只能用于 lunar 农历出生"
            );
        }

        options.virtual_age_boundary_policy =
            VirtualAgeBoundaryPolicy::LunarNewYear;

        options.include_hours = true;

        cout
            << "正在排本命盘...\n"
            << "历法: "
            << (calendar_type == "solar"
                    ? "阳历"
                    : "农历")
            << "\n";

        ZiWeiResult result =
            calendar_type == "solar"
                ? pai_pan_solar(
                    birth_year,
                    birth_month,
                    birth_day,
                    birth_hour,
                    is_male,
                    leap_month_policy,
                    natal_day_boundary
                )
                : pai_pan_lunar(
                    birth_year,
                    birth_month,
                    birth_day,
                    birth_hour,
                    is_male,
                    is_leap_month,
                    leap_month_policy,
                    natal_day_boundary
                );

        const int start_year =
            center_year - options.years_before;

        const int end_year =
            center_year + options.years_after;

        cout
            << "正在生成紫微全量数据集...\n"
            << "年份: "
            << start_year
            << " ～ "
            << end_year
            << "\n"
            << "出生闰月策略: "
            << (
                leap_month_policy ==
                    LeapMonthPolicy::SameAsRegularMonth
                    ? "same"
                    : leap_month_policy ==
                        LeapMonthPolicy::NextMonth
                        ? "next"
                        : "split15"
            )
            << "\n"
            << "出生换日: "
            << (
                natal_day_boundary ==
                    ZiHourDayBoundaryPolicy::Midnight
                    ? "midnight"
                    : "latezi"
            )
            << "\n"
            << "时间轴换日: "
            << (
                options.day_boundary_policy ==
                    ZiHourDayBoundaryPolicy::Midnight
                    ? "midnight"
                    : "latezi"
            )
            << "\n";

        string dataset =
            export_analysis_dataset(
                result,
                center_year,
                options
            );

        if (
            output_path.has_parent_path() &&
            !output_path.parent_path().empty()
        ) {
            filesystem::create_directories(
                output_path.parent_path()
            );
        }

        ofstream out(
            output_path,
            ios::binary | ios::trunc
        );

        if (!out) {
            throw runtime_error(
                "无法创建输出文件: " +
                output_path.string()
            );
        }

        out.write(
            dataset.data(),
            static_cast<streamsize>(
                dataset.size()
            )
        );

        if (!out) {
            throw runtime_error(
                "写入失败: " +
                output_path.string()
            );
        }

        out.close();

        cout
            << "✅ 导出完成\n"
            << "文件: "
            << output_path.string()
            << "\n"
            << "大小: "
            << filesystem::file_size(
                output_path
            )
            << " bytes\n";

        return 0;

    } catch (const exception& e) {
        cerr
            << "❌ "
            << e.what()
            << '\n';

        return 1;
    }
}
