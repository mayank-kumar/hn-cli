/**
 * @file storage.h
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

#include <fstream>
#include <memory>
#include <string>
#include <unordered_set>
#include "story.h"


namespace hackernewscmd {
	class Storage {
	private:
		struct Key{}; // Used to provide controlled access to ctor

	public:
		Storage(std::string&&, const Key&);
		~Storage();

		std::unordered_set<StoryId>* GetSkippedStoryIds();

		static Storage& GetInstance();
	private:
		std::string mFilepath;
		std::unordered_set<StoryId> mSkippedStoryIds;

		void ReadSkippedStoryIds();
		void WriteSkippedStoryIds();

		static std::unique_ptr<Storage> mInstance;
		static const std::string kFilename;
	};
} // namespace hackernewscmd