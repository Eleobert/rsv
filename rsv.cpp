#include "rsv.hpp"

#include <cstdint>
#include <vector>
#include <iostream>
#include <fstream>
#include <ranges>


// TODO: file should be open outside, so that the user can better manage opening errors
auto open(const std::string& name)
{
    std::ifstream file(name);

    if(not file.good())
    {
        std::perror(name.c_str());
    }
    return file;
}

auto find_positions(const std::string& line, const std::vector<rsv::internal::field>& fields, char sep)
{
    auto pos = std::vector<int64_t>();

    for (const auto& column_name: std::views::split(line, sep))
    {
        const auto cname = std::string_view(column_name.begin(), column_name.end());
        pos.push_back(-1);
        
        for(size_t i = 0; i < fields.size(); i++)
        {
            if(fields[i].pos == (std::ssize(pos) - 1) || fields[i].name == cname)
            {
                pos.back() = i;
                break;
            }
        }
    }
    return pos;
}

auto next_sep(const char* beg, const char* end, char sep)
{
    if(beg >= end)
    {
        std::cerr << "missing value!\n";
        std::exit(1);
    }

    bool on_quote = false;

    while(beg < end && (*beg != sep || on_quote))
    {
        if(*beg == '\"')
            on_quote = !on_quote;
        beg++;
    }
    
    
    return beg;
}

namespace rsv
{
    auto schema(std::vector<internal::field> fields) -> std::vector<internal::field>
    {
        // do nothing
        return fields;
    }

    // TODO: probably it is better to receive schema as positional argument
    auto read(const std::string& filename, const std::vector<internal::field>& sch, char sep) -> void
    {
        auto file = open(filename);
        auto line = std::string();
        std::getline(file, line);
        auto pos  = find_positions(line, sch, sep);

        while(not file.eof())
        {
            auto line = std::string();
            std::getline(file, line);

            if(line.empty())
                continue;

            const auto* beg  = line.data();

            for(size_t i = 0; i < pos.size(); i++)
            {
                auto p = pos[i];

                if(p < 0)
                    continue;

                auto end  = next_sep(beg, line.data() + line.size(), sep);
                auto view = std::string_view(beg, end);
                sch[p].del(view, sch[p].data);
                beg = end + 1;
            }
        }
    }
};
