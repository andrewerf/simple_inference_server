//
// Created by Andrey Aralov on 9/23/24.
//
#pragma once

#include "Job.h"

#include <unordered_map>
#include <expected>
#include <shared_mutex>

#include <drogon/drogon.h>


class Controller : public drogon::HttpController<Controller, false>
{
public:
    struct Config
    {
        std::filesystem::path invokePath;
        std::filesystem::path indexPath;
    };

    struct Error
    {
        drogon::HttpStatusCode code;
        std::string msg;

        operator drogon::HttpResponsePtr() const;
    };

    template <typename T>
    using ExpectedOrError = std::expected<T, Error>;

private:
    std::unordered_map<JobId, Job> jobs_;
    std::shared_mutex jobsMutex_;
    Config config_;

public:

    explicit Controller( const Config& config );


    METHOD_LIST_BEGIN
    ADD_METHOD_TO( Controller::submit, "/submit", drogon::Post );
    ADD_METHOD_TO( Controller::getInfo, "/get_job_info", drogon::Get );
    ADD_METHOD_TO( Controller::getResult, "/get_result", drogon::Get );

    ADD_METHOD_TO( Controller::submit_api, "/api/submit", drogon::Post );
    ADD_METHOD_TO( Controller::getInfo_api, "/api/get_job_info", drogon::Get );
    ADD_METHOD_TO( Controller::getResult, "/api/get_result", drogon::Get );
    METHOD_LIST_END

    drogon::Task<drogon::HttpResponsePtr> submit( drogon::HttpRequestPtr req );
    drogon::Task<drogon::HttpResponsePtr> getInfo( drogon::HttpRequestPtr req );
    drogon::Task<drogon::HttpResponsePtr> getResult( drogon::HttpRequestPtr req );

    drogon::Task<drogon::HttpResponsePtr> submit_api( drogon::HttpRequestPtr req );
    drogon::Task<drogon::HttpResponsePtr> getInfo_api( drogon::HttpRequestPtr req );

protected:
    ExpectedOrError<JobId> submit_( drogon::HttpRequestPtr req );
    ExpectedOrError<Job> getInfo_( drogon::HttpRequestPtr req );
    drogon::HttpResponsePtr getResult_( drogon::HttpRequestPtr req );
};


