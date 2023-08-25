#include "rsv.hpp"

#include <algorithm>
#include <cassert>
#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <system_error>
#include <vector>
#include <iostream>
#include <fstream>
#include <ranges>



// FIXME: this is too slow
auto find_positions(const std::vector<std::string>& column_names, const std::vector<rsv::internal::field>& fields)
{
    auto pos   = std::vector<int64_t>();
    auto found = std::vector<int8_t>(fields.size());

    for (const auto& cname: column_names)
    {
        pos.push_back(-1);
        
        for(size_t i = 0; i < fields.size(); i++)
        {
            if(fields[i].pos == (std::ssize(pos) - 1) ||
              (fields[i].pos < 0 && (std::ssize(column_names) + fields[i].pos == (std::ssize(pos) - 1))) ||
               fields[i].name == cname)
            {
                pos.back() = i;
                found[i]   = 1;
                break;
            }
        }
    }

    if(auto it = std::ranges::find(found, 0); it != found.end())
    {
        auto i = it - found.begin();
        // The programmer should not let this happen. Use rsv::columns to
        // inspect columns.
        std::cerr << "rsv: column '" << fields[i].name << "' not present\n";
        std::abort(); 
    }

    return pos;
}

auto next_sep(const char* beg, const char* end, char sep)
{
    if(beg >= end)
    {
        return end;
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


auto read_row(std::ifstream& file, std::string& row)
{
    auto on_quote = false;

    do
    {
        auto buffer = std::string();
        std::getline(file, buffer);

        if(not row.empty())
            row += '\n';

        for(auto c: buffer)
        {
            if(c == '\"')
                on_quote = !on_quote;
        }
        row += buffer;
    }
    while(on_quote);

    return row;
}


namespace rsv
{
    /**
    * Why not simply use ifstream.open?
    * : Because I plan to experiment with mapped file, so
    *   I will have to use custom streams.
    */
    auto open(const std::string& name) -> std::tuple<std::ifstream, std::error_condition>
    {
        std::ifstream file(name);
        auto err = std::error_condition();
        if(not file.good())
        {
            err = std::system_category().default_error_condition(errno);
        }
        return {std::move(file), err};
    }

    auto columns(std::ifstream& file, char sep) -> std::vector<std::string>
    {
        file.seekg(0);
        assert(file.good());
        auto line = std::string();
        std::getline(file, line);

        auto column_names = std::vector<std::string>();

        for (const auto& column_name: std::views::split(line, sep))
        {
            column_names.emplace_back(column_name.begin(), column_name.end());
        }
        file.seekg(0);
        return column_names;
    }

    auto read(std::ifstream& file, const std::vector<internal::field>& sch, char sep) -> void
    {
        file.seekg(0);
        assert(file.good());
        auto pos  = find_positions(columns(file, sep), sch);
        auto line = std::string();
        // skip first line
        std::getline(file, line);

        while(not file.eof())
        {
            auto line  = std::string();
            read_row(file, line);
            auto count = int64_t(0);

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

                count++;

                // if we reach the end of line but still have fields to process
                if(count != std::ssize(sch) and end == line.data() + line.size())
                {
                    std::cerr << "missing fields!!\n";
                    std::exit(1);
                }
            }
        }
    }
};
