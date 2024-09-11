#include <boost/lockfree/queue.hpp>
#include <drogon/drogon.h>
#include <fmt/format.h>
#include <boost/program_options.hpp>


using namespace drogon;


struct GlobalConfig
{
    std::filesystem::path invokePath;
} globalConfig;

bool invokeProcessing( const std::filesystem::path& input, const std::filesystem::path& output )
{
    auto exitCode = system( fmt::format( "{} {} {}", globalConfig.invokePath.string(), input.string(), output.string() ).c_str() );
    return exitCode == 0;
}

std::string getInputFileName( const std::string& taskId )
{
    return taskId + ".input.zip";
}
std::filesystem::path getInputPath( const std::string& taskId )
{
    return std::filesystem::path{ app().getUploadPath() } / getInputFileName( taskId );
}
std::filesystem::path getOutputPath( const std::string& taskId )
{
    return std::filesystem::path{ app().getUploadPath() } / ( taskId + ".zip" );
}


Task<HttpResponsePtr> trackResultHandler( HttpRequestPtr req )
{
    const auto maybeTask = req->getOptionalParameter<std::string>( "task" );
    const auto maybeOutput = maybeTask.transform( getOutputPath );

    if ( !maybeOutput || !std::filesystem::exists( *maybeOutput ) )
    {
        auto resp = HttpResponse::newHttpResponse();
        resp->setBody( "The task is not available (yet)" );
        resp->setStatusCode( HttpStatusCode::k200OK );
        resp->addHeader( "Refresh", "10" );
        co_return resp;
    }

    auto resp = HttpResponse::newHttpResponse();
        resp->setBody( fmt::format( "Result can be downloaded <a href=\"./get_result?task={}\">here</a>", *maybeTask ) );
    resp->setStatusCode( HttpStatusCode::k200OK );
    co_return resp;
}

Task<HttpResponsePtr> getResultHandler( HttpRequestPtr req )
{
    const auto maybeTask = req->getOptionalParameter<std::string>( "task" );
    const auto maybeOutput = maybeTask.transform( getOutputPath );
    if ( !maybeOutput || !std::filesystem::exists( *maybeOutput ) )
    {
        co_return HttpResponse::newNotFoundResponse();
    }

    auto resp = HttpResponse::newFileResponse( *maybeOutput, maybeOutput->filename() );
    co_return resp;
}

Task<HttpResponsePtr> submitTask( HttpRequestPtr req )
{
    MultiPartParser filesUpload;
    if ( filesUpload.parse(req) != 0 || filesUpload.getFiles().size() != 1 )
    {
        auto resp = HttpResponse::newHttpResponse();
        resp->setBody( "Must only be one file" );
        resp->setStatusCode( k403Forbidden );
        co_return resp;
    }

    auto file = filesUpload.getFiles()[0];
    if ( !file.getFileName().ends_with( ".zip" ) )
    {
        auto resp = HttpResponse::newHttpResponse();
        resp->setBody( "Must be zip file" );
        resp->setStatusCode( k403Forbidden );
        co_return resp;
    }

    const auto taskId = drogon::utils::getUuid();
    file.saveAs( getInputFileName( taskId ) );

    app().getLoop()->queueInLoop( [taskId]
    {
        const auto inputPath = getInputPath( taskId );
        const auto outputPath = getOutputPath( taskId );
        invokeProcessing( inputPath, outputPath );
    } );

    auto resp = HttpResponse::newHttpResponse();
    resp->setStatusCode( HttpStatusCode::k202Accepted );
    resp->setBody( fmt::format( "Your requested has been accepted. You can track it <a href=\"./track_result?task={}\">here</a>",
                                taskId ) );
    co_return resp;
}

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
    int port;

    po::options_description desc( "Simple Inference Server" );
    desc.add_options()
        ( "help,h", "Display help message" )
        ( "invokePath", po::value( &globalConfig.invokePath )->required(), "Path to the script that will be invoked" )
        ( "host", po::value( &host )->default_value( "127.0.0.1" ), "Host to bind to" )
        ( "port", po::value( &port )->default_value( 7654 ), "Port to bind to" )
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

    app().registerHandler( "/track_result", std::function{ trackResultHandler } );
    app().registerHandler( "/get_result", std::function{ getResultHandler } );
    app().registerHandler( "/", std::function{ submitTask }, { Post } );
    app()
        .setClientMaxBodySize( 1024*1024*1024 ) // 1gb
        .addListener( host, port )
        .run();

    return 0;
}
