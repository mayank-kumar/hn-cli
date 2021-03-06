/**
 * @file story.h
 *
 * Copyright 2015 Mayank Kumar
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#pragma once

#include <atomic>
#include <functional>
#include <string>
#include <utility>


namespace hackernewscmd {
	enum class StoryLoadStatus {
		NotStarted,
		Started,
		Failed,
		Completed
	}; // enum class StoryLoadStatus

	using StoryId = unsigned long long;

	struct Story {
		StoryId id = 0;
		std::wstring title;
		std::wstring url;
		unsigned score;
		unsigned descendants;
		time_t time;
		std::wstring by;
	}; // struct Story

	struct StoryStatus {
		std::atomic<StoryLoadStatus> loadStatus;
		bool isSkipped;

		StoryStatus() :
			loadStatus(StoryLoadStatus::NotStarted),
			isSkipped(false) {};

		StoryStatus(const StoryStatus& other) :
			loadStatus(other.loadStatus.load()),
			isSkipped(other.isSkipped) {};
	};

	using StoryAndStatus = std::pair < Story, StoryStatus >;
} // namespace hackernewscmd
