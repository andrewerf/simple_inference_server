#include <uuid.h>
#include <boost/lockfree/queue.hpp>
#include <drogon/drogon.h>

#include <spdlog/spdlog.h>

using namespace drogon;



struct GlobalConfig
{
    std::filesystem::path invokePath;
} globalConfig;

bool invokeProcessing( const std::filesystem::path& input, const std::filesystem::path& output )
{
    auto exitCode = system( fmt::format( "{} {} {}", globalConfig.invokePath.string(), input.string(), output.string() ).c_str() );
    return exitCode == 0;
//    std::this_thread::sleep_for( std::chrono::seconds( 10 ) );
//    system( fmt::format( "cp {} {}", input.string(), output.string() ).c_str() );
//    return true;
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


int main()
{
    globalConfig.invokePath = "/home/andrew/Projects/Adalisk/cbct-teeth-segmentation/web/invoke.sh";

    app().registerHandler( "/track_result", std::function{ trackResultHandler } );
    app().registerHandler( "/get_result", std::function{ getResultHandler } );
    app().registerHandler( "/", std::function{ submitTask }, { Post } );
    app()
        .setClientMaxBodySize( 1024*1024*1024 )
        .addListener( "127.0.0.1", 7654 )
        .run();


    return 0;
}
