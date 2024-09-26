//
// Created by Andrey Aralov on 9/22/24.
//
#pragma once
#include <string>


using JobId = std::string;

struct Job
{
    enum Status
    {
        InProgress,
        Success,
        Failed
    };

    Status status;
    JobId id;
};