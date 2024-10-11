//
// Created by Andrey Aralov on 9/23/24.
//
#include "Controller.h"

#include <fmt/format.h>

#include <json/json.h>

#include <magic_enum.hpp>


using namespace drogon;


namespace
{

bool
    invokeProcessing( const std::filesystem::path& invokePath,
                      const std::filesystem::path& input,
                      const std::filesystem::path& output )
{
    auto exitCode = system( fmt::format( "{} {} {}", invokePath.string(), input.string(), output.string() ).c_str() );
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

}


Controller::Error::operator HttpResponsePtr() const
{
    auto resp = HttpResponse::newHttpResponse();
    resp->setBody( msg );
    resp->setStatusCode( code );
    return resp;
}


Controller::Controller( const Config& config ):
    config_( config )
{}

Controller::ExpectedOrError<JobId> Controller::submit_( drogon::HttpRequestPtr req )
{
    MultiPartParser filesUpload;
    if ( filesUpload.parse(req) != 0 || filesUpload.getFiles().size() != 1 )
        return std::unexpected<Error>{ { k403Forbidden, "Must be only one file" } };

    auto file = filesUpload.getFiles()[0];
    if ( !file.getFileName().ends_with( ".zip" ) )
        return std::unexpected<Error>{ { k403Forbidden, "Must be a zip file" } };

    const auto jobId = drogon::utils::getUuid();
    file.saveAs( getInputFileName( jobId ) );

    app().getLoop()->queueInLoop( [jobId, this]
    {
        {
            Job job{
                .status = Job::InProgress,
                .id = jobId
            };
            std::lock_guard lock( jobsMutex_ );
            jobs_[jobId] = job;
        }

        const auto inputPath = getInputPath( jobId );
        const auto outputPath = getOutputPath( jobId );
        auto result = invokeProcessing( config_.invokePath, inputPath, outputPath );

        {
            std::shared_lock lock( jobsMutex_ );
            if ( result )
                jobs_[jobId].status = Job::Success;
            else
                jobs_[jobId].status = Job::Failed;
        }
    } );

    return jobId;
}

Controller::ExpectedOrError<Job> Controller::getInfo_( drogon::HttpRequestPtr req )
{
    const auto maybeJob
        = req->getOptionalParameter<std::string>( "job_id" )
        .and_then( [this] ( auto jobId ) -> std::optional<Job>
        {
            std::shared_lock lock( jobsMutex_ );
            auto it = jobs_.find( jobId );
            if ( it != jobs_.end() )
                return it->second;
            else
                return std::nullopt;
        } );

    if ( !maybeJob )
        return std::unexpected<Error>{ { k404NotFound, "Job is not found" } };

    return *maybeJob;
}

drogon::HttpResponsePtr Controller::getResult_( drogon::HttpRequestPtr req )
{
    const auto maybeTask = req->getOptionalParameter<std::string>( "job_id" );
    const auto maybeOutput = maybeTask.transform( getOutputPath );
    if ( !maybeOutput || !std::filesystem::exists( *maybeOutput ) )
    {
        return HttpResponse::newNotFoundResponse();
    }

    auto resp = HttpResponse::newFileResponse( *maybeOutput, maybeOutput->filename() );
    return resp;
}


Task<HttpResponsePtr> Controller::submit( HttpRequestPtr req )
{
    if ( auto res = submit_( req ) )
    {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode( HttpStatusCode::k202Accepted );
        resp->setBody( fmt::format( "Your requested has been accepted. You can track it <a href=\"./get_job_info?job_id={}\">here</a>",
                                    *res ) );
        co_return resp;
    }
    else
    {
        co_return res.error();
    }
}

Task<HttpResponsePtr> Controller::getInfo( HttpRequestPtr req )
{
    if ( auto res = getInfo_( req ) )
    {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode( k200OK );

        switch ( res->status )
        {
            case Job::InProgress:
                resp->setBody( "The job is running" );
                break;
            case Job::Failed:
                resp->setBody( "The job is failed" );
                break;
            case Job::Success:
                resp->setBody( fmt::format( "The job is completed. Result can be downloaded <a href=\"./get_result?job_id={}\">here</a>",
                                            res->id ) );
                break;
        }

        co_return resp;
    }
    else
    {
        co_return res.error();
    }
}

Task<HttpResponsePtr> Controller::getResult( HttpRequestPtr req )
{
    co_return getResult_( req );
}


Task<HttpResponsePtr> Controller::submit_api( HttpRequestPtr req )
{
    if ( auto res = submit_( req ) )
    {
        Json::Value root;
        root["job_id"] = *res;
        std::stringstream ss;
        ss << root;

        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode( HttpStatusCode::k202Accepted );
        resp->setBody( ss.str() );
        co_return resp;
    }
    else
    {
        co_return res.error();
    }
}


Task<HttpResponsePtr> Controller::getInfo_api( HttpRequestPtr req )
{
    if ( auto res = getInfo_( req ) )
    {
        Json::Value root;
        root["status"] = std::string( magic_enum::enum_name( res->status ) );
        std::stringstream ss;
        ss << root;

        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode( k200OK );
        resp->addHeader( "Content-Type", "application/json" );
        resp->setBody( ss.str() );
        co_return resp;
    }
    else
    {
        co_return res.error();
    }
}
