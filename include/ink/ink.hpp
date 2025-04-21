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
#include <ink/EnhancedJson.h>
#include <ink/EnhancedJsonUtils.h>
#include <ink/Inkogger.h>
#include <ink/InkAssert.h>
#include <ink/InkException.h>
#include <ink/LastWish.h>
#include <ink/ArgParser.h>
#include <ink/InkedList.h>
#include <ink/Queue.h>
#include <ink/RingBuffer.h>
#include <ink/ThreadPool.h>
#include <ink/String.h>
#include <ink/utils.h>


/*====================
 * INK DEFS
 *====================*/
#define INK_CHECK_RESULT(result_code, msg)     \
if (result_code != static_cast<ink_i32>(ink_result_t::SUCCESS)) \
    throw ink::InkException(result, msg)  \

#endif
