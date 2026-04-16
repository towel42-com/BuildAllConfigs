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

void generateAllConfigs( std::ostream &oss, const std::vector< std::string > &configs, bool ninja )
{
    std::size_t totalCount = 1ULL << configs.size();
    auto updateVal = std::min( totalCount / 10, 100ULL );
    if ( updateVal == 0 )
        updateVal = 1;

    std::cout << "Computing all permutations: (" << totalCount << ")\n";

    std::string header = R"__(#!/bin/bash

logFile=buildAllConfigs.log
totalNum=)__" + std::to_string( totalCount )
                         + R"__(
currentConfigNum=0
passed=()
failed=()

buildConfig() {
    local configName=$1
    local options=${@:2}

    local localLogFile=${configName}/${configName}.log

    echo "===========================================" | tee -a ${logFile}
    echo "Building configuration \"$configName\" (${currentConfigNum} of ${totalNum} Passed: ${#passed[@]} Failed: ${#failed[@]})" | tee -a ${logFile}
    rm -rf ${configName} |& tee -a ${logFile} || return 1
    mkdir -p $configName |& tee -a ${logFile} || return 1

    currentConfigNum=$(($currentConfigNum + 1))
    
    cmake -S . -B ${configName} -Wno-dev)__"
                         + ( ninja ? R"__(-G "Ninja Multi-Config -DCMAKE_CXX_COMPILER=cl -DCMAKE_C_COMPILER=cl -DCMAKE_LINKER_TYPE=MSVC)__" 
                                   : "" ) +
    R"__( -DTOWEL42_CMAKEUTILS_DIR=../T42-CMakeUtils/ $options |& tee -a ${logFile} > ${localLogFile}
    status=${PIPESTATUS[0]}
    if [[ $status == 0 ]]; then
        echo "    CMake ran successfully" | tee -a ${logFile} ${localLogFile}
    else
        echo "    BUILD: FAILED" | tee -a ${logFile} ${localLogFile}
        failed+=(${configName})
        return 1
    fi

    cmake --build ${configName} |& tee -a ${logFile} > ${localLogFile}
    status=${PIPESTATUS[0]}
    if [[ $status == 0 ]]; then
        echo "    BUILD: PASSED" | tee -a ${logFile} ${localLogFile}
        passed+=(${configName})
    else
        echo "    BUILD: FAILED" | tee -a ${logFile} ${localLogFile}
        failed+=(${configName})
    fi
    
    return 0
}

reportSummary() {
    echo "===========================================" | tee -a ${logFile}
    echo "Summary:"
    echo "Number of Configurations Run: ${currConfigNum}"
    echo "                         Passed: ${#passed[@]}"
    echo "                         Failed: ${#failed[@]}"
    echo "Failed Configurations:"
    for config in "${failed[@]}"; do
        echo "    $config"
    done
}

rm -rf ${logFile}

)__";

    oss << header;
    for ( auto ii = 0ULL; ii < totalCount; ++ii )
    {
        if ( ( ii % updateVal ) == 0 )
            std::cout << ii << " of " << totalCount << "\n";

        std::ostringstream os;
        os << "build_config_" << std::setw( 3 ) << std::setfill( '0' ) << ii;
        auto configName = os.str();

        oss << "buildConfig " << configName;
        for ( auto jj = configs.size() - 1; jj >= 0; --jj )
        {
            oss << " -D" << configs[ configs.size() - 1 - jj ] << "=";
            if ( ( ( ii >> jj ) & 0x01 ) != 0 )
                oss << "ON ";
            else
                oss << "OFF";
            if ( jj == 0 )
                break;
        }
        oss << "\n";
        if ( ii == 1 )
            oss << "reportSummary\nexit\n";
    }
    oss << "reportSummary\nexit\n";
}

void usage()
{
    std::cout << R"__(buildAllConfigs -script <filename> [-ninja] config1 [config2...]
      -script <filename>: Filename to generate (defaults to stdout if not set)
      -ninja: Use the Ninja Multi-Config cmake configuration (default false, uses the default for the system)"
    config1 [config2...]: Configurations to generate from required, no default
)__";
}

int main( int argc, char **argv )
{
    std::string scriptFile;
    bool ninja = false;
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
    bool firstConfig = true;

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
        else if ( std::string( argv[ ii ] ) == "--ninja" )
        {
            ninja = true;
        }
        else
        {
            if ( firstConfig )
                configs.clear();
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

    generateAllConfigs( *oss, configs, ninja );
    if ( !scriptFile.empty() )
        std::cout << "Finished creating script '" << scriptFile << "'\n";
    return 0;
}
