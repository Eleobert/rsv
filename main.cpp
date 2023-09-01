#include "rsv.hpp"

#include <cstdlib>
#include <system_error>
#include <vector>
#include <iostream>


int main()
{
    auto sequence  = std::vector<std::string>();
    auto charge    = std::vector<int>();
    auto ascesions = std::vector<std::string>();
    auto score     = std::vector<float>();

    auto [file, econd] = rsv::open("sample.tsv");

    if(econd)
    {
        std::cerr << econd.message() << '\n';
        std::exit(1);
    }

    auto schema = rsv::schema({
        rsv::f("charge"   , charge),
        rsv::f("ascesions", ascesions),
        rsv::f("sequence" , sequence),
        rsv::f("score"    , score)}
    );

    auto report = rsv::read(file, schema, '\t');

    auto cols = rsv::columns(file, '\t');
    
    for(auto e: cols)
    {
        std::cout << e << '\t';
    }

    std::cout << '\n';

    for(size_t i = 0; i < sequence.size(); i++)
    {
        std::cout << sequence[i] << '\t' 
                  << charge[i] << '\t'
                  << ascesions[i] << '\t'
                  << score[i] << '\n';
    }

    if(report.empty())
    {
        std::cout << "no problems while reading the file\n";
    }
    else
    {
        for(auto rec: report)
        {
            std::cout << "error at line " << rec.index << " column " << schema[rec.pos].name << "\n";
        }
    }
}