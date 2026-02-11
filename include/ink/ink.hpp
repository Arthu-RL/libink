/**
 * @file ink.hpp
 * @brief Main header file for INK library
 *
 * This file contains common definitions, type definitions, and includes all INK components.
 */

#ifndef INK_HPP
#define INK_HPP

#pragma once


/*====================
 * INCLUDE MODULES
 *====================*/
#include <ink/AlignedAllocator.h>
#include <ink/ArenaAllocator.h>
#include <ink/EnhancedJson.h>
#include <ink/EnhancedJsonUtils.h>
#include <ink/Inkogger.h>
#include <ink/InkAssert.h>
#include <ink/InkException.h>
#include <ink/InkixTree.h>
#include <ink/InkType.h>
#include <ink/InkOtp.h>
#include <ink/LastWish.h>
#include <ink/ArgParser.h>
#include <ink/InkedList.h>
#include <ink/Queue.h>
#include <ink/RingBuffer.h>
#include <ink/TimerWheel.h>
#include <ink/ThreadPool.h>
#include <ink/String.h>
#include <ink/utils.h>
#include <ink/WorkerThread.h>


/*====================
 * VERSION INFO
 *====================*/
#define INK_MAJOR_VERSION 1
#define INK_MINOR_VERSION 1
#define INK_PATCH_VERSION 0
#define INK_VERSION ((INK_MAJOR_VERSION * 10000) + (INK_MINOR_VERSION * 100) + INK_PATCH_VERSION)
#define INK_VERSION_STRING_FULL INK_STR(INK_MAJOR_VERSION) "." INK_STR(INK_MINOR_VERSION) "." INK_STR(INK_PATCH_VERSION)


/*====================
 * INK DEFS
 *====================*/
#define INK_CHECK_RESULT(result_code, msg)     \
if (result_code != static_cast<i32>(ink_result_t::SUCCESS)) \
    throw ink::InkException(result, msg)  \

#endif
