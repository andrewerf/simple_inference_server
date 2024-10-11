#include <drogon/drogon.h>
#include <fmt/format.h>
#include <boost/program_options.hpp>
#include <spdlog/spdlog.h>
#include <fstream>

#include "src/Controller.h"


using namespace drogon;


// Fix parsing std::filesystem::path with spaces (see https://github.com/boostorg/program_options/issues/69)
namespace boost
{
template <>
inline std::filesystem::path lexical_cast<std::filesystem::path, std::basic_string<char>>(const std::basic_string<char> &arg)
{
    return std::filesystem::path(arg);
}
}
namespace po = boost::program_options;

int main( int argc, char** argv )
{
    std::string host;
    std::filesystem::path certPath, keyPath;
    int port;
    Controller::Config config;

    po::options_description desc( "Simple Inference Server" );
    desc.add_options()
        ( "help,h", "Display help message" )
        ( "invokePath", po::value( &config.invokePath )->required(), "Path to the script that will be invoked" )
        ( "indexPath", po::value( &config.indexPath )->default_value( {} ), "Path to the index.html" )
        ( "host", po::value( &host )->default_value( "127.0.0.1" ), "Host to bind to" )
        ( "port", po::value( &port )->default_value( 7654 ), "Port to bind to" )
        ( "cert", po::value( &certPath )->default_value( {} ), "Path to SSL certificate" )
        ( "key", po::value( &keyPath )->default_value( {} ), "Path to SSL key" )
    ;

    po::variables_map vm;
    try
    {
        po::store( po::parse_command_line( argc, argv, desc ), vm );
        if ( vm.count( "help" ) )
        {
            std::cout << desc << std::endl;
            return 0;
        }

        po::notify( vm );
    }
    catch ( po::error& err )
    {
        std::cerr << "Failed to parse arguments: " << err.what() << std::endl;
        return -1;
    }

    const bool useSSL = exists( certPath ) && exists( keyPath );
    if ( useSSL && !app().supportSSL() )
    {
        spdlog::error( "Current configuration does not support SSL" );
        return -1;
    }

    if ( useSSL )
        spdlog::info( "Use SSL" );
    else
        spdlog::info( "Do not use SSL" );

    spdlog::default_logger()->flush();
    trantor::Logger::enableSpdLog( spdlog::default_logger() );

    app()
        .setClientMaxBodySize( 1024*1024*1024 ) // 1gb
        .addListener( host, port, useSSL, certPath, keyPath )
        .registerController( std::make_shared<Controller>( config ) )
        .run();

    return 0;
}
