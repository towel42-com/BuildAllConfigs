// The MIT License( MIT )
//
// Copyright( c ) 2022 Scott Aron Bloom
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files( the "Software" ), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sub-license, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include <iostream>
#include <conio.h>

#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <iomanip>

template< typename T >
void generateAllSubsetsPermutations( std::ostream &oss, const std::vector< T > &items )
{
    std::size_t totalCount = 1ULL << items.size();
    auto updateVal = std::min( totalCount / 10, 100ULL );
    if ( updateVal == 0 )
        updateVal = 1;

    std::cout << "Computing all permutations: (" << totalCount << ")\n";

    oss << R"__(#!/bin/bash

logFile=buildAllConfigs.log

buildConfig() {
    local configName=$1
    local currConfigNum=$2
    local totalConfigs=$3
    local options=${@:4}

    local localLogFile=${configName}/${configName}.log

    echo "===========================================" | tee -a ${logFile}
    echo "Building configuration \"$configName\" (${currConfigNum} of ${totalConfigs})" | tee -a ${logFile}
    rm -rf ${configName} |& tee -a ${logFile} || return 1
    mkdir -p $configName |& tee -a ${logFile} || return 1

    cmake -S . -B ${configName} -Wno-dev -DTOWEL42_CMAKEUTILS_DIR=../T42-CMakeUtils/ -DCMAKE_CXX_COMPILER=cl -DCMAKE_C_COMPILER=cl -DCMAKE_LINKER_TYPE=MSVC $options |& tee -a ${logFile} > ${localLogFile} || return 1
    echo "    CMake ran successfully" | tee -a ${logFile} ${localLogFile}

    cmake --build ${configName} |& tee -a ${logFile} > ${localLogFile}
    status=${PIPESTATUS[0]}
    echo "===========================================" | tee -a ${logFile} ${localLogFile}
    if [[ $status == 0 ]]; then
        echo "    BUILD: PASSED" | tee -a ${logFile} ${localLogFile}
    else
        echo "    BUILD: FAILED" | tee -a ${logFile} ${localLogFile}
    fi
    echo "===========================================" | tee -a ${logFile} ${localLogFile}
    
    return 0
}

rm -rf ${logFile}

)__";

    for ( auto ii = 0ULL; ii < totalCount; ++ii )
    {
        if ( ( ii % updateVal ) == 0 )
            std::cout << ii << " of " << totalCount << "\n";

        std::ostringstream os;
        os << "build_config_" << std::setw( 3 ) << std::setfill( '0' ) << ii;
        auto configName = os.str();

        oss << "buildConfig " << configName << " " << ii << " " << totalCount;
        for ( auto jj = items.size() - 1 ; jj >= 0; --jj )
        {
            oss << " -D" << items[ items.size() - 1 - jj ] << "=";
            if ( ( ( ii >> jj ) & 0x01 ) != 0 )
                oss << "ON ";
            else
                oss << "OFF";
            if ( jj == 0 )
                break;
        }
        oss << "\n";
        if ( ii == 1 )
            oss << "exit\n";
    }
}

void usage()
{
    std::cout << R"__(buildAllConfigs -script <filename> config1 [config2...]
      -script <filename>: Filename to generate (defaults to stdout if not set)
    config1 [config2...]: Configurations to generate from required, no default
)__";
}

int main( int argc, char **argv )
{
    std::string scriptFile;
    std::vector< std::string > configs;

#ifdef _DEBUG
    configs = {
        //
        "TOWEL42_BIFSUPPORT",   //
        "TOWEL42_GIFSUPPORT",   //
        "TOWEL42_DESIGNERPLUGIN_SUPPORT",   //
        "TOWEL42_ZIP_SUPPORT",   //
        "TOWEL42_ENABLE_TESTING",   //
        "TOWEL42_QAXOBJECT_SUPPORT",   //
        "TOWEL42_QCONCURRENT_SUPPORT",   //
        "TOWEL42_QCORE_SUPPORT",   //
        "TOWEL42_QNETWORK_SUPPORT",   //
        "TOWEL42_QSQL_SUPPORT",   //
        "TOWEL42_QSVG_SUPPORT",   //
        "TOWEL42_QWIDGETS_SUPPORT",   //
        "TOWEL42_QXML_SUPPORT"   //
    };
#endif

    for ( int ii = 1; ii < argc; ++ii )
    {
        if ( std::string( argv[ ii ] ) == "--script" )
        {
            if ( ii == ( argc - 1 ) )
                usage();
            else
            {
                scriptFile = argv[ ++ii ];
            }
        }
        else
        {
#ifdef _DEBUG
            if ( ii == 3 )
                configs.clear();
#endif
            configs.emplace_back( argv[ ii ] );
        }
    }

    std::ostream *oss = &std::cout;
    std::ofstream ofs;
    if ( !scriptFile.empty() )
    {
        ofs.open( scriptFile );
        if ( !ofs.is_open() )
        {
            std::cerr << "Could not open script file: '" << scriptFile << "'\n";
            return -1;
        }
        oss = &ofs;
    }

    generateAllSubsetsPermutations< std::string >(
        *oss,   //
        configs );
    if ( !scriptFile.empty() )
        std::cout << "Finished creating script '" << scriptFile << "'\n";
    return 0;
}
